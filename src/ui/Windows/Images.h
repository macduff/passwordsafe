/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// Images.h
//-----------------------------------------------------------------------------

#pragma once

#include "core/ItemData.h"

class Images
{
public:
  static Images *GetInstance(); // singleton
  static void DeleteInstance();

  CImageList *GetImageList() {return m_pImageList;}
  CImageList *GetImageList0() {return m_pImageList0;}

private:
  Images();
  ~Images() {}
  static Images *self; // singleton

  static CImageList *m_pImageList, *m_pImageList0;
};

//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
