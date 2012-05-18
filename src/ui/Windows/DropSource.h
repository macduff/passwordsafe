/*
 *Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
 *All rights reserved. Use of the code is allowed under the
 *Artistic License 2.0 terms, as specified in the LICENSE file
 *distributed with this code, or available from
 *http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "afxole.h"
// Drag and Drop source

// Note values to stop D&D between instances where data is of different lengths
enum {
  FROMCC           = 0x00, // From Column Chooser Dialog
  FROMHDR          = 0x01, // From ListCtrl Header
  FROMTREE_L       = 0x02, // From TreeCtrl/Explorer views - left  mouse D&D
  FROMTREE_R       = 0x04, // From TreeCtrl/Explorer views - right mouse D&D
  FROMTREE_RSC     = 0x08, // From TreeCtrl/Explorer views - right mouse D&D - create Shortcut allowed

  FROMROW0         = 0x10, // From Explorer view - top splitter row - OR'd with other values
  FROMROW1         = 0x20  // From Explorer view - bottom splitter row - OR'd with other values
};

class CDataSource : protected COleDataSource
{
public:
  CDataSource();
  virtual ~CDataSource();
  virtual DROPEFFECT StartDragging(BYTE *szData, DWORD dwLength,
    CLIPFORMAT cpfmt, RECT *rClient, CPoint *ptMousePos);

protected:
  virtual void CompleteMove() {};
};
