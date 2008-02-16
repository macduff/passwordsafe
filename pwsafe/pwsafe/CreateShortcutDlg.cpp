/*
* Copyright (c) 2003-2008 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file CCreateShortcutDlg.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "ThisMfcApp.h"
#include "DboxMain.h"
#include "CreateShortcutDlg.h"
#include "ControlExtns.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------
CCreateShortcutDlg::CCreateShortcutDlg(CWnd* pParent, 
  const CMyString &cs_tg, const CMyString &cs_tt, const CMyString &cs_tu)
  : CPWDialog(CCreateShortcutDlg::IDD, pParent),
  m_tg(cs_tg), m_tt(cs_tt), m_tu(cs_tu), m_group(cs_tg), m_username(cs_tu)
{
}

BOOL CCreateShortcutDlg::OnInitDialog() 
{
  CPWDialog::OnInitDialog();

  // Populate the combo box
  if(m_ex_group.GetCount() == 0) {
    CStringArray aryGroups;
    app.m_core.GetUniqueGroups(aryGroups);
    for (int igrp = 0; igrp < aryGroups.GetSize(); igrp++) {
      m_ex_group.AddString((LPCTSTR)aryGroups[igrp]);
    }
  }

  m_title.Format(IDS_SCTARGET, m_tt);

  CMyString cs_explanation, cs_target(_T(""));
  if (!m_tg.IsEmpty())
    cs_target = m_tg + _T(".");
  cs_target += m_tt;
  if (!m_tu.IsEmpty())
    cs_target += _T(".") + m_tu;
  cs_explanation.Format(IDS_SHORTCUTEXPLANATION, cs_target);
  GetDlgItem(IDC_ADDSCEXPLANATION)->SetWindowText(cs_explanation);

  m_ex_group.ChangeColour();

  UpdateData(FALSE);

  return TRUE;
}

void CCreateShortcutDlg::DoDataExchange(CDataExchange* pDX)
{
  CPWDialog::DoDataExchange(pDX);
  DDX_CBString(pDX, IDC_GROUP, (CString&)m_group);
  DDX_Text(pDX, IDC_TITLE, (CString&)m_title);
  DDX_Text(pDX, IDC_USERNAME, (CString&)m_username);

  DDX_Control(pDX, IDC_GROUP, m_ex_group);
  DDX_Control(pDX, IDC_USERNAME, m_ex_username);
  DDX_Control(pDX, IDC_TITLE, m_ex_title);
}

BEGIN_MESSAGE_MAP(CCreateShortcutDlg, CPWDialog)
  ON_BN_CLICKED(ID_HELP, OnHelp)
  ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

void CCreateShortcutDlg::OnCancel() 
{
  CPWDialog::OnCancel();
}

void CCreateShortcutDlg::OnOK() 
{
  if (UpdateData(TRUE) != TRUE)
    return;

  m_group.EmptyIfOnlyWhiteSpace();
  m_title.EmptyIfOnlyWhiteSpace();
  m_username.EmptyIfOnlyWhiteSpace();

  UpdateData(FALSE);

  //Check that data is valid
  if (m_title.IsEmpty()) {
    AfxMessageBox(IDS_MUSTHAVETITLE);
    ((CEdit*)GetDlgItem(IDC_TITLE))->SetFocus();
    return;
  }

  if (!m_group.IsEmpty() && m_group[0] == '.') {
    AfxMessageBox(IDS_DOTINVALID);
    ((CEdit*)GetDlgItem(IDC_GROUP))->SetFocus();
    return;
  }

  DboxMain* pDbx = static_cast<DboxMain *>(GetParent());
  ASSERT(pDbx != NULL);

  // If there is a matching entry in our list, tell the user to try again.
  if (pDbx->Find(m_group, m_title, m_username) != pDbx->End()) {
    CMyString temp;
    if (m_group.IsEmpty())
      temp.Format(IDS_ENTRYEXISTS2, m_title, m_username);
    else
      temp.Format(IDS_ENTRYEXISTS, m_group, m_title, m_username);
    AfxMessageBox(temp);
    ((CEdit*)GetDlgItem(IDC_TITLE))->SetSel(MAKEWORD(-1, 0));
    ((CEdit*)GetDlgItem(IDC_TITLE))->SetFocus();
    return;
  }
  //End check

  CPWDialog::OnOK();
}

void CCreateShortcutDlg::OnHelp() 
{
#if defined(POCKET_PC)
  CreateProcess( _T("PegHelp.exe"), _T("pws_ce_help.html#adddata"), NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL );
#else
  CString cs_HelpTopic;
  cs_HelpTopic = app.GetHelpFileName() + _T("::/html/entering_pwd.html");
  HtmlHelp(DWORD_PTR((LPCTSTR)cs_HelpTopic), HH_DISPLAY_TOPIC);
#endif
}

void CCreateShortcutDlg::OnBnClickedOk()
{
  OnOK();
}
