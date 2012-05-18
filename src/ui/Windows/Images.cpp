/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file Images.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Images.h"

#include "core/PwsPlatform.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Images *Images::self = NULL;

CImageList *Images::m_pImageList = NULL;
CImageList *Images::m_pImageList0 = NULL;

Images *Images::GetInstance()
{
  if (self == NULL) {
    self = new Images();
  }
  return self;
}

void Images::DeleteInstance()
{
  if (m_pImageList != NULL) {
    m_pImageList->DeleteImageList();
    delete m_pImageList;
    m_pImageList = NULL;
  }
  if (m_pImageList0 != NULL) {
    m_pImageList0->DeleteImageList();
    delete m_pImageList0;
    m_pImageList0 = NULL;
  }
  delete self;
  self = NULL;
}

Images::Images()
{
  // Coresponds to EntryImages in ItemData.h
  UINT bitmapResIDs[] = {IDB_GROUP,
    IDB_NORMAL, IDB_NORMAL_WARNEXPIRED, IDB_NORMAL_EXPIRED,
    IDB_ABASE, IDB_ABASE_WARNEXPIRED, IDB_ABASE_EXPIRED,
    IDB_ALIAS,
    IDB_SBASE, IDB_SBASE_WARNEXPIRED, IDB_SBASE_EXPIRED,
    IDB_SHORTCUT,
		IDB_EMPTY_GROUP,
    IDB_DATABASE
  };

  CBitmap bitmap;
  BITMAP bm;

  // Change all pixels in this 'grey' to transparent
  const COLORREF crTransparent = RGB(192, 192, 192);

  bitmap.LoadBitmap(IDB_GROUP);
  bitmap.GetBitmap(&bm);
  
  m_pImageList = new CImageList();
  BOOL status = m_pImageList->Create(bm.bmWidth, bm.bmHeight, 
                                     ILC_MASK | ILC_COLORDDB, 
                                     CItemData::EI_NUM_IMAGES, 0);
  ASSERT(status != 0);

  // Dummy Imagelist needed if user adds then removes Icon column
  m_pImageList0 = new CImageList();
  status = m_pImageList0->Create(1, 1, ILC_MASK | ILC_COLOR, 0, 1);
  ASSERT(status != 0);

  // Order of LoadBitmap() calls matches CPWTreeCtrl public enum
  // Also now used by CListCtrl!
  //bitmap.LoadBitmap(IDB_NODE); - already loaded above to get width
  m_pImageList->Add(&bitmap, crTransparent);
  bitmap.DeleteObject();

  for (int i = 1; i < CItemData::EI_NUM_IMAGES; i++) {
    bitmap.LoadBitmap(bitmapResIDs[i]);
    m_pImageList->Add(&bitmap, crTransparent);
    bitmap.DeleteObject();
  }
}
