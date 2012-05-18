/*
 *Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
 *All rights reserved. Use of the code is allowed under the
 *Artistic License 2.0 terms, as specified in the LICENSE file
 *distributed with this code, or available from
 *http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// ListView.h : interface of the CPWView class
//

#pragma once

#include "afxcview.h"  // For CTreeView and CListView
#include "afxole.h"

class CxSplitterWnd;

class CPWView : public CView
{

protected: // create from serialization only
  CPWView();
  ~CPWView();

  DECLARE_DYNCREATE(CPWView)

  //{{AFX_VIRTUAL(CPWView)
  virtual BOOL PreCreateWindow(CREATESTRUCT &cs);
  BOOL OnPreparePrinting(CPrintInfo *) {return FALSE;} // No printing allowed
  //}}AFX_VIRTUAL

protected:
  // Generated message map functions
  //{{AFX_MSG(CPWView)
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnDestroy();
  afx_msg void OnDraw(CDC *pDC);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
};
