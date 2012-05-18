/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// PWToolBar.cpp : implementation file
//

#include "stdafx.h"
#include "PWToolBar.h"
#include "resource.h"
#include "resource2.h"

#include <vector>
#include <map>
#include <algorithm>

// CPWToolBar

/*
  To add a new Toolbar button to this class:
  1. Design new bitmaps (1 x 'Classic', 1 x 'New' designs & 1 x 'New Disabled' 
     design).  All have a background colour of RGB(192, 192, 192).  Note: Create
     the 'New Disabled' from the 'New' by using a program to convert the bitmap
     to 8-bit greyscale.
  2. Add them to PaswordSafe.rc as BITMAPs
  3. Assign new resource Bitmap IDs to these i.e. "IDB_<new name>_CLASSIC",
     "IDB_<new name>_NEW" and "IDB_<new name>_NEW_D"
  4. Assign a new resource ID for the corresponding button e.g. 
     "ID_TOOLBUTTON_<new name>" or "ID_MENUITEM_<name>" if also on a Menu.
  5. Add the resource ID in the appropriate place in the m_MainToolBarDefinitions array
  6. Add the new bitmap IDs in the appropriate place in m_MainToolBarDefinitions, 
      - OR - m_OtherToolbarDefinitions, if not on the Toolbar but is on a menu.
  7. Add the new name in the appropriate place in the m_csMainButtons array (used 
     for customization/preferences and '~' represents a separator).
  8. Add the new resource ID ("ID_TOOLBUTTON_<new name>" or "ID_MENUITEM_<name>")
     in PasswordSafe.rc2 "Toolbar Tooltips" section as these are used during ToolBar
     customization to describe the button in the standard Customization dialog.
    
  NOTE: In message handlers, the toolbar control ALWAYS asks for information based 
  on the ORIGINAL configuration!!! This is not documented by MS.
  
*/

// The following is the Default toolbar up to HELP - buttons and separators.
// It should really be in PWSprefs but this is the only routine that uses it and
// it is best to keep it together.  These strings should NOT be translated to other
// languagues as they are used only in the configuration file.
// They should match m_MainToolBarIDs below.
// Note a separator is denoted by '~'
const CString CPWToolBar::m_csMainButtons[] = {
  L"new", L"open", L"close", L"save", L"~",
  L"copypassword", L"copyuser", L"copynotes", L"clearclipboard", L"~",
  L"autotype", L"browseurl", L"~",
  L"add", L"viewedit", L"~",
  L"delete", L"~",
  L"expandall", L"collapseall", L"~",
  L"options", L"~",
  L"help",
  // Optional (non-default) buttons next - MUST be in the same order
  // as the optional IDs in m_MainToolBarIDs
  L"exporttext", L"exportxml", L"importtext", L"importxml", 
  L"saveas", L"compare", L"merge", L"synchronize", L"undo", L"redo",
  L"passwordsubset", L"browse+autotype", L"runcommand", L"sendemail",
  L"listtree", L"find", L"viewreports", 
  L"applyfilters", L"clearfilters", L"setfilters", L"managefilters",
  L"addgroup", L"managepolicies"
};

const CPWToolBar::ToolbarDefinitions CPWToolBar::m_MainToolBarDefinitions[MAIN_TB_LAST] = {
  {ID_MENUITEM_NEW, IDB_NEW_CLASSIC, IDB_NEW_NEW, IDB_NEW_NEW_D},
  {ID_MENUITEM_OPEN, IDB_OPEN_CLASSIC, IDB_OPEN_NEW, IDB_OPEN_NEW_D},
  {ID_MENUITEM_CLOSE, IDB_CLOSE_CLASSIC, IDB_CLOSE_NEW, IDB_CLOSE_NEW_D},
  {ID_MENUITEM_SAVE, IDB_SAVE_CLASSIC, IDB_SAVE_NEW, IDB_SAVE_NEW_D},
  {ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR},
  {ID_MENUITEM_COPYPASSWORD, IDB_COPYPASSWORD_CLASSIC, IDB_COPYPASSWORD_NEW, IDB_COPYPASSWORD_NEW_D},
  {ID_MENUITEM_COPYUSERNAME, IDB_COPYUSER_CLASSIC, IDB_COPYUSER_NEW, IDB_COPYUSER_NEW_D},
  {ID_MENUITEM_COPYNOTESFLD, IDB_COPYNOTES_CLASSIC, IDB_COPYNOTES_NEW, IDB_COPYNOTES_NEW_D},
  {ID_MENUITEM_CLEARCLIPBOARD, IDB_CLEARCLIPBOARD_CLASSIC, IDB_CLEARCLIPBOARD_NEW, IDB_CLEARCLIPBOARD_NEW_D},
  {ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR},
  {ID_MENUITEM_AUTOTYPE, IDB_AUTOTYPE_CLASSIC, IDB_AUTOTYPE_NEW, IDB_AUTOTYPE_NEW_D},
  {ID_MENUITEM_BROWSEURL, IDB_BROWSEURL_CLASSIC, IDB_BROWSEURL_NEW, IDB_BROWSEURL_NEW_D},
  {ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR},
  {ID_MENUITEM_ADD, IDB_ADD_CLASSIC, IDB_ADD_NEW, IDB_ADD_NEW_D},
  {ID_MENUITEM_EDIT, IDB_VIEWEDIT_CLASSIC, IDB_VIEWEDIT_NEW, IDB_VIEWEDIT_NEW_D},
  {ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR},
  {ID_MENUITEM_DELETEENTRY, IDB_DELETE_CLASSIC, IDB_DELETE_NEW, IDB_DELETE_NEW_D},
  {ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR},
  {ID_MENUITEM_EXPANDALL, IDB_EXPANDALL_CLASSIC, IDB_EXPANDALL_NEW, IDB_EXPANDALL_NEW_D},
  {ID_MENUITEM_COLLAPSEALL, IDB_COLLAPSEALL_CLASSIC, IDB_COLLAPSEALL_NEW, IDB_COLLAPSEALL_NEW_D},
  {ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR},
  {ID_MENUITEM_OPTIONS, IDB_OPTIONS_CLASSIC, IDB_OPTIONS_NEW, IDB_OPTIONS_NEW_D},
  {ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR},
  {ID_HELP, IDB_HELP_CLASSIC, IDB_HELP_NEW, IDB_HELP_NEW_D},
   // End of Default Toolbar
   // Following are not in the "default" toolbar but can be selected by the user
  {ID_MENUITEM_EXPORT2PLAINTEXT, IDB_EXPORTTEXT_CLASSIC, IDB_EXPORTTEXT_NEW, IDB_EXPORTTEXT_NEW_D},
  {ID_MENUITEM_EXPORT2XML, IDB_EXPORTXML_CLASSIC, IDB_EXPORTXML_NEW, IDB_EXPORTXML_NEW_D},
  {ID_MENUITEM_IMPORT_PLAINTEXT, IDB_IMPORTTEXT_CLASSIC, IDB_IMPORTTEXT_NEW, IDB_IMPORTTEXT_NEW_D},
  {ID_MENUITEM_IMPORT_XML, IDB_IMPORTXML_CLASSIC, IDB_IMPORTXML_NEW, IDB_IMPORTXML_NEW_D},
  {ID_MENUITEM_SAVEAS, IDB_SAVEAS_CLASSIC, IDB_SAVEAS_NEW, IDB_SAVEAS_NEW_D},
  {ID_MENUITEM_COMPARE, IDB_COMPARE_CLASSIC, IDB_COMPARE_NEW, IDB_COMPARE_NEW_D},
  {ID_MENUITEM_MERGE, IDB_MERGE_CLASSIC, IDB_MERGE_NEW, IDB_MERGE_NEW_D},
  {ID_MENUITEM_SYNCHRONIZE, IDB_SYNCHRONIZE_CLASSIC, IDB_SYNCHRONIZE_NEW, IDB_SYNCHRONIZE_NEW_D},
  {ID_MENUITEM_UNDO, IDB_UNDO_CLASSIC, IDB_UNDO_NEW, IDB_UNDO_NEW_D},
  {ID_MENUITEM_REDO, IDB_REDO_CLASSIC, IDB_REDO_NEW, IDB_REDO_NEW_D},
  {ID_MENUITEM_PASSWORDSUBSET, IDB_PASSWORDCHARS_CLASSIC, IDB_PASSWORDCHARS_NEW, IDB_PASSWORDCHARS_NEW_D},
  {ID_MENUITEM_BROWSEURLPLUS, IDB_BROWSEURLPLUS_CLASSIC, IDB_BROWSEURLPLUS_NEW, IDB_BROWSEURLPLUS_NEW_D},
  {ID_MENUITEM_RUNCOMMAND, IDB_RUNCMD_CLASSIC, IDB_RUNCMD_NEW, IDB_RUNCMD_NEW_D},
  {ID_MENUITEM_SENDEMAIL, IDB_SENDEMAIL_CLASSIC, IDB_SENDEMAIL_NEW, IDB_SENDEMAIL_NEW_D},
  {ID_TOOLBUTTON_LISTTREE, IDB_LISTTREE_CLASSIC, IDB_LISTTREE_NEW, IDB_LISTTREE_NEW_D},
  {ID_MENUITEM_SHOWFINDTOOLBAR, IDB_FIND_CLASSIC, IDB_FIND_NEW, IDB_FIND_NEW_D},
  {ID_TOOLBUTTON_VIEWREPORTS, IDB_VIEWREPORTS_CLASSIC, IDB_VIEWREPORTS_NEW, IDB_VIEWREPORTS_NEW_D},
  {ID_MENUITEM_APPLYFILTER, IDB_APPLYFILTERS_CLASSIC, IDB_APPLYFILTERS_NEW, IDB_APPLYFILTERS_NEW_D},
  {ID_MENUITEM_CLEARFILTER, IDB_CLEARFILTERS_CLASSIC, IDB_CLEARFILTERS_NEW, IDB_CLEARFILTERS_NEW_D},
  {ID_MENUITEM_EDITFILTER, IDB_SETFILTERS_CLASSIC, IDB_SETFILTERS_NEW, IDB_SETFILTERS_NEW_D},
  {ID_MENUITEM_MANAGEFILTERS, IDB_MANAGEFILTERS_CLASSIC, IDB_MANAGEFILTERS_NEW, IDB_MANAGEFILTERS_NEW_D},
  {ID_MENUITEM_ADDGROUP, IDB_ADDGROUP_CLASSIC, IDB_ADDGROUP_NEW, IDB_ADDGROUP_NEW_D},
  {ID_MENUITEM_PSWD_POLICIES, IDB_PSWD_POLICIES_CLASSIC, IDB_PSWD_POLICIES_NEW, IDB_PSWD_POLICIES_NEW_D}
};

// Additional Control IDs not on ToolBar
const CPWToolBar::ToolbarDefinitions CPWToolBar::m_OtherToolbarDefinitions[OTHER_TB_LAST] = {
  {ID_MENUITEM_PROPERTIES, IDB_PROPERTIES_CLASSIC, IDB_PROPERTIES_NEW, IDB_PROPERTIES_NEW_D},
  {ID_MENUITEM_GROUPENTER, IDB_GROUPENTER_CLASSIC, IDB_GROUPENTER_NEW, IDB_GROUPENTER_NEW_D},
  {ID_MENUITEM_DUPLICATEENTRY, IDB_DUPLICATE_CLASSIC, IDB_DUPLICATE_NEW, IDB_DUPLICATE_NEW_D},
  {ID_CHANGEFONTMENU, IDB_CHANGEFONTMENU_CLASSIC, IDB_CHANGEFONTMENU_NEW, IDB_CHANGEFONTMENU_NEW_D},
  {ID_MENUITEM_CHANGETREEFONT, IDB_CHANGEFONTMENU_CLASSIC, IDB_CHANGEFONTMENU_NEW, IDB_CHANGEFONTMENU_NEW_D},
  {ID_MENUITEM_CHANGEPSWDFONT, IDB_CHANGEPSWDFONTMENU_CLASSIC, IDB_CHANGEPSWDFONTMENU_NEW, IDB_CHANGEPSWDFONTMENU_NEW_D},
  {ID_MENUITEM_REPORT_COMPARE, IDB_COMPARE_CLASSIC, IDB_COMPARE_NEW, IDB_COMPARE_NEW_D},
  {ID_MENUITEM_REPORT_FIND, IDB_FIND_CLASSIC, IDB_FIND_NEW, IDB_FIND_NEW_D},
  {ID_MENUITEM_REPORT_IMPORTTEXT, IDB_IMPORTTEXT_CLASSIC, IDB_IMPORTTEXT_NEW, IDB_IMPORTTEXT_NEW_D},
  {ID_MENUITEM_REPORT_IMPORTXML, IDB_IMPORTXML_CLASSIC, IDB_IMPORTXML_NEW, IDB_IMPORTXML_NEW_D},
  {ID_MENUITEM_REPORT_MERGE, IDB_MERGE_CLASSIC, IDB_MERGE_NEW, IDB_MERGE_NEW_D},
  {ID_MENUITEM_REPORT_SYNCHRONIZE, IDB_SYNCHRONIZE_CLASSIC, IDB_SYNCHRONIZE_NEW, IDB_SYNCHRONIZE_NEW_D},
  {ID_MENUITEM_REPORT_VALIDATE, IDB_VALIDATE_CLASSIC, IDB_VALIDATE_NEW, IDB_VALIDATE_NEW_D},
  {ID_MENUITEM_CHANGECOMBO, IDB_CHANGECOMBO_CLASSIC, IDB_CHANGECOMBO_NEW, IDB_CHANGECOMBO_NEW_D},
  {ID_MENUITEM_BACKUPSAFE, IDB_BACKUPSAFE_CLASSIC, IDB_BACKUPSAFE_NEW, IDB_BACKUPSAFE_NEW_D},
  {ID_MENUITEM_RESTORESAFE, IDB_RESTORE_CLASSIC, IDB_RESTORE_NEW, IDB_RESTORE_NEW_D},
  {ID_MENUITEM_EXIT, IDB_EXIT_CLASSIC, IDB_EXIT_NEW, IDB_EXIT_NEW_D},
  {ID_MENUITEM_ABOUT, IDB_ABOUT_CLASSIC, IDB_ABOUT_NEW, IDB_ABOUT_NEW_D},
  {ID_MENUITEM_TRAYUNLOCK, IDB_TRAYUNLOCK_CLASSIC, IDB_TRAYUNLOCK_NEW, IDB_TRAYUNLOCK_NEW_D},
  {ID_MENUITEM_TRAYLOCK, IDB_TRAYLOCK_CLASSIC, IDB_TRAYLOCK_NEW, IDB_TRAYLOCK_NEW_D},
  {ID_REPORTSMENU, IDB_VIEWREPORTS_CLASSIC, IDB_VIEWREPORTS_NEW, IDB_VIEWREPORTS_NEW_D},
  {ID_MENUITEM_MRUENTRY, IDB_PWSDB, IDB_PWSDB, IDB_PWSDB},
  {ID_EXPORTMENU, IDB_EXPORT_CLASSIC, IDB_EXPORT_NEW, IDB_EXPORT_NEW_D},
  {ID_IMPORTMENU, IDB_IMPORT_CLASSIC, IDB_IMPORT_NEW, IDB_IMPORT_NEW_D},
  {ID_MENUITEM_CREATESHORTCUT, IDB_CREATESHORTCUT_CLASSIC, IDB_CREATESHORTCUT_NEW, IDB_CREATESHORTCUT_NEW_D},
  {ID_MENUITEM_CUSTOMIZETOOLBAR, IDB_CUSTOMIZETBAR_CLASSIC, IDB_CUSTOMIZETBAR_NEW, IDB_CUSTOMIZETBAR_NEW},
  {ID_MENUITEM_VKEYBOARDFONT, IDB_CHANGEVKBDFONTMENU_CLASSIC, IDB_CHANGEVKBDFONTMENU_NEW, IDB_CHANGEVKBDFONTMENU_NEW_D},
  {ID_MENUITEM_COMPVIEWEDIT, IDB_VIEWEDIT_CLASSIC, IDB_VIEWEDIT_NEW, IDB_VIEWEDIT_NEW_D},
  {ID_MENUITEM_COPY_TO_ORIGINAL, IDB_IMPORT_CLASSIC, IDB_IMPORT_NEW, IDB_IMPORT_NEW_D},
  {ID_MENUITEM_EXPORTENT2PLAINTEXT, IDB_EXPORTTEXT_CLASSIC, IDB_EXPORTTEXT_NEW, IDB_EXPORTTEXT_NEW_D},
  {ID_MENUITEM_EXPORTENT2XML, IDB_EXPORTXML_CLASSIC, IDB_EXPORTXML_NEW, IDB_EXPORTXML_NEW_D},
  {ID_MENUITEM_DUPLICATEGROUP, IDB_DUPLICATEGROUP_CLASSIC, IDB_DUPLICATEGROUP_NEW, IDB_DUPLICATEGROUP_NEW_D},
  {ID_MENUITEM_REPORT_EXPORTTEXT, IDB_EXPORTTEXT_CLASSIC, IDB_EXPORTTEXT_NEW, IDB_EXPORTTEXT_NEW_D},
  {ID_MENUITEM_REPORT_EXPORTXML, IDB_EXPORTXML_CLASSIC, IDB_EXPORTXML_NEW, IDB_EXPORTXML_NEW_D},
  {ID_MENUITEM_COPYALL_TO_ORIGINAL, IDB_IMPORT_CLASSIC, IDB_IMPORT_NEW, IDB_IMPORT_NEW_D},
  {ID_MENUITEM_SYNCHRONIZEALL, IDB_IMPORT_CLASSIC, IDB_IMPORT_NEW, IDB_IMPORT_NEW_D}
};

IMPLEMENT_DYNAMIC(CPWToolBar, CToolBar)

CPWToolBar::CPWToolBar()
:  m_bitmode(1), m_iBrowseURL_BM_offset(-1), m_iSendEmail_BM_offset(-1)
{
  m_iMaxNumButtons = sizeof(m_MainToolBarDefinitions) / sizeof(m_MainToolBarDefinitions[0]);
  int iMainBitmaps = 0;
  for (int i = 0; i < m_iMaxNumButtons; i++) {
    if (m_MainToolBarDefinitions[i].ID != ID_SEPARATOR)
      iMainBitmaps++;
  }

  m_pOriginalTBinfo = new TBBUTTON[m_iMaxNumButtons];
  
  // Assumes 'Others' do not have separators!
  m_iNum_Bitmaps = iMainBitmaps + sizeof(m_OtherToolbarDefinitions) / sizeof(m_OtherToolbarDefinitions[0]);
}

CPWToolBar::~CPWToolBar()
{
  delete [] m_pOriginalTBinfo;
}

void CPWToolBar::OnDestroy()
{
  m_ImageLists[0].DeleteImageList();
  m_ImageLists[1].DeleteImageList();
  m_ImageLists[2].DeleteImageList();
  m_DisabledImageLists[0].DeleteImageList();
  m_DisabledImageLists[1].DeleteImageList();
}

BEGIN_MESSAGE_MAP(CPWToolBar, CToolBar)
  ON_NOTIFY_REFLECT(TBN_GETBUTTONINFO, OnToolBarGetButtonInfo)
  ON_NOTIFY_REFLECT(TBN_QUERYINSERT, OnToolBarQueryInsert)
  ON_NOTIFY_REFLECT(TBN_QUERYDELETE, OnToolBarQueryDelete)
  ON_NOTIFY_REFLECT(TBN_GETBUTTONINFO, OnToolBarQueryInfo)
  ON_NOTIFY_REFLECT(TBN_RESET, OnToolBarReset)
  ON_WM_DESTROY()
END_MESSAGE_MAP()

// CPWToolBar message handlers

void CPWToolBar::RefreshImages()
{
  m_ImageLists[0].DeleteImageList();
  m_ImageLists[1].DeleteImageList();
  m_ImageLists[2].DeleteImageList();
  m_DisabledImageLists[0].DeleteImageList();
  m_DisabledImageLists[1].DeleteImageList();

  Init(m_NumBits, true);

  ChangeImages(m_toolbarMode);
}

void CPWToolBar::OnToolBarQueryInsert(NMHDR *, LRESULT *pLResult)
{
  *pLResult = TRUE;
}

void CPWToolBar::OnToolBarQueryDelete(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  NMTOOLBAR* pNMToolbar = (NMTOOLBAR *)pNotifyStruct;

  if ((pNMToolbar->tbButton.idCommand != ID_SEPARATOR) &&
    GetToolBarCtrl().IsButtonHidden(pNMToolbar->tbButton.idCommand))
    *pLResult = FALSE;
  else
    *pLResult = TRUE;
}

void CPWToolBar::OnToolBarQueryInfo(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  NMTOOLBAR* pNMToolbar = (NMTOOLBAR *)pNotifyStruct;

  ASSERT(pNMToolbar->iItem < m_iMaxNumButtons);

  if ((pNMToolbar->iItem >= 0) &&
    (pNMToolbar->iItem < m_iMaxNumButtons)) {
      pNMToolbar->tbButton = m_pOriginalTBinfo[pNMToolbar->iItem];
      *pLResult = TRUE;
  } else {
    *pLResult = FALSE;
  }
}

void CPWToolBar::OnToolBarGetButtonInfo(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  NMTOOLBAR* pNMToolbar = (NMTOOLBAR *)pNotifyStruct;

  ASSERT(pNMToolbar->iItem <= m_iMaxNumButtons);

  // if the index is valid
  if ((pNMToolbar->iItem >= 0) && (pNMToolbar->iItem < m_iMaxNumButtons)) {
    // copy the stored button structure
    pNMToolbar->tbButton = m_pOriginalTBinfo[pNMToolbar->iItem];
    *pLResult = TRUE;
  } else {
    *pLResult = FALSE;
  }
}

void CPWToolBar::OnToolBarReset(NMHDR *, LRESULT *)
{
  Reset();
}

//  Other routines

void CPWToolBar::Init(const int NumBits, bool bRefresh)
{
  int i, j;
  const UINT iClassicFlags = ILC_MASK | ILC_COLOR8;
  const UINT iNewFlags1 = ILC_MASK | ILC_COLOR8;
  const UINT iNewFlags2 = ILC_MASK | ILC_COLOR24;

  m_NumBits = NumBits;

  if (NumBits >= 32) {
    m_bitmode = 2;
  }

  m_ImageLists[0].Create(16, 16, iClassicFlags, m_iNum_Bitmaps, 2);
  m_ImageLists[1].Create(16, 16, iNewFlags1, m_iNum_Bitmaps, 2);
  m_ImageLists[2].Create(16, 16, iNewFlags2, m_iNum_Bitmaps, 2);
  m_DisabledImageLists[0].Create(16, 16, iNewFlags1, m_iNum_Bitmaps, 2);
  m_DisabledImageLists[1].Create(16, 16, iNewFlags2, m_iNum_Bitmaps, 2);

  int iNum_Others  = _countof(m_OtherToolbarDefinitions);
  int iNum_Bitmaps = m_iNum_Bitmaps - iNum_Others;

  for (i = 0; i < iNum_Bitmaps; i++) {
    if (m_MainToolBarDefinitions[i].CLASSIC_BMP == IDB_BROWSEURL_CLASSIC) {
      m_iBrowseURL_BM_offset = i;
      break;
    }
  }

  m_iSendEmail_BM_offset = iNum_Bitmaps;  // First of the "Others"

  // Setup Classic images
  SetupImageList(&m_MainToolBarDefinitions[0], CLASSIC_TYPE, MAIN_TB_LAST, 0);
  SetupImageList(&m_OtherToolbarDefinitions[0], CLASSIC_TYPE, OTHER_TB_LAST, 0);

  // Setup New images - 8-bit
  SetupImageList(&m_MainToolBarDefinitions[0], NEW_TYPE, MAIN_TB_LAST, 1);
  SetupImageList(&m_OtherToolbarDefinitions[0], NEW_TYPE, OTHER_TB_LAST, 1);

  // Setup New images - 32-bit
  SetupImageList(&m_MainToolBarDefinitions[0], NEW_TYPE, MAIN_TB_LAST, 2);
  SetupImageList(&m_OtherToolbarDefinitions[0], NEW_TYPE, OTHER_TB_LAST, 2);

  if (bRefresh)
    return;

  j = 0;
  m_csDefaultButtonString.Empty();
  m_iNumDefaultButtons = m_iMaxNumButtons;
  for (i = 0; i < m_iMaxNumButtons; i++) {
    const bool bIsSeparator = m_MainToolBarDefinitions[i].ID == ID_SEPARATOR;
    BYTE fsStyle = bIsSeparator ? TBSTYLE_SEP : TBSTYLE_BUTTON;
    fsStyle &= ~BTNS_SHOWTEXT;
    if (!bIsSeparator) {
      fsStyle |= TBSTYLE_AUTOSIZE;
    }
    m_pOriginalTBinfo[i].iBitmap = bIsSeparator ? -1 : j;
    m_pOriginalTBinfo[i].idCommand = m_MainToolBarDefinitions[i].ID;
    m_pOriginalTBinfo[i].fsState = TBSTATE_ENABLED;
    m_pOriginalTBinfo[i].fsStyle = fsStyle;
    m_pOriginalTBinfo[i].dwData = 0;
    m_pOriginalTBinfo[i].iString = bIsSeparator ? -1 : j;

    if (i <= m_iNumDefaultButtons)
      m_csDefaultButtonString += m_csMainButtons[i] + L" ";

    if (m_MainToolBarDefinitions[i].ID == ID_HELP)
      m_iNumDefaultButtons = i;

    if (!bIsSeparator)
      j++;
  }
}

void CPWToolBar::CustomizeButtons(CString csButtonNames)
{
  if (csButtonNames.IsEmpty()) {
    // Add all buttons
    Reset();
    return;
  }

  int i, nCount;
  CToolBarCtrl& tbCtrl = GetToolBarCtrl();

  // Remove all of the existing buttons
  nCount = tbCtrl.GetButtonCount();

  for (i = nCount - 1; i >= 0; i--) {
    tbCtrl.DeleteButton(i);
  }

  std::vector<CString> vcsButtonNameArray;

  csButtonNames.MakeLower();

  for (i = 0; i < m_iMaxNumButtons; i++) {
    vcsButtonNameArray.push_back(m_csMainButtons[i]);
  }

  std::vector<CString>::const_iterator cstring_iter;

  int curPos(0);
  // Note all separators will be treated as the first!
  i = 0;
  CString csToken = csButtonNames.Tokenize(L" ", curPos);
  while (csToken != L"" && curPos != -1) {
    cstring_iter = std::find(vcsButtonNameArray.begin(), vcsButtonNameArray.end(), csToken);
    if (cstring_iter != vcsButtonNameArray.end()) {
      int index = (int)(cstring_iter - vcsButtonNameArray.begin());
      tbCtrl.AddButtons(1, &m_pOriginalTBinfo[index]);
    }
    csToken = csButtonNames.Tokenize(L" ", curPos);
  }

  tbCtrl.AutoSize();
}

CString CPWToolBar::GetButtonString()
{
  CString cs_buttonnames(L"");
  TBBUTTONINFO tbinfo;
  int num_buttons, i;

  CToolBarCtrl& tbCtrl = GetToolBarCtrl();

  num_buttons = tbCtrl.GetButtonCount();

  std::vector<UINT> vcsButtonIDArray;
  std::vector<UINT>::const_iterator uint_iter;

  for (i = 0; i < m_iMaxNumButtons; i++) {
    vcsButtonIDArray.push_back(m_MainToolBarDefinitions[i].ID);
  }

  SecureZeroMemory(&tbinfo, sizeof(tbinfo));
  tbinfo.cbSize = sizeof(tbinfo);
  tbinfo.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE;

  for (i = 0; i < num_buttons; i++) {
    tbCtrl.GetButtonInfo(i, &tbinfo);

    if (tbinfo.fsStyle & TBSTYLE_SEP) {
      cs_buttonnames += L"~ ";
      continue;
    }

    uint_iter = std::find(vcsButtonIDArray.begin(), vcsButtonIDArray.end(), 
                          tbinfo.idCommand);
    if (uint_iter != vcsButtonIDArray.end()) {
      int index = (int)(uint_iter - vcsButtonIDArray.begin());
      cs_buttonnames += m_csMainButtons[index] + L" ";
    }
  }

  if (cs_buttonnames.CompareNoCase(m_csDefaultButtonString) == 0) {
    cs_buttonnames.Empty();
  }

  return cs_buttonnames;
}

void CPWToolBar::Reset()
{
  int nCount, i;
  CToolBarCtrl& tbCtrl = GetToolBarCtrl();

  // Remove all of the existing buttons
  nCount = tbCtrl.GetButtonCount();

  for (i = nCount - 1; i >= 0; i--) {
    tbCtrl.DeleteButton(i);
  }

  // Restore the buttons
  for (i = 0; i <= m_iNumDefaultButtons; i++) {
    tbCtrl.AddButtons(1, &m_pOriginalTBinfo[i]);
  }

  tbCtrl.AutoSize();
}

void CPWToolBar::ChangeImages(const int toolbarMode)
{
  CToolBarCtrl& tbCtrl = GetToolBarCtrl();
  m_toolbarMode = toolbarMode;
  const int nImageListNum = (m_toolbarMode == ID_MENUITEM_OLD_TOOLBAR) ? 0 : m_bitmode;
  tbCtrl.SetImageList(&m_ImageLists[nImageListNum]);
  // We only do the New toolbar disabled images.  MS can handle the Classic OK
  if (nImageListNum != 0)
    tbCtrl.SetDisabledImageList(&m_DisabledImageLists[nImageListNum - 1]);
  else
    tbCtrl.SetDisabledImageList(NULL);
}

void CPWToolBar::LoadDefaultToolBar(const int toolbarMode)
{
  int nCount, i, j;
  CToolBarCtrl& tbCtrl = GetToolBarCtrl();
  nCount = tbCtrl.GetButtonCount();

  for (i = nCount - 1; i >= 0; i--) {
    tbCtrl.DeleteButton(i);
  }

  m_toolbarMode = toolbarMode;
  const int nImageListNum = (m_toolbarMode == ID_MENUITEM_OLD_TOOLBAR) ? 0 : m_bitmode;
  tbCtrl.SetImageList(&m_ImageLists[nImageListNum]);
  // We only do the New toolbar disabled images.  MS can handle the Classic OK
  if (nImageListNum != 0)
    tbCtrl.SetDisabledImageList(&m_DisabledImageLists[nImageListNum - 1]);
  else
    tbCtrl.SetDisabledImageList(NULL);

  // Create text for customization dialog using button tooltips.
  // Assume no button tooltip description exceeds 64 characters, also m_iMaxNumButtons
  // includes separators which don't have strings giving an even bigger buffer!
  // Because they are a concatenation of null terminated strings terminated by a double
  // null, they cannot be stored in a CString variable,
  wchar_t *lpszTBCustomizationStrings = new wchar_t[m_iMaxNumButtons * 64];
  const int maxlength = m_iMaxNumButtons * 64;

  // By clearing, ensures string ends with a double NULL
  SecureZeroMemory(lpszTBCustomizationStrings, maxlength * sizeof(wchar_t));

  j = 0;
  for (i = 0; i < m_iMaxNumButtons; i++) {
    if (m_MainToolBarDefinitions[i].ID != ID_SEPARATOR) {
      CString cs_buttondesc;
      cs_buttondesc.LoadString(m_MainToolBarDefinitions[i].ID);
      int iPos = cs_buttondesc.ReverseFind(L'\n');
      if (iPos < 0) // could happen with incomplete translation
        continue;
      cs_buttondesc = cs_buttondesc.Right(cs_buttondesc.GetLength() - iPos - 1);
      int idesclen = cs_buttondesc.GetLength();
      wchar_t *szDescription = cs_buttondesc.GetBuffer(idesclen);
#if (_MSC_VER >= 1400)
      memcpy_s(&lpszTBCustomizationStrings[j], maxlength - j, szDescription, 
               idesclen * sizeof(wchar_t));
#else
      ASSERT((maxlength - j) > idesclen * sizeof(wchar_t));
      memcpy(&lpszTBCustomizationStrings[j], szDescription, idesclen * sizeof(wchar_t));
#endif
      cs_buttondesc.ReleaseBuffer();
      j += idesclen + 1;
    }
  }

  tbCtrl.AddStrings(lpszTBCustomizationStrings);
  tbCtrl.AddButtons(m_iMaxNumButtons, &m_pOriginalTBinfo[0]);

  delete [] lpszTBCustomizationStrings;

  DWORD dwStyle, dwStyleEx;
  dwStyle = tbCtrl.GetStyle();
  dwStyle &= ~TBSTYLE_AUTOSIZE;
  tbCtrl.SetStyle(dwStyle | TBSTYLE_LIST);

  dwStyleEx = tbCtrl.GetExtendedStyle();
  tbCtrl.SetExtendedStyle(dwStyleEx | TBSTYLE_EX_MIXEDBUTTONS);
}

void CPWToolBar::MapControlIDtoImage(ID2ImageMap &IDtoImages)
{
  int i, j(0);
  int iNum_ToolBarIDs = _countof(m_MainToolBarDefinitions);
  for (i = 0; i < iNum_ToolBarIDs; i++) {
    UINT ID = m_MainToolBarDefinitions[i].ID;
    if (ID == ID_SEPARATOR)
      continue;
    IDtoImages[ID] = j;
    j++;
  }

  int iNum_OtherIDs  = _countof(m_OtherToolbarDefinitions);
  for (i = 0; i < iNum_OtherIDs; i++) {
    UINT ID = m_OtherToolbarDefinitions[i].ID;
    IDtoImages[ID] = j;
    j++;
  }

  // Delete Group has same image as Delete Entry
  ID2ImageMapIter iter;
  iter = IDtoImages.find(ID_MENUITEM_DELETEENTRY);
  if (iter != IDtoImages.end()) {
    IDtoImages[ID_MENUITEM_DELETEGROUP] = iter->second;
  }
  // View has same image as Edit
  iter = IDtoImages.find(ID_MENUITEM_EDIT);
  if (iter != IDtoImages.end()) {
    IDtoImages[ID_MENUITEM_VIEW] = iter->second;
  }

  // special case, pending re-org:
  // Edit->Find... menu uses same bitmap as View->Show Find Toolbar
  IDtoImages[ID_MENUITEM_FINDELLIPSIS] = IDtoImages[ID_MENUITEM_SHOWFINDTOOLBAR];
}

void CPWToolBar::SetupImageList(const ToolbarDefinitions *pTBDefs, const ImageType iType,
                                const int num_entries, const int nImageList)
{
  const COLORREF crCOLOR_3DFACE = GetSysColor(COLOR_3DFACE);

  CBitmap bmNormal, bmDisabled;

  for (int i = 0; i < num_entries; i++) {
    if (pTBDefs[i].ID == ID_SEPARATOR)
      continue;

    UINT uiImage(0), uiImageD(0);
    switch (iType) {
      case CLASSIC_TYPE:
        uiImage = pTBDefs[i].CLASSIC_BMP;
        // uiImageD not needed in Classic view
        break;
      case NEW_TYPE:
        uiImage = pTBDefs[i].NEW_BMP;
        uiImageD = pTBDefs[i].NEW_DISABLED_BMP;
        break;
      default:
        ASSERT(0);
    }
    BOOL brc = bmNormal.Attach(
                    ::LoadImage(::AfxFindResourceHandle(MAKEINTRESOURCE(uiImage), RT_BITMAP),
                    MAKEINTRESOURCE(uiImage), IMAGE_BITMAP, 0, 0,
                    (LR_DEFAULTSIZE | LR_CREATEDIBSECTION)));
    ASSERT(brc);
    SetBitmapBackground(bmNormal, crCOLOR_3DFACE);
    m_ImageLists[nImageList].Add(&bmNormal, crCOLOR_3DFACE);
    bmNormal.DeleteObject();

    if (nImageList != 0) {
      bmDisabled.Attach(
              ::LoadImage(::AfxFindResourceHandle(MAKEINTRESOURCE(uiImageD), RT_BITMAP),
              MAKEINTRESOURCE(uiImageD), IMAGE_BITMAP, 0, 0,
              (LR_DEFAULTSIZE | LR_CREATEDIBSECTION)));
      SetBitmapBackground(bmDisabled, crCOLOR_3DFACE);
      m_DisabledImageLists[nImageList - 1].Add(&bmDisabled, crCOLOR_3DFACE);
      bmDisabled.DeleteObject();
    }
  }
}

void CPWToolBar::SetBitmapBackground(CBitmap &bm, const COLORREF newbkgrndColour)
{
  // Get how many pixels in the bitmap
  BITMAP bmInfo;
  bm.GetBitmap(&bmInfo);

  const UINT numPixels(bmInfo.bmHeight * bmInfo.bmWidth);

  // get a pointer to the pixels
  DIBSECTION ds;
  VERIFY(bm.GetObject(sizeof(DIBSECTION), &ds) == sizeof(DIBSECTION));

  RGBTRIPLE *pixels = reinterpret_cast<RGBTRIPLE*>(ds.dsBm.bmBits);
  ASSERT(pixels != NULL);

  const RGBTRIPLE newbkgrndColourRGB = {GetBValue(newbkgrndColour),
    GetGValue(newbkgrndColour),
    GetRValue(newbkgrndColour)};

  for (UINT i = 0; i < numPixels; ++i) {
    if (pixels[i].rgbtBlue == 192 &&
      pixels[i].rgbtGreen == 192 &&
      pixels[i].rgbtRed == 192) {
        pixels[i] = newbkgrndColourRGB;
    }
  }
}
