/*
 * Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file safecombinationentry.cpp
* 
*/
// Generated by DialogBlocks, Sun 18 Jan 2009 09:22:13 PM IST

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes

#include "safecombinationentry.h"
#include "safecombinationsetup.h"
#include "version.h"
#include "corelib/PWSdirs.h"
#include "os/file.h"
////@begin XPM images
////@end XPM images
#include "../graphics/wxWidgets/cpane.xpm"
#include "../graphics/wxWidgets/psafetxt.xpm"


/*!
 * CSafeCombinationEntry type definition
 */

IMPLEMENT_CLASS( CSafeCombinationEntry, wxDialog )


/*!
 * CSafeCombinationEntry event table definition
 */

BEGIN_EVENT_TABLE( CSafeCombinationEntry, wxDialog )

////@begin CSafeCombinationEntry event table entries
  EVT_BUTTON( ID_ELLIPSIS, CSafeCombinationEntry::OnEllipsisClick )

  EVT_BUTTON( ID_NEWDB, CSafeCombinationEntry::OnNewDbClick )

  EVT_BUTTON( wxID_OK, CSafeCombinationEntry::OnOk )

  EVT_BUTTON( wxID_CANCEL, CSafeCombinationEntry::OnCancel )

////@end CSafeCombinationEntry event table entries

END_EVENT_TABLE()


/*!
 * CSafeCombinationEntry constructors
 */

CSafeCombinationEntry::CSafeCombinationEntry(PWScore &core)
: m_core(core), m_tries(0)
{
  Init();
}

CSafeCombinationEntry::CSafeCombinationEntry(wxWindow* parent, PWScore &core,
                                             wxWindowID id,
                                             const wxString& caption,
                                             const wxPoint& pos,
                                             const wxSize& size, long style)
  : m_core(core), m_tries(0)
{
  Init();
  Create(parent, id, caption, pos, size, style);
}


/*!
 * CSafeCombinationEntry creator
 */

bool CSafeCombinationEntry::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CSafeCombinationEntry creation
  SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
  wxDialog::Create( parent, id, caption, pos, size, style );

  CreateControls();
  if (GetSizer())
  {
    GetSizer()->SetSizeHints(this);
  }
  Centre();
////@end CSafeCombinationEntry creation
  return true;
}


/*!
 * CSafeCombinationEntry destructor
 */

CSafeCombinationEntry::~CSafeCombinationEntry()
{
////@begin CSafeCombinationEntry destruction
////@end CSafeCombinationEntry destruction
}


/*!
 * Member initialisation
 */

void CSafeCombinationEntry::Init()
{
  m_readOnly = m_core.IsReadOnly();
  m_filename = m_core.GetCurFile().c_str();
////@begin CSafeCombinationEntry member initialisation
////@end CSafeCombinationEntry member initialisation
}


/*!
 * Control creation for CSafeCombinationEntry
 */

void CSafeCombinationEntry::CreateControls()
{    
////@begin CSafeCombinationEntry content construction
  CSafeCombinationEntry* itemDialog1 = this;

  wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
  itemDialog1->SetSizer(itemBoxSizer2);

  wxStaticBitmap* itemStaticBitmap3 = new wxStaticBitmap(itemDialog1, wxID_STATIC,
                                                         wxBitmap(cpane_xpm),
                                                         wxDefaultPosition,
                                                         itemDialog1->ConvertDialogToPixels(wxSize(49, 46)), 0 );
  itemBoxSizer2->Add(itemStaticBitmap3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
  itemBoxSizer2->Add(itemBoxSizer4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer4->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxStaticBitmap* itemStaticBitmap6 = new wxStaticBitmap(itemDialog1, wxID_STATIC,
                                                          wxBitmap(psafetxt_xpm),
                                                         wxDefaultPosition,
                                                         itemDialog1->ConvertDialogToPixels(wxSize(111, 16)), 0 );
  itemBoxSizer5->Add(itemStaticBitmap6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText7 = new wxStaticText(itemDialog1, wxID_STATIC,
                                                   wxString::Format(_("Version %d.%d"),
                                                                    MAJORVERSION,
                                                                    MINORVERSION),
                                                   wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer5->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText8 = new wxStaticText( itemDialog1, wxID_STATIC, _("Open Password Database:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer4->Add(itemStaticText8, 0, wxALIGN_LEFT|wxALL, 3);

  wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer4->Add(itemBoxSizer9, 50, wxGROW|wxALL, 5);

  wxArrayString itemComboBox10Strings;
  wxComboBox* itemComboBox10 = new wxComboBox( itemDialog1, ID_DBASECOMBOBOX, _T(""), wxDefaultPosition, wxSize(itemDialog1->ConvertDialogToPixels(wxSize(140, -1)).x, -1), itemComboBox10Strings, wxCB_DROPDOWN );
  itemBoxSizer9->Add(itemComboBox10, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 0);
  itemComboBox10->SetFocus();

  wxButton* itemButton11 = new wxButton( itemDialog1, ID_ELLIPSIS, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
  itemBoxSizer9->Add(itemButton11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText12 = new wxStaticText( itemDialog1, wxID_STATIC, _("Safe Combination:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer4->Add(itemStaticText12, 0, wxALIGN_LEFT|wxALL, 3);

  wxTextCtrl* itemTextCtrl13 = new wxTextCtrl( itemDialog1, ID_PASSWORD, _T(""), wxDefaultPosition, wxSize(itemDialog1->ConvertDialogToPixels(wxSize(160, -1)).x, -1), wxTE_PASSWORD );
  itemBoxSizer4->Add(itemTextCtrl13, 0, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 5);

  wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer4->Add(itemBoxSizer14, 0, wxGROW|wxALL, 5);

  wxCheckBox* itemCheckBox15 = new wxCheckBox( itemDialog1, ID_READONLY, _("Open as read-only"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox15->SetValue(false);
  itemBoxSizer14->Add(itemCheckBox15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  itemBoxSizer14->Add(120, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton17 = new wxButton( itemDialog1, ID_NEWDB, _("New\nDatabase"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
  itemBoxSizer14->Add(itemButton17, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM, 5);

  wxStdDialogButtonSizer* itemStdDialogButtonSizer18 = new wxStdDialogButtonSizer;

  itemBoxSizer4->Add(itemStdDialogButtonSizer18, 0, wxGROW|wxALL, 0);
  wxButton* itemButton19 = new wxButton( itemDialog1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
  itemButton19->SetDefault();
  itemStdDialogButtonSizer18->AddButton(itemButton19);

  wxButton* itemButton20 = new wxButton( itemDialog1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
  itemStdDialogButtonSizer18->AddButton(itemButton20);

  wxButton* itemButton21 = new wxButton( itemDialog1, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, 0 );
  itemStdDialogButtonSizer18->AddButton(itemButton21);

  itemStdDialogButtonSizer18->Realize();

  // Set validators
  itemComboBox10->SetValidator( wxGenericValidator(& m_filename) );
  itemTextCtrl13->SetValidator( wxGenericValidator(& m_password) );
  itemCheckBox15->SetValidator( wxGenericValidator(& m_readOnly) );
////@end CSafeCombinationEntry content construction
  // if m_readOnly, then don't allow user to change it
  itemCheckBox15->Enable(!m_readOnly);
  // if filename field not empty, set focus to password:
  if (!m_filename.empty()) {
    FindWindow(ID_PASSWORD)->SetFocus();
  }
}


/*!
 * Should we show tooltips?
 */

bool CSafeCombinationEntry::ShowToolTips()
{
  return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CSafeCombinationEntry::GetBitmapResource( const wxString& name )
{
  // Bitmap retrieval
////@begin CSafeCombinationEntry bitmap retrieval
  wxUnusedVar(name);
  if (name == _T("../graphics/cpane.bmp"))
  {
    wxBitmap bitmap(_T("../graphics/cpane.bmp"), wxBITMAP_TYPE_BMP);
    return bitmap;
  }
  else if (name == _T("../graphics/psafetxt.bmp"))
  {
    wxBitmap bitmap(_T("../graphics/psafetxt.bmp"), wxBITMAP_TYPE_BMP);
    return bitmap;
  }
  return wxNullBitmap;
////@end CSafeCombinationEntry bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CSafeCombinationEntry::GetIconResource( const wxString& name )
{
  // Icon retrieval
////@begin CSafeCombinationEntry icon retrieval
  wxUnusedVar(name);
  return wxNullIcon;
////@end CSafeCombinationEntry icon retrieval
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void CSafeCombinationEntry::OnOk( wxCommandEvent& )
{
  if (Validate() && TransferDataFromWindow()) {
    if (m_password.empty()) {
      wxMessageDialog err(this, _("The combination cannot be blank."),
                          _("Error"), wxOK | wxICON_EXCLAMATION);
      err.ShowModal();
      return;
    }
    if (!pws_os::FileExists(m_filename.c_str())) {
      wxMessageDialog err(this, _("File or path not found."),
                          _("Error"), wxOK | wxICON_EXCLAMATION);
      err.ShowModal();
      return;
    }
    if (m_core.CheckPasskey(m_filename.c_str(),
                            m_password.c_str()) != PWScore::SUCCESS) {
      wxString errmess;
      if (m_tries >= 2) {
        errmess = _("Three strikes - yer out!");
      } else {
        m_tries++;
        errmess = _("Incorrect passkey, not a PasswordSafe database, or a corrupt database. (Backup database has same name as original, ending with '~')");
      }
      wxMessageDialog err(this, errmess,
                          _("Error"), wxOK | wxICON_EXCLAMATION);
      err.ShowModal();
      wxTextCtrl *txt = (wxTextCtrl *)FindWindow(ID_PASSWORD);
      txt->SetSelection(-1,-1);
      txt->SetFocus();
      return;
    }
    m_core.SetReadOnly(m_readOnly);
    m_core.SetCurFile(m_filename.c_str());
    EndModal(wxID_OK);
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void CSafeCombinationEntry::OnCancel( wxCommandEvent& evt )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in CSafeCombinationEntry.
  // Before editing this code, remove the block markers.
  evt.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in CSafeCombinationEntry. 
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ELLIPSIS
 */

void CSafeCombinationEntry::OnEllipsisClick( wxCommandEvent& /* evt */ )
{
  wxFileDialog fd(this, _("Please Choose a Database to Open:"),
                  wxT(""), wxT(""),
                  _("Password Safe Databases (*.psafe3; *.dat)"
                    "|*.psafe3; *.dat| All files (*.*)|*.*"),
                  (wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR));
  
  if (fd.ShowModal() == wxID_OK) {
    m_filename = fd.GetPath();
    wxComboBox *cb = (wxComboBox *)FindWindow(ID_DBASECOMBOBOX);
    cb->SetValue(m_filename);
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_NEWDB
 */

void CSafeCombinationEntry::OnNewDbClick( wxCommandEvent& /* evt */ )
{
  // 1. Get a filename from a file dialog box
  // 2. Get a password
  // 3. Set m_filespec && m_passkey to returned value!
  wxString newfile;
  wxString cs_msg, cs_title, cs_temp;

  wxString cf(_("pwsafe")); // reasonable default for first time user
  stringT v3FileName = PWSUtil::GetNewFileName(cf.c_str(), _("psafe3"));
  stringT dir = PWSdirs::GetSafeDir();

  while (1) {
    wxFileDialog fd(this, _("Please choose a name for the new database"),
                    dir.c_str(), v3FileName.c_str(),
                    _("Password Safe Databases (*.psafe3; *.dat)"
                      "|*.psafe3; *.dat| All files (*.*)|*.*"),
                  (wxFD_SAVE | wxFD_OVERWRITE_PROMPT| wxFD_CHANGE_DIR));
    int rc = fd.ShowModal();
    if (rc == wxID_OK) {
      newfile = fd.GetPath();
      break;
    } else
      return;
  }
  // 2. Get a password
  CSafeCombinationSetup pksetup(this);
  int rc = pksetup.ShowModal();

  if (rc != wxID_OK)
    return;  //User cancelled password entry

  // 3. Set m_filespec && m_passkey to returned value!
  m_core.SetCurFile(newfile.c_str());
  m_password = pksetup.GetPassword();
  EndModal(wxID_OK);
}

