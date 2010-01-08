/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// OptionsShortcuts.cpp : implementation file
//

#include "stdafx.h"
#include "Options_PropertySheet.h"

#if defined(POCKET_PC)
#include "pocketpc/resource.h"
#else
#include "resource.h"
#include "resource2.h"  // Menu   resources
#include "resource3.h"  // String resources
#endif

#include "OptionsShortcuts.h" // Must be after resource.h

#include <algorithm>

// COptionsShortcuts dialog

IMPLEMENT_DYNAMIC(COptionsShortcuts, COptions_PropertyPage)

COptionsShortcuts::COptionsShortcuts()
  : COptions_PropertyPage(COptionsShortcuts::IDD),
  m_bShortcutsChanged(false)
{
  //{{AFX_DATA_INIT(COptionsShortcuts)
  //}}AFX_DATA_INIT
}

COptionsShortcuts::~COptionsShortcuts()
{
}

void COptionsShortcuts::DoDataExchange(CDataExchange* pDX)
{
  COptions_PropertyPage::DoDataExchange(pDX);

  DDX_Control(pDX, IDC_SHORTCUTLIST, m_ShortcutLC);
  DDX_Control(pDX, IDC_STATIC_SHCTWARNING, m_stc_warning);
}

BEGIN_MESSAGE_MAP(COptionsShortcuts, COptions_PropertyPage)
  ON_WM_MEASUREITEM()
  ON_BN_CLICKED(IDC_RESETALLSHORTCUTS, OnBnClickedResetAll)
  ON_MESSAGE(PSM_QUERYSIBLINGS, OnQuerySiblings)
END_MESSAGE_MAP()

void COptionsShortcuts::InitialSetup(const MapMenuShortcuts MapMenuShortcuts,
                    const MapKeyNameID MapKeyNameID,
                    const std::vector<UINT> ExcludedMenuItems,
                    const std::vector<st_MenuShortcut> ReservedShortcuts)
{
  m_MapMenuShortcuts = MapMenuShortcuts;
  m_MapKeyNameID = MapKeyNameID;
  m_ExcludedMenuItems = ExcludedMenuItems;
  m_ReservedShortcuts = ReservedShortcuts;
}

BOOL COptionsShortcuts::OnInitDialog()
{
  BOOL brc;
  COptions_PropertyPage::OnInitDialog();

  m_ShortcutLC.Init(this);

  DWORD dwExtendedStyle = m_ShortcutLC.GetExtendedStyle() | LVS_EX_GRIDLINES;
  m_ShortcutLC.SetExtendedStyle(dwExtendedStyle);

  CString cs_colname;
  cs_colname.LoadString(IDS_COL_MENUITEM);
  m_ShortcutLC.InsertColumn(0, cs_colname);
  cs_colname.LoadString(IDS_COL_SHORTCUT);
  m_ShortcutLC.InsertColumn(1, cs_colname);

  MapMenuShortcutsIter iter, iter_parent;
  MapKeyNameIDConstIter citer;
  CString str;
  int iItem(0);

  for (iter = m_MapMenuShortcuts.begin(); iter != m_MapMenuShortcuts.end();
    iter++) {
    // We don't allow change of certain menu items
    // Just don't put in the list that the user sees.
    if (iter->second.uiParentID == 0)
      continue;

    if (std::find(m_ExcludedMenuItems.begin(),
                  m_ExcludedMenuItems.end(),
                  iter->first) != m_ExcludedMenuItems.end())
        continue;

    str = L"";
    if (iter->second.cVirtKey != 0) {
       st_KeyIDExt st_KIDEx;
       st_KIDEx.id = iter->second.cVirtKey;
       st_KIDEx.bExtended = (iter->second.cModifier & HOTKEYF_EXT) == HOTKEYF_EXT;
       citer = m_MapKeyNameID.find(st_KIDEx);
       if (citer != m_MapKeyNameID.end())
         str = CMenuShortcut::FormatShortcut(iter, citer);
    }

    // Remove the ampersand from the menu item the user sees here
    iter_parent = m_MapMenuShortcuts.find(iter->second.uiParentID);
    ASSERT(iter_parent != m_MapMenuShortcuts.end());
    CString sMenuItemtext = (CString(iter_parent->second.name.c_str()) + 
                             CString(L" \xbb ") +
                             CString(iter->second.name.c_str()));
    sMenuItemtext.Remove(L'&');
    iItem = m_ShortcutLC.InsertItem(iItem, sMenuItemtext);
    ASSERT(iItem != -1);
    brc = m_ShortcutLC.SetItemText(iItem, 1, str);
    ASSERT(brc != 0);
    DWORD dwData = MAKELONG(iter->first, iter->second.iMenuPosition);
    brc = m_ShortcutLC.SetItemData(iItem, dwData);
    ASSERT(brc != 0);
  } // foreach m_MapMenuShortcuts

  // Now sort via Menu item position
  brc = m_ShortcutLC.SortItems(CompareFunc, NULL);
  ASSERT(brc != 0);

  brc = m_ShortcutLC.SetColumnWidth(0, LVSCW_AUTOSIZE);
  ASSERT(brc != 0);
  brc = m_ShortcutLC.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
  ASSERT(brc != 0);

  brc = m_ShortcutLC.ModifyStyle(LVS_OWNERDRAWFIXED, 0, 0);
  ASSERT(brc != 0);

  CHeaderCtrl* pHCtrl;
  pHCtrl = m_ShortcutLC.GetHeaderCtrl();
  ASSERT(pHCtrl != NULL);
  pHCtrl->SetDlgCtrlID(IDC_SHORTCUTLC_HEADER);
  m_SHCTHeader.SubclassWindow(pHCtrl->GetSafeHwnd());
  m_SHCTHeader.SetStopChangeFlag(true);

  m_stc_warning.SetColour(RGB(255, 0, 0));
  m_stc_warning.ShowWindow(SW_HIDE);

  return TRUE;
}

void COptionsShortcuts::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
   // If this is our list control then increase height
   if (nIDCtl == IDC_SHORTCUTLIST) {
     lpMIS->itemHeight += 10;
   }
}

void COptionsShortcuts::OnBnClickedResetAll()
{
  MapMenuShortcutsIter iter;
  MapKeyNameIDConstIter citer;
  CString str;
  UINT id;
  st_KeyIDExt st_KIDEx;

  for (int i = 0; i < m_ShortcutLC.GetItemCount(); i++) {
    id = (UINT)LOWORD(m_ShortcutLC.GetItemData(i));

    iter = m_MapMenuShortcuts.find(id);
    st_KIDEx.id = iter->second.cdefVirtKey;
    st_KIDEx.bExtended = (iter->second.cModifier & HOTKEYF_EXT) == HOTKEYF_EXT;
    citer = m_MapKeyNameID.find(st_KIDEx);
    iter->second.cVirtKey = iter->second.cdefVirtKey;
    iter->second.cModifier = iter->second.cdefModifier;
  
    if (citer != m_MapKeyNameID.end() || iter->second.cdefVirtKey != 0) {
      str = CMenuShortcut::FormatShortcut(iter, citer);
    } else {
      str = L"";
    }
    m_ShortcutLC.SetItemText(i, 1, str);
  }

  ClearWarning();

  m_ShortcutLC.RedrawItems(0, m_ShortcutLC.GetItemCount());
  m_ShortcutLC.UpdateWindow();
  m_bShortcutsChanged = true;
}

// Functor for find_if to see if shortcut is reserved
struct reserved {
  reserved(st_MenuShortcut& st_mst) : m_st_mst(st_mst) {}
  bool operator()(st_MenuShortcut const& rdata) const
  {
    return (m_st_mst.cVirtKey  == rdata.cVirtKey &&
            m_st_mst.cModifier == rdata.cModifier);
  }

  st_MenuShortcut m_st_mst;
};

// Tortuous route to get here!
// m_HotKey looses focus and calls parent (CListCtrl) that calls here
void COptionsShortcuts::OnHotKeyKillFocus(const int item, const UINT id,
                                          const WORD wVirtualKeyCode, 
                                          const WORD wModifiers)
{
  CString str(L"");
  CString cs_warning;
  MapMenuShortcutsIter iter, inuse_iter;
  MapKeyNameIDConstIter citer;
  st_MenuShortcut st_mst;
  st_KeyIDExt st_KIDEx;

  st_mst.cVirtKey  = (unsigned char)wVirtualKeyCode;
  st_mst.cModifier = wVirtualKeyCode == 0 ? 0 : (unsigned char)wModifiers;

  st_KIDEx.id = (unsigned char)wVirtualKeyCode;
  st_KIDEx.bExtended = (wModifiers & HOTKEYF_EXT) == HOTKEYF_EXT;
  citer = m_MapKeyNameID.find(st_KIDEx);

  // Stop compiler complaining - put this here even if not needed
  already_inuse inuse(st_mst);

  if (citer == m_MapKeyNameID.end()) {
    // Invalid shortcut
    cs_warning.LoadString(IDS_SHCT_WARNING1);
    goto set_warning;
  }

  if (st_mst.cVirtKey != 0) {
    str = CMenuShortcut::FormatShortcut(st_mst, citer);
  }

  if (std::find_if(m_ReservedShortcuts.begin(),
                   m_ReservedShortcuts.end(),
                   reserved(st_mst)) != m_ReservedShortcuts.end()) {
    // Reserved shortcut ignored
    cs_warning.Format(IDS_SHCT_WARNING2, str);
    goto set_warning;
  }

  // Check not already in use (ignore if deleting current shortcut)
  iter = m_MapMenuShortcuts.find(id);
  if (st_mst.cVirtKey != (unsigned char)0) {
    inuse_iter = std::find_if(m_MapMenuShortcuts.begin(),
                              m_MapMenuShortcuts.end(),
                              inuse);
    if (inuse_iter != m_MapMenuShortcuts.end() && 
        inuse_iter->first != iter->first) {
      // Shortcut in use
      cs_warning.Format(IDS_SHCT_WARNING3, str, inuse_iter->second.name.c_str());
      goto set_warning;
    }
  }

  // Not reserved and not already in use - implement
  iter->second.cVirtKey = st_mst.cVirtKey;
  iter->second.cModifier = st_mst.cModifier;

  m_ShortcutLC.SetItemText(item, 1, str);
  m_ShortcutLC.RedrawItems(item, item);
  m_ShortcutLC.SetColumnWidth(0, LVSCW_AUTOSIZE);
  m_ShortcutLC.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
  m_ShortcutLC.UpdateWindow();
  m_bShortcutsChanged = true;
  return;

set_warning:
  m_stc_warning.SetWindowText(cs_warning);
  m_stc_warning.ShowWindow(SW_SHOW);
}

BOOL COptionsShortcuts::PreTranslateMessage(MSG* pMsg)
{
  // If HotKey active, allow Enter key to set instead of close
  // Property Page (OK button)
  if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN &&
      m_ShortcutLC.IsHotKeyActive()) {
    m_ShortcutLC.SaveHotKey();
    return TRUE;
  }

  return COptions_PropertyPage::PreTranslateMessage(pMsg);
}

LRESULT COptionsShortcuts::OnQuerySiblings(WPARAM wParam, LPARAM )
{
  UpdateData(TRUE);

  // Have any of my fields been changed?
  switch (wParam) {
    case PP_DATA_CHANGED:
      if (m_bShortcutsChanged) 
        return 1L;
      break;
    case PP_UPDATE_VARIABLES:
      // Since OnOK calls OnApply after we need to verify and/or
      // copy data into the entry - we do it ourselfs here first
      if (OnApply() == FALSE)
        return 1L;
  }
  return 0L;
}

int CALLBACK COptionsShortcuts::CompareFunc(LPARAM lParam1, LPARAM lParam2,
                                            LPARAM /* lParamSort */)
{
  // HIWORD is the menu position
  return (int)(HIWORD(lParam1) - HIWORD(lParam2));
}
