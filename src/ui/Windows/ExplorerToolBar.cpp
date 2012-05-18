/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// ExplToolBar.cpp : implementation file
//

#include "stdafx.h"

#include "DboxMain.h"
#include "ExplorerToolBar.h"

#include "resource.h"
#include "resource2.h"
#include "resource3.h"


// CExplorerToolBar

// ***** NOTE: THIS TOOLBAR IS NOT CUSTOMIZABLE *****

#define STATIC_CURRENTPATH_WIDTH 250 // width of Static control for Path Text

const UINT CExplorerToolBar::m_ExplorerToolBarIDs0[] = {
  ID_TOOLBUTTON_BACKWARDS0,
  ID_TOOLBUTTON_FORWARDS0,
  ID_TOOLBUTTON_UPWARDS0,
  ID_SEPARATOR,
  ID_TOOLBUTTON_CURRENTPATHCOMBO,  // For ComboBox containing the Paths
};

const UINT CExplorerToolBar::m_ExplorerToolBarIDs1[] = {
  ID_TOOLBUTTON_BACKWARDS1,
  ID_TOOLBUTTON_FORWARDS1,
  ID_TOOLBUTTON_UPWARDS1,
  ID_SEPARATOR,
  ID_TOOLBUTTON_CURRENTPATHCOMBO,  // For ComboBox containing the Paths
};

const UINT CExplorerToolBar::m_ExplorerToolBarBMs[] = {
  IDB_EXPLORER_BACKWARDS,
  IDB_EXPLORER_FORWARDS,
  IDB_EXPLORER_UPWARDS,
  IDB_FINDCTRLPLACEHOLDER,  // For ComboBox containing the Paths
};

IMPLEMENT_DYNAMIC(CExplorerToolBar, CToolBar)

CExplorerToolBar::CExplorerToolBar()
  : m_bitmode(1)
{
  m_iMaxNumButtons = _countof(m_ExplorerToolBarIDs0);
  m_pOriginalTBinfo = new TBBUTTON[m_iMaxNumButtons];

  m_iNum_Bitmaps = _countof(m_ExplorerToolBarBMs);

  LOGFONT lf = {0};
 
  // Since design guide says toolbars are fixed height so is the font.
  lf.lfHeight = -11;
  lf.lfWeight = FW_LIGHT;
  lf.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
  CString strDefaultFont = L"MS Sans Serif";
#if (_MSC_VER >= 1400)
  wcscpy_s(lf.lfFaceName, LF_FACESIZE, strDefaultFont);
#else
  wcscpy(lf.lfFaceName, strDefaultFont);
#endif  
  VERIFY(m_PathFont.CreateFontIndirect(&lf));
}

CExplorerToolBar::~CExplorerToolBar()
{
  delete [] m_pOriginalTBinfo;

  m_cbxCurrentPath.DestroyWindow();
}

void CExplorerToolBar::OnDestroy()
{
  m_ImageList.DeleteImageList();

  m_PathFont.DeleteObject();
}

BEGIN_MESSAGE_MAP(CExplorerToolBar, CToolBar)
  ON_WM_DESTROY()
  ON_CBN_SELCHANGE(ID_TOOLBUTTON_CURRENTPATHCOMBO, OnPathComboChanged)
END_MESSAGE_MAP()

// CExplorerToolBar message handlers

void CExplorerToolBar::OnPathComboChanged()
{
  int index = m_cbxCurrentPath.GetCurSel();
  m_pDbx->SetExplorerView(index);
}

//  Other routines

void CExplorerToolBar::Init(const int NumBits, const SplitterRow &iRow, CWnd *pDbx)
{
  COLORREF crBackground = RGB(192, 192, 192);
  UINT iFlags = ILC_MASK | ILC_COLOR32;

  m_NumBits = NumBits;

  if (NumBits >= 32) {
    m_bitmode = 2;
  }

  CBitmap bmTemp;
  m_ImageList.Create(16, 16, iFlags, m_iNum_Bitmaps, 2);
  for (int i = 0; i < m_iNum_Bitmaps; i++) {
    bmTemp.LoadBitmap(m_ExplorerToolBarBMs[i]);
    m_ImageList.Add(&bmTemp, crBackground);
    bmTemp.DeleteObject();
  }

  int j = 0;
  for (int i = 0; i < m_iMaxNumButtons; i++) {
    const bool bIsSeparator = m_ExplorerToolBarIDs0[i] == ID_SEPARATOR;
    BYTE fsStyle = bIsSeparator ? TBSTYLE_SEP : TBSTYLE_BUTTON;
    fsStyle &= ~BTNS_SHOWTEXT;
    if (!bIsSeparator) {
      fsStyle |= TBSTYLE_AUTOSIZE;
    }
    m_pOriginalTBinfo[i].iBitmap = bIsSeparator ? -1 : j;
    m_pOriginalTBinfo[i].idCommand = iRow == TOP ? m_ExplorerToolBarIDs0[i] : m_ExplorerToolBarIDs1[i];
    m_pOriginalTBinfo[i].fsState = TBSTATE_ENABLED;
    m_pOriginalTBinfo[i].fsStyle = fsStyle;
    m_pOriginalTBinfo[i].dwData = 0;
    m_pOriginalTBinfo[i].iString = bIsSeparator ? -1 : j;
    if (!bIsSeparator)
      j++;
  }

  m_pDbx = static_cast<DboxMain *>(pDbx);
}

void CExplorerToolBar::LoadDefaultToolBar()
{
  CToolBarCtrl& tbCtrl = GetToolBarCtrl();
  tbCtrl.SetImageList(&m_ImageList);

  tbCtrl.AddButtons(m_iMaxNumButtons, &m_pOriginalTBinfo[0]);

  AddExtraControls();

  tbCtrl.AutoSize();
  tbCtrl.SetMaxTextRows(0);
}

void CExplorerToolBar::AddExtraControls()
{
  CRect rect, rt;
  int index, iBtnHeight;

  GetItemRect(0, &rt);
  iBtnHeight = rt.Height();

  // Add Path ComboBx
  // Get the index of the placeholder's position in the toolbar
  index = CommandToIndex(ID_TOOLBUTTON_CURRENTPATHCOMBO);
  ASSERT(index != -1);

  // If we have been here before, destroy it first
  if (m_cbxCurrentPath.GetSafeHwnd() != NULL) {
    m_cbxCurrentPath.DestroyWindow();
  }

  // Convert that button to a separator
  SetButtonInfo(index, ID_TOOLBUTTON_CURRENTPATHCOMBO, TBBS_SEPARATOR, STATIC_CURRENTPATH_WIDTH);

  // Set "button" width
  GetItemRect(index, &rect);
  rect.right = rect.left + STATIC_CURRENTPATH_WIDTH;
  VERIFY(m_cbxCurrentPath.Create(WS_CHILD | WS_VISIBLE | CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
                                 rect, this, ID_TOOLBUTTON_CURRENTPATHCOMBO));

  m_cbxCurrentPath.SetFont(&m_PathFont);
}

void CExplorerToolBar::UpdatePathComboBox(std::vector<StringX> &vPath, int index)
{
  // If index < 0, just clear the CComboBox
  const int num = m_cbxCurrentPath.GetCount();
  for (int i = 0; i < num; i++) {
    m_cbxCurrentPath.DeleteString(0);
  }

  if (index < 0) {
    m_cbxCurrentPath.EnableWindow(FALSE);
    return;
  }

  if (m_cbxCurrentPath.IsWindowEnabled() == FALSE)
    m_cbxCurrentPath.EnableWindow(TRUE);

  const StringX sxRoot = m_pDbx->GetRoot();

  for (size_t i = 0; i < vPath.size(); i++) {
    if (vPath[i].empty())
      m_cbxCurrentPath.AddString(sxRoot.c_str());
    else
      m_cbxCurrentPath.AddString(vPath[i].c_str());
  }

  m_cbxCurrentPath.SetCurSel(index);
}

void CExplorerToolBar::ChangeIndex(const int iValue, const bool bDelete)
{
  int index = m_cbxCurrentPath.GetCurSel();
  if (bDelete)
    m_cbxCurrentPath.DeleteString(index);

  m_cbxCurrentPath.SetCurSel(index + iValue);
}
