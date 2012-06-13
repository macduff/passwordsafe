/*
 * Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
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
#include "core/PWSprefs.h"
#include "core/PWCharPool.h"
#include "core/PWHistory.h"
#include "core/UIinterface.h"

#include "addeditpropsheet.h"
#include "pwsclip.h"
#include "./wxutils.h"

#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>
#endif
#include <algorithm>

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

  EVT_CHECKBOX( ID_CHECKBOX7, AddEditPropSheet::OnEZreadOrProounceable )

  EVT_CHECKBOX( ID_CHECKBOX8, AddEditPropSheet::OnEZreadOrProounceable )

  EVT_CHECKBOX( ID_CHECKBOX9, AddEditPropSheet::OnUseHexCBClick )

  EVT_BUTTON( ID_BUTTON7, AddEditPropSheet::OnResetPWPolicy )

  EVT_UPDATE_UI(ID_BUTTON7, AddEditPropSheet::OnUpdateResetPWPolicyButton)
////@end AddEditPropSheet event table entries
  EVT_SPINCTRL(ID_SPINCTRL5, AddEditPropSheet::OnAtLeastChars)
  EVT_SPINCTRL(ID_SPINCTRL6, AddEditPropSheet::OnAtLeastChars)
  EVT_SPINCTRL(ID_SPINCTRL7, AddEditPropSheet::OnAtLeastChars)
  EVT_SPINCTRL(ID_SPINCTRL8, AddEditPropSheet::OnAtLeastChars)

  EVT_BUTTON( ID_BUTTON1,    AddEditPropSheet::OnClearPWHist )
END_EVENT_TABLE()


/*!
 * AddEditPropSheet constructors
 */

AddEditPropSheet::AddEditPropSheet(wxWindow* parent, PWScore &core,
                                   AddOrEdit type, const CItemData *item, UIInterFace* ui,
                                   const wxString& selectedGroup,
                                   wxWindowID id, const wxString& caption,
                                   const wxPoint& pos, const wxSize& size,
                                   long style)
: m_core(core), m_ui(ui), m_selectedGroup(selectedGroup), m_type(type)
{
  if (item != NULL)
    m_item = *item; // copy existing item to display values
  else
    m_item.CreateUUID(); // We're adding a new entry
  Init();
  wxString dlgTitle;
  if (caption == SYMBOL_AUTOPROPSHEET_TITLE) {
    switch(m_type) {
      case ADD:
        dlgTitle = SYMBOL_ADDPROPSHEET_TITLE;
        break;
      case EDIT:
        dlgTitle = SYMBOL_EDITPROPSHEET_TITLE;
        break;
      case VIEW:
        dlgTitle = SYMBOL_VIEWPROPSHEET_TITLE;
        break;
      default:
        dlgTitle = caption;
        break;
}
  }
  Create(parent, id, dlgTitle, pos, size, style);
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
  m_isNotesHidden = !PWSprefs::GetInstance()->GetPref(PWSprefs::ShowNotesDefault);
  m_BasicPanel = NULL;
  m_BasicFGSizer = NULL;
  m_groupCtrl = NULL;
  m_PasswordCtrl = NULL;
  m_ShowHideCtrl = NULL;
  m_Password2Ctrl = NULL;
  m_noteTX = NULL;
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

class PolicyValidator : public MultiCheckboxValidator
{
public:
  PolicyValidator(int rbID, int ids[], size_t num,
		  const wxString& msg, const wxString& title)
    : MultiCheckboxValidator(ids, num, msg, title), m_rbID(rbID) {}
  PolicyValidator(const PolicyValidator &other)
    : MultiCheckboxValidator(other), m_rbID(other.m_rbID) {}
  ~PolicyValidator() {}

  wxObject* Clone() const {return new PolicyValidator(m_rbID, m_ids, m_count, m_msg, m_title);}
  bool Validate(wxWindow* parent) {
    wxWindow* win = GetWindow()->FindWindow(m_rbID);
    if (win && win->IsEnabled()) {
      wxRadioButton* rb = wxDynamicCast(win, wxRadioButton);
      if (rb && rb->GetValue()) {
	return true;
      }
    }
    return MultiCheckboxValidator::Validate(parent);
  }
private:
  int m_rbID;
};

/*!
 * Control creation for AddEditPropSheet
 */

void AddEditPropSheet::CreateControls()
{
  PWSprefs *prefs = PWSprefs::GetInstance();
////@begin AddEditPropSheet content construction
  // Next variable currently not referenced
  // AddEditPropSheet* itemPropertySheetDialog = this;

  m_BasicPanel = new wxPanel( GetBookCtrl(), ID_PANEL_BASIC, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
  wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
  m_BasicPanel->SetSizer(itemBoxSizer3);

  wxStaticText* itemStaticText4 = new wxStaticText( m_BasicPanel, wxID_STATIC, _("To add a new entry, simply fill in the fields below. At least a title and a\npassword are required. If you have set a default username, it will appear in the\nusername field."), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  m_BasicFGSizer = new wxFlexGridSizer(0, 3, 0, 0);
  itemBoxSizer3->Add(m_BasicFGSizer, 0, wxALIGN_LEFT|wxALL, 5);
  wxStaticText* itemStaticText6 = new wxStaticText( m_BasicPanel, wxID_STATIC, _("Group:"), wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemStaticText6, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxArrayString m_groupCtrlStrings;
  m_groupCtrl = new wxComboBox( m_BasicPanel, ID_COMBOBOX1, wxEmptyString, wxDefaultPosition, wxDefaultSize, m_groupCtrlStrings, wxCB_DROPDOWN );
  m_BasicFGSizer->Add(m_groupCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_BasicFGSizer->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText9 = new wxStaticText( m_BasicPanel, wxID_STATIC, _("Title:"), wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl10 = new wxTextCtrl( m_BasicPanel, ID_TEXTCTRL5, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemTextCtrl10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_BasicFGSizer->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText12 = new wxStaticText( m_BasicPanel, wxID_STATIC, _("Username:"), wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemStaticText12, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl13 = new wxTextCtrl( m_BasicPanel, ID_TEXTCTRL1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemTextCtrl13, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_BasicFGSizer->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText15 = new wxStaticText( m_BasicPanel, wxID_STATIC, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemStaticText15, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_PasswordCtrl = new wxTextCtrl( m_BasicPanel, ID_TEXTCTRL2, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(m_PasswordCtrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
  m_BasicFGSizer->Add(itemBoxSizer17, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  itemBoxSizer17->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_ShowHideCtrl = new wxButton( m_BasicPanel, ID_BUTTON2, _("&Hide"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer17->Add(m_ShowHideCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  itemBoxSizer17->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton21 = new wxButton( m_BasicPanel, ID_BUTTON3, _("&Generate"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer17->Add(itemButton21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText22 = new wxStaticText( m_BasicPanel, wxID_STATIC, _("Confirm:"), wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemStaticText22, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_Password2Ctrl = new wxTextCtrl( m_BasicPanel, ID_TEXTCTRL3, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
  ApplyPasswordFont(m_Password2Ctrl);
  m_BasicFGSizer->Add(m_Password2Ctrl, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_BasicFGSizer->Add(10, 10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxStaticText* itemStaticText25 = new wxStaticText( m_BasicPanel, wxID_STATIC, _("URL:"), wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemStaticText25, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxTextCtrl* itemTextCtrl26 = new wxTextCtrl( m_BasicPanel, ID_TEXTCTRL4, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
  m_BasicFGSizer->Add(itemTextCtrl26, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
  m_BasicFGSizer->Add(itemBoxSizer27, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  itemBoxSizer27->Add(10, 10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  wxButton* itemButton29 = new wxButton( m_BasicPanel, ID_GO_BTN, _("Go"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer27->Add(itemButton29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxBoxSizer* itemBoxSizer30 = new wxBoxSizer(wxHORIZONTAL);
  itemBoxSizer3->Add(itemBoxSizer30, 0, wxGROW|wxALL, 5);
  wxStaticText* itemStaticText31 = new wxStaticText( m_BasicPanel, wxID_STATIC, _("Notes:"), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer30->Add(itemStaticText31, 1, wxALIGN_TOP|wxALL, 5);

  m_noteTX = new wxTextCtrl( m_BasicPanel, ID_TEXTCTRL7, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE );
  itemBoxSizer30->Add(m_noteTX, 5, wxALIGN_CENTER_VERTICAL|wxALL, 3);

  GetBookCtrl()->AddPage(m_BasicPanel, _("Basic"));

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

  m_ExpDate = new wxDatePickerCtrl( itemPanel54, ID_DATECTRL, wxDateTime(10, static_cast<wxDateTime::Month>(7), 2009), wxDefaultPosition, wxDefaultSize, wxDP_DEFAULT );
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

  wxPanel* itemPanel90 = new wxPanel( GetBookCtrl(), ID_PANEL_PPOLICY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxTAB_TRAVERSAL );
  wxStaticBox* itemStaticBoxSizer91Static = new wxStaticBox(itemPanel90, wxID_ANY, _("Random password generation rules"));
  wxStaticBoxSizer* itemStaticBoxSizer91 = new wxStaticBoxSizer(itemStaticBoxSizer91Static, wxVERTICAL);
  itemPanel90->SetSizer(itemStaticBoxSizer91);

  m_defPWPRB = new wxRadioButton( itemPanel90, ID_RADIOBUTTON2, _("Use Database Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
  m_defPWPRB->SetValue(false);
  itemStaticBoxSizer91->Add(m_defPWPRB, 0, wxALIGN_LEFT|wxALL, 5);

  m_ourPWPRB = new wxRadioButton( itemPanel90, ID_RADIOBUTTON3, _("Use the policy below:"), wxDefaultPosition, wxDefaultSize, 0 );
  m_ourPWPRB->SetValue(false);
  itemStaticBoxSizer91->Add(m_ourPWPRB, 0, wxALIGN_LEFT|wxALL, 5);

  wxStaticLine* itemStaticLine94 = new wxStaticLine( itemPanel90, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
  itemStaticBoxSizer91->Add(itemStaticLine94, 0, wxGROW|wxALL, 5);

  wxBoxSizer* itemBoxSizer95 = new wxBoxSizer(wxHORIZONTAL);
  itemStaticBoxSizer91->Add(itemBoxSizer95, 0, wxALIGN_LEFT|wxALL, 5);
  wxStaticText* itemStaticText96 = new wxStaticText( itemPanel90, wxID_STATIC, _("Password length: "), wxDefaultPosition, wxDefaultSize, 0 );
  itemBoxSizer95->Add(itemStaticText96, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpLenCtrl = new wxSpinCtrl( itemPanel90, ID_SPINCTRL3, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 4, 1024, prefs->GetPref(PWSprefs::PWDefaultLength));
  itemBoxSizer95->Add(m_pwpLenCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwMinsGSzr = new wxGridSizer(6, 2, 0, 0);
  itemStaticBoxSizer91->Add(m_pwMinsGSzr, 0, wxALIGN_LEFT|wxALL, 5);
  m_pwpUseLowerCtrl = new wxCheckBox( itemPanel90, ID_CHECKBOX3, _("Use lowercase letters"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpUseLowerCtrl->SetValue(prefs->GetPref(PWSprefs::PWUseLowercase));
  m_pwMinsGSzr->Add(m_pwpUseLowerCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwNumLCbox = new wxBoxSizer(wxHORIZONTAL);
  m_pwMinsGSzr->Add(m_pwNumLCbox, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  wxStaticText* itemStaticText101 = new wxStaticText( itemPanel90, wxID_STATIC, _("(At least "), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumLCbox->Add(itemStaticText101, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpLCSpin = new wxSpinCtrl( itemPanel90, ID_SPINCTRL5, wxEmptyString, wxDefaultPosition,
				wxSize(itemPanel90->ConvertDialogToPixels(wxSize(20, -1)).x, -1),
				wxSP_ARROW_KEYS, 0, 100,
				prefs->GetPref(PWSprefs::PWLowercaseMinLength));
  m_pwNumLCbox->Add(m_pwpLCSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText103 = new wxStaticText( itemPanel90, wxID_STATIC, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumLCbox->Add(itemStaticText103, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpUseUpperCtrl = new wxCheckBox( itemPanel90, ID_CHECKBOX4, _("Use UPPERCASE letters"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpUseUpperCtrl->SetValue(prefs->GetPref(PWSprefs::PWUseUppercase));
  m_pwMinsGSzr->Add(m_pwpUseUpperCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwNumUCbox = new wxBoxSizer(wxHORIZONTAL);
  m_pwMinsGSzr->Add(m_pwNumUCbox, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  wxStaticText* itemStaticText106 = new wxStaticText( itemPanel90, wxID_STATIC, _("(At least "), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumUCbox->Add(itemStaticText106, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpUCSpin = new wxSpinCtrl( itemPanel90, ID_SPINCTRL6, wxEmptyString, wxDefaultPosition,
				wxSize(itemPanel90->ConvertDialogToPixels(wxSize(20, -1)).x, -1),
				wxSP_ARROW_KEYS, 0, 100,
				prefs->GetPref(PWSprefs::PWUppercaseMinLength));
  m_pwNumUCbox->Add(m_pwpUCSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText108 = new wxStaticText( itemPanel90, wxID_STATIC, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumUCbox->Add(itemStaticText108, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpUseDigitsCtrl = new wxCheckBox( itemPanel90, ID_CHECKBOX5, _("Use digits"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpUseDigitsCtrl->SetValue(prefs->GetPref(PWSprefs::PWUseDigits));
  m_pwMinsGSzr->Add(m_pwpUseDigitsCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwNumDigbox = new wxBoxSizer(wxHORIZONTAL);
  m_pwMinsGSzr->Add(m_pwNumDigbox, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  wxStaticText* itemStaticText111 = new wxStaticText( itemPanel90, wxID_STATIC, _("(At least "), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumDigbox->Add(itemStaticText111, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpDigSpin = new wxSpinCtrl( itemPanel90, ID_SPINCTRL7, wxEmptyString, wxDefaultPosition,
				 wxSize(itemPanel90->ConvertDialogToPixels(wxSize(20, -1)).x, -1),
				 wxSP_ARROW_KEYS, 0, 100, prefs->GetPref(PWSprefs::PWDigitMinLength));
  m_pwNumDigbox->Add(m_pwpDigSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText113 = new wxStaticText( itemPanel90, wxID_STATIC, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumDigbox->Add(itemStaticText113, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpSymCtrl = new wxCheckBox( itemPanel90, ID_CHECKBOX6, _("Use symbols (i.e., ., %, $, etc.)"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpSymCtrl->SetValue(prefs->GetPref(PWSprefs::PWUseSymbols));
  m_pwMinsGSzr->Add(m_pwpSymCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwNumSymbox = new wxBoxSizer(wxHORIZONTAL);
  m_pwMinsGSzr->Add(m_pwNumSymbox, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
  wxStaticText* itemStaticText116 = new wxStaticText( itemPanel90, wxID_STATIC, _("(At least "), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumSymbox->Add(itemStaticText116, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpSymSpin = new wxSpinCtrl( itemPanel90, ID_SPINCTRL8, wxEmptyString, wxDefaultPosition,
				 wxSize(itemPanel90->ConvertDialogToPixels(wxSize(20, -1)).x, -1),
				 wxSP_ARROW_KEYS, 0, 100, prefs->GetPref(PWSprefs::PWSymbolMinLength));
  m_pwNumSymbox->Add(m_pwpSymSpin, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText118 = new wxStaticText( itemPanel90, wxID_STATIC, _(")"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwNumSymbox->Add(itemStaticText118, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  m_pwpEasyCtrl = new wxCheckBox( itemPanel90, ID_CHECKBOX7, _("Use only easy-to-read characters\n(i.e., no 'l', '1', etc.)"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpEasyCtrl->SetValue(prefs->GetPref(PWSprefs::PWUseEasyVision));
  m_pwMinsGSzr->Add(m_pwpEasyCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwMinsGSzr->Add(10, 10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwpPronounceCtrl = new wxCheckBox( itemPanel90, ID_CHECKBOX8, _("Generate pronounceable passwords"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpPronounceCtrl->SetValue(prefs->GetPref(PWSprefs::PWMakePronounceable));
  m_pwMinsGSzr->Add(m_pwpPronounceCtrl, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  m_pwMinsGSzr->Add(10, 10, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);

  wxStaticText* itemStaticText123 = new wxStaticText( itemPanel90, wxID_STATIC, _("Or"), wxDefaultPosition, wxDefaultSize, 0 );
  itemStaticBoxSizer91->Add(itemStaticText123, 0, wxALIGN_LEFT|wxALL, 5);

  m_pwpHexCtrl = new wxCheckBox( itemPanel90, ID_CHECKBOX9, _("Use hexadecimal digits only (0-9, a-f)"), wxDefaultPosition, wxDefaultSize, 0 );
  m_pwpHexCtrl->SetValue(prefs->GetPref(PWSprefs::PWUseHexDigits));
  itemStaticBoxSizer91->Add(m_pwpHexCtrl, 0, wxALIGN_LEFT|wxALL, 5);

  wxButton* itemButton125 = new wxButton( itemPanel90, ID_BUTTON7, _("Reset to Database Defaults"), wxDefaultPosition, wxDefaultSize, 0 );
  itemStaticBoxSizer91->Add(itemButton125, 0, wxALIGN_RIGHT|wxALL, 5);

  int checkbox_ids[] = {ID_CHECKBOX3, ID_CHECKBOX4, ID_CHECKBOX5, ID_CHECKBOX6, ID_CHECKBOX9};
  itemPanel90->SetValidator(PolicyValidator(ID_RADIOBUTTON2, checkbox_ids, WXSIZEOF(checkbox_ids),
					    _("At least one type of character (lowercase, uppercase, digits,\nsymbols, hexadecimal) must be permitted."),
					    _("Password Policy")));
  GetBookCtrl()->AddPage(itemPanel90, _("Password Policy"));

  // Set validators
  itemTextCtrl10->SetValidator( wxGenericValidator(& m_title) );
  itemTextCtrl13->SetValidator( wxGenericValidator(& m_user) );
  itemTextCtrl26->SetValidator( wxGenericValidator(& m_url) );
  m_noteTX->SetValidator( wxGenericValidator(& m_notes) );
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
  // Connect events and objects
  m_noteTX->Connect(ID_TEXTCTRL7, wxEVT_SET_FOCUS, wxFocusEventHandler(AddEditPropSheet::OnNoteSetFocus), NULL, this);
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

void AddEditPropSheet::UpdatePWPolicyControls(const PWPolicy& pwp)
{
    bool bUseVal; // keep picky compiler happy, code readable
  m_pwpLenCtrl->SetValue(pwp.length);
  bUseVal = (pwp.flags & PWSprefs::PWPolicyUseLowercase) != 0;
    m_pwpUseLowerCtrl->SetValue(bUseVal);
  m_pwpLCSpin->SetValue(pwp.lowerminlength);
  bUseVal = (pwp.flags & PWSprefs::PWPolicyUseUppercase) != 0;
    m_pwpUseUpperCtrl->SetValue(bUseVal);
  m_pwpUCSpin->SetValue(pwp.upperminlength);
  bUseVal = (pwp.flags & PWSprefs::PWPolicyUseDigits) != 0;
    m_pwpUseDigitsCtrl->SetValue(bUseVal);
  m_pwpDigSpin->SetValue(pwp.digitminlength);
  bUseVal = (pwp.flags & PWSprefs::PWPolicyUseSymbols) != 0;
    m_pwpSymCtrl->SetValue(bUseVal);
  m_pwpSymSpin->SetValue(pwp.symbolminlength);
  bUseVal = (pwp.flags & PWSprefs::PWPolicyUseEasyVision) != 0;
    m_pwpEasyCtrl->SetValue(bUseVal);
  bUseVal = (pwp.flags & PWSprefs::PWPolicyMakePronounceable) != 0;
    m_pwpPronounceCtrl->SetValue(bUseVal);
  bUseVal = (pwp.flags & PWSprefs::PWPolicyUseHexDigits) != 0;
    m_pwpHexCtrl->SetValue(bUseVal);

  EnableSizerChildren(m_pwMinsGSzr, !m_pwpHexCtrl->GetValue());
  ShowPWPSpinners(!m_pwpPronounceCtrl->GetValue() && !m_pwpEasyCtrl->GetValue());
  }

void AddEditPropSheet::EnablePWPolicyControls(bool enable)
{
  m_pwpLenCtrl->Enable(enable);
  EnableSizerChildren(m_pwMinsGSzr, enable && !m_pwpHexCtrl->GetValue());
  m_pwpHexCtrl->Enable(enable);
}

struct newer {
  bool operator()(const PWHistEntry& first, const PWHistEntry& second) const {
    return first.changetttdate > second.changetttdate;
  }
};

void AddEditPropSheet::ItemFieldsToPropSheet()
{
  // Populate the combo box
  std::vector<stringT> aryGroups;
  m_core.GetUniqueGroups(aryGroups);
  for (size_t igrp = 0; igrp < aryGroups.size(); igrp++) {
    m_groupCtrl->Append(aryGroups[igrp].c_str());
  }
  // select relevant group
  const StringX group = (m_type == ADD? tostringx(m_selectedGroup): m_item.GetGroup());
  if (!group.empty()) {
    bool foundGroup = false;
    for (size_t igrp = 0; igrp < aryGroups.size(); igrp++) {
      if (group == aryGroups[igrp].c_str()) {
        m_groupCtrl->SetSelection((int)igrp);
        foundGroup =true;
        break;
      }
    }
    if (!foundGroup)
      m_groupCtrl->SetValue(m_selectedGroup);
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
  m_notes = (m_type != ADD && m_isNotesHidden) ?
    wxString(_("[Notes hidden - click here to display]")) : towxstring(m_item.GetNotes());
  // Following has no effect under Linux :-(
  long style = m_noteTX->GetExtraStyle();
  if (prefs->GetPref(PWSprefs::NotesWordWrap))
    style |= wxTE_WORDWRAP;
  else
    style &= ~wxTE_WORDWRAP;
  m_noteTX->SetExtraStyle(style);
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
    PWSprefs *prefs1 = PWSprefs::GetInstance();
    m_keepPWHist = prefs1->GetPref(PWSprefs::SavePasswordHistory);
    m_maxPWHist = prefs1->GetPref(PWSprefs::NumPWHistoryDefault);
  } else { // EDIT or VIEW
    PWHistList pwhl;
    size_t pwh_max, num_err;

    const StringX pwh_str = m_item.GetPWHistory();
    if (!pwh_str.empty()) {
      m_PWHistory = towxstring(pwh_str);
      m_keepPWHist = CreatePWHistoryList(pwh_str,
                                         pwh_max, num_err,
                                         pwhl, TMC_LOCALE);
      if (size_t(m_PWHgrid->GetNumberRows()) < pwhl.size()) {
        m_PWHgrid->AppendRows(pwhl.size() - m_PWHgrid->GetNumberRows());
      }
      m_maxPWHist = int(pwh_max);
      //reverse-sort the history entries so that we list the newest first 
      std::sort(pwhl.begin(), pwhl.end(), newer());
      int row = 0;
      for (PWHistList::iterator iter = pwhl.begin(); iter != pwhl.end();
           ++iter) {
        m_PWHgrid->SetCellValue(row, 0, iter->changedate.c_str());
        m_PWHgrid->SetCellValue(row, 1, iter->password.c_str());
        row++;
      }
    } else { // empty history string
      // Get history preferences
      PWSprefs *prefs1 = PWSprefs::GetInstance();
      m_keepPWHist = prefs1->GetPref(PWSprefs::SavePasswordHistory);
      m_maxPWHist = prefs1->GetPref(PWSprefs::NumPWHistoryDefault);
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
  if (!defPwPolicy) {
    PWPolicy pwp;
    m_item.GetPWPolicy(pwp);
    UpdatePWPolicyControls(pwp);
    m_pwpLenCtrl->Enable(true);
}
  else {
    EnablePWPolicyControls(false);
  }
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_GO_BTN
 */

void AddEditPropSheet::OnGoButtonClick( wxCommandEvent& /* evt */ )
{
  ::wxLaunchDefaultBrowser(m_url, wxBROWSER_NEW_WINDOW);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON3
 */

void AddEditPropSheet::OnGenerateButtonClick( wxCommandEvent& /* evt */ )
{
  PWPolicy pwp = GetSelectedPWPolicy();
  StringX password = pwp.MakeRandomPassword();
  if (password.empty()) {
    wxMessageBox(_("Couldn't generate password - invalid policy"),
                 _("Error"), wxOK|wxICON_INFORMATION, this);
      return;
  }

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

void AddEditPropSheet::OnShowHideClick( wxCommandEvent& /* evt */ )
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

  // Per Dave Silvia's suggestion:
  // Following kludge since wxTE_PASSWORD style is immutable
  wxTextCtrl *tmp = m_PasswordCtrl;
  const wxString pwd = m_password.c_str();
  m_PasswordCtrl = new wxTextCtrl(m_BasicPanel, ID_TEXTCTRL2,
                                  pwd,
                                  wxDefaultPosition, wxDefaultSize,
                                  0);
  if (!pwd.IsEmpty()) {
    m_PasswordCtrl->ChangeValue(pwd);
    m_PasswordCtrl->SetModified(true);
  }
  m_BasicFGSizer->Replace(tmp, m_PasswordCtrl);
  delete tmp;
  m_BasicFGSizer->Layout();
  // Disable confirmation Ctrl, as the user can see the password entered
  m_Password2Ctrl->Clear();
  m_Password2Ctrl->Enable(false);
}

void AddEditPropSheet::HidePassword()
{
  m_isPWHidden = true;
  m_ShowHideCtrl->SetLabel(_("&Show"));

  // Per Dave Silvia's suggestion:
  // Following kludge since wxTE_PASSWORD style is immutable
  // Need verification as the user can not see the password entered
  wxTextCtrl *tmp = m_PasswordCtrl;
  const wxString pwd = m_password.c_str();
  m_PasswordCtrl = new wxTextCtrl(m_BasicPanel, ID_TEXTCTRL2,
                                  pwd,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_PASSWORD);
  ApplyPasswordFont(m_PasswordCtrl);
  m_BasicFGSizer->Replace(tmp, m_PasswordCtrl);
  delete tmp;
  m_BasicFGSizer->Layout();
  if (!pwd.IsEmpty()) {
    m_PasswordCtrl->ChangeValue(pwd);
    m_PasswordCtrl->SetModified(true);
  }
  m_Password2Ctrl->ChangeValue(pwd);
  m_Password2Ctrl->Enable(true);
}

void AddEditPropSheet::OnOk(wxCommandEvent& /* evt */)
{
  if (Validate() && TransferDataFromWindow()) {
    time_t t;
    const wxString group = m_groupCtrl->GetValue();
    const StringX password = tostringx(m_PasswordCtrl->GetValue());

    if (m_title.IsEmpty() || password.empty()) {
      GetBookCtrl()->SetSelection(0);
      if (m_title.IsEmpty())
        FindWindow(ID_TEXTCTRL5)->SetFocus();
      else
        m_PasswordCtrl->SetFocus();

      wxMessageBox(wxString(_("This entry must have a ")) << (m_title.IsEmpty() ? _("title"): _("password")),
                    _("Error"), wxOK|wxICON_INFORMATION, this);
      return;
    }

    if (m_isPWHidden) { // hidden passwords - compare both values
      const StringX p2 = tostringx(m_Password2Ctrl->GetValue());
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

      // Following ensures that untouched & hidden note
      // isn't marked as modified. Relies on fact that
      // Note field can't be modified w/o first getting focus
      // and that we turn off m_isNotesHidden when that happens.
      if (m_type != ADD && m_isNotesHidden)
        m_notes = m_item.GetNotes().c_str();

      // Create a new PWHistory string based on settings in this dialog, and compare it
      // with the PWHistory string from the item being edited, to see if the user modified it.
      // Note that we are not erasing the history here, even if the user has chosen to not
      // track PWHistory.  So there could be some password entries in the history 
      // but the first byte could be zero, meaning we are not tracking it _FROM_NOW_.
      // Clearing the history is something the user must do himself with the "Clear History" button

      // First, Get a list of all password history entries
      size_t pwh_max, num_err;
      PWHistList pwhl;
      (void)CreatePWHistoryList(tostringx(m_PWHistory), pwh_max, num_err, pwhl, TMC_LOCALE);

      // Create a new PWHistory header, as per settings in this dialog
      size_t numEntries = MIN(pwhl.size(), static_cast<size_t>(m_maxPWHist));
      m_PWHistory = towxstring(MakePWHistoryHeader(m_keepPWHist, m_maxPWHist, numEntries));
      //reverse-sort the history entries to retain only the newest
      std::sort(pwhl.begin(), pwhl.end(), newer());
      // Now add all the existing history entries, up to a max of what the user wants to track
      // This code is from CItemData::UpdatePasswordHistory()
      PWHistList::iterator iter;
      for (iter = pwhl.begin(); iter != pwhl.end() && numEntries > 0; iter++, numEntries--) {
        StringX buffer;
        Format(buffer, _T("%08x%04x%s"),
               static_cast<long>(iter->changetttdate), iter->password.length(),
               iter->password.c_str());
        m_PWHistory += towxstring(buffer);
      }

      wxASSERT_MSG(numEntries ==0, wxT("Could not save existing password history entries"));

      PWPolicy pwp;
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
                     (!isPWPDefault && ((pwp = GetPWPolicyFromUI()) != oldPWP)) ||
                     (isPWPDefault && oldPWP != PWPolicy()));

      bIsPSWDModified = (password != m_item.GetPassword());

      if (bIsModified) {
        // Just modify all - even though only 1 may have actually been modified
        m_item.SetGroup(tostringx(group));
        m_item.SetTitle(tostringx(m_title));
        m_item.SetUser(m_user.empty() ?
                       PWSprefs::GetInstance()->
                       GetPref(PWSprefs::DefaultUsername).c_str() : m_user.c_str());
        m_item.SetNotes(tostringx(m_notes));
        m_item.SetURL(tostringx(m_url));
        m_item.SetAutoType(tostringx(m_autotype));
        m_item.SetRunCommand(tostringx(m_runcmd));
        m_item.SetPWHistory(tostringx(m_PWHistory));
        m_item.SetPWPolicy(pwp);
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
      m_core.Execute(EditEntryCommand::Create(&m_core,
                                              m_core.GetEntry(listpos),
                                              m_item));
      if (m_ui)
        m_ui->GUIRefreshEntry(m_item);
    }
      break;

    case ADD:
      m_item.SetGroup(tostringx(group));
      m_item.SetTitle(tostringx(m_title));
      m_item.SetUser(m_user.empty() ?
                     PWSprefs::GetInstance()->
                      GetPref(PWSprefs::DefaultUsername).c_str() : m_user.c_str());
      m_item.SetNotes(tostringx(m_notes));
      m_item.SetURL(tostringx(m_url));
      m_item.SetPassword(password);
      m_item.SetAutoType(tostringx(m_autotype));
      m_item.SetRunCommand(tostringx(m_runcmd));
      m_item.SetDCA(m_DCA);
      time(&t);
      m_item.SetCTime(t);
      if (m_keepPWHist)
        m_item.SetPWHistory(MakePWHistoryHeader(TRUE, m_maxPWHist, 0));

      m_item.SetXTime(m_tttXTime);
      if (m_XTimeInt > 0 && m_XTimeInt <= 3650)
        m_item.SetXTimeInt(m_XTimeInt);
      if (m_ourPWPRB->GetValue())
        m_item.SetPWPolicy(GetPWPolicyFromUI());

#ifdef NOTYET
if (m_AEMD.ibasedata > 0) {
        // Password in alias format AND base entry exists
        // No need to check if base is an alias as already done in
        // call to PWScore::ParseBaseEntryPWD
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
    EndModal(wxID_OK);
  }
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

void AddEditPropSheet::OnOverrideDCAClick( wxCommandEvent& /* evt */ )
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

void AddEditPropSheet::OnSetXTime( wxCommandEvent& /* evt */ )
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

void AddEditPropSheet::OnClearXTime( wxCommandEvent& /* evt */ )
{
  m_XTime = _("Never");
  m_CurXTime.Clear();
  m_tttXTime = time_t(0);
  m_XTimeInt = 0;
  m_Recurring = false;
  TransferDataToWindow();
}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON
 */

void AddEditPropSheet::OnRadiobuttonSelected( wxCommandEvent& evt )
{
  bool On = (evt.GetEventObject() == m_OnRB);
  m_ExpDate->Enable(On);
  m_ExpTimeH->Enable(On);
  m_ExpTimeM->Enable(On);
  m_ExpTimeCtrl->Enable(!On);
  m_RecurringCtrl->Enable(!On);

}


/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON2
 */

void AddEditPropSheet::OnPWPRBSelected( wxCommandEvent& evt )
{
  EnablePWPolicyControls(evt.GetEventObject() != m_defPWPRB);
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

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX7
 */

void AddEditPropSheet::OnEZreadOrProounceable(wxCommandEvent& evt)
{
 if (Validate() && TransferDataFromWindow()) {
   if (m_pwpEasyCtrl->GetValue() && m_pwpPronounceCtrl->GetValue()) {
    wxMessageBox(_("Sorry, 'pronounceable' and 'easy-to-read' are not supported together"),
                        _("Password Policy"), wxOK | wxICON_EXCLAMATION, this);
    if (evt.GetEventObject() == m_pwpPronounceCtrl)
      m_pwpPronounceCtrl->SetValue(false);
    else
      m_pwpEasyCtrl->SetValue(false);
 }
   else {
     ShowPWPSpinners(!evt.IsChecked());
}
 }
}

void AddEditPropSheet::EnableNonHexCBs(bool enable)
{
  EnableSizerChildren(m_pwMinsGSzr, enable);
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX9
 */

void AddEditPropSheet::OnUseHexCBClick( wxCommandEvent& /* evt */ )
{
 if (Validate() && TransferDataFromWindow()) {
   bool useHex = m_pwpHexCtrl->GetValue();
   EnableNonHexCBs(!useHex);
 }
}


/*!
 * wxEVT_SET_FOCUS event handler for ID_TEXTCTRL7
 */

void AddEditPropSheet::OnNoteSetFocus( wxFocusEvent& /* evt */ )
{
  if (m_type != ADD && m_isNotesHidden) {
    m_isNotesHidden = false;
    m_notes = m_item.GetNotes().c_str();
    m_noteTX->ChangeValue(m_notes);
  }
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_BUTTON7
 */
void AddEditPropSheet::OnResetPWPolicy(wxCommandEvent& /*evt*/)
{
  UpdatePWPolicyControls(GetPWPolicyFromPrefs());
}

PWPolicy AddEditPropSheet::GetPWPolicyFromUI() const
{
  wxASSERT_MSG(m_ourPWPRB->GetValue() && !m_defPWPRB->GetValue(), wxT("Trying to get Password policy from UI when db defaults are to be used"));
  
  PWPolicy pwp;

  pwp.length = m_pwpLenCtrl->GetValue();
  pwp.flags = 0;
  pwp.lowerminlength = pwp.upperminlength =
    pwp.digitminlength = pwp.symbolminlength = 0;
  if (m_pwpUseLowerCtrl->GetValue()) {
    pwp.flags |= PWSprefs::PWPolicyUseLowercase;
    pwp.lowerminlength = m_pwpLCSpin->GetValue();
  }
  if (m_pwpUseUpperCtrl->GetValue()) {
    pwp.flags |= PWSprefs::PWPolicyUseUppercase;
    pwp.upperminlength = m_pwpUCSpin->GetValue();
  }
  if (m_pwpUseDigitsCtrl->GetValue()) {
    pwp.flags |= PWSprefs::PWPolicyUseDigits;
    pwp.digitminlength = m_pwpDigSpin->GetValue();
  }
  if (m_pwpSymCtrl->GetValue()) {
    pwp.flags |= PWSprefs::PWPolicyUseSymbols;
    pwp.symbolminlength = m_pwpSymSpin->GetValue();
  }

  wxASSERT_MSG(!m_pwpEasyCtrl->GetValue() || !m_pwpPronounceCtrl->GetValue(), wxT("UI Bug: both pronounceable and easy-to-read are set"));

  if (m_pwpEasyCtrl->GetValue())
    pwp.flags |= PWSprefs::PWPolicyUseEasyVision;
  else if (m_pwpPronounceCtrl->GetValue())
    pwp.flags |= PWSprefs::PWPolicyMakePronounceable;
  if (m_pwpHexCtrl->GetValue())
    pwp.flags = PWSprefs::PWPolicyUseHexDigits; //yes, its '=' and not '|='

  return pwp;
}

PWPolicy AddEditPropSheet::GetPWPolicyFromPrefs() const
{
  PWPolicy pwp;
  PWSprefs *prefs = PWSprefs::GetInstance();

  pwp.length = prefs->GetPref(PWSprefs::PWDefaultLength);
  pwp.flags = 0;
  pwp.flags |= (prefs->GetPref(PWSprefs::PWUseLowercase)     ? PWSprefs::PWPolicyUseLowercase:      0);
  pwp.flags |= (prefs->GetPref(PWSprefs::PWUseUppercase)     ? PWSprefs::PWPolicyUseUppercase:      0);
  pwp.flags |= (prefs->GetPref(PWSprefs::PWUseDigits)        ? PWSprefs::PWPolicyUseDigits   :      0);
  pwp.flags |= (prefs->GetPref(PWSprefs::PWUseSymbols)       ? PWSprefs::PWPolicyUseSymbols  :      0);
  pwp.flags |= (prefs->GetPref(PWSprefs::PWUseHexDigits)     ? PWSprefs::PWPolicyUseHexDigits:      0);
  pwp.flags |= (prefs->GetPref(PWSprefs::PWUseEasyVision)    ? PWSprefs::PWPolicyUseEasyVision:     0);
  pwp.flags |= (prefs->GetPref(PWSprefs::PWMakePronounceable)? PWSprefs::PWPolicyMakePronounceable: 0);
  pwp.lowerminlength = prefs->GetPref(PWSprefs::PWLowercaseMinLength);
  pwp.upperminlength = prefs->GetPref(PWSprefs::PWUppercaseMinLength);
  pwp.digitminlength = prefs->GetPref(PWSprefs::PWDigitMinLength);
  pwp.symbolminlength = prefs->GetPref(PWSprefs::PWSymbolMinLength);

  return pwp;
}

PWPolicy AddEditPropSheet::GetSelectedPWPolicy() const
{
  if (m_defPWPRB->GetValue())
    return GetPWPolicyFromPrefs();
  else
    return GetPWPolicyFromUI();
}

void AddEditPropSheet::OnUpdateResetPWPolicyButton(wxUpdateUIEvent& evt)
{
  evt.Enable(m_ourPWPRB->GetValue());
}

/*
 * Just trying to give the user some visual indication that
 * the password length has to be bigger than the sum of all
 * "at least" lengths.  This is not comprehensive & foolproof
 * since there are far too many ways to make the password length
 * smaller than the sum of "at least" lengths, to even think of.
 * 
 * In OnOk(), we just ensure the password length is greater than
 * the sum of all enabled "at least" lengths.  We have to do this in the
 * UI, or else password generation crashes
 */
void AddEditPropSheet::OnAtLeastChars(wxSpinEvent& /*evt*/)
{
  const int min = GetRequiredPWLength();
  //m_pwpLenCtrl->SetRange(min, pwlenCtrl->GetMax());
  if (min > m_pwpLenCtrl->GetValue())
    m_pwpLenCtrl->SetValue(min);
}

int AddEditPropSheet::GetRequiredPWLength() const {
  wxSpinCtrl* spinCtrls[] = {m_pwpUCSpin, m_pwpLCSpin, m_pwpDigSpin, m_pwpSymSpin};
  int total = 0;
  for (size_t idx = 0; idx < WXSIZEOF(spinCtrls); ++idx) {
    if (spinCtrls[idx]->IsEnabled())
      total += spinCtrls[idx]->GetValue();
  }
  return total;
}

void AddEditPropSheet::OnClearPWHist(wxCommandEvent& /*evt*/)
{
  m_PWHgrid->ClearGrid();
  if (m_MaxPWHistCtrl->TransferDataFromWindow() && m_keepPWHist && m_maxPWHist > 0) {
    m_PWHistory = towxstring(MakePWHistoryHeader(m_keepPWHist, m_maxPWHist, 0));
  }
  else
    m_PWHistory.Empty();
}
