/*
/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// file PWScore.cpp
//-----------------------------------------------------------------------------

#include "PWScore.h"
#include "corelib.h"
#include "Util.h"
#include "UUIDGen.h"
#include "SysInfo.h"
#include "UTF8Conv.h"
#include "Report.h"
#include "Match.h"
#include "PWSAttfileV3.h" // XXX cleanup with dynamic_cast

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

// hide w_char/char differences where possible:
#ifdef UNICODE
typedef std::wifstream ifstreamT;
typedef std::wofstream ofstreamT;
#else
typedef std::ifstream ifstreamT;
typedef
#endif

// Return whether uuid elem1 is less than uuid elem2
// Used in set_difference between 2 sets
bool uuid_lesser(st_UUID elem1, st_UUID elem2)
{
  return memcmp(elem1.uuid, elem2.uuid, sizeof(uuid_array_t)) < 0;
}

// Return whether mulitmap pair uuid is less than the other uuid
// Used in set_difference between 2 multimaps
bool mp_uuid_lesser(ItemMap_Pair p1, ItemMap_Pair p2)
{
  uuid_array_t uuid1, uuid2;
  p1.first.GetUUID(uuid1);
  p2.first.GetUUID(uuid2);
  int icomp = memcmp(uuid1, uuid2, sizeof(uuid_array_t));
  if (icomp == 0) {
    p1.second.GetUUID(uuid1);
    p2.second.GetUUID(uuid2);
    return memcmp(uuid1, uuid2, sizeof(uuid_array_t)) < 0;
  } else
    return icomp < 0;
}

struct GetATR {
  GetATR(const st_UUID &st_uuid)
  {
    memcpy(m_attmt_uuid, st_uuid.uuid, sizeof(uuid_array_t));
  }

  bool operator()(pair<st_UUID, ATRecord> p)
  {
    return (memcmp(p.second.attmt_uuid, m_attmt_uuid, sizeof(uuid_array_t)) == 0);
  }

private:
  uuid_array_t m_attmt_uuid;
};

int PWScore::ReadAttachmentFile(const bool bVerify)
{
  int status;
  stringT attmt_file;
  m_MM_entry_uuid_atr.clear();

  if (m_currfile.empty())
    return PWSAttfile::CANT_OPEN_FILE;

  // Generate attachment file names from database name
  stringT drv, dir, name, ext;

  pws_os::splitpath(m_currfile.c_str(), drv, dir, name, ext);
  attmt_file = drv + dir + name + stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  // If attachment file doesn't exist - OK
  if (!pws_os::FileExists(attmt_file)) {
    pws_os::Trace(_T("No attachment file exists.\n"));
    return PWSAttfile::SUCCESS;
  }

  PWSAttfile::VERSION version;
  version = PWSAttfile::V30;

  // 'Make' file
  PWSAttfile *in = PWSAttfile::MakePWSfile(attmt_file.c_str(), version,
                                           PWSAttfile::Read, status,
                                           m_pAsker, m_pReporter);

  if (status != PWSAttfile::SUCCESS) {
    delete in;
    return status;
  }

  // Open file
  status = in->Open(GetPassKey());
  if (status != PWSAttfile::SUCCESS) {
    pws_os::Trace(_T("Open attachment file failed RC=%d\n"), status);
    delete in;
    return PWSAttfile::CANT_OPEN_FILE;
  }

  PWSAttfile::AttHeaderRecord ahr;
  ahr = in->GetHeader();

  if (memcmp(ahr.DBfile_uuid, m_hdr.m_file_uuid_array, sizeof(uuid_array_t)) != 0) {
    pws_os::Trace(_T("Attachment header - database UUID inconsistent.\n"));
    in->Close();
    delete in;
    return PWSAttfile::HEADERS_INVALID;
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
  UUIDATRMMap mm_entry_uuid_atr; // std::multimap key = entry_uuid, value = attachment record

  bool go(true);
  do {
    ATRecord atr;
    bool bError(false);
    // Read record but not the actual attachment unless bVerify == true
    status = in->ReadAttmntRecord(atr, NULL_UUID, !bVerify);
    switch (status) {
      case PWSfile::FAILURE:
      {
        // Show a useful (?) error message - better than
        // silently losing data (but not by much)
        // Best if title intact. What to do if not?
        if (m_pReporter != NULL) {
          stringT cs_msg, cs_caption;
          LoadAString(cs_caption, IDSC_READ_ERROR);
          Format(cs_msg, IDSC_ENCODING_PROBLEM, atr.filename.c_str());
          cs_msg = cs_caption + _S(": ") + cs_caption;
          (*m_pReporter)(cs_msg);
        }
        break;
      }
      case PWSAttfile::SUCCESS:
      {
        pr = st_attmt_uuid.insert(atr.attmt_uuid);
        if (!pr.second) {
          bError = true;
          break;
        }

        mp_entry_uuid.insert(ItemMMap_Pair(atr.entry_uuid, atr.attmt_uuid));
        st_UUID stuuid(atr.entry_uuid);
        mm_entry_uuid_atr.insert(make_pair(stuuid, atr));
        break;
      }
      case PWSfile::END_OF_FILE:
        go = false;
        break;
    } // switch
    if (bError) {
      pws_os::Trace(_T("Duplicate entries found in attachment file and have been ignored.\n"));
    }
  } while (go);

  int closeStatus = in->Close();
  if (bVerify && closeStatus != PWSAttfile::SUCCESS)
    ASSERT(0);

  delete in;

  // All OK - update the entries in PWScore
  m_atthdr = ahr;
  m_MM_entry_uuid_atr = mm_entry_uuid_atr;

  return PWSAttfile::SUCCESS;
}

void PWScore::AddAttachment(const ATRecord &atr)
{
  // Add attachment record using the DB entry UUID as key
  st_UUID stuuid(atr.entry_uuid);
  m_MM_entry_uuid_atr.insert(make_pair(stuuid, atr));
}

void PWScore::AddAttachments(ATRVector &vNewATRecords)
{
  if (vNewATRecords.empty())
    return;

  // Add attachment record using the DB entry UUID as key
  st_UUID stuuid(vNewATRecords[0].entry_uuid);

  for (size_t i = 0; i < vNewATRecords.size(); i++) {
    m_MM_entry_uuid_atr.insert(make_pair(stuuid, vNewATRecords[i]));
  }
}

void PWScore::ChangeAttachment(const ATRecord &atr)
{
  // First delete old one
  // Find current entry by getting subset for this DB entry
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(atr.entry_uuid);

  // Now find this specific attachment record
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    if (memcmp(iter->second.attmt_uuid, atr.attmt_uuid, sizeof(uuid_array_t)) == 0) {
      m_MM_entry_uuid_atr.erase(iter);
      break;
    }
  }

  // Put back in the changed one
  st_UUID stuuid(atr.entry_uuid);
  m_MM_entry_uuid_atr.insert(make_pair(stuuid, atr));
}

bool PWScore::MarkAttachmentForDeletion(const ATRecord &atr)
{
  bool bRC(false);
  // Find current entry by getting subset for this DB entry
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(atr.entry_uuid);

  // Now find this specific attachment record
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    if (memcmp(iter->second.attmt_uuid, atr.attmt_uuid, sizeof(uuid_array_t)) == 0) {
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

  // Now find this specific attachment record
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    if (memcmp(iter->second.attmt_uuid, atr.attmt_uuid, sizeof(uuid_array_t)) == 0) {
      iter->second.uiflags &= ~(ATT_ATTACHMENT_FLGCHGD | ATT_ATTACHMENT_DELETED);
      bRC = true;
      break;
    }
  }
  return bRC;
}

void PWScore::MarkAllAttachmentsForDeletion(const uuid_array_t &entry_uuid)
{
  // Mark all attachment records for this database entry for deletion
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(entry_uuid);

  // Now update all attachment records
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    iter->second.uiflags |= (ATT_ATTACHMENT_FLGCHGD | ATT_ATTACHMENT_DELETED);
  }
}

void PWScore::UnMarkAllAttachmentsForDeletion(const uuid_array_t &entry_uuid)
{
  // UnMark all attachment records for this database entry for deletion
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  uuidairpair = m_MM_entry_uuid_atr.equal_range(entry_uuid);

  // Now update all attachment records
  for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
    iter->second.uiflags &= ~(ATT_ATTACHMENT_FLGCHGD | ATT_ATTACHMENT_DELETED);
  }
}

size_t PWScore::HasAttachments(const uuid_array_t &entry_uuid)
{
  st_UUID stuuid(entry_uuid);
  return m_MM_entry_uuid_atr.count(entry_uuid);
}

unsigned char * PWScore::GetAttachment(const ATRecord &in_atr, int &status)
{
  // Get the attachment referenced by the supplied attachment record.

  // On calling, pSource must be NULL and ulLen must be zero.  If successful, these will
  // be updated to point to the compressed attachment.
  // If the attachment is found, the caller is responsible for freeing the buffer!
  unsigned char *pSource(NULL);
  status = ReadAttachmentFile();

  if (status != PWSAttfile::SUCCESS)
    return NULL;

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

  if (status != PWSAttfile::SUCCESS) {
    delete in;
    return NULL;
  }

  // Open the data file
  status = in->Open(GetPassKey());
  if (status != PWSAttfile::SUCCESS) {
    delete in;
    status = PWSAttfile::CANT_OPEN_FILE;
    return NULL;
  }

  // Get the header (not sure why?)
  PWSAttfile::AttHeaderRecord ahr;
  ahr = in->GetHeader();
  unsigned long ulSourceLen;

  bool go(true);
  do {
    ATRecord atr;
    // Read the records but only retrieve the data portion if the data record UUID
    // is the same as in the supplied attachment record
    status = in->ReadAttmntRecord(atr, in_atr.attmt_uuid);
    switch (status) {
      case PWSAttfile::FAILURE:
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
      case PWSAttfile::SUCCESS:
        // The one we wanted?
        if (memcmp(atr.attmt_uuid, in_atr.attmt_uuid, sizeof(uuid_array_t)) != 0)
          break;

        // Save the data pointer and length
        pSource = new unsigned char[atr.uncsize];
        ulSourceLen = atr.uncsize;
        int zRC;
        zRC = uncompress(pSource, &ulSourceLen, atr.pData, atr.cmpsize);
        if (zRC != Z_OK) {
          // Error message to the user?
          trashMemory(atr.pData, atr.cmpsize);
          delete [] atr.pData;
          atr.pData = NULL;
          trashMemory(pSource, ulSourceLen);
          delete [] pSource;
          pSource = NULL;
          break;
        }
        ASSERT(ulSourceLen == atr.uncsize);
        // Stop reading!
        go = false;
        break;
      case PWSAttfile::END_OF_FILE:
        // If we got here - we have a problem as we didn't find the data record
        // corresponding to the supplied attachment record!
        go = false;
        break;
    } // switch
  } while (go);

  // Close the data file
  in->Close();
  return pSource;
}

size_t PWScore::GetAttachments(const uuid_array_t &entry_uuid,
                               ATRVector &vATRecords)
{
  std::pair<UAMMciter, UAMMciter> uuidairpair;
  vATRecords.clear();

  st_UUID stuuid(entry_uuid);
  size_t num = m_MM_entry_uuid_atr.count(stuuid);
  uuidairpair = m_MM_entry_uuid_atr.equal_range(stuuid);
  if (uuidairpair.first != m_MM_entry_uuid_atr.end()) {
    for (UAMMciter citer = uuidairpair.first; citer != uuidairpair.second; ++citer) {
      if ((citer->second.uiflags & ATT_ATTACHMENT_DELETED) != ATT_ATTACHMENT_DELETED)
        vATRecords.push_back(citer->second);
    }
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

void PWScore::SetAttachments(const uuid_array_t &entry_uuid,
                             ATRVector &vATRecords)
{
  // Delete any existing
  st_UUID stuuid(entry_uuid);
  m_MM_entry_uuid_atr.erase(stuuid);

  // Put back supplied versions
  for (size_t i = 0; i < vATRecords.size(); i++) {
    st_UUID stuuid(entry_uuid);
    m_MM_entry_uuid_atr.insert(make_pair(stuuid, vATRecords[i]));
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
  // 1. Yes - if there are new entries not yet added to attachment file
  // 2. Yes - if there are existing entries with changed flags
  // 3. Yes - if there are existing entries that are no longer required or missing
  //   otherwise No!

  bool bContinue(false);
  for (UAMMciter citer = m_MM_entry_uuid_atr.begin(); citer != m_MM_entry_uuid_atr.end();
       citer++) {
    ATRecord atr = citer->second;
    bool bDelete = bCleanup && ((atr.uiflags & ATT_ATTACHMENT_DELETED) != 0);
    bool bChanged = (atr.uiflags & ATT_ATTACHMENT_FLGCHGD) == ATT_ATTACHMENT_FLGCHGD;

    if ((!bDelete && atr.uncsize != 0) ||
        (bCleanup && bChanged)) {
      bContinue = true;
      break;
    }
  }

  if (!bContinue)
    return PWSAttfile::SUCCESS;

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
  if (status != PWSAttfile::SUCCESS) {
    delete out;
    return status;
  }

  // If there is an existing attachment file, 'make' old file
  if (pws_os::FileExists(current_file)) {
    in = PWSAttfile::MakePWSfile(current_file.c_str(), version,
                                 PWSAttfile::Read, status,
                                 m_pAsker, m_pReporter);

    if (status != PWSAttfile::SUCCESS) {
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

  try { // exception thrown on write error
    // Open new attachment file
    status = out3->Open(GetPassKey());

    if (status != PWSAttfile::SUCCESS) {
      delete out3;
      delete in;
      return status;
    }

    // If present, open current data file
    if (in != NULL) {
      status = in->Open(GetPassKey());

      if (status != PWSAttfile::SUCCESS) {
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

    ATRecord atr;
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
        st_UUID stuuid(atr.attmt_uuid);
        mp_attmt_uuid_atr.insert(make_pair(stuuid, atr));
      }
    }

    // First process all existing attachments
    bool go(true);
    while (go && in != NULL) {
      atr.Clear();
      status = in->ReadAttmntRecord(atr, NULL_UUID);
      switch (status) {
        case PWSAttfile::FAILURE:
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
        case PWSAttfile::SUCCESS:
          // Only need to write it out if we want to keep it
          if (st_attmt_uuid.find(atr.attmt_uuid) != st_attmt_uuid.end()) {
            // Get attachment record
            UAMiter iter = mp_attmt_uuid_atr.find(atr.attmt_uuid);
            if (iter != mp_attmt_uuid_atr.end()) {
              out3->WriteAttmntRecord(atr);
              vATRWritten.push_back(iter->second);
            }
          }
          trashMemory(atr.pData, atr.cmpsize);
          delete [] atr.pData;
          atr.pData = NULL;
          break;
        case PWSAttfile::END_OF_FILE:
          go = false;
          break;
      } // switch
    };

    // Now process new attachments
    for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();
         iter++) {
      ATRecord atr = iter->second;
      // A zero compressed file size says that it hasn't been added yet
      if (atr.cmpsize == 0) {
        // Insert date added in attachment record
        atr.dtime = dtime;

        StringX fname = atr.path + atr.filename;

        // Verify that the file is still there!
        int irc = _taccess(fname.c_str(), F_OK);
        if (irc != 0) {
          // irc = -1 ->
          //   errno = ENOENT (2) - file not found!
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

        // Get file as a string
        unsigned char *pSource = new unsigned char[atr.uncsize];
        int numread = fread(pSource, 1, atr.uncsize, fh);
        fclose(fh);
        if ((unsigned int)numread != atr.uncsize) {
          pws_os::Trace(_T("Attachment file fread - mismatch of data read. Expected=%d, Read=%d\n"),
                   atr.uncsize, numread);
          trashMemory(pSource, atr.uncsize);
          delete pSource;
          break;
        }

        // Get SHA1 hash and CRC of uncompressed data - only need one but..
        // display CRC and user can check with, say, WinZip to see if the same
        SHA1 context;
        context.Update(pSource, atr.uncsize);
        context.Final(atr.digest);
        atr.CRC = PWSUtil::Get_CRC(pSource, atr.uncsize);

        // Compress file
        // NOTE: destination size MUST be at least: source size + 10% + 12
        BYTE *pData(NULL);
        unsigned long ulDestLen, ulSrcLen(atr.uncsize);
        ulDestLen = ulSrcLen + (unsigned long)ceil(ulSrcLen * 0.1E0) + 12L;
        pData = new unsigned char[ulDestLen];

        int zRC;
        zRC = compress(pData, &ulDestLen, pSource, ulSrcLen);
        if (zRC != Z_OK) {
          pws_os::Trace(_T("Attachment file compression failed. RC=%d, Read=%d\n"), zRC);
          trashMemory(pData, ulDestLen);
          delete [] pData;
          trashMemory(pSource, atr.uncsize);
          delete [] pSource;
          break;
        }

        // Update last fields
        atr.cmpsize = ulDestLen;
        atr.pData = pData;

        // Update in-memory records
        iter->second.dtime = atr.dtime;
        memcpy(iter->second.digest, atr.digest, SHA1::HASHLEN);
        iter->second.CRC = atr.CRC;
        iter->second.cmpsize = atr.cmpsize;
        memcpy(iter->second.attmt_uuid, atr.attmt_uuid, sizeof(uuid_array_t));

        // Write out record
        out3->WriteAttmntRecord(atr);
        vATRWritten.push_back(atr);

        // Free file storage
        atr.pData = NULL;  // stop possible double 'free'
        trashMemory(pData, ulDestLen);
        delete [] pData;
        trashMemory(pSource, atr.uncsize);
        delete [] pSource;
      }
    }

  } catch (...) {
    if (in != NULL) {
      in->Close();
      delete in;
    }
    out3->Close();
    delete out3;
    pws_os::DeleteAFile(tempfilename);
    return FAILURE;
  }

  if (in != NULL) {
    in->Close();
    delete in;
  }
  out3->Close();
  delete out3;

  // Remove change flag now that records have been written
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  for (size_t i = 0; i < vATRWritten.size(); i++) {
    if ((vATRWritten[i].uiflags & ATT_ATTACHMENT_FLGCHGD) == 0)
      continue;

    uuidairpair = m_MM_entry_uuid_atr.equal_range(vATRWritten[i].entry_uuid);
    // Now find this specific attachment record
    for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
      if (memcmp(iter->second.attmt_uuid, vATRWritten[i].attmt_uuid, sizeof(uuid_array_t)) == 0) {
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

int PWScore::DuplicateAttachments(const uuid_array_t &old_entry_uuid,
                                  const uuid_array_t &new_entry_uuid,
                                  PWSAttfile::VERSION version)
{
  int status;

  st_UUID stuuid(old_entry_uuid);
  size_t num = m_MM_entry_uuid_atr.count(stuuid);
  if (num == 0)
    return FAILURE;

  /*
    Generate a temporary file name - write to this file.  If OK, rename current
    file and then rename the temporary file to the proper name.
    This does leave the older file around as backup (for the moment).
  */

  PWSAttfile *in(NULL), *out(NULL);
  stringT tempfilename, current_file, timestamp;
  stringT sDrive, sDir, sName, sExt;
  time_t time_now;

  // Generate temporary names
  time(&time_now);
  timestamp = PWSUtil::ConvertToDateTimeString(time_now, TMC_FILE).c_str();
  pws_os::splitpath(m_currfile.c_str(), sDrive, sDir, sName, sExt);
  tempfilename = sDrive + sDir + sName + timestamp +
                 stringT(ATT_DEFAULT_ATTMT_SUFFIX);
  current_file = sDrive + sDir + sName +
                 stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  // If duplicating - they must be there to begin with!
  if (!pws_os::FileExists(current_file))
    return FAILURE;

  // 'Make' new file
  out = PWSAttfile::MakePWSfile(tempfilename.c_str(), version,
                                PWSAttfile::Write, status,
                                m_pAsker, m_pReporter);
  if (status != PWSAttfile::SUCCESS) {
    delete out;
    return status;
  }

  // Must be an existing attachment file, 'make' old file
  in = PWSAttfile::MakePWSfile(current_file.c_str(), version,
                               PWSAttfile::Read, status,
                               m_pAsker, m_pReporter);

  if (status != PWSAttfile::SUCCESS) {
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

  ATRVector vATRWritten, vATRDuplicates;

  try { // exception thrown on write error
    // Open new attachment file
    status = out3->Open(GetPassKey());

    if (status != PWSAttfile::SUCCESS) {
      delete out3;
      delete in;
      return status;
    }

    // Open current data file
    status = in->Open(GetPassKey());

    if (status != PWSAttfile::SUCCESS) {
      out3->Close();
      delete out3;
      delete in;
      return status;
    }

    std::pair<UAMMciter, UAMMciter> uuidairpair;

    // Time stamp for date added for new attachments
    time_t dtime;
    time(&dtime);

    ATRecord atr;
    UUIDSet st_attmt_uuid;         // std::set from records on attmt_uuid
    UUIDATRMap mp_attmt_uuid_atr;  // std::map key = attmnt_uuid, value = attachment record

    // Find all existing attachments we want to keep (atr.cmpsize is filled in)
    for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();
         iter++) {
      if (iter->second.cmpsize != 0) {
        // Update set with the attachments we want to keep.
        st_attmt_uuid.insert(iter->second.attmt_uuid);
        // Save the attachment record
        st_UUID stuuid(iter->second.attmt_uuid);
        mp_attmt_uuid_atr.insert(make_pair(stuuid, iter->second));
      }
    }

    // First process all existing attachments
    bool go(true);
    while (go) {
      status = in->ReadAttmntRecord(atr, NULL_UUID);
      switch (status) {
        case PWSAttfile::FAILURE:
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
        case PWSAttfile::SUCCESS:
          // Only need to write it out if we want to keep it
          if (st_attmt_uuid.find(atr.attmt_uuid) != st_attmt_uuid.end()) {
            // Get record
            UAMiter iter = mp_attmt_uuid_atr.find(atr.attmt_uuid);
            if (iter != mp_attmt_uuid_atr.end()) {
              out3->WriteAttmntRecord(iter->second);
              vATRWritten.push_back(iter->second);

              // Now maybe duplicate it
              if (memcmp(iter->second.entry_uuid, old_entry_uuid, sizeof(uuid_array_t)) == 0) {
                uuid_array_t new_attmt_uuid;
                CUUIDGen attmt_uuid;
                attmt_uuid.GetUUID(new_attmt_uuid);
                ATRecord atr = iter->second;
                // Change date added timestamp even though it was added to original entry
                atr.dtime = dtime;
                memcpy(atr.attmt_uuid, new_attmt_uuid, sizeof(uuid_array_t));
                memcpy(atr.entry_uuid, new_entry_uuid, sizeof(uuid_array_t));
                vATRDuplicates.push_back(atr);
              }
            }
          }
          trashMemory(atr.pData, atr.cmpsize);
          delete [] atr.pData;
          atr.pData = NULL;
          break;
        case PWSAttfile::END_OF_FILE:
          go = false;
          break;
      } // switch
      atr.Clear();
    };

    // Now process any new attachments
    for (UAMMiter iter = m_MM_entry_uuid_atr.begin(); iter != m_MM_entry_uuid_atr.end();
         iter++) {
      ATRecord atr = iter->second;
      // A zero compressed file size says that it hasn't been added yet
      if (atr.cmpsize == 0) {
        // Insert date added in attachment record
        atr.dtime = dtime;

        StringX fname = atr.path + atr.filename;

        // Verify that the file is still there!
        int irc = _taccess(fname.c_str(), F_OK);
        if (irc != 0) {
          // irc = -1 ->
          //   errno = ENOENT (2) - file not found!
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

        // Get file as a string
        unsigned char *pSource = new unsigned char[atr.uncsize];
        int numread = fread(pSource, 1, atr.uncsize, fh);
        fclose(fh);
        if ((unsigned int)numread != atr.uncsize) {
          pws_os::Trace(_T("Attachment file fread - mismatch of data read. Expected=%d, Read=%d\n"),
                   atr.uncsize, numread);
          trashMemory(pSource, atr.uncsize);
          delete pSource;
          break;
        }

        // Get SHA1 hash and CRC of uncompressed data - only need one but..
        // display CRC and user can check with, say, WinZip to see if the same
        SHA1 context;
        context.Update(pSource, atr.uncsize);
        context.Final(atr.digest);
        atr.CRC = PWSUtil::Get_CRC(pSource, atr.uncsize);

        // Compress file
        // NOTE: destination size MUST be at least: source size + 10% + 12
        BYTE *pData(NULL);
        unsigned long ulDestLen, ulSrcLen(atr.uncsize);
        ulDestLen = ulSrcLen + (unsigned long)ceil(ulSrcLen * 0.1E0) + 12L;
        pData = new unsigned char[ulDestLen];

        int zRC;
        zRC = compress(pData, &ulDestLen, pSource, ulSrcLen);
        if (zRC != Z_OK) {
          pws_os::Trace(_T("Attachment file compression failed. RC=%d, Read=%d\n"), zRC);
          trashMemory(pData, ulDestLen);
          delete [] pData;
          trashMemory(pSource, atr.uncsize);
          delete [] pSource;
          break;
        }

        // Update last fields
        atr.cmpsize = ulDestLen;
        atr.pData = pData;

        // Update in-memory records
        iter->second.dtime = atr.dtime;
        memcpy(iter->second.digest, atr.digest, SHA1::HASHLEN);
        iter->second.CRC = atr.CRC;
        iter->second.cmpsize = atr.cmpsize;
        memcpy(iter->second.attmt_uuid, atr.attmt_uuid, sizeof(uuid_array_t));

        // Write out records
        out3->WriteAttmntRecord(atr);
        vATRWritten.push_back(atr);

        // Free file storage
        atr.pData = NULL;  // stop possible double 'free'
        trashMemory(pData, ulDestLen);
        delete [] pData;
        trashMemory(pSource, atr.uncsize);
        delete [] pSource;
        atr.Clear();
      }
    }

  } catch (...) {
    in->Close();
    delete in;
    out3->Close();
    delete out3;
    pws_os::DeleteAFile(tempfilename);
    return FAILURE;
  }

  in->Close();
  out3->Close();
  delete in;
  delete out3;

  // Remove change flag now that records have been written
  std::pair<UAMMiter, UAMMiter> uuidairpair;
  for (size_t i = 0; i < vATRWritten.size(); i++) {
    if ((vATRWritten[i].uiflags & ATT_ATTACHMENT_FLGCHGD) == 0)
      continue;

    uuidairpair = m_MM_entry_uuid_atr.equal_range(vATRWritten[i].entry_uuid);
    // Now find this specific attachment record
    for (UAMMiter iter = uuidairpair.first; iter != uuidairpair.second; ++iter) {
      if (memcmp(iter->second.attmt_uuid, vATRWritten[i].attmt_uuid, sizeof(uuid_array_t)) == 0) {
        iter->second.uiflags &= ~ATT_ATTACHMENT_FLGCHGD;
        break;
      }
    }
  }

  // Now update main multimap with the duplicate attachments
  st_UUID st_newuuid(new_entry_uuid);
  for (size_t i = 0; i < vATRDuplicates.size(); i++) {
    m_MM_entry_uuid_atr.insert(make_pair(st_newuuid, vATRDuplicates[i]));
  }

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
    return PWSAttfile::FAILURE;
  }

  if (m_MM_entry_uuid_atr.size() == 0) {
    // Delete new attachment file if they empty
    pws_os::DeleteAFile(tempfilename);
    return PWSAttfile::SUCCESS;
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
      return PWSAttfile::FAILURE;
    }
  }

  SetChanged(false, false);
  return PWSAttfile::SUCCESS;
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
    CUUIDGen att_uuid;
    att_uuid.GetUUID(attfile_uuid);
    memcpy(m_atthdr.attfile_uuid, attfile_uuid, sizeof(uuid_array_t));
    memcpy(m_atthdr.DBfile_uuid, m_hdr.m_file_uuid_array, sizeof(uuid_array_t));
  }
}

struct XMLAttachmentWriter {
  XMLAttachmentWriter(const ATFVector &vatf,  ofstream &ofs, size_t *pnum_exported,
                      PWScore *pcore)
  : m_of(ofs), m_vatf(vatf), m_pnum_exported(pnum_exported), m_pcore(pcore)
  {
    *m_pnum_exported = 0;
  }

  void operator()(const ATRecordEx &atrex) {
    ItemListIter iter = m_pcore->Find(atrex.atr.entry_uuid);
    if (m_pcore->AttMatches(atrex, m_vatf)) {
      (*m_pnum_exported)++;
      CUTF8Conv utf8conv;
      const unsigned char *utf8 = NULL;
      int utf8Len = 0;

      ostringstream oss; // ALWAYS a string of chars, never wchar_t!
      oss << "\t<attachment id=\"" << dec << (*m_pnum_exported) << "\" >" << endl;

      // NOTE - ORDER MAUST CORRESPOND TO ORDER IN SCHEMA
      if (!atrex.sxGroup.empty())
        PWSUtil::WriteXMLField(oss, "group", atrex.sxGroup, utf8conv);

      PWSUtil::WriteXMLField(oss, "title", atrex.sxTitle, utf8conv);

      if (!atrex.sxUser.empty())
        PWSUtil::WriteXMLField(oss, "username", atrex.sxUser, utf8conv);

      uuid_array_t attachment_uuid, entry_uuid;
      memcpy(attachment_uuid, atrex.atr.attmt_uuid, sizeof(uuid_array_t));
      memcpy(entry_uuid, atrex.atr.entry_uuid, sizeof(uuid_array_t));
      const CUUIDGen a_uuid(attachment_uuid), e_uuid(entry_uuid);
      oss << "\t\t<attachment_uuid><![CDATA[" << a_uuid << "]]></attachment_uuid>" << endl;
      oss << "\t\t<entry_uuid><![CDATA[" << e_uuid << "]]></entry_uuid>" << endl;

      PWSUtil::WriteXMLField(oss, "filename", atrex.atr.filename, utf8conv);
      PWSUtil::WriteXMLField(oss, "path", atrex.atr.path, utf8conv);
      PWSUtil::WriteXMLField(oss, "description", atrex.atr.description, utf8conv);

      oss << "\t\t<osize>" << dec << atrex.atr.uncsize << "</osize>" << endl;
      oss << "\t\t<csize>" << dec << atrex.atr.cmpsize << "</csize>" << endl;

      oss << "\t\t<crc>" << hex << setfill('0') << setw(8)
          << atrex.atr.CRC << "</crc>" << endl;

      stringT tmp, temp;
      // add digest of original file
      for (unsigned int i = 0; i < SHA1::HASHLEN; i++) {
        Format(temp, _T("%02x"), atrex.atr.digest[i]);
        tmp += temp;
      }
      utf8conv.ToUTF8(tmp.c_str(), utf8, utf8Len);
      oss << "\t\t<odigest>" << utf8 << "</odigest>" << endl;

      tmp.clear();
      temp.clear();
      // add digest of compressed file
      unsigned char cdigest[SHA1::HASHLEN];
      SHA1 context;
      context.Update(atrex.atr.pData, atrex.atr.cmpsize);
      context.Final(cdigest);
      for (unsigned int i = 0; i < SHA1::HASHLEN; i++) {
        Format(temp, _T("%02x"), cdigest[i]);
        tmp += temp;
      }
      utf8conv.ToUTF8(tmp.c_str(), utf8, utf8Len);
      oss << "\t\t<cdigest>" << utf8 << "</cdigest>" << endl;

      oss << PWSUtil::GetXMLTime(2, "ctime", atrex.atr.ctime, utf8conv);
      oss << PWSUtil::GetXMLTime(2, "atime", atrex.atr.atime, utf8conv);
      oss << PWSUtil::GetXMLTime(2, "mtime", atrex.atr.mtime, utf8conv);
      oss << PWSUtil::GetXMLTime(2, "dtime", atrex.atr.dtime, utf8conv);

      if (atrex.atr.flags != 0) {
        oss << "\t\t<flags>" << endl;
        if ((atrex.atr.flags & ATT_EXTRACTTOREMOVEABLE) == ATT_EXTRACTTOREMOVEABLE)
          oss << "\t\t\t<ExtractToRemoveable>1</ExtractToRemoveable>" << endl;
        if ((atrex.atr.flags & ATT_ERASEPGMEXISTS) == ATT_ERASEPGMEXISTS)
          oss << "\t\t\t<EraseProgamExists>1</EraseProgamExists>" << endl;
        if ((atrex.atr.flags & ATT_ERASEONDBCLOSE) == ATT_ERASEONDBCLOSE)
          oss << "\t\t\t<EraseOnDatabaseClose>1</EraseOnDatabaseClose>" << endl;
        oss << "\t\t</flags>" << endl;
      }

      tmp = PWSUtil::Base64Encode(atrex.atr.pData, atrex.atr.cmpsize).c_str();
      utf8conv.ToUTF8(tmp.c_str(), utf8, utf8Len);
      oss << "\t\t<data><![CDATA[" << endl;
      for (unsigned int i = 0; i < (unsigned int)utf8Len; i += 64) {
        char buffer[65];
        memset(buffer, 0, sizeof(buffer));
        if (utf8Len - i > 64)
          memcpy(buffer, utf8 + i, 64);
        else
          memcpy(buffer, utf8 + i, utf8Len - i);
        oss << "\t\t" << buffer << endl;
      }
      oss << "\t\t]]></data>" << endl;
      oss << "\t</attachment>" << endl;
      m_of.write(oss.str().c_str(),
                 static_cast<streamsize>(oss.str().length()));
    }
  }

private:
  XMLAttachmentWriter& operator=(const XMLAttachmentWriter&); // Do not implement
  const ATFVector &m_vatf;
  ofstream &m_of;
  size_t *m_pnum_exported;
  PWScore *m_pcore;
};

int PWScore::WriteXMLAttachmentFile(const StringX &filename, ATFVector &vatf,
                                    const ATRExVector &vAIRecordExs,
                                    size_t *pnum_exported)
{
  size_t num = vAIRecordExs.size();
  if (num == 0)
    return NO_ENTRIES_EXPORTED;

#ifdef UNICODE
  const unsigned char *fname = NULL;
  CUTF8Conv conv;
  int fnamelen;
  conv.ToUTF8(filename, fname, fnamelen); 
#else
  const char *fname = filename.c_str();
#endif

  ofstream ofs(reinterpret_cast<const char *>(fname));

  if (!ofs)
    return CANT_OPEN_FILE;

  oStringXStream oss_xml;
  StringX pwh, temp;
  time_t time_now;

  time(&time_now);
  const StringX now = PWSUtil::ConvertToDateTimeString(time_now, TMC_XML);

  ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
  ofs << "<?xml-stylesheet type=\"text/xsl\" href=\"pwsafe.xsl\"?>" << endl;
  ofs << endl;
  ofs << "<passwordsafe_attachments" << endl;
  temp = m_currfile;
  Replace(temp, StringX(_T("&")), StringX(_T("&amp;")));

  CUTF8Conv utf8conv;
  const unsigned char *utf8 = NULL;
  int utf8Len = 0;

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
    utf8conv.ToUTF8(wls.c_str(), utf8, utf8Len);
    ofs << "WhenLastSaved=\"";
    ofs.write(reinterpret_cast<const char *>(utf8), utf8Len);
    ofs << "\"" << endl;
  }

  CUUIDGen db_uuid(m_hdr.m_file_uuid_array, true); // true to print canoncally
  CUUIDGen att_uuid(m_atthdr.attfile_uuid, true); // true to print canoncally

  ofs << "Database_uuid=\"" << db_uuid << "\"" << endl;
  ofs << "Attachment_file_uuid=\"" << att_uuid << "\"" << endl;
  ofs << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << endl;
  ofs << "xsi:noNamespaceSchemaLocation=\"pwsafe.xsd\">" << endl;
  ofs << endl;

  XMLAttachmentWriter put_xml(vatf, ofs, pnum_exported, this);

  for_each(vAIRecordExs.begin(), vAIRecordExs.end(), put_xml);

  ofs << "</passwordsafe_attachments>" << endl;
  ofs.close();

 return SUCCESS;
}

#if !defined(USE_XML_LIBRARY) || (!defined(_WIN32) && USE_XML_LIBRARY == MSXML)
// Don't support importing XML on non-Windows platforms using Microsoft XML libraries
int PWScore::ImportXMLAttachmentFile(const stringT &,
                           const stringT &,
                           stringT &, stringT &,
                           int &, int &, int &,
                           CReport &, Command *&)
{
  return UNIMPLEMENTED;
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

  strXMLErrors = _T("");

  // Expat is not a validating parser - but we now do it ourselves!
  validation = true;
  status = iXML.Process(validation, strXMLFileName, strXSDFileName);
  strXMLErrors = iXML.getXMLErrors();

  if (!status) {
    return XML_FAILED_VALIDATION;
  }
  numValidated = iXML.getNumAttachmentsValidated();

  validation = false;
  status = iXML.Process(validation, strXMLFileName,
                        strXSDFileName);

  numImported = iXML.getNumAttachmentsImported();
  numSkipped = iXML.getNumAttachmentsSkipped();

  strXMLErrors = iXML.getXMLErrors();
  strSkippedList = iXML.getSkippedList();

  if (!status) { 
    delete pcommand;
    pcommand = NULL;
    return XML_FAILED_IMPORT;
  }

  if (numImported > 0)
    SetDBChanged(true);

  return SUCCESS ;
}
#endif

struct get_att_uuid {
  get_att_uuid(UUIDAVector &vatt_uuid)
  :  m_vatt_uuid(vatt_uuid)
  {}

  void operator()(std::pair<const st_UUID, ATRecord> const& p) const {
    st_UUID st(p.second.attmt_uuid);
    m_vatt_uuid.push_back(st);
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
