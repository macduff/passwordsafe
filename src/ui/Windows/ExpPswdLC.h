/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

class CExpPswdLC : public CListCtrl
{
public:
  CExpPswdLC();
  ~CExpPswdLC();

protected:
  WCHAR *m_pwchTip;
  char *m_pchTip;

  virtual BOOL PreTranslateMessage(MSG* pMsg);
  virtual void PreSubclassWindow();

  //{{AFX_MSG(CEBListCtrl)
  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
  afx_msg BOOL OnToolTipText(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
  CToolTipCtrl *m_pToolTipCtrl;
  int m_LastToolTipRow;
};