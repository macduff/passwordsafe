/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#include "PWSfileV3.h"
#include "UUIDGen.h"
#include "PWSrand.h"
#include "Util.h"
#include "SysInfo.h"
#include "PWScore.h"
#include "PWSFilters.h"
#include "PWSdirs.h"
#include "PWSprefs.h"
#include "core.h"
#include "return_codes.h"

#include "os/debug.h"
#include "os/file.h"

#include "XML/XMLDefs.h"  // Required if testing "USE_XML_LIBRARY"

#ifdef _WIN32
#include <io.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <iomanip>

using namespace std;

static unsigned char TERMINAL_BLOCK[TwoFish::BLOCKSIZE] = {
  'P', 'W', 'S', '3', '-', 'E', 'O', 'F',
  'P', 'W', 'S', '3', '-', 'E', 'O', 'F'};

PWSfileV3::PWSfileV3(const StringX &filename, RWmode mode, VERSION version)
  : PWSfile(filename, mode)
{
  m_curversion = version;
  m_IV = m_ipthing;
  m_terminal = TERMINAL_BLOCK;
}

PWSfileV3::~PWSfileV3()
{
}

int PWSfileV3::Open(const StringX &passkey)
{
  int status = PWSRC::SUCCESS;

  ASSERT(m_curversion == V30);
  if (passkey.empty()) { // Can happen if db 'locked'
    pws_os::Trace(_T("PWSfileV3::Open(empty_passkey)\n"));
    return PWSRC::WRONG_PASSWORD;
  }
  m_passkey = passkey;

  FOpen();
  if (m_fd == NULL)
    return PWSRC::CANT_OPEN_FILE;

  if (m_rw == Write) {
    status = WriteHeader();
  } else { // open for read
    status = ReadHeader();
    if (status != PWSRC::SUCCESS) {
      Close();
      return status;
    }
  }
  return status;
}

int PWSfileV3::Close()
{
  if (m_fd == NULL)
    return PWSRC::SUCCESS; // idempotent

  unsigned char digest[HMAC_SHA256::HASHLEN];
  m_hmac.Final(digest);

  // Write or verify HMAC, depending on RWmode.
  if (m_rw == Write) {
    size_t fret;
    fret = fwrite(TERMINAL_BLOCK, sizeof(TERMINAL_BLOCK), 1, m_fd);
    if (fret != 1) {
      PWSfile::Close();
      return PWSRC::FAILURE;
    }
    fret = fwrite(digest, sizeof(digest), 1, m_fd);
    if (fret != 1) {
      PWSfile::Close();
      return PWSRC::FAILURE;
    }
    return PWSfile::Close();
  } else { // Read
    // We're here *after* TERMINAL_BLOCK has been read
    // and detected (by _readcbc) - just read hmac & verify
    unsigned char d[HMAC_SHA256::HASHLEN];
    fread(d, sizeof(d), 1, m_fd);
    if (memcmp(d, digest, HMAC_SHA256::HASHLEN) == 0)
      return PWSfile::Close();
    else {
      PWSfile::Close();
      return PWSRC::BAD_DIGEST;
    }
  }
}

const char V3TAG[4] = {'P','W','S','3'}; // ASCII chars, not wchar

int PWSfileV3::CheckPasskey(const StringX &filename,
                            const StringX &passkey, FILE *a_fd,
                            unsigned char *aPtag, int *nITER)
{
  FILE *fd = a_fd;
  int retval = PWSRC::SUCCESS;
  SHA256 H;

  if (fd == NULL) {
    fd = pws_os::FOpen(filename.c_str(), _T("rb"));
  }
  if (fd == NULL)
    return PWSRC::CANT_OPEN_FILE;

  char tag[sizeof(V3TAG)];
  fread(tag, 1, sizeof(tag), fd);
  if (memcmp(tag, V3TAG, sizeof(tag)) != 0) {
    retval = PWSRC::NOT_PWS3_FILE;
    goto err;
  }

  unsigned char salt[SaltLengthV3];
  fread(salt, 1, sizeof(salt), fd);

  unsigned char Nb[sizeof(uint32)];
  fread(Nb, 1, sizeof(Nb), fd);
  { // block to shut up compiler warning w.r.t. goto
    const uint32 N = getInt32(Nb);

    ASSERT(N >= MIN_HASH_ITERATIONS);
    if (N < MIN_HASH_ITERATIONS) {
      retval = PWSRC::FAILURE;
      goto err;
    }

    if (nITER != NULL)
      *nITER = N;
    unsigned char Ptag[SHA256::HASHLEN];
    if (aPtag == NULL)
      aPtag = Ptag;
    
    StretchKey(salt, sizeof(salt), passkey, N, aPtag);
  }
  unsigned char HPtag[SHA256::HASHLEN];
  H.Update(aPtag, SHA256::HASHLEN);
  H.Final(HPtag);
  unsigned char readHPtag[SHA256::HASHLEN];
  fread(readHPtag, 1, sizeof(readHPtag), fd);
  if (memcmp(readHPtag, HPtag, sizeof(readHPtag)) != 0) {
    retval = PWSRC::WRONG_PASSWORD;
    goto err;
  }
err:
  if (a_fd == NULL) // if we opened the file, we close it...
    fclose(fd);
  return retval;
}

size_t PWSfileV3::WriteCBC(unsigned char type, const StringX &data)
{
  bool status;
  const unsigned char *utf8;
  size_t utf8Len;
  status = m_utf8conv.ToUTF8(data, utf8, utf8Len);
  if (!status)
    pws_os::Trace(_T("ToUTF8(%s) failed\n"), data.c_str());

  return WriteCBC(type, utf8, utf8Len);
}

size_t PWSfileV3::WriteCBC(unsigned char type, const unsigned char *data,
                           size_t length)
{
  m_hmac.Update(data, reinterpret_cast<int &>(length));
  return PWSfile::WriteCBC(type, data, length);
}

int PWSfileV3::WriteRecord(const CItemData &item)
{
  ASSERT(m_fd != NULL);
  ASSERT(m_curversion == V30);
  int status = PWSRC::SUCCESS;
  StringX tmp;
  uuid_array_t item_uuid;
  time_t t = 0;
  int32 i32;
  short i16;

  item.GetUUID(item_uuid);
  WriteCBC(CItemData::UUID, item_uuid, sizeof(uuid_array_t));
  tmp = item.GetGroup();
  if (!tmp.empty())
    WriteCBC(CItemData::GROUP, tmp);
  WriteCBC(CItemData::TITLE, item.GetTitle());
  WriteCBC(CItemData::USER, item.GetUser());
  WriteCBC(CItemData::PASSWORD, item.GetPassword());
  tmp = item.GetNotes();
  if (!tmp.empty())
    WriteCBC(CItemData::NOTES, tmp);
  tmp = item.GetURL();
  if (!tmp.empty())
    WriteCBC(CItemData::URL, tmp);
  tmp = item.GetAutoType();
  if (!tmp.empty())
    WriteCBC(CItemData::AUTOTYPE, tmp);
  item.GetCTime(t);
  if (t != 0) {
    i32 = static_cast<int>(t);
    WriteCBC(CItemData::CTIME, reinterpret_cast<unsigned char *>(&i32), sizeof(int32));
  }
  item.GetPMTime(t);
  if (t != 0) {
    i32 = static_cast<int>(t);
    WriteCBC(CItemData::PMTIME, reinterpret_cast<unsigned char *>(&i32), sizeof(int32));
  }
  item.GetATime(t);
  if (t != 0) {
    i32 = static_cast<int>(t);
    WriteCBC(CItemData::ATIME, reinterpret_cast<unsigned char *>(&i32), sizeof(int32));
  }
  item.GetXTime(t);
  if (t != 0) {
    i32 = static_cast<int>(t);
    WriteCBC(CItemData::XTIME, reinterpret_cast<unsigned char *>(&i32), sizeof(int32));
  }
  item.GetXTimeInt(i32);
  if (i32 > 0 && i32 <= 3650) {
    WriteCBC(CItemData::XTIME_INT, reinterpret_cast<unsigned char *>(&i32), sizeof(int32));
  }
  item.GetRMTime(t);
  if (t != 0) {
    i32 = static_cast<int>(t);
    WriteCBC(CItemData::RMTIME, reinterpret_cast<unsigned char *>(&i32), sizeof(int32));
  }
  tmp = item.GetPWPolicy();
  if (!tmp.empty())
    WriteCBC(CItemData::POLICY, tmp);
  tmp = item.GetPWHistory();
  if (!tmp.empty())
    WriteCBC(CItemData::PWHIST, tmp);
  tmp = item.GetRunCommand();
  if (!tmp.empty())
    WriteCBC(CItemData::RUNCMD, tmp);
  item.GetDCA(i16);
  if (i16 >= PWSprefs::minDCA && i16 <= PWSprefs::maxDCA)
    WriteCBC(CItemData::DCA, reinterpret_cast<unsigned char *>(&i16), sizeof(short));
  tmp = item.GetEmail();
  if (!tmp.empty())
    WriteCBC(CItemData::EMAIL, tmp);

  UnknownFieldsConstIter vi_IterURFE;
  for (vi_IterURFE = item.GetURFIterBegin();
       vi_IterURFE != item.GetURFIterEnd();
       vi_IterURFE++) {
    unsigned char type;
    size_t length = 0;
    unsigned char *pdata = NULL;
    item.GetUnknownField(type, length, pdata, vi_IterURFE);
    WriteCBC(type, pdata, length);
    trashMemory(pdata, length);
    delete[] pdata;
  }

  WriteCBC(CItemData::END, _T(""));

  return status;
}

size_t PWSfileV3::ReadCBC(unsigned char &type, unsigned char* &data,
                          size_t &length)
{
  size_t numRead = PWSfile::ReadCBC(type, data, length);

  if (numRead > 0) {
    m_hmac.Update(data, reinterpret_cast<unsigned long &>(length));
  }

  return numRead;
}

int PWSfileV3::ReadRecord(CItemData &item)
{
  ASSERT(m_fd != NULL);
  ASSERT(m_curversion == V30);

  int status = PWSRC::SUCCESS;

  signed long numread = 0;
  unsigned char type;

  int emergencyExit = 255; // to avoid endless loop.
  signed long fieldLen; // <= 0 means end of file reached

  do {
    unsigned char *utf8 = NULL;
    size_t utf8Len = 0;
    fieldLen = static_cast<signed long>(ReadCBC(type, utf8,
                                                utf8Len));

    if (fieldLen > 0) {
      numread += fieldLen;
      if (!item.SetField(type, utf8, utf8Len)) {
        status = PWSRC::FAILURE;
        break;
      }
    } // if (fieldLen > 0)

    if (utf8 != NULL) {
      trashMemory(utf8, utf8Len * sizeof(utf8[0]));
      delete[] utf8; utf8 = NULL; utf8Len = 0;
    }
  } while (type != CItemData::END && fieldLen > 0 && --emergencyExit > 0);
  if (numread > 0)
    return status;
  else
    return PWSRC::END_OF_FILE;
}

void PWSfileV3::StretchKey(const unsigned char *salt, unsigned long saltLen,
                           const StringX &passkey,
                           unsigned int N, unsigned char *Ptag)
{
  /*
  * P' is the "stretched key" of the user's passphrase and the SALT, as defined
  * by the hash-function-based key stretching algorithm in
  * http://www.schneier.com/paper-low-entropy.pdf (Section 4.1), with SHA-256
  * as the hash function, and N iterations.
  */
  size_t passLen = 0;
  unsigned char *pstr = NULL;

  ConvertString(passkey, pstr, passLen);
  unsigned char *X = Ptag;
  SHA256 H0;
  H0.Update(pstr, passLen);
  H0.Update(salt, saltLen);
  H0.Final(X);

#ifdef UNICODE
  trashMemory(pstr, passLen);
  delete[] pstr;
#endif

  ASSERT(N >= MIN_HASH_ITERATIONS); // minimal value we're willing to use
  for (unsigned int i = 0; i < N; i++) {
    SHA256 H;
    // The 2nd param in next line was sizeof(X) in Beta-1
    // (bug #1451422). This change broke the ability to read beta-1
    // generated databases. If this is really needed, we should
    // hack the read functionality to try both variants (ugh).
    H.Update(X, SHA256::HASHLEN);
    H.Final(X);
  }
}

const short VersionNum = 0x0307;

// Following specific for PWSfileV3::WriteHeader
#define SAFE_FWRITE(p, sz, cnt, stream) \
  { \
    size_t _ret = fwrite(p, sz, cnt, stream); \
    if (_ret != cnt) { status = PWSRC::FAILURE; goto end;} \
  }

int PWSfileV3::WriteHeader()
{
  // Following code is divided into {} blocks to
  // prevent "uninitialized" compile errors, as we use
  // goto for error handling
  int status = PWSRC::SUCCESS;
  size_t numWritten;
  unsigned char salt[SaltLengthV3];

  // See formatV3.txt for explanation of what's written here and why
  unsigned int NumHashIters;
  if (m_hdr.m_nITER < MIN_HASH_ITERATIONS)
    NumHashIters = MIN_HASH_ITERATIONS;
  else
    NumHashIters = m_hdr.m_nITER;

  SAFE_FWRITE(V3TAG, 1, sizeof(V3TAG), m_fd);

  // According to the spec, salt is just random data. I don't think though,
  // that it's good practice to directly expose the generated randomness
  // to the attacker. Therefore, we'll hash the salt.
  // The following takes shameless advantage of the fact that
  // SaltLengthV3 == SHA256::HASHLEN
  ASSERT(SaltLengthV3 == SHA256::HASHLEN); // if false, have to recode
  { // in a block to protect against goto
    PWSrand::GetInstance()->GetRandomData(salt, sizeof(salt));
    SHA256 salter;
    salter.Update(salt, sizeof(salt));
    salter.Final(salt);
    SAFE_FWRITE(salt, 1, sizeof(salt), m_fd);
  }

  unsigned char Nb[sizeof(NumHashIters)];
  putInt32(Nb, NumHashIters);
  SAFE_FWRITE(Nb, 1, sizeof(Nb), m_fd);

  unsigned char Ptag[SHA256::HASHLEN];

  StretchKey(salt, sizeof(salt), m_passkey, NumHashIters, Ptag);

  { 
    unsigned char HPtag[SHA256::HASHLEN];
    SHA256 H;
    H.Update(Ptag, sizeof(Ptag));
    H.Final(HPtag);
    SAFE_FWRITE(HPtag, 1, sizeof(HPtag), m_fd);
  }
  { 
    PWSrand::GetInstance()->GetRandomData(m_key, sizeof(m_key));
    unsigned char B1B2[sizeof(m_key)];
    unsigned char L[32]; // for HMAC
    ASSERT(sizeof(B1B2) == 32); // Generalize later
    TwoFish TF(Ptag, sizeof(Ptag));
    TF.Encrypt(m_key, B1B2);
    TF.Encrypt(m_key + 16, B1B2 + 16);
    SAFE_FWRITE(B1B2, 1, sizeof(B1B2), m_fd);
    PWSrand::GetInstance()->GetRandomData(L, sizeof(L));
    unsigned char B3B4[sizeof(L)];
    ASSERT(sizeof(B3B4) == 32); // Generalize later
    TF.Encrypt(L, B3B4);
    TF.Encrypt(L + 16, B3B4 + 16);
    SAFE_FWRITE(B3B4, 1, sizeof(B3B4), m_fd);
    m_hmac.Init(L, sizeof(L));
  }
  { 
    // See discussion on Salt to understand why we hash
    // random data instead of writing it directly
    unsigned char ip_rand[SHA256::HASHLEN];
    PWSrand::GetInstance()->GetRandomData(ip_rand, sizeof(ip_rand));
    SHA256 ipHash;
    ipHash.Update(ip_rand, sizeof(ip_rand));
    ipHash.Final(ip_rand);
    ASSERT(sizeof(ip_rand) >= sizeof(m_ipthing)); // compilation assumption
    memcpy(m_ipthing, ip_rand, sizeof(m_ipthing));
  }
  SAFE_FWRITE(m_ipthing, 1, sizeof(m_ipthing), m_fd);

  m_fish = new TwoFish(m_key, sizeof(m_key));

  // write some actual data (at last!)
  numWritten = 0;
  // Write version number
  unsigned char vnb[sizeof(VersionNum)];
  vnb[0] = static_cast<unsigned char>(VersionNum & 0xff);
  vnb[1] = static_cast<unsigned char>((VersionNum & 0xff00) >> 8);
  m_hdr.m_nCurrentMajorVersion = static_cast<unsigned short>((VersionNum & 0xff00) >> 8);
  m_hdr.m_nCurrentMinorVersion = static_cast<unsigned short>(VersionNum & 0xff);

  numWritten = WriteCBC(HDR_VERSION, vnb, sizeof(VersionNum));

  if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }

  // Write UUID
  uuid_array_t file_uuid_array;
  memset(file_uuid_array, 0, sizeof(uuid_array_t));
  // If not there or zeroed, create new
  if (memcmp(m_hdr.m_file_uuid_array,
             file_uuid_array, sizeof(uuid_array_t)) == 0) {
    CUUIDGen uuid;
    uuid.GetUUID(m_hdr.m_file_uuid_array);
  }

  numWritten = WriteCBC(HDR_UUID, m_hdr.m_file_uuid_array,
                        sizeof(uuid_array_t));
  if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }

  // Write (non default) user preferences
  numWritten = WriteCBC(HDR_NDPREFS, m_hdr.m_prefString.c_str());
  if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }

  // Write out display status
  if (!m_hdr.m_displaystatus.empty()) {
    StringX ds(_T(""));
    vector<bool>::const_iterator iter;
    for (iter = m_hdr.m_displaystatus.begin();
         iter != m_hdr.m_displaystatus.end(); iter++)
      ds += (*iter) ? _T("1") : _T("0");
    numWritten = WriteCBC(HDR_DISPSTAT, ds);
    if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }
  }

  // Write out time of this update
  time_t time_now;
  time(&time_now);
  numWritten = WriteCBC(HDR_LASTUPDATETIME,
                        reinterpret_cast<unsigned char *>(&time_now), sizeof(time_t));
  if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }
  m_hdr.m_whenlastsaved = time_now;

  // Write out who saved it!
  {
    const SysInfo *si = SysInfo::GetInstance();
    stringT user = si->GetRealUser();
    stringT sysname = si->GetRealHost();
    numWritten = WriteCBC(HDR_LASTUPDATEUSER, user.c_str());
    if (numWritten > 0)
      numWritten = WriteCBC(HDR_LASTUPDATEHOST, sysname.c_str());
    if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }
    m_hdr.m_lastsavedby = user.c_str();
    m_hdr.m_lastsavedon = sysname.c_str();
  }

  // Write out what saved it!
  numWritten = WriteCBC(HDR_LASTUPDATEAPPLICATION,
                        m_hdr.m_whatlastsaved);
  if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }

  if (!m_hdr.m_dbname.empty()) {
    numWritten = WriteCBC(HDR_DBNAME, m_hdr.m_dbname);
    if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }
  }
  if (!m_hdr.m_dbdesc.empty()) {
    numWritten = WriteCBC(HDR_DBDESC, m_hdr.m_dbdesc);
    if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }
  }
  if (!m_MapFilters.empty()) {
    ostringstream oss;
    m_MapFilters.WriteFilterXMLFile(oss, m_hdr, _T(""));
    numWritten = WriteCBC(HDR_FILTERS,
                          reinterpret_cast<const unsigned char *>(oss.str().c_str()),
                          oss.str().length());
    if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }
  }

  if (!m_hdr.m_RUEList.empty()) {
    ostringstream oss;
    size_t num = m_hdr.m_RUEList.size();
    if (num > 255)
      num = 255;  // Do not exceed 2 hex character length field
    oss << setw(2) << setfill('0') << hex << num;
    UUIDListIter iter = m_hdr.m_RUEList.begin();
    // Only save up to max as defined by FormatV3.
    for (size_t n = 0; n < num; n++) {
      for (size_t i = 0; i < sizeof(uuid_array_t); i++) {
        oss << setw(2) << setfill('0') << hex << int(iter->uuid[i]);
      }
      iter++;
    }

    numWritten = WriteCBC(HDR_RUE, 
                          reinterpret_cast<const unsigned char *>(oss.str().c_str()),
                          oss.str().length());
    if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }
  }

  if (!m_UHFL.empty()) {
    UnknownFieldList::iterator vi_IterUHFE;
    for (vi_IterUHFE = m_UHFL.begin();
         vi_IterUHFE != m_UHFL.end();
         vi_IterUHFE++) {
      UnknownFieldEntry &unkhfe = *vi_IterUHFE;
      numWritten = WriteCBC(unkhfe.uc_Type,
                            unkhfe.uc_pUField, static_cast<unsigned int>(unkhfe.st_length));
      if (numWritten <= 0) { status = PWSRC::FAILURE; goto end; }
    }
  }

  // Write zero-length end-of-record type item
  WriteCBC(HDR_END, NULL, 0);
 end:
  if (status != PWSRC::SUCCESS)
    Close();
  return status;
}

int PWSfileV3::ReadHeader()
{
  unsigned char Ptag[SHA256::HASHLEN];
  int status = CheckPasskey(m_filename, m_passkey, m_fd,
    Ptag, &m_hdr.m_nITER);

  if (status != PWSRC::SUCCESS) {
    Close();
    return status;
  }

  unsigned char B1B2[sizeof(m_key)];
  ASSERT(sizeof(B1B2) == 32); // Generalize later
  fread(B1B2, 1, sizeof(B1B2), m_fd);
  TwoFish TF(Ptag, sizeof(Ptag));
  TF.Decrypt(B1B2, m_key);
  TF.Decrypt(B1B2 + 16, m_key + 16);

  unsigned char L[32]; // for HMAC
  unsigned char B3B4[sizeof(L)];
  ASSERT(sizeof(B3B4) == 32); // Generalize later
  fread(B3B4, 1, sizeof(B3B4), m_fd);
  TF.Decrypt(B3B4, L);
  TF.Decrypt(B3B4 + 16, L + 16);

  m_hmac.Init(L, sizeof(L));

  fread(m_ipthing, 1, sizeof(m_ipthing), m_fd);

  m_fish = new TwoFish(m_key, sizeof(m_key));

  unsigned char fieldType;
  StringX text;
  size_t numRead;
  bool utf8status;
  unsigned char *utf8 = NULL;
  size_t utf8Len = 0;
  bool found0302UserHost = false; // to resolve potential conflicts

  do {
    numRead = ReadCBC(fieldType, utf8, utf8Len);

    if (numRead < 0) {
      Close();
      return PWSRC::FAILURE;
    }

    switch (fieldType) {
      case HDR_VERSION: /* version */
        // in Beta, VersionNum was an int (4 bytes) instead of short (2)
        // This hack keeps bwd compatability.
        if (utf8Len != sizeof(VersionNum) &&
            utf8Len != sizeof(int32)) {
          delete[] utf8;
          Close();
          return PWSRC::FAILURE;
        }
        if (utf8[1] !=
           static_cast<unsigned char>((VersionNum & 0xff00) >> 8)) {
          //major version mismatch
          delete[] utf8;
          Close();
          return PWSRC::UNSUPPORTED_VERSION;
        }
        // for now we assume that minor version changes will
        // be backward-compatible
        m_hdr.m_nCurrentMajorVersion = static_cast<unsigned short>(utf8[1]);
        m_hdr.m_nCurrentMinorVersion = static_cast<unsigned short>(utf8[0]);
        break;

      case HDR_UUID: /* UUID */
        if (utf8Len != sizeof(uuid_array_t)) {
          delete[] utf8;
          Close();
          return PWSRC::FAILURE;
        }
        memcpy(m_hdr.m_file_uuid_array, utf8,
               sizeof(uuid_array_t));
        break;

      case HDR_NDPREFS: /* Non-default user preferences */
        if (utf8Len != 0) {
          if (utf8 != NULL) utf8[utf8Len] = '\0';
          StringX pref;
          utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, pref);
          m_hdr.m_prefString = pref;
          if (!utf8status)
            pws_os::Trace0(_T("FromUTF8(m_prefString) failed\n"));
        } else
          m_hdr.m_prefString = _T("");
        break;

      case HDR_DISPSTAT: /* Tree Display Status */
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        for (StringX::iterator iter = text.begin(); iter != text.end(); iter++) {
          const TCHAR v = *iter;
          m_hdr.m_displaystatus.push_back(v == TCHAR('1'));
        }
        if (!utf8status)
          pws_os::Trace0(_T("FromUTF8(m_displaystatus) failed\n"));
        break;

      case HDR_LASTUPDATETIME: /* When last saved */
        if (utf8Len == 8) {
          // Handle pre-3.09 implementations that mistakenly
          // stored this as a hex value
          if (utf8 != NULL) utf8[utf8Len] = '\0';
            utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
            if (!utf8status)
              pws_os::Trace0(_T("FromUTF8(m_whenlastsaved) failed\n"));
            iStringXStream is(text);
            is >> hex >> m_hdr.m_whenlastsaved;
        } else if (utf8Len == 4) {
          // retrieve time_t
          m_hdr.m_whenlastsaved = *reinterpret_cast< time_t*>(utf8);
        } else {
          m_hdr.m_whenlastsaved = 0;
        }
        break;

      case HDR_LASTUPDATEUSERHOST: /* and by whom */
        // DEPRECATED, but we still know how to read this
        if (!found0302UserHost) { // if new fields also found, don't overwrite
          if (utf8 != NULL) utf8[utf8Len] = '\0';
          utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
          if (utf8status) {
            StringX tlen = text.substr(0, 4);
            iStringXStream is(tlen);
            int ulen = 0;
            is >> hex >> ulen;
            StringX uh = text.substr(4);
            m_hdr.m_lastsavedby = uh.substr(0,ulen);
            m_hdr.m_lastsavedon = uh.substr(ulen);
          } else
            pws_os::Trace0(_T("FromUTF8(m_wholastsaved) failed\n"));
        }
        break;

      case HDR_LASTUPDATEAPPLICATION: /* and by what */
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        m_hdr.m_whatlastsaved = text;
        if (!utf8status)
          pws_os::Trace0(_T("FromUTF8(m_whatlastsaved) failed\n"));
        break;

      case HDR_LASTUPDATEUSER:
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        found0302UserHost = true; // so HDR_LASTUPDATEUSERHOST won't override
        m_hdr.m_lastsavedby = text;
        break;

      case HDR_LASTUPDATEHOST:
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        found0302UserHost = true; // so HDR_LASTUPDATEUSERHOST won't override
        m_hdr.m_lastsavedon = text;
        break;

      case HDR_DBNAME:
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        m_hdr.m_dbname = text;
        break;

      case HDR_DBDESC:
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        m_hdr.m_dbdesc = text;
        break;

#if !defined(USE_XML_LIBRARY) || (!defined(_WIN32) && USE_XML_LIBRARY == MSXML)
      // Don't support importing XML from non-Windows platforms 
      // using Microsoft XML libraries
      // Will be treated as an 'unknown header field' by the 'default' clause below
#else
      case HDR_FILTERS:
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        if (utf8Len > 0) {
          stringT strErrors;
          stringT XSDFilename = PWSdirs::GetXMLDir() + _T("pwsafe_filter.xsd");
#if USE_XML_LIBRARY == MSXML || USE_XML_LIBRARY == XERCES
          if (!pws_os::FileExists(XSDFilename)) {
            // No filter schema => user won't be able to access stored filters
            // Inform her of the fact (probably an installation problem).
              stringT message, message2;
              Format(message, IDSC_MISSINGXSD, _T("pwsafe_filter.xsd"));
              LoadAString(message2, IDSC_FILTERSKEPT);
              message += stringT(_T("\n\n")) + message2;
              if (m_pReporter != NULL)
                (*m_pReporter)(message);

              // Treat it as an Unknown field!
              // Maybe user used a later version of PWS
              // and we don't want to lose anything
             UnknownFieldEntry unkhfe(fieldType, utf8Len, utf8);
             m_UHFL.push_back(unkhfe);
            break;
          }
#endif
          int rc = m_MapFilters.ImportFilterXMLFile(FPOOL_DATABASE, text.c_str(), _T(""),
                                                    XSDFilename.c_str(),
                                                    strErrors, m_pAsker);
          if (rc != PWSRC::SUCCESS) {
            // Can't parse it - treat as an unknown field,
            // Notify user that filter won't be available
            stringT message;
            LoadAString(message, IDSC_CANTPROCESSDBFILTERS);
            if (m_pReporter != NULL)
              (*m_pReporter)(message);
            UnknownFieldEntry unkhfe(fieldType, utf8Len, utf8);
            m_UHFL.push_back(unkhfe);
          }
        }
        break;
#endif

      case HDR_RUE:
      {
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        if (!utf8status)
          break;

        int num(0);
        StringX tlen = text.substr(0, 2);
        iStringXStream is(tlen);
        is >> hex >> num;

        if (text.length() != num * sizeof(uuid_array_t) * 2 + 2)
          break;

        LPCTSTR lpsz_string = text.c_str();
        lpsz_string += 2;
        unsigned char *pfield;
        // sscanf always outputs to an "int" using %x even though
        // target is only 1.  Read into larger buffer to prevent data being
        // overwritten and then copy to where we want it!
        pfield = new unsigned char[sizeof(uuid_array_t) + sizeof(int32)];
        for (int n = 0; n < num; n++) {
          uuid_array_t uuid;
          for (size_t i = 0; i < sizeof(uuid_array_t); i++) {
#if (_MSC_VER >= 1400)
            _stscanf_s(lpsz_string, _T("%02x"), &pfield[i]);
#else
            _stscanf(lpsz_string, _T("%02x"), &pfield[i]);
#endif
            lpsz_string += 2;
          }
          // Now copy only the first 16 characters where we need them
          memcpy(uuid, pfield, sizeof(uuid_array_t));
          m_hdr.m_RUEList.push_back(st_UUID(uuid));
        }
        delete [] pfield;
        break;
      }

      case HDR_END: /* process END so not to treat it as 'unknown' */
        break;

      default:
        // Save unknown fields that may be addded by future versions
        UnknownFieldEntry unkhfe(fieldType, utf8Len, utf8);

        m_UHFL.push_back(unkhfe);
#if 0
#ifdef _DEBUG
        stringT stimestamp;
        PWSUtil::GetTimeStamp(stimestamp);
        pws_os::Trace(_T("Header has unknown field: %02x, length %d/0x%04x, value:\n"), 
                       fieldType, utf8Len, utf8Len);
        pws_os::HexDump(utf8, utf8Len, stimestamp);
#endif
#endif
        break;
    }
    delete[] utf8; utf8 = NULL; utf8Len = 0;
  } while (fieldType != HDR_END);

  return PWSRC::SUCCESS;
}

bool PWSfileV3::IsV3x(const StringX &filename, VERSION &v)
{
  // This is written so as to support V30, V31, V3x...

  ASSERT(pws_os::FileExists(filename.c_str()));
  FILE *fd = pws_os::FOpen(filename.c_str(), _T("rb"));

  ASSERT(fd != NULL);
  char tag[sizeof(V3TAG)];
  fread(tag, 1, sizeof(tag), fd);
  fclose(fd);

  if (memcmp(tag, V3TAG, sizeof(tag)) == 0) {
    v = V30;
    return true;
  } else {
    v = UNKNOWN_VERSION;
    return false;
  }
}
