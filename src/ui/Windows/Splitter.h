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

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPWSplitterWnd window

class CPWSplitterWnd : public CSplitterWnd
{
public:
  CPWSplitterWnd ();
  virtual ~CPWSplitterWnd ();

  // Drag & Drop Members
  CImageList *m_DragImageList;
  BOOL m_bDragging;
  CWnd *m_pDragWnd;
  CWnd *m_pDropWnd;
  HTREEITEM m_TreeDragItem;
  HTREEITEM m_TreeDropItem;
  int m_ListDragIndex;
  int m_ListDropIndex;
  CPoint m_DropPoint;

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CPWSplitterWnd )
  //}}AFX_VIRTUAL

// Implementation
public:
  void StartTracking(int ht);
  CWnd *GetActivePane(int *pRow = NULL, int *pCol = NULL);
  void SetActivePane(int row, int col, CWnd *pWnd = NULL);
  void HideRow();
  void ShowRow();

protected:
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult);
  BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult);

  //{{AFX_MSG(CPWSplitterWnd )
    // NOTE - the ClassWizard will add and remove member functions here.
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

private:
  CRowColInfo m_saveRowInfo;
};
