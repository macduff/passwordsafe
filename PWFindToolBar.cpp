/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

// PWFindToolBar.cpp : implementation file
//

#include "stdafx.h"
#include "PWFindToolBar.h"
#include "ControlExtns.h"
#include "resource.h"
#include "resource2.h"
#include "resource3.h"

#include <vector>
#include <algorithm>

// CPWFindToolBar

// ***** NOTE: THIS TOOLBAR IS NOT CUSTOMIZABLE *****
// *****        IT CAN ONLY BE TOGGLED ON/OFF   *****

// See comments in PWToolBar.cpp for details of these arrays

#define EDITCTRL_WIDTH 100    // width of Edit control for Search Text
#define FINDRESULTS_WIDTH 400 // width of Edit control for Search Results

const UINT CPWFindToolBar::m_FindToolBarIDs[] = {
  ID_TOOLBUTTON_FINDEDITCTRL,
  ID_TOOLBUTTON_FIND,
  ID_TOOLBUTTON_CLEARFIND,
  ID_SEPARATOR,
  ID_TOOLBUTTON_FINDRESULTS
};

const UINT CPWFindToolBar::m_FindToolBarClassicBMs[] = {
  IDB_EDITCTRLPLACEHOLDER,
  IDB_FIND_CLASSIC,
  IDB_CLEARFIND_CLASSIC,
  IDB_EDITCTRLPLACEHOLDER
};

const UINT CPWFindToolBar::m_FindToolBarNew8BMs[] = {
  IDB_EDITCTRLPLACEHOLDER,
  IDB_FIND_NEW8,
  IDB_CLEARFIND_NEW8,
  IDB_EDITCTRLPLACEHOLDER
};

const UINT CPWFindToolBar::m_FindToolBarNew32BMs[] = {
  IDB_EDITCTRLPLACEHOLDER,
  IDB_FIND_NEW32,
  IDB_CLEARFIND_NEW32,
  IDB_EDITCTRLPLACEHOLDER
};

IMPLEMENT_DYNAMIC(CPWFindToolBar, CToolBar)

CPWFindToolBar::CPWFindToolBar()
  :  m_ClassicFlags(0), m_NewFlags(0), m_bitmode(1), m_bVisible(true)
{
  m_iMaxNumButtons = sizeof(m_FindToolBarIDs) / sizeof(UINT);
  m_pOriginalTBinfo = new TBBUTTON[m_iMaxNumButtons];

  ASSERT(sizeof(m_FindToolBarClassicBMs) / sizeof(UINT) ==
         sizeof(m_FindToolBarNew8BMs) / sizeof(UINT));
  ASSERT(sizeof(m_FindToolBarClassicBMs) / sizeof(UINT) ==
         sizeof(m_FindToolBarNew32BMs) / sizeof(UINT));

  m_iNum_Bitmaps = sizeof(m_FindToolBarClassicBMs) / sizeof(UINT);

  LOGFONT lf; 
  memset(&lf, 0, sizeof(lf)); 

  // Since design guide says toolbars are fixed height so is the font. 
  lf.lfHeight = -11; 
  lf.lfWeight = FW_LIGHT; 
  lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS; 
  CString strDefaultFont = _T("Arial"); 
  lstrcpy(lf.lfFaceName, strDefaultFont); 
  VERIFY(m_FindTextFont.CreateFontIndirect(&lf));
}

CPWFindToolBar::~CPWFindToolBar()
{
  m_findedit.DestroyWindow();
  m_ImageList.DeleteImageList();
  delete [] m_pOriginalTBinfo;
  m_FindTextFont.DeleteObject();
}

BEGIN_MESSAGE_MAP(CPWFindToolBar, CToolBar)
END_MESSAGE_MAP()

// CPWFindToolBar message handlers

BOOL CPWFindToolBar::PreTranslateMessage(MSG *pMsg)
{
  if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
    CWnd *pWnd = FromHandle(pMsg->hwnd);
    int nID = pWnd->GetDlgCtrlID();
    if (nID == ID_TOOLBUTTON_FINDEDITCTRL) {
      m_pMessageWindow->SendMessage(m_iWMSGID);
      return TRUE;
    }
  }

  return CToolBar::PreTranslateMessage(pMsg);
}

//  Other routines

void
CPWFindToolBar::Init(const int NumBits, CWnd *pMessageWindow, int iWMSGID)
{
  int i, j;
  m_ClassicBackground = RGB(192, 192, 192);
  m_NewBackground = RGB(192, 192, 192);

  m_ClassicFlags = ILC_MASK | ILC_COLOR8;

  if (NumBits >= 32) {
    m_NewFlags = ILC_MASK | ILC_COLOR32;
    m_NewBackground = RGB(196, 196, 196);
    m_bitmode = 2;
  } else {
    m_NewFlags = ILC_MASK | ILC_COLOR8;
  }

  CBitmap bmTemp;
  // Classic images are first in the ImageList followed by the New8 and then New32 images.
  m_ImageList.Create(16, 16, m_ClassicFlags, m_iNum_Bitmaps * 3, 2);
  for (i = 0; i < m_iNum_Bitmaps; i++) {
    bmTemp.LoadBitmap(m_FindToolBarClassicBMs[i]);
    m_ImageList.Add(&bmTemp, m_ClassicBackground);
    bmTemp.Detach();
  }

  for (i = 0; i < m_iNum_Bitmaps; i++) {
    bmTemp.LoadBitmap(m_FindToolBarNew8BMs[i]);
    m_ImageList.Add(&bmTemp, m_NewBackground);
    bmTemp.Detach();
  }

  for (i = 0; i < m_iNum_Bitmaps; i++) {
    bmTemp.LoadBitmap(m_FindToolBarNew32BMs[i]);
    m_ImageList.Add(&bmTemp, m_NewBackground);
    bmTemp.Detach();
  }
 
  j = 0;
  for (i = 0; i < m_iMaxNumButtons; i++) {
    const bool bIsSeparator = m_FindToolBarIDs[i] == ID_SEPARATOR;
    BYTE fsStyle = bIsSeparator ? TBSTYLE_SEP : TBSTYLE_BUTTON;
    fsStyle &= ~BTNS_SHOWTEXT;
    if (!bIsSeparator) {
      fsStyle |= TBSTYLE_AUTOSIZE;
    }
    m_pOriginalTBinfo[i].iBitmap = bIsSeparator ? -1 : j;
    m_pOriginalTBinfo[i].idCommand = m_FindToolBarIDs[i];
    m_pOriginalTBinfo[i].fsState = TBSTATE_ENABLED;
    m_pOriginalTBinfo[i].fsStyle = fsStyle;
    m_pOriginalTBinfo[i].dwData = 0;
    m_pOriginalTBinfo[i].iString = bIsSeparator ? -1 : j;
    if (!bIsSeparator)
      j++;
  }

  m_pMessageWindow = pMessageWindow;
  m_iWMSGID = iWMSGID;
}

void
CPWFindToolBar::LoadDefaultToolBar(const int toolbarMode)
{
  m_toolbarMode = toolbarMode;

  int i, j;
  CToolBarCtrl& tbCtrl = GetToolBarCtrl();

  tbCtrl.SetImageList(&m_ImageList);
  m_ImageList.Detach();

  j = 0;
  const int iOffset = (m_toolbarMode == ID_MENUITEM_OLD_TOOLBAR) ? 0 : m_bitmode *m_iNum_Bitmaps;
  for (i = 0; i < m_iMaxNumButtons; i++) {
    if (m_FindToolBarIDs[i] != ID_SEPARATOR) {
      m_pOriginalTBinfo[i].iBitmap = j + iOffset;
      j++;
    }
  }

  tbCtrl.AddButtons(m_iMaxNumButtons, &m_pOriginalTBinfo[0]);

  AddExtraControls();

  tbCtrl.AutoSize();
}

void
CPWFindToolBar::AddExtraControls()
{
  CRect rect, rt;
  int index, iBtnHeight;

  GetItemRect(0, &rt);
  iBtnHeight = rt.Height();

  // Add find search text CEdit control (CEditExtn)
  // Get the index of the placeholder's position in the toolbar
  index = CommandToIndex(ID_TOOLBUTTON_FINDEDITCTRL);
  ASSERT(index != -1);

  // If we have been here before, destroy it first
  if (m_findedit.GetSafeHwnd() != NULL) {
    m_findedit.DestroyWindow();
  }

  // Convert that button to a separator
  SetButtonInfo(index, ID_TOOLBUTTON_FINDEDITCTRL, TBBS_SEPARATOR, EDITCTRL_WIDTH);

  // Note: "ES_WANTRETURN | ES_MULTILINE".  This is to allow the return key to be 
  // trapped by PreTranslateMessage and treated as if the Find button had been
  // pressed
  rect = CRect(0, 0, EDITCTRL_WIDTH, iBtnHeight);
  VERIFY(m_findedit.Create(WS_CHILD | WS_VISIBLE | 
                           ES_AUTOHSCROLL | ES_LEFT | ES_WANTRETURN | ES_MULTILINE,
                           CRect(rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2),
                           this, ID_TOOLBUTTON_FINDEDITCTRL));

  GetItemRect(index, &rect);
  rect.top += max((rect.top - rt.top) / 2, 0);
  m_findedit.SetWindowPos(NULL, rect.left, rect.top + 2, 0, 0,
                          SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOCOPYBITS );

  m_findedit.SetFont(&m_FindTextFont);

  // Add find search results CStatic control
  // Get the index of the placeholder's position in the toolbar
  index = CommandToIndex(ID_TOOLBUTTON_FINDRESULTS);
  ASSERT(index != -1);

  // If we have been here before, destroy it first
  if (m_findresults.GetSafeHwnd() != NULL) {
    m_findresults.DestroyWindow();
  }

  // Convert that button to a separator
  SetButtonInfo(index, ID_TOOLBUTTON_FINDRESULTS, TBBS_SEPARATOR, FINDRESULTS_WIDTH);

  rect = CRect(0, 0, FINDRESULTS_WIDTH, iBtnHeight);
  VERIFY(m_findresults.Create(_T(""), WS_CHILD | WS_VISIBLE | 
                           SS_LEFTNOWORDWRAP,
                           CRect(rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2),
                           this, ID_TOOLBUTTON_FINDEDITCTRL));

  GetItemRect(index, &rect);
  rect.top += max((rect.top - rt.top) / 2, 0);
  m_findresults.SetWindowPos(NULL, rect.left, rect.top + 2, 0, 0,
                          SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOCOPYBITS );

  m_findresults.SetFont(&m_FindTextFont); 

  ModifyStyle(0, WS_CLIPCHILDREN);
}

void
CPWFindToolBar::ShowFindToolBar(bool bShow)
{
  if (bShow == m_bVisible)
    return;

  ::ShowWindow(this->GetSafeHwnd(), bShow ? SW_SHOW : SW_HIDE);
  ::EnableWindow(this->GetSafeHwnd(), bShow ? TRUE : FALSE);
  m_bVisible = !m_bVisible;
}

void
CPWFindToolBar::Enable(bool bEnable)
{
  if (bEnable == m_bEnabled)
    return;

  ::EnableWindow(this->GetSafeHwnd(), bEnable ? TRUE : FALSE);
  m_bEnabled = !m_bEnabled;
}

void
CPWFindToolBar::ChangeImages(const int toolbarMode)
{
  m_toolbarMode = toolbarMode;

  int nCount, i, j;
  // Classic images are first in the ImageList followed by the New images.
  const int iOffset = (m_toolbarMode == ID_MENUITEM_OLD_TOOLBAR) ? 0 : m_bitmode * m_iNum_Bitmaps;

  j = 0;
  for (i = 0; i < m_iMaxNumButtons; i++) {
    if (m_FindToolBarIDs[i] != ID_SEPARATOR) {
      m_pOriginalTBinfo[i].iBitmap = j + iOffset;
      j++;
    }
  }

  TBBUTTONINFO tbinfo;
  memset(&tbinfo, 0x00, sizeof(tbinfo));
  tbinfo.cbSize = sizeof(tbinfo);
  tbinfo.dwMask = TBIF_BYINDEX | TBIF_IMAGE | TBIF_COMMAND | TBIF_STYLE;

  CToolBarCtrl& tbCtrl = GetToolBarCtrl();
  nCount = tbCtrl.GetButtonCount();
  for (i = 0; i < nCount; i++) {
    tbCtrl.GetButtonInfo(i, &tbinfo);
    if (tbinfo.fsStyle & TBSTYLE_SEP)
      continue;

    tbinfo.iImage %= m_iNum_Bitmaps;
    tbinfo.iImage += iOffset;
    tbCtrl.SetButtonInfo(i, &tbinfo);
  }
}

void
CPWFindToolBar::UpdateResults(const int num_found)
{
  CString cs_status;
  
  if (num_found < 0) {
    cs_status.LoadString(IDS_ENTERSEARCHSTRING);
    m_findresults.SetWindowText(cs_status);
    return;
  }

  switch (num_found) {
    case 0:
      cs_status.LoadString(IDS_NOMATCHFOUND);
      // Need m_findedit to lose focus
      SetFocus();
      m_findedit.SetColour(RGB(250, 215, 230));  // Set it to pink!
      break;
    case 1:
      cs_status.LoadString(IDS_FOUNDAMATCH);
      break;
    default:
      cs_status.Format(IDS_FOUNDMATCHES, num_found);
      break;
  }
  m_findresults.SetWindowText(cs_status);
  Invalidate();
}

void
CPWFindToolBar::ClearFind()
{
  m_findedit.SetWindowText(_T(""));
  m_findresults.SetWindowText(_T(""));
  // Need m_findedit to lose focus
  SetFocus();

  m_findedit.SetColour(RGB(255, 255, 255));  // Set it to white
}
