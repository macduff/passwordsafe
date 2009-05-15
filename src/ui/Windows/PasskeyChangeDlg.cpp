/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file PasskeyChangeDlg.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "PasswordSafe.h"
#include "corelib/PwsPlatform.h"
#include "corelib/PWScore.h" // for error statuses from CheckPassword()
#include "corelib/PWCharPool.h" // for CheckPassword()
#include "corelib/pwsprefs.h"
#include "ThisMfcApp.h"
#include "os/dir.h"
#include "VirtualKeyboard/VKeyBoardDlg.h"

#if defined(POCKET_PC)
#include "pocketpc/resource.h"
#include "pocketpc/PocketPC.h"
#else
#include "resource.h"
#include "resource3.h"  // String resources
#endif

#include "PasskeyChangeDlg.h"
#include "PwFont.h"

#include <iomanip>  // For setbase and setw

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static TCHAR PSSWDCHAR = TCHAR('*');

//-----------------------------------------------------------------------------
CPasskeyChangeDlg::CPasskeyChangeDlg(CWnd* pParent)
  : CPWDialog(CPasskeyChangeDlg::IDD, pParent), m_pVKeyBoardDlg(NULL),
  m_LastFocus(IDC_OLDPASSKEY), m_OSK_module(NULL)
{
  m_oldpasskey = _T("");
  m_newpasskey = _T("");
  m_confirmnew = _T("");

  m_pctlNewPasskey = new CSecEditExtn;
  m_pctlOldPasskey = new CSecEditExtn;
  m_pctlConfirmNew = new CSecEditExtn;

  if (app.m_bOSK_module) {
    stringT dll_loc = pws_os::getexecdir();
#if defined( _DEBUG ) || defined( DEBUG )
    dll_loc += _T("pws_osk_D.dll");
#else
    dll_loc += _T("pws_osk.dll");
#endif
    m_OSK_module = LoadLibrary(dll_loc.c_str());

    if (m_OSK_module == NULL) {
      TRACE(_T("CPasskeyChangeDlg::CPasskeyChangeDlg - Unable to load OSK DLL. OSK not available.\n"));
    } else {
      TRACE(_T("CPasskeyChangeDlg::CPasskeyChangeDlg - OSK DLL loaded OK.\n"));
    }
  }
}

CPasskeyChangeDlg::~CPasskeyChangeDlg()
{
  delete m_pctlOldPasskey;
  delete m_pctlNewPasskey;
  delete m_pctlConfirmNew;
  
  if (m_OSK_module != NULL) {
    BOOL brc = FreeLibrary(m_OSK_module);
    pws_os::Trace(_T("CPasskeyChangeDlg::~CPasskeyChangeDlg - Free OSK DLL: %s\n"),
                  brc == TRUE ? _T("OK") : _T("FAILED"));
    m_OSK_module = NULL;
  }

  if (m_pVKeyBoardDlg != NULL) {
    // Save Last Used Keyboard
    UINT uiKLID = m_pVKeyBoardDlg->GetKLID();
    std::wostringstream os;
    os.fill(L'0');
    os << std::nouppercase << std::hex << std::setw(8) << uiKLID;
    StringX cs_KLID = os.str().c_str();
    PWSprefs::GetInstance()->SetPref(PWSprefs::LastUsedKeyboard, cs_KLID);

    m_pVKeyBoardDlg->DestroyWindow();
    delete m_pVKeyBoardDlg;
  }
}

void CPasskeyChangeDlg::DoDataExchange(CDataExchange* pDX)
{
  CPWDialog::DoDataExchange(pDX);

  // Can't use DDX_Text for CSecEditExtn
  m_pctlOldPasskey->DoDDX(pDX, m_oldpasskey);
  m_pctlNewPasskey->DoDDX(pDX, m_newpasskey);
  m_pctlConfirmNew->DoDDX(pDX, m_confirmnew);

  DDX_Control(pDX, IDC_CONFIRMNEW, *m_pctlConfirmNew);
  DDX_Control(pDX, IDC_NEWPASSKEY, *m_pctlNewPasskey);
  DDX_Control(pDX, IDC_OLDPASSKEY, *m_pctlOldPasskey);
}

BEGIN_MESSAGE_MAP(CPasskeyChangeDlg, CPWDialog)
  ON_BN_CLICKED(ID_HELP, OnHelp)
  ON_EN_SETFOCUS(IDC_OLDPASSKEY, OnPasskeySetfocus)
  ON_EN_SETFOCUS(IDC_NEWPASSKEY, OnNewPasskeySetfocus)
  ON_EN_SETFOCUS(IDC_CONFIRMNEW, OnConfirmNewSetfocus)
#if defined(POCKET_PC)
  ON_EN_KILLFOCUS(IDC_OLDPASSKEY, OnPasskeyKillfocus)
  ON_EN_KILLFOCUS(IDC_NEWPASSKEY, OnPasskeyKillfocus)
  ON_EN_KILLFOCUS(IDC_CONFIRMNEW, OnPasskeyKillfocus)
#endif
  ON_STN_CLICKED(IDC_VKB, OnVirtualKeyboard)
  ON_MESSAGE(WM_INSERTBUFFER, OnInsertBuffer)
END_MESSAGE_MAP()

BOOL CPasskeyChangeDlg::OnInitDialog()
{
  CPWDialog::OnInitDialog();

  ApplyPasswordFont(GetDlgItem(IDC_OLDPASSKEY));
  ApplyPasswordFont(GetDlgItem(IDC_NEWPASSKEY));
  ApplyPasswordFont(GetDlgItem(IDC_CONFIRMNEW));

  m_pctlOldPasskey->SetPasswordChar(PSSWDCHAR);
  m_pctlNewPasskey->SetPasswordChar(PSSWDCHAR);
  m_pctlConfirmNew->SetPasswordChar(PSSWDCHAR);

  // Only show virtual Keyboard menu if we can load DLL
  if (!app.m_bOSK_module) {
    GetDlgItem(IDC_VKB)->ShowWindow(SW_HIDE);
    GetDlgItem(IDC_VKB)->EnableWindow(FALSE);
  }

  return TRUE;
}

void CPasskeyChangeDlg::OnOK() 
{
  StringX errmess;
  CString cs_msg, cs_text;

  UpdateData(TRUE);
  int rc = app.m_core.CheckPassword(app.m_core.GetCurFile(), m_oldpasskey);
  if (rc == PWScore::WRONG_PASSWORD)
    AfxMessageBox(IDS_WRONGOLDPHRASE);
  else if (rc == PWScore::CANT_OPEN_FILE)
    AfxMessageBox(IDS_CANTVERIFY);
  else if (m_confirmnew != m_newpasskey)
    AfxMessageBox(IDS_NEWOLDDONOTMATCH);
  else if (m_newpasskey.IsEmpty())
    AfxMessageBox(IDS_CANNOTBEBLANK);
  // Vox populi vox dei - folks want the ability to use a weak
  // passphrase, best we can do is warn them...
  // If someone want to build a version that insists on proper
  // passphrases, then just define the preprocessor macro
  // PWS_FORCE_STRONG_PASSPHRASE in the build properties/Makefile
  // (also used in CPasskeySetup)
  else if (!CPasswordCharPool::CheckPassword(m_newpasskey, errmess)) {
    cs_msg.Format(IDS_WEAKPASSPHRASE, errmess.c_str());
#ifndef PWS_FORCE_STRONG_PASSPHRASE
    cs_text.LoadString(IDS_USEITANYWAY);
    cs_msg += cs_text;
    int rc = AfxMessageBox(cs_msg, MB_YESNO | MB_ICONSTOP);
    if (rc == IDYES)
      CPWDialog::OnOK();
#else
    cs_text.LoadString(IDS_TRYANOTHER);
    cs_msg += cs_text;
    AfxMessageBox(cs_msg, MB_OK | MB_ICONSTOP);
#endif // PWS_FORCE_STRONG_PASSPHRASE
  } else {
    CPWDialog::OnOK();
  }
}

void CPasskeyChangeDlg::OnCancel() 
{
  CPWDialog::OnCancel();
}

void CPasskeyChangeDlg::OnHelp() 
{
#if defined(POCKET_PC)
  CreateProcess( _T("PegHelp.exe"), _T("pws_ce_help.html#changecombo"), NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL );
#else
  CString cs_HelpTopic;
  cs_HelpTopic = app.GetHelpFileName() + _T("::/html/change_combo.html");
  HtmlHelp(DWORD_PTR((LPCTSTR)cs_HelpTopic), HH_DISPLAY_TOPIC);
#endif
}

#if defined(POCKET_PC)
/************************************************************************/
/* Restore the state of word completion when the password field loses   */
/* focus.                                                               */
/************************************************************************/
void CPasskeyChangeDlg::OnPasskeyKillfocus()
{
  EnableWordCompletion( m_hWnd );
}
#endif

void CPasskeyChangeDlg::OnPasskeySetfocus()
{
  m_LastFocus = IDC_OLDPASSKEY;

#if defined(POCKET_PC)
/************************************************************************/
/* When the password field is activated, pull up the SIP and disable    */
/* word completion.                                                     */
/************************************************************************/
  DisableWordCompletion( m_hWnd );
#endif
}

void CPasskeyChangeDlg::OnNewPasskeySetfocus()
{
  m_LastFocus = IDC_NEWPASSKEY;

#if defined(POCKET_PC)
/************************************************************************/
/* When the password field is activated, pull up the SIP and disable    */
/* word completion.                                                     */
/************************************************************************/
  DisableWordCompletion( m_hWnd );
#endif
}

void CPasskeyChangeDlg::OnConfirmNewSetfocus()
{
  m_LastFocus = IDC_CONFIRMNEW;

#if defined(POCKET_PC)
/************************************************************************/
/* When the password field is activated, pull up the SIP and disable    */
/* word completion.                                                     */
/************************************************************************/
  DisableWordCompletion( m_hWnd );
#endif
}

void CPasskeyChangeDlg::OnVirtualKeyboard()
{
  // Shouldn't be here if couldn't load DLL. Static control disabled/hidden
  if (!app.m_bOSK_module || m_OSK_module == NULL)
    return;

  if (m_pVKeyBoardDlg != NULL && m_pVKeyBoardDlg->IsWindowVisible()) {
    // Already there - move to top
    m_pVKeyBoardDlg->SetWindowPos(&wndTop , 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    return;
  }

  // If not already created - do it, otherwise just reset it
  if (m_pVKeyBoardDlg == NULL) {
    StringX cs_LUKBD = PWSprefs::GetInstance()->GetPref(PWSprefs::LastUsedKeyboard);
    m_pVKeyBoardDlg = new CVKeyBoardDlg(this, m_OSK_module, cs_LUKBD.c_str());
    m_pVKeyBoardDlg->Create(CVKeyBoardDlg::IDD);
  } else {
    m_pVKeyBoardDlg->ResetKeyboard();
  }

  // Now show it and make it top
  m_pVKeyBoardDlg->SetWindowPos(&wndTop , 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);

  return;
}

LRESULT CPasskeyChangeDlg::OnInsertBuffer(WPARAM, LPARAM)
{
  // Update the variables
  UpdateData(TRUE);

  // Get the buffer
  CSecString vkbuffer = m_pVKeyBoardDlg->GetPassphrase();

  CSecEditExtn * m_pSecCtl(NULL);
  CSecString * m_pSecString;

  switch (m_LastFocus) {
    case IDC_OLDPASSKEY:
      m_pSecCtl = m_pctlOldPasskey;
      m_pSecString = &m_oldpasskey;
      break;
    case IDC_NEWPASSKEY:
      m_pSecCtl = m_pctlNewPasskey;
      m_pSecString = &m_newpasskey;
      break;
    case IDC_CONFIRMNEW:
      m_pSecCtl = m_pctlConfirmNew;
      m_pSecString = &m_confirmnew;
      break;
    default:
      // Error!
      ASSERT(0);
      return 0L;
  }

  // Find the selected characters - if any
  int nStartChar, nEndChar;
  m_pSecCtl->GetSel(nStartChar, nEndChar);

  // If any characters seelcted - delete them
  if (nStartChar != nEndChar)
    m_pSecString->Delete(nStartChar, nEndChar - nStartChar);

  // Insert the buffer
  m_pSecString->Insert(nStartChar, vkbuffer);

  // Update the dialog
  UpdateData(FALSE);

  // Put cursor at end of inserted text
  m_pSecCtl->SetSel(nStartChar + vkbuffer.GetLength(), 
                    nStartChar + vkbuffer.GetLength());

  return 0L;
}
