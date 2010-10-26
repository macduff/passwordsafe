/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// ConfirmDeleteDlg.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "ThisMfcApp.h"
#include "ConfirmDeleteDlg.h"
#include "corelib/PwsPlatform.h"
#include "corelib/PWSprefs.h"

#if defined(POCKET_PC)
#include "pocketpc/PocketPC.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------
CConfirmDeleteDlg::CConfirmDeleteDlg(CWnd* pParent, const int numchildren, const size_t numatts,
     const StringX sxGroup, const StringX sxTitle, const StringX sxUser)
  : CPWDialog(numchildren > 0 ? CConfirmDeleteDlg::IDDGRP : CConfirmDeleteDlg::IDDENT, pParent),
  m_numchildren(numchildren), m_numatts(numatts),
  m_sxGroup(sxGroup), m_sxTitle(sxTitle), m_sxUser(sxUser)
{
  m_dontaskquestion = PWSprefs::GetInstance()->
    GetPref(PWSprefs::DeleteQuestion);
}

void CConfirmDeleteDlg::DoDataExchange(CDataExchange* pDX)
{
  BOOL B_dontaskquestion = m_dontaskquestion ? TRUE : FALSE;

  CPWDialog::DoDataExchange(pDX);
  if (m_numchildren == 0)
    DDX_Check(pDX, IDC_CLEARCHECK, B_dontaskquestion);
  m_dontaskquestion = B_dontaskquestion == TRUE;
}

BEGIN_MESSAGE_MAP(CConfirmDeleteDlg, CPWDialog)
END_MESSAGE_MAP()

BOOL CConfirmDeleteDlg::OnInitDialog(void)
{
  CString cs_text;
  if (m_numchildren > 0) {
    // Group delete
    if (m_numchildren == 1)
      cs_text.LoadString(IDS_NUMCHILD);
    else
      cs_text.Format(IDS_NUMCHILDREN, m_numchildren);

    // Tell them number of entries in this group & its sub-groups
    GetDlgItem(IDC_DELETECHILDREN)->EnableWindow(TRUE);
    GetDlgItem(IDC_DELETECHILDREN)->SetWindowText(cs_text);
  } else {
    StringX sxEntry;
    sxEntry = L"\xab" + m_sxGroup + L"\xbb " +
              L"\xab" + m_sxTitle + L"\xbb " +
              L"\xab" + m_sxUser  + L"\xbb";
    GetDlgItem(IDC_ENTRY)->SetWindowText(sxEntry.c_str());

    // Allow user to select not to be asked again
    GetDlgItem(IDC_CLEARCHECK)->EnableWindow(TRUE);
  }

  cs_text.LoadString((m_numchildren > 0) ? IDS_DELGRP : IDS_DELENT);
  GetDlgItem(IDC_DELITEM)->SetWindowText(cs_text);

  if (m_numatts > 0) {
    cs_text.Format(m_numchildren > 0 ? IDS_DELGRPATT : IDS_DELENTATT, m_numatts);
    GetDlgItem(IDC_HASATTACHMENTS)->SetWindowText(cs_text);

    if (m_numchildren == 0) {
      GetDlgItem(IDC_CLEARCHECK)->EnableWindow(FALSE);
      GetDlgItem(IDC_CLEARCHECK)->ShowWindow(SW_HIDE);
    }
  } else
    GetDlgItem(IDC_HASATTACHMENTS)->ShowWindow(SW_HIDE);

  return TRUE;
}

void CConfirmDeleteDlg::OnCancel() 
{
  CPWDialog::OnCancel();
}

void CConfirmDeleteDlg::OnOK() 
{
  if (m_numchildren == 0) {
    UpdateData(TRUE);
    PWSprefs::GetInstance()->
      SetPref(PWSprefs::DeleteQuestion, m_dontaskquestion);
  }
  CPWDialog::OnOK();
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
