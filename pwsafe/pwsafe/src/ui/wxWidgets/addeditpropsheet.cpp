/*
 * Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file addeditpropsheet.cpp
* 
*/
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "wx/bookctrl.h"
////@end includes
#include <wx/datetime.h>

#include <vector>
#include "corelib/PWSprefs.h"
#include "corelib/PWCharPool.h"
#include "corelib/PWHistory.h"

#include "addeditpropsheet.h"
#include "PWSgrid.h"
#include "PWStree.h"
#include "pwsclip.h"

////@begin XPM images
////@end XPM images


/*!
 * AddEditPropSheet type definition
 */

IMPLEMENT_CLASS( AddEditPropSheet, wxPropertySheetDialog )


/*!
 * AddEditPropSheet event table definition
 */

BEGIN_EVENT_TABLE( AddEditPropSheet, wxPropertySheetDialog )

  EVT_BUTTON( wxID_OK, AddEditPropSheet::OnOk )
////@begin AddEditPropSheet event table entries
  EVT_BUTTON( ID_BUTTON2, AddEditPropSheet::OnShowHideClick )

  EVT_BUTTON( ID_BUTTON3, AddEditPropSheet::OnGenerateButtonClick )

  EVT_BUTTON( ID_GO_BTN, AddEditPropSheet::OnGoButtonClick )

  EVT_CHECKBOX( ID_CHECKBOX, AddEditPropSheet::OnOverrideDCAClick )

  EVT_CHECKBOX( ID_CHECKBOX1, AddEditPropSheet::OnKeepHistoryClick )

  EVT_RADIOBUTTON( ID_RADIOBUTTON, AddEditPropSheet::OnRadiobuttonSelected )

  EVT_RADIOBUTTON( ID_RADIOBUTTON1, AddEditPropSheet::OnRadiobuttonSelected )

  EVT_BUTTON( ID_BUTTON5, AddEditPropSheet::OnSetXTime )

  EVT_BUTTON( ID_BUTTON6, AddEditPropSheet::OnClearXTime )

  EVT_RADIOBUTTON( ID_RADIOBUTTON2, AddEditPropSheet::OnPWPRBSelected )

  EVT_RADIOBUTTON( ID_RADIOBUTTON3, AddEditPropSheet::OnPWPRBSelected )

  EVT_CHECKBOX( ID_CHECKBOX7, AddEditPropSheet::OnEZreadCBClick )

  EVT_CHECKBOX( ID_CHECKBOX8, AddEditPropSheet::OnPronouceableCBClick )

  EVT_CHECKBOX( ID_CHECKBOX9, AddEditPropSheet::OnUseHexCBClick )

////@end AddEditPropSheet event table entries

END_EVENT_TABLE()


/*!
 * AddEditPropSheet constructors
 */

AddEditPropSheet::AddEditPropSheet(wxWindow* parent, PWScore &core,
                                   PWSGrid *grid, PWSTreeCtrl *tree,
                                   AddOrEdit type, const CItemData *item,
                                   wxWindowID id, const wxString& caption,
                                   const wxPoint& pos, const wxSize& size,
                                   long style)
: m_core(core), m_grid(grid), m_tree(tree), m_type(type)
{
  if (item != NULL)
    m_item = *item; // copy existing item to display values
  else
    m_item.CreateUUID(); // We're adding a new entry
  Init();
  Create(parent, id, caption, pos, size, style);
}


/*!
 * AddEditPropSheet creator
 */

bool AddEditPropSheet::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin AddEditPropSheet creation
  SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY|wxWS_EX_BLOCK_EVENTS);
  wxPropertySheetDialog::Create( parent, id, caption, pos, size, style );

  CreateButtons(wxOK|wxCANCEL|wxHELP);
  CreateControls();
  LayoutDialog();
  Centre();
////@end AddEditPropSheet creation
  ItemFieldsToPropSheet();
  return true;
}

/*!
 * AddEditPropSheet destructor
 */

AddEditPropSheet::~AddEditPropSheet()
{
////@begin AddEditPropSheet destruction
////@end AddEditPropSheet destruction
}

// following based on m_DCAcomboBoxStrings
// which is generated via DialogBlocks - unify these later
static struct {short pv; wxString name;}
  dcaMapping[] =
    {{PWSprefs::DoubleClickAutoType, _("Auto Type")},
     {PWSprefs::DoubleClickBrowse, _("Browse")},
     {PWSprefs::DoubleClickBrowsePlus, _("Browse + Auto Type")},
     {PWSprefs::DoubleClickCopyNotes, _("Copy Notes")},
     {PWSprefs::DoubleClickCopyPassword, _("Copy Password")},
     {PWSprefs::DoubleClickCopyPasswordMinimize, _("Copy Password + Minimize")},
     {PWSprefs::DoubleClickCopyUsername, _("Copy Username")},
     {PWSprefs::DoubleClickViewEdit, _("View/Edit Entry")},
     {PWSprefs::DoubleClickRun, _("Execute Run command")},
    };

/*!
 * Member initialisation
 */

void AddEditPropSheet::Init()
{
////@begin AddEditPropSheet member initialisation
  m_XTimeInt = 0;
  m_groupCtrl = NULL;
  m_PasswordCtrl = NULL;
  m_Password1HiddenCtrl = NULL;
  m_ShowHideCtrl = NULL;
  m_Password2Ctrl = NULL;
  m_DCAcomboBox = NULL;
  m_MaxPWHistCtrl = NULL;
  m_PWHgrid = NULL;
  m_OnRB = NULL;
  m_ExpDate = NULL;
  m_ExpTimeH = NULL;
  m_ExpTimeM = NULL;
  m_InRB = NULL;
  m_ExpTimeCtrl = NULL;
  m_RecurringCtrl = NULL;
  m_defPWPRB = NULL;
  m_ourPWPRB = NULL;
  m_pwpLenCtrl = NULL;
  m_pwMinsGSzr = NULL;
  m_pwpUseLowerCtrl = NULL;
  m_pwNumLCbox = NULL;
  m_pwpLCSpin = NULL;
  m_pwpUseUpperCtrl = NULL;
  m_pwNumUCbox = NULL;
  m_pwpUCSpin = NULL;
  m_pwpUseDigitsCtrl = NULL;
  m_pwNumDigbox = NULL;
  m_pwpDigSpin = NULL;
  m_pwpSymCtrl = NULL;
  m_pwNumSymbox = NULL;
  m_pwpSymSpin = NULL;
  m_pwpEasyCtrl = NULL;
  m_pwpPronounceCtrl = NULL;
  m_pwpHexCtrl = NULL;
////@end AddEditPropSheet member initialisation
}


const wxChar *DCAs[] = {
  _("Auto Type"),
  _("Browse"),
  _("Browse + Auto Type"),
  _("Copy Notes"),
  _("Copy Password"),
  _("Copy Password + Minimize"),
  _("Copy Username"),
  _("View/Edit Entry"),
  _("Execute Run command"),
};

/*!
 * Control creation for AddEditPropSheet
 */

void AddEditPropSheet::CreateControls()
{    
////@begin AddEditPropSheet content construction
  AddEditPropSheet* itemPropertySheetDialog1 = this;

  wxPanel* itemPanel2 = new wxPanel( GetBookCtrl(), ID_PANEL_BASIC, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
  wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
  itemPanel2->SetSizer(itemBoxSizer3);

  wxStaticText* itemStaticText4 = new wxStaticText( itemPanel2, wxID_STATIC, _("To add a new entry, simply fill in the fields below. At least a title and a\npassword are required. If you have set a default username, it will appear in the\nusername field."), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxFlexGridSizer* itemFlexGridSizer5 = new wxFlexGridSizer(0, 3, 0, 0);
  itemBoxSizer3->Add(itemFlexGridSizer5, 0, wxALIGN_LEFT|wxALL, 5);
  wxStaticText* itemStaticText6 = new wxStaticText( itemPanel2, wxID_STATIC, _("Group:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxArrayString m_groupCtrlStrings;
  m_groupCtrl = new wxComboBox( itemPanel2, ID_COMBOBOX1, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_groupCtrlStrings, wxCB_DROPDOWN );
  itemFlexGridSizer5->Add(m_groupCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemFlexGridSizer5->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText9 = new wxStaticText( itemPanel2, wxID_STATIC, _("Title:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl10 = new wxTextCtrl( itemPanel2, ID_TEXTCTRL5, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemTextCtrl10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemFlexGridSizer5->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText12 = new wxStaticText( itemPanel2, wxID_STATIC, _("Username:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemStaticText12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl13 = new wxTextCtrl( itemPanel2, ID_TEXTCTRL1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemTextCtrl13, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemFlexGridSizer5->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText15 = new wxStaticText( itemPanel2, wxID_STATIC, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemStaticText15, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_PasswordCtrl = new wxTextCtrl( itemPanel2, ID_TEXTCTRL2, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(m_PasswordCtrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_Password1HiddenCtrl = new wxTextCtrl( itemPanel2, ID_TEXTCTRL16, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
  m_Password1HiddenCtrl->Show(false);
  itemFlexGridSizer5->Add(m_Password1HiddenCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer5->Add(itemBoxSizer18, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  itemBoxSizer18->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_ShowHideCtrl = new wxButton( itemPanel2, ID_BUTTON2, _("&Hide"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer18->Add(m_ShowHideCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  itemBoxSizer18->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton22 = new wxButton( itemPanel2, ID_BUTTON3, _("&Generate"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer18->Add(itemButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText23 = new wxStaticText( itemPanel2, wxID_STATIC, _("Confirm:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemStaticText23, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_Password2Ctrl = new wxTextCtrl( itemPanel2, ID_TEXTCTRL3, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
  itemFlexGridSizer5->Add(m_Password2Ctrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemFlexGridSizer5->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText26 = new wxStaticText( itemPanel2, wxID_STATIC, _("URL:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemStaticText26, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl27 = new wxTextCtrl( itemPanel2, ID_TEXTCTRL4, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemTextCtrl27, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer28 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer5->Add(itemBoxSizer28, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  itemBoxSizer28->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton30 = new wxButton( itemPanel2, ID_GO_BTN, _("Go"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer28->Add(itemButton30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxBoxSizer* itemBoxSizer31 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer3->Add(itemBoxSizer31, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText32 = new wxStaticText( itemPanel2, wxID_STATIC, _("Notes:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer31->Add(itemStaticText32, 1, wxALIGN_TOP|wxALL, 5);

  wxTextCtrl* itemTextCtrl33 = new wxTextCtrl( itemPanel2, ID_TEXTCTRL7, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
  itemBoxSizer31->Add(itemTextCtrl33, 5, wxALIGN_CENTER_VERTICAL|wxALL, 3);

  GetBookCtrl()->AddPage(itemPanel2, _("Basic"));

  wxPanel* itemPanel34 = new wxPanel( GetBookCtrl(), ID_PANEL_ADDITIONAL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
  wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxVERTICAL);
  itemPanel34->SetSizer(itemBoxSizer35);

  wxFlexGridSizer* itemFlexGridSizer36 = new wxFlexGridSizer(0, 2, 0, 0);
  itemBoxSizer35->Add(itemFlexGridSizer36, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText37 = new wxStaticText( itemPanel34, wxID_STATIC, _("Autotype:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer36->Add(itemStaticText37, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl38 = new wxTextCtrl( itemPanel34, ID_TEXTCTRL6, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer36->Add(itemTextCtrl38, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText39 = new wxStaticText( itemPanel34, wxID_STATIC, _("Run Cmd:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer36->Add(itemStaticText39, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl40 = new wxTextCtrl( itemPanel34, ID_TEXTCTRL8, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer36->Add(itemTextCtrl40, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText41 = new wxStaticText( itemPanel34, wxID_STATIC, _("Double-Click\nAction:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer36->Add(itemStaticText41, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer42 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer36->Add(itemBoxSizer42, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);
  wxCheckBox* itemCheckBox43 = new wxCheckBox( itemPanel34, ID_CHECKBOX, _("Use Default"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox43->SetValue(false);
  itemBoxSizer42->Add(itemCheckBox43, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxArrayString m_DCAcomboBoxStrings;
  m_DCAcomboBoxStrings.Add(_("Auto Type"));
  m_DCAcomboBoxStrings.Add(_("Browse"));
  m_DCAcomboBoxStrings.Add(_("Browse + Auto Type"));
  m_DCAcomboBoxStrings.Add(_("Copy Notes"));
  m_DCAcomboBoxStrings.Add(_("Copy Password"));
  m_DCAcomboBoxStrings.Add(_("Copy Password + Minimize"));
  m_DCAcomboBoxStrings.Add(_("Copy Username"));
  m_DCAcomboBoxStrings.Add(_("View/Edit Entry"));
  m_DCAcomboBoxStrings.Add(_("Execute Run command"));
  m_DCAcomboBox = new wxComboBox( itemPanel34, ID_COMBOBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_DCAcomboBoxStrings, wxCB_READONLY );
  itemBoxSizer42->Add(m_DCAcomboBox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticBox* itemStaticBoxSizer45Static = new wxStaticBox(itemPanel34, wxID_ANY, _("Password History"));
  wxStaticBoxSizer* itemStaticBoxSizer45 = new wxStaticBoxSizer(itemStaticBoxSizer45Static, wxVERTICAL);
  itemBoxSizer35->Add(itemStaticBoxSizer45, 0, wxGROW|wxALL, 5);
  wxBoxSizer* itemBoxSizer46 = new wxBoxSizer(wxHORIZONTAL);
  itemStaticBoxSizer45->Add(itemBoxSizer46, 0, wxGROW|wxALL, 5);
  wxCheckBox* itemCheckBox47 = new wxCheckBox( itemPanel34, ID_CHECKBOX1, _("Keep"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox47->SetValue(false);
  itemBoxSizer46->Add(itemCheckBox47, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_MaxPWHistCtrl = new wxSpinCtrl( itemPanel34, ID_SPINCTRL, _T("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
  itemBoxSizer46->Add(m_MaxPWHistCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText49 = new wxStaticText( itemPanel34, wxID_STATIC, _("last passwords"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer46->Add(itemStaticText49, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_PWHgrid = new wxGrid( itemPanel34, ID_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
  m_PWHgrid->SetDefaultColSize(150);
  m_PWHgrid->SetDefaultRowSize(25);
  m_PWHgrid->SetColLabelSize(25);
  m_PWHgrid->SetRowLabelSize(0);
  m_PWHgrid->CreateGrid(5, 2, wxGrid::wxGridSelectRows);
  itemStaticBoxSizer45->Add(m_PWHgrid, 0, wxGROW|wxALL, 5);

  wxBoxSizer* itemBoxSizer51 = new wxBoxSizer(wxHORIZONTAL);
  itemStaticBoxSizer45->Add(itemBoxSizer51, 0, wxGROW|wxALL, 5);
  wxButton* itemButton52 = new wxButton( itemPanel34, ID_BUTTON1, _("Clear History"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer51->Add(itemButton52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemBoxSizer51->Add(10, 10, 10, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton54 = new wxButton( itemPanel34, ID_BUTTON4, _("Copy All"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer51->Add(itemButton54, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  GetBookCtrl()->AddPage(itemPanel34, _("Additional"));

  wxPanel* itemPanel55 = new wxPanel( GetBookCtrl(), ID_PANEL_DTIME, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
  wxBoxSizer* itemBoxSizer56 = new wxBoxSizer(wxVERTICAL);
  itemPanel55->SetSizer(itemBoxSizer56);

  wxStaticBox* itemStaticBoxSizer57Static = new wxStaticBox(itemPanel55, wxID_ANY, _("Password Expiry"));
  wxStaticBoxSizer* itemStaticBoxSizer57 = new wxStaticBoxSizer(itemStaticBoxSizer57Static, wxVERTICAL);
  itemBoxSizer56->Add(itemStaticBoxSizer57, 0, wxGROW|wxALL, 5);
  wxBoxSizer* itemBoxSizer58 = new wxBoxSizer(wxVERTICAL);
  itemStaticBoxSizer57->Add(itemBoxSizer58, 0, wxGROW|wxALL, 5);
  wxBoxSizer* itemBoxSizer59 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer58->Add(itemBoxSizer59, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText60 = new wxStaticText( itemPanel55, wxID_STATIC, _("Password Expires on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer59->Add(itemStaticText60, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText61 = new wxStaticText( itemPanel55, wxID_STATIC, _("Whenever"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer59->Add(itemStaticText61, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxFlexGridSizer* itemFlexGridSizer62 = new wxFlexGridSizer(0, 3, 0, 0);
  itemBoxSizer58->Add(itemFlexGridSizer62, 0, wxGROW|wxALL, 5);
  m_OnRB = new wxRadioButton( itemPanel55, ID_RADIOBUTTON, _("On"), wxDefaultPosition, wxDefaultSize, 0 );
  m_OnRB->SetValue(false);
  itemFlexGridSizer62->Add(m_OnRB, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_ExpDate = new wxDatePickerCtrl( itemPanel55, ID_DATECTRL, wxDateTime(10, (wxDateTime::Month) 7, 2009), wxDefaultPosition, wxDefaultSize, wxDP_DEFAULT );
  itemFlexGridSizer62->Add(m_ExpDate, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer65 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer62->Add(itemBoxSizer65, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);
  m_ExpTimeH = new wxSpinCtrl( itemPanel55, ID_SPINCTRL1, _T("12"), wxDefaultPosition, wxSize(itemPanel55->ConvertDialogToPixels(wxSize(20, -1)).x, -1), wxSP_ARROW_KEYS, 0, 23, 12 );
  itemBoxSizer65->Add(m_ExpTimeH, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText67 = new wxStaticText( itemPanel55, wxID_STATIC, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer65->Add(itemStaticText67, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_ExpTimeM = new wxSpinCtrl( itemPanel55, ID_SPINCTRL4, _T("0"), wxDefaultPosition, wxSize(itemPanel55->ConvertDialogToPixels(wxSize(20, -1)).x, -1), wxSP_ARROW_KEYS, 0, 59, 0 );
  itemBoxSizer65->Add(m_ExpTimeM, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_InRB = new wxRadioButton( itemPanel55, ID_RADIOBUTTON1, _("In"), wxDefaultPosition, wxDefaultSize, 0 );
  m_InRB->SetValue(false);
  itemFlexGridSizer62->Add(m_InRB, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer70 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer62->Add(itemBoxSizer70, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  m_ExpTimeCtrl = new wxSpinCtrl( itemPanel55, ID_SPINCTRL2, _T("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 3650, 1 );
  itemBoxSizer70->Add(m_ExpTimeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText72 = new wxStaticText( itemPanel55, wxID_STATIC, _("days"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer70->Add(itemStaticText72, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_RecurringCtrl = new wxCheckBox( itemPanel55, ID_CHECKBOX2, _("Recurring"), wxDefaultPosition, wxDefaultSize, 0 );
  m_RecurringCtrl->SetValue(false);
  itemFlexGridSizer62->Add(m_RecurringCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer74 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer58->Add(itemBoxSizer74, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText75 = new wxStaticText( itemPanel55, wxID_STATIC, _("Current Value:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer74->Add(itemStaticText75, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText76 = new wxStaticText( itemPanel55, wxID_STATIC, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer74->Add(itemStaticText76, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer77 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer58->Add(itemBoxSizer77, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  wxButton* itemButton78 = new wxButton( itemPanel55, ID_BUTTON5, _("&Set"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer77->Add(itemButton78, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemBoxSizer77->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton80 = new wxButton( itemPanel55, ID_BUTTON6, _("&Clear"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer77->Add(itemButton80, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticBox* itemStaticBoxSizer81Static = new wxStaticBox(itemPanel55, wxID_ANY, _("Statistics"));
  wxStaticBoxSizer* itemStaticBoxSizer81 = new wxStaticBoxSizer(itemStaticBoxSizer81Static, wxVERTICAL);
  itemBoxSizer56->Add(itemStaticBoxSizer81, 0, wxGROW|wxALL, 5);
  wxFlexGridSizer* itemFlexGridSizer82 = new wxFlexGridSizer(0, 2, 0, 0);
  itemStaticBoxSizer81->Add(itemFlexGridSizer82, 0, wxALIGN_LEFT|wxALL, 5);
  wxStaticText* itemStaticText83 = new wxStaticText( itemPanel55, wxID_STATIC, _("Created on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer82->Add(itemStaticText83, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText84 = new wxStaticText( itemPanel55, wxID_STATIC, _("10/06/2009 23:19:25"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer82->Add(itemStaticText84, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText85 = new wxStaticText( itemPanel55, wxID_STATIC, _("Password last changed on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer82->Add(itemStaticText85, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText86 = new wxStaticText( itemPanel55, wxID_STATIC, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer82->Add(itemStaticText86, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText87 = new wxStaticText( itemPanel55, wxID_STATIC, _("Last accessed on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer82->Add(itemStaticText87, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText88 = new wxStaticText( itemPanel55, wxID_STATIC, _("N/A"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer82->Add(itemStaticText88, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText89 = new wxStaticText( itemPanel55, wxID_STATIC, _("Any field last changed on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer82->Add(itemStaticText89, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText90 = new wxStaticText( itemPanel55, wxID_STATIC, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer82->Add(itemStaticText90, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  GetBookCtrl()->AddPage(itemPanel55, _("Dates and Times"));

  wxPanel* itemPanel91 = new wxPanel( GetBookCtrl(), ID_PANEL_PPOLICY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxTAB_TRAVERSAL );
  wxStaticBox* itemStaticBoxSizer92Static = new wxStaticBox(itemPanel91, wxID_ANY, _("Random password generation rules"));
  wxStaticBoxSizer* itemStaticBoxSizer92 = new wxStaticBoxSizer(itemStaticBoxSizer92Static, wxVERTICAL);
  itemPanel91->SetSizer(itemStaticBoxSizer92);

  m_defPWPRB = new wxRadioButton( itemPanel91, ID_RADIOBUTTON2, _("Use Database Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
  m_defPWPRB->SetValue(false);
  itemStaticBoxSizer92->Add(m_defPWPRB, 0, wxALIGN_LEFT|wxALL, 5);

  m_ourPWPRB = new wxRadioButton( itemPanel91, ID_RADIOBUTTON3, _("Use the policy below:"), wxDefaultPosition, wxDefaultSize, 0 );
  m_ourPWPRB->SetValue(false);
  itemStaticBoxSizer92->Add(m_ourPWPRB, 0, wxALIGN_LEFT|wxALL, 5);

  wxStaticLine* itemStaticLine95 = new wxStaticLine( itemPanel91, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
  itemStaticBoxSizer92->Add(itemStaticLine95, 0, wxGROW|wxALL, 5);

  wxBoxSizer* itemBoxSizer96 = new wxBoxSizer(wxHORIZONTAL);
  itemStaticBoxSizer92->Add(itemBoxSizer96, 0, wxALIGN_LEFT|wxALL, 5);
  wxStaticText* itemStaticText97 = new wxStaticText( itemPanel91, wxID_STATIC, _("Password length: "), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer96->Add(itemStaticText97, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpLenCtrl = new wxSpinCtrl( itemPanel91, ID_SPINCTRL3, _T("8"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 4, 1024, 8 );
  itemBoxSizer96->Add(m_pwpLenCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwMinsGSzr = new wxGridSizer(6, 2, 0, 0);
  itemStaticBoxSizer92->Add(m_pwMinsGSzr, 0, wxALIGN_LEFT|wxALL, 5);
  m_pwpUseLowerCtrl = new wxCheckBox( itemPanel91, ID_CHECKBOX3, _("Use lowercase letters"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpUseLowerCtrl->SetValue(false);
  m_pwMinsGSzr->Add(m_pwpUseLowerCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwNumLCbox = new wxBoxSizer(wxHORIZONTAL);
  m_pwMinsGSzr->Add(m_pwNumLCbox, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  wxStaticText* itemStaticText102 = new wxStaticText( itemPanel91, wxID_STATIC, _("(At least "), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumLCbox->Add(itemStaticText102, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpLCSpin = new wxSpinCtrl( itemPanel91, ID_SPINCTRL5, _T("0"), wxDefaultPosition, wxSize(itemPanel91->ConvertDialogToPixels(wxSize(20, -1)).x, -1), wxSP_ARROW_KEYS, 0, 100, 0 );
  m_pwNumLCbox->Add(m_pwpLCSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText104 = new wxStaticText( itemPanel91, wxID_STATIC, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumLCbox->Add(itemStaticText104, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpUseUpperCtrl = new wxCheckBox( itemPanel91, ID_CHECKBOX4, _("Use UPPERCASE letters"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpUseUpperCtrl->SetValue(false);
  m_pwMinsGSzr->Add(m_pwpUseUpperCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwNumUCbox = new wxBoxSizer(wxHORIZONTAL);
  m_pwMinsGSzr->Add(m_pwNumUCbox, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  wxStaticText* itemStaticText107 = new wxStaticText( itemPanel91, wxID_STATIC, _("(At least "), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumUCbox->Add(itemStaticText107, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpUCSpin = new wxSpinCtrl( itemPanel91, ID_SPINCTRL6, _T("0"), wxDefaultPosition, wxSize(itemPanel91->ConvertDialogToPixels(wxSize(20, -1)).x, -1), wxSP_ARROW_KEYS, 0, 100, 0 );
  m_pwNumUCbox->Add(m_pwpUCSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText109 = new wxStaticText( itemPanel91, wxID_STATIC, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumUCbox->Add(itemStaticText109, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpUseDigitsCtrl = new wxCheckBox( itemPanel91, ID_CHECKBOX5, _("Use digits"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpUseDigitsCtrl->SetValue(false);
  m_pwMinsGSzr->Add(m_pwpUseDigitsCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwNumDigbox = new wxBoxSizer(wxHORIZONTAL);
  m_pwMinsGSzr->Add(m_pwNumDigbox, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  wxStaticText* itemStaticText112 = new wxStaticText( itemPanel91, wxID_STATIC, _("(At least "), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumDigbox->Add(itemStaticText112, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpDigSpin = new wxSpinCtrl( itemPanel91, ID_SPINCTRL7, _T("0"), wxDefaultPosition, wxSize(itemPanel91->ConvertDialogToPixels(wxSize(20, -1)).x, -1), wxSP_ARROW_KEYS, 0, 100, 0 );
  m_pwNumDigbox->Add(m_pwpDigSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText114 = new wxStaticText( itemPanel91, wxID_STATIC, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumDigbox->Add(itemStaticText114, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpSymCtrl = new wxCheckBox( itemPanel91, ID_CHECKBOX6, _("Use symbols (i.e., ., %, $, etc.)"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpSymCtrl->SetValue(false);
  m_pwMinsGSzr->Add(m_pwpSymCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwNumSymbox = new wxBoxSizer(wxHORIZONTAL);
  m_pwMinsGSzr->Add(m_pwNumSymbox, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  wxStaticText* itemStaticText117 = new wxStaticText( itemPanel91, wxID_STATIC, _("(At least "), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumSymbox->Add(itemStaticText117, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpSymSpin = new wxSpinCtrl( itemPanel91, ID_SPINCTRL8, _T("0"), wxDefaultPosition, wxSize(itemPanel91->ConvertDialogToPixels(wxSize(20, -1)).x, -1), wxSP_ARROW_KEYS, 0, 100, 0 );
  m_pwNumSymbox->Add(m_pwpSymSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText119 = new wxStaticText( itemPanel91, wxID_STATIC, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumSymbox->Add(itemStaticText119, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpEasyCtrl = new wxCheckBox( itemPanel91, ID_CHECKBOX7, _("Use only easy-to-read characters\n(i.e., no 'l', '1', etc.)"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpEasyCtrl->SetValue(false);
  m_pwMinsGSzr->Add(m_pwpEasyCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwMinsGSzr->Add(10, 10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwpPronounceCtrl = new wxCheckBox( itemPanel91, ID_CHECKBOX8, _("Generate pronounceable passwords"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpPronounceCtrl->SetValue(false);
  m_pwMinsGSzr->Add(m_pwpPronounceCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwMinsGSzr->Add(10, 10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText124 = new wxStaticText( itemPanel91, wxID_STATIC, _("Or"), wxDefaultPosition, wxDefaultSize, 0 );
  itemStaticBoxSizer92->Add(itemStaticText124, 0, wxALIGN_LEFT|wxALL, 5);

  m_pwpHexCtrl = new wxCheckBox( itemPanel91, ID_CHECKBOX9, _("Use hexadecimal digits only (0-9, a-f)"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpHexCtrl->SetValue(false);
  itemStaticBoxSizer92->Add(m_pwpHexCtrl, 0, wxALIGN_LEFT|wxALL, 5);

  wxButton* itemButton126 = new wxButton( itemPanel91, ID_BUTTON7, _("Reset to Database Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
  itemStaticBoxSizer92->Add(itemButton126, 0, wxALIGN_RIGHT|wxALL, 5);

  GetBookCtrl()->AddPage(itemPanel91, _("Password Policy"));

  // Set validators
  itemTextCtrl10->SetValidator( wxGenericValidator(& m_title) );
  itemTextCtrl13->SetValidator( wxGenericValidator(& m_user) );
  itemTextCtrl27->SetValidator( wxGenericValidator(& m_url) );
  itemTextCtrl33->SetValidator( wxGenericValidator(& m_notes) );
  itemTextCtrl38->SetValidator( wxGenericValidator(& m_autotype) );
  itemTextCtrl40->SetValidator( wxGenericValidator(& m_runcmd) );
  itemCheckBox43->SetValidator( wxGenericValidator(& m_useDefaultDCA) );
  itemCheckBox47->SetValidator( wxGenericValidator(& m_keepPWHist) );
  m_MaxPWHistCtrl->SetValidator( wxGenericValidator(& m_maxPWHist) );
  itemStaticText61->SetValidator( wxGenericValidator(& m_XTime) );
  m_ExpTimeCtrl->SetValidator( wxGenericValidator(& m_XTimeInt) );
  m_RecurringCtrl->SetValidator( wxGenericValidator(& m_Recurring) );
  itemStaticText76->SetValidator( wxGenericValidator(& m_CurXTime) );
  itemStaticText84->SetValidator( wxGenericValidator(& m_CTime) );
  itemStaticText86->SetValidator( wxGenericValidator(& m_PMTime) );
  itemStaticText88->SetValidator( wxGenericValidator(& m_ATime) );
  itemStaticText90->SetValidator( wxGenericValidator(& m_RMTime) );
////@end AddEditPropSheet content construction
  m_PWHgrid->SetColLabelValue(0, _("Set Date/Time"));
  m_PWHgrid->SetColLabelValue(1, _("Password"));
}


/*!
 * Should we show tooltips?
 */

bool AddEditPropSheet::ShowToolTips()
{
  return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap AddEditPropSheet::GetBitmapResource( const wxString& name )
{
  // Bitmap retrieval
////@begin AddEditPropSheet bitmap retrieval
  wxUnusedVar(name);
  return wxNullBitmap;
////@end AddEditPropSheet bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon AddEditPropSheet::GetIconResource( const wxString& name )
{
  // Icon retrieval
////@begin AddEditPropSheet icon retrieval
  wxUnusedVar(name);
  return wxNullIcon;
////@end AddEditPropSheet icon retrieval
}

static void EnableSizerChildren(wxSizer *sizer, bool enable)
{
  wxSizerItemList items = sizer->GetChildren();
  wxSizerItemList::iterator iter;
  for (iter = items.begin(); iter != items.end(); iter++) {
    wxWindow *childW = (*iter)->GetWindow();
    if (childW != NULL)
      childW->Enable(enable);
    else { // if another sizer, recurse!
      wxSizer *childS = (*iter)->GetSizer();
      if (childS != NULL)
        EnableSizerChildren(childS, enable);
    }
  }
}

void AddEditPropSheet::UpdatePWPolicyControls(bool useDefault)
{
  m_pwpLenCtrl->Enable(!useDefault);
  EnableSizerChildren(m_pwMinsGSzr, !useDefault);
  m_pwpHexCtrl->Enable(!useDefault);
  if (!useDefault) { // policy override - get values
    m_item.GetPWPolicy(m_PWP);
    m_pwpLenCtrl->SetValue(m_PWP.length);
    m_pwpUseLowerCtrl->SetValue(m_PWP.flags & PWSprefs::PWPolicyUseLowercase);
    m_pwpLCSpin->SetValue(m_PWP.lowerminlength);
    m_pwpUseUpperCtrl->SetValue(m_PWP.flags & PWSprefs::PWPolicyUseUppercase);
    m_pwpUCSpin->SetValue(m_PWP.upperminlength);
    m_pwpUseDigitsCtrl->SetValue(m_PWP.flags & PWSprefs::PWPolicyUseDigits);
    m_pwpDigSpin->SetValue(m_PWP.digitminlength);
    m_pwpSymCtrl->SetValue(m_PWP.flags & PWSprefs::PWPolicyUseSymbols);
    m_pwpSymSpin->SetValue(m_PWP.symbolminlength);
    m_pwpEasyCtrl->SetValue(m_PWP.flags & PWSprefs::PWPolicyUseEasyVision);
    m_pwpPronounceCtrl->SetValue(m_PWP.flags & PWSprefs::PWPolicyMakePronounceable);
    m_pwpHexCtrl->SetValue(m_PWP.flags & PWSprefs::PWPolicyUseHexDigits);
  }
}

void AddEditPropSheet::ItemFieldsToPropSheet()
{
  // Populate the combo box
  std::vector<stringT> aryGroups;
  m_core.GetUniqueGroups(aryGroups);
  for (size_t igrp = 0; igrp < aryGroups.size(); igrp++) {
    m_groupCtrl->Append(aryGroups[igrp].c_str());
  }
  // select relevant group
  const StringX group = m_item.GetGroup();
  if (!group.empty())
    for (size_t igrp = 0; igrp < aryGroups.size(); igrp++)
      if (group == aryGroups[igrp].c_str()) {
        m_groupCtrl->SetSelection(igrp);
        break;
      }
  
  m_title = m_item.GetTitle().c_str();
  m_user = m_item.GetUser().c_str();
  m_url = m_item.GetURL().c_str();
  m_password = m_item.GetPassword();
  PWSprefs *prefs = PWSprefs::GetInstance();
  if (prefs->GetPref(PWSprefs::ShowPWDefault)) {
    ShowPassword();
  } else {
    HidePassword();
  }

  m_PasswordCtrl->ChangeValue(m_password.c_str());
  // Enable Go button iff m_url isn't empty
  wxWindow *goBtn = FindWindow(ID_GO_BTN);
  goBtn->Enable(!m_url.empty());
  m_notes = m_item.GetNotes().c_str();
  m_autotype = m_item.GetAutoType().c_str();
  m_runcmd = m_item.GetRunCommand().c_str();

  // double-click action:
  short iDCA;
  m_item.GetDCA(iDCA);
  m_useDefaultDCA = (iDCA < PWSprefs::minDCA || iDCA > PWSprefs::maxDCA);
  m_DCAcomboBox->Enable(!m_useDefaultDCA);
  if (m_useDefaultDCA) {
    m_DCA = short(PWSprefs::GetInstance()->
                  GetPref(PWSprefs::DoubleClickAction));
  } else {
    m_DCA = iDCA;
  }
  for (size_t i = 0; i < sizeof(dcaMapping)/sizeof(dcaMapping[0]); i++)
    if (m_DCA == dcaMapping[i].pv) {
      m_DCAcomboBox->SetValue(dcaMapping[i].name);
      break;
    }
  // History: If we're adding, use preferences, otherwise,
  // get values from m_item
  if (m_type == ADD) {
    // Get history preferences
    PWSprefs *prefs = PWSprefs::GetInstance();
    m_keepPWHist = prefs->GetPref(PWSprefs::SavePasswordHistory);
    m_maxPWHist = prefs->GetPref(PWSprefs::NumPWHistoryDefault);
  } else { // EDIT or VIEW
    PWHistList pwhl;
    size_t pwh_max, num_err;

    const StringX pwh_str = m_item.GetPWHistory();
    if (!pwh_str.empty()) {
      m_keepPWHist = CreatePWHistoryList(pwh_str,
                                         pwh_max, num_err,
                                         pwhl, TMC_LOCALE);
      m_maxPWHist = int(pwh_max);
      int row = 0;
      for (PWHistList::iterator iter = pwhl.begin(); iter != pwhl.end();
           ++iter) {
        m_PWHgrid->SetCellValue(row, 0, iter->changedate.c_str());
        m_PWHgrid->SetCellValue(row, 1, iter->password.c_str());
        row++;
      }
    } else { // empty history string
      // Get history preferences
      PWSprefs *prefs = PWSprefs::GetInstance();
      m_keepPWHist = prefs->GetPref(PWSprefs::SavePasswordHistory);
      m_maxPWHist = prefs->GetPref(PWSprefs::NumPWHistoryDefault);
    }
  } // m_type

  // Password Expiration
  m_XTime = m_CurXTime = m_item.GetXTimeL().c_str();
  if (m_XTime.empty())
    m_XTime = m_CurXTime = _("Never");
  m_item.GetXTime(m_tttXTime);

  m_item.GetXTimeInt(m_XTimeInt);

  if (m_XTimeInt == 0) { // expiration specified as date
    m_OnRB->SetValue(true);
    m_InRB->SetValue(false);
    m_ExpTimeCtrl->Enable(false);
    m_Recurring = false;
  } else { // exp. specified as interval
    m_OnRB->SetValue(false);
    m_InRB->SetValue(true);
    m_ExpDate->Enable(false);
    m_ExpTimeH->Enable(false);
    m_ExpTimeM->Enable(false);
    m_ExpTimeCtrl->SetValue(m_XTimeInt);
    m_Recurring = true;
  }
  m_RecurringCtrl->Enable(m_Recurring);

  // Modification times
  m_CTime = m_item.GetCTimeL().c_str();
  m_PMTime = m_item.GetPMTimeL().c_str();
  m_ATime = m_item.GetATimeL().c_str();
  m_RMTime = m_item.GetRMTimeL().c_str();

  // Password policy
  bool defPwPolicy = m_item.GetPWPolicy().empty();
  m_defPWPRB->SetValue(defPwPolicy);
  m_ourPWPRB->SetValue(!defPwPolicy);
  UpdatePWPolicyControls(defPwPolicy);
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GO_BTN
 */

void AddEditPropSheet::OnGoButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GO_BTN in AddEditPropSheet.
  // Before editing this code, remove the block markers.
  wxMessageBox(_("'Go' placeholder"));
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GO_BTN in AddEditPropSheet. 
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON3
 */

void AddEditPropSheet::OnGenerateButtonClick( wxCommandEvent& event )
{
  PWPolicy pwp;
  m_item.GetPWPolicy(pwp);
  StringX password = pwp.MakeRandomPassword();


  PWSclip::SetData(password);
  m_password = password.c_str();
  m_PasswordCtrl->ChangeValue(m_password.c_str());
  if (m_isPWHidden) {
    m_Password1HiddenCtrl->ChangeValue(m_password.c_str());
    m_Password2Ctrl->ChangeValue(m_password.c_str());
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON2
 */

void AddEditPropSheet::OnShowHideClick( wxCommandEvent& event )
{
  if (m_isPWHidden) {
    ShowPassword();
  } else {
    m_password = m_PasswordCtrl->GetValue(); // save visible password
    HidePassword();
  }
}

void AddEditPropSheet::ShowPassword()
{
  m_isPWHidden = false;
  m_ShowHideCtrl->SetLabel(_("&Hide"));

  m_PasswordCtrl->ChangeValue(m_password.c_str());
  // Following kludge since wxTE_PASSWORD style is immutable
  m_PasswordCtrl->Show();
  m_Password1HiddenCtrl->Hide();
  // Disable confirmation Ctrl, as the user can see the password entered
  m_Password2Ctrl->ChangeValue(_(""));
  m_Password2Ctrl->Enable(false);
}

void AddEditPropSheet::HidePassword()
{
  m_isPWHidden = true;
  m_ShowHideCtrl->SetLabel(_("&Show"));

  // Following kludge since wxTE_PASSWORD style is immutable
  m_PasswordCtrl->Hide();
  m_Password1HiddenCtrl->ChangeValue(m_password.c_str());
  m_Password1HiddenCtrl->Show();
  // Need verification as the user can not see the password entered
  m_Password2Ctrl->ChangeValue(m_password.c_str());
  m_Password2Ctrl->Enable(true);
}

void AddEditPropSheet::OnOk(wxCommandEvent& event)
{
  if (Validate() && TransferDataFromWindow()) {
    time_t t;
    const wxString group = m_groupCtrl->GetValue();
    StringX password;

    if (m_PasswordCtrl->IsShown()) {
      password = m_PasswordCtrl->GetValue().c_str();
    } else { // hidden passwords - compare both values
      password = m_Password1HiddenCtrl->GetValue().c_str();
      const StringX p2 = m_Password2Ctrl->GetValue().c_str();
      if (password != p2) {
        wxMessageDialog msg(this, _("Passwords do not match"), _("Error"),
                            wxOK|wxICON_ERROR);
        msg.ShowModal();
        return;
      }
    }

    switch (m_type) {
    case EDIT: {
      bool bIsModified, bIsPSWDModified;
      short lastDCA;
      m_item.GetDCA(lastDCA);
      if (m_useDefaultDCA) { // get value from global pref
        m_DCA = short(PWSprefs::GetInstance()->
                      GetPref(PWSprefs::DoubleClickAction));
      } else { // get value from field
        const wxString cv = m_DCAcomboBox->GetValue();      
        for (size_t i = 0; i < sizeof(dcaMapping)/sizeof(dcaMapping[0]); i++)
          if (cv == dcaMapping[i].name) {
            m_DCA = dcaMapping[i].pv;
            break;
          }
      }
      
      // Check if modified
      int lastXTimeInt;
      m_item.GetXTimeInt(lastXTimeInt);
      time_t lastXtime;
      m_item.GetXTime(lastXtime);
      bool isPWPDefault = m_defPWPRB->GetValue();
      PWPolicy oldPWP;
      m_item.GetPWPolicy(oldPWP);

      bIsModified = (group        != m_item.GetGroup().c_str()       ||
                     m_title      != m_item.GetTitle().c_str()       ||
                     m_user       != m_item.GetUser().c_str()        ||
                     m_notes      != m_item.GetNotes().c_str()       ||
                     m_url        != m_item.GetURL().c_str()         ||
                     m_autotype   != m_item.GetAutoType().c_str()    ||
                     m_runcmd     != m_item.GetRunCommand().c_str()  ||
                     m_DCA        != lastDCA                         ||
                     m_PWHistory  != m_item.GetPWHistory().c_str()   ||
                     m_tttXTime   != lastXtime                       ||
                     m_XTimeInt   != lastXTimeInt                    ||
                     m_origPWPdefault != isPWPDefault                ||
                     (!isPWPDefault && (m_PWP != oldPWP)));

      bIsPSWDModified = (password != m_item.GetPassword());

      if (bIsModified) {
        // Just modify all - even though only 1 may have actually been modified
        m_item.SetGroup(group.c_str());
        m_item.SetTitle(m_title.c_str());
        m_item.SetUser(m_user.empty() ?
                       m_core.GetDefUsername().c_str() : m_user.c_str());
        m_item.SetNotes(m_notes.c_str());
        m_item.SetURL(m_url.c_str());
        m_item.SetAutoType(m_autotype.c_str());
        m_item.SetRunCommand(m_runcmd.c_str());
        m_item.SetPWHistory(m_PWHistory.c_str());
        if (m_defPWPRB->GetValue())
          m_item.SetPWPolicy(_T(""));
        else {
          m_PWP.length = m_pwpLenCtrl->GetValue();
          m_PWP.flags = 0;
          m_PWP.lowerminlength = m_PWP.upperminlength =
            m_PWP.digitminlength = m_PWP.symbolminlength = 0;
          if (m_pwpUseLowerCtrl->GetValue()) {
            m_PWP.flags |= PWSprefs::PWPolicyUseLowercase;
            m_PWP.lowerminlength = m_pwpLCSpin->GetValue();
          }
          if (m_pwpUseUpperCtrl->GetValue()) {
            m_PWP.flags |= PWSprefs::PWPolicyUseUppercase;
            m_PWP.upperminlength = m_pwpUCSpin->GetValue();
          }
          if (m_pwpUseDigitsCtrl->GetValue()) {
            m_PWP.flags |= PWSprefs::PWPolicyUseDigits;
            m_PWP.digitminlength = m_pwpDigSpin->GetValue();
          }
          if (m_pwpSymCtrl->GetValue()) {
            m_PWP.flags |= PWSprefs::PWPolicyUseSymbols;
            m_PWP.symbolminlength = m_pwpSymSpin->GetValue();
          }
          if (m_pwpEasyCtrl->GetValue())
            m_PWP.flags |= PWSprefs::PWPolicyUseEasyVision;
          if (m_pwpPronounceCtrl->GetValue())
            m_PWP.flags |= PWSprefs::PWPolicyMakePronounceable;
          if (m_pwpHexCtrl->GetValue())
            m_PWP.flags |= PWSprefs::PWPolicyUseHexDigits;
          m_item.SetPWPolicy(m_PWP);
        }
        m_item.SetDCA(m_useDefaultDCA ? -1 : m_DCA);
      } // bIsModified

      time(&t);
      if (bIsPSWDModified) {
        m_item.UpdatePassword(password);
        m_item.SetPMTime(t);
      }
      if (bIsModified || bIsPSWDModified)
        m_item.SetRMTime(t);
      if (m_tttXTime != lastXtime)
        m_item.SetXTime(m_tttXTime);
      if (m_XTimeInt != lastXTimeInt)
        m_item.SetXTimeInt(m_XTimeInt);
      // All fields in m_item now reflect user's edits
      // Let's update the core's data
      uuid_array_t uuid;
      m_item.GetUUID(uuid);
      ItemListIter listpos = m_core.Find(uuid);
      ASSERT(listpos != m_core.GetEntryEndIter());
      m_core.RemoveEntryAt(listpos);
      m_core.AddEntry(m_item);
      // refresh tree view
      m_tree->UpdateItem(m_item);
      m_grid->UpdateItem(m_item);
    }
      break;

    case ADD:
      m_item.SetGroup(group.c_str());
      m_item.SetTitle(m_title.c_str());
      m_item.SetUser(m_user.empty() ?
                     m_core.GetDefUsername().c_str() : m_user.c_str());
      m_item.SetNotes(m_notes.c_str());
      m_item.SetURL(m_url.c_str());
      m_item.SetPassword(password);
      m_item.SetAutoType(m_autotype.c_str());
      m_item.SetRunCommand(m_runcmd.c_str());
      m_item.SetDCA(m_DCA);
      time(&t);
      m_item.SetCTime(t);
      if (m_keepPWHist)
        m_item.SetPWHistory(MakePWHistoryHeader(TRUE, m_maxPWHist, 0));

      m_item.SetXTime(m_tttXTime);
      if (m_XTimeInt > 0 && m_XTimeInt <= 3650)
        m_item.SetXTimeInt(m_XTimeInt);

#ifdef NOTYET
if (m_AEMD.ibasedata > 0) {
        // Password in alias format AND base entry exists
        // No need to check if base is an alias as already done in
        // call to PWScore::GetBaseEntry
        uuid_array_t alias_uuid;
        m_item.GetUUID(alias_uuid);
        m_AEMD.pcore->AddDependentEntry(m_AEMD.base_uuid, alias_uuid, CItemData::ET_ALIAS);
        m_item.SetPassword(_T("[Alias]"));
        m_item.SetAlias();
        ItemListIter iter = m_AEMD.pcore->Find(m_AEMD.base_uuid);
        if (iter != m_AEMD.pDbx->End()) {
          const CItemData &cibase = iter->second;
          DisplayInfo *di = (DisplayInfo *)cibase.GetDisplayInfo();
          int nImage = m_AEMD.pDbx->GetEntryImage(cibase);
          m_AEMD.pDbx->SetEntryImage(di->list_index, nImage, true);
          m_AEMD.pDbx->SetEntryImage(di->tree_item, nImage, true);
        }
      } else {
        m_item.SetPassword(m_AEMD.realpassword);
        m_item.SetNormal();
      }

      if (m_item.IsAlias()) {
        m_item.SetXTime((time_t)0);
        m_item.SetPWPolicy(_T(""));
      } else {
        m_item.SetXTime(m_AEMD.tttXTime);
        if (m_AEMD.ipolicy == DEFAULT_POLICY)
          m_item.SetPWPolicy(_T(""));
        else
          m_item.SetPWPolicy(m_AEMD.pwp);
      }
#endif
      break;
    case VIEW:
      // No Update
      break;
    default:
      ASSERT(0);
      break;
    }
  }
  EndModal(wxID_OK);
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX1
 */

void AddEditPropSheet::OnKeepHistoryClick(wxCommandEvent &)
{
   if (Validate() && TransferDataFromWindow()) {
     // disable spinbox if checkbox is false
     m_MaxPWHistCtrl->Enable(m_keepPWHist);
   }
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX
 */

void AddEditPropSheet::OnOverrideDCAClick( wxCommandEvent& event )
{
  if (Validate() && TransferDataFromWindow()) {
    m_DCAcomboBox->Enable(!m_useDefaultDCA);
    if (m_useDefaultDCA) { // restore default
      short dca = short(PWSprefs::GetInstance()->
                        GetPref(PWSprefs::DoubleClickAction));
      for (size_t i = 0; i < sizeof(dcaMapping)/sizeof(dcaMapping[0]); i++)
        if (dca == dcaMapping[i].pv) {
          m_DCAcomboBox->SetValue(dcaMapping[i].name);
          break;
        }
      m_DCA = -1; // 'use default' value
    }
  }
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON5
 */

void AddEditPropSheet::OnSetXTime( wxCommandEvent& event )
{
  if (Validate() && TransferDataFromWindow()) {
    wxDateTime xdt = m_ExpDate->GetValue();
    if (m_OnRB->GetValue()) { // absolute exp time
      xdt = m_ExpDate->GetValue();
      xdt.SetHour(m_ExpTimeH->GetValue());
      xdt.SetMinute(m_ExpTimeM->GetValue());
      m_XTimeInt = 0;
      m_XTime = xdt.FormatDate();
    } else { // relative, possibly recurring
      // If it's a non-recurring interval, just set XTime to
      // now + interval, XTimeInt should be stored as zero
      // (one-shot semantics)
      // Otherwise, XTime += interval, keep XTimeInt
      if (!m_Recurring) {
        xdt = wxDateTime::Now();
        xdt += wxDateSpan(0, 0, 0, m_XTimeInt);
        m_XTimeInt = 0;
        m_XTime = xdt.FormatDate();
      } else { // recurring exp. interval
        xdt = m_ExpDate->GetValue();
        xdt.SetHour(m_ExpTimeH->GetValue());
        xdt.SetMinute(m_ExpTimeM->GetValue());
        xdt += wxDateSpan(0, 0, 0, m_XTimeInt);
        m_XTime = xdt.FormatDate();
        wxString rstr;
        rstr.Printf(_(" (every %d days)"), m_XTimeInt);
        m_XTime += rstr;
      } // recurring
    } // relative
    m_tttXTime = xdt.GetTicks();
    Validate(); TransferDataToWindow();
  } // Validated & transferred from controls
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON6
 */

void AddEditPropSheet::OnClearXTime( wxCommandEvent& event )
{
  m_XTime = _("Never");
  m_CurXTime = _("");
  m_tttXTime = time_t(0);
  m_XTimeInt = 0;
  m_Recurring = false;
  TransferDataToWindow();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON
 */

void AddEditPropSheet::OnRadiobuttonSelected( wxCommandEvent& event )
{
  bool On = (event.GetEventObject() == m_OnRB);
  m_ExpDate->Enable(On);
  m_ExpTimeH->Enable(On);
  m_ExpTimeM->Enable(On);
  m_ExpTimeCtrl->Enable(!On);
  m_RecurringCtrl->Enable(!On);

}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON2
 */

void AddEditPropSheet::OnPWPRBSelected( wxCommandEvent& event )
{
  UpdatePWPolicyControls(event.GetEventObject() == m_defPWPRB);
}

void AddEditPropSheet::ShowPWPSpinners(bool show)
{
  m_pwMinsGSzr->Show(m_pwNumLCbox,  show, true);
  m_pwMinsGSzr->Show(m_pwNumUCbox,  show, true);
  m_pwMinsGSzr->Show(m_pwNumDigbox, show, true);
  m_pwMinsGSzr->Show(m_pwNumSymbox, show, true);
  m_pwMinsGSzr->Layout();
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX8
 */

void AddEditPropSheet::OnPronouceableCBClick( wxCommandEvent& event )
{
 if (Validate() && TransferDataFromWindow()) {
   bool wantsPronouceable = m_pwpPronounceCtrl->GetValue();
   ShowPWPSpinners(!wantsPronouceable);
 }
}


/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX7
 */

void AddEditPropSheet::OnEZreadCBClick( wxCommandEvent& event )
{
 if (Validate() && TransferDataFromWindow()) {
   bool wantsEZread = m_pwpEasyCtrl->GetValue();
   ShowPWPSpinners(!wantsEZread);
 }
}

void AddEditPropSheet::EnableNonHexCBs(bool enable)
{
  EnableSizerChildren(m_pwMinsGSzr, enable);
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX9
 */

void AddEditPropSheet::OnUseHexCBClick( wxCommandEvent& event )
{
 if (Validate() && TransferDataFromWindow()) {
   bool useHex = m_pwpHexCtrl->GetValue();
   EnableNonHexCBs(!useHex);
 }
}

