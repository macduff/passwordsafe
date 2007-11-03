/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

#pragma once

// CPWFindToolBar

#include "ControlExtns.h"
#include <vector>

class CPWFindToolBar : public CToolBar
{
  DECLARE_DYNAMIC(CPWFindToolBar)

public:
  CPWFindToolBar();
  virtual ~CPWFindToolBar();

  void Init(const int NumBits, CWnd *pMessageWindow, int iWMSGID);
  void LoadDefaultToolBar(const int toolbarMode);
  void AddExtraControls();
  void ChangeImages(const int toolbarMode);
  void Reset();
  void ShowFindToolBar(bool bShow);
  void Enable(bool bEnable);
  bool IsVisible() {return m_bVisible;}
  bool IsEnabled() {return m_bEnabled;}
  void GetSearchText(CString &csFindString)
    {m_findedit.GetWindowText(csFindString);}
  void Find();
  void ClearFind();
  void ShowFindAdvanced();
  void ToggleToolBarFindCase();
  BOOL CPWFindToolBar::IsFindCaseSet()
  {return m_bCaseSensitive ? TRUE : FALSE;}

  CEditExtn m_findedit;
  CStatic m_findresults;

protected:
  //{{AFX_MSG(CPWFindToolBar)
  //}}AFX_MSG

  BOOL PreTranslateMessage(MSG* pMsg);

  DECLARE_MESSAGE_MAP()

private:
  static const UINT m_FindToolBarIDs[];
  static const UINT m_FindToolBarClassicBMs[];
  static const UINT m_FindToolBarNew8BMs[];
  static const UINT m_FindToolBarNew32BMs[];

  CImageList m_ImageList;
  TBBUTTON *m_pOriginalTBinfo;
  CWnd *m_pDbx;
  CFont m_FindTextFont;
  int m_iMaxNumButtons, m_iNum_Bitmaps;
  int m_iWMSGID;
  int m_toolbarMode, m_bitmode;
  UINT m_ClassicFlags, m_NewFlags;
  COLORREF m_ClassicBackground, m_NewBackground;
  bool m_bVisible, m_bEnabled, m_bCaseSensitive, m_bAdvanced;

  std::vector<int> m_indices; // array of found items

  bool m_cs_search, m_last_cs_search;
  CMyString	m_search_text, m_last_search_text;
  CItemData::FieldBits m_bsFields, m_last_bsFields;
  CString m_subgroup_name, m_last_subgroup_name;
  int m_subgroup_set, m_last_subgroup_set;
  int m_subgroup_object, m_last_subgroup_object;
  int m_subgroup_function, m_last_subgroup_function;

  size_t m_lastshown; // last index selected, -1 indicates no search done yet
  size_t m_numFound; // number of matched items, as returned by DboxMain::FindAll

  int m_iCase_Insensitive_BM_offset;
  bool m_bLastView;
};
