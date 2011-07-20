/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// AddEdit_Attachments.cpp : implementation file
//

#include "stdafx.h"
#include "AddEdit_Attachments.h"
#include "AddEdit_PropertySheet.h"
#include "AddDescription.h"

#include "ThisMfcApp.h"    // For Help
#include "GeneralMsgBox.h"
#include "DboxMain.h"

#include "os/dir.h"

// Local routines
bool IsShortcut(LPCTSTR pszPath);
HRESULT ResolveShortcut(LPCWSTR lpszShortcutPath,
                        LPWSTR lpszFilePath);

// CAddEdit_Attachments dialog

IMPLEMENT_DYNAMIC(CAddEdit_Attachments, CAddEdit_PropertyPage)

CAddEdit_Attachments::CAddEdit_Attachments(CWnd *pParent, st_AE_master_data *pAEMD)
  : CAddEdit_PropertyPage(pParent, CAddEdit_Attachments::IDD, pAEMD),
  m_pToolTipCtrl(NULL), m_bInitdone(false)
{
  pAEMD->pci_original->GetUUID(m_entry_uuid);
}

CAddEdit_Attachments::~CAddEdit_Attachments()
{
  delete m_pToolTipCtrl;
}

void CAddEdit_Attachments::DoDataExchange(CDataExchange* pDX)
{
  CAddEdit_PropertyPage::DoDataExchange(pDX);

  DDX_Control(pDX, IDC_ATTACHMENT_LIST, m_AttLC);
  DDX_Control(pDX, IDC_NEWATTACHMENT_LIST, m_NewAttLC);

  DDX_Control(pDX, IDC_STATIC_WARNING, m_stc_warning);
  DDX_Control(pDX, IDC_STATIC_DROPATTACHMENT, m_stc_DropFiles);
}

BEGIN_MESSAGE_MAP(CAddEdit_Attachments, CAddEdit_PropertyPage)
  ON_WM_CTLCOLOR()
  ON_BN_CLICKED(IDC_ADD_ATTACHMENT, OnAddNewAttachment)
  ON_BN_CLICKED(IDC_DELETE_ATTACHMENT, OnDeleteNewAttachment)
  ON_COMMAND(ID_HELP, OnHelp)
  ON_NOTIFY(NM_CLICK, IDC_NEWATTACHMENT_LIST, OnNewAttachmentListSelected)
  ON_MESSAGE(PWS_MSG_ATTACHMENT_FLAG_CHANGED, OnAttachmentChanged)
  ON_MESSAGE(WM_DROPFILES, DropFiles)
  ON_MESSAGE(PSM_QUERYSIBLINGS, OnQuerySiblings)
END_MESSAGE_MAP()

BOOL CAddEdit_Attachments::PreTranslateMessage(MSG* pMsg)
{
  if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1) {
    PostMessage(WM_COMMAND, MAKELONG(ID_HELP, BN_CLICKED), NULL);
    return TRUE;
  }

  // Do tooltips
  if (m_pToolTipCtrl != NULL)
    m_pToolTipCtrl->RelayEvent(pMsg);

  return CAddEdit_PropertyPage::PreTranslateMessage(pMsg);
}

BOOL CAddEdit_Attachments::OnInitDialog()
{
  CAddEdit_PropertyPage::OnInitDialog();
  
  const bool bReadOnly = M_uicaller() == IDS_VIEWENTRY;

  CHeaderCtrl *pHCtrl;
  pHCtrl = m_AttLC.GetHeaderCtrl();
  ASSERT(pHCtrl != NULL);
  m_AttLCHeader.SubclassWindow(pHCtrl->GetSafeHwnd());
  m_AttLCHeader.SetStopChangeFlag(true);

  pHCtrl = m_NewAttLC.GetHeaderCtrl();
  ASSERT(pHCtrl != NULL);
  m_NewAttLCHeader.SubclassWindow(pHCtrl->GetSafeHwnd());
  m_NewAttLCHeader.SetStopChangeFlag(true);

  m_AttLC.Init(CPWAttLC::EXISTING, (CWnd *)M_pDbx(), bReadOnly);
  m_NewAttLC.Init(CPWAttLC::NEW, (CWnd *)M_pDbx(), bReadOnly);

  // Make some columns centered
  LVCOLUMN lvc;
  lvc.mask = LVCF_FMT;
  lvc.fmt = LVCFMT_CENTER;

  DWORD dwExStyle;
  dwExStyle = m_AttLC.GetExtendedStyle();
  dwExStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_SUBITEMIMAGES; // | LVS_EX_BORDERSELECT;
  m_AttLC.SetExtendedStyle(dwExStyle);

  dwExStyle = m_NewAttLC.GetExtendedStyle();
  dwExStyle |= LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_SUBITEMIMAGES; // | LVS_EX_BORDERSELECT;
  m_NewAttLC.SetExtendedStyle(dwExStyle);

  // Nothing is selected yet
  GetDlgItem(IDC_DELETE_ATTACHMENT)->EnableWindow(FALSE);

  // Save initial flags for existing attachments - used to check if changed
  for (ATRViter iter = M_vATRecords().begin(); iter != M_vATRecords().end(); iter++) {
    m_vAttFlags.push_back(iter->flags);
    m_vAttUIFlags.push_back(iter->uiflags & (~ATT_ATTACHMENT_FLGCHGD));
  }

  // Add existing attachments
  m_AttLC.AddAttachments(M_vATRecords());

  m_stc_warning.SetColour(RGB(255, 0, 0));

  if (bReadOnly) {
    GetDlgItem(IDC_ADD_ATTACHMENT)->EnableWindow(FALSE);
    m_stc_DropFiles.DragAcceptFiles(FALSE);
    m_stc_DropFiles.EnableWindow(FALSE);
  } else {
    GetDlgItem(IDC_ADD_ATTACHMENT)->EnableWindow(TRUE);
    m_stc_DropFiles.DragAcceptFiles(TRUE);
    m_stc_DropFiles.SetBkColour(RGB(222, 255, 222));  // Light green
  }

  UpdateData(FALSE);

  m_bInitdone = true;
  return TRUE;
}

HBRUSH CAddEdit_Attachments::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  HBRUSH hbr = CAddEdit_PropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);

  // Only deal with Static controls and then
  // Only with our special one - change colour of warning message
  if (nCtlColor == CTLCOLOR_STATIC && pWnd->GetDlgCtrlID() == IDC_STATIC_WARNING) {
    if (((CStaticExtn *)pWnd)->GetColourState()) {
      COLORREF cfUser = ((CStaticExtn *)pWnd)->GetUserColour();
      pDC->SetTextColor(cfUser);
    }
  }
  return hbr;
}

BOOL CAddEdit_Attachments::OnApply()
{
  return CAddEdit_PropertyPage::OnApply();
}

void CAddEdit_Attachments::OnHelp()
{
  CString cs_HelpTopic;
  cs_HelpTopic = app.GetHelpFileName() + L"::/html/attachments.html";
  HtmlHelp(DWORD_PTR((LPCWSTR)cs_HelpTopic), HH_DISPLAY_TOPIC);
}

BOOL CAddEdit_Attachments::OnKillActive()
{
  if (UpdateData(TRUE) == FALSE)
    return FALSE;

  return CAddEdit_PropertyPage::OnKillActive();
}

LRESULT CAddEdit_Attachments::OnQuerySiblings(WPARAM wParam, LPARAM)
{
  UpdateData(TRUE);

  // Have any of my fields been changed?
  switch (wParam) {
    case PP_DATA_CHANGED:
      switch (M_uicaller()) {
        case IDS_EDITENTRY:
          if (AnyNewAttachments() || HasExistingChanged())
            return 1L;
          break;
        case IDS_ADDENTRY:
          if (AnyNewAttachments())
            return 1L;
          break;
      }
      break;
    case PP_UPDATE_VARIABLES:
      // Since OnOK calls OnApply after we need to verify and/or
      // copy data into the entry - we do it ourselfs here first
      if (OnApply() == FALSE)
        return 1L;
  }
  return 0L;
}

// CAddEdit_Attachments message handlers

void CAddEdit_Attachments::UpdateLists()
{

  m_AttLC.DeleteAllItems();
  m_AttLC.m_vATRecords.clear();
  m_AttLC.AddAttachments(M_vATRecords(), true);
  // Force refresh and then do it now
  m_AttLC.Invalidate();
  m_AttLC.UpdateWindow();

  m_vAttFlags.clear();
  m_vAttUIFlags.clear();
  // Re-save initial flags for existing attachments - used to check if changed
  for (ATRViter iter = M_vATRecords().begin(); iter != M_vATRecords().end(); iter++) {
    m_vAttFlags.push_back(iter->flags);
    m_vAttUIFlags.push_back(iter->uiflags & (~ATT_ATTACHMENT_FLGCHGD));
  }

  m_NewAttLC.DeleteAllItems();
  m_NewAttLC.m_vATRecords.clear();
  // Force refresh and then do it now
  m_NewAttLC.Invalidate();
  m_NewAttLC.UpdateWindow();
}

void CAddEdit_Attachments::OnAddNewAttachment()
{
  // User clicks Add button. New entry added to list of New (but not yet added)
  // attachments
  ATRecord atr;

  // Disable PropertySheet and all PropertyPages whilst displaying FileDialog
  GetParent()->EnableWindow(FALSE);
  if (M_pDbx()->GetNewAttachmentInfo(atr, true)) {
    atr.entry_uuid = M_entry_uuid();
    atr.flags = ATT_EXTRACTTOREMOVEABLE | ATT_ERASURE_REQUIRED;
    M_vNewATRecords().push_back(atr);
    m_NewAttLC.AddNewAttachment(M_vNewATRecords().size() - 1, atr);
    m_ae_psh->SetAttachmentsChanged(true);
    // Force refresh and then do it now
    m_NewAttLC.Invalidate();
   m_NewAttLC.UpdateWindow();
  }

  // Re-enable everything
  GetParent()->EnableWindow(TRUE);
}

void CAddEdit_Attachments::OnDeleteNewAttachment()
{
  if (m_NewAttLC.m_iItem != -1) {
    // Mark new entry as deleted, delete from CListCtrl but DO NOT delete
    // from the vector
    DWORD_PTR dwData = m_NewAttLC.GetItemData(m_NewAttLC.m_iItem);
    M_vNewATRecords()[dwData].uiflags |= ATT_ATTACHMENT_DELETED;
    m_NewAttLC.DeleteItem(m_NewAttLC.m_iItem);

    bool bChanged = AnyNewAttachments() || HasExistingChanged();
    m_ae_psh->SetAttachmentsChanged(bChanged);
    GetDlgItem(IDC_DELETE_ATTACHMENT)->EnableWindow(FALSE);
    // Force refresh and then do it now
    m_NewAttLC.Invalidate();
    m_NewAttLC.UpdateWindow();
  }
}

bool CAddEdit_Attachments::AnyNewAttachments()
{
  // Look for non-deleted new attachments
  for (size_t i = 0; i < M_vNewATRecords().size(); i++) {
    if ((M_vNewATRecords()[i].uiflags & ATT_ATTACHMENT_DELETED) == 0)
      return true;
  }
  return false;
}

bool CAddEdit_Attachments::HasExistingChanged()
{
  ASSERT(m_vAttFlags.size() == M_vATRecords().size());

  // Update flags & uiflags from CListCtrl data
  BYTE flags, uiflags;
  for (size_t i = 0; i < M_vATRecords().size(); i++) {
    m_AttLC.GetAttachmentFlags(i, flags, uiflags);
    M_vATRecords()[i].flags = flags;
    M_vATRecords()[i].uiflags = uiflags & (~ATT_ATTACHMENT_FLGCHGD);
  }

  // Check if existing attachment flags changed
  for (size_t i = 0; i < m_vAttFlags.size(); i++) {
    if (m_vAttFlags[i] != M_vATRecords()[i].flags ||
        m_vAttUIFlags[i] != M_vATRecords()[i].uiflags) {
      return true;
    }
  }
  return false;
}

void CAddEdit_Attachments::OnNewAttachmentListSelected(NMHDR *pNMHDR, LRESULT *pResult)
{
  int iItem(-1);
  switch (pNMHDR->code) {
    case NM_CLICK:
    {
      LPNMITEMACTIVATE pLVItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
      iItem = pLVItemActivate->iItem;
      break;
    }
    case LVN_KEYDOWN:
    {
      LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
      iItem = m_NewAttLC.GetNextItem(-1, LVNI_SELECTED);
      int nCount = m_NewAttLC.GetItemCount();
      if (pLVKeyDown->wVKey == VK_DOWN)
        iItem = (iItem + 1) % nCount;
      if (pLVKeyDown->wVKey == VK_UP)
        iItem = (iItem - 1 + nCount) % nCount;
      break;
    }
    default:
      // No idea how we got here!
      return;
  }

  if (iItem != -1)
    GetDlgItem(IDC_DELETE_ATTACHMENT)->EnableWindow(TRUE);

  *pResult = 0;
}

LRESULT CAddEdit_Attachments::OnAttachmentChanged(WPARAM wParam, LPARAM lParam)
{
  // wParam = EXISTING for Existing Entries and NEW for New Entries
  // lParam = entry number
  BYTE flags, uiflags;

  switch (wParam) {
    case CPWAttLC::EXISTING:
      ASSERT(lParam < m_AttLC.GetItemCount());
      m_AttLC.GetAttachmentFlags(lParam, flags, uiflags);
      if (M_vATRecords()[lParam].flags != flags ||
          M_vATRecords()[lParam].uiflags != uiflags) {
        M_vATRecords()[lParam].flags = flags;
        M_vATRecords()[lParam].uiflags = uiflags | ATT_ATTACHMENT_FLGCHGD;
      }
      break;
    case CPWAttLC::NEW:
      ASSERT(lParam < m_NewAttLC.GetItemCount());
      m_NewAttLC.GetNewAttachmentFlags(lParam, flags);
      M_vNewATRecords()[lParam].flags = flags;
      break;
    default:
      ASSERT(0);
      break;
  }

  m_ae_psh->SetAttachmentsChanged(m_NewAttLC.GetItemCount() > 0 || HasExistingChanged());
  return 0L;
}

LRESULT CAddEdit_Attachments::DropFiles(WPARAM wParam, LPARAM)
{
  bool bAddAttachment(false);
  // Get the number of pathnames that have been dropped
  HDROP hDrop = (HDROP)wParam;
  const UINT uiNumFilesDropped = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);

  if (uiNumFilesDropped == 0)
    return 0L;

  // Allocate memory to contain full pathname & zero byte
  wchar_t *pszFile = new wchar_t[MAX_PATH + 1];

  // get all file names
  for (UINT uinum = 0; uinum < uiNumFilesDropped; uinum++) {
    // Get the number of bytes required by the file's full pathname
    const UINT uiPathnameSize = DragQueryFile(hDrop, uiNumFilesDropped - uinum - 1, NULL, 0);

    // If buffer to small - get a bigger one!
    if (uiPathnameSize > sizeof(pszFile + 1) / sizeof(wchar_t)) {
      delete [] pszFile;
      pszFile = new wchar_t[uiPathnameSize + 1];
    }
 
    // Copy the pathname into the buffer
    DragQueryFile(hDrop, uiNumFilesDropped - uinum - 1, pszFile, uiPathnameSize + 1);

    // We do not process directories themselves - only contents if user
    // drops a directory!
    CFileStatus filestatus;
    CFile::GetStatus(pszFile, filestatus);

    if ((filestatus.m_attribute & CFile::directory) ||
        (filestatus.m_attribute & CFile::volume)) {
      CGeneralMsgBox gmb;
      CString cs_msg(MAKEINTRESOURCE(IDS_NODIRECTORIES)),
              cs_title(MAKEINTRESOURCE(IDS_WILLNOTATTACH));
      gmb.MessageBox(cs_msg, cs_title, MB_OK | MB_ICONQUESTION);
      continue;
    }

    // Is it a shortcut - if so, try and resolve
    if (IsShortcut(pszFile)) {
      wchar_t sztarget[_MAX_PATH] = {0};
      if (ResolveShortcut(pszFile, sztarget) != S_OK) {
        continue;
      }

      CGeneralMsgBox gmb;
      CString cs_msg, cs_title(MAKEINTRESOURCE(IDS_ADDFILE));
      cs_msg.Format(IDS_SHORTCUTORTARGET, pszFile);
      gmb.SetStandardIcon(MB_ICONQUESTION);
      gmb.SetTitle(IDS_ADDFILE);
      gmb.SetMsg(cs_msg);
      gmb.AddButton(IDS_ADDSHORTCUT, IDS_ADDSHORTCUT);
      gmb.AddButton(IDS_ADDTARGET, IDS_ADDTARGET, TRUE, TRUE);
      INT_PTR rc = gmb.DoModal();

      if (rc == IDS_ADDTARGET) {
        // If buffer to small - get a bigger one!
        const size_t nLinkLength = wcslen(sztarget) + 1;
        if (nLinkLength > sizeof(pszFile + 1) / sizeof(wchar_t)) {
         delete [] pszFile;
         pszFile = new wchar_t[nLinkLength];
        }
        lstrcpyn(pszFile, sztarget, (int)nLinkLength);
      }
    }

    ATRecord atr;

    atr.filename = pszFile;
    if (M_pDbx()->GetNewAttachmentInfo(atr, false)) {
      SetForegroundWindow();
      bAddAttachment = true;
      CAddDescription dlg(this, pszFile);
      dlg.DoModal();
      atr.description = dlg.GetDescription();
      atr.entry_uuid = M_entry_uuid();
      atr.flags = ATT_EXTRACTTOREMOVEABLE | ATT_ERASURE_REQUIRED;
      M_vNewATRecords().push_back(atr);
      m_NewAttLC.AddNewAttachment(M_vNewATRecords().size() - 1, atr);
    }
  }

  if (bAddAttachment) {
    m_ae_psh->SetAttachmentsChanged(true);
   // Force refresh and then do it now
    m_NewAttLC.Invalidate();
    m_NewAttLC.UpdateWindow();
  }

  // clean up
  delete [] pszFile;
  return 0L;
}

bool IsShortcut(LPCTSTR pszPath)
{
  SHFILEINFO shFileInfo = { 0 };
  return ((SHGetFileInfo(pszPath,
                         0,
                         &shFileInfo,
                         sizeof(shFileInfo),
                         SHGFI_ATTRIBUTES)) && (shFileInfo.dwAttributes & SFGAO_LINK) == SFGAO_LINK);
}

HRESULT ResolveShortcut(LPCWSTR lpszShortcutPath,
                        LPWSTR lpszFilePath)
{
  HRESULT hRes = E_FAIL;
  CComPtr<IShellLink> ipShellLink;
  // buffer that receives the null-terminated string
  // for the drive and path
  TCHAR szPath[MAX_PATH];
  // buffer that receives the null-terminated
  // string for the description
  TCHAR szDesc[MAX_PATH];

  WCHAR wszTemp[MAX_PATH];

  lpszFilePath[0] = L'\0';
  // Get a pointer to the IShellLink interface
  hRes = CoCreateInstance(CLSID_ShellLink,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IShellLink,
                          (void**)&ipShellLink);

  if (SUCCEEDED(hRes)) {
    // Get a pointer to the IPersistFile interface
    CComQIPtr<IPersistFile> ipPersistFile(ipShellLink);
    // IPersistFile is using LPCOLESTR,
    // so make sure that the string is Unicode

    wcsncpy(wszTemp, lpszShortcutPath, MAX_PATH);
    // Open the shortcut file and initialize it from its contents

    hRes = ipPersistFile->Load(wszTemp, STGM_READ);
    if (SUCCEEDED(hRes)) {
      // Try to find the target of a shortcut,
      // even if it has been moved or renamed
      hRes = ipShellLink->Resolve(NULL, SLR_UPDATE);
      if (SUCCEEDED(hRes)) {
        // structure that receives the information about the shortcut
        WIN32_FIND_DATA wfd;
        // Get the path to the shortcut target
        hRes = ipShellLink->GetPath(szPath,
                                    MAX_PATH, &wfd, SLGP_RAWPATH);
        if (FAILED(hRes))
          return hRes;

        // Get the description of the target
        hRes = ipShellLink->GetDescription(szDesc, MAX_PATH);
        if (FAILED(hRes))
          return hRes;

        lstrcpyn(lpszFilePath, szPath, MAX_PATH);
      }
    }
  }

  return hRes;
}
