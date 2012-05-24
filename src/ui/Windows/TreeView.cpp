/*
 *Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
 *All rights reserved. Use of the code is allowed under the
 *Artistic License 2.0 terms, as specified in the LICENSE file
 *distributed with this code, or available from
 *http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// TreeView.cpp : implementation of the CPWTreeView class
//

#include "stdafx.h"

#include "TreeView.h"
#include "ListView.h"
#include "DboxMain.h"
#include "DDSupport.h"
#include "TreeUtils.h"
#include "Images.h"

#include "resource.h"

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace TreeUtils;
using namespace std;

extern wchar_t GROUP_SEP;
extern StringX sxDot;

#ifdef EXPLORER_DRAG_DROP
// Following header for D&D data passed over OLE:
// Process ID of sender (to determine if src == tgt)
// Type of data
// Length of actual payload, in bytes.
static const char *OLE_HDR_FMT = "%08x%02x%08x";
static const int OLE_HDR_LEN = 18;

class CTVDropTarget : public COleDropTarget
{
public:
  CTVDropTarget(CPWTreeView *parent)
    : m_TreeView(*parent) {}

  DROPEFFECT OnDragEnter(CWnd *pWnd, COleDataObject *pDataObject,
                         DWORD dwKeyState, CPoint point)
  {return m_TreeView.OnDragEnter(pWnd, pDataObject, dwKeyState, point);}

  DROPEFFECT OnDragOver(CWnd * pWnd, COleDataObject *pDataObject,
                        DWORD dwKeyState, CPoint point)
  {return m_TreeView.OnDragOver(pWnd, pDataObject, dwKeyState, point);}

  void OnDragLeave(CWnd * /*pWnd*/)
  {m_TreeView.OnDragLeave();}

  BOOL OnDrop(CWnd *pWnd, COleDataObject *pDataObject,
              DROPEFFECT dropEffect, CPoint point)
  {return m_TreeView.OnDrop(pWnd, pDataObject, dropEffect, point);}

private:
  CPWTreeView &m_TreeView;
};

class CTVDropSource : public COleDropSource
{
public:
  CTVDropSource(CPWTreeView *parent)
    : m_TreeView(*parent) {}

  virtual SCODE QueryContinueDrag(BOOL bEscapePressed, DWORD dwKeyState)
  {
    // To prevent processing in multiple calls to CStaticDataSource::OnRenderGlobalData
    //  Only process the request if data has been dropped.
    SCODE sCode = COleDropSource::QueryContinueDrag(bEscapePressed, dwKeyState);
    if (sCode == DRAGDROP_S_DROP) {
      pws_os::Trace(L"CTVDropSource::QueryContinueDrag - dropped\n");
      m_TreeView.EndDrop();
    }
    return sCode;
  }

  virtual SCODE GiveFeedback(DROPEFFECT dropEffect)
  {return m_TreeView.GiveFeedback(dropEffect);}

private:
  CPWTreeView &m_TreeView;
};

class CTVDataSource : public COleDataSource
{
public:
  CTVDataSource(CPWTreeView *parent, COleDropSource *ds)
    : m_TreeView(*parent), m_pDropSource(ds) {}

  DROPEFFECT StartDragging(CLIPFORMAT cpfmt, LPCRECT rClient)
  {
    DelayRenderData(cpfmt);
    DelayRenderData(CF_UNICODETEXT);
    DelayRenderData(CF_TEXT);

    m_TreeView.m_cfdropped = 0;
    //pws_os::Trace(L"CTVDataSource::StartDragging - calling DoDragDrop\n");
    DROPEFFECT de = DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE,
                               rClient, m_pDropSource);
    // Cleanup:
    // Standard processing is for the recipient to do this!!!
    if (de == DROPEFFECT_NONE) {
      if (m_TreeView.m_hgDataALL != NULL) {
        //pws_os::Trace(L"CTVDataSource::StartDragging - Unlock/Free m_hgDataALL\n");
        LPVOID lpData = GlobalLock(m_TreeView.m_hgDataALL);
        SIZE_T memsize = GlobalSize(m_TreeView.m_hgDataALL);
        if (lpData != NULL && memsize > 0) {
          trashMemory(lpData, memsize);
        }
        GlobalUnlock(m_TreeView.m_hgDataALL);
        GlobalFree(m_TreeView.m_hgDataALL);
        m_TreeView.m_hgDataALL = NULL;
      }
      if (m_TreeView.m_hgDataTXT != NULL) {
        //pws_os::Trace(L"CTVDataSource::StartDragging - Unlock/Free m_hgDataTXT\n");
        LPVOID lpData = GlobalLock(m_TreeView.m_hgDataTXT);
        SIZE_T memsize = GlobalSize(m_TreeView.m_hgDataTXT);
        if (lpData != NULL && memsize > 0) {
          trashMemory(lpData, memsize);
        }
        GlobalUnlock(m_TreeView.m_hgDataTXT);
        GlobalFree(m_TreeView.m_hgDataTXT);
        m_TreeView.m_hgDataTXT = NULL;
      }
      if (m_TreeView.m_hgDataUTXT != NULL) {
        //pws_os::Trace(L"CTVDataSource::StartDragging - Unlock/Free m_hgDataUTXT\n");
        LPVOID lpData = GlobalLock(m_TreeView.m_hgDataUTXT);
        SIZE_T memsize = GlobalSize(m_TreeView.m_hgDataUTXT);
        if (lpData != NULL && memsize > 0) {
          trashMemory(lpData, memsize);
        }
        GlobalUnlock(m_TreeView.m_hgDataUTXT);
        GlobalFree(m_TreeView.m_hgDataUTXT);
        m_TreeView.m_hgDataUTXT = NULL;
      }
    }
    return de;
  }

  BOOL OnRenderGlobalData(LPFORMATETC lpFormatEtc, HGLOBAL *phGlobal)
  {return m_TreeView.OnRenderGlobalData(lpFormatEtc, phGlobal);}

private:
  CPWTreeView &m_TreeView;
  COleDropSource *m_pDropSource;
};
#endif

/////////////////////////////////////////////////////////////////////////////
// CPWTreeView

IMPLEMENT_DYNCREATE(CPWTreeView, CTreeView)

BEGIN_MESSAGE_MAP(CPWTreeView, CTreeView)
  //{{AFX_MSG_MAP(CPWTreeView)
  ON_WM_CONTEXTMENU()
  ON_WM_CREATE()
  ON_WM_DESTROY()
  ON_WM_SETFOCUS()

  ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
  ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
  ON_COMMAND(ID_MENUITEM_RENAME, OnRename)
  ON_NOTIFY_REFLECT(NM_CLICK, OnItemClick)
  ON_NOTIFY_REFLECT(NM_DBLCLK, OnItemDoubleClick)

#ifdef EXPLORER_DRAG_DROP
  // Drag & Drop
  ON_WM_MOUSEMOVE()
  ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
  ON_NOTIFY_REFLECT(TVN_BEGINRDRAG, OnBeginDrag)
  ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
#endif
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPWTreeView construction/destruction

CPWTreeView::CPWTreeView()
 : m_pParent(NULL), m_pTreeCtrl(NULL), m_pListView(NULL),
 m_pOtherTreeView(NULL), m_pOtherTreeCtrl(NULL), m_pDbx(NULL),
 m_bInitDone(false), m_bAccEn(false), m_bInRename(false),
#ifdef EXPLORER_DRAG_DROP
 m_hgDataALL(NULL), m_hgDataTXT(NULL), m_hgDataUTXT(NULL),
 m_pDragImage(NULL), m_bDragging(false),
#endif
 m_this_row(INVALID_ROW)
{
#ifdef EXPLORER_DRAG_DROP
  // Register a clipboard format for drag & drop.
  // Note that it's OK to register same format more than once:
  // "If a registered format with the specified name already exists,
  // a new format is not registered and the return value identifies the existing format."

  CString cs_CPF(MAKEINTRESOURCE(IDS_CPF_TVDD));
  m_tcddCPFID = (CLIPFORMAT)RegisterClipboardFormat(cs_CPF);
  ASSERT(m_tcddCPFID != 0);

  // Instantiate "proxy" objects for D&D.
  // The members are currently pointers mainly to hide
  // their implementation from the header file. If this changes,
  // e.g., if we make them nested classes, then they should
  // be non-pointers.
  m_DropTarget = new CTVDropTarget(this);
  m_DropSource = new CTVDropSource(this);
  m_DataSource = new CTVDataSource(this, m_DropSource);
#endif
}

CPWTreeView::~CPWTreeView()
{
#ifdef EXPLORER_DRAG_DROP
  // see comment in constructor re these member variables
  delete m_pDragImage;
  delete m_DropTarget;
  delete m_DropSource;
  delete m_DataSource;
#endif
}

BOOL CPWTreeView::PreCreateWindow(CREATESTRUCT &cs)
{
  cs.style |= (TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_EDITLABELS);
  return CTreeView::PreCreateWindow(cs);
}

BOOL CPWTreeView::PreTranslateMessage(MSG *pMsg)
{
  // When an item is being edited make sure the edit control
  // receives certain important key strokes
  if (m_pTreeCtrl != NULL && m_pTreeCtrl->GetEditControl()) {
    ::TranslateMessage(pMsg);
    ::DispatchMessage(pMsg);
    return TRUE; // DO NOT process further
  }

  if (!m_bInRename) {
    // Process User's Delete shortcut
    if (m_pDbx != NULL && m_pDbx->CheckPreTranslateDelete(pMsg))
      return TRUE;

    // Process user's Rename shortcut
    if (m_pDbx != NULL && m_pDbx->CheckPreTranslateRename(pMsg)) {
      //  Send via main window to ensure it isn't an Edit in place
      PostMessage(WM_COMMAND, ID_MENUITEM_RENAME);
      return TRUE;
    }
  }

  // Pressing enter on folder in left pane, just displays contents in right pane
  if (m_pTreeCtrl != NULL && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
    HTREEITEM hSelected = m_pTreeCtrl->GetSelectedItem();
    if (hSelected != NULL) {
      DisplayFolder(hSelected);
      return TRUE;
    }
  }

  // Let the parent class do its thing
  return CTreeView::PreTranslateMessage(pMsg);
}

void CPWTreeView::Initialize()
{
  m_pTreeCtrl = &GetTreeCtrl();

  m_pTreeCtrl->ModifyStyle(0, TVS_SHOWSELALWAYS);

  Images *pImages = Images::GetInstance();

  CImageList *pil = pImages->GetImageList();
  ASSERT(pil);

  m_pTreeCtrl->SetImageList(pImages->GetImageList(), TVSIL_NORMAL);
  m_pTreeCtrl->SetImageList(pImages->GetImageList(), TVSIL_STATE);

#ifdef EXPLORER_DRAG_DROP
  m_DropTarget->Register(this);
#endif
  
  m_bInitDone = true;
}

void CPWTreeView::SetOtherView(const SplitterRow &i_this_row, CPWTreeView *pOtherTreeView)
{
  // Set what row we are and info on the other row
  m_this_row = i_this_row;

 m_pOtherTreeView = pOtherTreeView;
 m_pOtherTreeCtrl = &pOtherTreeView->GetTreeCtrl();
}

// CPWTreeView message handlers

int CPWTreeView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CTreeView::OnCreate(lpCreateStruct) == -1)
    return -1;

  if (!m_bInitDone)
    Initialize();

  m_pParent = (CPWSplitterWnd *)GetParent();
  return 0;
}

void CPWTreeView::OnDestroy()
{
  // Remove image list
  m_pTreeCtrl->SetImageList(NULL, TVSIL_NORMAL);
  m_pTreeCtrl->SetImageList(NULL, TVSIL_STATE);

#ifdef EXPLORER_DRAG_DROP  
  m_DropTarget->Revoke();
#endif
}

void CPWTreeView::OnContextMenu(CWnd *pWnd, CPoint screen)
{
  m_pDbx->PostMessage(WM_CONTEXTMENU, (WPARAM)pWnd->m_hWnd,
                      MAKELPARAM(screen.x, screen.y));
}

void CPWTreeView::OnRename()
{
  if (m_pDbx->IsDBReadOnly()) // disable in read-only mode
    return;

  HTREEITEM hItem = m_pTreeCtrl->GetSelectedItem();
  if (hItem != NULL) {
    m_bInRename = true;
    m_pTreeCtrl->EditLabel(hItem);
    m_bInRename = false;
  }
}

void CPWTreeView::OnSetFocus(CWnd *pOldWnd)
{
  CTreeView::OnSetFocus(pOldWnd);
  if (m_pDbx == NULL)
    return;

  m_pDbx->SetCurrentRow(m_this_row);
  m_pDbx->UpdateCurrentPath(m_sxCurrentPath);
}

void CPWTreeView::GetGroupMap(StringX sxRealPath,
                              PathMap &mapGroup2Item)
{
  pws_os::Trace(L"CPWTreeView::GetGroupMap: sxRealPath=%s\n", sxRealPath.c_str());
  PathMapConstIter lb_iter, ub_iter, iter;
  mapGroup2Item.clear();

  StringX sxRoot = m_pDbx->GetRoot();
  StringX sxPath = sxRoot;

  if (!sxRealPath.empty())
    sxPath += sxDot + sxRealPath;
  lb_iter = m_mapGroup2Item.lower_bound(sxPath);

  // Should not be empty!
  if (lb_iter == m_mapGroup2Item.end())
    return;

  // Make unlikely upper bound by adding large hex value to end of path
  ub_iter = m_mapGroup2Item.upper_bound(sxPath + StringX(L"/xffff"));

  // Copy over the group and all its subgroups
  for (iter = lb_iter; iter != ub_iter; iter++) {
     mapGroup2Item[iter->first] = iter->second;
  }
}

void CPWTreeView::ClearGroups()
{
  if (!m_bInitDone)
    Initialize();

  m_pTreeCtrl->SetRedraw(FALSE);
  m_pTreeCtrl->DeleteAllItems();
  m_pTreeCtrl->SetRedraw(TRUE);

  UpdateWindow();
}

void CPWTreeView::CreateTree(StringX &sxCurrentPath, const bool &bUpdateHistory)
{
  pws_os::Trace(L"CPWTreeView::CreateTree: sxCurrentPath=%s\n", sxCurrentPath.c_str());
  // If there is anything in the tree, remove it
  m_pTreeCtrl->SetRedraw(FALSE);
  m_pTreeCtrl->DeleteAllItems();
  m_mapGroup2Item.clear();

  m_sxCurrentPath = sxCurrentPath;

  // Get non-empty groups (sorted)
  vector<StringX> vGroups;
  m_pDbx->GetAllGroups(vGroups);
  sort(vGroups.begin(), vGroups.end());

  // Get empty grops (unsorted)
  PathSet setEmptyGroups;
  setEmptyGroups = m_pDbx->GetEmptyGroups();

  StringX sxRoot;
  sxRoot = m_pDbx->GetRoot();
  const size_t len = sxRoot.length();

  for (size_t i = 0; i < vGroups.size(); i++) {
    StringX sxPRGPath = sxRoot + sxDot + vGroups[i];
    PlaceInTree(sxPRGPath, len);
  }

  PathSetConstIter citer;
  for (citer = setEmptyGroups.begin(); citer != setEmptyGroups.end(); citer++) {
    StringX sxPRGPath = sxRoot + sxDot + *citer;
    PlaceInTree(sxPRGPath, len, true);
  }

  // Now change root item
  HTREEITEM hRoot = m_pTreeCtrl->GetRootItem();
  m_pTreeCtrl->SetItemImage(hRoot, CItemData::EI_DATABASE, CItemData::EI_DATABASE);

  // Expand first level - i.e. top level groups
  m_pTreeCtrl->Expand(hRoot, TVE_EXPAND);
  m_pTreeCtrl->SetRedraw(TRUE);
  UpdateWindow();

  HTREEITEM hSelected = hRoot;
  if (!m_sxCurrentPath.empty()) {
    //  Find out which group was selected and select it
    PathMapIter iter;
    iter = m_mapGroup2Item.find(m_sxCurrentPath);
    if (iter != m_mapGroup2Item.end())
      hSelected = iter->second;
  }
  m_pTreeCtrl->SetItemState(hSelected, TVIS_SELECTED, TVIS_SELECTED);

  m_pListView->DisplayEntries(m_sxCurrentPath, bUpdateHistory);

  m_pDbx->UpdateToolBarForSelectedItem(NULL);
  m_pDbx->SetDCAText(NULL);
}

void CPWTreeView::PlaceInTree(const StringX &sxPRGPath, const size_t &len,
                              const bool &bIsEmpty)
{
  //pws_os::Trace(L"CPWTreeView::PlaceInTree: sxPRGPath=%s\n", sxPRGPath.c_str());

  HTREEITEM hParent = NULL;
  StringX::size_type iDot(0);
  StringX sxCurrentGroup, sxSubGroups(sxPRGPath);
  StringX sxCurrentPath(L"");
  bool bFirst(true);

  // len is used to ignore the name of the database that is in the first element
  while (iDot != StringX::npos) {
    iDot = sxSubGroups.find_first_of(L'.', bFirst ? len : 1);
    if (iDot != StringX::npos) {
      sxCurrentGroup = sxSubGroups.substr(0, iDot);
      sxSubGroups = sxSubGroups.substr(iDot + 1);
    } else {
      sxCurrentGroup = sxSubGroups;
    }

    if (sxCurrentPath.empty())
      sxCurrentPath = sxCurrentGroup;
    else
      sxCurrentPath += sxDot + sxCurrentGroup;

    HTREEITEM hItem = DoesItExist(sxCurrentGroup, hParent);
    if (hItem == NULL) {
      // Add subgroup into group
      hParent = m_pTreeCtrl->InsertItem(sxCurrentGroup.c_str(), hParent);
      m_pTreeCtrl->SetItemImage(hParent,
                           bIsEmpty ? CItemData::EI_EMPTY_GROUP : CItemData::EI_GROUP,
                           bIsEmpty ? CItemData::EI_EMPTY_GROUP : CItemData::EI_GROUP);
      // Add to map
      m_mapGroup2Item[sxCurrentPath] = hParent;
    } else {
      hParent = hItem;
    }

    if (bFirst)
      bFirst = false;
  }
}

HTREEITEM CPWTreeView::DoesItExist(const StringX &sxGroup, HTREEITEM From)
{
  //pws_os::Trace(L"CPWTreeView::DoesItExist: sxGroup=%s\n", sxGroup.c_str());

  if (m_pTreeCtrl->ItemHasChildren(From)) {
    HTREEITEM hNextItem;
    HTREEITEM hChildItem = m_pTreeCtrl->GetChildItem(From);

    while (hChildItem != NULL) {
      hNextItem = m_pTreeCtrl->GetNextItem(hChildItem, TVGN_NEXT);
      StringX sxText = (LPCWSTR)m_pTreeCtrl->GetItemText(hChildItem);
      if (CompareNoCase(sxText, sxGroup) == 0) {
        return hChildItem;
      }
      hChildItem = hNextItem;
    }
  }
  return NULL;
}

void CPWTreeView::OnItemClick(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
  /*
    Like Explorer - single click on a folder in left pane, displays contents in right pane
  */

 *pResult = 0;

  pws_os::Trace(L"CPWTreeView::OnItemClick %s pane\n",
        m_this_row == TOP ? L"Top" : L"Bottom");

  // Find out what item is selected in the tree
  CPoint mp = ::GetMessagePos();
  ScreenToClient(&mp);
  UINT Flags(0);
  HTREEITEM hSelected = m_pTreeCtrl->HitTest(mp, &Flags);

  if (hSelected != NULL || (Flags & TVHT_ONITEM))
    m_pTreeCtrl->SelectItem(hSelected);
  else
    return;

  //HTREEITEM hSelected = m_pTreeCtrl->GetSelectedItem();
  DisplayFolder(hSelected);
}

void CPWTreeView::OnItemDoubleClick(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
  /*
    Like Explorer - double click on a folder in left pane, expands the folder and
    displays contents in right pane
  */

 *pResult = 0;

  pws_os::Trace(L"CPWTreeView::OnItemDoubleClick %s pane\n",
        m_this_row == TOP ? L"Top" : L"Bottom");

  TVHITTESTINFO htinfo = {0};
  CPoint local = ::GetMessagePos();
  m_pTreeCtrl->ScreenToClient(&local);
  htinfo.pt = local;
  m_pTreeCtrl->HitTest(&htinfo);

  HTREEITEM hSelected = htinfo.hItem;
  if (hSelected == NULL)
    return;

  switch (htinfo.flags) {
    case TVHT_ONITEM:
    case TVHT_ONITEMBUTTON:
    case TVHT_ONITEMICON:
    case TVHT_ONITEMINDENT:
    {
      int iCode = (m_pTreeCtrl->GetItemState(hSelected, TVIS_EXPANDED) & TVIS_EXPANDED) ?
                         TVE_COLLAPSE : TVE_EXPAND;
      m_pTreeCtrl->Expand(hSelected, iCode);
      DisplayFolder(hSelected);
      break;
    }
    default:
      return;
  }
}

void CPWTreeView::DisplayFolder(HTREEITEM &hSelected)
{
  // Get the string of the selected node
  StringX sxRealPath = m_pTreeCtrl->GetItemText(hSelected);

  pws_os::Trace(L"CPWTreeView::DisplayFolder sxRealPath #1: %s\n",
        sxRealPath.c_str());

  HTREEITEM hParent = hSelected;

  //Build the full path
  do {
    hParent = m_pTreeCtrl->GetParentItem(hParent);
    if (hParent != NULL)
      sxRealPath = StringX(m_pTreeCtrl->GetItemText(hParent)) + sxDot + sxRealPath;
  } while (hParent != NULL);

  StringX sxDBName = m_pDbx->GetRoot();
  if (sxRealPath == sxDBName)
    sxRealPath = L"";
  else
    sxRealPath = sxRealPath.substr(sxDBName.length() + 1);

  pws_os::Trace(L"CPWTreeView::DisplayFolder %s pane, sxRealPath #2: %s\n",
        m_this_row == TOP ? L"Top" : L"Bottom", sxRealPath.c_str());

  m_pListView->DisplayEntries(sxRealPath, true);
  m_pDbx->UpdateToolBarForSelectedItem(NULL);
  m_pDbx->UpdateStatusBar();
}

void CPWTreeView::OnListViewFolderSelected(StringX sxPath, UINT index,
                                           const bool bUpdateHistory)
{
  // Find out what item is selected in the tree
  HTREEITEM hSelected = m_pTreeCtrl->GetSelectedItem();

  pws_os::Trace(L"CPWTreeView::OnListViewFolderSelected %s pane, sxPath: %s\n",
       m_this_row == TOP ? L"Top" : L"Bottom", sxPath.c_str());

  //Open up the branch
  m_pTreeCtrl->Expand(hSelected, TVE_EXPAND);

  int count = 0;
  HTREEITEM hChild;
  hChild = m_pTreeCtrl->GetChildItem(hSelected);

  if (index > 0) {
    do {
      hChild = m_pTreeCtrl->GetNextItem(hChild, TVGN_NEXT);
      ++count;
    } while (count < (int)index);
  }

  if (hChild != NULL) {
    m_pTreeCtrl->SelectItem(hChild);
    m_pTreeCtrl->Expand(hChild, TVE_EXPAND);

    hSelected = hChild;

    // Get the string of the selected node
    StringX sxSelected = m_pTreeCtrl->GetItemText(hSelected);

    HTREEITEM hParent = hSelected;

    //Build the full path with wild cards
    do {
      hParent = m_pTreeCtrl->GetParentItem(hParent);
      if (hParent != NULL)
        sxSelected = StringX(m_pTreeCtrl->GetItemText(hParent)) +
                        sxDot + sxSelected;
    } while (hParent != NULL);

    StringX sxDBName = m_pDbx->GetRoot();
    if (sxSelected == sxDBName)
      sxSelected = L"";
    else
      sxSelected = sxSelected.substr(sxDBName.length() + 1);

    pws_os::Trace(L"CPWTreeView::OnListViewFolderSelected %s pane, sxSelected: %s\n",
          m_this_row == TOP ? L"Top" : L"Bottom", sxSelected.c_str());

    m_pListView->DisplayEntries(sxSelected, bUpdateHistory);
  }
}

void CPWTreeView::DeleteEntries()
{
  // Don't do anything if we are the top row - we shouldn't be called
  if (m_this_row == TOP)
    return;

  ClearGroups();
}

void CPWTreeView::PopulateEntries()
{
  // Don't do anything if we are the top row - we shouldn't be called
  if (m_this_row == TOP)
    return;

  // Clear everything just in case
  ClearGroups();

  // Add all groups
  CreateTree(m_sxCurrentPath, false);

  // Find display status in row 0 and duplicate here
  HTREEITEM hItem = NULL;
  vector<bool> vdstatus;

  // As we are the bottom row, we use the current state of entries in the
  // top row
  while (NULL != (hItem = GetNextTreeItem(m_pOtherTreeCtrl, hItem))) {
    if (m_pOtherTreeCtrl->ItemHasChildren(hItem)) {
      bool bState = (m_pOtherTreeCtrl->GetItemState(hItem, TVIS_EXPANDED) &
                             TVIS_EXPANDED) != 0;
      vdstatus.push_back(bState);
    }
  }

  const size_t num = vdstatus.size();
  if (num == 0)
    return;

  hItem = NULL;
  size_t i(0);
  while (NULL != (hItem = GetNextTreeItem(m_pTreeCtrl, hItem))) {
    if (m_pTreeCtrl->ItemHasChildren(hItem)) {
      m_pTreeCtrl->Expand(hItem, vdstatus[i] ? TVE_EXPAND : TVE_COLLAPSE);
      i++;
      if (i == num)
        break;
    }
  }

  // Find which is selected in row 0 and select it here
  hItem = m_pOtherTreeCtrl->GetSelectedItem();
  // Get its path
  CString cs_path = GetGroupFullPath(m_pOtherTreeCtrl, hItem);
  // Find it here
  hItem = FindItem(m_pTreeCtrl, cs_path, m_pTreeCtrl->GetRootItem());
  // and select it
  m_pTreeCtrl->SelectItem(hItem);
  m_pDbx->UpdateToolBarForSelectedItem(NULL);
}

void CPWTreeView::OnBeginLabelEdit(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  NMTVDISPINFO *ptvinfo = reinterpret_cast<NMTVDISPINFO *>(pNotifyStruct);

  *pLResult = TRUE; // TRUE cancels label editing

  if (m_pDbx->IsDBReadOnly())
    return;

  m_bEditLabelCompleted = false;
  HTREEITEM ti = ptvinfo->item.hItem;

  // In case we have to revert:
  m_sxOldText = StringX(m_pTreeCtrl->GetItemText(ti));
  // Allow in-place editing
  *pLResult = FALSE;
}

void CPWTreeView::OnEndLabelEdit(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  if (m_pDbx->IsDBReadOnly())
    return; // don't edit in read-only mode

  // Initial verification performed in OnBeginLabelEdit - so some events may not get here!
  // Only items visible will be changed

  // NOTE: TreeView only has GROUPS!

  NMTVDISPINFO *ptvinfo = reinterpret_cast<NMTVDISPINFO *>(pNotifyStruct);

  HTREEITEM ti = ptvinfo->item.hItem;
  if (ptvinfo->item.pszText == NULL ||     // NULL if edit cancelled,
      ptvinfo->item.pszText[0] == L'\0') { // empty if text deleted - not allowed
    // If called from AddGroup, user cancels EditLabel - save it
    // (Still called "New Group")
    if (m_pDbx->IsInAddGroup()) {
      m_pDbx->ResetInAddGroup();
      *pLResult = TRUE;
    }
    return;
  }

  // Get proposed new Group name
  StringX sxNewText = ptvinfo->item.pszText;

  // Set up what we need
  StringX sxOldPath, sxNewPath;                  // For Node
  CString prefix;                                // For Node

  PWScore *pcore = (PWScore *)m_pDbx->GetCore();

  // Check if entry or group already here with this name
  HTREEITEM parent = m_pTreeCtrl->GetParentItem(ti);
  HTREEITEM sibling = m_pTreeCtrl->GetChildItem(parent);
  do {
    if (sibling != ti && !IsLeaf(m_pTreeCtrl, sibling)) {
      const CString siblingText = m_pTreeCtrl->GetItemText(sibling);
      if (siblingText == sxNewText.c_str())
        goto bad_exit;
    }
    sibling = m_pTreeCtrl->GetNextSiblingItem(sibling);
  } while (sibling != NULL);

  // If we made it here, then name's unique.
  // PR2407325: If the user edits a group name so that it has
  // a GROUP_SEP, all hell breaks loose.
  // Right Thing (tm) would be to parse and create subgroups as
  // needed, but this is too hard (for now), so we'll just reject
  // any group name that has one or more GROUP_SEP.
  if (sxNewText.find(GROUP_SEP) != StringX::npos) {
    m_pTreeCtrl->SetItemText(ti, m_sxOldText.c_str());
    goto bad_exit;
  } else {
    // Get prefix which is path up to and NOT including renamed node
    HTREEITEM parent, current = ti;

    do {
      parent = m_pTreeCtrl->GetParentItem(current);
      if (parent == NULL)
        break;

      current = parent;
      if (!prefix.IsEmpty())
        prefix = GROUP_SEP + prefix;

      prefix = m_pTreeCtrl->GetItemText(current) + prefix;
    } while (1);

    if (prefix.IsEmpty()) {
      sxOldPath = m_sxOldText;
      sxNewPath = sxNewText;
    } else {
      sxOldPath = StringX(prefix) + sxDot + m_sxOldText;
      sxNewPath = StringX(prefix) + sxDot + sxNewText;
    }
  }

  MultiCommands *pmulticmds = MultiCommands::Create(pcore);

  // We refresh the view
  Command *pcmd1 = UpdateGUICommand::Create(pcore,
                                            UpdateGUICommand::WN_UNDO,
                                            UpdateGUICommand::GUI_REFRESH_TREE);
  pmulticmds->Add(pcmd1);

  // Update Group
  pmulticmds->Add(RenameGroupCommand::Create(pcore, sxOldPath, sxNewPath));

  // We refresh the view
  Command *pcmd2 = UpdateGUICommand::Create(pcore,
                                            UpdateGUICommand::WN_EXECUTE_REDO,
                                            UpdateGUICommand::GUI_REFRESH_TREE);
  pmulticmds->Add(pcmd2);

  m_pDbx->Execute(pmulticmds);

  // Mark database as modified
  m_pDbx->SetChanged(DboxMain::Data);
  m_pDbx->ChangeOkUpdate();

  // Put edited text in right order by sorting
  m_pTreeCtrl->SortChildren(m_pTreeCtrl->GetRootItem());

  // OK
  *pLResult = TRUE;
  m_bEditLabelCompleted = true;

  return;

bad_exit:
  // Refresh display to show old text - if we don't no one else will
  m_pDbx->RefreshViews();

  // restore text
  // (not that this is documented anywhere in MS's docs...)
  *pLResult = FALSE;
}

// Functor for find_if to get path for a tree item
struct get_path {
  get_path(HTREEITEM const& hItem) : m_hItem(hItem) {}

  bool operator()(std::pair<StringX, HTREEITEM> const & p) const
  {
    return (p.second  == m_hItem);
  }

private:
  HTREEITEM m_hItem;
};

bool CPWTreeView::FindGroup(HTREEITEM hItem, StringX &sxPath)
{
  get_path gp(hItem);

  PathMapIter iter;
  iter = std::find_if(m_mapGroup2Item.begin(), m_mapGroup2Item.end(), get_path(hItem));
  if (iter != m_mapGroup2Item.end()) {
    sxPath = iter->first;
    return true;
  } else {
    sxPath = L"";
    return false;
  }
}

#ifdef EXPLORER_DRAG_DROP
void CPWTreeView::OnMouseMove(UINT nFlags, CPoint point)
{
  if (!m_bMouseInWindow) {
    m_bMouseInWindow = true;
    TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
    VERIFY(TrackMouseEvent(&tme));
  }

  if (m_bDragging) {
    //
  }

  CTreeView::OnMouseMove(nFlags, point);
}

LRESULT CPWTreeView::OnMouseLeave(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
  m_bMouseInWindow = false;
  return 0L;
}

SCODE CPWTreeView::GiveFeedback(DROPEFFECT /*dropEffect*/)
{
  m_pDbx->ResetIdleLockCounter();
  return DRAGDROP_S_USEDEFAULTCURSORS;
}

DROPEFFECT CPWTreeView::OnDragEnter(CWnd * /*pWnd*/, COleDataObject *pDataObject,
                                    DWORD dwKeyState, CPoint /*point*/)
{
  // Is it ours?
  if (!pDataObject->IsDataAvailable(m_tcddCPFID, NULL))
    return DROPEFFECT_NONE;

  POINT pt, hs;
  CImageList *pil = CImageList::GetDragImage(&pt, &hs);
  if (pil != NULL) {
    pws_os::Trace(L"CPWTreeView::OnDragEnter() hide cursor\n");
    while (ShowCursor(FALSE) >= 0)
      ;
  }

  m_bWithinThisInstance = true;
  return ((dwKeyState & MK_CONTROL) == MK_CONTROL) ?
         DROPEFFECT_COPY : DROPEFFECT_MOVE;
}

DROPEFFECT CPWTreeView::OnDragOver(CWnd *pWnd, COleDataObject *pDataObject,
                                   DWORD dwKeyState, CPoint point)
{
  // Is it ours?
  if (!pDataObject->IsDataAvailable(m_tcddCPFID, NULL))
    return DROPEFFECT_NONE;

  CTreeCtrl *pDestTreeCtrl = &((CTreeView *)pWnd)->GetTreeCtrl();
  HTREEITEM hHitItem(NULL);

  POINT pt, hs;
  CImageList *pil = CImageList::GetDragImage(&pt, &hs);

  if (pil != NULL) pil->DragMove(point);

  hHitItem = pDestTreeCtrl->HitTest(point);

  if (hHitItem != NULL) {
    // Highlight the item under the mouse anyway
    if (pil != NULL) pil->DragLeave(this);
    pDestTreeCtrl->SelectDropTarget(hHitItem);
    m_hitemDrop = hHitItem;
    if (pil != NULL) pil->DragEnter(this, point);
  }

  CRect rectClient;
  pWnd->GetClientRect(&rectClient);
  pWnd->ClientToScreen(rectClient);
  pWnd->ClientToScreen(&point);

  // Scroll TREE control depending on mouse position
  int iMaxV = GetScrollLimit(SB_VERT);
  int iPosV = GetScrollPos(SB_VERT);

  const int SCROLL_BORDER = 10;
  int nScrollDir = -1;
  if ((point.y > rectClient.bottom - SCROLL_BORDER) && (iPosV != iMaxV))
    nScrollDir = SB_LINEDOWN;
  else if ((point.y < rectClient.top + SCROLL_BORDER) && (iPosV != 0))
    nScrollDir = SB_LINEUP;

  if (nScrollDir != -1) {
    int nScrollPos = pWnd->GetScrollPos(SB_VERT);
    WPARAM wParam = MAKELONG(nScrollDir, nScrollPos);
    pWnd->SendMessage(WM_VSCROLL, wParam);
  }

  int iPosH = GetScrollPos(SB_HORZ);
  int iMaxH = GetScrollLimit(SB_HORZ);

  nScrollDir = -1;
  if ((point.x < rectClient.left + SCROLL_BORDER) && (iPosH != 0))
    nScrollDir = SB_LINELEFT;
  else if ((point.x > rectClient.right - SCROLL_BORDER) && (iPosH != iMaxH))
    nScrollDir = SB_LINERIGHT;

  if (nScrollDir != -1) {
    int nScrollPos = pWnd->GetScrollPos(SB_VERT);
    WPARAM wParam = MAKELONG(nScrollDir, nScrollPos);
    pWnd->SendMessage(WM_HSCROLL, wParam);
  }

  DROPEFFECT dropeffectRet;
  // If we're dragging between processes, default is to COPY, Ctrl key
  // changes this to MOVE.
  // If we're dragging in the same process, default is to MOVE, Ctrl
  // key changes this to COPY
  if (pil == NULL)
    dropeffectRet = ((dwKeyState & MK_CONTROL) == MK_CONTROL) ?
                    DROPEFFECT_MOVE : DROPEFFECT_COPY;
  else
    dropeffectRet = ((dwKeyState & MK_CONTROL) == MK_CONTROL) ?
                    DROPEFFECT_COPY : DROPEFFECT_MOVE;

  return dropeffectRet;
}

void CPWTreeView::OnDragLeave()
{
  m_bWithinThisInstance = false;
  // ShowCursor's semantics are VERY odd - RTFM
  pws_os::Trace(L"CPWTreeView::OnDragLeave() show cursor\n");
  while (ShowCursor(TRUE) < 0)
    ;
}

bool CPWTreeView::CollectData(BYTE * &out_buffer, long &outLen)
{
  CDDObList out_oblist;

  // Only nodes to drag in Tree view
  StringX sxDragPath = GetGroupFullPath(m_pTreeCtrl, m_pTreeCtrl->GetParentItem(m_hitemDrag));
  m_nDragPathLen = sxDragPath.length();
  out_oblist.m_bDraggingGroup = true;

  m_pDbx->GetGroupEntriesData(out_oblist, sxDragPath);

  CSMemFile outDDmemfile;
  out_oblist.DDSerialize(outDDmemfile);

  outLen = (long)outDDmemfile.GetLength();
  out_buffer = (BYTE *)outDDmemfile.Detach();

  while (!out_oblist.IsEmpty()) {
    delete (CDDObject *)out_oblist.RemoveHead();
  }

  return (outLen > 0);
}

bool CPWTreeView::ProcessData(BYTE *in_buffer, const long &inLen,
                              const CSecString &DropGroup, const bool bCopy)
{
#ifdef DUMP_DATA
  std:wstring stimestamp;
  PWSUtil::GetTimeStamp(stimestamp);
  pws_os::Trace(L"Drop data: length %d/0x%04x, value:\n", inLen, inLen);
  pws_os::HexDump(in_buffer, inLen, stimestamp);
#endif /* DUMP_DATA */

  if (inLen <= 0)
    return false;

  CDDObList in_oblist;
  CSMemFile inDDmemfile;

  inDDmemfile.Attach((BYTE *)in_buffer, inLen);

  in_oblist.DDUnSerialize(inDDmemfile);

  inDDmemfile.Detach();

  if (!in_oblist.IsEmpty()) {
    m_pDbx->AddDDEntries(in_oblist, DropGroup, m_bWithinThisInstance, bCopy);

    // Finished with them - delete and remove from object list
    while (!in_oblist.IsEmpty()) {
      delete (CDDObject *)in_oblist.RemoveHead();
    }
  }
  return (inLen > 0);
}

BOOL CPWTreeView::OnDrop(CWnd *, COleDataObject *pDataObject,
                         DROPEFFECT dropEffect, CPoint point)
{
  // Is it ours?
  if (!pDataObject->IsDataAvailable(m_tcddCPFID, NULL))
    return FALSE;

  m_bDragging = false;
  pws_os::Trace(L"CPWTreeView::OnDrop() show cursor\n");
  while (ShowCursor(TRUE) < 0)
    ;

  POINT pt, hs;
  CImageList *pil = CImageList::GetDragImage(&pt, &hs);
  // pil will be NULL if we're the target of inter-process D&D

  if (pil != NULL) {
    pil->DragLeave(this);
    pil->EndDrag();
    pil->DeleteImageList();
  }

  if (m_pDbx->IsDBReadOnly())
    return FALSE; // don't drop in read-only mode

  if (!pDataObject->IsDataAvailable(m_tcddCPFID, NULL))
    return FALSE;

  UINT uFlags;
  HTREEITEM hitemDrop = m_pTreeCtrl->HitTest(point, &uFlags);

  bool bForceRoot(false);
  switch (uFlags) {
    case TVHT_ABOVE: case TVHT_BELOW: case TVHT_TOLEFT: case TVHT_TORIGHT:
      return FALSE;
    case TVHT_NOWHERE:
      if (hitemDrop == NULL) {
        // Treat as drop in root
        hitemDrop = m_pTreeCtrl->GetRootItem();
        bForceRoot = true;
      } else
        return FALSE;
      break;
    case TVHT_ONITEM: case TVHT_ONITEMBUTTON: case TVHT_ONITEMICON:
    case TVHT_ONITEMINDENT: case TVHT_ONITEMLABEL: case TVHT_ONITEMRIGHT:
    case TVHT_ONITEMSTATEICON:
      if (hitemDrop == NULL)
        return FALSE;
      break;
    default:
      return FALSE;
  }

  BOOL retval(FALSE);

  // On Drop of data from one tree to another
  HGLOBAL hGlobal = pDataObject->GetGlobalData(m_tcddCPFID);
  BYTE *pData = (BYTE *)GlobalLock(hGlobal);
  ASSERT(pData != NULL);

  SIZE_T memsize = GlobalSize(hGlobal);

  if (memsize <= OLE_HDR_LEN) // OLE_HDR_FMT
    goto exit;

  // iDDType = D&D type FROMTREE_L/FROMTREE_R or
  //    for column D&D only FROMCC, FROMHDR
  // lBufLen = Length of D&D data appended to this data
  unsigned long lPid;
  int iDDType;
  long lBufLen;

#if (_MSC_VER >= 1400)
  sscanf_s((char *)pData, OLE_HDR_FMT, &lPid, &iDDType, &lBufLen);
#else
  sscanf((char *)pData, OLE_HDR_FMT, &lPid, &iDDType, &lBufLen);
#endif
  pData += OLE_HDR_LEN; // so ProcessData won't sweat

  // NULL-ness of pil is also a good indicator of intra/inter-ness
  // alternately, we can also raise an m_flag in OnBeginDrag.
  // However, plugging the process ID in the header
  // is the most direct and straightforward way to do this,
  // and probably the most robust...
  m_bWithinThisInstance = (lPid == GetCurrentProcessId());

  // Check if it is from another TreeCtrl or Explorer view (left or right mouse drag)?
  // - we don't accept drop from anything else
  if (iDDType == FROMCC || iDDType == FROMHDR)
    goto exit;

  if (iDDType == FROMTREE_R || iDDType == FROMTREE_RSC) {
    CMenu menu;
    if (menu.LoadMenu(IDR_POPRIGHTDRAG)) {
      CMenu *pPopup = menu.GetSubMenu(0);
      ASSERT(pPopup != NULL);
      ClientToScreen(&point);
      pPopup->SetDefaultItem(GetKeyState(VK_CONTROL) < 0 ?
                             ID_MENUITEM_COPYHERE : ID_MENUITEM_MOVEHERE);
      if (!m_bWithinThisInstance || iDDType != FROMTREE_RSC)
        pPopup->EnableMenuItem(ID_MENUITEM_RCREATESHORTCUT, MF_BYCOMMAND | MF_GRAYED);

      DWORD dwcode = pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON |
                                            TPM_NONOTIFY | TPM_RETURNCMD,
                                            point.x, point.y, this);
      pPopup->DestroyMenu();
      switch (dwcode) {
        case ID_MENUITEM_COPYHERE:
          dropEffect = DROPEFFECT_COPY;
          break;
        case ID_MENUITEM_MOVEHERE:
          dropEffect = DROPEFFECT_MOVE;
          break;
        case ID_MENUITEM_RCREATESHORTCUT:
        {
          // Shortcut group from drop point, title & user from drag entry
          // This is only allowed if one entry is dragged
          // Dropping on a group is only possible in a TreeView

          // Get the entry's data (really "ProcessData" without the call to AddEntries)
          CDDObList in_oblist;
          CSMemFile inDDmemfile;
          inDDmemfile.Attach((BYTE *)pData, lBufLen);
          in_oblist.DDUnSerialize(inDDmemfile);
          inDDmemfile.Detach();
          
          ASSERT(in_oblist.GetSize() == 1);
          POSITION pos = in_oblist.GetHeadPosition();
          CDDObject *pin_obj = (CDDObject *)in_oblist.GetAt(pos);
          CItemData ci;
          pin_obj->ToItem(ci);

          // No longer need object
          delete (CDDObject *)in_oblist.RemoveHead();

          CSecString cs_group, cs_title, cs_user;
          cs_group = CSecString(GetGroupFullPath(m_pTreeCtrl, m_hitemDrop));
          cs_title.Format(IDS_SCTARGET, ci.GetTitle().c_str());
          cs_user = ci.GetUser();

          // If there is a matching entry in our list, generate unique one
          if (m_pDbx->Find(cs_group, cs_title, cs_user) != m_pDbx->End()) {
            cs_title = m_pDbx->GetUniqueTitle(cs_group, cs_title, cs_user, IDS_DRAGNUMBER);
          }
          StringX sxNewDBPrefsString(L"");
          m_pDbx->CreateShortcutEntry(&ci, cs_group, cs_title, cs_user, sxNewDBPrefsString);
          retval = TRUE;
          m_pTreeCtrl->SelectItem(NULL);  // Deselect
          goto exit;
        }
        case ID_MENUITEM_CANCEL:
        default:
          m_pTreeCtrl->SelectItem(NULL);  // Deselect
          goto exit;
      }
    }
  }

  // Can't use "m_pTreeCtrl->GetCount() == 0" to check empty database as in the
  // normal TreeCtrl since we only have groups and it may not have any but will
  // still have the database name as root
  if (hitemDrop == NULL && m_pDbx->GetNumEntries() == 0) {
    // Dropping on to an empty database
    CSecString DropGroup (L"");
    ProcessData(pData, lBufLen, DropGroup);
    m_pTreeCtrl->SelectItem(m_pTreeCtrl->GetRootItem());
    retval = TRUE;
    goto exit;
  }

  if (IsLeaf(m_pTreeCtrl, hitemDrop) || bForceRoot)
    hitemDrop = m_pTreeCtrl->GetParentItem(hitemDrop);

  if (m_bWithinThisInstance) {
    // From me! - easy
    HTREEITEM parent = m_pTreeCtrl->GetParentItem(m_hitemDrag);
    if (m_hitemDrag != hitemDrop &&
        //!IsChildNodeOf(m_pTreeCtrl, hitemDrop, m_hitemDrag) &&
        parent != hitemDrop) {
      // Drag operation allowed
      //ProcessData((BYTE *)pData, lBufLen, DropGroup, dropEffect == DROPEFFECT_COPY);
      retval = TRUE;
    } else {
      // drag failed or cancelled, revert to last selected
      m_pTreeCtrl->SelectItem(m_hitemDrag);
      goto exit;
    }
  } else { // from someone else!
    // Now add it
    CSecString DropGroup = CSecString(GetGroupFullPath(m_pTreeCtrl, hitemDrop));
    ProcessData((BYTE *)pData, lBufLen, DropGroup);
    m_pTreeCtrl->SelectItem(hitemDrop);
    retval = TRUE;
  }

  m_pTreeCtrl->SortChildren(TVI_ROOT);
  m_pDbx->FixListIndexes();
  GetParent()->SetFocus();

exit:
  GlobalUnlock(hGlobal);

  if (retval == TRUE) {
    m_pDbx->SetChanged(DboxMain::Data);
    m_pDbx->ChangeOkUpdate();
    if (m_pDbx->IsFilterActive())
      m_pDbx->RefreshViews();
  }
  return retval;
}

void CPWTreeView::OnBeginDrag(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  // This sets the whole D&D mechanism in motion...
  if (pNotifyStruct->code == TVN_BEGINDRAG)
    m_DDType = FROMTREE_L; // Left  mouse D&D
  else
    m_DDType = FROMTREE_R; // Right mouse D&D

  m_DDType |= (m_this_row == TOP) ? FROMROW0 : FROMROW1;

  CPoint ptAction;

  NM_TREEVIEW *pNMTreeView = reinterpret_cast<NM_TREEVIEW *>(pNotifyStruct);
  *pLResult = 0L;

  m_vDragItems.clear();
 
  GetCursorPos(&ptAction);
  ScreenToClient(&ptAction);
  m_hitemDrag = pNMTreeView->itemNew.hItem;
  m_hitemDrop = NULL;
  m_pTreeCtrl->SelectItem(m_hitemDrag);

  delete m_pDragImage;
  // Bug in MS TreeCtrl and CreateDragImage.  During Drag, it doesn't show
  // the entry's text as well as the drag image if the font is not MS Sans Serif !!!!
  SetFont(Fonts::GetInstance()->GetDragFixFont(), false);
  m_pDragImage = m_pTreeCtrl->CreateDragImage(m_hitemDrag);
  ASSERT(m_pDragImage);
  SetFont(Fonts::GetInstance()->GetCurrentFont(), false);

  m_pDragImage->SetDragCursorImage(0, CPoint(0, 0));
  m_pDragImage->BeginDrag(0, CPoint(0,0));
  m_pDragImage->DragMove(ptAction);
  m_pDragImage->DragEnter(GetDesktopWindow(), pNMTreeView->ptDrag);
  m_bDragging = true;

  pws_os::Trace(L"CPWTreeView::OnBeginDrag() hide cursor\n");
  while (ShowCursor(FALSE) >= 0)
    ;
  //SetCapture();

  RECT rClient;
  GetClientRect(&rClient);

  // Tree View does NOT have Entries - only Groups
  // Get all the UUIDs affected - this includes specific entries and all entries
  // within the selected group and its sub-groups.

  // Start dragging
  m_bDropped = false;
  DROPEFFECT de = m_DataSource->StartDragging(m_tcddCPFID, &rClient);

  // If inter-process Move, we need to delete original
  if (m_cfdropped == m_tcddCPFID &&
      (de & DROPEFFECT_MOVE) == DROPEFFECT_MOVE &&
      !m_bWithinThisInstance && !m_pDbx->IsDBReadOnly()) {
    StringX sxCurrentPath;
    CItemData *pci = (CItemData *)m_pTreeCtrl->GetItemData(m_hitemDrag);

    // Get complete group name
    sxCurrentPath = (StringX)GetGroupFullPath(m_pTreeCtrl, m_hitemDrag); // e.g., a.b.c

    std::vector<pws_os::CUUID> vGroupEntries;
    m_pDbx->GetGroupEntries(sxCurrentPath, &vGroupEntries, true);
    m_pDbx->Delete(pci, vGroupEntries);
  }

  m_pDragImage->DragLeave(GetDesktopWindow());
  m_pDragImage->EndDrag();

  if (de == DROPEFFECT_NONE) {
    pws_os::Trace(L"m_DataSource->StartDragging() failed\n");
    // Do cleanup - otherwise this is the responsibility of the recipient!
    if (m_hgDataALL != NULL) {
      LPVOID lpData = GlobalLock(m_hgDataALL);
      SIZE_T memsize = GlobalSize(m_hgDataALL);
      if (lpData != NULL && memsize > 0) {
        trashMemory(lpData, memsize);
      }
      GlobalUnlock(m_hgDataALL);
      GlobalFree(m_hgDataALL);
      m_hgDataALL = NULL;
    }
    if (m_hgDataTXT != NULL) {
      LPVOID lpData = GlobalLock(m_hgDataTXT);
      SIZE_T memsize = GlobalSize(m_hgDataTXT);
      if (lpData != NULL && memsize > 0) {
        trashMemory(lpData, memsize);
      }
      GlobalUnlock(m_hgDataTXT);
      GlobalFree(m_hgDataTXT);
      m_hgDataTXT = NULL;
    }
    if (m_hgDataUTXT != NULL) {
      LPVOID lpData = GlobalLock(m_hgDataUTXT);
      SIZE_T memsize = GlobalSize(m_hgDataUTXT);
      if (lpData != NULL && memsize > 0) {
        trashMemory(lpData, memsize);
      }
      GlobalUnlock(m_hgDataUTXT);
      GlobalFree(m_hgDataUTXT);
      m_hgDataUTXT = NULL;
    }
  }

  pws_os::Trace(L"CPWTreeView::OnBeginDrag() show cursor\n");
  while (ShowCursor(TRUE) < 0)
    ;

  // We did call SetCapture - do we release it here?  If not, where else?
  //ReleaseCapture();

BOOL CPWTreeView::OnRenderGlobalData(LPFORMATETC lpFormatEtc, HGLOBAL *phGlobal)
{
  if (m_hgDataALL != NULL) {
    pws_os::Trace(L"CPWTreeView::OnRenderGlobalData - Unlock/Free m_hgDataALL\n");
    LPVOID lpData = GlobalLock(m_hgDataALL);
    SIZE_T memsize = GlobalSize(m_hgDataALL);
    if (lpData != NULL && memsize > 0) {
      trashMemory(lpData, memsize);
    }
    GlobalUnlock(m_hgDataALL);
    GlobalFree(m_hgDataALL);
    m_hgDataALL = NULL;
  }

  if (m_hgDataTXT != NULL) {
    pws_os::Trace(L"CPWTreeView::OnRenderGlobalData - Unlock/Free m_hgDataTXT\n");
    LPVOID lpData = GlobalLock(m_hgDataTXT);
    SIZE_T memsize = GlobalSize(m_hgDataTXT);
    if (lpData != NULL && memsize > 0) {
      trashMemory(lpData, memsize);
    }
    GlobalUnlock(m_hgDataTXT);
    GlobalFree(m_hgDataTXT);
    m_hgDataTXT = NULL;
  }

  if (m_hgDataUTXT != NULL) {
    pws_os::Trace(L"CPWTreeView::OnRenderGlobalData - Unlock/Free m_hgDataUTXT\n");
    LPVOID lpData = GlobalLock(m_hgDataUTXT);
    SIZE_T memsize = GlobalSize(m_hgDataUTXT);
    if (lpData != NULL && memsize > 0) {
      trashMemory(lpData, memsize);
    }
    GlobalUnlock(m_hgDataUTXT);
    GlobalFree(m_hgDataUTXT);
    m_hgDataUTXT = NULL;
  }

  BOOL retval;
  if (lpFormatEtc->cfFormat == m_tcddCPFID) {
    m_cfdropped = m_tcddCPFID;
    retval = RenderAllData(phGlobal);
  } else
    return FALSE;

  return retval;
}

BOOL CPWTreeView::RenderAllData(HGLOBAL *phGlobal)
{
  long lBufLen;
  BYTE *buffer = NULL;

  ASSERT(m_hgDataALL == NULL);

  // CollectData allocates buffer - need to free later
  if (!CollectData(buffer, lBufLen))
    return FALSE;

  char header[OLE_HDR_LEN + 1];
  // Note: GetDDType will return any except FROMCC and FROMHDR
#if (_MSC_VER >= 1400)
  sprintf_s(header, sizeof(header),
            OLE_HDR_FMT, GetCurrentProcessId(), GetDDType(), lBufLen);
#else
  sprintf(header, OLE_HDR_FMT, GetCurrentProcessId(), GetDDType(), lBufLen);
#endif
  CMemFile mf;
  mf.Write(header, OLE_HDR_LEN);
  mf.Write(buffer, lBufLen);

  // Finished with buffer - trash it and free it
  trashMemory((void *)buffer, lBufLen);
  free(buffer);

  LPVOID lpData(NULL);
  LPVOID lpDataBuffer;
  DWORD dwBufLen = (DWORD)mf.GetLength();
  lpDataBuffer = (LPVOID)(mf.Detach());

#ifdef DUMP_DATA
  std::wstring stimestamp;
  PWSUtil::GetTimeStamp(stimestamp);
  pws_os::Trace(L"Drag data: length %d/0x%04x, value:\n", dwBufLen, dwBufLen);
  pws_os::HexDump(lpDataBuffer, dwBufLen, stimestamp);
#endif /* DUMP_DATA */

  BOOL retval(FALSE);
  if (*phGlobal == NULL) {
    pws_os::Trace(L"CPWTreeView::OnRenderAllData - Alloc global memory\n");
    m_hgDataALL = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, dwBufLen);
    ASSERT(m_hgDataALL != NULL);
    if (m_hgDataALL == NULL)
      goto bad_return;

    lpData = GlobalLock(m_hgDataALL);
    ASSERT(lpData != NULL);
    if (lpData == NULL)
      goto bad_return;

    // Copy data
    memcpy(lpData, lpDataBuffer, dwBufLen);
    *phGlobal = m_hgDataALL;
    retval = TRUE;
  } else {
    pws_os::Trace(L"CPWTreeView::OnRenderAllData - *phGlobal NOT NULL!\n");
    SIZE_T inSize = GlobalSize(*phGlobal);
    SIZE_T ourSize = GlobalSize(m_hgDataALL);
    if (inSize < ourSize) {
      // Pre-allocated space too small.  Not allowed to increase it - FAIL
      pws_os::Trace(L"CPWTreeView::OnRenderAllData - NOT enough room - FAIL\n");
    } else {
      // Enough room - copy our data into supplied area
      pws_os::Trace(L"CPWTreeView::OnRenderAllData - enough room - copy our data\n");
      LPVOID pInGlobalLock = GlobalLock(*phGlobal);
      ASSERT(pInGlobalLock != NULL);
      if (pInGlobalLock == NULL)
        goto bad_return;

      memcpy(pInGlobalLock, lpDataBuffer, ourSize);
      GlobalUnlock(*phGlobal);
      retval = TRUE;
    }
  }

bad_return:
  if (dwBufLen != 0 && lpDataBuffer != NULL) {
    trashMemory(lpDataBuffer, dwBufLen);
    free(lpDataBuffer);
    lpDataBuffer = NULL;
  }

  // If retval == TRUE, recipient is responsible for freeing the global memory
  // if D&D succeeds
  if (retval == FALSE) {
    pws_os::Trace(L"CPWTreeView::RenderAllData - returning FALSE!\n");
    if (m_hgDataALL != NULL) {
      LPVOID lpData = GlobalLock(m_hgDataALL);
      SIZE_T memsize = GlobalSize(m_hgDataALL);
      if (lpData != NULL && memsize > 0) {
        trashMemory(lpData, memsize);
      }
      GlobalUnlock(m_hgDataALL);
      GlobalFree(m_hgDataALL);
      m_hgDataALL = NULL;
    }
  }

  if (lpData != NULL)
    GlobalUnlock(m_hgDataALL);

  return retval;
}
#endif
