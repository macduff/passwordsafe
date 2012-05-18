/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "TreeUtils.h"

#include "core/ItemData.h"

extern wchar_t GROUP_SEP;
extern StringX sxDot;

HTREEITEM TreeUtils::GetNextTreeItem(CTreeCtrl *pTreeCtrl, HTREEITEM hItem)
{
  if (hItem == NULL)
    return pTreeCtrl->GetRootItem();

  // First, try to go to this item's 1st child
  HTREEITEM hReturn = pTreeCtrl->GetChildItem(hItem);

  // If no more child items...
  while (hItem && !hReturn) {
    // Get this item's next sibling
    hReturn = pTreeCtrl->GetNextSiblingItem(hItem);

    // If hReturn is NULL, then there are no sibling items, and we're on a leaf node.
    // Backtrack up the tree one level, and we'll look for a sibling on the next
    // iteration (or we'll reach the root and quit).
    hItem = pTreeCtrl->GetParentItem(hItem);
  }
  return hReturn;
}

HTREEITEM TreeUtils::FindItem(CTreeCtrl *pTreeCtrl, const CString &path,
                              const HTREEITEM hRoot)
{
  // check whether the current item is the searched one
  CString cs_thispath = GetGroupFullPath(pTreeCtrl, hRoot);
  if (cs_thispath.Compare(path) == 0)
    return hRoot;

  // get a handle to the first child item
  HTREEITEM hSub = pTreeCtrl->GetChildItem(hRoot);

  // iterate as long a new item is found
  while (hSub) {
    // check the children of the current item
    HTREEITEM hFound = FindItem(pTreeCtrl, path, hSub);
    if (hFound)
      return hFound;

    // get the next sibling of the current item
    hSub = pTreeCtrl->GetNextSiblingItem(hSub);
  }

  // return NULL if nothing was found
  return NULL;
}

// Return the full path leading up to a given item, but
// not including the name of the item itself.
CString TreeUtils::GetGroupFullPath(CTreeCtrl *pTreeCtrl, HTREEITEM hItem)
{
  CString retval(L""), nodeText;
  if (hItem == TVI_ROOT)
    return retval;

  while (hItem != NULL) {
    nodeText = pTreeCtrl->GetItemText(hItem);
    if (!retval.IsEmpty())
      nodeText += GROUP_SEP;

    retval = nodeText + retval;
    hItem = pTreeCtrl->GetParentItem(hItem);
  }
  return retval;
}

CSecString TreeUtils::GetPathElem(CSecString &path)
{
  // Get first path element and chop it off, i.e., if
  // path = "a.b.c.d"
  // will return "a" and path will be "b.c.d"
  // (assuming GROUP_SEP is '.')

  CSecString retval;
  int N = path.Find(GROUP_SEP);
  if (N == -1) {
    retval = path;
    path = L"";
  } else {
    const int Len = path.GetLength();
    retval = CSecString(path.Left(N));
    path = CSecString(path.Right(Len - N - 1));
  }
  return retval;
}

bool TreeUtils::SplitLeafText(const wchar_t *lt, StringX &newTitle,
                              StringX &newUser, StringX &newPassword)
{
  bool bPasswordSet(false);

  newTitle = newUser = newPassword = L"";

  CString cs_leafText(lt);
  cs_leafText.Trim();
  if (cs_leafText.IsEmpty())
    return false;

  // Check no duplicate braces
  int OpenSquareBraceIndex = cs_leafText.Find(L'[');
  if (OpenSquareBraceIndex >= 0)
    if (cs_leafText.Find(L'[', OpenSquareBraceIndex + 1) != -1)
      return false;

  int CloseSquareBraceIndex = cs_leafText.Find(L']');
  if (CloseSquareBraceIndex >= 0)
    if (cs_leafText.Find(L']', CloseSquareBraceIndex + 1) != -1)
      return false;

  int OpenCurlyBraceIndex = cs_leafText.Find(L'{');
  if (OpenCurlyBraceIndex >= 0)
    if (cs_leafText.Find(L'{', OpenCurlyBraceIndex + 1) != -1)
      return false;

  int CloseCurlyBraceIndex = cs_leafText.Find(L'}');
  if (CloseCurlyBraceIndex >= 0)
    if (cs_leafText.Find(L'}', CloseCurlyBraceIndex + 1) != -1)
      return false;

  // Check we have both open and close brackets
  if (OpenSquareBraceIndex >= 0 && CloseSquareBraceIndex == -1)
    return false;
  if (OpenSquareBraceIndex == -1 && CloseSquareBraceIndex >= 0)
    return false;
  if (OpenCurlyBraceIndex >= 0 && CloseCurlyBraceIndex == -1)
    return false;
  if (OpenCurlyBraceIndex == -1 && CloseCurlyBraceIndex >= 0)
    return false;

  // Check we are in the right order - open before close
  if (OpenSquareBraceIndex >= 0 && CloseSquareBraceIndex < OpenSquareBraceIndex)
    return false;
  if (OpenCurlyBraceIndex >= 0 && CloseCurlyBraceIndex < OpenCurlyBraceIndex)
    return false;

  // Check we are in the right order - square before curly
  if (OpenSquareBraceIndex >= 0 && OpenCurlyBraceIndex >= 0 &&
    OpenCurlyBraceIndex < OpenSquareBraceIndex)
    return false;

  if (OpenSquareBraceIndex == -1 && OpenCurlyBraceIndex == -1) {
    // title
    newTitle = cs_leafText;
    return true;
  }

  if (OpenSquareBraceIndex >= 0 && OpenCurlyBraceIndex == -1) {
    // title [user]
    newTitle = cs_leafText.Left(OpenSquareBraceIndex);
    Trim(newTitle);
    newUser = cs_leafText.Mid(OpenSquareBraceIndex + 1,
                              CloseSquareBraceIndex - OpenSquareBraceIndex - 1);
    Trim(newUser);
    goto final_check;
  }

  if (OpenSquareBraceIndex == -1 && OpenCurlyBraceIndex >= 0) {
    // title {password}
    newTitle = cs_leafText.Left(OpenCurlyBraceIndex);
    Trim(newTitle);
    newPassword = cs_leafText.Mid(OpenCurlyBraceIndex + 1,
                                  CloseCurlyBraceIndex - OpenCurlyBraceIndex - 1);
    Trim(newPassword);
    bPasswordSet = true;
    goto final_check;
  }

  if (OpenSquareBraceIndex >= 0 && OpenCurlyBraceIndex >= 0) {
    // title [user] {password}
    newTitle = cs_leafText.Left(OpenSquareBraceIndex);
    Trim(newTitle);
    newUser = cs_leafText.Mid(OpenSquareBraceIndex + 1,
                              CloseSquareBraceIndex - OpenSquareBraceIndex - 1);
    Trim(newUser);
    newPassword = cs_leafText.Mid(OpenCurlyBraceIndex + 1,
                                  CloseCurlyBraceIndex - OpenCurlyBraceIndex - 1);
    Trim(newPassword);
    bPasswordSet = true;
    goto final_check;
  }

  return false; // Should never get here!

final_check:
  bool bRC(true);
  if (newTitle.empty())
    bRC = false;

  if (bPasswordSet && newPassword.empty())
    bRC = false;

  return bRC;
}

bool TreeUtils::IsLeaf(CTreeCtrl *pTreeCtrl, const HTREEITEM hItem)
{
  int i, dummy;
  BOOL status = pTreeCtrl->GetItemImage(hItem, i, dummy);
  ASSERT(status);
  return (i != CItemData::EI_GROUP && i != CItemData::EI_EMPTY_GROUP &&
          i != CItemData::EI_DATABASE);
}

bool TreeUtils::ExistsInTree(CTreeCtrl *pTreeCtrl, const HTREEITEM &node,
                             const CSecString &s, HTREEITEM &si)
{
  // returns true iff s is a direct descendant of node
  HTREEITEM ti = pTreeCtrl->GetChildItem(node);

  while (ti != NULL) {
    const CSecString itemText = pTreeCtrl->GetItemText(ti);
    if (itemText == s)
      if (!IsLeaf(pTreeCtrl, ti)) { // A non-node doesn't count
        si = ti;
        return true;
      }
    ti = pTreeCtrl->GetNextItem(ti, TVGN_NEXT);
  }
  return false;
}

bool TreeUtils::IsChildNodeOf(CTreeCtrl *pTreeCtrl,
                              HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent)
{
  do {
    if (hitemChild == hitemSuspectedParent)
      break;
  } while ((hitemChild = pTreeCtrl->GetParentItem(hitemChild)) != NULL);

  return (hitemChild != NULL);
}

HTREEITEM TreeUtils::AddGroup(CTreeCtrl *pTreeCtrl, const StringX &sxNewGroup,
                              bool &bAlreadyExists, const bool &bIsEmpty,
                              PathMap *pmapPaths2TreeItem)
{
  // Add a group at the end of path
  HTREEITEM ti = TVI_ROOT;
  HTREEITEM si;
  bAlreadyExists = true;
  if (!sxNewGroup.empty()) {
    CSecString path = sxNewGroup;
    CSecString s;
    StringX path2root(L"");
    do {
      s = GetPathElem(path);
      if (path2root.empty())
        path2root = (LPCWSTR)s;
      else
        path2root += sxDot + StringX(s);

      if (!ExistsInTree(pTreeCtrl, ti, s, si)) {
        ti = pTreeCtrl->InsertItem(s, ti, TVI_SORT);
        pTreeCtrl->SetItemImage(ti, CItemData::EI_GROUP, CItemData::EI_GROUP);
        bAlreadyExists = false;
      } else
        ti = si;
      if (pmapPaths2TreeItem != NULL)
        pmapPaths2TreeItem->insert(std::make_pair(path2root,ti));
    } while (!path.IsEmpty());
  }
  if (bIsEmpty)
    pTreeCtrl->SetItemImage(ti, CItemData::EI_EMPTY_GROUP, CItemData::EI_EMPTY_GROUP);

  return ti;
}

void TreeUtils::ExpandAll(CTreeCtrl *pTreeCtrl)
{
  // Updated to test for zero entries!
  HTREEITEM hItem = pTreeCtrl->GetRootItem();
  if (hItem == NULL)
    return;

  pTreeCtrl->SetRedraw(FALSE);
  do {
    pTreeCtrl->Expand(hItem, TVE_EXPAND);
    hItem = pTreeCtrl->GetNextItem(hItem,TVGN_NEXTVISIBLE);
  } while (hItem);
  pTreeCtrl->EnsureVisible(pTreeCtrl->GetSelectedItem());
  pTreeCtrl->SetRedraw(TRUE);
}

void TreeUtils::CollapseAll(CTreeCtrl *pTreeCtrl)
{
  // Courtesy of Zafir Anjum from www.codeguru.com
  // Updated to test for zero entries!
  HTREEITEM hItem = pTreeCtrl->GetRootItem();
  if (hItem == NULL)
    return;

  pTreeCtrl->SetRedraw(FALSE);
  do {
    CollapseBranch(pTreeCtrl, hItem);
  } while ((hItem = pTreeCtrl->GetNextSiblingItem(hItem)) != NULL);
  pTreeCtrl->SetRedraw(TRUE);
}

void TreeUtils::CollapseBranch(CTreeCtrl *pTreeCtrl, HTREEITEM hItem)
{
  // Courtesy of Zafir Anjumfrom www.codeguru.com
  if (pTreeCtrl->ItemHasChildren(hItem)) {
    pTreeCtrl->Expand(hItem, TVE_COLLAPSE);
    hItem = pTreeCtrl->GetChildItem(hItem);
    do {
      CollapseBranch(pTreeCtrl, hItem);
    } while ((hItem = pTreeCtrl->GetNextSiblingItem(hItem)) != NULL);
  }
}
