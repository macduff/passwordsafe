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
////@end AddEditPropSheet member initialisation
}


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
  itemBoxSizer3->Add(itemFlexGridSizer5, 0, wxGROW|wxALL, 5);
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

  wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer5->Add(itemBoxSizer17, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  itemBoxSizer17->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_ShowHideCtrl = new wxButton( itemPanel2, ID_BUTTON2, _("&Hide"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer17->Add(m_ShowHideCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  itemBoxSizer17->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton21 = new wxButton( itemPanel2, ID_BUTTON3, _("&Generate"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer17->Add(itemButton21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText22 = new wxStaticText( itemPanel2, wxID_STATIC, _("Confirm:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemStaticText22, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_Password2Ctrl = new wxTextCtrl( itemPanel2, ID_TEXTCTRL3, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
  itemFlexGridSizer5->Add(m_Password2Ctrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemFlexGridSizer5->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText25 = new wxStaticText( itemPanel2, wxID_STATIC, _("URL:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemStaticText25, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl26 = new wxTextCtrl( itemPanel2, ID_TEXTCTRL4, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer5->Add(itemTextCtrl26, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer5->Add(itemBoxSizer27, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  itemBoxSizer27->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton29 = new wxButton( itemPanel2, ID_GO_BTN, _("Go"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer27->Add(itemButton29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxBoxSizer* itemBoxSizer30 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer3->Add(itemBoxSizer30, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText31 = new wxStaticText( itemPanel2, wxID_STATIC, _("Notes:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer30->Add(itemStaticText31, 1, wxALIGN_TOP|wxALL, 5);

  wxTextCtrl* itemTextCtrl32 = new wxTextCtrl( itemPanel2, ID_TEXTCTRL7, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
  itemBoxSizer30->Add(itemTextCtrl32, 5, wxALIGN_CENTER_VERTICAL|wxALL, 3);

  GetBookCtrl()->AddPage(itemPanel2, _("Basic"));

  wxPanel* itemPanel33 = new wxPanel( GetBookCtrl(), ID_PANEL_ADDITIONAL, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
  wxBoxSizer* itemBoxSizer34 = new wxBoxSizer(wxVERTICAL);
  itemPanel33->SetSizer(itemBoxSizer34);

  wxFlexGridSizer* itemFlexGridSizer35 = new wxFlexGridSizer(0, 2, 0, 0);
  itemBoxSizer34->Add(itemFlexGridSizer35, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText36 = new wxStaticText( itemPanel33, wxID_STATIC, _("Autotype:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer35->Add(itemStaticText36, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl37 = new wxTextCtrl( itemPanel33, ID_TEXTCTRL6, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer35->Add(itemTextCtrl37, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText38 = new wxStaticText( itemPanel33, wxID_STATIC, _("Run Cmd:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer35->Add(itemStaticText38, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl39 = new wxTextCtrl( itemPanel33, ID_TEXTCTRL8, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer35->Add(itemTextCtrl39, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText40 = new wxStaticText( itemPanel33, wxID_STATIC, _("Double-Click\nAction:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer35->Add(itemStaticText40, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer41 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer35->Add(itemBoxSizer41, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);
  wxCheckBox* itemCheckBox42 = new wxCheckBox( itemPanel33, ID_CHECKBOX, _("Use Default"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox42->SetValue(false);
  itemBoxSizer41->Add(itemCheckBox42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

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
  m_DCAcomboBox = new wxComboBox( itemPanel33, ID_COMBOBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_DCAcomboBoxStrings, wxCB_READONLY );
  itemBoxSizer41->Add(m_DCAcomboBox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticBox* itemStaticBoxSizer44Static = new wxStaticBox(itemPanel33, wxID_ANY, _("Password History"));
  wxStaticBoxSizer* itemStaticBoxSizer44 = new wxStaticBoxSizer(itemStaticBoxSizer44Static, wxVERTICAL);
  itemBoxSizer34->Add(itemStaticBoxSizer44, 0, wxGROW|wxALL, 5);
  wxBoxSizer* itemBoxSizer45 = new wxBoxSizer(wxHORIZONTAL);
  itemStaticBoxSizer44->Add(itemBoxSizer45, 0, wxGROW|wxALL, 5);
  wxCheckBox* itemCheckBox46 = new wxCheckBox( itemPanel33, ID_CHECKBOX1, _("Keep"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox46->SetValue(false);
  itemBoxSizer45->Add(itemCheckBox46, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_MaxPWHistCtrl = new wxSpinCtrl( itemPanel33, ID_SPINCTRL, _T("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 0 );
  itemBoxSizer45->Add(m_MaxPWHistCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText48 = new wxStaticText( itemPanel33, wxID_STATIC, _("last passwords"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer45->Add(itemStaticText48, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_PWHgrid = new wxGrid( itemPanel33, ID_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
  m_PWHgrid->SetDefaultColSize(150);
  m_PWHgrid->SetDefaultRowSize(25);
  m_PWHgrid->SetColLabelSize(25);
  m_PWHgrid->SetRowLabelSize(0);
  m_PWHgrid->CreateGrid(5, 2, wxGrid::wxGridSelectRows);
  itemStaticBoxSizer44->Add(m_PWHgrid, 0, wxGROW|wxALL, 5);

  wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxHORIZONTAL);
  itemStaticBoxSizer44->Add(itemBoxSizer50, 0, wxGROW|wxALL, 5);
  wxButton* itemButton51 = new wxButton( itemPanel33, ID_BUTTON1, _("Clear History"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer50->Add(itemButton51, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemBoxSizer50->Add(10, 10, 10, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton53 = new wxButton( itemPanel33, ID_BUTTON4, _("Copy All"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer50->Add(itemButton53, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  GetBookCtrl()->AddPage(itemPanel33, _("Additional"));

  wxPanel* itemPanel54 = new wxPanel( GetBookCtrl(), ID_PANEL_DTIME, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
  wxBoxSizer* itemBoxSizer55 = new wxBoxSizer(wxVERTICAL);
  itemPanel54->SetSizer(itemBoxSizer55);

  wxStaticBox* itemStaticBoxSizer56Static = new wxStaticBox(itemPanel54, wxID_ANY, _("Password Expiry"));
  wxStaticBoxSizer* itemStaticBoxSizer56 = new wxStaticBoxSizer(itemStaticBoxSizer56Static, wxVERTICAL);
  itemBoxSizer55->Add(itemStaticBoxSizer56, 0, wxGROW|wxALL, 5);
  wxBoxSizer* itemBoxSizer57 = new wxBoxSizer(wxVERTICAL);
  itemStaticBoxSizer56->Add(itemBoxSizer57, 0, wxGROW|wxALL, 5);
  wxBoxSizer* itemBoxSizer58 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer57->Add(itemBoxSizer58, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText59 = new wxStaticText( itemPanel54, wxID_STATIC, _("Password Expires on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer58->Add(itemStaticText59, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText60 = new wxStaticText( itemPanel54, wxID_STATIC, _("Whenever"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer58->Add(itemStaticText60, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxFlexGridSizer* itemFlexGridSizer61 = new wxFlexGridSizer(0, 3, 0, 0);
  itemBoxSizer57->Add(itemFlexGridSizer61, 0, wxGROW|wxALL, 5);
  m_OnRB = new wxRadioButton( itemPanel54, ID_RADIOBUTTON, _("On"), wxDefaultPosition, wxDefaultSize, 0 );
  m_OnRB->SetValue(false);
  itemFlexGridSizer61->Add(m_OnRB, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_ExpDate = new wxDatePickerCtrl( itemPanel54, ID_DATECTRL, wxDateTime(10, (wxDateTime::Month) 7, 2009), wxDefaultPosition, wxDefaultSize, wxDP_DEFAULT );
  itemFlexGridSizer61->Add(m_ExpDate, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer61->Add(itemBoxSizer64, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);
  m_ExpTimeH = new wxSpinCtrl( itemPanel54, ID_SPINCTRL1, _T("12"), wxDefaultPosition, wxSize(itemPanel54->ConvertDialogToPixels(wxSize(20, -1)).x, -1), wxSP_ARROW_KEYS, 0, 23, 12 );
  itemBoxSizer64->Add(m_ExpTimeH, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText66 = new wxStaticText( itemPanel54, wxID_STATIC, _(":"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer64->Add(itemStaticText66, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_ExpTimeM = new wxSpinCtrl( itemPanel54, ID_SPINCTRL4, _T("0"), wxDefaultPosition, wxSize(itemPanel54->ConvertDialogToPixels(wxSize(20, -1)).x, -1), wxSP_ARROW_KEYS, 0, 59, 0 );
  itemBoxSizer64->Add(m_ExpTimeM, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_InRB = new wxRadioButton( itemPanel54, ID_RADIOBUTTON1, _("In"), wxDefaultPosition, wxDefaultSize, 0 );
  m_InRB->SetValue(false);
  itemFlexGridSizer61->Add(m_InRB, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer69 = new wxBoxSizer(wxHORIZONTAL);
  itemFlexGridSizer61->Add(itemBoxSizer69, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  m_ExpTimeCtrl = new wxSpinCtrl( itemPanel54, ID_SPINCTRL2, _T("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 3650, 1 );
  itemBoxSizer69->Add(m_ExpTimeCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText71 = new wxStaticText( itemPanel54, wxID_STATIC, _("days"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer69->Add(itemStaticText71, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_RecurringCtrl = new wxCheckBox( itemPanel54, ID_CHECKBOX2, _("Recurring"), wxDefaultPosition, wxDefaultSize, 0 );
  m_RecurringCtrl->SetValue(false);
  itemFlexGridSizer61->Add(m_RecurringCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer73 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer57->Add(itemBoxSizer73, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText74 = new wxStaticText( itemPanel54, wxID_STATIC, _("Current Value:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer73->Add(itemStaticText74, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText75 = new wxStaticText( itemPanel54, wxID_STATIC, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer73->Add(itemStaticText75, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer76 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer57->Add(itemBoxSizer76, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
  wxButton* itemButton77 = new wxButton( itemPanel54, ID_BUTTON5, _("&Set"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer76->Add(itemButton77, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  itemBoxSizer76->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton79 = new wxButton( itemPanel54, ID_BUTTON6, _("&Clear"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer76->Add(itemButton79, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticBox* itemStaticBoxSizer80Static = new wxStaticBox(itemPanel54, wxID_ANY, _("Statistics"));
  wxStaticBoxSizer* itemStaticBoxSizer80 = new wxStaticBoxSizer(itemStaticBoxSizer80Static, wxVERTICAL);
  itemBoxSizer55->Add(itemStaticBoxSizer80, 0, wxGROW|wxALL, 5);
  wxFlexGridSizer* itemFlexGridSizer81 = new wxFlexGridSizer(0, 2, 0, 0);
  itemStaticBoxSizer80->Add(itemFlexGridSizer81, 0, wxALIGN_LEFT|wxALL, 5);
  wxStaticText* itemStaticText82 = new wxStaticText( itemPanel54, wxID_STATIC, _("Created on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer81->Add(itemStaticText82, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText83 = new wxStaticText( itemPanel54, wxID_STATIC, _("10/06/2009 23:19:25"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer81->Add(itemStaticText83, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText84 = new wxStaticText( itemPanel54, wxID_STATIC, _("Password last changed on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer81->Add(itemStaticText84, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText85 = new wxStaticText( itemPanel54, wxID_STATIC, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer81->Add(itemStaticText85, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText86 = new wxStaticText( itemPanel54, wxID_STATIC, _("Last accessed on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer81->Add(itemStaticText86, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText87 = new wxStaticText( itemPanel54, wxID_STATIC, _("N/A"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer81->Add(itemStaticText87, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText88 = new wxStaticText( itemPanel54, wxID_STATIC, _("Any field last changed on:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer81->Add(itemStaticText88, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText89 = new wxStaticText( itemPanel54, wxID_STATIC, _("Static text"), wxDefaultPosition, wxDefaultSize, 0 );
  itemFlexGridSizer81->Add(itemStaticText89, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  GetBookCtrl()->AddPage(itemPanel54, _("Dates and Times"));

  wxPanel* itemPanel90 = new wxPanel( GetBookCtrl(), ID_PANEL_PPOLICY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
  wxStaticBox* itemStaticBoxSizer91Static = new wxStaticBox(itemPanel90, wxID_ANY, _("Random password generation rules"));
  wxStaticBoxSizer* itemStaticBoxSizer91 = new wxStaticBoxSizer(itemStaticBoxSizer91Static, wxVERTICAL);
  itemPanel90->SetSizer(itemStaticBoxSizer91);

  wxRadioButton* itemRadioButton92 = new wxRadioButton( itemPanel90, ID_RADIOBUTTON2, _("Use Database Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
  itemRadioButton92->SetValue(false);
  itemStaticBoxSizer91->Add(itemRadioButton92, 0, wxALIGN_LEFT|wxALL, 5);

  wxRadioButton* itemRadioButton93 = new wxRadioButton( itemPanel90, ID_RADIOBUTTON3, _("Use the policy below:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemRadioButton93->SetValue(false);
  itemStaticBoxSizer91->Add(itemRadioButton93, 0, wxALIGN_LEFT|wxALL, 5);

  wxStaticLine* itemStaticLine94 = new wxStaticLine( itemPanel90, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
  itemStaticBoxSizer91->Add(itemStaticLine94, 0, wxGROW|wxALL, 5);

  wxBoxSizer* itemBoxSizer95 = new wxBoxSizer(wxHORIZONTAL);
  itemStaticBoxSizer91->Add(itemBoxSizer95, 0, wxALIGN_LEFT|wxALL, 5);
  wxStaticText* itemStaticText96 = new wxStaticText( itemPanel90, wxID_STATIC, _("Password length: "), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer95->Add(itemStaticText96, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxSpinCtrl* itemSpinCtrl97 = new wxSpinCtrl( itemPanel90, ID_SPINCTRL3, _T("8"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 4, 100, 8 );
  itemBoxSizer95->Add(itemSpinCtrl97, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxCheckBox* itemCheckBox98 = new wxCheckBox( itemPanel90, ID_CHECKBOX3, _("Use lowercase letters"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox98->SetValue(false);
  itemStaticBoxSizer91->Add(itemCheckBox98, 0, wxALIGN_LEFT|wxALL, 5);

  wxCheckBox* itemCheckBox99 = new wxCheckBox( itemPanel90, ID_CHECKBOX4, _("Use UPPERCASE letters"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox99->SetValue(false);
  itemStaticBoxSizer91->Add(itemCheckBox99, 0, wxALIGN_LEFT|wxALL, 5);

  wxCheckBox* itemCheckBox100 = new wxCheckBox( itemPanel90, ID_CHECKBOX5, _("Use digits"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox100->SetValue(false);
  itemStaticBoxSizer91->Add(itemCheckBox100, 0, wxALIGN_LEFT|wxALL, 5);

  wxCheckBox* itemCheckBox101 = new wxCheckBox( itemPanel90, ID_CHECKBOX6, _("Use symbols (i.e., ., %, $, etc.)"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox101->SetValue(false);
  itemStaticBoxSizer91->Add(itemCheckBox101, 0, wxALIGN_LEFT|wxALL, 5);

  wxCheckBox* itemCheckBox102 = new wxCheckBox( itemPanel90, ID_CHECKBOX7, _("Use only easy-to-read charachters (i.e., no 'l', '1', etc.)"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox102->SetValue(false);
  itemStaticBoxSizer91->Add(itemCheckBox102, 0, wxALIGN_LEFT|wxALL, 5);

  wxCheckBox* itemCheckBox103 = new wxCheckBox( itemPanel90, ID_CHECKBOX8, _("Generate pronounceable passwords"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox103->SetValue(false);
  itemStaticBoxSizer91->Add(itemCheckBox103, 0, wxALIGN_LEFT|wxALL, 5);

  wxStaticText* itemStaticText104 = new wxStaticText( itemPanel90, wxID_STATIC, _("Or"), wxDefaultPosition, wxDefaultSize, 0 );
  itemStaticBoxSizer91->Add(itemStaticText104, 0, wxALIGN_LEFT|wxALL, 5);

  wxCheckBox* itemCheckBox105 = new wxCheckBox( itemPanel90, ID_CHECKBOX9, _("Use hexadecimal digits only (0-9, a-f)"), wxDefaultPosition, wxDefaultSize, 0 );
  itemCheckBox105->SetValue(false);
  itemStaticBoxSizer91->Add(itemCheckBox105, 0, wxALIGN_LEFT|wxALL, 5);

  wxButton* itemButton106 = new wxButton( itemPanel90, ID_BUTTON7, _("Reset to Database Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
  itemStaticBoxSizer91->Add(itemButton106, 0, wxALIGN_RIGHT|wxALL, 5);

  GetBookCtrl()->AddPage(itemPanel90, _("Password Policy"));

  // Set validators
  itemTextCtrl10->SetValidator( wxGenericValidator(& m_title) );
  itemTextCtrl13->SetValidator( wxGenericValidator(& m_user) );
  itemTextCtrl26->SetValidator( wxGenericValidator(& m_url) );
  itemTextCtrl32->SetValidator( wxGenericValidator(& m_notes) );
  itemTextCtrl37->SetValidator( wxGenericValidator(& m_autotype) );
  itemTextCtrl39->SetValidator( wxGenericValidator(& m_runcmd) );
  itemCheckBox42->SetValidator( wxGenericValidator(& m_useDefaultDCA) );
  itemCheckBox46->SetValidator( wxGenericValidator(& m_keepPWHist) );
  m_MaxPWHistCtrl->SetValidator( wxGenericValidator(& m_maxPWHist) );
  itemStaticText60->SetValidator( wxGenericValidator(& m_XTime) );
  m_ExpTimeCtrl->SetValidator( wxGenericValidator(& m_XTimeInt) );
  m_RecurringCtrl->SetValidator( wxGenericValidator(& m_Recurring) );
  itemStaticText75->SetValidator( wxGenericValidator(& m_CurXTime) );
  itemStaticText83->SetValidator( wxGenericValidator(& m_CTime) );
  itemStaticText85->SetValidator( wxGenericValidator(& m_PMTime) );
  itemStaticText87->SetValidator( wxGenericValidator(& m_ATime) );
  itemStaticText89->SetValidator( wxGenericValidator(& m_RMTime) );
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
  m_XTime = m_item.GetXTimeL().c_str();
  if (m_XTime.empty())
    m_XTime = _("Never");
  m_item.GetXTimeInt(m_XTimeInt);

  if (m_XTimeInt == 0) { // expiration specified as date
    m_OnRB->SetValue(true);
    m_InRB->SetValue(false);
    m_ExpTimeCtrl->Enable(false);
    m_RecurringCtrl->Enable(false);
  } else { // exp. specified as interval
    m_OnRB->SetValue(false);
    m_InRB->SetValue(true);
    m_ExpDate->Enable(false);
    m_ExpTimeH->Enable(false);
    m_ExpTimeM->Enable(false);
    m_ExpTimeCtrl->SetValue(m_XTimeInt);
  }

  // Modification times
  m_CTime = m_item.GetCTimeL().c_str();
  m_PMTime = m_item.GetPMTimeL().c_str();
  m_ATime = m_item.GetATimeL().c_str();
  m_RMTime = m_item.GetRMTimeL().c_str();
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
  // XXX Can't change wxTE_PASSWORD style - need to think of something else.
  // Disable confirmation Ctrl, as the user can see the password entered
  m_Password2Ctrl->ChangeValue(_(""));
  m_Password2Ctrl->Enable(false);
}

void AddEditPropSheet::HidePassword()
{
  m_isPWHidden = true;
  m_ShowHideCtrl->SetLabel(_("&Show"));

  // XXX Can't change wxTE_PASSWORD style - need to think of something else.

  // Need verification as the user can not see the password entered
  m_Password2Ctrl->ChangeValue(m_password.c_str());
  m_Password2Ctrl->Enable(true);
}

void AddEditPropSheet::OnOk(wxCommandEvent& event)
{
  if (Validate() && TransferDataFromWindow()) {
    time_t t;
    const wxString group = m_groupCtrl->GetValue();
    const StringX password = m_PasswordCtrl->GetValue().c_str();

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

      bIsModified = (group        != m_item.GetGroup().c_str()       ||
                     m_title      != m_item.GetTitle().c_str()       ||
                     m_user       != m_item.GetUser().c_str()        ||
                     m_notes      != m_item.GetNotes().c_str()       ||
                     m_url        != m_item.GetURL().c_str()         ||
                     m_autotype   != m_item.GetAutoType().c_str()    ||
                     m_runcmd != m_item.GetRunCommand().c_str()      ||
                     m_DCA        != lastDCA                         ||
                     m_PWHistory  != m_item.GetPWHistory().c_str()   ||
                     m_XTime      != m_item.GetXTimeL().c_str()      ||
                     m_XTimeInt   != lastXTimeInt                    ||
#ifdef NOTYET
                     m_AEMD.ipolicy     != m_AEMD.oldipolicy           ||
                     (m_AEMD.ipolicy     == SPECIFIC_POLICY &&
                      m_AEMD.pwp         != m_AEMD.oldpwp)
#else 
                     0
#endif
                     );

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
#ifdef NOTYET
        if (m_AEMD.ipolicy == DEFAULT_POLICY)
          m_item.SetPWPolicy(_T(""));
        else
          m_item.SetPWPolicy(m_AEMD.pwp);
#endif
        m_item.SetDCA(m_useDefaultDCA ? -1 : m_DCA);
      } // bIsModified

      time(&t);
      if (bIsPSWDModified) {
        m_item.UpdatePassword(password);
        m_item.SetPMTime(t);
      }
      if (bIsModified || bIsPSWDModified)
        m_item.SetRMTime(t);
#ifdef NOTYET
      if (m_AEMD.oldlocXTime != m_AEMD.locXTime)
        m_item.SetXTime(m_AEMD.tttXTime);
#endif
      if (m_XTimeInt   != lastXTimeInt)
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
      
#ifdef NOTYET
      if (m_AEMD.XTimeInt > 0 && m_AEMD.XTimeInt <= 3650)
        m_item.SetXTimeInt(m_AEMD.XTimeInt);

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
    if (m_OnRB->GetValue()) { // absolute exp time
      wxDateTime xdt = m_ExpDate->GetValue();
      xdt.SetHour(m_ExpTimeH->GetValue());
      xdt.SetMinute(m_ExpTimeM->GetValue());
    } else { // relative, possibly recurring
      // If it's a non-recurring interval, just set XTime to
      // now + interval, XTimeInt should be stored as zero
      // (one-shot semantics)
      // Otherwise, XTime += interval, keep XTimeInt
    }
#if 0
  CTime XTime, LDate, LDateTime;

  if (m_how == ABSOLUTE_EXP) {
    VERIFY(m_pTimeCtl.GetTime(XTime) == GDT_VALID);
    VERIFY(m_pDateCtl.GetTime(LDate) == GDT_VALID);

    LDateTime = CTime(LDate.GetYear(), LDate.GetMonth(), LDate.GetDay(),
                      XTime.GetHour(), XTime.GetMinute(), 0, -1);
    M_XTimeInt() = 0;
  } else { // m_how == RELATIVE_EXP
    if (m_ReuseOnPswdChange == FALSE) { // non-recurring
      LDateTime = CTime::GetCurrentTime() + CTimeSpan(m_numDays, 0, 0, 0);
      M_XTimeInt() = 0;
    } else { // recurring interval
      LDateTime = CTime(M_tttCPMTime()) + CTimeSpan(m_numDays, 0, 0, 0);
      M_XTimeInt() = m_numDays;
    }
  }

  // m_XTimeInt is non-zero iff user specified a relative & recurring exp. date
  M_tttXTime() = (time_t)LDateTime.GetTime();
  M_locXTime() = PWSUtil::ConvertToDateTimeString(M_tttXTime(), TMC_LOCALE);

  CString cs_text(L"");
  if (M_XTimeInt() != 0) // recurring expiration
    cs_text.Format(IDS_IN_N_DAYS, M_XTimeInt());

  GetDlgItem(IDC_XTIME)->SetWindowText(M_locXTime());
  GetDlgItem(IDC_XTIME_RECUR)->SetWindowText(cs_text);
#endif
  }
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

