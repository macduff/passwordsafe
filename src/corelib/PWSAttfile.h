/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef __PWSFILEA_H
#define __PWSFILEA_H

// PWSfile.h
// Abstract the gory details of reading and writing an encrypted database
//-----------------------------------------------------------------------------

#include <stdio.h> // for FILE *
#include <vector>

#include "attachments.h"
#include "UUIDGen.h"
#include "UnknownField.h"
#include "StringX.h"
#include "Proxy.h"
#include "sha256.h"

#include "coredefs.h"

#define MIN_HASH_ITERATIONS 2048

#define ATT_DEFAULT_ATTMT_SUFFIX   _T(".psatt3")
#define ATT_DEFAULT_ATTBKUP_SUFFIX _T(".ibakatt3")


typedef std::map<st_UUID, ATRecord> UUIDATRMap;
typedef UUIDATRMap::iterator UAMiter;

typedef std::multimap<st_UUID, ATRecord> UUIDATRMMap;
typedef UUIDATRMMap::const_iterator UAMMciter;
typedef UUIDATRMMap::iterator UAMMiter;

class Fish;
class Asker;

class PWSAttfile
{
public:
  enum VERSION {V30, VCURRENT = V30,
    NEWFILE = 98,
    UNKNOWN_VERSION = 99};

  enum RWmode {Read, Write};

  enum {SUCCESS = 0, FAILURE = 1,
    UNSUPPORTED_VERSION,                     //  2
    WRONG_VERSION,                           //  3
    NOT_PWS3_FILE,                           //  4
    WRONG_PASSWORD,                          //  5 - see PWScore.h
    BAD_DIGEST,                              //  6 - see PWScore.h
    END_OF_FILE,                             //  7
    CANT_OPEN_FILE,                          //  10 - see PWScore.h
    HEADERS_INVALID = 81,                    //  81
    BAD_ATTACHMENT,                          //  82
  };

  /**
  * The format defines a handful of fields in the file's header
  * Since the application needs these after the PWSfile object's
  * lifetime, it makes sense to define a nested header structure that
  * the app. can keep a copy of, rather than duplicating
  * data members, getters and setters willy-nilly.
  */

  struct AttHeaderRecord {
    AttHeaderRecord();
    AttHeaderRecord(const AttHeaderRecord &ahr);
    AttHeaderRecord &operator =(const AttHeaderRecord &ahr);

    int nITER; // Formally not part of the header.

    unsigned short nCurrentMajorVersion, nCurrentMinorVersion;
    uuid_array_t attfile_uuid;
    uuid_array_t DBfile_uuid;

    time_t whenlastsaved;   // When last saved
    StringX whatlastsaved;  // and by what application
    StringX lastsavedby;    // and by whom
    StringX lastsavedon;    // and by which machine
  };

  enum AttachmentFields {
    ATTMT_UUID         = 0x01,
    ATTMT_ENTRY_UUID   = 0x02,
    ATTMT_FLAGS        = 0x03,
    ATTMT_FNAME        = 0x04,
    ATTMT_PATH         = 0x05,
    ATTMT_DESC         = 0x06,
    ATTMT_USIZE        = 0x07,
    ATTMT_CSIZE        = 0x08,
    ATTMT_CRC          = 0x09,
    ATTMT_DIGEST       = 0x0a,
    ATTMT_CTIME        = 0x0b,
    ATTMT_ATIME        = 0x0c,
    ATTMT_MTIME        = 0x0d,
    ATTMT_DTIME        = 0x0e,
    ATTMT_LAST,
    ATTMT_DATA         = 0xfe,
    ATTMT_END          = 0xff
  };

  static PWSAttfile *MakePWSfile(const StringX &a_filename, VERSION &version,
                                 RWmode mode, int &status,
                                 Asker *pAsker = NULL, Reporter *pReporter = NULL);

  static VERSION ReadVersion(const StringX &filename);
  static int CheckPasskey(const StringX &filename,
                          const StringX &passkey, VERSION &version);

  virtual ~PWSAttfile();

  virtual int Open(const StringX &passkey) = 0;
  virtual int Close();

  virtual int ReadAttmntRecord(ATRecord &atr, const uuid_array_t &data_uuid,
                               const bool bSkipAll = false,
                               const bool bSkipUnless = true) = 0;
  virtual int WriteAttmntRecord(const ATRecord &atr) = 0;

  const AttHeaderRecord &GetHeader() const {return m_atthdr;}
  void SetHeader(const AttHeaderRecord &ah) {m_atthdr = ah;}

  void SetCurVersion(VERSION v) {m_curversion = v;}

protected:
  PWSAttfile(const StringX &filename, RWmode mode);
  void FOpen(); // calls right variant of m_fd = fopen(m_filename);
  virtual size_t WriteCBC(unsigned char type, const StringX &data) = 0;
  virtual size_t WriteCBC(unsigned char type, const unsigned char *data,
                          unsigned int length);
  virtual size_t ReadCBC(unsigned char &type, unsigned char* &data,
                         unsigned int &length,
                         bool bSkip = false, unsigned char *pSkipTypes = NULL);

  const StringX m_filename;
  StringX m_passkey;
  FILE *m_fd;
  VERSION m_curversion;
  const RWmode m_rw;
  unsigned char *m_IV; // points to correct m_ipthing for *CBC()
  Fish *m_fish;
  unsigned char *m_terminal;
  AttHeaderRecord m_atthdr;

  size_t m_fileLength;
  Asker *m_pAsker;
  Reporter *m_pReporter;
};

#endif /* __PWSFILEA_H */
