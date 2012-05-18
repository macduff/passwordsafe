/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "CWTreeCtrl.h"
#include "TreeUtils.h"
#include "Images.h"

#include "core/ItemData.h"
#include "core/PWSprefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern StringX sxDot;

CCWTreeCtrl::CCWTreeCtrl()
{
}

CCWTreeCtrl::~CCWTreeCtrl()
{
}

BEGIN_MESSAGE_MAP(CCWTreeCtrl, CTreeCtrl)
  //{{AFX_MSG_MAP(CCWTreeCtrl)
  ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCWTreeCtrl::OnBeginLabelEdit(NMHDR *, LRESULT *pLResult)
{
  *pLResult = TRUE; // TRUE cancels label editing
}

CSecString CCWTreeCtrl::MakeTreeDisplayString(const CItemData &ci) const
{
  CSecString treeDispString = ci.GetTitle();

  treeDispString += L" [";
  treeDispString += ci.GetUser();
  treeDispString += L"]";

  if (ci.IsProtected())
    treeDispString += L" #";

  return treeDispString;
}
