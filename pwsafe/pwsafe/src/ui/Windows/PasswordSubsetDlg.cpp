/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file PasswordSubsetDlg.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "PasswordSubsetDlg.h"
#include "DboxMain.h"
#include "PwFont.h"
#include "core/StringX.h"
#include "core/PWSprefs.h"

#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CNumEdit, CEdit)
  //{{AFX_MSG_MAP(CNumEdit)
  ON_WM_CHAR()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CNumEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  // Ensure character is a digit or a valid delimiter
  // Otherwise just ignore it!
  if (isdigit(nChar) || nChar == L' ' || nChar == L';' || nChar == L',' ||
      nChar == VK_BACK) {
    CEdit::OnChar(nChar, nRepCnt, nFlags);
  }
  if (nChar == L' ' || nChar == L';' || nChar == L',' || nChar == VK_RETURN)
    GetParent()->SendMessage(WM_DISPLAYPASSWORDSUBSET);
}

//-----------------------------------------------------------------------------
CPasswordSubsetDlg::CPasswordSubsetDlg(CWnd* pParent, const StringX &passwd)
  : CPWDialog(CPasswordSubsetDlg::IDD, pParent),
    m_passwd(passwd), m_bshown(false), m_warningmsg(L"")
{
  m_pDbx = static_cast<DboxMain *>(pParent);
}

void CPasswordSubsetDlg::DoDataExchange(CDataExchange* pDX)
{
  CPWDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CPasswordSubsetDlg)
  DDX_Text(pDX, IDC_SUBSET, m_subset);
  DDX_Text(pDX, IDC_STATICSUBSETWARNING, m_warningmsg);
  DDX_Control(pDX, IDC_SUBSETRESULTS, m_results);
  DDX_Control(pDX, IDC_SUBSET, m_ne_subset);
  DDX_Control(pDX, IDC_STATICSUBSETWARNING, m_stcwarningmsg);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPasswordSubsetDlg, CPWDialog)
  //{{AFX_MSG_MAP(CPasswordSubsetDlg)
  ON_WM_CTLCOLOR()
  ON_MESSAGE(WM_DISPLAYPASSWORDSUBSET, OnDisplayStatus)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CPasswordSubsetDlg::OnInitDialog()
{
  CPWDialog::OnInitDialog();

  ApplyPasswordFont(GetDlgItem(IDC_SUBSETRESULTS));

  CRect rect;
  PWSprefs::GetInstance()->GetPrefPSSRect(rect.top, rect.bottom, 
                                          rect.left, rect.right);

  if (rect.top == -1 && rect.bottom == -1 && rect.left == -1 && rect.right == -1) {
    GetWindowRect(&rect);
  }
  m_pDbx->PlaceWindow(this, &rect, SW_SHOW);

  return TRUE;
}

HBRUSH CPasswordSubsetDlg::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
  HBRUSH hbr = CPWDialog::OnCtlColor(pDC, pWnd, nCtlColor);

  // Only deal with Static controls and then
  // Only with our special one - change colour of warning message
  if (nCtlColor == CTLCOLOR_STATIC && pWnd->GetDlgCtrlID() == IDC_STATICSUBSETWARNING) {
    if (((CStaticExtn *)pWnd)->GetColourState()) {
      COLORREF cfUser = ((CStaticExtn *)pWnd)->GetUserColour();
      pDC->SetTextColor(cfUser);
    }
  }

  // Let's get out of here
  return hbr;
}

void CPasswordSubsetDlg::OnCancel()
{
  CRect rect;
  GetWindowRect(&rect);
  PWSprefs::GetInstance()->SetPrefPSSRect(rect.top, rect.bottom,
                                          rect.left, rect.right);
  if (m_bshown)
    CPWDialog::EndDialog(4);
  else
    CPWDialog::OnCancel();
}

LRESULT CPasswordSubsetDlg::OnDisplayStatus(WPARAM /* wParam */, LPARAM /* lParam */)
{
  UpdateData(TRUE);
  m_stcwarningmsg.SetWindowText(L"");
  m_stcwarningmsg.ResetColour();
  m_subset.Trim();

  int icurpos(0), lastpos;
  std::vector<int> vpos;
  CString resToken(m_subset);
  const size_t ipwlengh = m_passwd.length();

  while (resToken != L"" && icurpos != -1) {
    lastpos = icurpos;
    resToken = m_subset.Tokenize(L";, ", icurpos);
    if (resToken == L"")
      continue;

    int ipos = _wtoi(resToken);
    if (ipos > (int)ipwlengh || ipos == 0) {
      if (ipos != 0)
        m_warningmsg.Format(IDS_SUBSETINDEXTOOBIG,ipwlengh);
      else
        m_warningmsg.LoadString(IDS_SUBSETINDEXZERO);

      m_stcwarningmsg.SetWindowText(m_warningmsg);
      m_stcwarningmsg.SetColour(RGB(255, 0, 0));
      m_stcwarningmsg.Invalidate();
      vpos.clear();
      m_ne_subset.SetSel(lastpos, icurpos);
      m_ne_subset.SetFocus();
      return 0L;
    }
    vpos.push_back(ipos - 1);
  };

  std::vector<int>::const_iterator citer;
  StringX sSubset;
  for (citer = vpos.begin(); citer != vpos.end(); citer++) {
    sSubset += m_passwd[*citer];
    sSubset += L" ";
  }
  m_results.SetWindowText(sSubset.c_str());
  m_bshown = true;
  return 1L;
}
//-----------------------------------------------------------------------------
