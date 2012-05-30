/*
 *Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
 *All rights reserved. Use of the code is allowed under the
 *Artistic License 2.0 terms, as specified in the LICENSE file
 *distributed with this code, or available from
 *http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// TreeView.h : interface of the CPWTreeView class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "afxcview.h"  // For CTreeView and CListView

#include "WindowsDefs.h"
#include "SecString.h"

#include "core/Command.h"

#include "os/UUID.h"

#include <map>

class CPWSplitterWnd ;

class CPWTreeView : public CTreeView
{
friend class CPWListView;
friend class DboxMain;

protected: // create from serialization only
  CPWTreeView();
  ~CPWTreeView();

  DECLARE_DYNCREATE(CPWTreeView)

  //{{AFX_VIRTUAL(CPWTreeView)
  virtual BOOL PreCreateWindow(CREATESTRUCT &cs);
  virtual BOOL PreTranslateMessage(MSG *pMsg);
  BOOL OnPreparePrinting(CPrintInfo *) {return FALSE;} // No printing allowed
  //}}AFX_VIRTUAL

  //{{AFX_MSG(CPWTreeView)
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnDestroy();
  afx_msg void OnItemClick(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnItemDoubleClick(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnContextMenu(CWnd *pWnd, CPoint screen);
  afx_msg void OnBeginLabelEdit(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnEndLabelEdit(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnRename();
  afx_msg void OnSetFocus(CWnd *pOldWnd);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
  DboxMain *m_pDbx;

  // Me
  CTreeCtrl *m_pTreeCtrl;
  // And my partner on my row
  CPWListView *m_pListView;
  // My parent
  CPWSplitterWnd *m_pParent;

  // If I am one row, this is the other set by DboxMain
  CPWTreeView *m_pOtherTreeView;
  CTreeCtrl *m_pOtherTreeCtrl;

  void SetOtherView(const SplitterRow &ithis_row, CPWTreeView *pOtherTreeView);

  void SetListView(CPWListView *pListView)
  {m_pListView = pListView;}
  void SetMainDlg(DboxMain *pDbx)
  {m_pDbx = pDbx;}

  StringX GetCurrentPath() {return m_sxCurrentPath;}
  void SetCurrentPath(const StringX sxCurrentPath) {m_sxCurrentPath = sxCurrentPath;}

  void DeleteEntries();
  void PopulateEntries();

  void Initialize();
  void OnListViewFolderSelected(StringX sxPath, UINT index,
                                const bool bUpdateHistory = false);
  void ClearGroups();
  void CreateTree(StringX &sxCurrentPath, const bool &bUpdateHistory);
  void PlaceInTree(const StringX &sxPRGPath, const size_t &len,
                   const bool &bIsEmpty = false);
  HTREEITEM DoesItExist(const StringX &sxGroup, HTREEITEM From);
  void GetGroupMap(StringX sxRealPath, PathMap &mapGroups2Item);
  bool FindGroup(HTREEITEM hItem, StringX &sxPath);
  void DisplayFolder(HTREEITEM &hSelected);

  PathMap m_mapGroup2Item;
  bool m_bInitDone;
  SplitterRow m_this_row;

  StringX m_sxCurrentPath;
  bool m_bAccEn, m_bInRename;
  StringX m_sxOldText; // label at start of edit, if we need to revert
  bool m_bEditLabelCompleted;
};
