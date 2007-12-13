/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */
// file DboxMain.cpp
//
// The implementation of DboxMain's member functions is spread over source
// files that start with "Main" and reflect the main menu partitioning:
// MainFile.cpp implements the methods pertaining to the functions under
// the File menu, MainView.cpp does the same for the View menu, etc.
//
//This file contains what's left.
//-----------------------------------------------------------------------------

#include "PasswordSafe.h"
#include "ThisMfcApp.h"
#include "AboutDlg.h"
#include "PwFont.h"
#include "corelib/PWSprefs.h"
#include "corelib/PWSrand.h"

#if defined(POCKET_PC)
  #include "pocketpc/resource.h"
#else
  #include "resource.h"
  #include "resource2.h"  // Menu, Toolbar & Accelerator resources
  #include "resource3.h"  // String resources
#endif

// dialog boxen
#include "DboxMain.h"

#include "TryAgainDlg.h"
#include "PasskeyEntry.h"
#include "ExpPWListDlg.h"
#include "GeneralMsgBox.h"

// widget override?
#include "SysColStatic.h"

#ifdef POCKET_PC
  #include "pocketpc/PocketPC.h"
  #include "ShowPasswordDlg.h"
#endif

#include <afxpriv.h>
#include <stdlib.h> // for qsort

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
 * This is the string to be displayed instead of the actual password, unless
 * the user chooses to see the password:
 */

const TCHAR *HIDDEN_PASSWORD = _T("**************");
CString DboxMain::CS_DELETEENTRY;
CString DboxMain::CS_DELETEGROUP;
CString DboxMain::CS_RENAMEENTRY;
CString DboxMain::CS_RENAMEGROUP;
CString DboxMain::CS_EDITENTRY;
CString DboxMain::CS_VIEWENTRY;
CString DboxMain::CS_BROWSEURL;
CString DboxMain::CS_SENDEMAIL;
CString DboxMain::CS_EXPCOLGROUP;

//-----------------------------------------------------------------------------
DboxMain::DboxMain(CWnd* pParent)
   : CDialog(DboxMain::IDD, pParent),
     m_bSizing(false), m_needsreading(true), m_windowok(false),
     m_toolbarsSetup(FALSE),
     m_bSortAscending(true), m_iTypeSortColumn(CItemData::TITLE),
     m_core(app.m_core), m_pFontTree(NULL),
     m_selectedAtMinimize(NULL), m_bTSUpdated(false),
     m_iSessionEndingStatus(IDIGNORE),
     m_pchTip(NULL), m_pwchTip(NULL),
     m_bValidate(false), m_bOpen(false), 
     m_IsStartClosed(false), m_IsStartSilent(false),
     m_bStartHiddenAndMinimized(false),
     m_bAlreadyToldUserNoSave(false), m_inExit(false),
     m_pCC(NULL), m_bBoldItem(false), m_bIsRestoring(false), m_bImageInLV(false)
{
  CS_EXPCOLGROUP.LoadString(IDS_MENUEXPCOLGROUP);
  CS_EDITENTRY.LoadString(IDS_MENUEDITENTRY);
  CS_VIEWENTRY.LoadString(IDS_MENUVIEWENTRY);
  CS_DELETEENTRY.LoadString(IDS_MENUDELETEENTRY);
  CS_DELETEGROUP.LoadString(IDS_MENUDELETEGROUP);
  CS_RENAMEENTRY.LoadString(IDS_MENURENAMEENTRY);
  CS_RENAMEGROUP.LoadString(IDS_MENURENAMEGROUP);
  CS_BROWSEURL.LoadString(IDS_MENUBROWSEURL);
  CS_SENDEMAIL.LoadString(IDS_MENUSENDEMAIL);
  //{{AFX_DATA_INIT(DboxMain)
  // NOTE: the ClassWizard will add member initialization here
  //}}AFX_DATA_INIT

  m_hIcon = app.LoadIcon(IDI_CORNERICON);
  m_hIconSm = (HICON) ::LoadImage(app.m_hInstance, MAKEINTRESOURCE(IDI_CORNERICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

  ClearData();

#if !defined(POCKET_PC)
  m_titlebar = _T("");
  m_toolbarsSetup = FALSE;
#endif
}

DboxMain::~DboxMain()
{
  delete m_pchTip;
  delete m_pwchTip;
  delete m_pFontTree;
}

BEGIN_MESSAGE_MAP(DboxMain, CDialog)
//{{AFX_MSG_MAP(DboxMain)

ON_UPDATE_COMMAND_UI_RANGE(ID_MENUTOOLBAR_START, ID_MENUTOOLBAR_END, OnUpdateMenuToolbar)

// File Menu
ON_COMMAND(ID_MENUITEM_NEW, OnNew)
ON_COMMAND(ID_MENUITEM_OPEN, OnOpen)
ON_COMMAND(ID_MENUITEM_CLOSE, OnClose)
ON_COMMAND(ID_MENUITEM_CLEAR_MRU, OnClearMRU)
ON_COMMAND(ID_MENUITEM_SAVE, OnSave)
ON_COMMAND(ID_MENUITEM_SAVEAS, OnSaveAs)
ON_COMMAND_RANGE(ID_MENUITEM_EXPORT2OLD1XFORMAT, ID_MENUITEM_EXPORT2V2FORMAT, OnExportVx)
ON_COMMAND(ID_MENUITEM_EXPORT2PLAINTEXT, OnExportText)
ON_COMMAND(ID_MENUITEM_EXPORT2XML, OnExportXML)
ON_COMMAND(ID_MENUITEM_IMPORT_PLAINTEXT, OnImportText)
ON_COMMAND(ID_MENUITEM_IMPORT_KEEPASS, OnImportKeePass)
ON_COMMAND(ID_MENUITEM_IMPORT_XML, OnImportXML)
ON_COMMAND(ID_MENUITEM_MERGE, OnMerge)
ON_COMMAND(ID_MENUITEM_COMPARE, OnCompare)
ON_COMMAND(ID_MENUITEM_PROPERTIES, OnProperties)

// Edit Menu
ON_COMMAND(ID_MENUITEM_ADD, OnAdd)
ON_COMMAND(ID_MENUITEM_ADDGROUP, OnAddGroup)
ON_COMMAND(ID_MENUITEM_ADDSHORTCUT, OnAddShortcut)
ON_COMMAND(ID_MENUITEM_EDIT, OnEdit)
ON_COMMAND(ID_MENUITEM_GROUPENTER, OnEdit)
ON_COMMAND(ID_MENUITEM_BROWSEURL, OnBrowse)
ON_COMMAND(ID_MENUITEM_SENDEMAIL, OnBrowse)
ON_COMMAND(ID_MENUITEM_COPYPASSWORD, OnCopyPassword)
ON_COMMAND(ID_MENUITEM_COPYNOTESFLD, OnCopyNotes)
ON_COMMAND(ID_MENUITEM_COPYUSERNAME, OnCopyUsername)
ON_COMMAND(ID_MENUITEM_COPYURL, OnCopyURL)
ON_COMMAND(ID_MENUITEM_CLEARCLIPBOARD, OnClearClipboard)
ON_COMMAND(ID_MENUITEM_DELETE, OnDelete)
ON_COMMAND(ID_MENUITEM_RENAME, OnRename)
ON_COMMAND(ID_MENUITEM_FIND, OnFind)
ON_COMMAND(ID_MENUITEM_DUPLICATEENTRY, OnDuplicateEntry)
ON_COMMAND(ID_MENUITEM_AUTOTYPE, OnAutoType)

// View Menu
ON_COMMAND(ID_MENUITEM_LIST_VIEW, OnListView)
ON_COMMAND(ID_MENUITEM_TREE_VIEW, OnTreeView)
ON_COMMAND(ID_MENUITEM_OLD_TOOLBAR, OnOldToolbar)
ON_COMMAND(ID_MENUITEM_NEW_TOOLBAR, OnNewToolbar)
ON_COMMAND(ID_MENUITEM_EXPANDALL, OnExpandAll)
ON_COMMAND(ID_MENUITEM_COLLAPSEALL, OnCollapseAll)
ON_COMMAND(ID_MENUITEM_CHANGEFONT, OnChangeFont)
ON_COMMAND_RANGE(ID_MENUITEM_REPORT_COMPARE, ID_MENUITEM_REPORT_VALIDATE, OnViewReports)

// Manage Menu
ON_COMMAND(ID_MENUITEM_CHANGECOMBO, OnPasswordChange)
ON_COMMAND(ID_MENUITEM_BACKUPSAFE, OnBackupSafe)
ON_COMMAND(ID_MENUITEM_RESTORE, OnRestore)
ON_COMMAND(ID_MENUITEM_OPTIONS, OnOptions)

// Help Menu
ON_COMMAND(ID_MENUITEM_ABOUT, OnAbout)
ON_COMMAND(ID_MENUITEM_PWSAFE_WEBSITE, OnPasswordSafeWebsite)
ON_COMMAND(ID_MENUITEM_U3SHOP_WEBSITE, OnU3ShopWebsite)

// List view Column Picker
ON_COMMAND(ID_MENUITEM_COLUMNPICKER, OnColumnPicker)
ON_COMMAND(ID_MENUITEM_RESETCOLUMNS, OnResetColumns)

// Others
ON_COMMAND(ID_MENUITEM_VALIDATE, OnValidate)

#if defined(POCKET_PC)
ON_WM_CREATE()
#else
ON_WM_CONTEXTMENU()
#endif

// Windows Messages
ON_WM_DESTROY()
ON_WM_ENDSESSION()
ON_WM_INITMENU()
ON_WM_INITMENUPOPUP()
ON_WM_QUERYENDSESSION()
ON_WM_MOVE()
ON_WM_SIZE()
ON_WM_SYSCOMMAND()
ON_WM_TIMER()
ON_WM_WINDOWPOSCHANGING()

ON_WM_DRAWITEM()
ON_WM_MEASUREITEM()

ON_NOTIFY(NM_CLICK, IDC_ITEMLIST, OnListItemSelected)
ON_NOTIFY(NM_CLICK, IDC_ITEMTREE, OnTreeItemSelected)
ON_NOTIFY(LVN_KEYDOWN, IDC_ITEMLIST, OnKeydownItemlist)
ON_NOTIFY(NM_DBLCLK, IDC_ITEMLIST, OnItemDoubleClick)
ON_NOTIFY(NM_DBLCLK, IDC_ITEMTREE, OnItemDoubleClick)
ON_NOTIFY(LVN_COLUMNCLICK, IDC_ITEMLIST, OnColumnClick)
ON_NOTIFY(NM_RCLICK, IDC_LIST_HEADER, OnHeaderRClick)
ON_NOTIFY(HDN_BEGINDRAG, IDC_LIST_HEADER, OnHeaderBeginDrag)
ON_NOTIFY(HDN_ENDDRAG, IDC_LIST_HEADER, OnHeaderEndDrag)
ON_NOTIFY(HDN_ENDTRACK, IDC_LIST_HEADER, OnHeaderNotify)
ON_NOTIFY(HDN_ITEMCHANGED, IDC_LIST_HEADER, OnHeaderNotify)

ON_COMMAND(ID_MENUITEM_EXIT, OnOK)
ON_COMMAND(ID_MENUITEM_MINIMIZE, OnMinimize)
ON_COMMAND(ID_MENUITEM_UNMINIMIZE, OnUnMinimize)

#if defined(POCKET_PC)
ON_COMMAND(ID_MENUITEM_SHOWPASSWORD, OnShowPassword)
#else
ON_WM_DROPFILES()
ON_WM_SIZING()
ON_NOTIFY(NM_SETFOCUS, IDC_ITEMLIST, OnChangeItemFocus)
ON_NOTIFY(NM_KILLFOCUS, IDC_ITEMLIST, OnChangeItemFocus)
ON_NOTIFY(NM_SETFOCUS, IDC_ITEMTREE, OnChangeItemFocus)
ON_NOTIFY(NM_KILLFOCUS, IDC_ITEMTREE, OnChangeItemFocus)
ON_COMMAND(ID_MENUITEM_TRAYLOCK, OnTrayLockUnLock)
ON_COMMAND(ID_MENUITEM_TRAYUNLOCK, OnTrayLockUnLock)
ON_COMMAND(ID_MENUITEM_CLEARRECENTENTRIES, OnTrayClearRecentEntries)
ON_COMMAND(ID_TOOLBUTTON_LISTTREE, OnToggleView)
ON_COMMAND(ID_TOOLBUTTON_VIEWREPORTS, OnViewReports)

ON_COMMAND(ID_TOOLBUTTON_CLOSEFIND, OnHideFindToolBar)
ON_COMMAND(ID_TOOLBUTTON_FIND, OnToolBarFind)
ON_COMMAND(ID_TOOLBUTTON_FINDCASE, OnToolBarFindCase)
ON_COMMAND(ID_TOOLBUTTON_FINDCASE_I, OnToolBarFindCase)
ON_COMMAND(ID_TOOLBUTTON_FINDCASE_S, OnToolBarFindCase)
ON_COMMAND(ID_TOOLBUTTON_FINDADVANCED, OnToolBarFindAdvanced)
ON_COMMAND(ID_TOOLBUTTON_CLEARFIND, OnToolBarClearFind)
#endif

#ifndef POCKET_PC
ON_BN_CLICKED(IDOK, OnEdit)
#endif

ON_MESSAGE(WM_ICON_NOTIFY, OnTrayNotification)
ON_MESSAGE(WM_HOTKEY, OnHotKey)
ON_MESSAGE(WM_CCTOHDR_DD_COMPLETE, OnCCToHdrDragComplete)
ON_MESSAGE(WM_HDRTOCC_DD_COMPLETE, OnHdrToCCDragComplete)
ON_MESSAGE(WM_HDR_DRAG_COMPLETE, OnHeaderDragComplete)
ON_MESSAGE(WM_COMPARE_RESULT_FUNCTION, OnProcessCompareResultFunction)
ON_MESSAGE(WM_TOOLBAR_FIND, OnToolBarFindMessage)

ON_COMMAND(ID_MENUITEM_CUSTOMIZETOOLBAR, OnCustomizeToolbar)
   
//}}AFX_MSG_MAP
ON_COMMAND_EX_RANGE(ID_FILE_MRU_ENTRY1, ID_FILE_MRU_ENTRYMAX, OnOpenMRU)
ON_UPDATE_COMMAND_UI(ID_FILE_MRU_ENTRY1, OnUpdateMRU)  // Note: can't be in OnUpdateMenuToolbar!
#ifndef POCKET_PC
ON_COMMAND_RANGE(ID_MENUITEM_TRAYCOPYUSERNAME1, ID_MENUITEM_TRAYCOPYUSERNAMEMAX, OnTrayCopyUsername)
ON_UPDATE_COMMAND_UI_RANGE(ID_MENUITEM_TRAYCOPYUSERNAME1, ID_MENUITEM_TRAYCOPYUSERNAMEMAX, OnUpdateTrayCopyUsername)
ON_COMMAND_RANGE(ID_MENUITEM_TRAYCOPYPASSWORD1, ID_MENUITEM_TRAYCOPYPASSWORDMAX, OnTrayCopyPassword)
ON_UPDATE_COMMAND_UI_RANGE(ID_MENUITEM_TRAYCOPYPASSWORD1, ID_MENUITEM_TRAYCOPYPASSWORDMAX, OnUpdateTrayCopyPassword)
ON_COMMAND_RANGE(ID_MENUITEM_TRAYCOPYNOTES1, ID_MENUITEM_TRAYCOPYNOTESMAX, OnTrayCopyNotes)
ON_UPDATE_COMMAND_UI_RANGE(ID_MENUITEM_TRAYCOPYNOTES1, ID_MENUITEM_TRAYCOPYNOTESMAX, OnUpdateTrayCopyNotes)
ON_COMMAND_RANGE(ID_MENUITEM_TRAYBROWSE1, ID_MENUITEM_TRAYBROWSEMAX, OnTrayBrowse)
ON_UPDATE_COMMAND_UI_RANGE(ID_MENUITEM_TRAYBROWSE1, ID_MENUITEM_TRAYBROWSEMAX, OnUpdateTrayBrowse)
ON_COMMAND_RANGE(ID_MENUITEM_TRAYDELETE1, ID_MENUITEM_TRAYDELETEMAX, OnTrayDeleteEntry)
ON_UPDATE_COMMAND_UI_RANGE(ID_MENUITEM_TRAYDELETE1, ID_MENUITEM_TRAYDELETEMAX, OnUpdateTrayDeleteEntry)
ON_COMMAND_RANGE(ID_MENUITEM_TRAYAUTOTYPE1, ID_MENUITEM_TRAYAUTOTYPEMAX, OnTrayAutoType)
ON_UPDATE_COMMAND_UI_RANGE(ID_MENUITEM_TRAYAUTOTYPE1, ID_MENUITEM_TRAYAUTOTYPEMAX, OnUpdateTrayAutoType)
ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
#endif
END_MESSAGE_MAP()

// Command ID, OpenRW, OpenRO, Empty, Closed
const DboxMain::UICommandTableEntry DboxMain::m_UICommandTable[] = {
  // File menu
  {ID_MENUITEM_OPEN, true, true, true, true},
  {ID_MENUITEM_NEW, true, true, true, true},
  {ID_MENUITEM_CLOSE, true, true, true, false},
  {ID_MENUITEM_SAVE, true, false, true, false},
  {ID_MENUITEM_SAVEAS, true, true, true, false},
  {ID_MENUITEM_CLEAR_MRU, true, true, true, true},
  {ID_MENUITEM_EXPORT2PLAINTEXT, true, true, false, false},
  {ID_MENUITEM_EXPORT2OLD1XFORMAT, true, true, false, false},
  {ID_MENUITEM_EXPORT2V2FORMAT, true, true, false, false},
  {ID_MENUITEM_EXPORT2XML, true, true, false, false},
  {ID_MENUITEM_IMPORT_XML, true, false, true, false},
  {ID_MENUITEM_IMPORT_PLAINTEXT, true, false, true, false},
  {ID_MENUITEM_IMPORT_KEEPASS, true, false, true, false},
  {ID_MENUITEM_MERGE, true, false, true, false},
  {ID_MENUITEM_COMPARE, true, true, false, false},
  {ID_MENUITEM_PROPERTIES, true, true, true, false},
  {ID_MENUITEM_EXIT, true, true, true, true},
  // Edit menu
  {ID_MENUITEM_ADD, true, false, true, false},
  {ID_MENUITEM_ADDSHORTCUT, true, false, true, false},
  {ID_MENUITEM_EDIT, true, true, false, false},
  {ID_MENUITEM_GROUPENTER, true, true, false, false},
  {ID_MENUITEM_DELETE, true, false, false, false},
  {ID_MENUITEM_RENAME, true, false, false, false},
  {ID_MENUITEM_FIND, true, true, false, false},
  {ID_MENUITEM_DUPLICATEENTRY, true, false, false, false},
  {ID_MENUITEM_ADDGROUP, true, false, true, false},
  {ID_MENUITEM_COPYPASSWORD, true, true, false, false},
  {ID_MENUITEM_COPYUSERNAME, true, true, false, false},
  {ID_MENUITEM_COPYNOTESFLD, true, true, false, false},
  {ID_MENUITEM_CLEARCLIPBOARD, true, true, true, false},
  {ID_MENUITEM_BROWSEURL, true, true, false, false},
  {ID_MENUITEM_SENDEMAIL, true, true, false, false},
  {ID_MENUITEM_AUTOTYPE, true, true, false, false},
  {ID_MENUITEM_COPYURL, true, true, false, false},
  // View menu
  {ID_MENUITEM_LIST_VIEW, true, true, true, false},
  {ID_MENUITEM_TREE_VIEW, true, true, true, false},
  {ID_MENUITEM_NEW_TOOLBAR, true, true, true, true},
  {ID_MENUITEM_OLD_TOOLBAR, true, true, true, true},
  {ID_MENUITEM_EXPANDALL, true, true, true, false},
  {ID_MENUITEM_COLLAPSEALL, true, true, true, false},
  {ID_MENUITEM_CHANGEFONT, true, true, true, false},
  {ID_MENUITEM_REPORT_COMPARE, true, true, true, true},
  {ID_MENUITEM_REPORT_IMPORTTEXT, true, true, true, true},
  {ID_MENUITEM_REPORT_IMPORTXML, true, true, true, true},
  {ID_MENUITEM_REPORT_MERGE, true, true, true, true},
  {ID_MENUITEM_REPORT_VALIDATE, true, true, true, true},
  // Manage menu
  {ID_MENUITEM_CHANGECOMBO, true, false, true, false},
  {ID_MENUITEM_BACKUPSAFE, true, true, true, false},
  {ID_MENUITEM_RESTORE, true, false, true, false},
  {ID_MENUITEM_OPTIONS, true, true, true, true},
  // Help Menu
  {ID_MENUITEM_PWSAFE_WEBSITE, true, true, true, true},
  {ID_MENUITEM_ABOUT, true, true, true, true},
  {ID_MENUITEM_U3SHOP_WEBSITE, true, true, true, true},
  // Column popup menu
  {ID_MENUITEM_COLUMNPICKER, true, true, true, false},
  {ID_MENUITEM_RESETCOLUMNS, true, true, true, false},
  // Compare popup menu
  {ID_MENUITEM_COMPVIEWEDIT, true, false, true, false},
  {ID_MENUITEM_COPY_TO_ORIGINAL, true, false, true, false},
  {ID_MENUITEM_COPY_TO_COMPARISON, true, true, true, false},
  // Tray popup menu
  {ID_MENUITEM_TRAYUNLOCK, true, true, true, false},
  {ID_MENUITEM_TRAYLOCK, true, true, true, false},
  {ID_MENUITEM_CLEARRECENTENTRIES, true, true, true, false},
  {ID_MENUITEM_MINIMIZE, true, true, true, true},
  {ID_MENUITEM_UNMINIMIZE, true, true, true, true},
  {ID_MENUITEM_TRAYAUTOTYPE, true, true, false, false},
  {ID_MENUITEM_TRAYBROWSE, true, true, false, false},
  {ID_MENUITEM_TRAYCOPYNOTES, true, true, false, false},
  {ID_MENUITEM_TRAYCOPYPASSWORD, true, true, false, false},
  {ID_MENUITEM_TRAYCOPYUSERNAME, true, true, false, false},
  {ID_MENUITEM_TRAYDELETE, true, false, false, false},
  {ID_MENUITEM_TRAYVIEWEDIT, true, true, false, false},
  // Default Main Toolbar buttons - if not menu items
  //   None
  // Optional Main Toolbar buttons
  {ID_TOOLBUTTON_LISTTREE, true, true, true, false},
  {ID_TOOLBUTTON_VIEWREPORTS, true, true, true, false},
  // Find Toolbar
  {ID_TOOLBUTTON_CLOSEFIND, true, true, true, false},
  {ID_TOOLBUTTON_FINDEDITCTRL, true, true, false, false},
  {ID_TOOLBUTTON_FIND, true, true, false, false},
  {ID_TOOLBUTTON_FINDCASE, true, true, false, false},
  {ID_TOOLBUTTON_FINDCASE_I, true, true, false, false},
  {ID_TOOLBUTTON_FINDCASE_S, true, true, false, false},
  {ID_TOOLBUTTON_FINDADVANCED, true, true, false, false},
  {ID_TOOLBUTTON_CLEARFIND, true, true, false, false},
  // Customize Main Toolbar
  {ID_MENUITEM_CUSTOMIZETOOLBAR, true, true, true, true},
};

void
DboxMain::InitPasswordSafe()
{  
  PWSprefs *prefs = PWSprefs::GetInstance();
  // Real initialization done here
  // Requires OnInitDialog to have passed OK
  UpdateAlwaysOnTop();

  // ... same for UseSystemTray
  // StartSilent trumps preference (but StartClosed doesn't)
  if (!m_IsStartSilent && !prefs->GetPref(PWSprefs::UseSystemTray))
    app.HideIcon();

  m_RUEList.SetMax(prefs->GetPref(PWSprefs::MaxREItems));

  // Set timer for user-defined lockout, if selected
  if (prefs->GetPref(PWSprefs::LockOnIdleTimeout)) {
    const UINT MINUTE = 60*1000;
    SetTimer(TIMER_USERLOCK, MINUTE, NULL);
    ResetIdleLockCounter();
  }

  // JHF : no hotkeys on WinCE
#if !defined(POCKET_PC)
  // Set Hotkey, if active
  if (prefs->GetPref(PWSprefs::HotKeyEnabled)) {
    const DWORD value = DWORD(prefs->GetPref(PWSprefs::HotKey));
    WORD wVirtualKeyCode = WORD(value & 0xffff);
    WORD mod = WORD(value >> 16);
    WORD wModifiers = 0;
    // Translate between CWnd & CHotKeyCtrl modifiers
    if (mod & HOTKEYF_ALT)
      wModifiers |= MOD_ALT;
    if (mod & HOTKEYF_CONTROL)
      wModifiers |= MOD_CONTROL;
    if (mod & HOTKEYF_SHIFT)
      wModifiers |= MOD_SHIFT;
    RegisterHotKey(m_hWnd, PWS_HOTKEY_ID, UINT(wModifiers), UINT(wVirtualKeyCode));
    // registration might fail if combination already registered elsewhere,
    // but don't see any elegant way to notify the user here, so fail silently
  } else {
    // No sense in unregistering at this stage, really.
  }
#endif

  m_windowok = true;

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog

  SetIcon(m_hIcon, TRUE);  // Set big icon
  SetIcon(m_hIconSm, FALSE); // Set small icon
  // Init stuff for tree view
  m_pImageList = new CImageList();
  // Number (12) corresponds to number in CPWTreeCtrl public enum
  BOOL status = m_pImageList->Create(9, 9, ILC_COLOR, 12, 0);
  ASSERT(status != 0);

  // Dummy Imagelist needed if user adds then removes Icon column
  m_pImageList0 = new CImageList();
  status = m_pImageList0->Create(1, 1, ILC_COLOR, 0, 1);
  ASSERT(status != 0);

  CBitmap bitmap;

  // Order of LoadBitmap() calls matches CPWTreeCtrl public enum
  // Also now used by CListCtrl!
  bitmap.LoadBitmap(IDB_NODE);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_NORMAL);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_NORMAL_WARNEXPIRED);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_NORMAL_EXPIRED);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_ABASE);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_ABASE_WARNEXPIRED);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_ABASE_EXPIRED);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_ALIAS);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_SBASE);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_SBASE_WARNEXPIRED);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_SBASE_EXPIRED);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();
  bitmap.LoadBitmap(IDB_SHORTCUT);
  m_pImageList->Add(&bitmap, (COLORREF)0x0);
  bitmap.DeleteObject();

  m_ctlItemTree.SetImageList(m_pImageList, TVSIL_NORMAL);
  m_ctlItemTree.SetImageList(m_pImageList, TVSIL_STATE);

  DWORD dw_ExtendedStyle = LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP;
  if (prefs->GetPref(PWSprefs::ListViewGridLines))
    dw_ExtendedStyle |= LVS_EX_GRIDLINES;

  m_ctlItemList.SetExtendedStyle(dw_ExtendedStyle);
  m_ctlItemList.Initialize(this);

  // Override default HeaderCtrl ID of 0
  m_LVHdrCtrl.SetDlgCtrlID(IDC_LIST_HEADER);

  // Initialise DropTargets - should be in OnCreate()s, really
  m_LVHdrCtrl.Initialize(&m_LVHdrCtrl);
  m_ctlItemTree.Initialize();

  // Set up fonts before playing with Tree/List views
  m_pFontTree = new CFont;
  CString szTreeFont = prefs->GetPref(PWSprefs::TreeFont);

  if (szTreeFont != _T("")) {
    LOGFONT *ptreefont = new LOGFONT;
    memset(ptreefont, 0, sizeof(LOGFONT)); 
    ExtractFont(szTreeFont, ptreefont);
    m_pFontTree->CreateFontIndirect(ptreefont);
    // transfer the fonts to the tree windows
    m_ctlItemTree.SetFont(m_pFontTree);
    m_ctlItemList.SetFont(m_pFontTree);
    m_LVHdrCtrl.SetFont(m_pFontTree);
    delete ptreefont;
  }

  const CString lastView = prefs->GetPref(PWSprefs::LastView);
  if (lastView != _T("list"))
    OnTreeView();
  else
    OnListView();

  CalcHeaderWidths();

  CString cs_ListColumns = prefs->GetPref(PWSprefs::ListColumns);
  CString cs_ListColumnsWidths = prefs->GetPref(PWSprefs::ColumnWidths);

  if (cs_ListColumns.IsEmpty())
    SetColumns();
  else
    SetColumns(cs_ListColumns);

  m_iTypeSortColumn = prefs->GetPref(PWSprefs::SortedColumn);
  switch (m_iTypeSortColumn) {
    case CItemData::UUID:  // Used for sorting on Image!
    case CItemData::GROUP:
    case CItemData::TITLE:
    case CItemData::USER:
    case CItemData::NOTES:
    case CItemData::PASSWORD:
    case CItemData::CTIME:
    case CItemData::PMTIME:
    case CItemData::ATIME:
    case CItemData::LTIME:
    case CItemData::RMTIME:
    case CItemData::URL:
    case CItemData::AUTOTYPE:
      break;
    case CItemData::POLICY:  // Not implemented
    case CItemData::PWHIST:  // Not displayed in ListView
    default:
      // Title is a mandatory column - so can't go wrong!
      m_iTypeSortColumn = CItemData::TITLE;
      break;
  }
  m_bSortAscending = prefs->GetPref(PWSprefs::SortAscending);

  // refresh list will add and size password column if necessary...
  RefreshViews();

  ChangeOkUpdate();

  setupBars(); // Just to keep things a little bit cleaner

#if !defined(POCKET_PC)
  // {kjp} Can't drag and drop files onto an application in PocketPC
  DragAcceptFiles(TRUE);

  // {kjp} meaningless when target is a PocketPC device.
  CRect rect;
  prefs->GetPrefRect(rect.top, rect.bottom, rect.left, rect.right);

  if (rect.top == -1 || rect.bottom == -1 || rect.left == -1 || rect.right == -1) {
    GetWindowRect(&rect);
    SendMessage(WM_SIZE, SIZE_RESTORED, MAKEWPARAM(rect.Width(), rect.Height()));
  } else {
    // Sanity checks on stored rect - displays change...
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    // MS adds 4 pixels around the max screen size so if, maximized, then
    // top/left = (-4,-4) instead of (0,0) and bottom/right = (W+4, H+4)
    // If height/width too big, make them max. allowed values instead.
    // If off screen - bring it back

    // Too tall
    if (rect.Height() > screenHeight) {
      rect.top = -4;
      rect.bottom = screenHeight + 4;
    }

    // Too wide
    if (rect.Width() > screenWidth) {
      rect.left = -4;
      rect.right = screenWidth + 4;
    }

    // Too far off to the right
    if (rect.left > screenWidth) {
      int iw = rect.Width();
      rect.right = screenWidth;
      rect.left = rect.right - iw - 4;
      if (rect.left < -4) {
        rect.left = -4;
        rect.right = rect.left + iw;
      }      
    }

    // Too far off to the left
    if (rect.right < -4) {
      int iw = rect.Width();
      rect.left = -4;
      rect.right = rect.left + iw;
    }

    // Too far down
    if (rect.top > screenHeight) {
      int ih = rect.Height();
      rect.bottom = screenHeight;
      rect.top = rect.bottom - ih - 4;
      if (rect.top < -4) {
        rect.top = -4;
        rect.bottom = rect.top + ih;
      }      
    }

    // Too far up
    if (rect.bottom < -4) {
      int ih = rect.Height();
      rect.top = -4;
      rect.bottom = rect.top + ih;
    }

    // Now move it and size it
    ::MoveWindow(this->m_hWnd, rect.left, rect.top, rect.Width(), rect.Height(), TRUE);
  }
#endif

  m_core.SetUseDefUser(prefs->GetPref(PWSprefs::UseDefaultUser));
  m_core.SetDefUsername(prefs->GetPref(PWSprefs::DefaultUsername));

  SetMenu(app.m_mainmenu);  // Now show menu...

  // Now do widths!
  if (!cs_ListColumns.IsEmpty())
    SetColumnWidths(cs_ListColumnsWidths);

}

LRESULT
DboxMain::OnHotKey(WPARAM , LPARAM)
{
  // since we only have a single HotKey, the value assigned
  // to it is meaningless & unused, hence params ignored
  // The hotkey is used to invoke the app window, prompting
  // for passphrase if needed.

  app.SetHotKeyPressed(true);
  if (IsIconic()) {
    SendMessage(WM_COMMAND, ID_MENUITEM_UNMINIMIZE);
  }
  SetActiveWindow();
  SetForegroundWindow();
  return 0;
}

LRESULT
DboxMain::OnHeaderDragComplete(WPARAM /* wParam */, LPARAM /* lParam */)
{
  MSG msg;
  while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))	{
    // so there is a message process it.
    if (!AfxGetThread()->PumpMessage())
      break;
  }

  // Now update header info
  SetHeaderInfo();

  return 0L;
}

LRESULT
DboxMain::OnCCToHdrDragComplete(WPARAM wType, LPARAM afterIndex)
{
  if (wType == CItemData::UUID) {
    m_bImageInLV = true;
    m_ctlItemList.SetImageList(m_pImageList, LVSIL_NORMAL);
    m_ctlItemList.SetImageList(m_pImageList, LVSIL_SMALL);
  }

  AddColumn((int)wType, (int)afterIndex);

  return 0L;
}

LRESULT
DboxMain::OnHdrToCCDragComplete(WPARAM wType, LPARAM /* lParam */)
{
  if (wType == CItemData::UUID) {
    m_bImageInLV = false;
    m_ctlItemList.SetImageList(m_pImageList0, LVSIL_NORMAL);
    m_ctlItemList.SetImageList(m_pImageList0, LVSIL_SMALL);
  }

  DeleteColumn((int)wType);
 
  RefreshViews(iListOnly);

  return 0L;
}

BOOL
DboxMain::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Set up UPDATE_UI data map.
  const int num_CommandTable_entries = sizeof(m_UICommandTable) / sizeof(UICommandTableEntry);
  for (int i = 0; i < num_CommandTable_entries; i++)
    m_MapUICommandTable[m_UICommandTable[i].ID] = i;

  // Install menu popups for full path on MRU entries
  m_menuManager.Install(AfxGetMainWnd());
  m_menuTipManager.Install(AfxGetMainWnd());

  // Subclass the ListView HeaderCtrl
  CHeaderCtrl* pHeader;
  pHeader = m_ctlItemList.GetHeaderCtrl();
  if(pHeader && pHeader->GetSafeHwnd()) {
    m_LVHdrCtrl.SubclassWindow(pHeader->GetSafeHwnd());
  }

  ConfigureSystemMenu();
  InitPasswordSafe();
  
  // Validation does integrity check & repair on database
  // currently invoke it iff m_bValidate set (e.g., user passed '-v' flag)
  if (m_bValidate) {
    PostMessage(WM_COMMAND, ID_MENUITEM_VALIDATE);
    m_bValidate = false;
  }

  if (m_IsStartSilent) {
    m_bStartHiddenAndMinimized = true;
  }

  if (m_IsStartClosed) {
    Close();
    if (!m_IsStartSilent)
      ShowWindow(SW_SHOW);
  }

  if (!m_IsStartClosed && !m_IsStartSilent) {
    OpenOnInit();
    m_ctlItemTree.SetRestoreMode(true);
    RefreshViews();
    m_ctlItemTree.SetRestoreMode(false);
  }

  SetInitialDatabaseDisplay();
  OnHideFindToolBar();

  return TRUE;  // return TRUE unless you set the focus to a control
}

void DboxMain::SetInitialDatabaseDisplay()
{
  if (m_ctlItemTree.GetCount() > 0)
  switch (PWSprefs::GetInstance()->GetPref(PWSprefs::TreeDisplayStatusAtOpen)) {
  	case PWSprefs::AllCollapsed:
  		m_ctlItemTree.OnCollapseAll();
  		break;
  	case PWSprefs::AllExpanded:
  		m_ctlItemTree.OnExpandAll();
  		break;
  	case PWSprefs::AsPerLastSave:
  		RestoreDisplayStatus();
  		break;
  	default:
  		ASSERT(0);
  }
}

void
DboxMain::OnDestroy()
{
  const CMyString filename(m_core.GetCurFile());
  // The only way we're the locker is if it's locked & we're !readonly
  if (!filename.IsEmpty() && !m_core.IsReadOnly() && m_core.IsLockedFile(filename))
    m_core.UnlockFile(filename);

  // Get rid of hotkey
  UnregisterHotKey(m_hWnd, PWS_HOTKEY_ID);

  // Stop subclassing the ListView HeaderCtrl
  if (m_LVHdrCtrl.GetSafeHwnd() != NULL)
      m_LVHdrCtrl.UnsubclassWindow();

  // Stop Drag & Drop OLE
  m_LVHdrCtrl.Terminate();

  // and goodbye
  CDialog::OnDestroy();
}


void DboxMain::OnWindowPosChanging( WINDOWPOS* lpwndpos )
{
	if (m_bStartHiddenAndMinimized) {
		lpwndpos->flags |= (SWP_HIDEWINDOW | SWP_NOACTIVATE);
		lpwndpos->flags &= ~SWP_SHOWWINDOW;
		PostMessage(WM_COMMAND, ID_MENUITEM_MINIMIZE);
	}

	CDialog::OnWindowPosChanging(lpwndpos);
}

void DboxMain::OnMove(int x, int y)
{
  CDialog::OnMove(x, y);
  // turns out that minimizing calls this
  // with x = y = -32000. Oh joy.
  if (m_windowok && IsWindowVisible() == TRUE &&
      x >= 0 && y >= 0) {
    CRect rect;
    GetWindowRect(&rect);
    PWSprefs::GetInstance()->SetPrefRect(rect.top, rect.bottom,
                                         rect.left, rect.right);
  }
}

void DboxMain::FixListIndexes()
{
  int N = m_ctlItemList.GetItemCount();
  for (int i = 0; i < N; i++) {
    CItemData *ci = (CItemData *)m_ctlItemList.GetItemData(i);
    ASSERT(ci != NULL);
    DisplayInfo *di = (DisplayInfo *)ci->GetDisplayInfo();
    ASSERT(di != NULL);
    if (di->list_index != i)
      di->list_index = i;
  }
}

void
DboxMain::OnItemDoubleClick(NMHDR * /* pNotifyStruct */, LRESULT *pLResult)
{
  *pLResult = 0L;
  UnFindItem();

	// TreeView only - use DoubleClick to Expand/Collapse group
	if (m_ctlItemTree.IsWindowVisible()) {
		HTREEITEM hItem = m_ctlItemTree.GetSelectedItem();
		// Only if a group is selected
		if ((hItem != NULL && !m_ctlItemTree.IsLeaf(hItem))) {
			// Do standard double-click processing - i.e. toggle expand/collapse!
			return;
		}
	}

  // Now set we have processed the event
  *pLResult = 1L;

	// Continue if in ListView  or Leaf in TreeView

#if defined(POCKET_PC)
  if ( app.GetProfileInt(_T(PWS_REG_OPTIONS), _T("dcshowspassword"), FALSE) == FALSE ) {
    OnCopyPassword();
  } else {
    OnShowPassword();
  }
#else
  switch (PWSprefs::GetInstance()->
          GetPref(PWSprefs::DoubleClickAction)) {
  case PWSprefs::DoubleClickAutoType:
    PostMessage(WM_COMMAND, ID_MENUITEM_AUTOTYPE);
    break;
  case PWSprefs::DoubleClickBrowse:
    PostMessage(WM_COMMAND, ID_MENUITEM_BROWSEURL);
    break;
  case PWSprefs::DoubleClickCopyNotes:
    OnCopyNotes();
    break;
  case PWSprefs::DoubleClickCopyPassword:
    OnCopyPassword();
    break;
  case PWSprefs::DoubleClickCopyUsername:
    OnCopyUsername();
    break;
  case PWSprefs::DoubleClickViewEdit:
    PostMessage(WM_COMMAND, ID_MENUITEM_EDIT);
    break;
  default:
    ASSERT(0);
  }
#endif
}

// Called to open a web browser to the URL associated with an entry.
void DboxMain::OnBrowse()
{
  CItemData *ci = getSelectedItem();
  CItemData *ci_original(ci);

  if(ci != NULL) {
    if (ci->IsShortcut()) {
      // This is an shortcut
      uuid_array_t entry_uuid, base_uuid;
      ci->GetUUID(entry_uuid);
      m_core.GetShortcutBaseUUID(entry_uuid, base_uuid);

      ItemListIter iter = m_core.Find(base_uuid);
      if (iter != End()) {
        ci = &iter->second;
      }
    }

    if (!ci->IsURLEmpty()) {
      LaunchBrowser(ci->GetURL());
      UpdateAccessTime(ci_original);
    }
  }
}

// this tells OnSize that the user is currently
// changing the size of the dialog, and not restoring it
void DboxMain::OnSizing(UINT fwSide, LPRECT pRect)
{
#if !defined(POCKET_PC)
  CDialog::OnSizing(fwSide, pRect);

  m_bSizing = true;
#endif
}

void
DboxMain::OnUpdateNSCommand(CCmdUI *pCmdUI)
{
  // Use this callback  for commands that need to
  // be disabled if not supported (yet)
  pCmdUI->Enable(FALSE);
}

void 
DboxMain::SetStartSilent(bool state)
{
  m_IsStartSilent = state;
  if (state) {
    // start silent implies use system tray.
    PWSprefs::GetInstance()->SetPref(PWSprefs::UseSystemTray, true);  
  }
}

void
DboxMain::SetChanged(ChangeType changed)
{
  switch (changed) {
  case Data:
    if (PWSprefs::GetInstance()->GetPref(PWSprefs::SaveImmediately)) {
      Save();
    } else {
      m_core.SetChanged(true);
    }
    break;
  case Clear:
    m_core.SetChanged(false);
    m_bTSUpdated = false;
    break;
  case TimeStamp:
    if (PWSprefs::GetInstance()->GetPref(PWSprefs::MaintainDateTimeStamps))
      m_bTSUpdated = true;
    break;
  default:
    ASSERT(0);
  }
}

void
DboxMain::ChangeOkUpdate()
{
  if (!m_windowok)
    return;

#if defined(POCKET_PC)
  CMenu *menu	= m_wndMenu;
#else
  CMenu *menu	= GetMenu();
#endif

  // Don't need to worry about R-O, as IsChanged can't be true in this case
  menu->EnableMenuItem(ID_MENUITEM_SAVE,
                       m_core.IsChanged() ? MF_ENABLED : MF_GRAYED);
  if (m_toolbarsSetup == TRUE) {
	m_MainToolBar.GetToolBarCtrl().EnableButton(ID_MENUITEM_SAVE,
	                   m_core.IsChanged() ? TRUE : FALSE);
  }
#ifdef DEMO
  int update = OnUpdateMenuToolbar(ID_MENUITEM_ADD);
  // Cheat, as we know that the logic for ADD applies to others, in DEMO mode
  // see OnUpdateMenuToolbar
  if (m_MainToolBar.GetSafeHwnd() != NULL && update != -1) {
    BOOL state = update ? TRUE : FALSE;
    m_MainToolBar.GetToolBarCtrl().EnableButton(ID_MENUITEM_ADD, state);
    m_MainToolBar.GetToolBarCtrl().EnableButton(ID_MENUITEM_ADDSHORTCUT, state);
    m_MainToolBar.GetToolBarCtrl().EnableButton(ID_MENUITEM_IMPORT_PLAINTEXT, state);
    m_MainToolBar.GetToolBarCtrl().EnableButton(ID_MENUITEM_IMPORT_XML, state);
    m_MainToolBar.GetToolBarCtrl().EnableButton(ID_MENUITEM_MERGE, state);
  }
#endif
  UpdateStatusBar();
}

void
DboxMain::OnAbout()
{
  CAboutDlg about;
  about.DoModal();
}

void
DboxMain::OnPasswordSafeWebsite()
{
  HINSTANCE stat = ::ShellExecute(NULL, NULL, _T("http://passwordsafe.sourceforge.net/"),
                                  NULL, _T("."), SW_SHOWNORMAL);
  if (int(stat) <= 32) {
#ifdef _DEBUG
    AfxMessageBox(_T("oops"));
#endif
  }
}

void
DboxMain::OnU3ShopWebsite()
{
#ifdef DEMO
    ::ShellExecute(NULL, NULL,
                   _T("http://software.u3.com/Product_Details.aspx?productId=294&Selection=7"),
                   NULL, _T("."), SW_SHOWNORMAL);
/*
 * TBD - point to language-specific sites:
 * FRENCH: Same as above plus "&Lang=f"
 * ITALIAN: "&Lang=it"
 * GERMAN:  "&Lang=de"
 * SPANISH: "&Lang=es "
 */
#endif
}

int
DboxMain::GetAndCheckPassword(const CMyString &filename,
                              CMyString& passkey,
                              int index,
                              bool bReadOnly,
                              bool bForceReadOnly,
                              PWScore *pcore,
                              int adv_type)
{
    // index:
    //	GCP_FIRST      (0) first
    //	GCP_NORMAL     (1) OK, CANCEL & HELP buttons
    //	GCP_UNMINIMIZE (2) OK, CANCEL & HELP buttons
    //	GCP_WITHEXIT   (3) OK, CANCEL, EXIT & HELP buttons
    //	GCP_ADVANCED   (4) OK, CANCEL, HELP & ADVANCED buttons

    // Called for an existing database. Prompt user
    // for password, verify against file. Lock file to
    // prevent multiple r/w access.
    int retval;
    bool bFileIsReadOnly = false;

    if (pcore == 0) pcore = &m_core;

    if (!filename.IsEmpty()) {
        bool exists = pcore->FileExists(filename, bFileIsReadOnly);

        if (!exists) {
            // Used to display an error message, but this is really the caller's business
            return PWScore::CANT_OPEN_FILE;
        } // !exists
    } // !filename.IsEmpty()

    /*
     * with my unsightly hacks of PasskeyEntry, it should now accept
     * a blank filename, which will disable passkey entry and the OK button
     */

    if (bFileIsReadOnly || bForceReadOnly) {
        // As file is read-only, we must honour it and not permit user to change it
        pcore->SetReadOnly(true);
    }
    static CPasskeyEntry *dbox_pkentry = NULL;
    INT_PTR rc = 0;
    if (dbox_pkentry == NULL) {
        dbox_pkentry = new CPasskeyEntry(this, filename,
                                         index, bReadOnly || bFileIsReadOnly,
                                         bFileIsReadOnly || bForceReadOnly,
                                         adv_type);

        int nMajor(0), nMinor(0), nBuild(0);
        DWORD dwMajorMinor = app.GetFileVersionMajorMinor();
        DWORD dwBuildRevision = app.GetFileVersionBuildRevision();

        if (dwMajorMinor > 0) {
            nMajor = HIWORD(dwMajorMinor);
            nMinor = LOWORD(dwMajorMinor);
            nBuild = HIWORD(dwBuildRevision);
        }
        if (nBuild == 0)
            dbox_pkentry->m_appversion.Format(_T("Version %d.%02d"),
                                              nMajor, nMinor);
        else
            dbox_pkentry->m_appversion.Format(_T("Version %d.%02d.%02d"),
                                              nMajor, nMinor, nBuild);

        app.DisableAccelerator();
        rc = dbox_pkentry->DoModal();
        app.EnableAccelerator();

        if (rc == IDOK && index == GCP_ADVANCED) {
          m_bAdvanced = dbox_pkentry->m_bAdvanced;
          m_bsFields = dbox_pkentry->m_bsFields;
          m_subgroup_set = dbox_pkentry->m_subgroup_set;
          if (m_subgroup_set == BST_CHECKED) {
            m_subgroup_name = dbox_pkentry->m_subgroup_name;
            m_subgroup_object = dbox_pkentry->m_subgroup_object;
            m_subgroup_function = dbox_pkentry->m_subgroup_function;
          }
        }
    } else { // already present - bring to front
        dbox_pkentry->BringWindowToTop(); // can happen with systray lock
        return PWScore::USER_CANCEL; // multi-thread,
        // original thread will continue processing
    }

    if (rc == IDOK) {
        DBGMSG("PasskeyEntry returns IDOK\n");
        const CString &curFile = dbox_pkentry->GetFileName();
        pcore->SetCurFile(curFile);
        CMyString locker(_T("")); // null init is important here
        passkey = dbox_pkentry->GetPasskey();
        // This dialog's setting of read-only overrides file dialog
        bool bIsReadOnly = dbox_pkentry->IsReadOnly();
        pcore->SetReadOnly(bIsReadOnly);
        // Set read-only mode if user explicitly requested it OR
        // we could not create a lock file.
        switch (index) {
            case GCP_FIRST: // if first, then m_IsReadOnly is set in Open
                pcore->SetReadOnly(bIsReadOnly || !pcore->LockFile(curFile, locker));
                break;
            case GCP_NORMAL:
            case GCP_ADVANCED:
                if (!bIsReadOnly) // !first, lock if !bIsReadOnly
                    pcore->SetReadOnly(!pcore->LockFile(curFile, locker));
                else
                    pcore->SetReadOnly(bIsReadOnly);
                break;
            case GCP_UNMINIMIZE:
            case GCP_WITHEXIT:
            default:
                // user can't change R-O status
                break;
        }
        UpdateToolBar(bIsReadOnly);
        // locker won't be null IFF tried to lock and failed, in which case
        // it shows the current file locker
        if (!locker.IsEmpty()) {
            CString cs_user_and_host, cs_PID;
            cs_user_and_host = (CString)locker;
            int i_pid = cs_user_and_host.ReverseFind(_T(':'));
            if (i_pid > -1) {
                // If PID present then it is ":%08d" = 9 chars in length
                ASSERT((cs_user_and_host.GetLength() - i_pid) == 9);
                cs_PID.Format(IDS_PROCESSID, cs_user_and_host.Right(8));
                cs_user_and_host = cs_user_and_host.Left(i_pid);
            } else
                cs_PID = _T("");
            const CString cs_title(MAKEINTRESOURCE(IDS_FILEINUSE));
            CString cs_msg;
            CGeneralMsgBox gmb;
            gmb.SetTitle(cs_title);
            gmb.SetStandardIcon(MB_ICONQUESTION);
#ifdef PWS_STRICT_LOCKING // define if you don't want to allow user override
            cs_msg.Format(IDS_STRICT_LOCKED, curFile, cs_user_and_host, cs_PID);
            gmb.SetMsg(cs_msg);
            gmb.AddButton(1, _T("Read-Only"));
            gmb.AddButton(3, _T("Exit"), TRUE, TRUE);
#else
            cs_msg.Format(IDS_LOCKED, curFile, cs_user_and_host, cs_PID);
            gmb.SetMsg(cs_msg);
            gmb.AddButton(1, _T("Read-Only"));
            gmb.AddButton(2, _T("Read-Write"));
            gmb.AddButton(3, _T("Exit"), TRUE, TRUE);
#endif
            int user_choice = gmb.DoModal();
            switch(user_choice) {
                case 1:
                    pcore->SetReadOnly(true);
                    UpdateToolBar(true);
                    retval = PWScore::SUCCESS;
                    break;
                case 2:
                    pcore->SetReadOnly(false); // Caveat Emptor!
                    UpdateToolBar(false);
                    retval = PWScore::SUCCESS;
                    break;
                case 3:
                    retval = PWScore::USER_CANCEL;
                    break;
                default:
                    ASSERT(false);
                    retval = PWScore::USER_CANCEL;
            }
        } else { // locker.IsEmpty() means no lock needed or lock was successful
            if (dbox_pkentry->GetStatus() == TAR_NEW) {
                // Save new file
                pcore->NewFile(dbox_pkentry->GetPasskey());
                rc = pcore->WriteCurFile();
                
                if (rc == PWScore::CANT_OPEN_FILE) {
                    CString cs_temp, cs_title(MAKEINTRESOURCE(IDS_FILEWRITEERROR));
                    cs_temp.Format(IDS_CANTOPENWRITING, pcore->GetCurFile());
                    MessageBox(cs_temp, cs_title, MB_OK|MB_ICONWARNING);
                    retval = PWScore::USER_CANCEL;
                } else
                    retval = PWScore::SUCCESS;
            } else // no need to create file
                retval = PWScore::SUCCESS;
        }
    } else {/*if (rc==IDCANCEL) */ //Determine reason for cancel
        int cancelreturn = dbox_pkentry->GetStatus();
        switch (cancelreturn) {
            case TAR_OPEN:
                ASSERT(0); // now handled entirely in CPasskeyEntry
            case TAR_CANCEL:
            case TAR_NEW:
                retval = PWScore::USER_CANCEL;
                break;
            case TAR_EXIT:
                retval = PWScore::USER_EXIT;
                break;
            default:
                DBGMSG("Default to WRONG_PASSWORD\n");
                retval = PWScore::WRONG_PASSWORD;	//Just a normal cancel
                break;
        }
    }
    delete dbox_pkentry;
    dbox_pkentry = NULL;
    return retval;
}

BOOL
DboxMain::OnToolTipText(UINT,
                        NMHDR* pNMHDR,
                        LRESULT* pResult)
// This code is copied from the DLGCBR32 example that comes with MFC
// Updated by MS on 25/09/2005
{
#if !defined(POCKET_PC)
  ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

  // allow top level routing frame to handle the message
  if (GetRoutingFrame() != NULL)
    return FALSE;

  // need to handle both ANSI and UNICODE versions of the message
  TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
  TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
  TCHAR tc_FullText[4096];  // Maxsize of a string in a resource file
  CString cs_TipText;
  UINT nID = pNMHDR->idFrom;
  if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
      pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND)) {
    // idFrom is actually the HWND of the tool
    nID = ((UINT)(WORD)::GetDlgCtrlID((HWND)nID));
  }

  if (nID != 0) { // will be zero on a separator
    if (AfxLoadString(nID, tc_FullText, 4095) == 0)
      return FALSE;

    // this is the command id, not the button index
    if (AfxExtractSubString(cs_TipText, tc_FullText, 1, '\n') == FALSE)
      return FALSE;
  } else
    return FALSE;

  // Assume ToolTip is greater than 80 characters in ALL cases and so use
  // the pointer approach.
  // Otherwise comment out the definition of LONG_TOOLTIPS below

#define LONG_TOOLTIPS

#ifdef LONG_TOOLTIPS
#ifndef _UNICODE
  if(pNMHDR->code == TTN_NEEDTEXTA) {
    delete m_pchTip;

    m_pchTip = new TCHAR[cs_TipText.GetLength() + 1];
    lstrcpyn(m_pchTip, cs_TipText, cs_TipText.GetLength()+1);
    pTTTW->lpszText = (WCHAR*)m_pchTip;
  } else {
    if (cs_TipText.GetLength() > 0) {
      delete m_pwchTip;

      m_pwchTip = new WCHAR[cs_TipText.GetLength() + 1];
#if _MSC_VER >= 1400
      size_t numconverted;
      mbstowcs_s(&numconverted, m_pwchTip, cs_TipText.GetLength() + 1, cs_TipText, cs_TipText.GetLength() + 1);
#else
      mbstowcs(m_pwchTip, cs_TipText, cs_TipText.GetLength() + 1);
#endif
      pTTTW->lpszText = m_pwchTip;
    }
  }
#else
  if(pNMHDR->code == TTN_NEEDTEXTA) {
    delete m_pchTip;

    m_pchTip = new _TCHAR[cs_TipText.GetLength() + 1];
    lstrcpyn(m_pchTip, cs_TipText, cs_TipText.GetLength() + 1);
    pTTTA->lpszText = (LPSTR)m_pchTip;
  } else {
    delete m_pwchTip;

    m_pwchTip = new WCHAR[cs_TipText.GetLength() + 1];
    lstrcpyn(m_pwchTip, cs_TipText, cs_TipText.GetLength() + 1);
    pTTTA->lpszText = (LPSTR)m_pwchTip;
  }
#endif

#else // Short Tooltips!

#ifndef _UNICODE
  if (pNMHDR->code == TTN_NEEDTEXTA)
#if _MSC_VER >= 1400
    _tcsncpy_s(pTTTA->szText, (sizeof(pTTTA->szText)/sizeof(pTTTA->szText[0])),
               cs_TipText, _TRUNCATE);
#else
  _tcsncpy(pTTTA->szText, cs_TipText, (sizeof(pTTTA->szText)/sizeof(pTTTA->szText[0])));
#endif
  else {
    int n = MultiByteToWideChar(CP_ACP, 0, cs_TipText, -1, pTTTW->szText,
                                sizeof(pTTTW->szText)/sizeof(pTTTW->szText[0]));
    if (n > 0)
      pTTTW->szText[n-1] = 0;
  }
#else
  if (pNMHDR->code == TTN_NEEDTEXTA) {
    int n = WideCharToMultiByte(CP_ACP, 0, cs_TipText, -1,
                                pTTTA->szText,
                                sizeof(pTTTA->szText)/sizeof(pTTTA->szText[0]),
                                NULL, NULL);
    if (n > 0)
      pTTTA->szText[n-1] = 0;
  } else
#if _MSC_VER >= 1400
    _tcsncpy_s(pTTTW->szText, (sizeof(pTTTW->szText)/sizeof(pTTTW->szText[0])),
               cs_TipText, _TRUNCATE);
#else
  _tcsncpy(pTTTW->szText, cs_TipText, (sizeof(pTTTW->szText)/sizeof(pTTTW->szText[0])));
#endif
#endif // _UNICODE
#endif // LONG_TOOLTIPS

  *pResult = 0;

  // bring the tooltip window above other popup windows
  ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);
#endif  // POCKET_PC
  return TRUE;    // message was handled
}

#if !defined(POCKET_PC)
void
DboxMain::OnDropFiles(HDROP hDrop)
{
  //SetActiveWindow();
  SetForegroundWindow();

#if 0
  // here's what we really want - sorta
  HDROP m_hDropInfo = hDropInfo;
  CString Filename;

  if (m_hDropInfo) {
    int iFiles = DragQueryFile(m_hDropInfo, (UINT)-1, NULL, 0);
    for (int i=0; i<ifiles; i++) {
      char* pFilename = Filename.GetBuffer(_MAX_PATH);
      // do whatever...
    }   // for each files...
  }       // if DropInfo

  DragFinish(m_hDropInfo);

  m_hDropInfo = 0;
#endif

  DragFinish(hDrop);
}
#endif

void
DboxMain::UpdateAlwaysOnTop()
{
#if !defined(POCKET_PC)
  CMenu*	sysMenu = GetSystemMenu( FALSE );

  if (PWSprefs::GetInstance()->GetPref(PWSprefs::AlwaysOnTop)) {
    SetWindowPos( &wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
    sysMenu->CheckMenuItem( ID_SYSMENU_ALWAYSONTOP, MF_BYCOMMAND | MF_CHECKED );
  } else {
    SetWindowPos( &wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
    sysMenu->CheckMenuItem( ID_SYSMENU_ALWAYSONTOP, MF_BYCOMMAND | MF_UNCHECKED );
  }
#endif
}

void
DboxMain::OnSysCommand( UINT nID, LPARAM lParam )
{
#if !defined(POCKET_PC)
  if ( ID_SYSMENU_ALWAYSONTOP == nID ) {
    PWSprefs *prefs = PWSprefs::GetInstance();
    bool oldAlwaysOnTop = prefs->GetPref(PWSprefs::AlwaysOnTop);
    prefs->SetPref(PWSprefs::AlwaysOnTop, !oldAlwaysOnTop);
    UpdateAlwaysOnTop();
    return;
  }

  if ((nID & 0xFFF0) == SC_RESTORE) {
    UnMinimize(true);
    if (!m_passphraseOK)	// password bad or cancel pressed
      return;
  }

  CDialog::OnSysCommand( nID, lParam );

#endif
}

void
DboxMain::ConfigureSystemMenu()
{
#if defined(POCKET_PC)
  m_wndCommandBar = (CCeCommandBar*) m_pWndEmptyCB;
  m_wndMenu		= m_wndCommandBar->InsertMenuBar( IDR_MAINMENU );

  ASSERT( m_wndMenu != NULL );
#else
  CMenu*	sysMenu = GetSystemMenu( FALSE );
  const CString str(MAKEINTRESOURCE(IDS_ALWAYSONTOP));

  sysMenu->InsertMenu( 5, MF_BYPOSITION | MF_STRING, ID_SYSMENU_ALWAYSONTOP, (LPCTSTR)str );
#endif
}

void
DboxMain::OnUpdateMRU(CCmdUI* pCmdUI)
{
  if (app.GetMRU() == NULL)
  	return;

  if (!app.m_mruonfilemenu) {
    if (pCmdUI->m_nIndex == 0) { // Add to popup menu
      app.GetMRU()->UpdateMenu( pCmdUI );
    } else {
      return;
    }
  } else {
	app.GetMRU()->UpdateMenu( pCmdUI );
  }
}

// Called just before any pulldown or popup menu is displayed, so that menu items
// can be enabled/disabled or checked/unchecked dynamically.
void
DboxMain::OnInitMenu(CMenu* pMenu)
{
  // This routine just changes the text in the menu via "ModifyMenu" and
  // adds the "check" mark via CheckMenuRadioItem for view type and toolbar
  // "EnableMenuItem" is handled by the UPDATE_UI routines
  const bool bTreeView = m_ctlItemTree.IsWindowVisible() == TRUE;
  const bool bItemSelected = (SelItemOk() == TRUE);

  bool bGroupSelected = false;
  if (bTreeView) {
    HTREEITEM hi = m_ctlItemTree.GetSelectedItem();
    bGroupSelected = (hi != NULL && !m_ctlItemTree.IsLeaf(hi));
  }

  if (bGroupSelected) {
    pMenu->ModifyMenu(ID_MENUITEM_DELETE, MF_BYCOMMAND,
                      ID_MENUITEM_DELETE, CS_DELETEGROUP);
    pMenu->ModifyMenu(ID_MENUITEM_RENAME, MF_BYCOMMAND,
                      ID_MENUITEM_RENAME, CS_RENAMEGROUP);
    pMenu->ModifyMenu(ID_MENUITEM_EDIT, MF_BYCOMMAND,
                      ID_MENUITEM_GROUPENTER, CS_EXPCOLGROUP);
  } else {
    pMenu->ModifyMenu(ID_MENUITEM_DELETE, MF_BYCOMMAND,
                      ID_MENUITEM_DELETE, CS_DELETEENTRY);
    pMenu->ModifyMenu(ID_MENUITEM_RENAME, MF_BYCOMMAND,
                      ID_MENUITEM_RENAME, CS_RENAMEENTRY);
    if (m_core.IsReadOnly()) {
      // Do both - not sure what was last selected!
      pMenu->ModifyMenu(ID_MENUITEM_EDIT, MF_BYCOMMAND,
                        ID_MENUITEM_EDIT, CS_VIEWENTRY);
      pMenu->ModifyMenu(ID_MENUITEM_GROUPENTER, MF_BYCOMMAND,
                        ID_MENUITEM_EDIT, CS_VIEWENTRY);
    } else {
      // Do both - not sure what was last selected!
      pMenu->ModifyMenu(ID_MENUITEM_EDIT, MF_BYCOMMAND,
                        ID_MENUITEM_EDIT, CS_EDITENTRY);
      pMenu->ModifyMenu(ID_MENUITEM_GROUPENTER, MF_BYCOMMAND,
                        ID_MENUITEM_EDIT, CS_EDITENTRY);
    }
  }

  if (bItemSelected) {
    CItemData *ci = getSelectedItem();
    ASSERT(ci != NULL);

    if (ci->IsShortcut()) {
      // This is an shortcut
      uuid_array_t entry_uuid, base_uuid;
      ci->GetUUID(entry_uuid);
      m_core.GetShortcutBaseUUID(entry_uuid, base_uuid);

      ItemListIter iter = m_core.Find(base_uuid);
      if (iter != End()) {
        ci = &iter->second;
      }
    }

    if (!ci->IsURLEmpty()) {
      const bool bIsEmail = ci->GetURL().Find(_T("mailto:")) != -1;
      if (bIsEmail) {
        pMenu->ModifyMenu(ID_MENUITEM_BROWSEURL, MF_BYCOMMAND,
                          ID_MENUITEM_SENDEMAIL, CS_SENDEMAIL);
      } else {
        pMenu->ModifyMenu(ID_MENUITEM_SENDEMAIL, MF_BYCOMMAND,
                          ID_MENUITEM_BROWSEURL, CS_BROWSEURL);
      }
      UpdateBrowseURLSendEmailButton(bIsEmail);
    } else {
      pMenu->ModifyMenu(ID_MENUITEM_SENDEMAIL, MF_BYCOMMAND,
                        ID_MENUITEM_BROWSEURL, CS_BROWSEURL);
    }
  }

  pMenu->CheckMenuRadioItem(ID_MENUITEM_LIST_VIEW, ID_MENUITEM_TREE_VIEW,
                            (bTreeView ? ID_MENUITEM_TREE_VIEW : ID_MENUITEM_LIST_VIEW), MF_BYCOMMAND);
  
  // JHF m_toolbarMode is not for WinCE (as in .h)
#if !defined(POCKET_PC)
  pMenu->CheckMenuRadioItem(ID_MENUITEM_NEW_TOOLBAR, ID_MENUITEM_OLD_TOOLBAR,
                            m_toolbarMode, MF_BYCOMMAND);
#endif
}

// helps with MRU by allowing ON_UPDATE_COMMAND_UI
void
DboxMain::OnInitMenuPopup(CMenu* pPopupMenu, UINT, BOOL)
{
  // http://www4.ncsu.edu:8030/~jgbishop/codetips/dialog/updatecommandui_menu.html
  // This code comes from the MFC Documentation, and is adapted from CFrameWnd::OnInitMenuPopup() in WinFrm.cpp.
  ASSERT(pPopupMenu != NULL);
  CCmdUI state; // Check the enabled state of various menu items
  state.m_pMenu = pPopupMenu;
  ASSERT(state.m_pOther == NULL);
  ASSERT(state.m_pParentMenu == NULL);

  // Is the menu in question a popup in the top-level menu? If so, set m_pOther
  // to this menu. Note that m_pParentMenu == NULL indicates that the menu is a
  // secondary popup.
  CMenu *hParentMenu;
  if(AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu) {
    state.m_pParentMenu = pPopupMenu; // Parent == child for tracking popup.
  } else if((hParentMenu = this->GetMenu()) != NULL) {
    CWnd* pParent = this;
    // Child windows don't have menus--need to go to the top!
    if(pParent != NULL && (hParentMenu = pParent->GetMenu()) != NULL) {
      int nIndexMax = hParentMenu->GetMenuItemCount();
      for (int nIndex = 0; nIndex < nIndexMax; nIndex++) {
        CMenu *submenu = hParentMenu->GetSubMenu(nIndex);
        if(submenu != NULL && submenu->m_hMenu == pPopupMenu->m_hMenu) {
          // When popup is found, m_pParentMenu is containing menu.
          state.m_pParentMenu = CMenu::FromHandle(hParentMenu->GetSafeHmenu());
          break;
        }
      }
    }
  }

  state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
  for(state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++) {
    state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
    if(state.m_nID == 0)
      continue; // Menu separator or invalid cmd - ignore it.
    ASSERT(state.m_pOther == NULL);
    ASSERT(state.m_pMenu != NULL);
    if(state.m_nID == (UINT)-1) {
      // Possibly a popup menu, route to first item of that popup.
      state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
      if(state.m_pSubMenu == NULL ||
         (state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
         state.m_nID == (UINT)-1) {
        continue; // First item of popup can't be routed to.
      }
      state.DoUpdate(this, TRUE); // Popups are never auto disabled.
    } else {
      // Normal menu item.
      // Auto enable/disable if frame window has m_bAutoMenuEnable
      // set and command is _not_ a system command.
      state.m_pSubMenu = NULL;
      state.DoUpdate(this, FALSE);
    }

    // Adjust for menu deletions and additions.
    UINT nCount = pPopupMenu->GetMenuItemCount();
    if(nCount < state.m_nIndexMax) {
      state.m_nIndex -= (state.m_nIndexMax - nCount);
      while(state.m_nIndex < nCount &&
            pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID) {
        state.m_nIndex++;
      }
    }
    state.m_nIndexMax = nCount;
  }

  if (!m_bImageInLV)
    return;

  MENUINFO minfo;
  memset(&minfo, 0x00, sizeof(minfo));
  minfo.cbSize = sizeof(minfo);
  minfo.fMask = MIM_MENUDATA;
  pPopupMenu->GetMenuInfo(&minfo);
  if (minfo.dwMenuData != 1)  // Set by SystemTray for its RUE list menu only
    return;

  minfo.fMask = MIM_STYLE;
  minfo.dwStyle = MNS_CHECKORBMP | MNS_AUTODISMISS;
  pPopupMenu->SetMenuInfo(&minfo);

  MENUITEMINFO miinfo;
  memset(&miinfo, 0x00, sizeof(miinfo));
  miinfo.cbSize = sizeof(miinfo);
  CRUEItemData *pmd;

  for (UINT pos = 0; pos < pPopupMenu->GetMenuItemCount(); pos++) {
    miinfo.fMask = MIIM_FTYPE | MIIM_DATA;
    pPopupMenu->GetMenuItemInfo(pos, &miinfo, TRUE);
    pmd = (CRUEItemData *)miinfo.dwItemData;

    if (pmd && pmd->IsRUEID() && !(miinfo.fType & MFT_OWNERDRAW)) {
      miinfo.fMask = MIIM_FTYPE | MIIM_BITMAP;
      miinfo.hbmpItem = HBMMENU_CALLBACK;
      miinfo.fType = MFT_STRING;

      pPopupMenu->SetMenuItemInfo(pos, &miinfo, TRUE);
    }
  }
}

#if defined(POCKET_PC)
void
DboxMain::OnShowPassword()
{
  if (SelItemOk() == TRUE) {
    CItemData			item;
    CMyString			password;
    CMyString			name;
    CMyString			title;
    CMyString			username;
    CShowPasswordDlg	pwDlg( this );

    item	= m_pwlist.GetAt( Find(getSelectedItem()) );

    item.GetPassword(password);
    item.GetName( name );

    SplitName( name, title, username );

    pwDlg.SetTitle( title );
    pwDlg.SetPassword( password );
    pwDlg.DoModal();
  }
}
#endif

LRESULT DboxMain::OnTrayNotification(WPARAM , LPARAM )
{
#if 0
  return m_TrayIcon.OnTrayNotification(wParam, lParam);
#else
  return 0L;
#endif
}

void
DboxMain::OnMinimize()
{
  // Called when the System Tray Minimize menu option is used
  if (m_bStartHiddenAndMinimized)
	  m_bStartHiddenAndMinimized = false;

  ShowWindow(SW_MINIMIZE);
}

void
DboxMain::OnUnMinimize()
{
  // Called when the System Tray Restore menu option is used
  UnMinimize(true);
}

void
DboxMain::UnMinimize(bool update_windows)
{
  m_passphraseOK = false;
  if (!m_bOpen) {
    // first they may be nothing to do!
    if (update_windows) {
      if (m_IsStartSilent) {
        // Show initial dialog ONCE (if succeeds)
        if (!m_IsStartClosed) {
          if (OpenOnInit()) {
            m_IsStartSilent = false;
            RefreshViews();
            ShowWindow(SW_RESTORE);
            SetInitialDatabaseDisplay();
            UpdateSystemTray(UNLOCKED);
          }
        } else { // m_IsStartClosed (&& m_IsStartSilent)
          m_IsStartClosed = m_IsStartSilent = false;
          ShowWindow(SW_RESTORE);
          UpdateSystemTray(UNLOCKED);
        }
        return;
      } // m_IsStartSilent
      ShowWindow(SW_RESTORE);
    } // update_windows
    UpdateSystemTray(CLOSED);
    return;
  }

  // Case 1 - data available but is currently locked
  if (!m_needsreading
      && (app.GetSystemTrayState() == ThisMfcApp::LOCKED)
      && (PWSprefs::GetInstance()->GetPref(PWSprefs::UseSystemTray))) {

    CMyString passkey;
    int rc;
    rc = GetAndCheckPassword(m_core.GetCurFile(), passkey, GCP_UNMINIMIZE);  // OK, CANCEL, HELP
    if (rc != PWScore::SUCCESS)
      return;  // don't even think of restoring window!

    app.SetSystemTrayState(ThisMfcApp::UNLOCKED);
    m_passphraseOK = true;
    if (update_windows) {
      RefreshViews();
      ShowWindow(SW_RESTORE);
    }
    return;
  }

  // Case 2 - data unavailable
  if (m_needsreading && m_windowok) {
    CMyString passkey, temp;
    int rc, rc2;
    const bool useSysTray = PWSprefs::GetInstance()->
      GetPref(PWSprefs::UseSystemTray);

    rc = PWScore::USER_CANCEL;
    if (m_bOpen)
      rc = GetAndCheckPassword(m_core.GetCurFile(),
                               passkey,
                               useSysTray ? GCP_UNMINIMIZE : GCP_WITHEXIT,
                               m_core.IsReadOnly());
    CString cs_temp, cs_title;
    switch (rc) {
    case PWScore::SUCCESS:
      rc2 = m_core.ReadCurFile(passkey);
#if !defined(POCKET_PC)
      m_titlebar = NormalizeTTT(CMyString(_T("Password Safe - ")) + m_core.GetCurFile());
#endif
      break;
    case PWScore::CANT_OPEN_FILE:
      cs_temp.Format(IDS_CANTOPEN, m_core.GetCurFile());
      cs_title.LoadString(IDS_FILEOPEN);
      MessageBox(cs_temp, cs_title, MB_OK|MB_ICONWARNING);
    case TAR_NEW:
      rc2 = New();
      break;
    case TAR_OPEN:
      rc2 = Open();
      break;
    case PWScore::WRONG_PASSWORD:
      rc2 = PWScore::NOT_SUCCESS;
      break;
    case PWScore::USER_CANCEL:
      rc2 = PWScore::NOT_SUCCESS;
      break;
    case PWScore::USER_EXIT:
      m_core.UnlockFile(m_core.GetCurFile());
      PostQuitMessage(0);
      return;
    default:
      rc2 = PWScore::NOT_SUCCESS;
      break;
    }

    if (rc2 == PWScore::SUCCESS) {
      UpdateSystemTray(UNLOCKED);
      startLockCheckTimer();
      m_passphraseOK = true;
      m_needsreading = false;
      if (update_windows) {
        ShowWindow(SW_RESTORE);
        RestoreDisplayStatus();
        BringWindowToTop();
      }
    } else {
      ShowWindow(useSysTray ? SW_HIDE : SW_MINIMIZE);
    }
    return;
  }
  if (update_windows) {
    ShowWindow(SW_RESTORE);
    RestoreDisplayStatus();
    BringWindowToTop();
  }
}

void
DboxMain::startLockCheckTimer(){
  const UINT INTERVAL = 5000; // every 5 seconds should suffice

  if (PWSprefs::GetInstance()->
      GetPref(PWSprefs::LockOnWindowLock )==TRUE){
    SetTimer(TIMER_CHECKLOCK, INTERVAL, NULL);
  }
}

BOOL
DboxMain::PreTranslateMessage(MSG* pMsg)
{
  // Do NOT pass the ESC along if preference EscExits is false.
  if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE &&
      !PWSprefs::GetInstance()->
      GetPref(PWSprefs::EscExits)) {
    return TRUE;
  }
  return CDialog::PreTranslateMessage(pMsg);
}

void
DboxMain::ResetIdleLockCounter()
{
  m_IdleLockCountDown = PWSprefs::GetInstance()->
    GetPref(PWSprefs::IdleTimeout);

}

bool
DboxMain::DecrementAndTestIdleLockCounter()
{
  if (m_IdleLockCountDown > 0)
    return (--m_IdleLockCountDown == 0);
  else
    return false; // so we return true only once if idle
}

LRESULT
DboxMain::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  static DWORD last_t = 0;
  DWORD t = GetTickCount();
  if (t != last_t) {
    PWSrand::GetInstance()->AddEntropy((unsigned char *)&t, sizeof(t));
    last_t = t;
  }
  // list of all the events that signify actual user activity, as opposed
  // to Windows internal events...
  if (message == WM_KEYDOWN ||
      message == WM_COMMAND ||
      message == WM_SYSCOMMAND ||
      message == WM_MOUSEMOVE ||
      message == WM_MOVE ||
      message == WM_LBUTTONDOWN ||
      message == WM_LBUTTONDBLCLK ||
      message == WM_CONTEXTMENU ||
	  // JHF undeclared identifier -> removed to get code to compile
#if !defined(POCKET_PC)
      message == WM_MENUSELECT ||
#endif
      message == WM_VSCROLL)
    ResetIdleLockCounter();

  if (message == WM_SYSCOLORCHANGE ||
      message == WM_SETTINGCHANGE)
    RefreshImages();

  return CDialog::WindowProc(message, wParam, lParam);
}

void
DboxMain::RefreshImages()
{
  m_MainToolBar.RefreshImages();
  m_FindToolBar.RefreshImages();
  m_menuManager.SetImageList(&m_MainToolBar);
}

void
DboxMain::CheckExpiredPasswords()
{
  time_t now, exptime, LTime;
  time(&now);

  if (PWSprefs::GetInstance()->GetPref(PWSprefs::PreExpiryWarn)) {
    int idays = PWSprefs::GetInstance()->GetPref(PWSprefs::PreExpiryWarnDays);
    struct tm st;
#if _MSC_VER >= 1400
    errno_t err;
    err = localtime_s(&st, &now);  // secure version
    ASSERT(err == 0);
#else
    st = *localtime(&now);
#endif
    st.tm_mday += idays;
    exptime = mktime(&st);
    if (exptime == (time_t)-1)
      exptime = now;
  } else
    exptime = now;

  ExpiredList expPWList;

  ItemListConstIter listPos;
  for (listPos = m_core.GetEntryIter();
       listPos != m_core.GetEntryEndIter();
       listPos++) {
    const CItemData &curitem = m_core.GetEntry(listPos);
    if (curitem.IsAlias())
      continue;

    curitem.GetLTime(LTime);

    if (((long)LTime != 0) && (LTime < exptime)) {
      ExpPWEntry exppwentry(curitem, now, LTime);
      expPWList.push_back(exppwentry);
    }
	}

  if (!expPWList.empty()) {
    CExpPWListDlg dlg(this, expPWList, m_core.GetCurFile());
    dlg.DoModal();
  }
}

void
DboxMain::UpdateAccessTime(CItemData *ci)
{
  // Mark access time if so configured
  ASSERT(ci != NULL);

  // First add to RUE List
  uuid_array_t RUEuuid;
  ci->GetUUID(RUEuuid);
  m_RUEList.AddRUEntry(RUEuuid);

  bool bMaintainDateTimeStamps = PWSprefs::GetInstance()->
    GetPref(PWSprefs::MaintainDateTimeStamps);

  if (!m_core.IsReadOnly() && bMaintainDateTimeStamps) {
    ci->SetATime();
    SetChanged(TimeStamp);
    // Need to update view if there
    if (m_nColumnIndexByType[CItemData::ATIME] != -1) {
      // Get index of entry
      DisplayInfo *di = (DisplayInfo *)ci->GetDisplayInfo();
      // Get value in correct format
      CString cs_atime = ci->GetATimeL();
      // Update it
      m_ctlItemList.SetItemText(di->list_index,
                                m_nColumnIndexByType[CItemData::ATIME], cs_atime);
    }
  }
}

BOOL
DboxMain::OnQueryEndSession()
{
	m_iSessionEndingStatus = IDOK;

  PWSprefs *prefs = PWSprefs::GetInstance();
  if (!m_core.GetCurFile().IsEmpty())
    prefs->SetPref(PWSprefs::CurrentFile, m_core.GetCurFile());
  // Save Application related preferences
  prefs->SaveApplicationPreferences();

	if (m_core.IsReadOnly())
		return TRUE;

  bool autoSave = true; // false if user saved or decided not to 
  BOOL retval = TRUE;

	if (m_core.IsChanged()) {
		CString cs_msg;
		cs_msg.Format(IDS_SAVECHANGES, m_core.GetCurFile());
		m_iSessionEndingStatus = AfxMessageBox(cs_msg,
                                           (MB_ICONWARNING |
                                            MB_YESNOCANCEL | MB_DEFBUTTON3));
		switch (m_iSessionEndingStatus) {
    case IDCANCEL:
      // Cancel shutdown\restart\logoff
      return FALSE;
    case IDYES:
      // Save the changes and say OK to shutdown\restart\logoff
      Save();
      autoSave = false;
      retval = TRUE;
      break;
    case IDNO:
      // Don't save the changes but say OK to shutdown\restart\logoff
      autoSave = false;
      retval = TRUE;
      break;
		}
	} // database was changed

  if (autoSave) {
    //Store current filename for next time
    if ((m_bTSUpdated || m_core.WasDisplayStatusChanged())
        && m_core.GetNumEntries() > 0) {
      Save();
      return TRUE;
    }
  }

	return retval;
}

void
DboxMain::OnEndSession(BOOL bEnding)
{
	if (bEnding == TRUE) {
		switch (m_iSessionEndingStatus) {
			case IDOK:
				// Either not changed, R/O or already saved timestamps
			case IDCANCEL:
				// Shouldn't happen as the user said NO to shutdown\restart\logoff
			case IDNO:
				// User said NO to saving - so don't!
				break;
			case IDYES:
				// User said YES they wanted to save - so do!
				OnOK();
				break;
			default:
				ASSERT(0);
		}
	} else {
		// Reset status since the EndSession was cancelled
		m_iSessionEndingStatus = IDIGNORE;
	}
}

void
DboxMain::UpdateStatusBar()
{
  if (m_toolbarsSetup == TRUE) {
    CString s;
  	if (m_bOpen) {
      s = m_core.IsChanged() ? _T("*") : _T(" ");
      m_statusBar.SetPaneText(SB_MODIFIED, s);
      s = m_core.IsReadOnly() ? _T("R-O") : _T("R/W");
      m_statusBar.SetPaneText(SB_READONLY, s);
      s.Format(IDS_NUMITEMS, m_core.GetNumEntries());
      m_statusBar.SetPaneText(SB_NUM_ENT, s);
    } else {
      s.LoadString(IDS_STATCOMPANY);
      m_statusBar.SetPaneText(SB_DBLCLICK, s);
      m_statusBar.SetPaneText(SB_MODIFIED, _T(" "));
      m_statusBar.SetPaneText(SB_READONLY, _T(" "));
      m_statusBar.SetPaneText(SB_NUM_ENT, _T(" "));
    }
  }
  /*
    This doesn't exactly belong here, but it makes sure that the
    title is fresh...
  */
#if !defined(POCKET_PC)
  SetWindowText(LPCTSTR(m_titlebar));
#endif
}

void
DboxMain::SetDCAText()
{
	const int dca = int(PWSprefs::GetInstance()->
			GetPref(PWSprefs::DoubleClickAction));
	int i_dca_text;
	switch (dca) {
		case PWSprefs::DoubleClickAutoType: i_dca_text = IDS_STATAUTOTYPE; break;
		case PWSprefs::DoubleClickBrowse: i_dca_text = IDS_STATBROWSE; break;
		case PWSprefs::DoubleClickCopyNotes: i_dca_text = IDS_STATCOPYNOTES; break;
		case PWSprefs::DoubleClickCopyPassword: i_dca_text = IDS_STATCOPYPASSWORD; break;
		case PWSprefs::DoubleClickCopyUsername: i_dca_text = IDS_STATCOPYUSERNAME; break;
		case PWSprefs::DoubleClickViewEdit: i_dca_text = IDS_STATVIEWEDIT; break;
		default: i_dca_text = IDS_STATCOMPANY;
	}
	CString s;
	s.LoadString(i_dca_text);
	m_statusBar.SetPaneText(SB_DBLCLICK, s);
}

// Returns a list of entries as they appear in tree in DFS order
void DboxMain::MakeOrderedItemList(OrderedItemList &il)
{
  // Walk the Tree!
  HTREEITEM hItem = NULL;
  while ( NULL != (hItem = m_ctlItemTree.GetNextTreeItem(hItem)) ) {
    if (!m_ctlItemTree.ItemHasChildren(hItem)) {
      CItemData *ci = (CItemData *)m_ctlItemTree.GetItemData(hItem);
      if (ci != NULL) {// NULL if there's an empty group [bug #1633516]
        il.push_back(*ci);
      }
    }
  }
}

// Returns the number of children of this group
int DboxMain::CountChildren(HTREEITEM hStartItem)
{
  // Walk the Tree!
  int num = 0;
  if (hStartItem != NULL && m_ctlItemTree.ItemHasChildren(hStartItem)) {
    HTREEITEM hChildItem = m_ctlItemTree.GetChildItem(hStartItem);

    while (hChildItem != NULL) {
      if (m_ctlItemTree.ItemHasChildren(hChildItem)) {
        num += CountChildren(hChildItem);
        hChildItem = m_ctlItemTree.GetNextSiblingItem(hChildItem);
      } else {
        hChildItem = m_ctlItemTree.GetNextSiblingItem(hChildItem);
        num++;
      }
    }
  }
  return num;
}

void
DboxMain::UpdateMenuAndToolBar(const bool bOpen)
{
  // Initial setup of menu items and toolbar buttons
	// First set new open/close status
	m_bOpen = bOpen;

	// For open/close
	const UINT imenuflags = bOpen ? MF_ENABLED : MF_DISABLED | MF_GRAYED;

	// Change Main Menus if a database is Open or not
	CWnd* pMain = AfxGetMainWnd();
	CMenu* xmainmenu = pMain->GetMenu();

  if (m_bOpen) {
    for (unsigned int i = 0; i < xmainmenu->GetMenuItemCount(); i++) {
      xmainmenu->EnableMenuItem(i, MF_BYPOSITION | MF_ENABLED);
    }
  }

	// Look for "File" menu.
	CString cs_text;
	cs_text.LoadString(IDS_FILEMENU);
	int pos = app.FindMenuItem(xmainmenu, cs_text);
	if (pos == -1) // Couldn't find it - wrong language?
		pos = 0; // best guess...

	CMenu* xfilesubmenu = xmainmenu->GetSubMenu(pos);
	if (xfilesubmenu != NULL) {	// Look for "Save As"
		pos = app.FindMenuItem(xfilesubmenu, ID_MENUITEM_SAVEAS);
		// Disable/enable Export and Import menu items (skip over separator)
		xfilesubmenu->EnableMenuItem(pos + 2, MF_BYPOSITION | imenuflags);
		xfilesubmenu->EnableMenuItem(pos + 3, MF_BYPOSITION | imenuflags);
  }

	// Look for "Edit" menu.
	cs_text.LoadString(IDS_EDITMENU);
	pos = app.FindMenuItem(xmainmenu, cs_text);
	if (pos == -1) // Couldn't find it - wrong language?
		pos = 1; // best guess...
  xmainmenu->EnableMenuItem(pos, MF_BYPOSITION | imenuflags);

	// Look for "View" menu.
	cs_text.LoadString(IDS_VIEWMENU);
	pos = app.FindMenuItem(xmainmenu, cs_text);
	if (pos == -1) // Couldn't find it - wrong language?
		pos = 2; // best guess...
  xmainmenu->EnableMenuItem(pos, MF_BYPOSITION | imenuflags);

	// Look for "Manage" menu.
  cs_text.LoadString(IDS_MANAGEMENU);
	pos = app.FindMenuItem(xmainmenu, cs_text);
	if (pos == -1) // Couldn't find it - wrong language?
		pos = 3; // best guess...
  xfilesubmenu = xmainmenu->GetSubMenu(pos);

	// Disable/enable menu items:
	//   "Change Safe Combination", "Make Backup" & "Restore from Backup"
	xfilesubmenu->EnableMenuItem(ID_MENUITEM_CHANGECOMBO, MF_BYCOMMAND | imenuflags);
	xfilesubmenu->EnableMenuItem(ID_MENUITEM_BACKUPSAFE, MF_BYCOMMAND | imenuflags);
	xfilesubmenu->EnableMenuItem(ID_MENUITEM_RESTORE, MF_BYCOMMAND | imenuflags);

	if (m_toolbarsSetup == TRUE) {
    TBBUTTONINFO tbinfo;
    int i, nCount, iEnable;
    CToolBarCtrl &tbCtrl = m_MainToolBar.GetToolBarCtrl();
    nCount = tbCtrl.GetButtonCount();

    memset(&tbinfo, 0x00, sizeof(tbinfo));
    tbinfo.cbSize = sizeof(tbinfo);
    tbinfo.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE;

    for (i = 0; i < nCount; i++) {
      tbCtrl.GetButtonInfo(i, &tbinfo);

      if (tbinfo.fsStyle & TBSTYLE_SEP || 
          tbinfo.idCommand < ID_MENUTOOLBAR_START ||
          tbinfo.idCommand > ID_MENUTOOLBAR_END)
        continue;

      iEnable = OnUpdateMenuToolbar(tbinfo.idCommand);
      if (iEnable < 0)
        continue;
      tbCtrl.EnableButton(tbinfo.idCommand, iEnable);
    }

    if (m_FindToolBar.IsVisible() && !bOpen) {
      OnHideFindToolBar();
    }
	}
}

void DboxMain::U3ExitNow()
{
    // Here upon "soft eject" from U3 device
    if (OnQueryEndSession()) {
        m_inExit = true;
        PostQuitMessage(0);
    }
}

void
DboxMain::OnUpdateMenuToolbar(CCmdUI *pCmdUI)
{
  int iEnable(-1);

  switch (pCmdUI->m_nID) {
    // Set menu text to "UnLock" or "Lock" according to state
    case ID_MENUITEM_TRAYUNLOCK:
    case ID_MENUITEM_TRAYLOCK:
      {
        const int i_state = app.GetSystemTrayState();
        switch (i_state) {
          case ThisMfcApp::UNLOCKED:
          case ThisMfcApp::LOCKED:
            break;
          case ThisMfcApp::CLOSED:
            {
              const CString csClosed(MAKEINTRESOURCE(IDS_NOSAFE));
              pCmdUI->SetText(csClosed);
              iEnable = FALSE;
            }
            break;
          default:
            ASSERT(0);
            break;
        }
        if (i_state != ThisMfcApp::CLOSED) {
          // If dialog visible - obviously unlocked and no need to have option to lock
          iEnable = this->IsWindowVisible() == FALSE ? TRUE : FALSE;
        }
      }
      break;
    // Enable/Disable menu item depending on data supplied
    case ID_MENUITEM_CLEARRECENTENTRIES:
      if (pCmdUI->m_pSubMenu != NULL) {
        // disable or enable entire popup for "Recent Entries"
        // CCmdUI::Enable is a no-op for this case, so we
        //   must do what it would have done.
        pCmdUI->m_pMenu->EnableMenuItem(pCmdUI->m_nIndex, MF_BYPOSITION |
           (app.GetSystemTrayState() == ThisMfcApp::UNLOCKED ? MF_ENABLED : MF_GRAYED));
        return;
      }
      // otherwise enable
      iEnable = m_RUEList.GetCount() != 0 ? TRUE : FALSE;
      break;
    default:
      // "Standard" processing for everything else!!!
      iEnable = OnUpdateMenuToolbar(pCmdUI->m_nID);
  }
  if (iEnable < 0)
    return;

  pCmdUI->Enable(iEnable);
}

int
DboxMain::OnUpdateMenuToolbar(const UINT nID)
{
  // Return codes:
  // = -1       : don't set pCmdUI->Enable
  // = FALSE(0) : set pCmdUI-Enable(FALSE)
  // = TRUE(1)  : set pCmdUI-Enable(TRUE)

  MapUICommandTableConstIter it;
  it = m_MapUICommandTable.find(nID);

  if (it == m_MapUICommandTable.end()) {
    // Don't have it - allow by default
    ASSERT(0); // means m_UICommandTable is out of date!
    return TRUE;
  }

  int item, iEnable;
  if (!m_bOpen)
    item = UICommandTableEntry::InClosed;
  else if (m_core.IsReadOnly())
    item = UICommandTableEntry::InOpenRO;
  else
    item = UICommandTableEntry::InOpenRW;

  if (item == UICommandTableEntry::InOpenRW && m_core.GetNumEntries() == 0)
    item = UICommandTableEntry::InEmpty;  // OpenRW + empty

  iEnable = m_UICommandTable[it->second].bOK[item] ? TRUE : FALSE;

  // All following special processing will only ever DISABLE an item
  // The previous lookup table is the only mechanism to ENABLE an item

  const bool bTreeView = m_ctlItemTree.IsWindowVisible() == TRUE;
  bool bGroupSelected = false;
  if (bTreeView) {
    HTREEITEM hi = m_ctlItemTree.GetSelectedItem();
    bGroupSelected = (hi != NULL && !m_ctlItemTree.IsLeaf(hi));
  }

  // Special processing!
  switch (nID) {
    // Items not allowed if a Group is selected
  case ID_MENUITEM_DUPLICATEENTRY:
  case ID_MENUITEM_COPYPASSWORD:
  case ID_MENUITEM_COPYUSERNAME:
  case ID_MENUITEM_COPYNOTESFLD:
  case ID_MENUITEM_AUTOTYPE:
  case ID_MENUITEM_EDIT:
#if defined(POCKET_PC)
  case ID_MENUITEM_SHOWPASSWORD:
#endif
    if (bGroupSelected)
      iEnable = FALSE;
    break;
    // Not allowed if Group selected or the item selected has an empty URL
  case ID_MENUITEM_BROWSEURL:
  case ID_MENUITEM_SENDEMAIL:
  case ID_MENUITEM_COPYURL:
    if (bGroupSelected) {
      // Not allowed if a Group is selected
      iEnable = FALSE;
    } else {
      CItemData *ci = getSelectedItem();
      if (ci == NULL) {
        iEnable = FALSE;
      } else {
        if (ci->IsShortcut()) {
          // This is an shortcut
          uuid_array_t entry_uuid, base_uuid;
          ci->GetUUID(entry_uuid);
          m_core.GetShortcutBaseUUID(entry_uuid, base_uuid);

          ItemListIter iter = m_core.Find(base_uuid);
          if (iter != End()) {
            ci = &iter->second;
          }
        }

        if (ci->IsURLEmpty()) {
          iEnable = FALSE;
        }
      }
    }
    break;
    // Items not allowed in List View
  case ID_MENUITEM_ADDGROUP:
  case ID_MENUITEM_RENAME:
  case ID_MENUITEM_EXPANDALL:
  case ID_MENUITEM_COLLAPSEALL:
    if (m_IsListView)
      iEnable = FALSE;
    break;
    // If not changed, no need to allow Save!
  case ID_MENUITEM_SAVE:
    if (!m_core.IsChanged())
      iEnable = FALSE;
    break;
    // Special processing for viewing reports, if they exist
  case ID_MENUITEM_REPORT_COMPARE:
  case ID_MENUITEM_REPORT_IMPORTTEXT:
  case ID_MENUITEM_REPORT_IMPORTXML:
  case ID_MENUITEM_REPORT_MERGE:
  case ID_MENUITEM_REPORT_VALIDATE:
    iEnable = OnUpdateViewReports(nID);
    break;
    // Disable choice of toolbar if at low resolution
  case ID_MENUITEM_OLD_TOOLBAR:
  case ID_MENUITEM_NEW_TOOLBAR:
    {
#if !defined(POCKET_PC)
      // JHF m_toolbarMode is not for WinCE (as in .h)
      CDC* pDC = this->GetDC();
      int NumBits = (pDC ? pDC->GetDeviceCaps(12 /*BITSPIXEL*/) : 32);
      if (NumBits < 16 && m_toolbarMode == ID_MENUITEM_OLD_TOOLBAR) {
        // Less that 16 color bits available, no choice, disable menu items
        iEnable = FALSE;
      }
#endif
    }
    break;
    // Disable Minimize if already minimized
  case ID_MENUITEM_MINIMIZE:
    {
      WINDOWPLACEMENT wndpl;
      GetWindowPlacement(&wndpl);
      if (wndpl.showCmd == SW_SHOWMINIMIZED)
        iEnable = FALSE;
    }
    break;
    // Disable Restore if already visible
  case ID_MENUITEM_UNMINIMIZE:
    {
      WINDOWPLACEMENT wndpl;
      GetWindowPlacement(&wndpl);
      if (wndpl.showCmd != SW_SHOWMINIMIZED)
        iEnable = FALSE;
    }
    break;
    // Set the state of the "Case Sensitivity" button
  case ID_TOOLBUTTON_FINDCASE:
  case ID_TOOLBUTTON_FINDCASE_I:
  case ID_TOOLBUTTON_FINDCASE_S:
    m_FindToolBar.GetToolBarCtrl().CheckButton(m_FindToolBar.IsFindCaseSet() ?
                                               ID_TOOLBUTTON_FINDCASE_S : ID_TOOLBUTTON_FINDCASE_I, 
                                               m_FindToolBar.IsFindCaseSet());
    return -1;
  default:
    break;
  }
  // Last but not least, DEMO build support:
#ifdef DEMO
  if (!m_core.IsReadOnly()) {
    bool isLimited = (m_core.GetNumEntries() >= MAXDEMO);
    if (isLimited) {
      switch (nID) {
      case ID_MENUITEM_ADD:
      case ID_MENUITEM_ADDSHORTCUT:
      case ID_MENUITEM_ADDGROUP:
      case ID_MENUITEM_DUPLICATEENTRY:
      case ID_MENUITEM_IMPORT_KEEPASS:
      case ID_MENUITEM_IMPORT_PLAINTEXT:
      case ID_MENUITEM_IMPORT_XML:
      case ID_MENUITEM_MERGE:
        iEnable = FALSE;
        break;
      default:
        break;
      }
    }
  }
#endif
  return iEnable;
}
