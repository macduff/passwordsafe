/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// OptionsBackup.cpp : implementation file
//

#include "stdafx.h"
#include "passwordsafe.h"
#include "GeneralMsgBox.h"
#include "Options_PropertySheet.h"

#include "corelib/PwsPlatform.h"
#include "corelib/PWSprefs.h" // for DoubleClickAction enums
#include "corelib/util.h" // for datetime string

#include "os/dir.h"

#if defined(POCKET_PC)
#include "pocketpc/resource.h"
#else
#include "resource.h"
#include "resource3.h"  // String resources
#endif

#include "OptionsBackup.h" // Must be after resource.h

#include <shlwapi.h>
#include <shlobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int CALLBACK SetSelProc(HWND hWnd, UINT uMsg, LPARAM , LPARAM lpData);

/////////////////////////////////////////////////////////////////////////////
// COptionsBackup property page

IMPLEMENT_DYNCREATE(COptionsBackup, COptions_PropertyPage)

COptionsBackup::COptionsBackup()
  : COptions_PropertyPage(COptionsBackup::IDD),
  m_pToolTipCtrl(NULL)
{
  //{{AFX_DATA_INIT(COptionsBackup)
  //}}AFX_DATA_INIT
}

COptionsBackup::~COptionsBackup()
{
  delete m_pToolTipCtrl;
}

void COptionsBackup::SetCurFile(const CString &currentFile)
{
  // derive current db's directory and basename:
  std::wstring path(currentFile);
  std::wstring drive, dir, base, ext;

  pws_os::splitpath(path, drive, dir, base, ext);
  path = pws_os::makepath(drive, dir, L"", L"");
  m_currentFileDir = path.c_str();
  m_currentFileBasename = base.c_str();
}

void COptionsBackup::DoDataExchange(CDataExchange* pDX)
{
  COptions_PropertyPage::DoDataExchange(pDX);

  //{{AFX_DATA_MAP(COptionsBackup)
  DDX_Check(pDX, IDC_SAVEIMMEDIATELY, m_saveimmediately);
  DDX_Check(pDX, IDC_BACKUPBEFORESAVE, m_backupbeforesave);
  DDX_Radio(pDX, IDC_DFLTBACKUPPREFIX, m_backupprefix); // only first!
  DDX_Text(pDX, IDC_USERBACKUPPREFIXVALUE, m_userbackupprefix);
  DDX_Control(pDX, IDC_BACKUPSUFFIX, m_backupsuffix_cbox);
  DDX_Radio(pDX, IDC_DFLTBACKUPLOCATION, m_backuplocation); // only first!
  DDX_Text(pDX, IDC_USERBACKUPOTHRLOCATIONVALUE, m_userbackupotherlocation);
  DDX_Text(pDX, IDC_BACKUPMAXINC, m_maxnumincbackups);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptionsBackup, COptions_PropertyPage)
  //{{AFX_MSG_MAP(COptionsBackup)
  ON_BN_CLICKED(IDC_BACKUPBEFORESAVE, OnBackupBeforeSave)
  ON_BN_CLICKED(IDC_DFLTBACKUPPREFIX, OnBackupPrefix)
  ON_BN_CLICKED(IDC_USERBACKUPPREFIX, OnBackupPrefix)
  ON_BN_CLICKED(IDC_DFLTBACKUPLOCATION, OnBackupDirectory)
  ON_BN_CLICKED(IDC_USERBACKUPOTHERLOCATION, OnBackupDirectory)
  ON_BN_CLICKED(IDC_BROWSEFORLOCATION, OnBrowseForLocation)
  ON_CBN_SELCHANGE(IDC_BACKUPSUFFIX, OnComboChanged)
  ON_EN_KILLFOCUS(IDC_USERBACKUPPREFIXVALUE, OnUserPrefixKillfocus)
  ON_MESSAGE(PSM_QUERYSIBLINGS, OnQuerySiblings)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL COptionsBackup::OnInitDialog()
{
  COptions_PropertyPage::OnInitDialog();

  if (m_backupsuffix_cbox.GetCount() == 0) {
    // add the strings in alphabetical order
    CString cs_text(MAKEINTRESOURCE(IDS_NONE));
    int nIndex;
    nIndex = m_backupsuffix_cbox.AddString(cs_text);
    m_backupsuffix_cbox.SetItemData(nIndex, PWSprefs::BKSFX_None);
    m_BKSFX_to_Index[PWSprefs::BKSFX_None] = nIndex;

    cs_text.LoadString(IDS_DATETIMESTRING);
    nIndex = m_backupsuffix_cbox.AddString(cs_text);
    m_backupsuffix_cbox.SetItemData(nIndex, PWSprefs::BKSFX_DateTime);
    m_BKSFX_to_Index[PWSprefs::BKSFX_DateTime] = nIndex;

    cs_text.LoadString(IDS_INCREMENTNUM);
    nIndex = m_backupsuffix_cbox.AddString(cs_text);
    m_backupsuffix_cbox.SetItemData(nIndex, PWSprefs::BKSFX_IncNumber);
    m_BKSFX_to_Index[PWSprefs::BKSFX_IncNumber] = nIndex;
  }

  if (m_backupsuffix < PWSprefs::minBKSFX ||
      m_backupsuffix > PWSprefs::maxBKSFX)
    m_backupsuffix = PWSprefs::BKSFX_None;

  m_backupsuffix_cbox.SetCurSel(m_BKSFX_to_Index[m_backupsuffix]);

  GetDlgItem(IDC_BACKUPEXAMPLE)->SetWindowText(L"");

  CSpinButtonCtrl* pspin = (CSpinButtonCtrl *)GetDlgItem(IDC_BKPMAXINCSPIN);

  pspin->SetBuddy(GetDlgItem(IDC_BACKUPMAXINC));
  pspin->SetRange(1, 999);
  pspin->SetBase(10);
  pspin->SetPos(m_maxnumincbackups);

  OnComboChanged();
  OnBackupBeforeSave();

  m_saveuserbackupprefix = m_userbackupprefix;
  m_saveuserbackupotherlocation = m_userbackupotherlocation;
  m_savesaveimmediately = m_saveimmediately;
  m_savebackupbeforesave = m_backupbeforesave;
  m_savebackupprefix = m_backupprefix;
  m_savebackuplocation = m_backuplocation;
  m_savemaxnumincbackups = m_maxnumincbackups;
  m_savebackupsuffix = m_backupsuffix;

  // Tooltips on Property Pages
  EnableToolTips();

  m_pToolTipCtrl = new CToolTipCtrl;
  if (!m_pToolTipCtrl->Create(this, TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX)) {
    TRACE(L"Unable To create Property Page ToolTip\n");
    return TRUE;
  }

  // Activate the tooltip control.
  m_pToolTipCtrl->Activate(TRUE);
  m_pToolTipCtrl->SetMaxTipWidth(300);
  // Quadruple the time to allow reading by user - there is a lot there!
  int iTime = m_pToolTipCtrl->GetDelayTime(TTDT_AUTOPOP);
  m_pToolTipCtrl->SetDelayTime(TTDT_AUTOPOP, 4 * iTime);

  // Set the tooltip
  // Note naming convention: string IDS_xxx corresponds to control IDC_xxx
  CString cs_ToolTip;
  cs_ToolTip.LoadString(IDS_BACKUPBEFORESAVE);
  m_pToolTipCtrl->AddTool(GetDlgItem(IDC_BACKUPBEFORESAVE), cs_ToolTip);
  cs_ToolTip.LoadString(IDS_USERBACKUPOTHERLOCATION);
  m_pToolTipCtrl->AddTool(GetDlgItem(IDC_USERBACKUPOTHERLOCATION), cs_ToolTip);

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// COptionsBackup message handlers

void COptionsBackup::OnComboChanged()
{
  int nIndex = m_backupsuffix_cbox.GetCurSel();
  m_backupsuffix = m_backupsuffix_cbox.GetItemData(nIndex);
  if (m_backupsuffix == PWSprefs::BKSFX_IncNumber) {
    GetDlgItem(IDC_BACKUPMAXINC)->EnableWindow(TRUE);
    GetDlgItem(IDC_BKPMAXINCSPIN)->EnableWindow(TRUE);
    GetDlgItem(IDC_BACKUPMAX)->EnableWindow(TRUE);
  } else {
    GetDlgItem(IDC_BACKUPMAXINC)->EnableWindow(FALSE);
    GetDlgItem(IDC_BKPMAXINCSPIN)->EnableWindow(FALSE);
    GetDlgItem(IDC_BACKUPMAX)->EnableWindow(FALSE);
  }
  SetExample();
}

void COptionsBackup::OnBackupPrefix()
{
  UpdateData(TRUE);
  switch (m_backupprefix) {
    case 0:
      GetDlgItem(IDC_USERBACKUPPREFIXVALUE)->EnableWindow(FALSE);
      m_userbackupprefix = L"";
      break;
    case 1:
      GetDlgItem(IDC_USERBACKUPPREFIXVALUE)->EnableWindow(TRUE);
      break;
    default:
      ASSERT(0);
      break;
  }
  UpdateData(FALSE);
  SetExample();
}

void COptionsBackup::OnBackupDirectory()
{
  UpdateData(TRUE);
  switch (m_backuplocation) {
    case 0:
      GetDlgItem(IDC_USERBACKUPOTHRLOCATIONVALUE)->EnableWindow(FALSE);
      GetDlgItem(IDC_BROWSEFORLOCATION)->EnableWindow(FALSE);
      m_userbackupotherlocation = L"";
      break;
    case 1:
      GetDlgItem(IDC_USERBACKUPOTHRLOCATIONVALUE)->EnableWindow(TRUE);
      GetDlgItem(IDC_BROWSEFORLOCATION)->EnableWindow(TRUE);
      break;
    default:
      ASSERT(0);
      break;
  }
  UpdateData(FALSE);
}

void COptionsBackup::OnBackupBeforeSave()
{
  UpdateData(TRUE);

  GetDlgItem(IDC_DFLTBACKUPPREFIX)->EnableWindow(m_backupbeforesave);
  GetDlgItem(IDC_USERBACKUPPREFIX)->EnableWindow(m_backupbeforesave);
  GetDlgItem(IDC_USERBACKUPPREFIXVALUE)->EnableWindow(m_backupbeforesave);
  GetDlgItem(IDC_BACKUPSUFFIX)->EnableWindow(m_backupbeforesave);
  GetDlgItem(IDC_DFLTBACKUPLOCATION)->EnableWindow(m_backupbeforesave);
  GetDlgItem(IDC_USERBACKUPOTHERLOCATION)->EnableWindow(m_backupbeforesave);
  GetDlgItem(IDC_USERBACKUPOTHRLOCATIONVALUE)->EnableWindow(m_backupbeforesave);

  if (m_backupbeforesave == TRUE) {
    OnBackupPrefix();
    OnBackupDirectory();
    SetExample();
  }
}

void COptionsBackup::SetExample()
{
  CString cs_example;
  UpdateData(TRUE);
  switch (m_backupprefix) {
  case 0:
    cs_example = m_currentFileBasename;
    break;
  case 1:
    cs_example = m_userbackupprefix;
    break;
  default:
    ASSERT(0);
    break;
  }

  switch (m_backupsuffix) {
  case 1:
    {
      time_t now;
      time(&now);
      CString cs_datetime = PWSUtil::ConvertToDateTimeString(now,
                                                             TMC_EXPORT_IMPORT).c_str();
      cs_example += L"_";
      cs_example = cs_example + cs_datetime.Left(4) +  // YYYY
        cs_datetime.Mid(5,2) +  // MM
        cs_datetime.Mid(8,2) +  // DD
        L"_" +
        cs_datetime.Mid(11,2) +  // HH
        cs_datetime.Mid(14,2) +  // MM
        cs_datetime.Mid(17,2);   // SS
      break;
    }
  case 2:
    cs_example += L"_001";
    break;
  case 0:
  default:
    break;
  }

  cs_example += L".ibak";
  GetDlgItem(IDC_BACKUPEXAMPLE)->SetWindowText(cs_example);
}

BOOL COptionsBackup::OnKillActive()
{
  COptions_PropertyPage::OnKillActive();

  if (m_backupbeforesave != TRUE)
    return TRUE;

  CGeneralMsgBox gmb;
  // Check that correct fields are non-blank.
  if (m_backupprefix == 1  && m_userbackupprefix.IsEmpty()) {
    gmb.AfxMessageBox(IDS_OPTBACKUPPREF);
    ((CEdit*)GetDlgItem(IDC_USERBACKUPPREFIXVALUE))->SetFocus();
    return FALSE;
  }

  if (m_backuplocation == 1) {
    if (m_userbackupotherlocation.IsEmpty()) {
      gmb.AfxMessageBox(IDS_OPTBACKUPLOCATION);
      ((CEdit*)GetDlgItem(IDC_USERBACKUPOTHRLOCATIONVALUE))->SetFocus();
      return FALSE;
    }

    if (m_userbackupotherlocation.Right(1) != L"\\") {
      m_userbackupotherlocation += L"\\";
      UpdateData(FALSE);
    }

    if (PathIsDirectory(m_userbackupotherlocation) == FALSE) {
      gmb.AfxMessageBox(IDS_OPTBACKUPNOLOC);
      ((CEdit*)GetDlgItem(IDC_USERBACKUPOTHRLOCATIONVALUE))->SetFocus();
      return FALSE;
    }
  }

  if (m_backupsuffix == PWSprefs::BKSFX_IncNumber &&
    ((m_maxnumincbackups < 1) || (m_maxnumincbackups > 999))) {
      gmb.AfxMessageBox(IDS_OPTBACKUPMAXNUM);
      ((CEdit*)GetDlgItem(IDC_BACKUPMAXINC))->SetFocus();
      return FALSE;
  }

  //End check

  return TRUE;
}

void COptionsBackup::OnUserPrefixKillfocus()
{
  SetExample();
}

// Override PreTranslateMessage() so RelayEvent() can be
// called to pass a mouse message to CPWSOptions's
// tooltip control for processing.
BOOL COptionsBackup::PreTranslateMessage(MSG* pMsg)
{
  if (m_pToolTipCtrl != NULL)
    m_pToolTipCtrl->RelayEvent(pMsg);

  return COptions_PropertyPage::PreTranslateMessage(pMsg);
}

LRESULT COptionsBackup::OnQuerySiblings(WPARAM wParam, LPARAM )
{
  UpdateData(TRUE);

  // Have any of my fields been changed?
  switch (wParam) {
    case PP_DATA_CHANGED:
      if (m_saveuserbackupprefix        != m_userbackupprefix        ||
          m_saveuserbackupotherlocation != m_userbackupotherlocation ||
          m_savesaveimmediately         != m_saveimmediately         ||
          m_savebackupbeforesave        != m_backupbeforesave        ||
          m_savebackupprefix            != m_backupprefix            ||
          m_savebackupsuffix            != m_backupsuffix            ||
          m_savebackuplocation          != m_backuplocation          ||
          m_savemaxnumincbackups        != m_maxnumincbackups)
        return 1L;
      break;
    case PP_UPDATE_VARIABLES:
      // Since OnOK calls OnApply after we need to verify and/or
      // copy data into the entry - we do it ourselfs here first
      if (OnApply() == FALSE)
        return 1L;
  }
  return 0L;
}

void COptionsBackup::OnBrowseForLocation()
{
  CString cs_initiallocation;
  if (m_userbackupotherlocation.IsEmpty()) {
    cs_initiallocation = m_currentFileDir;
  } else
    cs_initiallocation = m_userbackupotherlocation;

  // The BROWSEINFO struct tells the shell
  // how it should display the dialog.
  BROWSEINFO bi;
  SecureZeroMemory(&bi, sizeof(bi));

  bi.hwndOwner = this->GetSafeHwnd();
  bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
  CString cs_text(MAKEINTRESOURCE(IDS_OPTBACKUPTITLE));
  bi.lpszTitle = cs_text;
  bi.lpfn = SetSelProc;
  bi.lParam = (LPARAM)(LPCWSTR) cs_initiallocation;

  // Show the dialog and get the itemIDList for the
  // selected folder.
  LPITEMIDLIST pIDL = ::SHBrowseForFolder(&bi);

  if (pIDL != NULL) {
    // Create a buffer to store the path, then
    // get the path.
    wchar_t buffer[_MAX_PATH] = { 0 };
    if(::SHGetPathFromIDList(pIDL, buffer) != 0)
      m_userbackupotherlocation = CString(buffer);
    else
      m_userbackupotherlocation = L"";

    UpdateData(FALSE);

    // free the item id list
    CoTaskMemFree(pIDL);
  }
}

//  SetSelProc
//  Callback procedure to set the initial selection of the browser.
int CALLBACK SetSelProc(HWND hWnd, UINT uMsg, LPARAM , LPARAM lpData)
{
  if (uMsg == BFFM_INITIALIZED) {
    ::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
  }
  return 0;
}