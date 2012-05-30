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
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPWTreeView construction/destruction

CPWTreeView::CPWTreeView()
 : m_pParent(NULL), m_pTreeCtrl(NULL), m_pListView(NULL),
 m_pOtherTreeView(NULL), m_pOtherTreeCtrl(NULL), m_pDbx(NULL),
 m_bInitDone(false), m_bAccEn(false), m_bInRename(false),
 m_this_row(INVALID_ROW)
{
}

CPWTreeView::~CPWTreeView()
{
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
  if (hItem != NULL && hItem != m_pTreeCtrl->GetRootItem()) {
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
  PathMapCIter lb_iter, ub_iter, iter;
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

  // Get empty groups (unsorted)
  PathSet setEmptyGroups;
  setEmptyGroups = m_pDbx->GetEmptyGroups();

  StringX sxRoot;
  sxRoot = m_pDbx->GetRoot();
  const size_t len = sxRoot.length();

  for (size_t i = 0; i < vGroups.size(); i++) {
    StringX sxPRGPath = sxRoot + sxDot + vGroups[i];
    PlaceInTree(sxPRGPath, len);
  }

  PathSetCIter citer;
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
  UINT uiFlags(0);
  HTREEITEM hSelected = m_pTreeCtrl->HitTest(mp, &uiFlags);

  if (hSelected != NULL && (uiFlags & (TVHT_ONITEM | TVHT_ONITEMBUTTON | TVHT_ONITEMINDENT)))
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
  
  // Don't allow rename/edit of Database name
  if (ti == m_pTreeCtrl->GetRootItem())
    return;

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
