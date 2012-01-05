/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#pragma once

// PasswordSubsetDlg.h : header file
//-----------------------------------------------------------------------------

#include "core/PwsPlatform.h"
#include "PWDialog.h"
#include "ControlExtns.h"

class DboxMain;

#define WM_DISPLAYPASSWORDSUBSET (WM_APP + 1)

// Simple class to ensure only numbers, space, comma and semi-colons
// are entered
class CNumEdit : public CEdit
{
  // Generated message map functions
protected:
  //{{AFX_MSG(CNumEdit)
  afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

class CPasswordSubsetDlg : public CPWDialog
{
public:
  CPasswordSubsetDlg(CWnd* pParent, const StringX &passwd); // standard constructor

  enum { IDD = IDD_PASSWORDSUBSET };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
  BOOL OnInitDialog();
  virtual void OnCancel();
  //{{AFX_MSG(CPasswordSubsetDlg)
  afx_msg HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

private:
  DboxMain *m_pDbx;
  LRESULT OnDisplayStatus(WPARAM /* wParam */, LPARAM /* lParam */);

  const StringX m_passwd;
  CNumEdit m_ne_subset;
  CStaticExtn m_stcwarningmsg;
  CEdit m_results;
  CString m_subset, m_warningmsg;
  bool m_bshown;
};
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
