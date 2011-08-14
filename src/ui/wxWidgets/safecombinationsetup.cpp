/*
 * Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file safecombinationsetup.cpp
* 
*/
// Generated by DialogBlocks, Wed 21 Jan 2009 09:07:57 PM IST

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

#include "safecombinationsetup.h"
#include "core/PWCharPool.h" // for CheckPassword()
#include "./wxutils.h"          // for ApplyPasswordFont()
#include "./ExternalKeyboardButton.h"

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>
#endif

////@begin XPM images
////@end XPM images


/*!
 * CSafeCombinationSetup type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CSafeCombinationSetup, wxDialog )


/*!
 * CSafeCombinationSetup event table definition
 */

BEGIN_EVENT_TABLE( CSafeCombinationSetup, wxDialog )

////@begin CSafeCombinationSetup event table entries
  EVT_BUTTON( wxID_OK, CSafeCombinationSetup::OnOkClick )

  EVT_BUTTON( wxID_CANCEL, CSafeCombinationSetup::OnCancelClick )

////@end CSafeCombinationSetup event table entries

END_EVENT_TABLE()


/*!
 * CSafeCombinationSetup constructors
 */

CSafeCombinationSetup::CSafeCombinationSetup()
{
  Init();
}

CSafeCombinationSetup::CSafeCombinationSetup( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
  Init();
  Create(parent, id, caption, pos, size, style);
}


/*!
 * CSafeCombinationSetup creator
 */

bool CSafeCombinationSetup::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CSafeCombinationSetup creation
  SetExtraStyle(wxWS_EX_BLOCK_EVENTS);
  wxDialog::Create( parent, id, caption, pos, size, style );

  CreateControls();
  if (GetSizer())
  {
    GetSizer()->SetSizeHints(this);
  }
  Centre();
////@end CSafeCombinationSetup creation
  return true;
}


/*!
 * CSafeCombinationSetup destructor
 */

CSafeCombinationSetup::~CSafeCombinationSetup()
{
////@begin CSafeCombinationSetup destruction
////@end CSafeCombinationSetup destruction
}


/*!
 * Member initialisation
 */

void CSafeCombinationSetup::Init()
{
////@begin CSafeCombinationSetup member initialisation
////@end CSafeCombinationSetup member initialisation
}


/*!
 * Control creation for CSafeCombinationSetup
 */

void CSafeCombinationSetup::CreateControls()
{    
////@begin CSafeCombinationSetup content construction
  CSafeCombinationSetup* itemDialog1 = this;

  wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);

  wxStaticText* itemStaticText3 = new wxStaticText( itemDialog1, wxID_STATIC, _("A new password database will be created.\nThe safe combination will be used to encrypt the password database file.\nYou can use any keyboard character. The combination is case-sensitive.\n"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer2->Add(itemStaticText3, 0, wxALIGN_LEFT|wxALL, 5);

  wxGridSizer* itemGridSizer4 = new wxGridSizer(2, 2, 0, -50);
  itemBoxSizer2->Add(itemGridSizer4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxStaticText* itemStaticText5 = new wxStaticText( itemDialog1, wxID_STATIC, _("Safe Combination:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemGridSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl6 = new wxTextCtrl( itemDialog1, ID_PASSWORD, wxEmptyString, wxDefaultPosition, wxSize(itemDialog1->ConvertDialogToPixels(wxSize(120, -1)).x, -1), wxTE_PASSWORD );
  itemGridSizer4->Add(itemTextCtrl6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText7 = new wxStaticText( itemDialog1, wxID_STATIC, _("Verify:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemGridSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl8 = new wxTextCtrl( itemDialog1, ID_VERIFY, wxEmptyString, wxDefaultPosition, wxSize(itemDialog1->ConvertDialogToPixels(wxSize(120, -1)).x, -1), wxTE_PASSWORD );
  itemGridSizer4->Add(itemTextCtrl8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStdDialogButtonSizer* sizer = CreateStdDialogButtonSizer(wxOK|wxCANCEL|wxHELP);
  sizer->Add(new ExternalKeyboardButton(itemDialog1), wxSizerFlags().Border(wxLEFT));
  itemBoxSizer2->Add(sizer, wxSizerFlags().Border().Expand().Proportion(0));

  SetSizerAndFit(itemBoxSizer2);

  // Set validators
  itemTextCtrl6->SetValidator( wxGenericValidator(& m_password) );
  itemTextCtrl8->SetValidator( wxGenericValidator(& m_verify) );
////@end CSafeCombinationSetup content construction
}


/*!
 * Should we show tooltips?
 */

bool CSafeCombinationSetup::ShowToolTips()
{
  return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CSafeCombinationSetup::GetBitmapResource( const wxString& name )
{
  // Bitmap retrieval
////@begin CSafeCombinationSetup bitmap retrieval
  wxUnusedVar(name);
  return wxNullBitmap;
////@end CSafeCombinationSetup bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CSafeCombinationSetup::GetIconResource( const wxString& name )
{
  // Icon retrieval
////@begin CSafeCombinationSetup icon retrieval
  wxUnusedVar(name);
  return wxNullIcon;
////@end CSafeCombinationSetup icon retrieval
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void CSafeCombinationSetup::OnOkClick( wxCommandEvent& /* evt */ )
{
  if (Validate() && TransferDataFromWindow()) {
    if (m_password != m_verify) {
      wxMessageDialog err(this, _("The two entries do not match."),
                          _("Error"), wxOK | wxICON_EXCLAMATION);
      err.ShowModal();
      return;
    }
    if (m_password.empty()) {
      wxMessageDialog err(this, _("Please enter the key and verify it."),
                          _("Error"), wxOK | wxICON_EXCLAMATION);
      err.ShowModal();
      return;
    }
    // Vox populi vox dei - folks want the ability to use a weak
    // passphrase, best we can do is warn them...
    // If someone want to build a version that insists on proper
    // passphrases, then just define the preprocessor macro
    // PWS_FORCE_STRONG_PASSPHRASE in the build properties/Makefile
    // (also used in CPasskeyChangeDlg)
#ifndef _DEBUG // for debug, we want no checks at all, to save time
    StringX errmess;
    if (!CPasswordCharPool::CheckPassword(tostringx(m_password), errmess)) {
      wxString cs_msg;
      cs_msg = _("Weak passphrase:\n\n");
      cs_msg += errmess.c_str();
#ifndef PWS_FORCE_STRONG_PASSPHRASE
      cs_msg += _("\nUse it anyway?");
      wxMessageDialog mb(this, cs_msg, _("Warning"),
                      wxYES_NO | wxNO_DEFAULT | wxICON_HAND);
      int rc = mb.ShowModal();
    if (rc == wxID_NO)
      return;
#else
    cs_msg += _("\nPlease try another");
    wxMessageDialog mb(this, cs_msg, _("Error"), wxOK | wxICON_HAND);
    mb.ShowModal();
    return;
#endif // PWS_FORCE_STRONG_PASSPHRASE
    }
#endif // _DEBUG
    EndModal(wxID_OK);
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void CSafeCombinationSetup::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in CSafeCombinationSetup.
  // Before editing this code, remove the block markers.
  event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in CSafeCombinationSetup. 
}
