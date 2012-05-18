/*
 *Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
 *All rights reserved. Use of the code is allowed under the
 *Artistic License 2.0 terms, as specified in the LICENSE file
 *distributed with this code, or available from
 *http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// ListView.cpp : implementation of the CPWView class
//

#include "stdafx.h"

#include "View.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CPWView

IMPLEMENT_DYNCREATE(CPWView, CView)

BEGIN_MESSAGE_MAP(CPWView, CView)
  //{{AFX_MSG_MAP(CPWView)
  ON_WM_CREATE()
  ON_WM_DESTROY()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPWView construction/destruction

CPWView::CPWView()
{
}

CPWView::~CPWView()
{
}

BOOL CPWView::PreCreateWindow(CREATESTRUCT &cs)
{
  return CView::PreCreateWindow(cs);
}

// CPWView message handlers

int CPWView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CView::OnCreate(lpCreateStruct) == -1)
    return -1;

  return 0;
}

void CPWView::OnDestroy()
{
}

void CPWView::OnDraw(CDC *pDC)
{
  CView::OnDraw(pDC);
}
