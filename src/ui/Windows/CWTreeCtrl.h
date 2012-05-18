/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#pragma once

#include "SecString.h"

#include "core/ItemData.h"

class CCWTreeCtrl : public CTreeCtrl
{
public:
  CCWTreeCtrl();
  ~CCWTreeCtrl();

  CSecString MakeTreeDisplayString(const CItemData &ci) const;

protected:
  //{{AFX_MSG(CCWTreeCtrl)
  afx_msg void OnBeginLabelEdit(NMHDR *pNotifyStruct, LRESULT *pLResult);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()
};
