/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// OptionsPasswordHistory.cpp : implementation file
//

#include "stdafx.h"
#include "passwordsafe.h"
#include "GeneralMsgBox.h"
#include "DboxMain.h"  // needed for DboxMain::UpdatePasswordHistory
#include "Options_PropertySheet.h"

#include "corelib/PwsPlatform.h"

#if defined(POCKET_PC)
#include "pocketpc/resource.h"
#else
#include "resource.h"
#include "resource3.h"  // String resources
#endif

#include "OptionsPasswordHistory.h" // Must be after resource.h

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsPasswordHistory property page

IMPLEMENT_DYNCREATE(COptionsPasswordHistory, COptions_PropertyPage)

COptionsPasswordHistory::COptionsPasswordHistory()
  : COptions_PropertyPage(COptionsPasswordHistory::IDD),
  m_ToolTipCtrl(NULL), m_pDboxMain(NULL), m_pwhaction(0)
{
  //{{AFX_DATA_INIT(COptionsPasswordHistory)
  //}}AFX_DATA_INIT
}

COptionsPasswordHistory::~COptionsPasswordHistory()
{
  delete m_ToolTipCtrl;
}

void COptionsPasswordHistory::DoDataExchange(CDataExchange* pDX)
{
  COptions_PropertyPage::DoDataExchange(pDX);

  //{{AFX_DATA_MAP(COptionsPasswordHistory)
  DDX_Check(pDX, IDC_SAVEPWHISTORY, m_savepwhistory);
  DDX_Text(pDX, IDC_DEFPWHNUM, m_pwhistorynumdefault);
  //}}AFX_DATA_MAP
  DDX_Radio(pDX, IDC_PWHISTORYNOACTION, m_pwhaction);
}

BEGIN_MESSAGE_MAP(COptionsPasswordHistory, COptions_PropertyPage)
  //{{AFX_MSG_MAP(COptionsPasswordHistory)
  ON_BN_CLICKED(IDC_SAVEPWHISTORY, OnSavePWHistory)
  ON_BN_CLICKED(IDC_APPLYPWHCHANGESNOW, OnApplyPWHChanges)
  //}}AFX_MSG_MAP
  ON_BN_CLICKED(IDC_PWHISTORYNOACTION, OnPWHistoryNoAction)
  ON_BN_CLICKED(IDC_RESETPWHISTORYOFF, OnPWHistoryDoAction)
  ON_BN_CLICKED(IDC_RESETPWHISTORYON, OnPWHistoryDoAction)
  ON_BN_CLICKED(IDC_SETMAXPWHISTORY, OnPWHistoryDoAction)
  ON_MESSAGE(PSM_QUERYSIBLINGS, OnQuerySiblings)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsPasswordHistory message handlers

BOOL COptionsPasswordHistory::OnInitDialog() 
{
  COptions_PropertyPage::OnInitDialog();

  CSpinButtonCtrl *pspin = (CSpinButtonCtrl *)GetDlgItem(IDC_PWHSPIN);

  pspin->SetBuddy(GetDlgItem(IDC_DEFPWHNUM));
  pspin->SetRange(1, 255);
  pspin->SetBase(10);
  pspin->SetPos(m_pwhistorynumdefault);

  GetDlgItem(IDC_PWHSPIN)->EnableWindow(m_savepwhistory);
  GetDlgItem(IDC_DEFPWHNUM)->EnableWindow(m_savepwhistory);

  m_savesavepwhistory = m_savepwhistory;
  m_savepwhistorynumdefault = m_pwhistorynumdefault;

  // Tooltips on Property Pages
  EnableToolTips();

  m_ToolTipCtrl = new CToolTipCtrl;
  if (!m_ToolTipCtrl->Create(this, TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX)) {
    TRACE(L"Unable To create Property Page ToolTip\n");
    return TRUE;
  }

  // Activate the tooltip control.
  m_ToolTipCtrl->Activate(TRUE);
  m_ToolTipCtrl->SetMaxTipWidth(300);
  // Quadruple the time to allow reading by user - there is a lot there!
  int iTime = m_ToolTipCtrl->GetDelayTime(TTDT_AUTOPOP);
  m_ToolTipCtrl->SetDelayTime(TTDT_AUTOPOP, 4 * iTime);

  // Set the tooltip
  // Note naming convention: string IDS_xxx corresponds to control IDC_xxx
  CString cs_ToolTip;
  cs_ToolTip.LoadString(IDS_RESETPWHISTORYOFF);
  m_ToolTipCtrl->AddTool(GetDlgItem(IDC_RESETPWHISTORYOFF), cs_ToolTip);
  cs_ToolTip.LoadString(IDS_RESETPWHISTORYON);
  m_ToolTipCtrl->AddTool(GetDlgItem(IDC_RESETPWHISTORYON), cs_ToolTip);
  cs_ToolTip.LoadString(IDS_SETMAXPWHISTORY);
  m_ToolTipCtrl->AddTool(GetDlgItem(IDC_SETMAXPWHISTORY), cs_ToolTip);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL COptionsPasswordHistory::OnKillActive()
{
  CGeneralMsgBox gmb;
  // Check that options, as set, are valid.
  if (m_savepwhistory && ((m_pwhistorynumdefault < 1) || (m_pwhistorynumdefault > 255))) {
    gmb.AfxMessageBox(IDS_DEFAULTNUMPWH);
    ((CEdit*)GetDlgItem(IDC_DEFPWHNUM))->SetFocus();
    return FALSE;
  }
  //End check

  return COptions_PropertyPage::OnKillActive();;
}

void COptionsPasswordHistory::OnSavePWHistory() 
{
  BOOL enable = (((CButton*)GetDlgItem(IDC_SAVEPWHISTORY))->GetCheck() == 1) ? TRUE : FALSE;
  GetDlgItem(IDC_PWHSPIN)->EnableWindow(enable);
  GetDlgItem(IDC_DEFPWHNUM)->EnableWindow(enable);
}

void COptionsPasswordHistory::OnApplyPWHChanges()
{
  ASSERT(m_pDboxMain != NULL);

  UpdateData(TRUE);
  m_pDboxMain->UpdatePasswordHistory(m_pwhaction, m_pwhistorynumdefault);

  m_pwhaction = 0;
  GetDlgItem(IDC_APPLYPWHCHANGESNOW)->EnableWindow(FALSE);
  UpdateData(FALSE);
}

// Override PreTranslateMessage() so RelayEvent() can be 
// called to pass a mouse message to CPWSOptions's 
// tooltip control for processing.
BOOL COptionsPasswordHistory::PreTranslateMessage(MSG* pMsg) 
{
  if (m_ToolTipCtrl != NULL)
    m_ToolTipCtrl->RelayEvent(pMsg);

  return COptions_PropertyPage::PreTranslateMessage(pMsg);
}

LRESULT COptionsPasswordHistory::OnQuerySiblings(WPARAM wParam, LPARAM )
{
  UpdateData(TRUE);

  // Have any of my fields been changed?
  switch (wParam) {
    case PP_DATA_CHANGED:
      if (m_savesavepwhistory        != m_savepwhistory        ||
          (m_savepwhistory           == TRUE &&
           m_savepwhistorynumdefault != m_pwhistorynumdefault))
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

void COptionsPasswordHistory::OnPWHistoryNoAction()
{
  GetDlgItem(IDC_APPLYPWHCHANGESNOW)->EnableWindow(FALSE);
}

void COptionsPasswordHistory::OnPWHistoryDoAction() 
{
  GetDlgItem(IDC_APPLYPWHCHANGESNOW)->EnableWindow(TRUE);
}
