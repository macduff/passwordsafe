/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "SecString.h"

#include "WindowsDefs.h"

#include <map>

namespace TreeUtils {
  // namespace of common tree utility functions

  HTREEITEM GetNextTreeItem(CTreeCtrl *pTreeCtrl, HTREEITEM hItem);
  HTREEITEM FindItem(CTreeCtrl *pTreeCtrl, const CString &path,
                     const HTREEITEM hRoot);
  CString GetGroupFullPath(CTreeCtrl *pTreeCtrl, HTREEITEM hItem);
  CSecString GetPathElem(CSecString &path);
  HTREEITEM AddGroup(CTreeCtrl *pTreeCtrl, const StringX &sxNewGroup,
                     bool &bAlreadyExists, const bool &bIsEmpty = false,
                     PathMap *pmapPaths2TreeItem = NULL);
  bool ExistsInTree(CTreeCtrl *pTreeCtrl, const HTREEITEM &node,
                    const CSecString &s, HTREEITEM &si);

  bool SplitLeafText(const wchar_t *lt, StringX &newTitle,
                     StringX &newUser, StringX &newPassword);

  bool IsLeaf(CTreeCtrl *pTreeCtrl, const HTREEITEM ti);
  bool IsChildNodeOf(CTreeCtrl *pTreeCtrl,
                     HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent);
  
  void ExpandAll(CTreeCtrl *pTreeCtrl);
  void CollapseAll(CTreeCtrl *pTreeCtrl);
  void CollapseBranch(CTreeCtrl *pTreeCtrl, HTREEITEM hItem);
}
