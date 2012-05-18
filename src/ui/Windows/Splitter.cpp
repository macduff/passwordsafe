/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/*
* Based on code submitted by Gerd Klevesaat to Codeguru on 11 May 1998
* See http://www.codeguru.com/article.php/c1979
*
* Hiding/Showing rows based on Kirk Stowell's extension of a similar
* CSplitterWnd class written by Oleg Galkin published on 7 August 1998
* at http://www.codeguru.com/Cpp/W-D/splitter/article.php/c1543
* Kirk's extension is within the legacy post in the comments made on
* 28 December 2000
*/

// Splitter.cpp : implementation file
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "Splitter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// HitTest return values (values and spacing between values is important)
// Values taken from WinSplit.cpp in MS MFS
enum HitTestValue
{
  noHit                   = 0,
  vSplitterBox            = 1,
  hSplitterBox            = 2,
  bothSplitterBox         = 3,        // just for keyboard
  vSplitterBar1           = 101,
  vSplitterBar15          = 115,
  hSplitterBar1           = 201,
  hSplitterBar15          = 215,
  splitterIntersection1   = 301,
  splitterIntersection225 = 525
};

/////////////////////////////////////////////////////////////////////////////
// CPWSplitterWnd 

CPWSplitterWnd ::CPWSplitterWnd ()
{
  m_DragImageList = new CImageList;
}

CPWSplitterWnd ::~CPWSplitterWnd ()
{
  if (m_DragImageList != NULL) {
    m_DragImageList->DeleteImageList();
    delete m_DragImageList;
    m_DragImageList = NULL;
  }
}

BEGIN_MESSAGE_MAP(CPWSplitterWnd , CSplitterWnd)
  //{{AFX_MSG_MAP(CPWSplitterWnd )
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CWnd *CPWSplitterWnd ::GetActivePane(int *pRow, int *pCol)
{
  ASSERT_VALID(this);

  CWnd *pView = GetFocus();

  // make sure the pane is a child pane of the splitter
  if (pView != NULL && !IsChildPane(pView, pRow, pCol))
    pView = NULL;

  return pView;
}

void CPWSplitterWnd ::SetActivePane(int row, int col, CWnd *pWnd)
{
  // set the focus to the pane
  CWnd *pPane = pWnd == NULL ? GetPane(row, col) : pWnd;
  pPane->SetFocus();
}

void CPWSplitterWnd ::StartTracking(int ht)
{
  ASSERT_VALID(this);
  if (ht == noHit)
    return;

  // GetHitRect will restrict 'm_rectLimit' as appropriate
  GetInsideRect(m_rectLimit);

  if (ht >= splitterIntersection1 && ht <= splitterIntersection225) {
    // split two directions (two tracking rectangles)
    int row = (ht - splitterIntersection1) / 15;
    int col = (ht - splitterIntersection1) % 15;

    GetHitRect(row + vSplitterBar1, m_rectTracker);
    int yTrackOffset = m_ptTrackOffset.y;
    m_bTracking2 = TRUE;
    GetHitRect(col + hSplitterBar1, m_rectTracker2);
    m_ptTrackOffset.y = yTrackOffset;
  }
  else if (ht == bothSplitterBox)  {
    // hit on splitter boxes (for keyboard)
    GetHitRect(vSplitterBox, m_rectTracker);
    int yTrackOffset = m_ptTrackOffset.y;
    m_bTracking2 = TRUE;
    GetHitRect(hSplitterBox, m_rectTracker2);
    m_ptTrackOffset.y = yTrackOffset;

    // center it
    m_rectTracker.OffsetRect(0, m_rectLimit.Height()/2);
    m_rectTracker2.OffsetRect(m_rectLimit.Width()/2, 0);
  }  else {
    // only hit one bar
    GetHitRect(ht, m_rectTracker);
  }

  // steal focus and capture
  SetCapture();
  SetFocus();

  // make sure no updates are pending
  RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW);

  // set tracking state and appropriate cursor
  m_bTracking = TRUE;
  OnInvertTracker(m_rectTracker);
  if (m_bTracking2)
    OnInvertTracker(m_rectTracker2);

  m_htTrack = ht;
  SetSplitCursor(ht);
}

void CPWSplitterWnd ::HideRow()
{
  // Hide row 1 panes
  int nActiveRow, nActiveCol;

  // If the row 1 has an active window -- change it
  if (GetActivePane(&nActiveRow, &nActiveCol) != NULL) {
    if (nActiveRow != 0) {
      nActiveRow = 0;
      SetActivePane(nActiveRow, nActiveCol);
    }
  }

  // Hide all row 1 column panes.
  for (int nCol = 0; nCol < m_nCols; nCol++) {
    CWnd *pPaneHide = GetPane(1, nCol);
    ASSERT(pPaneHide != NULL);
    pPaneHide->ShowWindow(SW_HIDE);
  }


  // Save row information
  m_saveRowInfo = m_pRowInfo[1];

  m_nRows--; // Remove rows 1 == now 1
  RecalcLayout();
}

void CPWSplitterWnd ::ShowRow()
{
  // Show row 1 & 2 panes
  int cyNew = m_pRowInfo[0].nCurSize / 2;
  m_nRows++;  // add nRows 1 == now 2
  
  ASSERT(m_nRows == m_nMaxRows);
  
  // Show the hidden row 1 panes
  // Can't use "GetPane(nRow, nCol) as Splitter currently thinks there is only
  // one row - hence the use of Control IDs set in DboxMain::OnInitDialog using
  // formula: Control ID = AFX_IDW_PANE_FIRST + nRow * 16 + nCol
  for (int nCol = 0; nCol < m_nCols; nCol++) {
    CWnd *pPaneShow = GetDlgItem(AFX_IDW_PANE_FIRST + 1 * 16 + nCol);
    ASSERT(pPaneShow != NULL);
    pPaneShow->ShowWindow(SW_SHOWNA);
  }

  // Restore row info
  m_pRowInfo[1] = m_saveRowInfo;
  m_pRowInfo[0].nIdealSize = cyNew;
  RecalcLayout();
}

/////////////////////////////////////////////////////////////////////////////
// CSplitterWnd command routing

BOOL CPWSplitterWnd ::OnCommand(WPARAM wParam, LPARAM lParam)
{
  if (CWnd::OnCommand(wParam, lParam))
    return TRUE;

  // route commands to the splitter to the parent frame window
  return GetParent()->SendMessage(WM_COMMAND, wParam, lParam);
}

BOOL CPWSplitterWnd ::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
  if (CWnd::OnNotify(wParam, lParam, pResult))
    return TRUE;

  // route commands to the splitter to the parent frame window
  *pResult = GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
  return TRUE;
}

BOOL CPWSplitterWnd ::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
  return CWnd::OnWndMsg(message, wParam, lParam, pResult);
}
