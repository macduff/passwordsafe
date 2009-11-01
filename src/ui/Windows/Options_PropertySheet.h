/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "PWPropertySheet.h"
#include "ControlExtns.h"

class COptions_PropertySheet : public CPWPropertySheet
{
public:
  COptions_PropertySheet(UINT nID, CWnd* pDbx);
  ~COptions_PropertySheet();

  DECLARE_DYNAMIC(COptions_PropertySheet)
};
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
