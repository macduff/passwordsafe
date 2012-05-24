/*
 *Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
 *All rights reserved. Use of the code is allowed under the
 *Artistic License 2.0 terms, as specified in the LICENSE file
 *distributed with this code, or available from
 *http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// ListView.h : interface of the CPWListView class
//

#pragma once

#include "afxcview.h"  // For CTreeView and CListView

#ifdef EXPLORER_DRAG_DROP
#include "afxole.h"    // Drag & Drop
#endif

#include "WindowsDefs.h"

#include "SecString.h"

#include "core/ItemData.h"
#include "core/Command.h"

#include "os/UUID.h"

#ifdef EXPLORER_DRAG_DROP
class CDDObList;
class CLVDropTarget;
class CLVDropSource;
class CLVDataSource;
#endif

class CPWListView;
class DboxMain;

/*
 * The standard CListCtrl only allows in-place editing of the first column.
 * In order to edit any subitem, we need to add a CEdit control over this field
 * and use it to edit.
 * Since the first column is the icon, we need this approach.
*/

class CLVEdit: public CEdit
{
public:
  CLVEdit(CWnd *pParent, int nItem, int nSubItem, CSecString csLabel);

  virtual ~CLVEdit() {}

  int m_x, m_y;
  int m_nItem, m_nSubItem;

protected:
  CPWListView *m_pPWListView;
  DboxMain *m_pDbx;

  bool m_bVK_ESCAPE;
  CSecString m_csLabel;

  virtual BOOL PreTranslateMessage(MSG *pMsg);

  //{{AFX_MSG(CLVEdit)
  afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnNcDestroy();
  afx_msg void OnKillFocus(CWnd *pNewWnd);
  afx_msg void OnWindowPosChanged(WINDOWPOS *lpwndpos);
  afx_msg void OnSetFocus(CWnd *pOldWnd);

  DECLARE_MESSAGE_MAP()
};


class CPWSplitterWnd ;

class CPWListView : public CListView
{
friend class CPWTreeView;
friend class DboxMain;

protected: // create from serialization only
  CPWListView();
  ~CPWListView();

  DECLARE_DYNCREATE(CPWListView)

  //{{AFX_VIRTUAL(CPWListView)
  virtual BOOL PreCreateWindow(CREATESTRUCT &cs);
  virtual BOOL PreTranslateMessage(MSG *pMsg);
  BOOL OnPreparePrinting(CPrintInfo *) {return FALSE;} // No printing allowed
  //}}AFX_VIRTUAL

protected:
  // Generated message map functions
  //{{AFX_MSG(CPWListView)
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnDestroy();
  afx_msg void OnHeaderClicked(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnItemClick(NMHDR *pNotifyStruct, LRESULT *pResult);
  afx_msg void OnItemDoubleClick(NMHDR *pNotifyStruct, LRESULT *pResult);
  afx_msg void OnContextMenu(CWnd *pWnd, CPoint screen);
  afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
  afx_msg void OnEndLabelEdit(NMHDR *pNotifyStruct, LRESULT *pResult);
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnRename();
  afx_msg void OnSetFocus(CWnd *pOldWnd);

#ifdef EXPLORER_DRAG_DROP
  // Drag & Drop
  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
  afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnBeginDrag(NMHDR *pNotifyStruct, LRESULT *pLResult);
#endif
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
  DboxMain *m_pDbx;

  // Me
  CListCtrl *m_pListCtrl;
  // And my partner on my row
  CPWTreeView *m_pTreeView;
  // My parent
  CPWSplitterWnd *m_pParent;

  // If I am in one row, this is the other row set by DboxMain
  CPWListView *m_pOtherListView;
  CListCtrl *m_pOtherListCtrl;

  void SetOtherView(const SplitterRow &ithis_row, CPWListView *pOtherListView);

  void SetTreeView(CPWTreeView *pTreeView)
  {m_pTreeView = pTreeView;}
  void SetMainDlg(DboxMain *pDbx)
  {m_pDbx = pDbx;}

  void SetUpHeader();
  StringX GetCurrentPath() {return m_sxCurrentPath;}
  void SetCurrentPath(const StringX sxCurrentPath) {m_sxCurrentPath = sxCurrentPath;}

  void DeleteEntries();
  void PopulateEntries();

  void Initialize();
  void DisplayEntries(const StringX strRealPath, const bool bUpdateHistory = false);
  void ClearEntries();
  bool IsLeaf(int iIndex) const;
  void DisplayFoundEntries(const std::vector<pws_os::CUUID> &vFoundEntries);
  BOOL SelectItem(const pws_os::CUUID &uuid, const BOOL &MakeVisible);
  bool IsDisplayingFoundEntries() {return m_bDisplayingFoundEntries;}
  void SetEntryText(const CItemData *pci, const StringX &sxnewText);

  // If return is NULL, use full path to get the information (item is a group)
  // Otherwise use retuen value (item is an entry)
  CItemData *GetFullPath(const int iItem, StringX &sx_FullPath);

  static int CALLBACK CompareFunction(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareFoundEntriesFunction(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

  bool m_bAccEn, m_bInRename;
  bool m_bSortAscending;
  int m_iSortedColumn;
  bool m_bInitDone;
  bool m_bDisplayingFoundEntries;
  SplitterRow m_this_row;

  StringX m_sxOldText; // label at start of edit, if we need to revert
  void UpdateEntry(StringX &sxOldGTU, StringX &sxNewGTU);

  // For in place editing
  int m_edit_item;
  bool m_bEditLabelCompleted;

  //It contains the current Item of which subItem is being edited.
  int m_eItem, m_eSubItem;

  // Our current path
  StringX m_sxCurrentPath;

#ifdef EXPLORER_DRAG_DROP
  // Drag-n-Drop interface - called indirectly via src/tgt member functions
  // Source methods
  SCODE GiveFeedback(DROPEFFECT dropEffect);
  // target methods
  BOOL OnDrop(CWnd *pWnd, COleDataObject *pDataObject,
              DROPEFFECT dropEffect, CPoint point);
  DROPEFFECT OnDragEnter(CWnd *pWnd, COleDataObject *pDataObject,
                         DWORD dwKeyState, CPoint point);
  DROPEFFECT OnDragOver(CWnd *pWnd, COleDataObject *pDataObject,
                        DWORD dwKeyState, CPoint point);
  void OnDragLeave();
  bool IsDropOnMe() {return m_bWithinThisInstance;}
  int GetDDType() {return m_DDType;}
  void EndDrop() {m_bDropped = true;}

  bool CollectData(BYTE * &out_buffer, long &outLen);
  bool ProcessData(BYTE *in_buffer, const long &inLen, const CSecString &DropGroup,
                   const bool bCopy = true);
  BOOL OnRenderGlobalData(LPFORMATETC lpFormatEtc, HGLOBAL *phGlobal);
  BOOL RenderAllData(HGLOBAL *phGlobal);
  CImageList *CreateDragImageEx(LPPOINT lpPoint);
  BOOL SelectDropTarget(int iItem);

  int m_iItemDrag;
  int m_iItemDrop;

  // Hovering related items
  int m_nDragPathLen; // Empty if dragging a leaf, path if dragging a group
  bool m_bWithinThisInstance;
  // For dealing with distinguishing between left & right-mouse drag
  int m_DDType;

  CLVDropTarget *m_DropTarget;
  CLVDropSource *m_DropSource;
  CLVDataSource *m_DataSource;

  friend class CTVDropTarget;
  friend class CTVDropSource;
  friend class CTVDataSource;
  friend class CLVDropTarget;
  friend class CLVDropSource;
  friend class CLVDataSource;

  // Clipboard format for our Drag & Drop
  CLIPFORMAT m_tcddCPFID;
  HGLOBAL m_hgDataALL, m_hgDataUTXT, m_hgDataTXT;
  CLIPFORMAT m_cfdropped;
  bool m_bDropped, m_bDragging;
  std::vector<pws_os::CUUID> m_vDragItems;
  CImageList* m_pDragImage;

  bool m_bMouseInWindow;
#endif
};
