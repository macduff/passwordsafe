/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// file MainAttachments.cpp
//
// Attachment-related methods of DboxMain
//-----------------------------------------------------------------------------

#include "PasswordSafe.h"

#include "ThisMfcApp.h"
#include "DboxMain.h"
#include "PWFileDialog.h"
#include "GeneralMsgBox.h"
#include "ExtractAttachment.h"
#include "ViewAttachments.h"

#include "resource3.h"  // String resources

#include "corelib/attachments.h"

#include "os/file.h"
#include "os/dir.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool DboxMain::GetNewAttachmentInfo(ATRecord &atr, const bool bGetFileName)
{
  StringX sx_Filename;
  CString cs_text(MAKEINTRESOURCE(IDS_CHOOSEATTACHMENT));
  std::wstring dir = L"";
  wchar_t strPath[MAX_PATH] = {0};

  BOOL brc = SHGetSpecialFolderPath(NULL, strPath, CSIDL_PERSONAL, FALSE);
  ASSERT(brc == TRUE);
  if (brc == TRUE)
    dir = strPath;

  while (1) {
    // Open-type dialog box (with extra description edit control)
    INT_PTR rc2(IDOK);
    StringX sFullFileName = atr.filename;

    if (bGetFileName) {
      CPWFileDialog fd(TRUE,
                       NULL,
                       NULL,
                       OFN_FILEMUSTEXIST | OFN_LONGNAMES | OFN_DONTADDTORECENT |
                          OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
                       CString(MAKEINTRESOURCE(IDS_FDF_ALL)),
                       this, 0, true);

      fd.m_ofn.lpstrTitle = cs_text;
      if (!dir.empty())
        fd.m_ofn.lpstrInitialDir = dir.c_str();

      rc2 = fd.DoModal();
      if (rc2 == IDOK) {
        // Note: Using fd.GetFileName() for the filename may truncate the extension
        // to 3 characters
        sFullFileName = (LPCWSTR)fd.GetPathName();
        atr.description = fd.GetDescription();
      }
    }

    if (m_inExit) {
      // If U3ExitNow called while in CPWFileDialog,
      // PostQuitMessage makes us return here instead
      // of exiting the app. Try resignalling
      PostQuitMessage(0);
      return false;
    }

    if (rc2 == IDOK) {
      std::wstring sDrive, sDir, sName, sExt;
      pws_os::splitpath(sFullFileName.c_str(), sDrive, sDir, sName, sExt);
      atr.path = StringX(sDrive.c_str()) + StringX(sDir.c_str());
      atr.filename = StringX(sName.c_str()) + StringX(sExt.c_str());
      atr.flags = 0;
      atr.uiflags = 0;
      struct _stat stat_buffer;
      int irc = _wstat(sFullFileName.c_str(), &stat_buffer);
      if (irc == 0) {
        if (stat_buffer.st_size == 0) {
          CGeneralMsgBox gmb;
          CString cs_msg(MAKEINTRESOURCE(IDS_NOZEROSIZEEXTRACT)),
              cs_title(MAKEINTRESOURCE(IDS_WILLNOTATTACH));
          gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONSTOP);
        } else {
          atr.mtime = stat_buffer.st_mtime;
          atr.atime = stat_buffer.st_atime;
          atr.ctime = stat_buffer.st_ctime;
          atr.dtime = 0;
          atr.uncsize = stat_buffer.st_size;
          atr.cmpsize = 0;
          atr.CRC = 0;
          uuid_array_t new_attmt_uuid;
          CUUIDGen attmt_uuid;
          attmt_uuid.GetUUID(new_attmt_uuid);
          memcpy(atr.attmt_uuid, new_attmt_uuid, sizeof(uuid_array_t));
          memset(atr.entry_uuid, 0, sizeof(atr.entry_uuid));
          memset(atr.digest, 0, SHA1::HASHLEN);
          return true;
        }
      }
    } else {
      // Can inly be the user pressed cancel
      return false;
    }
  }
  return false;
}

void DboxMain::OnExtractAttachment()
{
  // Go and retrieve the attachment for the user
  CItemData *pci(NULL);
  if (SelItemOk() == TRUE) {
    pci = getSelectedItem();
    ASSERT(pci != NULL);

    if (pci->IsShortcut())
      pci = GetBaseEntry(pci);
  } else
    return;

  CExtractAttachment dlg(this);

  uuid_array_t entry_uuid;
  pci->GetUUID(entry_uuid);
  ATRVector vATRecords;
  m_core.GetAttachments(entry_uuid, vATRecords);
  dlg.SetAttachments(vATRecords);

  // Now let them decide where to extract it to and the new file name
  // and then do it
  dlg.DoModal();
}

void DboxMain::OnViewAttachments()
{
  CViewAttachments dlg(this);

  ATRExVector vAIRecordExs;
  m_core.GetAllAttachments(vAIRecordExs);
  dlg.SetExAttachments(vAIRecordExs);

  // Now let them decide where to extract it to and the new file name
  // and then do it
  dlg.DoModal();
}

LRESULT DboxMain::OnExtractAttachment(WPARAM wParam, LPARAM )
{
  ATRecord *pATR = (ATRecord *)wParam;
  DoAttachmentExtraction(*pATR);

  return 0L;
}

void DboxMain::DoAttachmentExtraction(const ATRecord &atr)
{
  unsigned char *pSource(NULL);
  CGeneralMsgBox gmb;
  CString cs_msg, cs_title, csFilter;
  StringX sxFile;
  std::wstring sDrv, sDir, sFname, sExt, sxFilter, wsNewName(L"");
  int status;
  SHA1 context;
  BOOL brc;

  // If user mandated that a secure delete program is there, first check that they have
  // specified one and then check it exists on this system.
  if ((atr.flags & ATT_ERASEPGMEXISTS) == ATT_ERASEPGMEXISTS) {
    StringX sxErasePgm = PWSprefs::GetInstance()->GetPref(PWSprefs::EraseProgram);
    if (sxErasePgm.empty() || !pws_os::FileExists(sxErasePgm.c_str())) {
      cs_msg.LoadString(IDS_NOERASEPGM);
      cs_title.LoadString(IDS_WILLNOTEXTRACT);
      gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONSTOP);
      goto exit;
    }
  }

  sxFile = atr.path + atr.filename;
  pws_os::splitpath(sxFile.c_str(), sDrv, sDir, sFname, sExt);
  bool bNoExtn = sExt.empty();
  cs_title.LoadString(IDS_EXTRACTTITLE);
  if (bNoExtn) {
    csFilter.LoadString(IDS_FDF_ALL);
  } else {
    csFilter.Format(IDS_ATTACHMENT_EXTN, sExt.c_str(), sExt.c_str());
  }

  while (1) {
    CPWFileDialog fd(FALSE,
                     bNoExtn ? NULL : sExt.substr(1).c_str(),  // skip leading '.'
                     sxFile.c_str(),
                     OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
                        OFN_LONGNAMES | OFN_OVERWRITEPROMPT,
                     csFilter,
                     this);
    fd.m_ofn.lpstrTitle = cs_title;
    INT_PTR rc = fd.DoModal();
    if (ExitRequested()) {
      // If U3ExitNow called while in CPWFileDialog,
      // PostQuitMessage makes us return here instead
      // of exiting the app. Try resignalling
      ::PostQuitMessage(0);
      return;
    }

    if (rc == IDOK) {
      wsNewName = fd.GetPathName();
      // If user mandated that it can only be extracted to a removeable device
      // check that they have selected such a device
      if ((atr.flags & ATT_EXTRACTTOREMOVEABLE) == ATT_EXTRACTTOREMOVEABLE) {
        std::wstring wsNewDrive, wsNewDir, wsNewFileName, wsNewExt;

        pws_os::splitpath(wsNewName, wsNewDrive, wsNewDir, wsNewFileName, wsNewExt);
        wsNewDrive += L"\\";

        UINT uiDT = GetDriveType(wsNewDrive.c_str());
        // Do not allow unless to a removeable device
        if (uiDT != DRIVE_REMOVABLE && uiDT != DRIVE_RAMDISK) {
          CGeneralMsgBox gmb;
          cs_msg.LoadString(IDS_NOTREMOVEABLE);
          cs_title.LoadString(IDS_WILLNOTEXTRACT);
          gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONSTOP);
          goto exit;
        }
      } else {
        break;
      }
    } else
      return;
  }

  if (wsNewName.empty())
    goto exit;

  // Long job
  BeginWaitCursor();

  pSource = GetAttachment(atr, status);

  if (status != PWSAttfile::SUCCESS || pSource == NULL) {
    // Error message to user
    EndWaitCursor();
    cs_msg.LoadString(IDS_CANTGETATTACHMENT);
    cs_title.LoadString(IDS_WILLNOTEXTRACT);
    gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONINFORMATION);
    goto exit;
  }

  // Now verify CRC and SHA1 hash of data retrieved
  unsigned char digest[SHA1::HASHLEN];
  context.Update(pSource, atr.uncsize);
  context.Final(digest);
  unsigned long CRC = PWSUtil::Get_CRC(pSource, atr.uncsize);
  if (CRC != atr.CRC ||
      memcmp(digest, atr.digest, SHA1::HASHLEN) != 0) {
    // Need  our own CGeneralMsgBox as may use one defined above at end and
    // it can only be used once.
    // CRC and/or SHA1 Hash mis-match, ask user whether to continue anyway
    EndWaitCursor();
    CGeneralMsgBox gmb;
    cs_msg.LoadString(IDS_CONTINUEEXTRACT);
    cs_title.LoadString(IDS_CRCHASHERROR);
    int rc = gmb.MessageBox(cs_msg, cs_title,
                           MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
    if (rc == IDNO)
      goto exit;
  }
  
  // Open the file
  HANDLE hFile;
  hFile = CreateFile(wsNewName.c_str(), GENERIC_WRITE, 
                         0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    // Error message to user
    EndWaitCursor();
    cs_msg.LoadString(IDS_CANTCREATEFILE);
    cs_title.LoadString(IDS_WILLNOTEXTRACT);
    gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONINFORMATION);
    goto exit;
  }

  // Write out the data
  DWORD numwritten(0);
  brc = WriteFile(hFile, pSource, atr.uncsize, &numwritten, NULL);
  if (brc == 0 || (unsigned int)numwritten != atr.uncsize) {
    // Error message to user
    CloseHandle(hFile);
    EndWaitCursor();
    cs_msg.LoadString(IDS_BADATTACHMENTWRITE);
    cs_title.LoadString(IDS_EXTRACTED);
    gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONINFORMATION);
    goto exit;
  }

  // Set times back to original
  FILETIME ct, at, mt;
  LONGLONG LL;  // Note that LONGLONG is a 64-bit integer value

  LL = Int32x32To64(atr.ctime, 10000000) + 116444736000000000;
  ct.dwLowDateTime = (DWORD)LL;
  ct.dwHighDateTime = (DWORD)(LL >> 32);

  LL = Int32x32To64(atr.atime, 10000000) + 116444736000000000;
  at.dwLowDateTime = (DWORD)LL;
  at.dwHighDateTime = (DWORD)(LL >> 32);

  LL = Int32x32To64(atr.mtime, 10000000) + 116444736000000000;
  mt.dwLowDateTime = (DWORD)LL;
  mt.dwHighDateTime = (DWORD)(LL >> 32);

  //write the new creation, accessed, and last written time
  SetFileTime(hFile, &ct, &at, &mt);
  CloseHandle(hFile);

  EndWaitCursor();

  cs_msg.LoadString(IDS_EXTRACTOK);
  cs_title.LoadString(IDS_EXTRACTED);
  gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONINFORMATION);

exit:
  if (pSource != NULL) {
    trashMemory(pSource, atr.uncsize);
    delete pSource;
  }

  return;
}
