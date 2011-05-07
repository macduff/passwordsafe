/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
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
#include "ExportAttDlg.h"

#include "resource3.h"  // String resources

#include "core/attachments.h"
#include "core/PWSdirs.h"
#include "core/core.h"
#include "core/return_codes.h"

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
  // Check if we have read access - if not - quick exit
  if (!bGetFileName && _waccess_s(atr.filename.c_str(), R_OK) != 0) {
    CGeneralMsgBox gmb;
    CString cs_msg, cs_title(MAKEINTRESOURCE(IDS_WILLNOTATTACH));
    cs_msg.Format(IDS_NOREADACCESS, atr.filename.c_str());
    gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONEXCLAMATION);
    return false;
  }

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
      // Quick exit if we don't have read access (but only if
      // we had to find the file namse as already checked if given it)
      if (bGetFileName && _waccess_s(sFullFileName.c_str(), R_OK) != 0) {
        CGeneralMsgBox gmb;
        CString cs_msg, cs_title(MAKEINTRESOURCE(IDS_WILLNOTATTACH));
        cs_msg.Format(IDS_NOREADACCESS, sFullFileName.c_str());
        gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONEXCLAMATION);
        return false;
      }

      struct _stat stat_buffer;
      int irc = _wstat(sFullFileName.c_str(), &stat_buffer);
      if (irc == 0) {
        CGeneralMsgBox gmb;
        if (stat_buffer.st_size == 0) {
          CString cs_msg(MAKEINTRESOURCE(IDS_NOZEROSIZEEXTRACT)),
                  cs_title(MAKEINTRESOURCE(IDS_WILLNOTATTACH));
          gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONSTOP);
        } else {
          if (stat_buffer.st_size > (ATT_MAXSIZE << 20)) {
            CString cs_msg, cs_title(MAKEINTRESOURCE(IDS_WILLNOTATTACH));
            cs_msg.Format(IDS_ATTACHMENTTOOBIG, ATT_MAXSIZE);
            gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONSTOP);
            return false;
          }
          std::wstring sDrive, sDir, sName, sExt;
          pws_os::splitpath(sFullFileName.c_str(), sDrive, sDir, sName, sExt);
          atr.path = StringX(sDrive.c_str()) + StringX(sDir.c_str());
          atr.filename = StringX(sName.c_str()) + StringX(sExt.c_str());
          atr.flags = 0;
          atr.uiflags = 0;
          atr.mtime = stat_buffer.st_mtime;
          atr.atime = stat_buffer.st_atime;
          atr.ctime = stat_buffer.st_ctime;
          atr.dtime = 0;
          atr.uncsize = stat_buffer.st_size;
          atr.cmpsize = 0;
          atr.blksize = 0;
          atr.CRC = 0;
          uuid_array_t new_attmt_uuid;
          CUUID attmt_uuid;
          attmt_uuid.GetUUID(new_attmt_uuid);
          atr.attmt_uuid = new_attmt_uuid;
          atr.entry_uuid = CUUID::NullUUID();
          memset(atr.odigest, 0, SHA1::HASHLEN);
          memset(atr.cdigest, 0, SHA1::HASHLEN);
          return true;
        }
      }
    } else {
      // Can only be the user pressed cancel
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

LRESULT DboxMain::OnExportAttachment(WPARAM wParam, LPARAM )
{
  ATRecordEx *pATREx = (ATRecordEx *)wParam;
  if (pATREx->sxTitle.empty()) {
    // Title is mandatory - implies caller didn't fill in necessary fields
    ItemListIter iter = Find(pATREx->atr.entry_uuid);
    if (iter == End())
      return 0L;
    pATREx->sxGroup = iter->second.GetGroup();
    pATREx->sxTitle = iter->second.GetTitle();
    pATREx->sxUser = iter->second.GetUser();
  }
  DoExportAttachmentsXML(pATREx);

  return 0L;
}

LRESULT DboxMain::OnChangeAttachment(WPARAM wParam, LPARAM )
{
  ATRecord *pATR = (ATRecord *)wParam;

  pATR->uiflags |= ATT_ATTACHMENT_FLGCHGD;
  m_core.ChangeAttachment(*pATR);
  WriteAttachmentFile(false);

  return 0L;
}

void DboxMain::OnExportAllAttachments()
{
  DoExportAttachmentsXML();
}

void DboxMain::DoExportAttachmentsXML(ATRecordEx *pATREx)
{
  ATFVector vatf;
  for (int i = 0; i < CAdvancedAttDlg::NUM_TESTS; i++) {
    ATFilter atf;
    vatf.push_back(atf);
  }

  CGeneralMsgBox gmb;
  CExportAttDlg eXML(this, vatf);
  CString cs_text, cs_title, cs_temp;
  size_t num_exported(0);

  INT_PTR rc = eXML.DoModal();

  if (rc == IDOK) {
    CGeneralMsgBox gmb;
    StringX newfile;
    StringX pw(eXML.GetPasskey());
    if (m_core.CheckPasskey(m_core.GetCurFile(), pw) == PWSRC::SUCCESS) {
      // Do the export - SaveAs-type dialog box
      // New filename reflects attachment file name (if only one) or
      // database filename (if all eattachments)
      std::wstring XMLFileName;
      if (pATREx == NULL) {
        XMLFileName = m_core.GetCurFile().c_str();
      } else {
        XMLFileName = pATREx->atr.filename.c_str();
      }
      XMLFileName = PWSUtil::GetNewFileName(XMLFileName.c_str(), L"xml");

      cs_text.LoadString(IDS_NAMEXMLFILE);
      std::wstring dir;
      if (m_core.GetCurFile().empty())
        dir = PWSdirs::GetSafeDir();
      else {
        std::wstring cdrive, cdir, dontCare;
        pws_os::splitpath(m_core.GetCurFile().c_str(), cdrive, cdir, dontCare, dontCare);
        dir = cdrive + cdir;
      }

      while (1) {
        CPWFileDialog fd(FALSE,
                         L"xml",
                         XMLFileName.c_str(),
                         OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
                            OFN_LONGNAMES | OFN_OVERWRITEPROMPT,
                         CString(MAKEINTRESOURCE(IDS_FDF_X_ALL)),
                         this);

        fd.m_ofn.lpstrTitle = cs_text;

        if (!dir.empty())
          fd.m_ofn.lpstrInitialDir = dir.c_str();

        rc = fd.DoModal();

        if (m_inExit) {
          // If U3ExitNow called while in CPWFileDialog,
          // PostQuitMessage makes us return here instead
          // of exiting the app. Try resignalling
          PostQuitMessage(0);
          return;
        }
        if (rc == IDOK) {
          newfile = fd.GetPathName();
          break;
        } else
          return;
      } // while (1)

      ATRExVector vAIRecordExs;
      if (pATREx != NULL) {
        vAIRecordExs.push_back(*pATREx);
      }

      rc = WriteXMLAttachmentFile(newfile, vatf, vAIRecordExs, num_exported);

      if (rc != PWSRC::SUCCESS) {
        pws_os::DeleteAFile(newfile.c_str());
        cs_text.LoadString(IDS_USERABORTEDXMLEXPORT);
        cs_title.LoadString(IDS_EXPORTED);
        gmb.MessageBox(cs_text, cs_title, MB_OK | MB_ICONEXCLAMATION);
        return;
      }
    } else {
      gmb.AfxMessageBox(IDS_BADPASSKEY);
      ::Sleep(3000); // protect against automatic attacks
    }
  }

  cs_text.Format(IDS_EXPORTOK, num_exported);
  cs_title.LoadString(IDS_EXPORTED);
  gmb.MessageBox(cs_text, cs_title, MB_OK | MB_ICONINFORMATION);
  return;
}

void DboxMain::OnImportAttachments()
{
  // disable in read-only mode and empty database
  if (m_core.IsReadOnly() || m_core.GetNumEntries() == 0)
    return;

  CString cs_title, cs_temp, cs_text;
  cs_title.LoadString(IDS_XMLIMPORTFAILED);

  CGeneralMsgBox gmb;
  // Initialize set
  GTUSet setGTU;
  if (!m_core.GetUniqueGTUValidated() && !m_core.InitialiseGTU(setGTU)) {
    // Database is not unique to start with - tell user to validate it first
    cs_temp.Format(IDS_DBHASDUPLICATES, m_core.GetCurFile().c_str());
    gmb.MessageBox(cs_temp, cs_title, MB_ICONEXCLAMATION);
    return;
  }

  const std::wstring XSDfn(L"pwsafe_att.xsd");
  std::wstring XSDFilename = PWSdirs::GetXMLDir() + XSDfn;

#if USE_XML_LIBRARY == MSXML || USE_XML_LIBRARY == XERCES
  // Expat is a non-validating parser - no use for Schema!
  if (!pws_os::FileExists(XSDFilename)) {
    CGeneralMsgBox gmb;
    cs_temp.Format(IDSC_MISSINGXSD, XSDfn.c_str());
    cs_title.LoadString(IDSC_CANTVALIDATEXML);
    gmb.MessageBox(cs_temp, cs_title, MB_OK | MB_ICONSTOP);
    return;
  }
#endif

  std::wstring dir;
  if (m_core.GetCurFile().empty())
    dir = PWSdirs::GetSafeDir();
  else {
    std::wstring cdrive, cdir, dontCare;
    pws_os::splitpath(m_core.GetCurFile().c_str(), cdrive, cdir, dontCare, dontCare);
    dir = cdrive + cdir;
  }

  CPWFileDialog fd(TRUE,
                   L"xml",
                   NULL,
                   OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_LONGNAMES,
                   CString(MAKEINTRESOURCE(IDS_FDF_XML)),
                   this);

  cs_text.LoadString(IDS_PICKXMLFILE);
  fd.m_ofn.lpstrTitle = cs_text;

  if (!dir.empty())
    fd.m_ofn.lpstrInitialDir = dir.c_str();

  INT_PTR rc = fd.DoModal();

  if (m_inExit) {
    // If U3ExitNow called while in CPWFileDialog,
    // PostQuitMessage makes us return here instead
    // of exiting the app. Try resignalling
    PostQuitMessage(0);
    return;
  }

  if (rc == IDOK) {
    std::wstring strXMLErrors, strSkippedList;
    CString XMLFilename = fd.GetPathName();
    int numValidated, numImported, numSkipped;

    CWaitCursor waitCursor;  // This may take a while!

    /* Create report as we go */
    CReport rpt;
    CString cs_text;
    cs_text.LoadString(IDS_RPTIMPORTXML);
    rpt.StartReport(cs_text, m_core.GetCurFile().c_str());
    cs_text.LoadString(IDS_XML);
    cs_temp.Format(IDS_IMPORTFILE, cs_text, XMLFilename);
    rpt.WriteLine((LPCWSTR)cs_temp);
    rpt.WriteLine();
    std::vector<StringX> vgroups;
    Command *pcmd = NULL;

    int xrc = m_core.ImportXMLAttachmentFile(std::wstring(XMLFilename),
                                             XSDFilename.c_str(),
                                             strXMLErrors, strSkippedList,
                                             numValidated, numImported, numSkipped,
                                             rpt, pcmd);
    waitCursor.Restore();  // Restore normal cursor

    std::wstring csErrors(L"");
    switch (xrc) {
      case PWSRC::XML_FAILED_VALIDATION:
        rpt.WriteLine(strXMLErrors.c_str());
        cs_temp.Format(IDS_FAILEDXMLVALIDATE, fd.GetFileName(), L"");
        delete pcmd;
        break;
      case PWSRC::XML_FAILED_IMPORT:
        rpt.WriteLine(strXMLErrors.c_str());
        cs_temp.Format(IDS_XMLERRORS, fd.GetFileName(), L"");
        delete pcmd;
        break;
      case PWSRC::SUCCESS:
      case PWSRC::OK_WITH_ERRORS:
        cs_title.LoadString(rc == PWSRC::SUCCESS ? IDS_COMPLETE : IDS_OKWITHERRORS);
        if (pcmd != NULL)
          Execute(pcmd);

        if (!strXMLErrors.empty() || numSkipped > 0) {
          if (!strXMLErrors.empty()) {
            csErrors = strXMLErrors + L"\n";
          }

          if (!csErrors.empty()) {
            rpt.WriteLine(csErrors.c_str());
          }

          CString cs_renamed(L""), cs_skipped(L"");
          if (numSkipped > 0) {
            cs_skipped.LoadString(IDS_TITLESKIPPED);
            rpt.WriteLine((LPCWSTR)cs_skipped);
            cs_skipped.Format(IDS_XMLIMPORTSKIPPED, numSkipped);
            rpt.WriteLine(strSkippedList.c_str());
            rpt.WriteLine();
          }

          cs_temp.Format(IDS_XMLIMPORTWITHERRORS,
                         fd.GetFileName(), numValidated, numImported,
                         cs_skipped, cs_renamed, L"");

          ChangeOkUpdate();
        } else {
          const CString cs_validate(MAKEINTRESOURCE(numValidated == 1 ? IDSC_ENTRY : IDSC_ENTRIES));
          const CString cs_imported(MAKEINTRESOURCE(numImported == 1 ? IDSC_ENTRY : IDSC_ENTRIES));
          cs_temp.Format(IDS_XMLIMPORTOK, numValidated, cs_validate, numImported, cs_imported);
          ChangeOkUpdate();
        }

        RefreshViews();
        break;
      default:
        ASSERT(0);
    } // switch

    // Finish Report
    rpt.WriteLine((LPCWSTR)cs_temp);
    rpt.EndReport();

    if (rc != PWSRC::SUCCESS || !strXMLErrors.empty())
      gmb.SetStandardIcon(MB_ICONEXCLAMATION);
    else
      gmb.SetStandardIcon(MB_ICONINFORMATION);

    gmb.SetTitle(cs_title);
    gmb.SetMsg(cs_temp);
    gmb.AddButton(IDS_OK, IDS_OK, TRUE, TRUE);
    gmb.AddButton(IDS_VIEWREPORT, IDS_VIEWREPORT);
    INT_PTR rc = gmb.DoModal();
    if (rc == IDS_VIEWREPORT)
      ViewReport(rpt);
  }
}

LRESULT DboxMain::OnExtractAttachment(WPARAM wParam, LPARAM )
{
  ATRecord *pATR = (ATRecord *)wParam;
  DoAttachmentExtraction(*pATR);

  return 0L;
}

void DboxMain::DoAttachmentExtraction(const ATRecord &atr)
{
  CGeneralMsgBox gmb;
  CString cs_msg, cs_title, csFilter;
  StringX sxFile;
  std::wstring sDrv, sDir, sFname, sExt, sxFilter, wsNewName(L"");

  int ga_status(PWSRC::SUCCESS);

  // If user mandated that a secure delete program is there, first check that they have
  // specified one and then check it exists on this system.
  if ((atr.flags & ATT_ERASEPGMEXISTS) == ATT_ERASEPGMEXISTS) {
    StringX sxErasePgm = PWSprefs::GetInstance()->GetPref(PWSprefs::EraseProgram);
    if (sxErasePgm.empty() || !pws_os::FileExists(sxErasePgm.c_str())) {
      cs_msg.LoadString(IDS_NOERASEPGM);
      cs_title.LoadString(IDS_WILLNOTEXTRACT);
      gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONSTOP);
      return;
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
          ga_status = PWSRC::BADTARGETDEVICE;
          goto exit;
        }
      } else {
        break;
      }
    } else
      return;
  }

  if (wsNewName.empty())
    return;

  // Invoke thread
  ga_status = GetAttachment(wsNewName, atr);

exit:
  UINT uiTitle(0), uiMSG(0);
  int flags(MB_OK | MB_ICONEXCLAMATION);
  switch (ga_status) {
    case PWSRC::SUCCESS:
      uiTitle = IDS_EXTRACTATT;
      uiMSG = IDS_COMPLETEDOK;
      flags = MB_OK | MB_ICONINFORMATION;
      break;
    case PWSRC::USER_CANCEL:
      uiTitle = IDS_EXTRACTATT;
      uiMSG = IDS_USERCANCELLEDEXTRACT;
      break;
    case PWSRC::BADTARGETDEVICE:
      uiTitle = IDS_WILLNOTEXTRACT;
      uiMSG = IDS_NOTREMOVEABLE;
      break;
    case PWSRC::CANTCREATEFILE:
      uiTitle =IDS_WILLNOTEXTRACT;
      uiMSG = IDS_CANTCREATEFILE;
      break;
    case PWSRC::CANTFINDATTACHMENT:
      uiTitle = IDS_WILLNOTEXTRACT;
      uiMSG = IDS_CANTGETATTACHMENT;
      break;
    case PWSRC::BADDATA:
      uiTitle = IDS_WILLNOTEXTRACT;
      uiMSG = IDS_BADATTDATA;
      break;
    case PWSRC::BADATTACHMENTWRITE:
      uiTitle = IDS_EXTRACTATT;
      uiMSG = IDS_BADATTACHMENTWRITE;
      break;
    case PWSRC::BADCRCDIGEST:
      uiTitle = IDS_CRCHASHERROR;
      uiMSG = IDS_CONTINUEEXTRACT;
      break;
    case PWSRC::BADLENGTH:
      uiTitle = IDS_WILLNOTEXTRACT;
      uiMSG = IDS_BADATTLENGTH;
      break;
    case PWSRC::BADINFLATE:
      uiTitle = IDS_WILLNOTEXTRACT;
      uiMSG = IDS_BADINFLATE;
      break;
    default:
      uiTitle = IDS_WILLNOTEXTRACT;
      uiMSG = IDSC_ATT_UNKNOWNERROR;
      break;
  }

  if (uiTitle != 0) {
    cs_title.LoadString(uiTitle);
    if (uiMSG == IDSC_ATT_UNKNOWNERROR)
      cs_msg.Format(IDSC_ATT_UNKNOWNERROR, ga_status);
    else
      cs_msg.LoadString(uiMSG);
    gmb.MessageBox(cs_msg, cs_title, flags);
  }
}

int DboxMain::XGetAttachment(const stringT &newfile, const ATRecord &atr)
{
  // Thread version!
  HANDLE hFile(INVALID_HANDLE_VALUE);
  int status, zRC;

  // Long job
  BeginWaitCursor();

  // Get attachment
  status = m_core.GetAttachment(newfile, atr, zRC);

  if (status == PWSRC::SUCCESS) {
    // To set times - need to use CreateFile
    hFile = CreateFile(newfile.c_str(), GENERIC_WRITE,
                       0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
      goto exit;

    // Set times back to original
    FILETIME ct, at, mt;
    ULONGLONG ulltime;  // Note that ULONGLONG is an unsigned 64-bit integer value
  
    // FILETIME is a 64-bit unsigned integer representing
    // the number of 100-nanosecond intervals since January 1, 1601
    // UNIX timestamp is number of seconds since January 1, 1970
    // Note extra 89 leap days!
    // 116444736000000000 = 10000000 * [60 * 60 * 24 * 365 * 369 + (89 * 86400)]

    ulltime = UInt32x32To64(atr.ctime, 10000000) + 116444736000000000ull;
    ct.dwLowDateTime = (DWORD)ulltime;
    ct.dwHighDateTime = (DWORD)(ulltime >> 32);

    ulltime = UInt32x32To64(atr.atime, 10000000) + 116444736000000000ull;
    at.dwLowDateTime = (DWORD)ulltime;
    at.dwHighDateTime = (DWORD)(ulltime >> 32);

    ulltime = UInt32x32To64(atr.mtime, 10000000) + 116444736000000000ull;
    mt.dwLowDateTime = (DWORD)ulltime;
    mt.dwHighDateTime = (DWORD)(ulltime >> 32);

    // Write the new creation, accessed, and last written time
    SetFileTime(hFile, &ct, &at, &mt);
    CloseHandle(hFile);

    // Reset handle
    hFile = INVALID_HANDLE_VALUE;
  }

exit:
  EndWaitCursor();

  // Check all went OK?
  if (status != PWSRC::SUCCESS) {
    // No - Delete the file
    pws_os::DeleteAFile(newfile);
  }
  return status;
}

bool DboxMain::AnyAttachments(HTREEITEM hItem)
{
  CItemData *pci = (CItemData *)m_ctlItemTree.GetItemData(hItem);
  if (pci != NULL) {
    // Leaf
    uuid_array_t entry_uuid;
    pci->GetUUID(entry_uuid);
    return (m_core.HasAttachments(entry_uuid) > 0);
  }

  // Must be a group - Walk the Tree from here to determine if any entries have attachments
  while ((hItem = m_ctlItemTree.GetNextTreeItem(hItem)) != NULL) {
    if (!m_ctlItemTree.ItemHasChildren(hItem)) {
      CItemData *pci = (CItemData *)m_ctlItemTree.GetItemData(hItem);
      if (pci != NULL) {  // NULL if there's an empty group [bug #1633516]
        uuid_array_t entry_uuid;
        pci->GetUUID(entry_uuid);
        if (m_core.HasAttachments(entry_uuid) > 0)
          return true;
      }
    }
  }
  return false;
}

int DboxMain::AttachmentProgress(const ATTProgress &st_atpg)
{
  int rc = ATT_PROGRESS_CONTINUE;
  if (m_pProgressDlg != NULL) {
    m_pProgressDlg->SendMessage(ATTPRG_UPDATE_GUI, (WPARAM)&st_atpg, 0);

    if (m_bStopVerify)
      rc = ATT_PROGRESS_STOPVERIFY;
    if (m_bAttachmentCancel)
      rc += ATT_PROGRESS_CANCEL;
  }
  return rc;
}

int DboxMain::ReadAttachmentFile(bool bVerify)
{
  if (m_bNoChangeToAttachments)
    return PWSRC::SUCCESS;

  if (m_core.GetCurFile().empty()) {
    return PWSRC::CANT_OPEN_FILE;
  }

  // Generate attachment file names from database name
  stringT attmt_file, drv, dir, name, ext;

  pws_os::splitpath(m_core.GetCurFile().c_str(), drv, dir, name, ext);
  attmt_file = drv + dir + name + stringT(ATT_DEFAULT_ATTMT_SUFFIX);

  // If attachment file doesn't exist - OK
  if (!pws_os::FileExists(attmt_file)) {
    pws_os::Trace(_T("No attachment file exists.\n"));
    return PWSRC::SUCCESS;
  }

  ATThreadParms *pthdpms = new ATThreadParms;
  pthdpms->function = READ;
  pthdpms->bVerify = bVerify;

  int status = DoAttachmentThread(pthdpms);

  delete pthdpms;
  return status;
}

int DboxMain::WriteAttachmentFile(const bool bCleanup,
                                  PWSAttfile::VERSION version)
{
  if (m_bNoChangeToAttachments)
    return PWSRC::SUCCESS;

  ATThreadParms *pthdpms = new ATThreadParms;
  pthdpms->function = WRITE;
  pthdpms->bCleanup = bCleanup;
  pthdpms->version = version;

  int status = DoAttachmentThread(pthdpms);

  delete pthdpms;
  return status;
}

int DboxMain::DuplicateAttachments(const CUUID &old_entry_uuid,
                                   const CUUID &new_entry_uuid,
                                   PWSAttfile::VERSION version)
{
  if (m_bNoChangeToAttachments)
    return PWSRC::SUCCESS;

  ATThreadParms *pthdpms = new ATThreadParms;
  pthdpms->function = DUPLICATE;
  pthdpms->old_entry_uuid = old_entry_uuid;
  pthdpms->new_entry_uuid = new_entry_uuid;
  pthdpms->version = version;

  int status = DoAttachmentThread(pthdpms);

  delete pthdpms;
  return status;
}

int DboxMain::WriteXMLAttachmentFile(const StringX &filename, ATFVector &vatf,
                                     ATRExVector &vAIRecordExs, size_t &num_exported)
{
  if (m_bNoChangeToAttachments)
    return PWSRC::SUCCESS;

  ATThreadParms *pthdpms = new ATThreadParms;
  pthdpms->function = XML_EXPORT;
  pthdpms->filename = filename;
  pthdpms->vatf = vatf;
  pthdpms->vAIRecordExs = vAIRecordExs;
  pthdpms->num_exported = num_exported;

  int status = DoAttachmentThread(pthdpms);

  num_exported = pthdpms->num_exported;
  delete pthdpms;
  return status;
}

int DboxMain::GetAttachment(const stringT &newfile, const ATRecord &atr)
{
  if (m_bNoChangeToAttachments)
    return PWSRC::SUCCESS;

  ATThreadParms *pthdpms = new ATThreadParms;
  pthdpms->function = GET;
  pthdpms->atr = atr;
  pthdpms->newfile = newfile;

  int status = DoAttachmentThread(pthdpms);

  delete pthdpms;
  return status;
}

int DboxMain::CompleteImportFile(const stringT &impfilename,
                                 PWSAttfile::VERSION version)
{
  if (m_bNoChangeToAttachments)
    return PWSRC::SUCCESS;

  ATThreadParms *pthdpms = new ATThreadParms;
  pthdpms->function = COMPLETE_XML_IMPORT;
  pthdpms->impfilename = impfilename.c_str();
  pthdpms->version = version;

  int status = DoAttachmentThread(pthdpms);

  delete pthdpms;
  return status;
}

static UINT AttachmentThread(LPVOID pParam)
{
  ATThreadParms *pthdpms = (ATThreadParms *)pParam;

  int status(PWSRC::SUCCESS);
  switch (pthdpms->function) {
    case READ:
      status = pthdpms->pcore->ReadAttachmentFile(pthdpms->bVerify);
      break;
    case WRITE:
      status = pthdpms->pcore->WriteAttachmentFile(pthdpms->bCleanup, pthdpms->version);
      break;
    case DUPLICATE:
      status = pthdpms->pcore->DuplicateAttachments(pthdpms->old_entry_uuid,
                                                    pthdpms->new_entry_uuid,
                                                    pthdpms->version);
      break;
    case COMPLETE_XML_IMPORT:
      status = pthdpms->pcore->CompleteImportFile(pthdpms->impfilename,
                                                  pthdpms->version);
      break;
    case XML_EXPORT:
      status = pthdpms->pcore->WriteXMLAttachmentFile(pthdpms->filename, pthdpms->vatf,
                                                      pthdpms->vAIRecordExs,
                                                      pthdpms->num_exported);
      break;
    case GET:
      status = pthdpms->pDbx->XGetAttachment(pthdpms->newfile, pthdpms->atr);
      break;
    default:
      ASSERT(0);
  }

  // Set the thread return code for caller
  pthdpms->status = status;
  // Tell DboxMain that thread has ended
  if (pthdpms->pDbx != NULL)
    pthdpms->pDbx->PostMessage(PWS_MSG_ATTACHMENT_THREAD_ENDED, (WPARAM)pthdpms, 0);

  return 0;
}

int DboxMain::DoAttachmentThread(ATThreadParms * &pthdpms)
{
  pthdpms->pDbx = this;
  pthdpms->pcore = &m_core;

  m_bStopVerify = m_bAttachmentCancel = false;
  m_pProgressDlg = new CAttProgressDlg(this, &m_bAttachmentCancel,
                 pthdpms->function == READ ? &m_bStopVerify : NULL,
                 pthdpms);

  pthdpms->pProgressDlg = m_pProgressDlg;

  m_pAttThread = AfxBeginThread(AttachmentThread, pthdpms,
                                THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

  if (m_pAttThread == NULL) {
    pws_os::Trace(_T("Unable to create Attachment thread\n"));
    return PWSRC::FAILURE;
  }

  // Stop automatic deletion and then resume thread
  m_pAttThread->m_bAutoDelete = FALSE;
  m_pAttThread->ResumeThread();
  m_pProgressDlg->SetThread(m_pAttThread);

  // Show the progress dialog
  m_pProgressDlg->DoModal();

  // Only ever gets here when thread ends - even if user presses cancel,
  // it goes back to the thread which will end.
  if (pthdpms->function == READ && m_bAttachmentCancel)
    m_bNoChangeToAttachments = true;

  delete m_pProgressDlg;
  m_pProgressDlg = NULL;

  // Now get the attachment thread return status as we set return code
  // and the caller will delete the thread parms when we return
  const int status = m_bAttachmentCancel ? PWSRC::USER_CANCEL : pthdpms->status;
  if (status != PWSRC::SUCCESS) {
#ifdef _DEBUG
    CString cs_function;
    switch (pthdpms->function) {
      case READ:
        cs_function = L"READ";
        break;
      case WRITE:
        cs_function = L"WRITE";
        break;
      case DUPLICATE:
        cs_function = L"DUPLICATE";
        break;
      case COMPLETE_XML_IMPORT:
        cs_function = L"COMPLETE_XML_IMPORT";
        break;
      case XML_EXPORT:
        cs_function = L"XML_EXPORT";
        break;
      case GET:
        cs_function = L"GET";
        break;
      default:
        ASSERT(0);
    }
    pws_os::Trace(_T("Problem processing attachments. Function=%s, rc=%d\n"),
                  cs_function, pthdpms->status);
#endif
    CString cs_msg, cs_title;
    cs_title.LoadString(IDSC_ATT_ERRORS);
    UINT uimsg(0);
    switch (pthdpms->status) {
      case PWSRC::HEADERS_INVALID:
        uimsg = IDSC_ATT_HDRMISMATCH;
        break;
      case PWSRC::USER_CANCEL:
        uimsg = IDS_CANCELATTACHMENT;
        break;
      default:
        // File extraction has its own messages
        if (pthdpms->function != GET)
          uimsg = IDSC_ATT_UNKNOWNERROR;
    }

    // Tell user
    if (uimsg > 0) {
      CGeneralMsgBox gmb;
      if (uimsg == IDSC_ATT_UNKNOWNERROR)
        cs_msg.Format(IDSC_ATT_UNKNOWNERROR, pthdpms->status);
      else
        cs_msg.LoadString(uimsg);
      gmb.AfxMessageBox(cs_msg, cs_title, MB_OK | MB_ICONEXCLAMATION);
    }
  }

  return status;
}

LRESULT DboxMain::OnAttachmentThreadEnded(WPARAM wParam, LPARAM )
{
  ATThreadParms *pthdpms = (ATThreadParms *)wParam;

  // Wait for it
  WaitForSingleObject(m_pAttThread->m_hThread, INFINITE);

  // Thread ended - send message to progress dialog to end
  pthdpms->pProgressDlg->SendMessage(ATTPRG_THREAD_ENDED);

  // Now tidy up (m_bAutoDelete was set to FALSE)
  delete m_pAttThread;
  m_pAttThread = NULL;

  return 0L;
}
