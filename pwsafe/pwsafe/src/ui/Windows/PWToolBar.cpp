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

#include <map>
#include <algorithm>
#include <iterator>

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
  5. Add the resource ID in the appropriate place in the m_MainToolBarIDs array
  6. Add the new bitmap IDs in the appropriate place in m_MainToolBarClassicBMs, 
     m_MainToolBarNewBMs & m_MainToolBarNewDisBMs arrays (these should correspond
     to the position of the "resource ID" in step 4 (ignoring separators)) - OR -
     m_OtherClassicBMs, m_OtherNewBMs or m_OtherNewDisBMs arrays if not on the 
     Toolbar but is on a menu.
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

const UINT CPWToolBar::m_MainToolBarIDs[] = {
  ID_MENUITEM_NEW,
  ID_MENUITEM_OPEN,
  ID_MENUITEM_CLOSE,
  ID_MENUITEM_SAVE,
  ID_SEPARATOR,
  ID_MENUITEM_COPYPASSWORD,
  ID_MENUITEM_COPYUSERNAME,
  ID_MENUITEM_COPYNOTESFLD,
  ID_MENUITEM_CLEARCLIPBOARD,
  ID_SEPARATOR,
  ID_MENUITEM_AUTOTYPE,
  ID_MENUITEM_BROWSEURL,
  ID_SEPARATOR,
  ID_MENUITEM_ADD,
  ID_MENUITEM_EDIT,
  ID_SEPARATOR,
  ID_MENUITEM_DELETEENTRY,
  ID_SEPARATOR,
  ID_MENUITEM_EXPANDALL,
  ID_MENUITEM_COLLAPSEALL,
  ID_SEPARATOR,
  ID_MENUITEM_OPTIONS,
  ID_SEPARATOR,
  ID_HELP,
  // End of Default Toolbar
  // Following are not in the "default" toolbar but can be selected by the user
  ID_MENUITEM_EXPORT2PLAINTEXT,
  ID_MENUITEM_EXPORT2XML,
  ID_MENUITEM_IMPORT_PLAINTEXT,
  ID_MENUITEM_IMPORT_XML,
  ID_MENUITEM_SAVEAS,
  ID_MENUITEM_COMPARE,
  ID_MENUITEM_MERGE,
  ID_MENUITEM_SYNCHRONIZE,
  ID_MENUITEM_UNDO,
  ID_MENUITEM_REDO,
  ID_MENUITEM_PASSWORDSUBSET,
  ID_MENUITEM_BROWSEURLPLUS,
  ID_MENUITEM_RUNCOMMAND,
  ID_MENUITEM_SENDEMAIL,
  ID_TOOLBUTTON_LISTTREE,
  ID_MENUITEM_SHOWFINDTOOLBAR,
  ID_TOOLBUTTON_VIEWREPORTS,
  ID_MENUITEM_APPLYFILTER,
  ID_MENUITEM_CLEARFILTER,
  ID_MENUITEM_EDITFILTER,
  ID_MENUITEM_MANAGEFILTERS,
  ID_MENUITEM_ADDGROUP,
  ID_MENUITEM_PSWD_POLICIES
};

// Additional Control IDs not on ToolBar
const UINT CPWToolBar::m_OtherIDs[] = {
  ID_MENUITEM_PROPERTIES,
  ID_MENUITEM_GROUPENTER,
  ID_MENUITEM_DUPLICATEENTRY,
  ID_CHANGEFONTMENU,
  ID_MENUITEM_CHANGETREEFONT,
  ID_MENUITEM_CHANGEPSWDFONT,
  ID_MENUITEM_REPORT_COMPARE,
  ID_MENUITEM_REPORT_FIND,
  ID_MENUITEM_REPORT_IMPORTTEXT,
  ID_MENUITEM_REPORT_IMPORTXML,
  ID_MENUITEM_REPORT_MERGE,
  ID_MENUITEM_REPORT_SYNCHRONIZE,
  ID_MENUITEM_REPORT_VALIDATE,
  ID_MENUITEM_CHANGECOMBO,
  ID_MENUITEM_BACKUPSAFE,
  ID_MENUITEM_RESTORESAFE,
  ID_MENUITEM_EXIT,
  ID_MENUITEM_ABOUT,
  ID_MENUITEM_TRAYUNLOCK,
  ID_MENUITEM_TRAYLOCK,
  ID_REPORTSMENU,
  ID_MENUITEM_MRUENTRY,
  ID_EXPORTMENU,
  ID_IMPORTMENU,
  ID_MENUITEM_CREATESHORTCUT,
  ID_MENUITEM_CUSTOMIZETOOLBAR,
  ID_MENUITEM_VKEYBOARDFONT,
  ID_MENUITEM_COMPVIEWEDIT,
  ID_MENUITEM_COPY_TO_ORIGINAL,
  ID_MENUITEM_EXPORTENT2PLAINTEXT,
  ID_MENUITEM_EXPORTENT2XML,
  ID_MENUITEM_DUPLICATEGROUP,
  ID_MENUITEM_REPORT_EXPORTTEXT,
  ID_MENUITEM_REPORT_EXPORTXML,
  ID_MENUITEM_COPYALL_TO_ORIGINAL,
  ID_MENUITEM_SYNCHRONIZEALL
};

const UINT CPWToolBar::m_MainToolBarClassicBMs[] = {
  IDB_NEW_CLASSIC,
  IDB_OPEN_CLASSIC,
  IDB_CLOSE_CLASSIC,
  IDB_SAVE_CLASSIC,
  IDB_COPYPASSWORD_CLASSIC,
  IDB_COPYUSER_CLASSIC,
  IDB_COPYNOTES_CLASSIC,
  IDB_CLEARCLIPBOARD_CLASSIC,
  IDB_AUTOTYPE_CLASSIC,
  IDB_BROWSEURL_CLASSIC,
  IDB_ADD_CLASSIC,
  IDB_VIEWEDIT_CLASSIC,
  IDB_DELETE_CLASSIC,
  IDB_EXPANDALL_CLASSIC,
  IDB_COLLAPSEALL_CLASSIC,
  IDB_OPTIONS_CLASSIC,
  IDB_HELP_CLASSIC,
  // End of Default Toolbar
  // Following are not in the "default" toolbar but can be selected by the user
  IDB_EXPORTTEXT_CLASSIC,
  IDB_EXPORTXML_CLASSIC,
  IDB_IMPORTTEXT_CLASSIC,
  IDB_IMPORTXML_CLASSIC,
  IDB_SAVEAS_CLASSIC,
  IDB_COMPARE_CLASSIC,
  IDB_MERGE_CLASSIC,
  IDB_SYNCHRONIZE_CLASSIC,
  IDB_UNDO_CLASSIC,
  IDB_REDO_CLASSIC,
  IDB_PASSWORDCHARS_CLASSIC,
  IDB_BROWSEURLPLUS_CLASSIC,
  IDB_RUNCMD_CLASSIC,
  IDB_SENDEMAIL_CLASSIC,
  IDB_LISTTREE_CLASSIC,
  IDB_FIND_CLASSIC,
  IDB_VIEWREPORTS_CLASSIC,
  IDB_APPLYFILTERS_CLASSIC,
  IDB_CLEARFILTERS_CLASSIC,
  IDB_SETFILTERS_CLASSIC,
  IDB_MANAGEFILTERS_CLASSIC,
  IDB_ADDGROUP_CLASSIC,
  IDB_PSWD_POLICIES_CLASSIC
};

// Additional bitmaps not on ToolBar
const UINT CPWToolBar::m_OtherClassicBMs[] = {
  IDB_PROPERTIES_CLASSIC,
  IDB_GROUPENTER_CLASSIC,
  IDB_DUPLICATE_CLASSIC,
  IDB_CHANGEFONTMENU_CLASSIC,
  IDB_CHANGEFONTMENU_CLASSIC,
  IDB_CHANGEPSWDFONTMENU_CLASSIC,
  IDB_COMPARE_CLASSIC,      // For report of the same name
  IDB_FIND_CLASSIC,         // For report of the same name
  IDB_IMPORTTEXT_CLASSIC,   // For report of the same name
  IDB_IMPORTXML_CLASSIC,    // For report of the same name
  IDB_MERGE_CLASSIC,        // For report of the same name
  IDB_SYNCHRONIZE_CLASSIC,  // For report of the same name
  IDB_VALIDATE_CLASSIC,     // For report of the same name
  IDB_CHANGECOMBO_CLASSIC,
  IDB_BACKUPSAFE_CLASSIC,
  IDB_RESTORE_CLASSIC,
  IDB_EXIT_CLASSIC,
  IDB_ABOUT_CLASSIC,
  IDB_TRAYUNLOCK_CLASSIC,
  IDB_TRAYLOCK_CLASSIC,
  IDB_VIEWREPORTS_CLASSIC,
  IDB_PWSDB,
  IDB_EXPORT_CLASSIC,
  IDB_IMPORT_CLASSIC,
  IDB_CREATESHORTCUT_CLASSIC,
  IDB_CUSTOMIZETBAR_CLASSIC,
  IDB_CHANGEVKBDFONTMENU_CLASSIC,
  IDB_VIEWEDIT_CLASSIC,
  IDB_IMPORT_CLASSIC,
  IDB_EXPORTTEXT_CLASSIC,
  IDB_EXPORTXML_CLASSIC,
  IDB_DUPLICATEGROUP_CLASSIC,
  IDB_EXPORTTEXT_CLASSIC,   // For report of the same name
  IDB_EXPORTXML_CLASSIC,    // For report of the same name
  IDB_IMPORT_CLASSIC,
  IDB_IMPORT_CLASSIC,
};

const UINT CPWToolBar::m_MainToolBarNewBMs[] = {
  IDB_NEW_NEW,
  IDB_OPEN_NEW,
  IDB_CLOSE_NEW,
  IDB_SAVE_NEW,
  IDB_COPYPASSWORD_NEW,
  IDB_COPYUSER_NEW,
  IDB_COPYNOTES_NEW,
  IDB_CLEARCLIPBOARD_NEW,
  IDB_AUTOTYPE_NEW,
  IDB_BROWSEURL_NEW,
  IDB_ADD_NEW,
  IDB_VIEWEDIT_NEW,
  IDB_DELETE_NEW,
  IDB_EXPANDALL_NEW,
  IDB_COLLAPSEALL_NEW,
  IDB_OPTIONS_NEW,
  IDB_HELP_NEW,
  // End of Default Toolbar
  // Following are not in the "default" toolbar but can be selected by the user
  IDB_EXPORTTEXT_NEW,
  IDB_EXPORTXML_NEW,
  IDB_IMPORTTEXT_NEW,
  IDB_IMPORTXML_NEW,
  IDB_SAVEAS_NEW,
  IDB_COMPARE_NEW,
  IDB_MERGE_NEW,
  IDB_SYNCHRONIZE_NEW,
  IDB_UNDO_NEW,
  IDB_REDO_NEW,
  IDB_PASSWORDCHARS_NEW,
  IDB_BROWSEURLPLUS_NEW,
  IDB_RUNCMD_NEW,
  IDB_SENDEMAIL_NEW,
  IDB_LISTTREE_NEW,
  IDB_FIND_NEW,
  IDB_VIEWREPORTS_NEW,
  IDB_APPLYFILTERS_NEW,
  IDB_CLEARFILTERS_NEW,
  IDB_SETFILTERS_NEW,
  IDB_MANAGEFILTERS_NEW,
  IDB_ADDGROUP_NEW,
  IDB_PSWD_POLICIES_NEW
};

const UINT CPWToolBar::m_MainToolBarNewDisBMs[] = {
  IDB_NEW_NEW_D,
  IDB_OPEN_NEW_D,
  IDB_CLOSE_NEW_D,
  IDB_SAVE_NEW_D,
  IDB_COPYPASSWORD_NEW_D,
  IDB_COPYUSER_NEW_D,
  IDB_COPYNOTES_NEW_D,
  IDB_CLEARCLIPBOARD_NEW_D,
  IDB_AUTOTYPE_NEW_D,
  IDB_BROWSEURL_NEW_D,
  IDB_ADD_NEW_D,
  IDB_VIEWEDIT_NEW_D,
  IDB_DELETE_NEW_D,
  IDB_EXPANDALL_NEW_D,
  IDB_COLLAPSEALL_NEW_D,
  IDB_OPTIONS_NEW_D,
  IDB_HELP_NEW_D,
  // End of Default Toolbar
  // Following are not in the "default" toolbar but can be selected by the user
  IDB_EXPORTTEXT_NEW_D,
  IDB_EXPORTXML_NEW_D,
  IDB_IMPORTTEXT_NEW_D,
  IDB_IMPORTXML_NEW_D,
  IDB_SAVEAS_NEW_D,
  IDB_COMPARE_NEW_D,
  IDB_MERGE_NEW_D,
  IDB_SYNCHRONIZE_NEW_D,
  IDB_UNDO_NEW_D,
  IDB_REDO_NEW_D,
  IDB_PASSWORDCHARS_NEW_D,
  IDB_BROWSEURLPLUS_NEW_D,
  IDB_RUNCMD_NEW_D,
  IDB_SENDEMAIL_NEW_D,
  IDB_LISTTREE_NEW_D,
  IDB_FIND_NEW_D,
  IDB_VIEWREPORTS_NEW_D,
  IDB_APPLYFILTERS_NEW_D,
  IDB_CLEARFILTERS_NEW_D,
  IDB_SETFILTERS_NEW_D,
  IDB_MANAGEFILTERS_NEW_D,
  IDB_ADDGROUP_NEW_D,
  IDB_PSWD_POLICIES_NEW_D
};

// Additional bitmaps not on ToolBar
const UINT CPWToolBar::m_OtherNewBMs[] = {
  IDB_PROPERTIES_NEW,
  IDB_GROUPENTER_NEW,
  IDB_DUPLICATE_NEW,
  IDB_CHANGEFONTMENU_NEW,
  IDB_CHANGEFONTMENU_NEW,
  IDB_CHANGEPSWDFONTMENU_NEW,
  IDB_COMPARE_NEW,        // For report of the same name
  IDB_FIND_NEW,           // For report of the same name
  IDB_IMPORTTEXT_NEW,     // For report of the same name
  IDB_IMPORTXML_NEW,      // For report of the same name
  IDB_MERGE_NEW,          // For report of the same name
  IDB_SYNCHRONIZE_NEW,    // For report of the same name
  IDB_VALIDATE_NEW,       // For report of the same name
  IDB_CHANGECOMBO_NEW,
  IDB_BACKUPSAFE_NEW,
  IDB_RESTORE_NEW,
  IDB_EXIT_NEW,
  IDB_ABOUT_NEW,
  IDB_TRAYUNLOCK_NEW,
  IDB_TRAYLOCK_NEW,
  IDB_VIEWREPORTS_NEW,
  IDB_PWSDB,
  IDB_EXPORT_NEW,
  IDB_IMPORT_NEW,
  IDB_CREATESHORTCUT_NEW,
  IDB_CUSTOMIZETBAR_NEW,
  IDB_CHANGEVKBDFONTMENU_NEW,
  IDB_VIEWEDIT_NEW,
  IDB_IMPORT_NEW,
  IDB_EXPORTTEXT_NEW,
  IDB_EXPORTXML_NEW,
  IDB_DUPLICATEGROUP_NEW,
  IDB_EXPORTTEXT_NEW,     // For report of the same name
  IDB_EXPORTXML_NEW,      // For report of the same name
  IDB_IMPORT_NEW,
  IDB_IMPORT_NEW
};

// Additional bitmaps not on ToolBar
const UINT CPWToolBar::m_OtherNewDisBMs[] = {
  IDB_PROPERTIES_NEW_D,
  IDB_GROUPENTER_NEW_D,
  IDB_DUPLICATE_NEW_D,
  IDB_CHANGEFONTMENU_NEW_D,
  IDB_CHANGEFONTMENU_NEW_D,
  IDB_CHANGEPSWDFONTMENU_NEW_D,
  IDB_COMPARE_NEW_D,       // For report of the same name
  IDB_FIND_NEW_D,          // For report of the same name
  IDB_IMPORTTEXT_NEW_D,    // For report of the same name
  IDB_IMPORTXML_NEW_D,     // For report of the same name
  IDB_MERGE_NEW_D,         // For report of the same name
  IDB_SYNCHRONIZE_NEW_D,   // For report of the same name
  IDB_VALIDATE_NEW_D,      // For report of the same name
  IDB_CHANGECOMBO_NEW_D,
  IDB_BACKUPSAFE_NEW_D,
  IDB_RESTORE_NEW_D,
  IDB_EXIT_NEW_D,
  IDB_ABOUT_NEW_D,
  IDB_TRAYUNLOCK_NEW_D,
  IDB_TRAYLOCK_NEW_D,
  IDB_VIEWREPORTS_NEW_D,
  IDB_PWSDB,
  IDB_EXPORT_NEW_D,
  IDB_IMPORT_NEW_D,
  IDB_CREATESHORTCUT_NEW_D,
  IDB_CUSTOMIZETBAR_NEW,    // Don't need another disabled bitmap as never dispayed
  IDB_CHANGEVKBDFONTMENU_NEW_D,
  IDB_VIEWEDIT_NEW_D,
  IDB_IMPORT_NEW_D,
  IDB_EXPORTTEXT_NEW_D,
  IDB_EXPORTXML_NEW_D,
  IDB_DUPLICATEGROUP_NEW_D,
  IDB_EXPORTTEXT_NEW_D,    // For report of the same name
  IDB_EXPORTXML_NEW_D,     // For report of the same name
  IDB_IMPORT_NEW_D,
  IDB_IMPORT_NEW_D
};

IMPLEMENT_DYNAMIC(CPWToolBar, CToolBar)

CPWToolBar::CPWToolBar()
:  m_bitmode(1), m_iBrowseURL_BM_offset(-1), m_iSendEmail_BM_offset(-1)
{
  // Make sure the developer has kept everything in step!
  ASSERT(_countof(m_MainToolBarIDs) == _countof(m_csMainButtons));

  ASSERT(_countof(m_MainToolBarClassicBMs) == _countof(m_MainToolBarNewBMs));
  ASSERT(_countof(m_MainToolBarNewBMs) == _countof(m_MainToolBarNewDisBMs));

  ASSERT(_countof(m_OtherIDs) == _countof(m_OtherClassicBMs));
  ASSERT(_countof(m_OtherClassicBMs) == _countof(m_OtherNewBMs));
  ASSERT(_countof(m_OtherNewBMs) == _countof(m_OtherNewDisBMs));

  m_iMaxNumButtons = _countof(m_MainToolBarIDs);
  m_pOriginalTBinfo = new TBBUTTON[m_iMaxNumButtons];
  m_iNum_Bitmaps = _countof(m_MainToolBarClassicBMs) + _countof(m_OtherClassicBMs);
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

  int iNum_Bitmaps = _countof(m_MainToolBarClassicBMs);
  int iNum_Others  = _countof(m_OtherClassicBMs);

  for (i = 0; i < iNum_Bitmaps; i++) {
    if (m_MainToolBarClassicBMs[i] == IDB_BROWSEURL_CLASSIC) {
      m_iBrowseURL_BM_offset = i;
      break;
    }
  }

  m_iSendEmail_BM_offset = iNum_Bitmaps;  // First of the "Others"

  SetupImageList(&m_MainToolBarClassicBMs[0], NULL, iNum_Bitmaps, 0);
  SetupImageList(&m_OtherClassicBMs[0], NULL, iNum_Others, 0);

  SetupImageList(&m_MainToolBarNewBMs[0], &m_MainToolBarNewDisBMs[0], iNum_Bitmaps, 1);
  SetupImageList(&m_OtherNewBMs[0], &m_OtherNewDisBMs[0], iNum_Others, 1);

  SetupImageList(&m_MainToolBarNewBMs[0], &m_MainToolBarNewDisBMs[0], iNum_Bitmaps, 2);
  SetupImageList(&m_OtherNewBMs[0], &m_OtherNewDisBMs[0], iNum_Others, 2);

  if (bRefresh)
    return;

  j = 0;
  m_csDefaultButtonString.Empty();
  m_iNumDefaultButtons = m_iMaxNumButtons;
  for (i = 0; i < m_iMaxNumButtons; i++) {
    const bool bIsSeparator = m_MainToolBarIDs[i] == ID_SEPARATOR;
    BYTE fsStyle = bIsSeparator ? TBSTYLE_SEP : TBSTYLE_BUTTON;
    fsStyle &= ~BTNS_SHOWTEXT;
    if (!bIsSeparator) {
      fsStyle |= TBSTYLE_AUTOSIZE;
    }
    m_pOriginalTBinfo[i].iBitmap = bIsSeparator ? -1 : j;
    m_pOriginalTBinfo[i].idCommand = m_MainToolBarIDs[i];
    m_pOriginalTBinfo[i].fsState = TBSTATE_ENABLED;
    m_pOriginalTBinfo[i].fsStyle = fsStyle;
    m_pOriginalTBinfo[i].dwData = 0;
    m_pOriginalTBinfo[i].iString = bIsSeparator ? -1 : j;

    if (i <= m_iNumDefaultButtons)
      m_csDefaultButtonString += m_csMainButtons[i] + L" ";

    if (m_MainToolBarIDs[i] == ID_HELP)
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


  csButtonNames.MakeLower();
  int curPos(0);
  // Note all separators will be treated as the first!
  i = 0;
  CString csToken = csButtonNames.Tokenize(L" ", curPos);
  while (csToken != L"" && curPos != -1) {
    const CString *cstring_iter = std::find(m_csMainButtons,
                                            m_csMainButtons + m_iMaxNumButtons, csToken);
    if (cstring_iter != m_csMainButtons + m_iMaxNumButtons) {
      int index = std::distance(m_csMainButtons, cstring_iter);
      tbCtrl.AddButtons(1, &m_pOriginalTBinfo[index]);
    }
    csToken = csButtonNames.Tokenize(L" ", curPos);
  }

  tbCtrl.AutoSize();
}

CString CPWToolBar::GetButtonString() const
{
  CString cs_buttonnames(L"");
  TBBUTTONINFO tbinfo;
  int num_buttons, i;

  CToolBarCtrl& tbCtrl = GetToolBarCtrl();

  num_buttons = tbCtrl.GetButtonCount();

  SecureZeroMemory(&tbinfo, sizeof(tbinfo));
  tbinfo.cbSize = sizeof(tbinfo);
  tbinfo.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE;

  for (i = 0; i < num_buttons; i++) {
    tbCtrl.GetButtonInfo(i, &tbinfo);

    if (tbinfo.fsStyle & TBSTYLE_SEP) {
      cs_buttonnames += L"~ ";
      continue;
    }

    const UINT *uint_iter = std::find(m_MainToolBarIDs,
                                      m_MainToolBarIDs + m_iMaxNumButtons,
                                      tbinfo.idCommand);
    if (uint_iter != m_MainToolBarIDs + m_iMaxNumButtons) {
      int index = std::distance(m_MainToolBarIDs, uint_iter);
      cs_buttonnames += m_csMainButtons[index] + L" ";
    } else {
      ASSERT(0);
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
    if (m_MainToolBarIDs[i] != ID_SEPARATOR) {
      CString cs_buttondesc;
      cs_buttondesc.LoadString(m_MainToolBarIDs[i]);
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
  int iNum_ToolBarIDs = _countof(m_MainToolBarIDs);
  for (i = 0; i < iNum_ToolBarIDs; i++) {
    UINT ID = m_MainToolBarIDs[i];
    if (ID == ID_SEPARATOR)
      continue;
    IDtoImages[ID] = j;
    j++;
  }

  int iNum_OtherIDs  = _countof(m_OtherIDs);
  for (i = 0; i < iNum_OtherIDs; i++) {
    UINT ID = m_OtherIDs[i];
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

void CPWToolBar::SetupImageList(const UINT *pBM_IDs, const UINT *pDisBM_IDs,
                                const int numBMs, const int nImageList)
{
  const COLORREF crCOLOR_3DFACE = GetSysColor(COLOR_3DFACE);

  CBitmap bmNormal, bmDisabled;

  for (int i = 0; i < numBMs; i++) {
    BOOL brc = bmNormal.Attach(::LoadImage(::AfxFindResourceHandle(MAKEINTRESOURCE(pBM_IDs[i]), RT_BITMAP),
                               MAKEINTRESOURCE(pBM_IDs[i]), IMAGE_BITMAP, 0, 0,
                               (LR_DEFAULTSIZE | LR_CREATEDIBSECTION)));
    ASSERT(brc);
    SetBitmapBackground(bmNormal, crCOLOR_3DFACE);
    m_ImageLists[nImageList].Add(&bmNormal, crCOLOR_3DFACE);
    bmNormal.DeleteObject();

    if (nImageList != 0) {
      bmDisabled.Attach(::LoadImage(::AfxFindResourceHandle(MAKEINTRESOURCE(pDisBM_IDs[i]), RT_BITMAP),
                                    MAKEINTRESOURCE(pDisBM_IDs[i]), IMAGE_BITMAP, 0, 0,
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
