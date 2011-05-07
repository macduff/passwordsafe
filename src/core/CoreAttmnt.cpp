/*
/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// file CoreAttmnt.cpp
//-----------------------------------------------------------------------------

#include "PWScore.h"
#include "core.h"
#include "Util.h"
#include "SysInfo.h"
#include "UTF8Conv.h"
#include "Report.h"
#include "Match.h"
#include "PWSAttfileV3.h" // XXX cleanup with dynamic_cast
#include "return_codes.h"

#include "XML/XMLDefs.h"  // Required if testing "USE_XML_LIBRARY"

#if USE_XML_LIBRARY == EXPAT
#include "XML/Expat/EAtteXMLProcessor.h"
#elif USE_XML_LIBRARY == MSXML
#include "XML/MSXML/MAttXMLProcessor.h"
#elif USE_XML_LIBRARY == XERCES
#include "XML/Xerces/XAttXMLProcessor.h"
#endif

#include "os/typedefs.h"
#include "os/dir.h"
#include "os/file.h"
#include "os/UUID.h"

#include "zlib/zlib.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <set>

#include <errno.h>

using namespace std;
using pws_os::CUUID;

// hide w_char/char differences where possible:
#ifdef UNICODE
typedef std::wifstream ifstreamT;
typedef std::wofstream ofstreamT;
#else
typedef std::ifstream ifstreamT;
typedef
#endif

// Formula to calculate blocksizes for data processing:
//   min(max(MINBLOCKSIZE, filesize/50), MAXBLOCKSIZE)
//      where MINBLOCKSIZE is 32KB and MAXBLOCKSIZE is 256KB.
// Then to nearest 4KB boundary (2^12)

#define MINBLOCKSIZE  32768
#define MAXBLOCKSIZE 262144

#define GetBlocksize(n) ((min(max(MINBLOCKSIZE, n / 50), MAXBLOCKSIZE) >> 12) << 12)

// uuid_lesser is replace by bool CUUID::operator<(const CUUIDgen &)

// Return whether mulitmap pair uuid is less than the other uuid
// Used in set_difference between 2 multimaps
bool mp_uuid_lesser(ItemMap_Pair p1, ItemMap_Pair p2)
{
  // Assuming first and second point to CItemData

  if (*p1.first.GetUUID() == *p2.first.GetUUID()) {
    return (*p1.second.GetUUID() < *p2.second.GetUUID());
   } else
    return (*p1.first.GetUUID() < *p2.first.GetUUID());
}

struct GetATR {
  GetATR(const CUUID &uuid) : m_attmt_uuid(uuid) {}

  bool operator()(pair<CUUID, ATRecord> p)
  {
    return (p.second.attmt_uuid == m_attmt_uuid);
  }

private:
  const CUUID &m_attmt_uuid;
};

int PWScore::ReadAttachmentFile(bool bVerify)
{
  // Parameter must not be constant as user may cancel verification
  int status;
  stringT attmt_file;
  m_MM_entry_uuid_atr.clear();

  if (m_currfile.empty()) {
    return PWSRC::CANT_OPEN_FILE;
  }

  // Generate attachment file names from database name
  stringT drv, dir, name, ext;

  pws_os::splitpath(m_currfile.c_str(), drv, dir, name, ext);
  attmt_file = drv + dir + name + stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  // If attachment file doesn't exist - OK
  if (!pws_os::FileExists(attmt_file)) {
    pws_os::Trace(_T("No attachment file exists.\n"));
    return PWSRC::SUCCESS;
  }

  PWSAttfile::VERSION version;
  version = PWSAttfile::V30;

  // 'Make' file
  PWSAttfile *in = PWSAttfile::MakePWSfile(attmt_file.c_str(), version,
                                           PWSAttfile::Read, status,
                                           m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS) {
    delete in;
    return status;
  }

  // Open file
  status = in->Open(GetPassKey());
  if (status != PWSRC::SUCCESS) {
    pws_os::Trace(_T("Open attachment file failed RC=%d\n"), status);
    delete in;
    return PWSRC::CANT_OPEN_FILE;
  }

  PWSAttfile::AttHeaderRecord ahr;
  ahr = in->GetHeader();

  if (memcmp(ahr.DBfile_uuid, m_hdr.m_file_uuid_array, sizeof(uuid_array_t)) != 0) {
    pws_os::Trace(_T("Attachment header - database UUID inconsistent.\n"));
    in->Close();
    delete in;
    return PWSRC::HEADERS_INVALID;
  }

  // Now open file and build sets & maps and ensure that they
  // are fully consistent
  // Format of these set/map names are:
  //
  //  tt_vvvvvv_uuid, where
  //    tt     = type: st for set, mp for map
  //    vvvvvv = field value:
  //             'attmt' = UUID of attachment record,
  //             'entry' = UUID of associated entry that has this attachment

  UUIDSetPair pr;               // Used to confirm uniqueness during insert into std::set

  // From file
  UUIDSet st_attmt_uuid;        // std::set on attachment uuid
  ItemMap mp_entry_uuid;        // std::map key = st_attmt_uuid, value = entry_uuid

  // Also from attachment records
  // std::multimap key = entry_uuid, value = attachment record
  UUIDATRMMap mm_entry_uuid_atr;

  ATRecord atr;
  ATTProgress st_atpg;

  st_atpg.function = ATT_PROGRESS_START;
  LoadAString(st_atpg.function_text, IDSC_ATT_READVERIFY);
  // If in verify mode - allow user to stop verification
  st_atpg.value = bVerify ? -1 : 0;
  AttachmentProgress(st_atpg);
  st_atpg.function_text.clear();

  bool go(true), bCancel(false);
  do {
    bool bError(false);

    // Read pre-data information
    status = in->ReadAttmntRecordPreData(atr);

    st_atpg.function = ATT_PROGRESS_PROCESSFILE;
    st_atpg.value = 0;
    st_atpg.atr = atr;
    AttachmentProgress(st_atpg);

    switch (status) {
      case PWSRC::FAILURE:
      {
        // Show a useful (?) error message - better than
        // silently losing data (but not by much)
        // Best if title intact. What to do if not?
        if (m_pReporter != NULL) {
          stringT cs_msg, cs_caption;
          LoadAString(cs_caption, IDSC_READ_ERROR);
          Format(cs_msg, IDSC_ENCODING_PROBLEM, (atr.path + atr.filename).c_str());
          cs_msg = cs_caption + _S(": ") + cs_caption;
          (*m_pReporter)(cs_msg);
        }
        break;
      }
      case PWSRC::SUCCESS:
      {
        pr = st_attmt_uuid.insert(atr.attmt_uuid);
        if (!pr.second) {
          bError = true;
          break;
        }

        unsigned char readtype;
        size_t cmpsize, count;
        unsigned char cdigest[SHA1::HASHLEN];

        // Get SHA1 hash and CRC of compressed data - only need one but..
        // display CRC and user can check with, say, WinZip to see if the same
        SHA1 ccontext;
        cmpsize = 0;
        count = 0;

        // Read all data
        do {
          size_t uiCmpLen;
          unsigned char *pCmpData(NULL);

          status = in->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, !bVerify);
          cmpsize += uiCmpLen;

          // Should be "atr.cmpsize" but we don't know it yet
          count += atr.blksize;
          st_atpg.function = ATT_PROGRESS_PROCESSFILE;
          st_atpg.value = (int)((count * 1.0E02) / atr.uncsize);
          st_atpg.atr = atr;
          // Update progress dialog and check if user cancelled verification
          int rc = AttachmentProgress(st_atpg);
          if ((rc & ATT_PROGRESS_STOPVERIFY) == ATT_PROGRESS_STOPVERIFY) {
            // Cancel verification
            bVerify = false;
            // Update progress dialog window text
            LoadAString(st_atpg.function_text, IDSC_ATT_READFILE);
            st_atpg.value = -1;  // Get Window text updated but not progress bar
            st_atpg.atr = atr;
            AttachmentProgress(st_atpg);
            st_atpg.function_text.clear();
          }
          if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
            // Cancel reading attachment file
            bCancel = true;
            go = false;
          }

          if (bVerify) {
            ccontext.Update(pCmpData, reinterpret_cast<unsigned int &>(uiCmpLen));
          }
          if (pCmpData != 0 && uiCmpLen > 0) {
             trashMemory(pCmpData, uiCmpLen);
             delete [] pCmpData;
             pCmpData = NULL;
          }
         } while (status == PWSRC::SUCCESS && readtype != PWSAttfileV3::ATTMT_LASTDATA);

        // Read post-data
        status = in->ReadAttmntRecordPostData(atr);
        if (bVerify) {
          ccontext.Final(cdigest);
          if (atr.cmpsize != cmpsize ||
              memcmp(atr.cdigest, cdigest, SHA1::HASHLEN) != 0) {
            ASSERT(0);
          }
        }

        st_atpg.function = ATT_PROGRESS_PROCESSFILE;
        st_atpg.value = 100;
        st_atpg.atr = atr;
        AttachmentProgress(st_atpg);

        mp_entry_uuid.insert(ItemMMap_Pair(atr.entry_uuid, atr.attmt_uuid));
        mm_entry_uuid_atr.insert(make_pair(atr.entry_uuid, atr));
        break;
      }
      case PWSRC::END_OF_FILE:
        go = false;
        break;
    } // switch
    if (bError) {
      pws_os::Trace(_T("Duplicate entries found in attachment file and have been ignored.\n"));
    }
  } while (go);

  int closeStatus = in->Close();
  if (bVerify && closeStatus != PWSRC::SUCCESS)
    ASSERT(0);

  delete in;

  if (bCancel) {
    mm_entry_uuid_atr.clear();
    ahr.Clear();
  }

  // All OK - update the entries in PWScore
  m_atthdr = ahr;
  m_MM_entry_uuid_atr = mm_entry_uuid_atr;

  // Terminate thread
  st_atpg.function = ATT_PROGRESS_END;
  AttachmentProgress(st_atpg);

  return bCancel ? PWSRC::USER_CANCEL : PWSRC::SUCCESS;
}

void PWScore::AddAttachments(ATRVector &vNewATRecords)
{
  if (vNewATRecords.empty())
    return;

  // Add attachment record using the DB entry UUID as key
  const CUUID uuid(vNewATRecords[0].entry_uuid);

  for (size_t i = 0; i < vNewATRecords.size(); i++) {
    m_MM_entry_uuid_atr.insert(make_pair(uuid, vNewATRecords[i]));
  }
}

void PWScore::ChangeAttachment(const ATRecord &atr)
{
  // First delete old one
  // Find current entry by getting subset for this DB entry
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(atr.entry_uuid);

  // Verify that it is there!
  if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
      (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
    ASSERT(0);
    return;
  }

  // Now find this specific attachment record
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    if (iter->second.attmt_uuid == atr.attmt_uuid) {
      m_MM_entry_uuid_atr.erase(iter);
      break;
    }
  }

  // Put back in the changed one
  m_MM_entry_uuid_atr.insert(make_pair(atr.entry_uuid, atr));
}

void PWScore::UpdateATRecord(ATRecord &atr)
{
  // First delete old one
  // Find current entry by getting subset for this DB entry
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(atr.entry_uuid);

  // Verify that it is there!
  if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
      (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
    ASSERT(0);
    return;
  }

  // Now find this specific attachment record
  UAMMiter iter;
  for (iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    if (iter->second.attmt_uuid == atr.attmt_uuid)
      break;
  }

  // Put back in the changed one
  if (iter != m_MM_entry_uuid_atr.end())
    atr = iter->second;
}

bool PWScore::MarkAttachmentForDeletion(const ATRecord &atr)
{
  bool bRC(false);
  // Find current entry by getting subset for this DB entry
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(atr.entry_uuid);

  // Verify that it is there!
  if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
      (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
    ASSERT(0);
    return false;
  }

  // Now find this specific attachment record
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    if (iter->second.attmt_uuid == atr.attmt_uuid) {
      iter->second.uiflags |= (ATT_ATTACHMENT_FLGCHGD | ATT_ATTACHMENT_DELETED);
      bRC = true;
      break;
    }
  }
  return bRC;
}

bool PWScore::UnMarkAttachmentForDeletion(const ATRecord &atr)
{
  bool bRC(false);
  // Find current entry by getting subset for this DB entry
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(atr.entry_uuid);

  // Verify that it is there!
  if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
      (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
    ASSERT(0);
    return false;
  }

  // Now find this specific attachment record
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    if (iter->second.attmt_uuid == atr.attmt_uuid) {
      iter->second.uiflags &= ~(ATT_ATTACHMENT_FLGCHGD | ATT_ATTACHMENT_DELETED);
      bRC = true;
      break;
    }
  }
  return bRC;
}

void PWScore::MarkAllAttachmentsForDeletion(const CUUID &entry_uuid)
{
  // Mark all attachment records for this database entry for deletion
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(entry_uuid);

  // Verify that it is there!
  if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
      (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
    ASSERT(0);
    return;
  }

  // Now update all attachment records
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    iter->second.uiflags |= (ATT_ATTACHMENT_FLGCHGD | ATT_ATTACHMENT_DELETED);
  }
}

void PWScore::UnMarkAllAttachmentsForDeletion(const CUUID &entry_uuid)
{
  // UnMark all attachment records for this database entry for deletion
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(entry_uuid);

  // Verify that it is there!
  if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
      (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
    ASSERT(0);
    return;
  }

  // Now update all attachment records
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    iter->second.uiflags &= ~(ATT_ATTACHMENT_FLGCHGD | ATT_ATTACHMENT_DELETED);
  }
}

size_t PWScore::HasAttachments(const uuid_array_t &entry_uuid)
{
  size_t num = m_MM_entry_uuid_atr.count(entry_uuid);
  if (num == 0)
    return 0;

  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(entry_uuid);

  // Now reduce by any marked for deletion
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    if ((iter->second.uiflags & ATT_ATTACHMENT_DELETED) == ATT_ATTACHMENT_DELETED)
      num--;
  }
  return num;
}

int PWScore::GetAttachment(const stringT &newfile, const ATRecord &in_atr, int &zRC)
{
  PWSAttfile *in(NULL);
  PWSAttfile::AttHeaderRecord ahr;
  ATRecord atr;
  size_t cmpsize, uncsize(0);
  SHA1 context;
  FILE *pFile(NULL);

  size_t uiCmpLen(0), uiUncLength(0);
  unsigned char *pUncData(NULL);
  unsigned char *pCmpData(NULL);
  unsigned char odigest[SHA1::HASHLEN];
  unsigned char readtype('\0');
  unsigned long CRC;
  ATTProgress st_atpg;
  int status;
  bool bFound(false), bInflateInit(false), bProgess(false);

  // Initialize the zlib return code just in case we terminate the processing
  // before actually do any inflate
  zRC = Z_OK;

  // Open the file
  pFile = pws_os::FOpen(newfile.c_str(), _T("wb"));

  if (pFile == NULL)
    return PWSRC::CANTCREATEFILE;

  // Get Attachment database name and open it
  stringT attmnt_file;
  stringT drv, dir, name, ext;
  StringX sxFilename;

  pws_os::splitpath(m_currfile.c_str(), drv, dir, name, ext);
  ext = stringT(ATT_DEFAULT_ATTMT_SUFFIX);
  attmnt_file = drv + dir + name + ext;
  sxFilename = attmnt_file.c_str();

  PWSAttfile::VERSION version = PWSAttfile::V30;

  // 'Make' the data file
  in = PWSAttfile::MakePWSfile(sxFilename, version,
                               PWSAttfile::Read, status,
                               m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS)
    goto exit;

  // Open the data file
  status = in->Open(GetPassKey());
  if (status != PWSRC::SUCCESS) {
    status = PWSRC::CANT_OPEN_FILE;
    goto exit;
  }

  st_atpg.function = ATT_PROGRESS_START;
  LoadAString(st_atpg.function_text, IDSC_ATT_SEARCHFILE);
  st_atpg.value = 0;
  AttachmentProgress(st_atpg);
  bProgess = true;

  st_atpg.function_text.clear();
  st_atpg.function = ATT_PROGRESS_SEARCHFILE;
  st_atpg.value = 0;
  AttachmentProgress(st_atpg);

  // Get the header (not sure why?)
  ahr = in->GetHeader();

  // Search for our record
  do {
    cmpsize = 0;
    status = in->ReadAttmntRecordPreData(atr);
    if (status != PWSRC::SUCCESS)
      goto exit;

    // We need the updated data not yet read in during extract
    UpdateATRecord(atr);

    // If ours - return so we can be called again for the data
    if (atr.attmt_uuid == in_atr.attmt_uuid) {
      cmpsize = 0;
      bFound = true;
      break;
    }

    st_atpg.function = ATT_PROGRESS_SEARCHFILE;
    st_atpg.value = 0;
    st_atpg.atr = atr;
    AttachmentProgress(st_atpg);

    // Not ours - read all 'unwanted' data
    size_t uiCmpLen;
    unsigned char *pUnwantedData(NULL);
    readtype = '\0';
    do {
      status = in->ReadAttmntRecordData(pUnwantedData, uiCmpLen, readtype, true);
      cmpsize += uiCmpLen;

      st_atpg.function = ATT_PROGRESS_SEARCHFILE;
      st_atpg.value = (atr.cmpsize > 0) ? (int)((cmpsize * 1.0E02) / atr.cmpsize) : 0;

      st_atpg.atr = atr;
      int rc = AttachmentProgress(st_atpg);
      if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
        status = PWSRC::USER_CANCEL;
        goto exit;
      }
    } while (status == PWSRC::SUCCESS && readtype != PWSAttfileV3::ATTMT_LASTDATA);

    // Read post-data for this attachment
    status = in->ReadAttmntRecordPostData(atr);
  } while (status == PWSRC::SUCCESS);

  if (!bFound) {
    // If we got here - we didn't find the attachment!
    status = PWSRC::CANTFINDATTACHMENT;
    goto exit;
  }

  // Got our record - now get the data, inflate it and save it
  // Set up uncompress environment
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  status = inflateInit(&strm);
  if (status != Z_OK) {
    // Can't set up inflate environment
    status = PWSRC::BADINFLATE;
    goto exit;
  }

  // Indicate inflate environment has been set up
  bInflateInit = true;

  st_atpg.function = ATT_PROGRESS_START;
  LoadAString(st_atpg.function_text, IDSC_ATT_EXTRACTINGFILE);
  st_atpg.value = 0;
  AttachmentProgress(st_atpg);
  st_atpg.function_text.clear();

  st_atpg.function = ATT_PROGRESS_EXTRACTFILE;
  st_atpg.value = 0;
  st_atpg.atr = atr;
  AttachmentProgress(st_atpg);

  // Allocate re-useable uncompressed buffer
  uiUncLength = atr.blksize + 1;
  pUncData = new unsigned char[uiUncLength];

  // Initialise CRC
  PWSUtil::Get_CRC_Incremental_Init();

  // reset readtype
  readtype = '\0';

  do {
    // ReadAttmntRecordData allocates buffer and sets the length
    status = in->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, false);
    if (status != PWSRC::SUCCESS)
      goto exit;

    // Set up zlib stream pointing to new buffer
    strm.avail_in = static_cast<unsigned int>(uiCmpLen);
    strm.next_in = (Bytef *)pCmpData;

    // Keep track of compressed size
    cmpsize += uiCmpLen;
    st_atpg.function = ATT_PROGRESS_EXTRACTFILE;
    st_atpg.value = (in_atr.cmpsize > 0) ? (int)((cmpsize * 1.0E02) / in_atr.cmpsize) : 0;
    st_atpg.atr = atr;
    int rc = AttachmentProgress(st_atpg);
    if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
      status = PWSRC::USER_CANCEL;
      goto exit;
    }

    // Amount inflated as we process the compressed data
    size_t have;
    do {
      strm.avail_out = uiUncLength;
      strm.next_out = (Bytef *)pUncData;

      // Inflate it
      zRC = inflate(&strm, Z_NO_FLUSH);
      ASSERT(zRC != Z_STREAM_ERROR);  /* check state not clobbered */

      switch (zRC) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          status = PWSRC::BADINFLATE;
          goto exit;
      }

      have = uiUncLength - strm.avail_out;
      if (have > 0) {
        uncsize += have;
        if (fwrite(pUncData, 1, have, pFile) != have || ferror(pFile)) {
          status = PWSRC::BADATTACHMENTWRITE;
          goto exit;
        }

        // Now update CRC and SHA1 hash of data retrieved
        PWSUtil::Get_CRC_Incremental_Update(pUncData, have);
        context.Update(pUncData, reinterpret_cast<unsigned int &>(have));
      }
    } while (strm.avail_out == 0);

    // Delete the compressed data buffer ready for next allocation by ReadAttmntRecordData
    trashMemory(pCmpData, uiCmpLen);
    delete [] pCmpData;
    pCmpData = NULL;
  } while (readtype != PWSAttfileV3::ATTMT_LASTDATA);

  // We must have a good status, must have read the last record and
  // inflate must think we have all the data - otherwise we failed.
  if (status != PWSRC::SUCCESS ||
      readtype != PWSAttfile::ATTMT_LASTDATA ||
      zRC != Z_STREAM_END) {
    // Error message to user
    status = PWSRC::BADDATA;
    goto exit;
  }

  // Get the post-data info
  status = in->ReadAttmntRecordPostData(atr);

  // We're done extracting
  st_atpg.function = ATT_PROGRESS_EXTRACTFILE;
  st_atpg.value = 100;
  AttachmentProgress(st_atpg);

  // Get final CRC & uncompressed digest
  CRC = PWSUtil::Get_CRC_Incremental_Final();
  context.Final(odigest);

  // Check we have all the same data
  if (CRC != atr.CRC ||
      memcmp(odigest, atr.odigest, SHA1::HASHLEN) != 0) {
    status = PWSRC::BADCRCDIGEST;
    goto exit;
  }

  // Check we read the same number of bytes (can't see how this can't be true
  // if the CRC and digest already agree!)
  if (uncsize != atr.uncsize || cmpsize != atr.cmpsize) {
    status = PWSRC::BADLENGTH;
    goto exit;
  }

exit:
  // Clear buffer storage
  if (pCmpData != NULL) {
    trashMemory(pCmpData, uiCmpLen);
    delete [] pCmpData;
    pCmpData = NULL;
  }
  if (pUncData != NULL) {
    trashMemory(pUncData, uiUncLength);
    delete [] pUncData;
    pUncData = NULL;
  }

  // Close the attachment database file
  if (in != NULL) {
    in->Close();
    delete in;
    in = NULL;
  }

  // Close output file
  if (pFile != NULL) {
    fflush(pFile);
    fclose(pFile);
    pFile = NULL;
  }

  // Tidy up uncompress environment
  if (bInflateInit)
    inflateEnd(&strm);

  // End the progress dialog
  if (bProgess) {
    st_atpg.function = ATT_PROGRESS_END;
    AttachmentProgress(st_atpg);
  }

  return status;
}

size_t PWScore::GetAttachments(const CUUID &entry_uuid,
                               ATRVector &vATRecords)
{
  vATRecords.clear();

  size_t num = m_MM_entry_uuid_atr.count(entry_uuid);
  if (num == 0)
    return 0;

  std::pair<UAMMciter, UAMMciter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(entry_uuid);
  for (UAMMciter citer = uuidairpair.first; citer != uuidairpair.second; ++citer) {
    if ((citer->second.uiflags & ATT_ATTACHMENT_DELETED) != ATT_ATTACHMENT_DELETED)
      vATRecords.push_back(citer->second);
  }
  return num;
}

size_t PWScore::GetAllAttachments(ATRExVector &vATRecordExs)
{
  // Used by View All Attachments
  vATRecordExs.clear();
  ATRecordEx atrex;
  size_t num = 0;

  for (UAMMciter citer = m_MM_entry_uuid_atr.begin(); citer != m_MM_entry_uuid_atr.end();
       ++citer) {
    if ((citer->second.uiflags & ATT_ATTACHMENT_DELETED) != ATT_ATTACHMENT_DELETED) {
      atrex.Clear();
      atrex.atr = citer->second;
      ItemListIter entry_iter = Find(citer->second.entry_uuid);
      if (entry_iter != GetEntryEndIter()) {
        atrex.sxGroup = entry_iter->second.GetGroup();
        atrex.sxTitle = entry_iter->second.GetTitle();
        atrex.sxUser = entry_iter->second.GetUser();
      } else {
        atrex.sxGroup = _T("?");
        atrex.sxTitle = _T("?");
        atrex.sxUser = _T("?");
      }
      vATRecordExs.push_back(atrex);
      num++;
    }
  }

  return num;
}

void PWScore::SetAttachments(const CUUID &entry_uuid,
                             ATRVector &vATRecords)
{
  // Delete any existing
  m_MM_entry_uuid_atr.erase(entry_uuid);

  // Put back supplied versions
  for (size_t i = 0; i < vATRecords.size(); i++) {
    m_MM_entry_uuid_atr.insert(make_pair(entry_uuid, vATRecords[i]));
  }
}

/**
 * There is a lot in common between the member functions:
 *   WriteAttachmentFile & DuplicateAttachments
 *
 * There needs to be some splitting out of common functions
 */

int PWScore::WriteAttachmentFile(const bool bCleanup, PWSAttfile::VERSION version)
{
  int status;
  /*
    Generate a temporary file name - write to this file.  If OK, rename current
    file and then rename the temporary file to the proper name.
    This does leave the older file around as backup (for the moment).
  */

  // First check if anything to do!
  //   1. Yes - if there are new entries not yet added to attachment file
  //   2. Yes - if there are existing entries with changed flags
  //   3. Yes - if there are existing entries that are no longer required or missing
  // Otherwise No!

  bool bContinue(false);
  for (UAMMciter citer = m_MM_entry_uuid_atr.begin(); citer != m_MM_entry_uuid_atr.end();
       citer++) {
    ATRecord atr = citer->second;
    bool bDelete = ((atr.uiflags & ATT_ATTACHMENT_DELETED) != 0);
    bool bChanged = (atr.uiflags & ATT_ATTACHMENT_FLGCHGD) == ATT_ATTACHMENT_FLGCHGD;

    // 'atr.cmpsize == 0' implies a new attachment; or cleaning up and work to do
    if (atr.cmpsize == 0 || (bCleanup && (bDelete || bChanged))) {
      bContinue = true;
      break;
    }
  }

  if (!bContinue)
    return PWSRC::SUCCESS;

  PWSAttfile *in(NULL), *out(NULL);
  stringT tempfilename, current_file, timestamp;
  stringT sDrive, sDir, sName, sExt;
  time_t time_now;

  // Generate temporary name
  time(&time_now);
  timestamp = PWSUtil::ConvertToDateTimeString(time_now, TMC_FILE).c_str();
  pws_os::splitpath(m_currfile.c_str(), sDrive, sDir, sName, sExt);

  tempfilename = sDrive + sDir + sName + timestamp +
                 stringT(ATT_DEFAULT_ATTMT_SUFFIX);
  current_file = sDrive + sDir + sName +
                 stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  // 'Make' new file
  out = PWSAttfile::MakePWSfile(tempfilename.c_str(), version,
                                PWSAttfile::Write, status,
                                m_pAsker, m_pReporter);
  if (status != PWSRC::SUCCESS) {
    delete out;
    return status;
  }

  // If there is an existing attachment file, 'make' old file
  if (pws_os::FileExists(current_file)) {
    in = PWSAttfile::MakePWSfile(current_file.c_str(), version,
                                 PWSAttfile::Read, status,
                                 m_pAsker, m_pReporter);

    if (status != PWSRC::SUCCESS) {
      delete out;
      return status;
    }
  }

  // XXX cleanup gross dynamic_cast
  PWSAttfileV3 *out3 = dynamic_cast<PWSAttfileV3 *>(out);

  // Set up header records
  SetupAttachmentHeader();

  // Set them - will be written during Open below
  out3->SetHeader(m_atthdr);

  ATRVector vATRWritten;
  ATRecord atr;
  ATTProgress st_atpg;

  try { // exception thrown on write error
    // Open new attachment file
    status = out3->Open(GetPassKey());

    if (status != PWSRC::SUCCESS) {
      delete out3;
      delete in;
      return status;
    }

    // If present, open current data file
    if (in != NULL) {
      status = in->Open(GetPassKey());

      if (status != PWSRC::SUCCESS) {
        out3->Close();
        delete out3;
        delete in;
        return status;
      }
    }

    std::pair<UAMMciter, UAMMciter> uuidairpair;

    // Time stamp for date added for new attachments
    time_t dtime;
    time(&dtime);

    UUIDSet st_attmt_uuid;         // std::set from attachment records on attmt_uuid
    UUIDATRMap mp_attmt_uuid_atr;  // std::map key = attmt_uuid, value = attachment record

    // Find all existing attachments we want to keep (atr.cmpsize is filled in)
    for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();
         iter++) {
      ATRecord atr = iter->second;
      bool bDelete = bCleanup && ((atr.uiflags & ATT_ATTACHMENT_DELETED) != 0);

      if (!bDelete && atr.cmpsize != 0) {
        // Update set with the attachments we want to keep.
        st_attmt_uuid.insert(atr.attmt_uuid);
        // Save the attachment record
        mp_attmt_uuid_atr.insert(make_pair(atr.attmt_uuid, atr));
      }
    }

    st_atpg.function = ATT_PROGRESS_START;
    LoadAString(st_atpg.function_text, IDSC_ATT_COPYFILE);
    st_atpg.value = 0;
    AttachmentProgress(st_atpg);

    // First process all existing attachments
    bool go(true), bCancel(false);
    while (go && in != NULL) {
      atr.Clear();
      status = in->ReadAttmntRecordPreData(atr);
      switch (status) {
        case PWSRC::FAILURE:
        {
          // Show a useful (?) error message - better than
          // silently losing data (but not by much)
          // Best if title intact. What to do if not?
          if (m_pReporter != NULL) {
            stringT cs_msg, cs_caption;
            LoadAString(cs_caption, IDSC_READ_ERROR);
            Format(cs_msg, IDSC_ENCODING_PROBLEM, _T("???"));
            cs_msg = cs_caption + _S(": ") + cs_caption;
            (*m_pReporter)(cs_msg);
          }
          break;
        }
        case PWSRC::SUCCESS:
        {
          unsigned char readtype;
          size_t count(0);
          int status_r, status_w;

          // Only need to write it out if we want to keep it
          if (st_attmt_uuid.find(atr.attmt_uuid) != st_attmt_uuid.end()) {
            // Get attachment record
            UAMiter iter = mp_attmt_uuid_atr.find(atr.attmt_uuid);
            if (iter != mp_attmt_uuid_atr.end()) {
              // Update this entry with the fields that could have been changed:
              atr.flags = iter->second.flags;
              atr.description = iter->second.description;
              st_atpg.function = ATT_PROGRESS_PROCESSFILE;
              st_atpg.value = 0;
              st_atpg.atr = atr;
              AttachmentProgress(st_atpg);

              // Write out pre-data
              out3->WriteAttmntRecordPreData(atr);

              do {
                unsigned char *pCmpData(NULL);
                size_t uiCmpLen;

                // Read in data records
                status_r = in->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, false);
                ASSERT(status_r == PWSRC::SUCCESS);

                // Write them back out
                status_w = out3->WriteAttmntRecordData(pCmpData, uiCmpLen, readtype);
                ASSERT(status_w == PWSRC::SUCCESS);

                // tidy up
                trashMemory(pCmpData, uiCmpLen);
                delete [] pCmpData;
                pCmpData = NULL;

                // Update progress dialog
                count += atr.blksize;
                st_atpg.function = ATT_PROGRESS_PROCESSFILE;
                st_atpg.value = (int)((count * 1.0E02) / atr.uncsize);
                st_atpg.atr = atr;
                int rc = AttachmentProgress(st_atpg);
                if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
                  bCancel = true;
                }
              } while (!bCancel && readtype != PWSAttfile::ATTMT_LASTDATA);

              if (bCancel) {
                in->Close();
                delete in;
                out3->Close();
                delete out3;

                pws_os::DeleteAFile(tempfilename);

                st_atpg.function = ATT_PROGRESS_END;
                AttachmentProgress(st_atpg);
                return PWSRC::USER_CANCEL;
              }
              ASSERT(readtype == PWSAttfile::ATTMT_LASTDATA);

              // Read in post-date
              status = in->ReadAttmntRecordPostData(atr);

              // Write out post-data
              out3->WriteAttmntRecordPostData(atr);
              vATRWritten.push_back(atr);

              st_atpg.function = ATT_PROGRESS_PROCESSFILE;
              st_atpg.value = 100;
              st_atpg.atr = atr;
              AttachmentProgress(st_atpg);
            } else {
              // Should not happen!
              ASSERT(0);
            }
          } else {
            // We don't want this one - so we must skip over its data
            st_atpg.function = ATT_PROGRESS_START;
            LoadAString(st_atpg.function_text, IDSC_ATT_SKIPPINGDELFILE);
            st_atpg.value = 0;
            AttachmentProgress(st_atpg);
            do {
              unsigned char *pCmpData(NULL);
              size_t uiCmpLen;

              // Read in data records
              status_r = in->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, true);
              ASSERT(status_r == PWSRC::SUCCESS);

              // Update progress dialog
              count += atr.blksize;
              st_atpg.function = ATT_PROGRESS_PROCESSFILE;
              st_atpg.value = (int)((count * 1.0E02) / atr.uncsize);
              st_atpg.atr = atr;
              int rc = AttachmentProgress(st_atpg);
              if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
                bCancel = true;
              }
            } while (!bCancel && readtype != PWSAttfile::ATTMT_LASTDATA);

            if (bCancel) {
              in->Close();
              delete in;
              out3->Close();
              delete out3;

              pws_os::DeleteAFile(tempfilename);

              st_atpg.function = ATT_PROGRESS_END;
              AttachmentProgress(st_atpg);
              return PWSRC::USER_CANCEL;
            }

            ASSERT(readtype == PWSAttfile::ATTMT_LASTDATA);

            // Read in post-date
            status = in->ReadAttmntRecordPostData(atr);

            st_atpg.function = ATT_PROGRESS_PROCESSFILE;
            st_atpg.value = 100;
            AttachmentProgress(st_atpg);
          }
          break;
        }
        case PWSRC::END_OF_FILE:
          go = false;
          break;
      } // switch
    };

    if (in != NULL) {
      in->Close();
      delete in;
    }

    st_atpg.function = ATT_PROGRESS_START;
    LoadAString(st_atpg.function_text, IDSC_ATT_APPEND_NEW);
    st_atpg.value = 0;
    AttachmentProgress(st_atpg);
    st_atpg.function_text.clear();

    int num_new(0);
    bCancel = false;
    // Now process new attachments
    for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();
         iter++) {
      ATRecord atr = iter->second;
      // A zero compressed file size says that it hasn't been added yet
      if (atr.cmpsize == 0) {
        if (num_new == 0) {
          st_atpg.function = ATT_PROGRESS_PROCESSFILE;
          st_atpg.value = 0;
          st_atpg.atr = atr;
          AttachmentProgress(st_atpg);
        }

        num_new++;
        // Insert date added in attachment record
        atr.dtime = dtime;
        StringX fname = atr.path + atr.filename;

        // Verify that the file is still there!
        int irc = _taccess(fname.c_str(), F_OK);
        if (irc != 0) {
          // irc = -1 ->
          //   errno = ENOENT (2)  - file not found!
          //         = EACCES (13) - no access
          //         = EINVAL (22) - invalid parameter
          pws_os::Trace(_T("Attachment file _taccess failed. RC=%d, errno=%d\n"), irc, errno);
          break;
        }

        // Open the file
        FILE *fh;
        errno_t err = _tfopen_s(&fh, fname.c_str(), _T("rb"));
        if (err != 0) {
          pws_os::Trace(_T("Attachment file _tfopen_s failed. Error code=%d\n"), err);
          break;
        }

        const bool bDoInIncrements = (atr.uncsize > MINBLOCKSIZE);
        if (atr.uncsize > MINBLOCKSIZE)
          atr.blksize = GetBlocksize(atr.uncsize);
        else
          atr.blksize = atr.uncsize;

        const size_t uiUncLen(atr.blksize);
        BYTE *pUncData = new BYTE[atr.blksize];
        size_t totalread(0), numread;

        // Write out pre-data fields
        out3->WriteAttmntRecordPreData(atr);

        // Initialise progress dialog for this stage
        st_atpg.function = ATT_PROGRESS_PROCESSFILE;
        st_atpg.value = 0;
        st_atpg.atr = atr;
        AttachmentProgress(st_atpg);

        // Get SHA1 hash and CRC of uncompressed data and hash of compressed data
        // Only need one of hash/CRC for uncompressed data (hash more secure) but
        // the user can check the displayed CRC against that shown by, say, WinZip
        // to see if the same
        SHA1 ocontext, ccontext;
        int zRC;

        if (!bDoInIncrements) {
          // Get maximum compressed buffer size
          const size_t uiMaxCmpLen = compressBound(static_cast<uLong>(uiUncLen)) + 1;
          BYTE *pCmpData = new BYTE[uiMaxCmpLen];

          // Read it in one go and compress
          numread = fread(pUncData, 1, atr.uncsize, fh);

          // Make sure we got it all
          ASSERT(numread == atr.uncsize);
          totalread = numread;

          // Calculate complete CRC and odigest of original data
          ocontext.Update(pUncData, reinterpret_cast<unsigned int &>(atr.uncsize));
          ocontext.Final(atr.odigest);
          atr.CRC = PWSUtil::Get_CRC(pUncData, atr.uncsize);

          // Now compress it
          unsigned long ulCmpLen(static_cast<uLong>(uiMaxCmpLen));
          zRC = compress(pCmpData, &ulCmpLen, pUncData, static_cast<uLong>(uiUncLen));
          ASSERT(zRC == Z_OK);

          // Save compressed length
          atr.cmpsize = ulCmpLen;

          // Get digest of compressed data
          ccontext.Update(pCmpData, reinterpret_cast<unsigned int &>(atr.cmpsize));
          ccontext.Final(atr.cdigest);

          // Write out the one and only record
          ASSERT(ulCmpLen > 0);
          out3->WriteAttmntRecordData(pCmpData, atr.cmpsize,
                                      PWSAttfileV3::ATTMT_LASTDATA);

          // Clean up buffers
          trashMemory(pCmpData, uiMaxCmpLen);
          delete [] pCmpData;
          trashMemory(pUncData, atr.uncsize);
          delete [] pUncData;
        } else {
          // Read in increments and compress as we go
          // Allocate deflate state
          z_stream strm;
          strm.zalloc = Z_NULL;
          strm.zfree = Z_NULL;
          strm.opaque = Z_NULL;
          strm.total_out = 0;
          zRC = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
          ASSERT(zRC == Z_OK);

          // Use deflateBound buffer size for output buffer (~ input size + 14%)
          const size_t uiMaxCmpLen = deflateBound(&strm, static_cast<uLong>(uiUncLen)) + 1;
          BYTE *pCmpData = new BYTE[uiMaxCmpLen];
          size_t have;
          int iflush;
          totalread = 0;

          PWSUtil::Get_CRC_Incremental_Init();

          do {
            // Read a block
            numread = fread(pUncData, 1, atr.blksize, fh);
            iflush = feof(fh) ? Z_FINISH : Z_NO_FLUSH;

            // Keep record of data read
            totalread += numread;

            // Update CRC & odigest
            ocontext.Update(pUncData, reinterpret_cast<unsigned int &>(numread));
            PWSUtil::Get_CRC_Incremental_Update(pUncData, numread);

            // Compress it
            strm.avail_in = static_cast<unsigned int>(numread);
            strm.next_in = (Bytef *)pUncData;

            do {
              // Deflate block
              strm.avail_out = static_cast<unsigned int>(uiMaxCmpLen);
              strm.next_out = pCmpData;
              zRC = deflate(&strm, iflush);
              ASSERT(zRC != Z_STREAM_ERROR);     // Check state not clobbered

              // Get size of compressed data (may exclude bits not yet returned
              // if iflush != Z_FINISH)
              have = uiMaxCmpLen - strm.avail_out;

              // Update CRC & cdigest
              if (have > 0)
                ccontext.Update(pCmpData, reinterpret_cast<unsigned int &>(have));

              // Need to check - it just could be that the file size is an exact
              // multipe of the block size chosen!
              if (have > 0 || iflush == Z_FINISH) {
                out3->WriteAttmntRecordData(pCmpData, have,
                              (unsigned char)((iflush == Z_FINISH) ?
                                          PWSAttfileV3::ATTMT_LASTDATA :
                                          PWSAttfileV3::ATTMT_DATA));
              }
              atr.cmpsize += have;
            } while (strm.avail_out == 0);

            // Update progress dialog
            st_atpg.value = (int)((totalread * 1.0E02) / atr.uncsize);
            st_atpg.atr = atr;
            int rc = AttachmentProgress(st_atpg);
            if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
              bCancel = true;
            }
          } while (!bCancel && iflush != Z_FINISH);

          if (bCancel) {
            out3->Close();
            delete out3;
            pws_os::DeleteAFile(tempfilename);

            st_atpg.function = ATT_PROGRESS_END;
            AttachmentProgress(st_atpg);

            fclose(fh);
            vATRWritten.clear();
            return PWSRC::USER_CANCEL;
          }

          // Tidy up compression
          if (zRC == Z_STREAM_END)
            zRC = Z_OK;

          // Clean up deflate environment
          deflateEnd(&strm);

          // Finish off CRC and digest and compressed size
          ocontext.Final(atr.odigest);
          ccontext.Final(atr.cdigest);
          atr.CRC = PWSUtil::Get_CRC_Incremental_Final();

          trashMemory(pCmpData, uiMaxCmpLen);
          delete [] pCmpData;
          trashMemory(pUncData, atr.blksize);
          delete [] pUncData;
        }

        // Write out post-data
        out3->WriteAttmntRecordPostData(atr);

        // Update progress dialog
        st_atpg.function = ATT_PROGRESS_PROCESSFILE;
        st_atpg.value = 100;
        st_atpg.atr = atr;
        AttachmentProgress(st_atpg);

        fclose(fh);
        if (totalread != atr.uncsize) {
          pws_os::Trace(_T("Attachment file read - mismatch of data read. Expected=%d, Read=%d\n"),
                   atr.uncsize, totalread);
          ASSERT(0);
        }

        // Update in-memory records
        iter->second.dtime = atr.dtime;
        memcpy(iter->second.odigest, atr.odigest, SHA1::HASHLEN);
        memcpy(iter->second.cdigest, atr.cdigest, SHA1::HASHLEN);
        iter->second.CRC = atr.CRC;
        iter->second.cmpsize = atr.cmpsize;
        iter->second.blksize = atr.blksize;
        iter->second.attmt_uuid = atr.attmt_uuid;

        // Update info about records written
        vATRWritten.push_back(atr);
      }
    }

    st_atpg.function = ATT_PROGRESS_END;
    AttachmentProgress(st_atpg);

  } catch (...) {
    out3->Close();
    delete out3;
    pws_os::DeleteAFile(tempfilename);

    st_atpg.function = ATT_PROGRESS_END;
    AttachmentProgress(st_atpg);
    return PWSRC::FAILURE;
  }

  out3->Close();
  delete out3;

  // Remove change flag now that records have been written
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  for (size_t i = 0; i < vATRWritten.size(); i++) {
    if ((vATRWritten[i].uiflags & ATT_ATTACHMENT_FLGCHGD) == 0)
      continue;

    uuidairpair = m_MM_entry_uuid_atr.equal_range(vATRWritten[i].entry_uuid);
    // Verify that it is there!
    if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
        (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
      ASSERT(0);
      continue;
    }

    // Now find this specific attachment record
    for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
      if (iter->second.attmt_uuid == vATRWritten[i].attmt_uuid) {
        iter->second.uiflags &= ~ATT_ATTACHMENT_FLGCHGD;
        break;
      }
    }
  }

  // If cleaning up - delete ATRecords no longer needed
  if (bCleanup) {
    for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();) {
      if ((iter->second.uiflags & ATT_ATTACHMENT_DELETED) != 0) {
        m_MM_entry_uuid_atr.erase(iter++);
      } else {
        ++iter;
      }
    }
  }

  status = SaveAttachmentFile(tempfilename);
  return status;
}

int PWScore::DuplicateAttachments(const CUUID &old_entry_uuid,
                                  const CUUID &new_entry_uuid,
                                  PWSAttfile::VERSION version)
{
  int status;

  size_t num = m_MM_entry_uuid_atr.count(old_entry_uuid);
  if (num == 0)
    return PWSRC::FAILURE;

  /*
    Generate a temporary file name - write to this file.  If OK, rename current
    file and then rename the temporary file to the proper name.
    This does leave the older file around as backup (for the moment).
  */

  PWSAttfile *in(NULL), *out(NULL), *dup(NULL);
  stringT tempfilename, current_file, dupfilename, timestamp;
  stringT sDrive, sDir, sName, sExt;
  time_t time_now;

  // Generate temporary names
  time(&time_now);
  timestamp = PWSUtil::ConvertToDateTimeString(time_now, TMC_FILE).c_str();
  pws_os::splitpath(m_currfile.c_str(), sDrive, sDir, sName, sExt);

  tempfilename = sDrive + sDir + sName + timestamp +
                 stringT(ATT_DEFAULT_ATTMT_SUFFIX);
  dupfilename = sDrive + sDir + sName + timestamp +
                 stringT(ATT_DEFAULT_ATTDUP_SUFFIX);
  current_file = sDrive + sDir + sName +
                 stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  // If duplicating - they must be there to begin with!
  if (!pws_os::FileExists(current_file))
    return PWSRC::FAILURE;

  // 'Make' new file
  out = PWSAttfile::MakePWSfile(tempfilename.c_str(), version,
                                PWSAttfile::Write, status,
                                m_pAsker, m_pReporter);
  if (status != PWSRC::SUCCESS) {
    delete out;
    return status;
  }

  // Must be an existing attachment file, 'make' old file
  in = PWSAttfile::MakePWSfile(current_file.c_str(), version,
                               PWSAttfile::Read, status,
                               m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS) {
    delete in;
    delete out;
    return status;
  }

  // 'Make' duplicates temporary file
  dup = PWSAttfile::MakePWSfile(dupfilename.c_str(), version,
                                PWSAttfile::Write, status,
                                m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS) {
    delete in;
    delete out;
    delete dup;
    return status;
  }

  // XXX cleanup gross dynamic_cast
  PWSAttfileV3 *out3 = dynamic_cast<PWSAttfileV3 *>(out);
  PWSAttfileV3 *dup3 = dynamic_cast<PWSAttfileV3 *>(dup);

  // Set up header records
  SetupAttachmentHeader();

  // Set them - will be written during Open below
  out3->SetHeader(m_atthdr);
  dup3->SetHeader(m_atthdr);

  ATRecord atr;
  ATTProgress st_atpg;

  ATRVector vATRWritten, vATRDuplicates;

  try { // exception thrown on write error
    // Open new attachment file
    status = out3->Open(GetPassKey());

    if (status != PWSRC::SUCCESS) {
      delete out3;
      delete dup3;
      delete in;
      return status;
    }

    // Open temporary duplicates file
    status = dup3->Open(GetPassKey());
    if (status != PWSRC::SUCCESS) {
      out3->Close();
      delete out3;
      delete dup3;
      delete in;
      return status;
    }

    // Open current data file
    status = in->Open(GetPassKey());

    if (status != PWSRC::SUCCESS) {
      out3->Close();
      dup3->Close();
      delete out3;
      delete dup3;
      delete in;
      return status;
    }

    st_atpg.function = ATT_PROGRESS_START;
    LoadAString(st_atpg.function_text, IDSC_ATT_COPYFILE);
    st_atpg.value = 0;
    AttachmentProgress(st_atpg);
    st_atpg.function_text.clear();

    std::pair<UAMMciter, UAMMciter> uuidairpair;

    // Time stamp for date added for new attachments
    time_t dtime;
    time(&dtime);

    UUIDSet st_attmt_uuid;         // std::set from records on attmt_uuid
    UUIDATRMap mp_attmt_uuid_atr;  // std::map key = attmnt_uuid, value = attachment record

    // Find all existing attachments we want to keep (atr.cmpsize is filled in)
    for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();
         iter++) {
      if (iter->second.cmpsize != 0) {
        // Update set with the attachments we want to keep.
        st_attmt_uuid.insert(iter->second.attmt_uuid);
        // Save the attachment record
        mp_attmt_uuid_atr.insert(make_pair(iter->second.attmt_uuid, iter->second));
      }
    }

    // First process all existing attachments
    bool go(true), bCancel(false);
    while (go && in != NULL) {
      atr.Clear();
      status = in->ReadAttmntRecordPreData(atr);

      switch (status) {
        case PWSRC::FAILURE:
        {
          // Show a useful (?) error message - better than
          // silently losing data (but not by much)
          // Best if title intact. What to do if not?
          if (m_pReporter != NULL) {
            stringT cs_msg, cs_caption;
            LoadAString(cs_caption, IDSC_READ_ERROR);
            Format(cs_msg, IDSC_ENCODING_PROBLEM, _T("???"));
            cs_msg = cs_caption + _S(": ") + cs_caption;
            (*m_pReporter)(cs_msg);
          }
          break;
        }
        case PWSRC::SUCCESS:
        {
          // Only need to write it out if we want to keep it - can't duplicate
          // attachments being deleted
          if (st_attmt_uuid.find(atr.attmt_uuid) != st_attmt_uuid.end()) {
            // Get attachment record
            UAMiter iter = mp_attmt_uuid_atr.find(atr.attmt_uuid);
            if (iter != mp_attmt_uuid_atr.end()) {
              // Update this entry with the fields that could have been changed:
              atr.flags = iter->second.flags;
              atr.description = iter->second.description;
              st_atpg.function = ATT_PROGRESS_PROCESSFILE;
              st_atpg.value = 0;
              st_atpg.atr = atr;
              AttachmentProgress(st_atpg);

              unsigned char readtype(0);
              unsigned char *pCmpData;
              size_t uiCmpLen, count(0);
              int status_r, status_w, status_d;

              ATRecord atr_dup(atr);
              bool bDuplicate(false);
              // Now maybe duplicate it
              if (iter->second.entry_uuid == old_entry_uuid) {
                bDuplicate = true;
                uuid_array_t new_attmt_uuid;
                CUUID attmt_uuid;
                attmt_uuid.GetUUID(new_attmt_uuid);
                // Change date added timestamp even though it was added to original entry
                atr_dup.dtime = dtime;
                atr_dup.attmt_uuid = new_attmt_uuid;
                atr_dup.entry_uuid = new_entry_uuid;
              }

              // Write out pre-data
              out3->WriteAttmntRecordPreData(atr);
              if (bDuplicate)
                dup3->WriteAttmntRecordPreData(atr_dup);

              status_d = PWSRC::SUCCESS;
              do {
                // Read in data records
                status_r = in->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, false);

                // Write them back out
                if (status_r == PWSRC::SUCCESS) {
                  status_w = out3->WriteAttmntRecordData(pCmpData, uiCmpLen, readtype);
                  if (bDuplicate)
                    status_d = dup3->WriteAttmntRecordData(pCmpData, uiCmpLen, readtype);
                }

                // tidy up
                trashMemory(pCmpData, uiCmpLen);
                delete [] pCmpData;
                pCmpData = NULL;

                // Update progress dialog
                count += atr.blksize;
                st_atpg.function = ATT_PROGRESS_PROCESSFILE;
                st_atpg.value = (int)((count * 1.0E02) / atr.uncsize);
                st_atpg.atr = atr;
                int rc = AttachmentProgress(st_atpg);
                if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
                  bCancel = true;
                }
              } while(!bCancel && status_r == PWSRC::SUCCESS &&
                      status_w == PWSRC::SUCCESS &&
                      status_d == PWSRC::SUCCESS &&
                      readtype != PWSAttfileV3::ATTMT_LASTDATA);

              if (bCancel) {
                in->Close();
                delete in;
                out3->Close();
                delete out3;
                dup3->Close();
                delete dup3;

                pws_os::DeleteAFile(tempfilename);
                pws_os::DeleteAFile(dupfilename);

                st_atpg.function = ATT_PROGRESS_END;
                AttachmentProgress(st_atpg);

                return PWSRC::USER_CANCEL;
              }

              // Write out post-data
              in->ReadAttmntRecordPostData(atr);
              out3->WriteAttmntRecordPostData(atr);
              if (bDuplicate) {
                // Update post-data fields
                atr_dup.cmpsize = atr.cmpsize;
                atr_dup.CRC = atr.CRC;
                memcpy(atr_dup.odigest, atr.odigest, SHA1::HASHLEN);
                memcpy(atr_dup.cdigest, atr.cdigest, SHA1::HASHLEN);
                dup3->WriteAttmntRecordPostData(atr_dup);
                vATRDuplicates.push_back(atr_dup);
              }

              vATRWritten.push_back(atr);

              st_atpg.function = ATT_PROGRESS_PROCESSFILE;
              st_atpg.value = 100;
              st_atpg.atr = atr;
              AttachmentProgress(st_atpg);
            }
          }
          break;
        }
        case PWSRC::END_OF_FILE:
          go = false;
          break;
      } // switch
    };
  } catch (...) {
    in->Close();
    delete in;
    out3->Close();
    delete out3;
    dup3->Close();
    delete dup3;

    pws_os::DeleteAFile(tempfilename);
    pws_os::DeleteAFile(dupfilename);

    st_atpg.function = ATT_PROGRESS_END;
    AttachmentProgress(st_atpg);

    return PWSRC::FAILURE;
  }

  // Close input file
  in->Close();
  delete in;
  in = NULL;

  // Close duplicates temporary file
  dup3->Close();
  delete dup3;
  dup = dup3 = NULL;

  // Now re-open duplicates file and now copy them at the end of our new file
  dup = PWSAttfile::MakePWSfile(dupfilename.c_str(), version,
                                PWSAttfile::Read, status,
                                m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS) {
    out3->Close();
    delete out3;
    delete dup;

    pws_os::DeleteAFile(tempfilename);
    pws_os::DeleteAFile(dupfilename);
    return status;
  }

  dup3 = dynamic_cast<PWSAttfileV3 *>(dup);
  bool bCancel(false);
  try { // exception thrown on write error
    // Open duplicate attachment file
    status = dup3->Open(GetPassKey());

    if (status != PWSRC::SUCCESS) {
      out3->Close();
      delete out3;
      delete dup3;

      pws_os::DeleteAFile(tempfilename);
      pws_os::DeleteAFile(dupfilename);

      st_atpg.function = ATT_PROGRESS_END;
      AttachmentProgress(st_atpg);
      return status;
    }

    st_atpg.function = ATT_PROGRESS_START;
    LoadAString(st_atpg.function_text, IDSC_ATT_APPEND_DUPS);
    st_atpg.value = 0;
    AttachmentProgress(st_atpg);
    st_atpg.function_text.clear();

    // Process all duplicate attachments and append to output file
    bool go(true);
    ATRecord atr;
    while (go && dup != NULL) {
      atr.Clear();
      status = dup3->ReadAttmntRecordPreData(atr);
      switch (status) {
        case PWSRC::FAILURE:
        {
          // Show a useful (?) error message - better than
          // silently losing data (but not by much)
          // Best if title intact. What to do if not?
          if (m_pReporter != NULL) {
            stringT cs_msg, cs_caption;
            LoadAString(cs_caption, IDSC_READ_ERROR);
            Format(cs_msg, IDSC_ENCODING_PROBLEM, _T("???"));
            cs_msg = cs_caption + _S(": ") + cs_caption;
            (*m_pReporter)(cs_msg);
          }
          break;
        }
        case PWSRC::SUCCESS:
        {
          st_atpg.function = ATT_PROGRESS_PROCESSFILE;
          st_atpg.value = 0;
          st_atpg.atr = atr;
          AttachmentProgress(st_atpg);

          unsigned char readtype(0);
          unsigned char *pCmpData;
          size_t uiCmpLen, count(0);
          int status_r, status_w;

          // Write out pre-data
          out3->WriteAttmntRecordPreData(atr);

          do {
            // Read in data records
            status_r = dup3->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, false);

            // Write them back out
            if (status_r == PWSRC::SUCCESS)
              status_w = out3->WriteAttmntRecordData(pCmpData, uiCmpLen, readtype);

            // tidy up
            trashMemory(pCmpData, uiCmpLen);
            delete [] pCmpData;
            pCmpData = NULL;

            // Update progress dialog
            count += atr.blksize;
            st_atpg.value = (int)((count * 1.0E02) / atr.uncsize);
            st_atpg.atr = atr;
            int rc = AttachmentProgress(st_atpg);
            if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
              // Cancel reading attachment file
              bCancel = true;
            }
          } while(!bCancel &&
                  status_r == PWSRC::SUCCESS &&
                  status_w == PWSRC::SUCCESS &&
                  readtype != PWSAttfileV3::ATTMT_LASTDATA);

          if (bCancel) {
            status = PWSRC::END_OF_FILE;
            break;
          }

          // Write out post-data
          dup3->ReadAttmntRecordPostData(atr);
          out3->WriteAttmntRecordPostData(atr);
          vATRWritten.push_back(atr);

          st_atpg.function = ATT_PROGRESS_PROCESSFILE;
          st_atpg.value = 100;
          st_atpg.atr = atr;
          AttachmentProgress(st_atpg);
          break;
        }
        case PWSRC::END_OF_FILE:
          go = false;
          break;
      } // switch
    };

  } catch (...) {
    out3->Close();
    delete out3;
    dup3->Close();
    delete dup3;

    pws_os::DeleteAFile(tempfilename);
    pws_os::DeleteAFile(dupfilename);

    st_atpg.function = ATT_PROGRESS_END;
    AttachmentProgress(st_atpg);
    return PWSRC::FAILURE;
  }

  // Now close output file
  out3->Close();
  delete out3;

  // Now close temporary duplicates file
  dup3->Close();
  delete dup3;

  // Delete temporary duplicates file
  pws_os::DeleteAFile(dupfilename);

  if (bCancel) {
    // Delete temporary file
    pws_os::DeleteAFile(tempfilename);
    return PWSRC::FAILURE;
  }

  // Remove change flag now that records have been written
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  for (size_t i = 0; i < vATRWritten.size(); i++) {
    if ((vATRWritten[i].uiflags & ATT_ATTACHMENT_FLGCHGD) == 0)
      continue;

    uuidairpair = m_MM_entry_uuid_atr.equal_range(vATRWritten[i].entry_uuid);
    // Verify that it is there!
    if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
        (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
      ASSERT(0);
      continue;
    }

    // Now find this specific attachment record
    for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
      if (iter->second.attmt_uuid == vATRWritten[i].attmt_uuid) {
        iter->second.uiflags &= ~ATT_ATTACHMENT_FLGCHGD;
        break;
      }
    }
  }

  // Now update main multimap with the duplicate attachments
  for (size_t i = 0; i < vATRDuplicates.size(); i++) {
    m_MM_entry_uuid_atr.insert(make_pair(new_entry_uuid, vATRDuplicates[i]));
  }

  st_atpg.function = ATT_PROGRESS_END;
  AttachmentProgress(st_atpg);

  status = SaveAttachmentFile(tempfilename);
  return status;
}

int PWScore::SaveAttachmentFile(const stringT &tempfilename)
{
  // Now get current date/time to be used to rename the current file
  // so that we can save it and then rename the new one appropriately
  stringT current_data_file, timestamp;
  stringT sDrive, sDir, sName, sExt;
  stringT oldname, backup_name;
  time_t now;

  time(&now);
  timestamp = PWSUtil::ConvertToDateTimeString(now, TMC_FILE).c_str();

  pws_os::splitpath(m_currfile.c_str(), sDrive, sDir, sName, sExt);

  backup_name = sDrive + sDir + sName + timestamp +
                        stringT(ATT_DEFAULT_ATTBKUP_SUFFIX);

  oldname = sDrive + sDir + sName +
                    stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  bool brc(true);

  bool bOldFileExists = pws_os::FileExists(oldname);

  if (bOldFileExists) {
    // Rename old file
    brc = pws_os::RenameFile(oldname, backup_name);
  }

  if (!brc) {
    pws_os::Trace(_T("Unable to rename existing attachment file.\n"));
    return PWSRC::FAILURE;
  }

  if (m_MM_entry_uuid_atr.size() == 0) {
    // Delete new attachment file if they empty
    pws_os::DeleteAFile(tempfilename);
    return PWSRC::SUCCESS;
  }

  if (brc) {
    // Either the file existed and was renamed successfully or
    // it didn't exist in the first place
    if (!pws_os::RenameFile(tempfilename, oldname)) {
      // Rename failed - rename back if it previously existed and also keep new file!
      pws_os::Trace(_T("Unable to rename new file.\n"));
      // Put old ones back
      if (bOldFileExists)
        pws_os::RenameFile(backup_name, oldname);
      return PWSRC::FAILURE;
    }
  }

  SetChanged(false, false);
  return PWSRC::SUCCESS;
}

void PWScore::SetupAttachmentHeader()
{
  // Set up header records
  m_atthdr.whatlastsaved = m_AppNameAndVersion.c_str();

  time_t time_now;
  time(&time_now);
  m_atthdr.whenlastsaved = time_now;

  uuid_array_t attfile_uuid, null_uuid = {0};
  if (memcmp(m_atthdr.attfile_uuid, null_uuid, sizeof(uuid_array_t)) == 0) {
    CUUID att_uuid;
    att_uuid.GetUUID(attfile_uuid);
    memcpy(m_atthdr.attfile_uuid, attfile_uuid, sizeof(uuid_array_t));
    memcpy(m_atthdr.DBfile_uuid, m_hdr.m_file_uuid_array, sizeof(uuid_array_t));
  }
}

// functor to check if the current attachment matches the one we want to export
struct MatchAUUID {
  MatchAUUID(const CUUID &attmt_uuid) : m_attmt_uuid(attmt_uuid)
  {
  }

  // Does it match?
  bool operator()(const ATRecordEx &atrex) {
    return (m_attmt_uuid == atrex.atr.attmt_uuid);
  }

private:
  MatchAUUID& operator=(const MatchAUUID&); // Do not implement
  CUUID m_attmt_uuid;
};

int PWScore::WriteXMLAttachmentFile(const StringX &filename, ATFVector &vatf,
                                    ATRExVector &vAIRecordExs,
                                    size_t &num_exported)
{
  num_exported = 0;
  const bool bAll = vAIRecordExs.size() == 0;
  if (bAll)
    GetAllAttachments(vAIRecordExs);

  CUTF8Conv utf8conv;
  const unsigned char *utf8 = NULL;
  size_t utf8Len = 0;

#ifdef UNICODE
  utf8conv.ToUTF8(filename, utf8, utf8Len);
#else
  utf8 = filename.c_str();
#endif

  ofstream ofs(reinterpret_cast<const char *>(utf8));

  if (!ofs)
    return PWSRC::CANT_OPEN_FILE;

  bool bCancel(false);

  ATRecord atr;
  ATTProgress st_atpg;
  int status;

  stringT attmnt_file;
  stringT drv, dir, name, ext;
  StringX sxFilename;

  pws_os::splitpath(m_currfile.c_str(), drv, dir, name, ext);
  ext = stringT(ATT_DEFAULT_ATTMT_SUFFIX);
  attmnt_file = drv + dir + name + ext;
  sxFilename = attmnt_file.c_str();

  PWSAttfile::VERSION version = PWSAttfile::V30;

  // 'Make' the data file
  PWSAttfile *in = PWSAttfile::MakePWSfile(sxFilename, version,
                               PWSAttfile::Read, status,
                               m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS) {
    delete in;
    in = NULL;
    return status;
  }

  // Open the data file
  status = in->Open(GetPassKey());
  if (status != PWSRC::SUCCESS) {
    delete in;
    in = NULL;
    return PWSRC::CANT_OPEN_FILE;
  }

  UUIDATRMap mp_attmt_uuid_atr;  // std::map key = attmt_uuid, value = attachment record

  // Find all existing attachments
  for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();
       iter++) {
    ATRecord atr = iter->second;
    // Save the attachment record
    mp_attmt_uuid_atr.insert(make_pair(atr.attmt_uuid, atr));
  }

  oStringXStream oss_xml;
  time_t time_now;
  StringX tmp, temp;

  time(&time_now);
  const StringX now = PWSUtil::ConvertToDateTimeString(time_now, TMC_XML);

  ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
  ofs << "<?xml-stylesheet type=\"text/xsl\" href=\"pwsafe.xsl\"?>" << endl;
  ofs << endl;
  ofs << "<passwordsafe_attachments" << endl;
  temp = m_currfile;
  Replace(temp, StringX(_T("&")), StringX(_T("&amp;")));

  utf8conv.ToUTF8(temp, utf8, utf8Len);
  ofs << "Database=\"";
  ofs.write(reinterpret_cast<const char *>(utf8), utf8Len);
  ofs << "\"" << endl;
  utf8conv.ToUTF8(now, utf8, utf8Len);
  ofs << "ExportTimeStamp=\"";
  ofs.write(reinterpret_cast<const char *>(utf8), utf8Len);
  ofs << "\"" << endl;
  ofs << "FromDatabaseFormat=\"";
  ostringstream osv; // take advantage of UTF-8 == ascii for version string
  osv << m_hdr.m_nCurrentMajorVersion
      << "." << setw(2) << setfill('0')
      << m_hdr.m_nCurrentMinorVersion;
  ofs.write(osv.str().c_str(), osv.str().length());
  ofs << "\"" << endl;
  if (!m_hdr.m_lastsavedby.empty() || !m_hdr.m_lastsavedon.empty()) {
    oStringXStream oss;
    oss << m_hdr.m_lastsavedby << _T(" on ") << m_hdr.m_lastsavedon;
    utf8conv.ToUTF8(oss.str(), utf8, utf8Len);
    ofs << "WhoSaved=\"";
    ofs.write(reinterpret_cast<const char *>(utf8), utf8Len);
    ofs << "\"" << endl;
  }
  if (!m_hdr.m_whatlastsaved.empty()) {
    utf8conv.ToUTF8(m_hdr.m_whatlastsaved, utf8, utf8Len);
    ofs << "WhatSaved=\"";
    ofs.write(reinterpret_cast<const char *>(utf8), utf8Len);
    ofs << "\"" << endl;
  }
  if (m_hdr.m_whenlastsaved != 0) {
    StringX wls = PWSUtil::ConvertToDateTimeString(m_hdr.m_whenlastsaved,
                                                   TMC_XML);
    utf8conv.ToUTF8(wls, utf8, utf8Len);
    ofs << "WhenLastSaved=\"";
    ofs.write(reinterpret_cast<const char *>(utf8), utf8Len);
    ofs << "\"" << endl;
  }

  CUUID db_uuid(m_hdr.m_file_uuid_array, true); // true to print canoncally
  CUUID att_uuid(m_atthdr.attfile_uuid, true); // true to print canoncally

  ofs << "Database_uuid=\"" << db_uuid << "\"" << endl;
  ofs << "Attachment_file_uuid=\"" << att_uuid << "\"" << endl;
  ofs << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << endl;
  ofs << "xsi:noNamespaceSchemaLocation=\"pwsafe.xsd\">" << endl;
  ofs << endl;

  st_atpg.function = ATT_PROGRESS_START;
  LoadAString(st_atpg.function_text, IDSC_ATT_READFILE);
  st_atpg.value = 0;
  AttachmentProgress(st_atpg);

  // Get the header (not sure why?)
  PWSAttfile::AttHeaderRecord ahr = in->GetHeader();

  // We will use in-storage copy of atr (as it has all the data we need)
  // but we have to read the attachment data from the file!
  size_t cmpsize;

  do {
    ATRecordEx atrex;
    status = in->ReadAttmntRecordPreData(atr);
    if (status == PWSRC::END_OF_FILE)
      break;

    if (status != PWSRC::SUCCESS) {
      in->Close();
      delete in;
      in = NULL;

      st_atpg.function = ATT_PROGRESS_END;
      AttachmentProgress(st_atpg);
      return status;
    }

    // Now find this in the in-storage copy (so we don't have to wait till the
    // end of the data to get items we need to write now!
    UAMciter citer = mp_attmt_uuid_atr.find(atr.attmt_uuid);
    ASSERT(citer != mp_attmt_uuid_atr.end());
    atrex.atr = citer->second;

    // Set up extra fields
    ItemListIter entry_iter = Find(atrex.atr.entry_uuid);
    atrex.sxGroup = entry_iter->second.GetGroup();
    atrex.sxTitle = entry_iter->second.GetTitle();
    atrex.sxUser = entry_iter->second.GetUser();

    // If matches - write out pre-data
    const bool bMatches = AttMatches(atrex, vatf) && (bAll ||
       std::find_if(vAIRecordExs.begin(), vAIRecordExs.end(), MatchAUUID(atr.attmt_uuid)) !=
                    vAIRecordExs.end());
    if (bMatches) {
      num_exported++;
      LoadAString(st_atpg.function_text, IDSC_ATT_EXPORTINGFILE);
      ofs << "\t<attachment id=\"" << dec << num_exported << "\" >" << endl;

      // NOTE - ORDER ***MUST*** CORRESPOND TO ORDER IN SCHEMA
      if (!atrex.sxGroup.empty())
        PWSUtil::WriteXMLField(ofs, "group", atrex.sxGroup, utf8conv);

      PWSUtil::WriteXMLField(ofs, "title", atrex.sxTitle, utf8conv);

      if (!atrex.sxUser.empty())
        PWSUtil::WriteXMLField(ofs, "username", atrex.sxUser, utf8conv);

      ofs << "\t\t<attachment_uuid><![CDATA[" << atrex.atr.attmt_uuid <<
                      "]]></attachment_uuid>" << endl;
      ofs << "\t\t<entry_uuid><![CDATA[" << atrex.atr.entry_uuid <<
                      "]]></entry_uuid>" << endl;

      PWSUtil::WriteXMLField(ofs, "filename", atrex.atr.filename, utf8conv);
      PWSUtil::WriteXMLField(ofs, "path", atrex.atr.path, utf8conv);
      PWSUtil::WriteXMLField(ofs, "description", atrex.atr.description, utf8conv);

      ofs << "\t\t<osize>" << dec << atrex.atr.uncsize << "</osize>" << endl;
      ofs << "\t\t<bsize>" << dec << atrex.atr.blksize << "</bsize>" << endl;
      ofs << "\t\t<csize>" << dec << atrex.atr.cmpsize << "</csize>" << endl;

      ofs << "\t\t<crc>" << hex << setfill('0') << setw(8)
          << atrex.atr.CRC << "</crc>" << endl;

      // add digest of original file
      for (unsigned int i = 0; i < SHA1::HASHLEN; i++) {
        Format(temp, _T("%02x"), atrex.atr.odigest[i]);
        tmp += temp;
      }
      utf8conv.ToUTF8(tmp, utf8, utf8Len);
      ofs << "\t\t<odigest>" << utf8 << "</odigest>" << endl;

      tmp.clear();
      temp.clear();
      // add digest of compressed file
      for (unsigned int i = 0; i < SHA1::HASHLEN; i++) {
        Format(temp, _T("%02x"), atrex.atr.cdigest[i]);
        tmp += temp;
      }
      utf8conv.ToUTF8(tmp, utf8, utf8Len);
      ofs << "\t\t<cdigest>" << utf8 << "</cdigest>" << endl;

      ofs << PWSUtil::GetXMLTime(2, "ctime", atrex.atr.ctime, utf8conv);
      ofs << PWSUtil::GetXMLTime(2, "atime", atrex.atr.atime, utf8conv);
      ofs << PWSUtil::GetXMLTime(2, "mtime", atrex.atr.mtime, utf8conv);
      ofs << PWSUtil::GetXMLTime(2, "dtime", atrex.atr.dtime, utf8conv);

      if (atrex.atr.flags != 0) {
        ofs << "\t\t<flags>" << endl;
        if ((atrex.atr.flags & ATT_EXTRACTTOREMOVEABLE) == ATT_EXTRACTTOREMOVEABLE)
          ofs << "\t\t\t<extracttoremoveable>1</extracttoremoveable>" << endl;
        if ((atrex.atr.flags & ATT_ERASEPGMEXISTS) == ATT_ERASEPGMEXISTS)
          ofs << "\t\t\t<eraseprogamexists>1</eraseprogamexists>" << endl;
        if ((atrex.atr.flags & ATT_ERASEONDBCLOSE) == ATT_ERASEONDBCLOSE)
          ofs << "\t\t\t<eraseondatabaseclose>1</eraseondatabaseclose>" << endl;
        ofs << "\t\t</flags>" << endl;
      }
    } else {
      LoadAString(st_atpg.function_text, IDSC_ATT_SKIPPINGFILE);
    }

    st_atpg.function = ATT_PROGRESS_START;
    st_atpg.value = 0;
    AttachmentProgress(st_atpg);

    st_atpg.function = bMatches ? ATT_PROGRESS_EXPORTFILE : ATT_PROGRESS_SEARCHFILE;
    st_atpg.value = 0;
    st_atpg.atr = atr;
    AttachmentProgress(st_atpg);

    // Read all data
    size_t uiCmpLen;
    unsigned char *pCmpData(NULL);
    cmpsize = 0;
    int num_data_records = 0;
    unsigned char readtype;

    do {
      status = in->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, !bMatches);

      if (bMatches) {
        ofs << "\t\t<data" << (readtype == PWSAttfileV3::ATTMT_DATA ? "80" : "81") << "><![CDATA[" << endl;
        cmpsize += uiCmpLen;
        num_data_records++;

        tmp = PWSUtil::Base64Encode(pCmpData, uiCmpLen).c_str();
        utf8conv.ToUTF8(tmp, utf8, utf8Len);
        for (unsigned int i = 0; i < (unsigned int)utf8Len; i += 64) {
          char buffer[65];
          memset(buffer, 0, sizeof(buffer));
          if (utf8Len - i > 64)
            memcpy(buffer, utf8 + i, 64);
          else
            memcpy(buffer, utf8 + i, utf8Len - i);

          ofs << "\t\t" << buffer << endl;
        }

        ofs << "\t\t]]></data" << (readtype == PWSAttfileV3::ATTMT_DATA ? "80" : "81") << ">" << endl;

        // tidy up
        trashMemory(pCmpData, uiCmpLen);
        delete [] pCmpData;
        pCmpData = NULL;
      }

      st_atpg.function = bMatches ? ATT_PROGRESS_EXPORTFILE : ATT_PROGRESS_SEARCHFILE;
      st_atpg.value = (int)((cmpsize * 1.0E02) / atrex.atr.cmpsize);
      st_atpg.atr = atrex.atr;
      int rc = AttachmentProgress(st_atpg);
      if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
        bCancel = true;
      }
    } while (!bCancel && status == PWSRC::SUCCESS &&
             readtype != PWSAttfileV3::ATTMT_LASTDATA);

    if (bCancel) {
      st_atpg.function = ATT_PROGRESS_END;
      AttachmentProgress(st_atpg);

      ahr.Clear();

      // Close the data file
      in->Close();
      delete in;
      in = NULL;

      ofs << "***USER ABORTED***" << endl;
      ofs.close();

      return PWSRC::USER_CANCEL;
    }

    // Read post-data for this attachment
    status = in->ReadAttmntRecordPostData(atr);
    if (bMatches) {
      ofs << "\t</attachment>" << endl;
    }

    st_atpg.function = bMatches ? ATT_PROGRESS_EXPORTFILE : ATT_PROGRESS_SEARCHFILE;
    st_atpg.value = 100;
    st_atpg.atr = atr;
    AttachmentProgress(st_atpg);

  } while (status == PWSRC::SUCCESS);

  st_atpg.function = ATT_PROGRESS_END;
  AttachmentProgress(st_atpg);

  ahr.Clear();

  // Close the data file
  in->Close();
  delete in;
  in = NULL;

  ofs << "</passwordsafe_attachments>" << endl;
  ofs.close();

  return PWSRC::SUCCESS;
}

#if !defined(USE_XML_LIBRARY) || (!defined(_WIN32) && USE_XML_LIBRARY == MSXML)
// Don't support importing XML on non-Windows platforms using Microsoft XML libraries
int PWScore::ImportXMLAttachmentFile(const stringT &,
                           const stringT &,
                           stringT &, stringT &,
                           int &, int &, int &,
                           CReport &, Command *&)
{
  return PWSRC::UNIMPLEMENTED;
}
#else
int PWScore::ImportXMLAttachmentFile(const stringT &strXMLFileName,
                           const stringT &strXSDFileName,
                           stringT &strXMLErrors, stringT &strSkippedList,
                           int &numValidated, int &numImported, int &numSkipped,
                           CReport &rpt, Command *&pcommand)
{
  UUIDVector Possible_Aliases, Possible_Shortcuts;
  MultiCommands *pmulticmds = MultiCommands::Create(this);
  pcommand = pmulticmds;

#if   USE_XML_LIBRARY == EXPAT
  EAttXMLProcessor iXML(this, pmulticmds, &rpt);
#elif USE_XML_LIBRARY == MSXML
  MAttXMLProcessor iXML(this, pmulticmds, &rpt);
#elif USE_XML_LIBRARY == XERCES
  XAttXMLProcessor iXML(this, pmulticmds, &rpt);
#endif

  bool status, validation;
  int istatus;

  strXMLErrors = _T("");

  // Expat is not a validating parser - but we now do it ourselves!
  validation = true;
  status = iXML.Process(validation, strXMLFileName, strXSDFileName, NULL);
  strXMLErrors = iXML.getXMLErrors();

  if (!status) {
    return PWSRC::XML_FAILED_VALIDATION;
  }
  numValidated = iXML.getNumAttachmentsValidated();

  validation = false;
  stringT impfilename;
  PWSAttfile *pimport = CreateImportFile(impfilename);

  status = iXML.Process(validation, strXMLFileName,
                        strXSDFileName, pimport);

  istatus = XCompleteImportFile(impfilename);

  numImported = iXML.getNumAttachmentsImported();
  numSkipped = iXML.getNumAttachmentsSkipped();

  strXMLErrors = iXML.getXMLErrors();
  strSkippedList = iXML.getSkippedList();

  if (!status || istatus != PWSRC::SUCCESS) {
    delete pcommand;
    pcommand = NULL;
    return PWSRC::XML_FAILED_IMPORT;
  }

  if (numImported > 0)
    SetDBChanged(true);

  return PWSRC::SUCCESS ;
}

PWSAttfile *PWScore::CreateImportFile(stringT &impfilename, PWSAttfile::VERSION version)
{
  PWSAttfile *imp(NULL);
  stringT timestamp;
  stringT sDrive, sDir, sName, sExt;
  time_t time_now;
  int status;

  time(&time_now);
  timestamp = PWSUtil::ConvertToDateTimeString(time_now, TMC_FILE).c_str();
  pws_os::splitpath(m_currfile.c_str(), sDrive, sDir, sName, sExt);

  impfilename = sDrive + sDir + sName + timestamp +
                 stringT(ATT_DEFAULT_ATTIMP_SUFFIX);

  // 'Make' new file
  imp = PWSAttfile::MakePWSfile(impfilename.c_str(), version,
                                PWSAttfile::Write, status,
                                m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS) {
    delete imp;
    imp = NULL;
    return imp;
  }

  PWSAttfileV3 *imp3 = dynamic_cast<PWSAttfileV3 *>(imp);
  status = imp3->Open(GetPassKey());

  if (status != PWSRC::SUCCESS) {
    delete imp3;
    imp = NULL;
  }
  return imp;
}

int PWScore::CompleteImportFile(const stringT &impfilename, PWSAttfile::VERSION version)
{
  PWSAttfile *in(NULL), *out(NULL), *imp(NULL);
  stringT tempfilename, current_file, timestamp;
  stringT sDrive, sDir, sName, sExt;
  time_t time_now;
  int status;
  bool go(true), bCancel(false);

  // Generate temporary names
  time(&time_now);
  timestamp = PWSUtil::ConvertToDateTimeString(time_now, TMC_FILE).c_str();
  pws_os::splitpath(m_currfile.c_str(), sDrive, sDir, sName, sExt);

  tempfilename = sDrive + sDir + sName + timestamp +
                 stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  current_file = sDrive + sDir + sName +
                 stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  // 'Make' new file
  out = PWSAttfile::MakePWSfile(tempfilename.c_str(), version,
                                PWSAttfile::Write, status,
                                m_pAsker, m_pReporter);
  if (status != PWSRC::SUCCESS) {
    delete out;
    return status;
  }

  // Must be an existing attachment file, 'make' old file
  in = PWSAttfile::MakePWSfile(current_file.c_str(), version,
                               PWSAttfile::Read, status,
                               m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS) {
    delete in;
    delete out;
    return status;
  }

  // XXX cleanup gross dynamic_cast
  PWSAttfileV3 *out3 = dynamic_cast<PWSAttfileV3 *>(out);

  // Set up header records
  SetupAttachmentHeader();

  // Set them - will be written during Open below
  out3->SetHeader(m_atthdr);

  ATRecord atr;
  ATTProgress st_atpg;

  st_atpg.function = ATT_PROGRESS_START;
  LoadAString(st_atpg.function_text, IDSC_ATT_COPYFILE);
  st_atpg.value = 0;
  AttachmentProgress(st_atpg);
  st_atpg.function_text.clear();

  ATRVector vATRWritten, vATRImports;

  try { // exception thrown on write error
    // Open new attachment file
    status = out3->Open(GetPassKey());

    if (status != PWSRC::SUCCESS) {
      delete out3;
      delete in;

      st_atpg.function = ATT_PROGRESS_END;
      AttachmentProgress(st_atpg);
      return status;
    }

    // Open current data file
    status = in->Open(GetPassKey());

    if (status != PWSRC::SUCCESS) {
      out3->Close();
      delete out3;
      delete in;

      st_atpg.function = ATT_PROGRESS_END;
      AttachmentProgress(st_atpg);
      return status;
    }

    std::pair<UAMMciter, UAMMciter> uuidairpair;

    // Time stamp for date added for new attachments
    time_t dtime;
    time(&dtime);

    UUIDSet st_attmt_uuid;         // std::set from records on attmt_uuid
    UUIDATRMap mp_attmt_uuid_atr;  // std::map key = attmnt_uuid, value = attachment record

    // Find all existing attachments we want to keep (atr.cmpsize is filled in)
    for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();
         iter++) {
      if (iter->second.cmpsize != 0) {
        // Update set with the attachments we want to keep.
        st_attmt_uuid.insert(iter->second.attmt_uuid);
        // Save the attachment record
        mp_attmt_uuid_atr.insert(make_pair(iter->second.attmt_uuid, iter->second));
      }
    }

    // First process all existing attachments
    bool go(true);
    while (go && in != NULL) {
      atr.Clear();
      status = in->ReadAttmntRecordPreData(atr);

      st_atpg.function = ATT_PROGRESS_PROCESSFILE;
      st_atpg.value = 0;
      st_atpg.atr = atr;
      AttachmentProgress(st_atpg);

      switch (status) {
        case PWSRC::FAILURE:
        {
          // Show a useful (?) error message - better than
          // silently losing data (but not by much)
          // Best if title intact. What to do if not?
          if (m_pReporter != NULL) {
            stringT cs_msg, cs_caption;
            LoadAString(cs_caption, IDSC_READ_ERROR);
            Format(cs_msg, IDSC_ENCODING_PROBLEM, _T("???"));
            cs_msg = cs_caption + _S(": ") + cs_caption;
            (*m_pReporter)(cs_msg);
          }
          break;
        }
        case PWSRC::SUCCESS:
        {
          // Only need to write it out if we want to keep it - can't duplicate
          // attachments being deleted
          if (st_attmt_uuid.find(atr.attmt_uuid) != st_attmt_uuid.end()) {
            // Get attachment record
            UAMiter iter = mp_attmt_uuid_atr.find(atr.attmt_uuid);
            if (iter != mp_attmt_uuid_atr.end()) {
              // Update this entry with the fields that could have been changed:
              atr.flags = iter->second.flags;
              atr.description = iter->second.description;

              unsigned char readtype(0);
              unsigned char *pCmpData;
              size_t uiCmpLen, count(0);
              int status_r, status_w;

              // Write out pre-data
              out3->WriteAttmntRecordPreData(atr);
              status_w = PWSRC::SUCCESS;

              do {
                // Read in data records
                status_r = in->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, false);

                // Write them back out
                if (status_r == PWSRC::SUCCESS) {
                  status_w = out3->WriteAttmntRecordData(pCmpData, uiCmpLen, readtype);
                }

                // tidy up
                trashMemory(pCmpData, uiCmpLen);
                delete [] pCmpData;
                pCmpData = NULL;

                // Update progress dialog
                count += atr.blksize;
                st_atpg.function = ATT_PROGRESS_PROCESSFILE;
                st_atpg.value = (int)((count * 1.0E02) / atr.uncsize);
                st_atpg.atr = atr;
                int rc = AttachmentProgress(st_atpg);
                if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
                  bCancel = true;
                }
              } while(!bCancel &&
                      status_r == PWSRC::SUCCESS &&
                      status_w == PWSRC::SUCCESS &&
                      readtype != PWSAttfileV3::ATTMT_LASTDATA);

              if (bCancel) {
                in->Close();
                delete in;
                out3->Close();
                delete out3;

                pws_os::DeleteAFile(tempfilename);
                pws_os::DeleteAFile(impfilename);

                st_atpg.function = ATT_PROGRESS_END;
                AttachmentProgress(st_atpg);
                return PWSRC::USER_CANCEL;
              }
              // Write out post-data
              in->ReadAttmntRecordPostData(atr);
              out3->WriteAttmntRecordPostData(atr);
              vATRWritten.push_back(atr);

              st_atpg.function = ATT_PROGRESS_PROCESSFILE;
              st_atpg.value = 100;
              st_atpg.atr = atr;
              AttachmentProgress(st_atpg);
            }
          }
          break;
        }
        case PWSRC::END_OF_FILE:
          go = false;
          break;
      } // switch
    };
  } catch (...) {
    in->Close();
    delete in;
    out3->Close();
    delete out3;

    pws_os::DeleteAFile(tempfilename);
    pws_os::DeleteAFile(impfilename);

    st_atpg.function = ATT_PROGRESS_END;
    AttachmentProgress(st_atpg);
    return PWSRC::FAILURE;
  }

  // Close input file
  in->Close();
  delete in;
  in = NULL;

  // Now re-open duplicates file and now copy them at the end of our new file
  imp = PWSAttfile::MakePWSfile(impfilename.c_str(), version,
                                PWSAttfile::Read, status,
                                m_pAsker, m_pReporter);

  if (status != PWSRC::SUCCESS) {
    out3->Close();
    delete out3;

    pws_os::DeleteAFile(tempfilename);
    pws_os::DeleteAFile(impfilename);

    st_atpg.function = ATT_PROGRESS_END;
    AttachmentProgress(st_atpg);
    return status;
  }

  PWSAttfileV3 *imp3 = dynamic_cast<PWSAttfileV3 *>(imp);
  try { // exception thrown on write error
    // Open duplicate attachment file
    status = imp3->Open(GetPassKey());

    if (status != PWSRC::SUCCESS) {
      out3->Close();
      delete out3;
      delete imp3;

      pws_os::DeleteAFile(tempfilename);
      pws_os::DeleteAFile(impfilename);

      st_atpg.function = ATT_PROGRESS_END;
      AttachmentProgress(st_atpg);
      return status;
    }

    st_atpg.function = ATT_PROGRESS_START;
    LoadAString(st_atpg.function_text, IDSC_ATT_APPEND_IMP);
    st_atpg.value = 0;
    AttachmentProgress(st_atpg);
    st_atpg.function_text.clear();

    // Process all imported attachments and append to output file
    go = true;
    bCancel = false;
    while (go && imp != NULL) {
      atr.Clear();
      status = imp3->ReadAttmntRecordPreData(atr);
      switch (status) {
        case PWSRC::FAILURE:
        {
          // Show a useful (?) error message - better than
          // silently losing data (but not by much)
          // Best if title intact. What to do if not?
          if (m_pReporter != NULL) {
            stringT cs_msg, cs_caption;
            LoadAString(cs_caption, IDSC_READ_ERROR);
            Format(cs_msg, IDSC_ENCODING_PROBLEM, _T("???"));
            cs_msg = cs_caption + _S(": ") + cs_caption;
            (*m_pReporter)(cs_msg);
          }
          break;
        }
        case PWSRC::SUCCESS:
        {
          unsigned char readtype(0);
          unsigned char *pCmpData;
          size_t uiCmpLen, count(0);
          int status_r, status_w;

          st_atpg.function = ATT_PROGRESS_PROCESSFILE;
          st_atpg.value = 0;
          st_atpg.atr = atr;
          AttachmentProgress(st_atpg);

          // Write out pre-data
          out3->WriteAttmntRecordPreData(atr);
          status_w = PWSRC::SUCCESS;

          do {
            // Read in data records
            status_r = imp3->ReadAttmntRecordData(pCmpData, uiCmpLen, readtype, false);

            // Write them back out
            if (status_r == PWSRC::SUCCESS)
              status_w = out3->WriteAttmntRecordData(pCmpData, uiCmpLen, readtype);

            // tidy up
            trashMemory(pCmpData, uiCmpLen);
            delete [] pCmpData;
            pCmpData = NULL;

            // Update progress dialog
            count += atr.blksize;
            st_atpg.value = (int)((count * 1.0E02) / atr.uncsize);
            st_atpg.atr = atr;
            int rc = AttachmentProgress(st_atpg);
            if ((rc & ATT_PROGRESS_CANCEL) == ATT_PROGRESS_CANCEL) {
              // Cancel reading attachment file
              bCancel = true;
            }
          } while(!bCancel &&
                  status_r == PWSRC::SUCCESS &&
                  status_w == PWSRC::SUCCESS &&
                  readtype != PWSAttfileV3::ATTMT_LASTDATA);

          if (bCancel) {
            out3->Close();
            delete out3;
            imp3->Close();
            delete imp3;

            pws_os::DeleteAFile(tempfilename);
            pws_os::DeleteAFile(impfilename);

            st_atpg.function = ATT_PROGRESS_END;
            AttachmentProgress(st_atpg);
            return PWSRC::USER_CANCEL;
          }

          // Write out post-data
          imp3->ReadAttmntRecordPostData(atr);
          out3->WriteAttmntRecordPostData(atr);
          vATRImports.push_back(atr);

          st_atpg.function = ATT_PROGRESS_PROCESSFILE;
          st_atpg.value = 100;
          AttachmentProgress(st_atpg);
          break;
        }
        case PWSRC::END_OF_FILE:
          go = false;
          break;
      } // switch
    };
  } catch (...) {
    out3->Close();
    delete out3;
    imp3->Close();
    delete imp3;

    pws_os::DeleteAFile(tempfilename);
    pws_os::DeleteAFile(impfilename);

    st_atpg.function = ATT_PROGRESS_END;
    AttachmentProgress(st_atpg);
    return PWSRC::FAILURE;
  }

  // Now close output file
  out3->Close();
  delete out3;

  // Now close temporary import file
  imp3->Close();
  delete imp3;

  // Delete temporary duplicates file
  pws_os::DeleteAFile(impfilename);

  // Remove change flag now that records have been written
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  for (size_t i = 0; i < vATRWritten.size(); i++) {
    if ((vATRWritten[i].uiflags & ATT_ATTACHMENT_FLGCHGD) == 0)
      continue;

    uuidairpair = m_MM_entry_uuid_atr.equal_range(vATRWritten[i].entry_uuid);
    // Verify that it is there!
    if ((uuidairpair.first  == m_MM_entry_uuid_atr.end()) &&
        (uuidairpair.second == m_MM_entry_uuid_atr.end())) {
      ASSERT(0);
      continue;
    }

    // Now find this specific attachment record
    for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
      if (iter->second.attmt_uuid == vATRWritten[i].attmt_uuid) {
        iter->second.uiflags &= ~ATT_ATTACHMENT_FLGCHGD;
        break;
      }
    }
  }

  // Now update main multimap with the imported attachments
  for (size_t i = 0; i < vATRImports.size(); i++) {
    m_MM_entry_uuid_atr.insert(make_pair(vATRImports[i].entry_uuid, vATRImports[i]));
  }

  st_atpg.function = ATT_PROGRESS_END;
  AttachmentProgress(st_atpg);

  status = SaveAttachmentFile(tempfilename);
  return status;
}
#endif

struct get_att_uuid {
  get_att_uuid(UUIDAVector &vatt_uuid)
  :  m_vatt_uuid(vatt_uuid)
  {}

  void operator()(std::pair<const CUUID, ATRecord> const& p) const {
    m_vatt_uuid.push_back(p.second.attmt_uuid);
  }

private:
  UUIDAVector &m_vatt_uuid;
};

UUIDAVector PWScore::GetAttachmentUUIDs()
{
  UUIDAVector vatt_uuid;
  get_att_uuid gatt_uuid(vatt_uuid);

  for_each(m_MM_entry_uuid_atr.begin(), m_MM_entry_uuid_atr.end(), gatt_uuid);
  return vatt_uuid;
}

bool PWScore::AttMatches(const ATRecordEx &atrex, const ATFVector &atfv)
{
  // Tests are only OR - not AND
  int iMatch(0);
  size_t num_tests = atfv.size();
  for (size_t i = 0; i < num_tests; i++) {
    if (atfv[i].set != 0) {
      if (AttMatches(atrex, atfv[i].object, atfv[i].function, atfv[i].value))
        return true;  // Passed - get out now
      else
        iMatch--;     // Failed test - never know, another might be ok
    }
  }
  // If it passed a test - already returned true
  // If no tests - would have dropped through and iMatch == 0
  // So if iMatch < 0, it didn't pass any user tests
  return (iMatch == 0);
}

bool PWScore::AttMatches(const ATRecordEx &atrex, const int &iObject, const int &iFunction,
                         const stringT &value) const
{
  ASSERT(iFunction != 0); // must be positive or negative!

  StringX csObject;
  switch(iObject) {
    case ATTGROUP:
      csObject = atrex.sxGroup;
      break;
    case ATTTITLE:
      csObject = atrex.sxTitle;
      break;
    case ATTUSER:
      csObject = atrex.sxUser;
      break;
    case ATTGROUPTITLE:
      csObject = atrex.sxGroup + TCHAR('.') + atrex.sxTitle;
      break;
    case ATTPATH:
      csObject = atrex.atr.path;
      break;
    case ATTFILENAME:
      csObject = atrex.atr.filename;
      break;
    case ATTDESCRIPTION:
      csObject = atrex.atr.description;
      break;
    default:
      ASSERT(0);
  }

  const bool bValue = !csObject.empty();
  if (iFunction == PWSMatch::MR_PRESENT || iFunction == PWSMatch::MR_NOTPRESENT) {
    return PWSMatch::Match(bValue, iFunction);
  }

  if (!bValue) // String empty - always return false for other comparisons
    return false;
  else
    return PWSMatch::Match(value.c_str(), csObject, iFunction);
}
