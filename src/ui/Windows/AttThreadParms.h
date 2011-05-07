/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file AttThreadParms.h
//-----------------------------------------------------------------------------

#pragma once

// Thread parameters
enum {INVALID = -1, READ, WRITE, DUPLICATE, COMPLETE_XML_IMPORT,
                       XML_EXPORT, GET};

#include "core/StringX.h"
#include "core/attachments.h"
#include "core/PWSAttfile.h"

#include "os/UUID.h"  // for uuid_array_t

class DboxMain;
class CAttProgressDlg;
class PWScore;

struct ATThreadParms {
  ATThreadParms()
  : status(0), function(INVALID), pDbx(NULL), pProgressDlg(NULL), pcore(NULL),
  bVerify(false), bCleanup(false), impfilename(_T("")), newfile(_T("")),
  filename(_T("")), num_exported(0), version(PWSAttfile::VCURRENT)
  {
  }

  ATThreadParms(const ATThreadParms &thpms)
    : status(thpms.status), function(thpms.function), pDbx(thpms.pDbx),
    pProgressDlg(thpms.pProgressDlg), pcore(thpms.pcore),
    bVerify(thpms.bVerify), bCleanup(thpms.bCleanup),
    impfilename(thpms.impfilename), newfile(thpms.newfile),
    atr(thpms.atr), filename(thpms.filename), vatf(thpms.vatf),
    vAIRecordExs(thpms.vAIRecordExs), num_exported(thpms.num_exported),
    version(thpms.version),
    old_entry_uuid(thpms.old_entry_uuid), new_entry_uuid(thpms.new_entry_uuid)
  {
  }

  ATThreadParms &operator=(const ATThreadParms &thpms)
  {
    if (this != &thpms) {
      status = thpms.status;
      function = thpms.function;
      pDbx = thpms.pDbx;
      pProgressDlg = thpms.pProgressDlg;
      pcore = thpms.pcore;

      bVerify = thpms.bVerify;
      bCleanup = thpms.bCleanup;

      impfilename = thpms.impfilename;

      newfile = thpms.newfile;
      atr = thpms.atr;

      filename = thpms.filename;
      vatf = thpms.vatf;
      vAIRecordExs = thpms.vAIRecordExs;
      num_exported = thpms.num_exported;

      version = thpms.version;

      old_entry_uuid = thpms.old_entry_uuid;
      new_entry_uuid = thpms.new_entry_uuid;
    }
    return *this;
  }

  int function;
  int status;
  DboxMain *pDbx;
  CAttProgressDlg *pProgressDlg;
  PWScore *pcore;

  // Read Attachment file
  bool bVerify;

  // Write Attachment file
  bool bCleanup;

  // Duplicate Attachments
  pws_os::CUUID old_entry_uuid;
  pws_os::CUUID new_entry_uuid;

  // Complete XML Import
  stringT impfilename;

  // Get Attachment
  ATRecord atr;
  stringT newfile;

  // Write Attachment XML File
  StringX filename;
  ATFVector vatf;
  ATRExVector vAIRecordExs;
  size_t num_exported;

  // Version used by: WRITE, Duplicate, Complete XML,
  PWSAttfile::VERSION version;
};
