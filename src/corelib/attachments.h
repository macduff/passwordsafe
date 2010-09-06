/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// attachments.h
//-----------------------------------------------------------------------------

#ifndef __ATTACHMENTS_H
#define __ATTACHMENTS_H

#include "UUIDGen.h"
#include "StringX.h"
#include "sha1.h"

#include <time.h> // for time_t
#include <vector>

extern void trashMemory(void* buffer, size_t length);

/*
Name                        Value        Type
----------------------------------------------------
UUID of this entry          0x01        UUID
UUID of associated entry    0x02        UUID
Flags                       0x03        2 characters
Attachment filename.ext     0x04        Text
Attachment original path    0x05        Text
Attachment description      0x06        Text
Attachment original size    0x07        4 bytes
Attachment compressed size  0x08        4 bytes
Attachment CRC              0x09        4 bytes
Attachment SHA1 digest      0x0a        20 characters
Attachment Create Time      0x0b        time_t
Attachment Last Access Time 0x0c        time_t
Attachment Modifed Time     0x0d        time_t
Date Attachment added       0x0e        time_t
Data                        0xfe        Text
End of Entry                0xff        [empty]
*/

// flags - Note: ATT_ERASEONDBCLOSE not yet implemented
#define ATT_EXTRACTTOREMOVEABLE 0x80
#define ATT_ERASEPGMEXISTS      0x40
#define ATT_ERASEONDBCLOSE      0x20
// Unused                       0x1f

// uiflags - Internal indicators (not in record in file)
#define ATT_ATTACHMENT_FLGCHGD  0x80
#define ATT_ATTACHMENT_DELETED  0x40
// Unused                       0x3f

// Values for matching criteria for Export to XML
enum {ATTGROUP, ATTTITLE, ATTGROUPTITLE, ATTUSER, ATTPATH, ATTFILENAME, ATTDESCRIPTION};

struct ATRecord {
  ATRecord()
  : uncsize(0), cmpsize(0), CRC(0), flags(0), uiflags(0),
    ctime(0), atime(0), mtime(0), dtime(0),
    filename(_T("")), path(_T("")), description(_T("")), pData(NULL)
  {
    memset(entry_uuid, 0, sizeof(uuid_array_t));
    memset(attmt_uuid, 0, sizeof(uuid_array_t));
    memset(digest, 0, SHA1::HASHLEN);
  }

  ~ATRecord()
  {
    delete [] pData;
  }

  ATRecord(const ATRecord &atr)
    : uncsize(atr.uncsize), cmpsize(atr.cmpsize), CRC(atr.CRC),
    flags(atr.flags), uiflags(atr.uiflags),
    ctime(atr.ctime), atime(atr.atime), mtime(atr.mtime), dtime(atr.dtime),
    filename(atr.filename), path(atr.path), description(atr.description)
  {
    if (atr.cmpsize != 0 && atr.pData != NULL) {
      pData = new unsigned char[atr.cmpsize];
      memcpy(pData, atr.pData, atr.cmpsize);
    } else {
      pData = NULL;
    }

    memcpy(entry_uuid, atr.entry_uuid, sizeof(uuid_array_t));
    memcpy(attmt_uuid, atr.attmt_uuid, sizeof(uuid_array_t));
    memcpy(digest, atr.digest, SHA1::HASHLEN);
  }

  ATRecord &operator =(const ATRecord &atr)
  {
    if (this != &atr) {
      uncsize = atr.uncsize;
      cmpsize = atr.cmpsize;

      if (cmpsize != 0 && pData != NULL) {
        trashMemory(pData, cmpsize);
        delete [] pData;
      }

      if (cmpsize != 0 && atr.pData != NULL) {
        pData = new unsigned char[cmpsize];
        memcpy(pData, atr.pData, cmpsize);
      } else
        pData = NULL;

      CRC = atr.CRC;
      flags = atr.flags;
      uiflags = atr.uiflags;
      ctime = atr.ctime;
      atime = atr.atime;
      mtime = atr.mtime;
      dtime = atr.dtime;
      filename = atr.filename;
      path = atr.path;
      description = atr.description;
      memcpy(entry_uuid, atr.entry_uuid, sizeof(uuid_array_t));
      memcpy(attmt_uuid, atr.attmt_uuid, sizeof(uuid_array_t));
      memcpy(digest, atr.digest, SHA1::HASHLEN);
    }
    return *this;
  }

  void Clear() {
    uncsize = cmpsize = 0;
    flags = uiflags = 0;
    CRC = 0;
    ctime = atime = mtime = dtime = 0;
    filename = path = description = _T("");
    memset(entry_uuid, 0, sizeof(uuid_array_t));
    memset(attmt_uuid, 0, sizeof(uuid_array_t));
    memset(digest, 0, SHA1::HASHLEN);

    if (pData != NULL) {
      delete [] pData;
      pData = NULL;
    }
  }

  unsigned int uncsize;
  unsigned int cmpsize;
  unsigned int CRC;
  time_t ctime;
  time_t atime;
  time_t mtime;
  time_t dtime;
  BYTE flags;
  BYTE uiflags;   // Internal flags not in record in file
  unsigned char *pData;
  unsigned char digest[SHA1::HASHLEN];
  StringX filename;
  StringX path;
  StringX description;

  uuid_array_t attmt_uuid;
  uuid_array_t entry_uuid;
};

struct ATRecordEx {
  ATRecordEx()
  : sxGroup(_T("")), sxTitle(_T("")), sxUser(_T(""))
  {
    atr.Clear();
  }

  ATRecordEx(const ATRecordEx &atrex)
    : atr(atrex.atr), sxGroup(atrex.sxGroup),
      sxTitle(atrex.sxTitle), sxUser(atrex.sxUser)
  {
  }

  ATRecordEx &operator =(const ATRecordEx &atrex)
  {
    if (this != &atrex) {
      atr = atrex.atr;
      sxGroup = atrex.sxGroup;
      sxTitle = atrex.sxTitle;
      sxUser = atrex.sxUser;
    }
    return *this;
  }

  void Clear() {
    atr.Clear();
    sxGroup = sxTitle = sxUser = _T("");
  }

  ATRecord atr;
  StringX sxGroup;
  StringX sxTitle;
  StringX sxUser;
};

struct ATFilter {
  ATFilter()
  : set(0), object(0), function(0),
    value(_T(""))
  {}

  ATFilter(const ATFilter &af)
    : set(af.set), object(af.object), function(af.function),
    value(af.value)
  {}

  ATFilter &operator =(const ATFilter &af)
  {
    if (this != &af) {
      set = af.set;
      object = af.object;
      function = af.function;
      value = af.value;
    }
    return *this;
  }

  void Clear()  {
    set = object = function = 0;
    value = _T("");
  }

  int set;
  int object;
  int function;
  stringT value;
};

typedef std::vector<ATRecord> ATRVector;
typedef std::vector<ATRecordEx> ATRExVector;
typedef std::vector<ATFilter> ATFVector;

typedef std::vector<ATRecord>::iterator ATRViter;
typedef std::vector<ATRecordEx>::iterator ATRExViter;
typedef std::vector<ATFilter>::iterator ATFViter;


#endif /* __ATTACHMENTS_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
