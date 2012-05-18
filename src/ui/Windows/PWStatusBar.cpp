/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// PWStatusBar.cpp : implementation file
//

#include "stdafx.h"

#include "WindowsDefs.h"

#include "PWStatusBar.h"

#include "os/debug.h"

#include "resource.h"
#include "resource3.h"

// CPWStatusBar

IMPLEMENT_DYNAMIC(CPWStatusBar, CStatusBar)

CPWStatusBar::CPWStatusBar()
  : m_bFilterStatus(false), m_pSBToolTips(NULL), m_bUseToolTips(false),
  m_bMouseInWindow(false)
{
  m_bmFilter.LoadBitmap(IDB_FILTER_ACTIVE);
  BITMAP bm;

  m_bmFilter.GetBitmap(&bm);
  m_bmWidth = bm.bmWidth;
  m_bmHeight = bm.bmHeight;
}

CPWStatusBar::~CPWStatusBar()
{
  m_bmFilter.DeleteObject();
}

BEGIN_MESSAGE_MAP(CPWStatusBar, CStatusBar)
  //{{AFX_MSG_MAP(CPWStatusBar)
  ON_WM_CREATE()
  ON_WM_MOUSEMOVE()
  ON_WM_TIMER()

  ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CPWStatusBar::OnCreate(LPCREATESTRUCT pCreateStruct)
{
	if (CStatusBar::OnCreate(pCreateStruct) == -1)
		return -1;

  m_pSBToolTips = new CInfoDisplay;

  if (!m_pSBToolTips->Create(0, 0, L"", this)) {
    // failed
    delete m_pSBToolTips;
    m_pSBToolTips = NULL;
  } else {
    m_pSBToolTips->ShowWindow(SW_HIDE);
    m_bUseToolTips = true;
  }

  // Set minimum height to take bitmaps (add 12.5% for space around it!)
  GetStatusBarCtrl().SetMinHeight(m_bmHeight + (m_bmHeight / 4));

  return 0;
}

void CPWStatusBar::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
  if (lpDrawItemStruct->itemID == SB_FILTER) {
    // Attach to a CDC object
    CDC dc;
    dc.Attach(lpDrawItemStruct->hDC);
    
    // Get the pane rectangle and calculate text coordinates
    CRect rect(&lpDrawItemStruct->rcItem);
    if (m_bFilterStatus) {
      // Centre bitmap in pane.
      int ileft = rect.left + rect.Width() / 2 - m_bmWidth / 2;
      int itop = rect.top + rect.Height() / 2 - m_bmHeight / 2;

      CBitmap *pBitmap = &m_bmFilter;
      CDC srcDC; // select current bitmap into a compatible CDC
      srcDC.CreateCompatibleDC(NULL);
      CBitmap* pOldBitmap = srcDC.SelectObject(pBitmap);
      dc.BitBlt(ileft, itop, m_bmWidth, m_bmHeight,
                &srcDC, 0, 0, SRCCOPY); // BitBlt to pane rect
      srcDC.SelectObject(pOldBitmap);
    } else {
      dc.FillSolidRect(&rect, ::GetSysColor(COLOR_BTNFACE));
    }
    // Detach from the CDC object, otherwise the hDC will be
    // destroyed when the CDC object goes out of scope
    dc.Detach();

    return;
  }

  CStatusBar::DrawItem(lpDrawItemStruct);
}

BOOL CPWStatusBar::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
  if (message== WM_NOTIFY) {
    NMMOUSE *pNMMouse = reinterpret_cast<NMMOUSE *>(lParam);
    if (pNMMouse->dwItemSpec >= 0 &&
        pNMMouse->dwItemSpec < (unsigned int)m_nCount) {
      UINT uCommandId;
      if (pNMMouse->hdr.code== NM_DBLCLK) {
        uCommandId = GetItemID(pNMMouse->dwItemSpec);
        // Only interested in double-click on R-O or R/W indicator or filter bitmap
        if (uCommandId == IDS_READ_ONLY || uCommandId == IDB_FILTER_ACTIVE)
          GetParent()->SendMessage(WM_COMMAND, uCommandId, 0);
      }
    }
  }

  return CStatusBar::OnChildNotify(message, wParam, lParam, pResult);
}

bool CPWStatusBar::ShowToolTip(int nPane, const bool bVisible)
{
  if (!m_bUseToolTips)
    return false;

  if (nPane < 0 || nPane >= SB_TOTAL || !bVisible) {
    m_pSBToolTips->ShowWindow(SW_HIDE);
    return false;
  }

  const UINT uiMsg[SB_TOTAL] =  {
    IDS_SB_TT_DBCLICK      /* SB_DBLCLICK        */,
    IDS_SB_TT_CBACTION     /* SB_CLIPBOARDACTION */,
#if defined(_DEBUG) || defined(DEBUG)
    IDS_SB_TT_CONFIG       /* SB_CONFIG          */,
#endif
    IDS_SB_TT_MODIFIED     /* SB_MODIFIED        */,
    IDS_SB_TT_MODE         /* SB_READONLY        */,
    IDS_SB_TT_NUMENTRIES   /* SB_NUM_ENT         */,
    IDS_SB_TT_FILTER       /* SB_FILTER          */};

  CString cs_ToolTip;
  cs_ToolTip.LoadString(uiMsg[nPane]);
  m_pSBToolTips->SetWindowText(cs_ToolTip);

  CPoint pt;
  ::GetCursorPos(&pt);

  pt.y += ::GetSystemMetrics(SM_CYCURSOR) / 2; // half-height of cursor

  m_pSBToolTips->SetWindowPos(&wndTopMost, pt.x, pt.y, 0, 0,
                              SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);

  return true;
}

void CPWStatusBar::OnTimer(UINT_PTR nIDEvent)
{
  switch (nIDEvent) {
    case TIMER_SB_HOVER:
      KillTimer(m_nHoverSBTimerID);
      m_nHoverSBTimerID = 0;
      if (ShowToolTip(m_HoverSBnPane, true)) {
        if (m_nShowSBTimerID) {
          KillTimer(m_nShowSBTimerID);
          m_nShowSBTimerID = 0;
        }
        m_nShowSBTimerID = SetTimer(TIMER_SB_SHOWING, TIMEINT_SB_SHOWING, NULL);
      }
      break;
    case TIMER_SB_SHOWING:
      KillTimer(m_nShowSBTimerID);
      m_nShowSBTimerID = 0;
      m_HoverSBPoint = CPoint(0, 0);
      m_HoverSBnPane = -1;
      ShowToolTip(m_HoverSBnPane, false);
      break;
    default:
      CStatusBar::OnTimer(nIDEvent);
      break;
  }
}

void CPWStatusBar::OnMouseMove(UINT nFlags, CPoint point)
{
  if (!m_bUseToolTips) {
    goto exit;
  } else {
    CRect rectClient;
    GetClientRect(&rectClient);
    int nPane = -1;

    if (rectClient.PtInRect(point)) {
      CPoint pointScreen;
      ::GetCursorPos(&pointScreen);
      RECT rc;
      for (int i = 0; i < SB_TOTAL; i++) {
        GetItemRect(i, &rc);
        // Fix width for the last Panel under Windows XP (not later versions)
        // http://stackoverflow.com/questions/628933/cstatusbarctrl-getitemrect-xp-manifest
        if (i == SB_TOTAL - 1) {
           UINT uiID, uiStyle;
           int cxWidth;
           GetPaneInfo(i, uiID, uiStyle, cxWidth);
           if (rc.right - rc.left < cxWidth)
             rc.right = rc.left + ::GetSystemMetrics(SM_CXVSCROLL) + ::GetSystemMetrics(SM_CXBORDER) * 2;
        }
        if (PtInRect(&rc, point)) {
          nPane = i;
          break;
        }
      }
    }

    if (m_nHoverSBTimerID) {
      if (nPane >= 0 && m_HoverSBnPane == nPane)
        return;

      KillTimer(m_nHoverSBTimerID);
      m_nHoverSBTimerID = 0;
    }

    if (m_nShowSBTimerID) {
      if (nPane >= 0 && m_HoverSBnPane == nPane)
        return;

      KillTimer(m_nShowSBTimerID);
      m_nShowSBTimerID = 0;
      ShowToolTip(m_HoverSBnPane, false);
    }

    if (!m_bMouseInWindow) {
      m_bMouseInWindow = true;
      TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
      VERIFY(TrackMouseEvent(&tme));
    }

    m_nHoverSBTimerID = SetTimer(TIMER_SB_HOVER, HOVER_TIME_SB, NULL);
    m_HoverSBPoint = point;
    m_HoverSBnPane = nPane;
  }

exit:
  CStatusBar::OnMouseMove(nFlags, point);
}

LRESULT CPWStatusBar::OnMouseLeave(WPARAM, LPARAM)
{
  if (m_bUseToolTips) {
    KillTimer(m_nHoverSBTimerID);
    KillTimer(m_nShowSBTimerID);
    m_nHoverSBTimerID = m_nShowSBTimerID = 0;
    m_HoverSBPoint = CPoint(0, 0);
    m_HoverSBnPane = -1;
    ShowToolTip(m_HoverSBnPane, false);
    m_bMouseInWindow = false;
  }

  return 0L;
}

void CPWStatusBar::CreateBitmapMask(CBitmap &bmImage, CBitmap *pbmMask, COLORREF crTransparent)
{
	CDC dcMem, dcMem2;

  // Create a mask image of same size as original
	pbmMask->CreateBitmap(m_bmWidth, m_bmHeight, 1, 1, NULL);

  // Create Device Contexts
	dcMem.CreateCompatibleDC(NULL);
	dcMem2.CreateCompatibleDC(NULL);

  // Select the images
	CBitmap *pOldImage = dcMem.SelectObject(&bmImage);
	CBitmap *pOldMask = dcMem2.SelectObject(pbmMask);

  // Set the background colour
	dcMem.SetBkColor(crTransparent);

  // Create mask by selecting the image and then inverting it
	dcMem2.BitBlt(0, 0, m_bmWidth, m_bmHeight, &dcMem, 0, 0, SRCCOPY);
	dcMem.BitBlt(0, 0, m_bmWidth, m_bmHeight, &dcMem2, 0, 0, SRCINVERT);

  // Put old bitmaps back into Device Contexts
	dcMem.SelectObject(pOldImage);
	dcMem2.SelectObject(pOldMask);

  // Delete Device Contexts
	dcMem.DeleteDC();
	dcMem2.DeleteDC();
}
