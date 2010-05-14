/*
 * Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file passwordsafeframe.h
 * 
 */

// Generated by DialogBlocks, Wed 14 Jan 2009 10:24:11 PM IST

#ifndef _PASSWORDSAFEFRAME_H_
#define _PASSWORDSAFEFRAME_H_


/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
////@end includes
#include "wx/treebase.h" // for wxTreeItemId
#include "corelib/PWScore.h"
#include "corelib/UIinterface.h"
#include "RUEList.h"
/*!
 * Forward declarations
 */

////@begin forward declarations
class PWSGrid;
class PWSTreeCtrl;
class SystemTray;
////@end forward declarations
class PasswordSafeSearch;

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_PASSWORDSAFEFRAME 10001
#define ID_MENU_CLEAR_MRU 10011
#define ID_EXPORTMENU 10013
#define ID_EXPORT2OLD1XFORMAT 10013
#define ID_EXPORT2V2FORMAT 10014
#define ID_EXPORT2PLAINTEXT 10015
#define ID_EXPORT2XML 10016
#define ID_IMPORTMENU 10017
#define ID_IMPORT_PLAINTEXT 10018
#define ID_IMPORT_XML 10019
#define ID_IMPORT_KEEPASS 10020
#define ID_MERGE 10021
#define ID_COMPARE 10022
#define ID_EDIT 10023
#define ID_RENAME 10024
#define ID_DUPLICATEENTRY 10025
#define ID_ADDGROUP 10026
#define ID_CLEARCLIPBOARD 10027
#define ID_COPYPASSWORD 10028
#define ID_COPYUSERNAME 10029
#define ID_COPYNOTESFLD 10030
#define ID_COPYURL 10031
#define ID_BROWSEURL 10032
#define ID_AUTOTYPE 10033
#define ID_GOTOBASEENTRY 10034
#define ID_LIST_VIEW 10035
#define ID_TREE_VIEW 10036
#define ID_SHOWHIDE_TOOLBAR 10037
#define ID_SHOWHIDE_DRAGBAR 10038
#define ID_EXPANDALL 10039
#define ID_COLLAPESALL 10040
#define ID_FILTERMENU 10041
#define ID_EDITFILTER 10042
#define ID_APPLYFILTER 10043
#define ID_MANAGEFILTERS 10044
#define ID_CUSTOMIZETOOLBAR 10045
#define ID_CHANGEFONTMENU 10046
#define ID_CHANGETREEFONT 10047
#define ID_CHANGEPSWDFONT 10048
#define ID_REPORTSMENU 10049
#define ID_REPORT_COMPARE 10050
#define ID_REPORT_FIND 10051
#define ID_REPORT_IMPORTTEXT 10052
#define ID_REPORT_IMPORTXML 10053
#define ID_REPORT_MERGE 10054
#define ID_REPORT_VALIDATE 10055
#define ID_CHANGECOMBO 10056
#define ID_BACKUP 10057
#define ID_RESTORE 10058
#define ID_OPTIONS_M 10059
#define ID_MENUITEM 10012
#define SYMBOL_PASSWORDSAFEFRAME_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_PASSWORDSAFEFRAME_TITLE _("PasswordSafe")
#define SYMBOL_PASSWORDSAFEFRAME_IDNAME ID_PASSWORDSAFEFRAME
#define SYMBOL_PASSWORDSAFEFRAME_SIZE wxSize(400, 300)
#define SYMBOL_PASSWORDSAFEFRAME_POSITION wxDefaultPosition
////@end control identifiers
enum {
  ID_EDITMENU_FIND_NEXT  = 10200,
  ID_EDITMENU_FIND_PREVIOUS,
  ID_PASSWORDSUBSET,
  ID_COPYEMAIL,
  ID_RUNCOMMAND,
  ID_COPYRUNCOMMAND,
  ID_BROWSEURLPLUS,
  ID_SENDEMAIL,
  ID_CREATESHORTCUT,
  ID_EDITBASEENTRY,
  ID_SYSTRAY_RESTORE,
  ID_SYSTRAY_LOCK,
  ID_SYSTRAY_UNLOCK,
  ID_SYSTRAY_CLOSE,
  ID_SYSTRAY_EXIT
};


/*!
 * PasswordSafeFrame class declaration
 */

class PasswordSafeFrame: public wxFrame, public UIInterFace
{    
    DECLARE_CLASS( PasswordSafeFrame )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    PasswordSafeFrame(PWScore &core);
    PasswordSafeFrame(wxWindow* parent, PWScore &core,
                      wxWindowID id = SYMBOL_PASSWORDSAFEFRAME_IDNAME, const wxString& caption = SYMBOL_PASSWORDSAFEFRAME_TITLE, const wxPoint& pos = SYMBOL_PASSWORDSAFEFRAME_POSITION, const wxSize& size = SYMBOL_PASSWORDSAFEFRAME_SIZE, long style = SYMBOL_PASSWORDSAFEFRAME_STYLE );

    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_PASSWORDSAFEFRAME_IDNAME, const wxString& caption = SYMBOL_PASSWORDSAFEFRAME_TITLE, const wxPoint& pos = SYMBOL_PASSWORDSAFEFRAME_POSITION, const wxSize& size = SYMBOL_PASSWORDSAFEFRAME_SIZE, long style = SYMBOL_PASSWORDSAFEFRAME_STYLE );

    /// Destructor
    ~PasswordSafeFrame();

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

    ItemList::size_type GetNumEntries() const {return m_core.GetNumEntries();}

    // UIinterface concrete methods:
    virtual void DatabaseModified(bool bChanged);
    virtual void UpdateGUI(UpdateGUICommand::GUI_Action ga,
                           uuid_array_t &entry_uuid,
                           CItemData::FieldType ft);
    virtual void GUISetupDisplayInfo(CItemData &ci);

    virtual void UpdateGUI(UpdateGUICommand::GUI_Action ga,
                           uuid_array_t &entry_uuid,
                           CItemData::FieldType ft = CItemData::START,
                           bool bUpdateGUI = true);
    
    virtual void GUIRefreshEntry(const CItemData&);
    
  ////@begin PasswordSafeFrame event handler declarations

  /// wxEVT_CLOSE_WINDOW event handler for ID_PASSWORDSAFEFRAME
  void OnCloseWindow( wxCloseEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_NEW
  void OnNewClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_OPEN
  void OnOpenClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLOSE
  void OnCloseClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
  void OnSaveClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PROPERTIES
  void OnPropertiesClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
  void OnExitClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD
  void OnAddClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_EDIT
  void OnEditClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE
  void OnDeleteClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_CLEARCLIPBOARD
  void OnClearclipboardClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_COPYPASSWORD
  void OnCopypasswordClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_COPYUSERNAME
  void OnCopyusernameClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_COPYNOTESFLD
  void OnCopynotesfldClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_COPYURL
  void OnCopyurlClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_COPYEMAIL
  void OnCopyEmailClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_LIST_VIEW
  void OnListViewClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_TREE_VIEW
  void OnTreeViewClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_CHANGECOMBO
  void OnChangePasswdClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_OPTIONS_M
  void OnOptionsMClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ABOUT
  void OnAboutClick( wxCommandEvent& evt);

////@end PasswordSafeFrame event handler declarations
  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxEVT_FIND
  void OnFindClick( wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_EDITMENU_FIND_NEXT
  void OnFindNext(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_EDITMENU_FIND_PREVIOUS
  void OnFindPrevious(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_BROWSEURL
  void OnBrowseURL(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_SENDEMAIL
  void OnSendEmail(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_RUNCOMMAND
  void OnRunCommand(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_AUTOTYPE
  void OnAutoType(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_BROWSEURLPLUS
  void OnBrowseUrlAndAutotype(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_GOTOBASEENTRY
  void OnGotoBase(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_EDITBASEENTRY
  void OnEditBase(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_CREATESHORTCUT
  void OnCreateShortcut(wxCommandEvent& evt);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_CLEAR_MRU
  void OnClearRecentHistory(wxCommandEvent& evt);


  /// wxEVT_UPDATE_UI event handler for all command ids
  void OnUpdateUI(wxUpdateUIEvent& evt);

  /// wxEVT_ICONIZE event handler
  void OnIconize(wxIconizeEvent& evt);
////@begin PasswordSafeFrame member function declarations

  /// Retrieves bitmap resources
  wxBitmap GetBitmapResource( const wxString& name );

  /// Retrieves icon resources
  wxIcon GetIconResource( const wxString& name );
////@end PasswordSafeFrame member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

    // Overriden virtuals
    virtual bool Show(bool show = true);
    virtual void SetTitle(const wxString& title);
    
    // PasswordSafe specifics:
    int Load(const wxString &passwd);
    
    // Hilites the item.  Used for search
    void SelectItem(const CUUIDGen& uuid);

    ItemListConstIter GetEntryIter() const {return m_core.GetEntryIter();}
    ItemListConstIter GetEntryEndIter() const {return m_core.GetEntryEndIter();}
    
    bool IsTreeView() const {return m_currentView == TREE;}
    void RefreshView() {if (IsTreeView()) ShowTree(); else ShowGrid();}
    void FlattenTree(OrderedItemList& olist);

    void DispatchDblClickAction(CItemData &item); //called by grid/tree

    /// Centralized handling of right click in the grid or the tree view
    void OnContextMenu(const CItemData* item);

    /// Called by wxTaskbarIcon derived class on clicking of system tray's Restore menu item
    void UnlockSafe(bool restoreUI);

    /// Called by app when the inactivity timer arrives
    void HideUI(bool lock);

    /// Called by system tray unlock the UI (and optionally restore the main window)
    void UnlockUI(bool restoreFrame);
    
    /// Returns true if the user enters the correct safe combination and presses OK
    bool VerifySafeCombination(wxString& password);

    void GetAllMenuItemStrings(std::vector<RUEntryData>& vec) const { m_RUEList.GetAllMenuItemStrings(vec); };
    void DeleteRUEntry(size_t index) { m_RUEList.DeleteRUEntry(index); }

    
////@begin PasswordSafeFrame member variables
  PWSGrid* m_grid;
  PWSTreeCtrl* m_tree;
////@end PasswordSafeFrame member variables
 private:
  int New();
  int NewFile(StringX &fname);
  int Open(const wxString &fname); // prompt for password, try to Load.
  int SaveIfChanged();
  int Save();
  void ShowGrid(bool show = true);
  void ShowTree(bool show = true);
  void ClearData();
  bool ReloadDatabase(const wxString& password);
  bool SaveAndClearDatabase();
  void CleanupAfterReloadFailure(bool tellUser);
  Command *Delete(CItemData *pci);
  Command *Delete(wxTreeItemId tid); // for group delete
  CItemData *GetSelectedEntry() const;
  CItemData* GetBaseOfSelectedEntry(); //traverses to the base item if the selected item is a shortcut 
  void UpdateAccessTime(CItemData &ci);
  void CreateMainToolbar();
  bool IsRUEEvent(const wxCommandEvent& evt) {
    long index = evt.GetExtraLong();
    return index && index < 256 && size_t(index) < m_RUEList.GetCount(); 
  }
  long GetRUEIndex(const wxCommandEvent& evt) { return evt.GetExtraLong(); }

  // Do* member functions for dbl-click and menu-accessible actions
  void DoCopyPassword(CItemData &item);
  void DoCopyNotes(CItemData &item);
  void DoCopyUsername(CItemData &item);
  void DoCopyURL(CItemData &item);
  void DoCopyEmail(CItemData &item);
  void DoEdit(CItemData &item);
  void DoAutotype(CItemData &item);
  void DoAutotype(const StringX& sx_autotype, const std::vector<size_t>& vactionverboffsets);
  void DoBrowse(CItemData &item);
  void DoRun(CItemData &item);
  void DoEmail(CItemData &item);
  
  PWScore &m_core;
  enum {TREE, GRID} m_currentView;
  PasswordSafeSearch* m_search;
  SystemTray* m_sysTray;
  bool m_exitFromMenu; 
  CRUEList m_RUEList;
};

#endif
    // _PASSWORDSAFEFRAME_H_
