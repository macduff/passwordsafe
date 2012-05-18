/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

// CExplorerToolBar - Explorer toolbar

#include "WindowsDefs.h"

#include "XTFlatComboBox.h"

class DboxMain;

class CExplorerToolBar : public CToolBar
{
  DECLARE_DYNAMIC(CExplorerToolBar)

public:
  CExplorerToolBar();
  virtual ~CExplorerToolBar();

  void Init(const int NumBits, const SplitterRow &iRow, CWnd *pMessageWindow);
  void LoadDefaultToolBar();
  void AddExtraControls();
  void UpdatePathComboBox(std::vector<StringX> &vPath, int index);
  void ChangeIndex(const int iValue, const bool bDelete = false);

protected:
  //{{AFX_MSG(CExplorerToolBar)
  afx_msg void OnDestroy();
  afx_msg void OnPathComboChanged();
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
  static const UINT m_ExplorerToolBarIDs0[];
  static const UINT m_ExplorerToolBarIDs1[];
  static const UINT m_ExplorerToolBarBMs[];

  CImageList m_ImageList;
  TBBUTTON *m_pOriginalTBinfo;
  DboxMain *m_pDbx;

  CXTFlatComboBox m_cbxCurrentPath;
  CFont m_PathFont;

  int m_iMaxNumButtons, m_iNum_Bitmaps, m_NumBits;
  int m_bitmode;
};
