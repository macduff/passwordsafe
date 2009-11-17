/*
 * Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file passwordsafeframe.cpp
* 
*/

// Generated by DialogBlocks, Wed 14 Jan 2009 10:24:11 PM IST

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "safecombinationchange.h"
#include "about.h"
#include "PWSgrid.h"
#include "PWStree.h"
////@end includes
#include "PWSgridtable.h"

#include "passwordsafeframe.h"
#include "safecombinationprompt.h"
#include "properties.h"
#include "optionspropsheet.h"
#include "corelib/PWSprefs.h"
#include "corelib/PWSdirs.h"
#include "PasswordSafeSearch.h"
#include "pwsclip.h"

////@begin XPM images
////@end XPM images


/*!
 * PasswordSafeFrame type definition
 */

IMPLEMENT_CLASS( PasswordSafeFrame, wxFrame )


/*!
 * PasswordSafeFrame event table definition
 */

BEGIN_EVENT_TABLE( PasswordSafeFrame, wxFrame )

////@begin PasswordSafeFrame event table entries
  EVT_CLOSE( PasswordSafeFrame::OnCloseWindow )

  EVT_MENU( wxID_OPEN, PasswordSafeFrame::OnOpenClick )

  EVT_MENU( wxID_CLOSE, PasswordSafeFrame::OnCloseClick )

  EVT_MENU( wxID_SAVE, PasswordSafeFrame::OnSaveClick )

  EVT_MENU( wxID_PROPERTIES, PasswordSafeFrame::OnPropertiesClick )

  EVT_MENU( wxID_EXIT, PasswordSafeFrame::OnExitClick )

  EVT_MENU( wxID_ADD, PasswordSafeFrame::OnAddClick )

  EVT_MENU( ID_EDIT, PasswordSafeFrame::OnEditClick )

  EVT_MENU( wxID_DELETE, PasswordSafeFrame::OnDeleteClick )

  EVT_MENU( ID_CLEARCLIPBOARD, PasswordSafeFrame::OnClearclipboardClick )

  EVT_MENU( ID_COPYPASSWORD, PasswordSafeFrame::OnCopypasswordClick )

  EVT_MENU( ID_COPYUSERNAME, PasswordSafeFrame::OnCopyusernameClick )

  EVT_MENU( ID_COPYNOTESFLD, PasswordSafeFrame::OnCopynotesfldClick )

  EVT_MENU( ID_COPYURL, PasswordSafeFrame::OnCopyurlClick )

  EVT_MENU( ID_LIST_VIEW, PasswordSafeFrame::OnListViewClick )

  EVT_MENU( ID_TREE_VIEW, PasswordSafeFrame::OnTreeViewClick )

  EVT_MENU( ID_CHANGECOMBO, PasswordSafeFrame::OnChangePasswdClick )

  EVT_MENU( ID_OPTIONS_M, PasswordSafeFrame::OnOptionsMClick )

  EVT_MENU( wxID_ABOUT, PasswordSafeFrame::OnAboutClick )

////@end PasswordSafeFrame event table entries
  EVT_MENU( wxID_FIND, PasswordSafeFrame::OnFindClick )

  EVT_MENU( ID_EDITMENU_FIND_NEXT, PasswordSafeFrame::OnFindNext )

  EVT_MENU( ID_EDITMENU_FIND_PREVIOUS, PasswordSafeFrame::OnFindPrevious )


END_EVENT_TABLE()


/*!
 * PasswordSafeFrame constructors
 */

PasswordSafeFrame::PasswordSafeFrame(PWScore &core)
: m_core(core), m_currentView(GRID), m_search(0)
{
    Init();
}

PasswordSafeFrame::PasswordSafeFrame(wxWindow* parent, PWScore &core,
                                     wxWindowID id, const wxString& caption,
                                     const wxPoint& pos, const wxSize& size,
                                     long style)
  : m_core(core), m_currentView(GRID), m_search(0)
{
    Init();
    if (PWSprefs::GetInstance()->GetPref(PWSprefs::AlwaysOnTop))
      style |= wxSTAY_ON_TOP;
    Create( parent, id, caption, pos, size, style );
}


/*!
 * PasswordSafeFrame creator
 */

bool PasswordSafeFrame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PasswordSafeFrame creation
  wxFrame::Create( parent, id, caption, pos, size, style );

  CreateControls();
  Centre();
////@end PasswordSafeFrame creation
  m_search = new PasswordSafeSearch(this);
    return true;
}


/*!
 * PasswordSafeFrame destructor
 */

PasswordSafeFrame::~PasswordSafeFrame()
{
////@begin PasswordSafeFrame destruction
////@end PasswordSafeFrame destruction
  delete m_search;
  m_search = 0;
}


/*!
 * Member initialisation
 */

void PasswordSafeFrame::Init()
{
////@begin PasswordSafeFrame member initialisation
  m_grid = NULL;
  m_tree = NULL;
////@end PasswordSafeFrame member initialisation
}


/*!
 * Control creation for PasswordSafeFrame
 */

void PasswordSafeFrame::CreateControls()
{    
  PWSprefs *prefs = PWSprefs::GetInstance();
  const StringX lastView = prefs->GetPref(PWSprefs::LastView);
  m_currentView = (lastView == _T("list")) ? GRID : TREE;

  PasswordSafeFrame* itemFrame1 = this;

  wxMenuBar* menuBar = new wxMenuBar;
  wxMenu* itemMenu3 = new wxMenu;
  itemMenu3->Append(wxID_NEW, _("&New..."), _T(""), wxITEM_NORMAL);
  itemMenu3->Append(wxID_OPEN, _("&Open..."), _T(""), wxITEM_NORMAL);
  itemMenu3->Append(wxID_CLOSE, _("&Close"), _T(""), wxITEM_NORMAL);
  itemMenu3->AppendSeparator();
  itemMenu3->Append(ID_MENU_CLEAR_MRU, _("Clear Recent Safe List"), _T(""), wxITEM_NORMAL);
  itemMenu3->AppendSeparator();
  itemMenu3->Append(wxID_SAVE, _("&Save..."), _T(""), wxITEM_NORMAL);
  itemMenu3->Append(wxID_SAVEAS, _("Save &As..."), _T(""), wxITEM_NORMAL);
  itemMenu3->AppendSeparator();
  wxMenu* itemMenu13 = new wxMenu;
  itemMenu13->Append(ID_EXPORT2OLD1XFORMAT, _("v&1.x format..."), _T(""), wxITEM_NORMAL);
  itemMenu13->Append(ID_EXPORT2V2FORMAT, _("v&2 format..."), _T(""), wxITEM_NORMAL);
  itemMenu13->Append(ID_EXPORT2PLAINTEXT, _("&Plain Text (tab separated)..."), _T(""), wxITEM_NORMAL);
  itemMenu13->Append(ID_EXPORT2XML, _("&XML format..."), _T(""), wxITEM_NORMAL);
  itemMenu3->Append(ID_EXPORTMENU, _("Export &To"), itemMenu13);
  wxMenu* itemMenu18 = new wxMenu;
  itemMenu18->Append(ID_IMPORT_PLAINTEXT, _("&Plain Text..."), _T(""), wxITEM_NORMAL);
  itemMenu18->Append(ID_IMPORT_XML, _("&XML format..."), _T(""), wxITEM_NORMAL);
  itemMenu18->Append(ID_IMPORT_KEEPASS, _("&KeePass..."), _T(""), wxITEM_NORMAL);
  itemMenu3->Append(ID_IMPORTMENU, _("Import &From"), itemMenu18);
  itemMenu3->Append(ID_MERGE, _("Merge..."), _T(""), wxITEM_NORMAL);
  itemMenu3->Append(ID_COMPARE, _("Compare..."), _T(""), wxITEM_NORMAL);
  itemMenu3->AppendSeparator();
  itemMenu3->Append(wxID_PROPERTIES, _("&Properties"), _T(""), wxITEM_NORMAL);
  itemMenu3->AppendSeparator();
  itemMenu3->Append(wxID_EXIT, _("E&xit"), _T(""), wxITEM_NORMAL);
  menuBar->Append(itemMenu3, _("&File"));
  wxMenu* itemMenu28 = new wxMenu;
  itemMenu28->Append(wxID_ADD, _("&Add Entry...\tCtrl+A"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_EDIT, _("Edit/&View Entry...\tCtrl+Enter"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(wxID_DELETE, _("&Delete Entry\tDel"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_RENAME, _("Rename Entry\tF2"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(wxID_FIND, _("&Find Entry...\tCtrl+F"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_DUPLICATEENTRY, _("&Duplicate Entry\tCtrl+D"), _T(""), wxITEM_NORMAL);
  itemMenu28->AppendSeparator();
  itemMenu28->Append(ID_ADDGROUP, _("Add Group"), _T(""), wxITEM_NORMAL);
  itemMenu28->AppendSeparator();
  itemMenu28->Append(ID_CLEARCLIPBOARD, _("C&lear Clipboard\tCtrl+Del"), _T(""), wxITEM_NORMAL);
  itemMenu28->AppendSeparator();
  itemMenu28->Append(ID_COPYPASSWORD, _("&Copy Password to Clipboard\tCtrl+C"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_COPYUSERNAME, _("Copy &Username to Clipboard\tCtrl+U"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_COPYNOTESFLD, _("Copy &Notes to Clipboard\tCtrl+G"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_COPYURL, _("Copy URL to Clipboard\tCtrl+Alt+L"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_BROWSEURL, _("&Browse to URL\tCtrl+L"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_AUTOTYPE, _("Perform Auto&type\tCtrl+T"), _T(""), wxITEM_NORMAL);
  itemMenu28->Append(ID_GOTOBASEENTRY, _("Go to Base entry"), _T(""), wxITEM_NORMAL);
  menuBar->Append(itemMenu28, _("&Edit"));
  wxMenu* itemMenu47 = new wxMenu;
  itemMenu47->Append(ID_LIST_VIEW, _("Flattened &List"), _T(""), wxITEM_RADIO);
  itemMenu47->Append(ID_TREE_VIEW, _("Nested &Tree"), _T(""), wxITEM_RADIO);
  itemMenu47->AppendSeparator();
  itemMenu47->Append(ID_SHOWHIDE_TOOLBAR, _("Tooolbar &visible"), _T(""), wxITEM_CHECK);
  itemMenu47->Append(ID_SHOWHIDE_DRAGBAR, _("&Dragbar visible"), _T(""), wxITEM_CHECK);
  itemMenu47->AppendSeparator();
  itemMenu47->Append(ID_EXPANDALL, _("Expand All"), _T(""), wxITEM_NORMAL);
  itemMenu47->Append(ID_COLLAPESALL, _("Collapse All"), _T(""), wxITEM_NORMAL);
  wxMenu* itemMenu56 = new wxMenu;
  itemMenu56->Append(ID_EDITFILTER, _("&New/Edit Filter..."), _T(""), wxITEM_NORMAL);
  itemMenu56->Append(ID_APPLYFILTER, _("&Apply current"), _T(""), wxITEM_NORMAL);
  itemMenu56->Append(ID_MANAGEFILTERS, _("&Manage..."), _T(""), wxITEM_NORMAL);
  itemMenu47->Append(ID_FILTERMENU, _("&Filters"), itemMenu56);
  itemMenu47->AppendSeparator();
  itemMenu47->Append(ID_CUSTOMIZETOOLBAR, _("Customize &Main Toolbar..."), _T(""), wxITEM_NORMAL);
  wxMenu* itemMenu62 = new wxMenu;
  itemMenu62->Append(ID_CHANGETREEFONT, _("&Tree/List Font"), _T(""), wxITEM_NORMAL);
  itemMenu62->Append(ID_CHANGEPSWDFONT, _("&Password Font"), _T(""), wxITEM_NORMAL);
  itemMenu47->Append(ID_CHANGEFONTMENU, _("Change &Font"), itemMenu62);
  wxMenu* itemMenu65 = new wxMenu;
  itemMenu65->Append(ID_REPORT_COMPARE, _("&Compare"), _T(""), wxITEM_NORMAL);
  itemMenu65->Append(ID_REPORT_FIND, _("&Find"), _T(""), wxITEM_NORMAL);
  itemMenu65->Append(ID_REPORT_IMPORTTEXT, _("Import &Text"), _T(""), wxITEM_NORMAL);
  itemMenu65->Append(ID_REPORT_IMPORTXML, _("Import &XML"), _T(""), wxITEM_NORMAL);
  itemMenu65->Append(ID_REPORT_MERGE, _("I&Merge"), _T(""), wxITEM_NORMAL);
  itemMenu65->Append(ID_REPORT_VALIDATE, _("&Validate"), _T(""), wxITEM_NORMAL);
  itemMenu47->Append(ID_REPORTSMENU, _("Reports"), itemMenu65);
  menuBar->Append(itemMenu47, _("&View"));
  wxMenu* itemMenu72 = new wxMenu;
  itemMenu72->Append(ID_CHANGECOMBO, _("&Change Safe Combination..."), _T(""), wxITEM_NORMAL);
  itemMenu72->AppendSeparator();
  itemMenu72->Append(ID_BACKUP, _("Make &Backup\tCtrl+B"), _T(""), wxITEM_NORMAL);
  itemMenu72->Append(ID_RESTORE, _("&Restore from Backup...\tCtrl+R"), _T(""), wxITEM_NORMAL);
  itemMenu72->AppendSeparator();
  itemMenu72->Append(ID_OPTIONS_M, _("&Options..."), _T(""), wxITEM_NORMAL);
  menuBar->Append(itemMenu72, _("&Manage"));
  wxMenu* itemMenu79 = new wxMenu;
  itemMenu79->Append(wxID_HELP, _("Get &Help"), _T(""), wxITEM_NORMAL);
  itemMenu79->Append(ID_MENUITEM, _("Visit Password Safe &website..."), _T(""), wxITEM_NORMAL);
  itemMenu79->Append(wxID_ABOUT, _("&About Password Safe..."), _T(""), wxITEM_NORMAL);
  menuBar->Append(itemMenu79, _("&Help"));
  itemFrame1->SetMenuBar(menuBar);

  wxBoxSizer* itemBoxSizer83 = new wxBoxSizer(wxHORIZONTAL);
  itemFrame1->SetSizer(itemBoxSizer83);

  m_grid = new PWSGrid( itemFrame1, m_core, ID_LISTBOX, wxDefaultPosition,
                        itemFrame1->GetClientSize(), wxHSCROLL|wxVSCROLL );
  itemBoxSizer83->Add(m_grid, wxSizerFlags().Expand().Border(0).Proportion(1));

  m_tree = new PWSTreeCtrl( itemFrame1, m_core, ID_TREECTRL, wxDefaultPosition,
                            itemFrame1->GetClientSize(),
                            wxTR_EDIT_LABELS|wxTR_HAS_BUTTONS |wxTR_HIDE_ROOT|wxTR_SINGLE );
  itemBoxSizer83->Add(m_tree, wxSizerFlags().Expand().Border(0).Proportion(1));
  itemBoxSizer83->Layout();

  if (m_currentView == TREE)
    itemMenu47->Check(ID_TREE_VIEW, true);
}


/*!
 * Should we show tooltips?
 */

bool PasswordSafeFrame::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PasswordSafeFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PasswordSafeFrame bitmap retrieval
  wxUnusedVar(name);
  return wxNullBitmap;
////@end PasswordSafeFrame bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PasswordSafeFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PasswordSafeFrame icon retrieval
  wxUnusedVar(name);
  return wxNullIcon;
////@end PasswordSafeFrame icon retrieval
}

void PasswordSafeFrame::SetTitle(const wxString& title)
{
  wxString newtitle = _T("PasswordSafe");
  if (!title.empty()) {
    newtitle += _T(" - ");
    StringX fname = title.c_str();
    StringX::size_type findex = fname.rfind(_T("/"));
    if (findex != StringX::npos)
      fname = fname.substr(findex + 1);
    newtitle += fname.c_str();
  }
  wxFrame::SetTitle(newtitle);
}

int PasswordSafeFrame::Load(const wxString &passwd)
{
  int status = m_core.ReadCurFile(passwd.c_str());
  if (status == PWScore::SUCCESS) {
    SetTitle(m_core.GetCurFile().c_str());
  } else {
    SetTitle(_T(""));
  }
  return status;
}

bool PasswordSafeFrame::Show(bool show)
{
  ShowGrid(show && (m_currentView == GRID));
  ShowTree(show && (m_currentView == TREE));
  return wxFrame::Show(show);
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void PasswordSafeFrame::OnExitClick( wxCommandEvent& event )
{
  Close();
}

void PasswordSafeFrame::ShowGrid(bool show)
{
  if (show) {
    m_grid->SetTable(new PWSGridTable(m_grid), true); // true => auto-delete
    m_grid->AutoSizeColumns();
    m_grid->EnableEditing(false);
  }
  int w,h;
  GetClientSize(&w, &h);
  m_grid->SetSize(w, h);
  m_grid->Show(show);
}

void PasswordSafeFrame::ShowTree(bool show)
{
  if (show) {
    m_tree->Clear();
    ItemListConstIter iter;
    for (iter = m_core.GetEntryIter();
         iter != m_core.GetEntryEndIter();
         iter++) {
      m_tree->AddItem(iter->second);
    }
  }

  int w,h;
  GetClientSize(&w, &h);
  m_tree->SetSize(w, h);
  m_tree->Show(show);
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_LIST_VIEW
 */

void PasswordSafeFrame::OnListViewClick( wxCommandEvent& event )
{
  PWSprefs::GetInstance()->SetPref(PWSprefs::LastView, _T("list"));
  ShowTree(false);
  ShowGrid(true);
  m_currentView = GRID;
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_TREE_VIEW
 */

void PasswordSafeFrame::OnTreeViewClick( wxCommandEvent& event )
{
  PWSprefs::GetInstance()->SetPref(PWSprefs::LastView, _T("tree"));
  ShowGrid(false);
  ShowTree(true);
  m_currentView = TREE;
}

int PasswordSafeFrame::Save()
{
  int rc = m_core.WriteCurFile();
  if (rc != PWScore::SUCCESS) {
    wxString msg(_("Failed to save database "));
    msg += m_core.GetCurFile().c_str();
    wxMessageDialog dlg(this, msg, GetTitle(),
                        (wxICON_ERROR | wxOK));
    dlg.ShowModal();
  }
  return rc;
}

int PasswordSafeFrame::SaveIfChanged()
{
  // offer to save existing database if it was modified.
  // used before loading another
  // returns PWScore::SUCCESS if save succeeded or if user decided
  // not to save

  if (m_core.IsChanged() || PWSprefs::GetInstance()->IsDBprefsChanged()) {
    wxString prompt(_("Do you want to save changes to the password database: "));
    prompt += m_core.GetCurFile().c_str();
    prompt += _T("?");
    wxMessageDialog dlg(this, prompt, GetTitle(),
                        (wxICON_QUESTION | wxCANCEL |
                         wxYES_NO | wxYES_DEFAULT));
    int rc = dlg.ShowModal();
    switch (rc) {
      case wxID_CANCEL:
        return PWScore::USER_CANCEL;
      case wxID_YES:
        rc = Save();
        // Make sure that file was successfully written
        if (rc == PWScore::SUCCESS) {
          m_core.UnlockFile(m_core.GetCurFile().c_str());
          break;
        } else
          return PWScore::CANT_OPEN_FILE;
      case wxID_NO:
        // Reset changed flag
        // SetChanged(Clear);
        break;
    }
  }
  return PWScore::SUCCESS;
}

void PasswordSafeFrame::ClearData()
{
  //m_core.ClearData();
  m_core.ReInit();
  m_grid->BeginBatch();
  m_grid->ClearGrid();
  m_grid->EndBatch();
  m_tree->Clear();
}

CItemData *PasswordSafeFrame::GetSelectedEntry() const
{
  if (m_tree->IsShown()) {
    // get selected from tree
    return m_tree->GetItem(m_tree->GetSelection());
  } else if (m_grid->IsShown()) {
    // get selected from grid
    return m_grid->GetItem(m_grid->GetGridCursorRow());
  }
  return NULL;
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_OPEN
 */

void PasswordSafeFrame::OnOpenClick( wxCommandEvent& event )
{
  stringT dir = PWSdirs::GetSafeDir();
  //Open-type dialog box
  wxFileDialog fd(this, _("Please Choose a Database to Open:"),
                  dir.c_str(), _("pwsafe.psafe3"),
                  _("Password Safe Databases (*.psafe3; *.dat)|*.psafe3; *.dat|"
                    "Password Safe Backups (*.bak)|*.bak|"
                    "Password Safe Intermediate Backups (*.ibak)|*.ibak|"
                    "All files (*.*)|*.*"),
                  (wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR));

  while (1) {
    if (fd.ShowModal() == wxID_OK) {
      int rc = Open(fd.GetPath()); // prompt for password of new file and load.
      if (rc == PWScore::SUCCESS) {
        break;
      }
    } else { // user cancelled 
      break;
    }
  }
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLOSE
 */

void PasswordSafeFrame::OnCloseClick( wxCommandEvent& event )
{
  PWSprefs *prefs = PWSprefs::GetInstance();

  // Save Application related preferences
  prefs->SaveApplicationPreferences();
  if( !m_core.GetCurFile().empty() ) {
    int rc = SaveIfChanged();
    if (rc != PWScore::SUCCESS)
      return;
    m_core.SetCurFile(_T(""));
    ClearData();
    SetTitle(_T(""));
  }
}

int PasswordSafeFrame::Open(const wxString &fname)
{ // prompt for password, try to Load.
  CSafeCombinationPrompt pwdprompt(this, m_core, fname);
  if (pwdprompt.ShowModal() == wxID_OK) {
    m_core.SetCurFile(fname.c_str());
    wxString password = pwdprompt.GetPassword();
    int retval = Load(password);
    if (retval == PWScore::SUCCESS)
      Show();
    return retval;
  } else
    return PWScore::USER_CANCEL;
#if 0
  //Check that this file isn't already open
  if (pszFilename == m_core.GetCurFile() && !m_needsreading) {
    //It is the same damn file
    cs_text.LoadString(IDS_ALREADYOPEN);
    cs_title.LoadString(IDS_OPENDATABASE);
    MessageBox(cs_text, cs_title, MB_OK|MB_ICONWARNING);
    return PWScore::ALREADY_OPEN;
  }

  rc = SaveIfChanged();
  if (rc != PWScore::SUCCESS)
    return rc;

  rc = GetAndCheckPassword(pszFilename, passkey, GCP_NORMAL, bReadOnly);  // OK, CANCEL, HELP
  switch (rc) {
    case PWScore::SUCCESS:
      app.AddToMRU(pszFilename.c_str());
      m_bAlreadyToldUserNoSave = false;
      break; // Keep going...
    case PWScore::CANT_OPEN_FILE:
      temp.Format(IDS_SAFENOTEXIST, pszFilename.c_str());
      cs_title.LoadString(IDS_FILEOPENERROR);
      MessageBox(temp, cs_title, MB_OK|MB_ICONWARNING);
    case TAR_OPEN:
      return Open();
    case TAR_NEW:
      return New();
    case PWScore::WRONG_PASSWORD:
    case PWScore::USER_CANCEL:
      /*
      If the user just cancelled out of the password dialog,
      assume they want to return to where they were before...
      */
      return PWScore::USER_CANCEL;
    default:
      ASSERT(0); // we should take care of all cases explicitly
      return PWScore::USER_CANCEL; // conservative behaviour for release version
  }

  // clear the data before loading the new file
  ClearData();

  cs_title.LoadString(IDS_FILEREADERROR);
  MFCAsker q;
  MFCReporter r;
  m_core.SetAsker(&q);
  m_core.SetReporter(&r);
  rc = m_core.ReadFile(pszFilename, passkey);
  m_core.SetAsker(NULL);
  m_core.SetReporter(NULL);
  switch (rc) {
    case PWScore::SUCCESS:
      break;
    case PWScore::CANT_OPEN_FILE:
      temp.Format(IDS_CANTOPENREADING, pszFilename.c_str());
      MessageBox(temp, cs_title, MB_OK|MB_ICONWARNING);
      /*
      Everything stays as is... Worst case,
      they saved their file....
      */
      return PWScore::CANT_OPEN_FILE;
    case PWScore::BAD_DIGEST:
    {
      temp.Format(IDS_FILECORRUPT, pszFilename.c_str());
      const int yn = MessageBox(temp, cs_title, MB_YESNO|MB_ICONERROR);
      if (yn == IDYES) {
        rc = PWScore::SUCCESS;
        break;
      } else
        return rc;
    }
#ifdef DEMO
    case PWScore::LIMIT_REACHED:
    {
      CString cs_msg; cs_msg.Format(IDS_LIMIT_MSG, MAXDEMO);
      CString cs_title(MAKEINTRESOURCE(IDS_LIMIT_TITLE));
      const int yn = MessageBox(cs_msg, cs_title, MB_YESNO|MB_ICONWARNING);
      if (yn == IDNO) {
        return PWScore::USER_CANCEL;
      }
      rc = PWScore::SUCCESS;
      m_MainToolBar.GetToolBarCtrl().EnableButton(ID_MENUITEM_ADD, FALSE);
      break;
    }
#endif
    default:
      temp.Format(IDS_UNKNOWNERROR, pszFilename.c_str());
      MessageBox(temp, cs_title, MB_OK|MB_ICONERROR);
      return rc;
  }
  m_core.SetCurFile(pszFilename);
#if !defined(POCKET_PC)
  m_titlebar = PWSUtil::NormalizeTTT(_T("Password Safe - ") +
                                     m_core.GetCurFile()).c_str();
  SetWindowText(LPCTSTR(m_titlebar));
#endif
  CheckExpiredPasswords();
  ChangeOkUpdate();

  // Tidy up filters
  m_currentfilter.Empty();
  m_bFilterActive = false;

  RefreshViews();
  SetInitialDatabaseDisplay();
  m_core.SetDefUsername(PWSprefs::GetInstance()->
                        GetPref(PWSprefs::DefaultUsername));
  m_core.SetUseDefUser(PWSprefs::GetInstance()->
                       GetPref(PWSprefs::UseDefaultUser) ? true : false);
  m_needsreading = false;
  SelectFirstEntry();

  return rc;
#endif
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PROPERTIES
 */

void PasswordSafeFrame::OnPropertiesClick( wxCommandEvent& event )
{
  CProperties props(this, m_core);
  props.ShowModal();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_CHANGECOMBO
 */

void PasswordSafeFrame::OnChangePasswdClick( wxCommandEvent& event )
{
  CSafeCombinationChange* window = new CSafeCombinationChange(this, m_core);
  int returnValue = window->ShowModal();
  if (returnValue == wxID_OK) {
    m_core.ChangePassword(window->GetNewpasswd().c_str());
  }
  window->Destroy();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void PasswordSafeFrame::OnSaveClick( wxCommandEvent& event )
{
  Save();
}


/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_PASSWORDSAFEFRAME
 */

void PasswordSafeFrame::OnCloseWindow( wxCloseEvent& event )
{
  if (event.CanVeto()) {
    int rc = SaveIfChanged();
    if (rc == PWScore::USER_CANCEL) {
      event.Veto();
      return;
    }
  }
  Destroy();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ABOUT
 */

void PasswordSafeFrame::OnAboutClick( wxCommandEvent& event )
{
  CAbout* window = new CAbout(this);
  window->ShowModal();
  window->Destroy();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_OPTIONS_M
 */

void PasswordSafeFrame::OnOptionsMClick( wxCommandEvent& event )
{
  COptions *window = new COptions(this);
  window->ShowModal();
  window->Destroy();
}

void PasswordSafeFrame::SeletItem(const CUUIDGen& uuid)
{
    if (m_currentView == GRID) {
      m_grid->SelectItem(uuid);
    }
    else {
      m_tree->SelectItem(uuid);
    }

}

void PasswordSafeFrame::UpdateAccessTime(CItemData &ci)
{
  // Mark access time if so configured
#ifdef NOTYET
  // First add to RUE List
  uuid_array_t RUEuuid;
  ci.GetUUID(RUEuuid);
  m_RUEList.AddRUEntry(RUEuuid);
#endif
  bool bMaintainDateTimeStamps = PWSprefs::GetInstance()->
              GetPref(PWSprefs::MaintainDateTimeStamps);

  if (!m_core.IsReadOnly() && bMaintainDateTimeStamps) {
    ci.SetATime();
#ifdef NOTYET
    // Need to update view if there
    if (m_nColumnIndexByType[CItemData::ATIME] != -1) {
      // Get index of entry
      DisplayInfo *pdi = (DisplayInfo *)ci.GetDisplayInfo();
      // Get value in correct format
      CString cs_atime = ci.GetATimeL().c_str();
      // Update it
      m_ctlItemList.SetItemText(pdi->list_index,
        m_nColumnIndexByType[CItemData::ATIME], cs_atime);
    }
#endif
  }
}

void PasswordSafeFrame::DispatchDblClickAction(CItemData &item)
{
  switch (PWSprefs::GetInstance()->GetPref(PWSprefs::DoubleClickAction)) {
  case PWSprefs::DoubleClickAutoType:
    DoAutotype(item);
    break;
  case PWSprefs::DoubleClickBrowse:
    DoBrowse(item);
    break;
  case PWSprefs::DoubleClickCopyNotes:
    DoCopyNotes(item);
    break;
  case PWSprefs::DoubleClickCopyPassword:
    DoCopyPassword(item);
    break;
  case PWSprefs::DoubleClickCopyUsername:
    DoCopyUsername(item);
    break;
  case PWSprefs::DoubleClickCopyPasswordMinimize:
    DoCopyPassword(item);
    Iconize(true);
    break;
  case PWSprefs::DoubleClickViewEdit:
    DoEdit(item);
    break;
  case PWSprefs::DoubleClickBrowsePlus:
    DoBrowse(item);
    // Wait a little?
    DoAutotype(item);
    break;
  case PWSprefs::DoubleClickRun:
    DoRun(item);
    break;
  case PWSprefs::DoubleClickSendEmail:
    DoEmail(item);
    break;
  default: {
    wxString action;
    action.Format(_("Unknown code: %d"),
                  PWSprefs::GetInstance()->GetPref(PWSprefs::DoubleClickAction));
    wxMessageBox(action);
    break;
  }
  }
}

static void FlattenTree(wxTreeItemId id, PWSTreeCtrl* tree, OrderedItemList& olist)
{
  wxTreeItemIdValue cookie;
  for (wxTreeItemId childId = tree->GetFirstChild(id, cookie); childId.IsOk(); 
                          childId = tree->GetNextChild(id, cookie)) {
    CItemData* item = tree->GetItem(childId);
    if (item)
      olist.push_back(*item);

    if (tree->HasChildren(childId))
      ::FlattenTree(childId, tree, olist);
  }
}

void PasswordSafeFrame::FlattenTree(OrderedItemList& olist)
{
  ::FlattenTree(m_tree->GetRootItem(), m_tree, olist);
}

///////////////////////////////////////////
// Handles right-click event forwarded by the tree and list views
// The logic is the same as DboxMain::OnContextMenu in src/ui/Windows/MainMenu.cpp
void PasswordSafeFrame::OnContextMenu(CItemData* item)
{
  if (TREE == m_currentView && !item) {
    wxMenu groupEditMenu;
    groupEditMenu.Append(wxID_ADD, wxT("Add &Entry"));
    groupEditMenu.Append(ID_ADDGROUP, wxT("Add &Group"));
    groupEditMenu.Append(ID_RENAME, wxT("&Rename Group"));
    groupEditMenu.Append(wxID_DELETE, wxT("&Delete Group"));

    m_tree->PopupMenu(&groupEditMenu);
  }
  else {
    wxMenu itemEditMenu;
    itemEditMenu.Append(ID_COPYUSERNAME,   wxT("Copy &Username to Clipboard"));
    itemEditMenu.Append(ID_COPYPASSWORD,   wxT("&Copy Password to Clipboard"));
    itemEditMenu.Append(ID_PASSWORDSUBSET, wxT("Display subset of Password"));
    itemEditMenu.Append(ID_COPYNOTESFLD,   wxT("Copy &Notes to Clipboard"));
    itemEditMenu.Append(ID_COPYURL,        wxT("Copy UR&L to Clipboard"));
    itemEditMenu.Append(ID_COPYEMAIL,      wxT("Copy email to Clipboard"));
    itemEditMenu.Append(ID_COPYRUNCOMMAND, wxT("Copy Run Command to Clipboard"));
    itemEditMenu.AppendSeparator();
    itemEditMenu.Append(ID_BROWSEURL,      wxT("&Browse to URL"));
    itemEditMenu.Append(ID_BROWSEURLPLUS,  wxT("Browse to URL + &Autotype"));
    itemEditMenu.Append(ID_SENDEMAIL,       wxT("Send &email"));
    itemEditMenu.Append(ID_RUNCOMMAND,     wxT("&Run Command"));
    itemEditMenu.Append(ID_AUTOTYPE,       wxT("Perform Auto &Type"));
    itemEditMenu.AppendSeparator();
    itemEditMenu.Append(ID_EDIT,           wxT("Edit/&View Entry..."));
    itemEditMenu.Append(ID_RENAME,         wxT("Rename Entry"));
    itemEditMenu.Append(ID_DUPLICATEENTRY, wxT("&Duplicate Entry"));
    itemEditMenu.Append(wxID_DELETE,       wxT("Delete Entry"));
    itemEditMenu.Append(ID_CREATESHORTCUT, wxT("Create &Shortcut"));
    itemEditMenu.Append(ID_GOTOBASEENTRY,  wxT("&Go to Base entry"));
    itemEditMenu.Append(ID_EDITBASEENTRY,  wxT("&Edit Base entry"));

    switch (item->GetEntryType()) {
      case CItemData::ET_NORMAL:
      case CItemData::ET_SHORTCUTBASE:
        itemEditMenu.Delete(ID_GOTOBASEENTRY);
        itemEditMenu.Delete(ID_EDITBASEENTRY);
        break;

      case CItemData::ET_ALIASBASE:
        itemEditMenu.Delete(ID_CREATESHORTCUT);
        itemEditMenu.Delete(ID_GOTOBASEENTRY);
        itemEditMenu.Delete(ID_EDITBASEENTRY);
        break;

      case CItemData::ET_ALIAS:
      case CItemData::ET_SHORTCUT:
        itemEditMenu.Delete(ID_CREATESHORTCUT);
        break;
      
      default:
        wxASSERT_MSG(false, wxT("Unexpected CItemData type"));
        break;
    }
    
    uuid_array_t entry_uuid, base_uuid;
    if (item->IsShortcut()) {
      item->GetUUID(entry_uuid);
      m_core.GetShortcutBaseUUID(entry_uuid, base_uuid);
      ItemListIter iter = m_core.Find(base_uuid);
      if (iter != m_core.GetEntryEndIter())
        item = &iter->second;
    }

    if (item->IsUserEmpty())
      itemEditMenu.Delete(ID_COPYUSERNAME);

    if (item->IsNotesEmpty())
      itemEditMenu.Delete(ID_COPYNOTESFLD);

    if (item->IsEmailEmpty() && !item->IsURLEmail()) {
      itemEditMenu.Delete(ID_COPYEMAIL);
      itemEditMenu.Delete(ID_SENDEMAIL);
    }

    if ( item->IsURLEmpty()) {
      itemEditMenu.Delete(ID_COPYURL);
      itemEditMenu.Delete(ID_BROWSEURL);
      itemEditMenu.Delete(ID_BROWSEURLPLUS);
    }

    if (item->IsRunCommandEmpty()) {
      itemEditMenu.Delete(ID_COPYRUNCOMMAND);
      itemEditMenu.Delete(ID_RUNCOMMAND);
    }

    if ( m_currentView == TREE )
      m_tree->PopupMenu(&itemEditMenu);
    else
      m_grid->PopupMenu(&itemEditMenu);
  }
}

//-----------------------------------------------------------------
// Remove all DialogBlock-generated stubs below this line, as we
// already have them implemented in mainEdit.cpp
// (how to get DB to stop generating them??)
//-----------------------------------------------------------------
