#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "PasswordSafeSearch.h"
#include "../../corelib/PwsPlatform.h"
#include "passwordsafeframe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////@begin XPM images
#include "../graphics/find.xpm"
#include "../graphics/findreport.xpm"
#include "../graphics/find_disabled.xpm"
#include "../graphics/findadvanced.xpm"
#include "../graphics/findcase_i.xpm"
#include "../graphics/findcase_s.xpm"
#include "../graphics/findclear.xpm"
#include "../graphics/findclose.xpm"
////@end XPM images
#include <wx/statline.h> 
#include <wx/valgen.h>

#include <functional>

enum { FIND_MENU_POSITION = 4 } ;

struct {
  const charT* name;
  CItemData::FieldType type;
} subgroups[] = { {wxT("Group"),       CItemData::GROUP},
                  {wxT("Group/Title"), CItemData::GROUPTITLE},
                  {wxT("Notes"),       CItemData::NOTES},
                  {wxT("Title"),       CItemData::TITLE},
                  {wxT("URL"),         CItemData::URL},
                  {wxT("User Name"),   CItemData::USER} } ;

struct {
  const charT* name;
  PWSMatch::MatchRule function;
} subgroupFunctions[] = { {wxT("equals"),              PWSMatch::MR_EQUALS},
                          {wxT("does not equal"),      PWSMatch::MR_NOTEQUAL},
                          {wxT("begins with"),         PWSMatch::MR_BEGINS},
                          {wxT("does not begin with"), PWSMatch::MR_NOTBEGIN},
                          {wxT("ends with"),           PWSMatch::MR_ENDS},
                          {wxT("does not end with"),   PWSMatch::MR_NOTEND},
                          {wxT("contains"),            PWSMatch::MR_CONTAINS},
                          {wxT("does not contain"),    PWSMatch::MR_NOTCONTAIN} } ;

struct {
    const charT* name;
    CItemData::FieldType type;
} fieldNames[] = {  {wxT("Group"),              CItemData::GROUP},
                    {wxT("Title"),              CItemData::TITLE},
                    {wxT("User Name"),          CItemData::USER},
                    {wxT("Notes"),              CItemData::NOTES},
                    {wxT("Password"),           CItemData::PASSWORD},
                    {wxT("URL"),                CItemData::URL},
                    {wxT("Autotype"),           CItemData::AUTOTYPE},
                    {wxT("Password History"),   CItemData::PWHIST},
                    {wxT("Run Command"),        CItemData::RUNCMD},
                    {wxT("Email"),              CItemData::EMAIL}
                 };


////////////////////////////////////////////////////////////////////////////
// AdvancedSearchOptionsDlg implementation

IMPLEMENT_CLASS( AdvancedSearchOptionsDlg, wxDialog )

BEGIN_EVENT_TABLE( AdvancedSearchOptionsDlg, wxDialog )
  EVT_BUTTON( wxID_OK, AdvancedSearchOptionsDlg::OnOk )
  EVT_BUTTON( ID_SELECT_SOME, AdvancedSearchOptionsDlg::OnSelectSome )
  EVT_BUTTON( ID_SELECT_ALL, AdvancedSearchOptionsDlg::OnSelectAll )
  EVT_BUTTON( ID_REMOVE_SOME, AdvancedSearchOptionsDlg::OnRemoveSome )
  EVT_BUTTON( ID_REMOVE_ALL, AdvancedSearchOptionsDlg::OnRemoveAll )
END_EVENT_TABLE()

AdvancedSearchOptionsDlg::AdvancedSearchOptionsDlg(wxWindow* parentWnd, 
                                                   PasswordSafeSearchContext& context): m_context(context)
{
  SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
  m_searchData = m_context.Get();
  CreateControls(parentWnd);
}

void AdvancedSearchOptionsDlg::CreateControls(wxWindow* parentWnd)
{
  wxDialog::Create(parentWnd, wxID_ANY, wxT("Advanced Find Options"));
  
  wxPanel* panel = new wxPanel(this);
  wxBoxSizer* dlgSizer = new wxBoxSizer(wxVERTICAL);
  //Subset entries
  {
    wxStaticBoxSizer* sizer = new wxStaticBoxSizer(wxVERTICAL, panel);

    sizer->AddSpacer(5);
    wxCheckBox* check = new wxCheckBox(panel, wxID_ANY, wxT("&Restrict to a subset of entries:"));
    check->SetValidator(wxGenericValidator(&m_searchData.m_fUseSubgroups));
    sizer->Add(check);
    sizer->Add(10, 10);

    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
    hbox->Add(new wxStaticText(panel, wxID_ANY, wxT("&Where")), wxSizerFlags(0));
    hbox->AddSpacer(20);
    
    wxComboBox* comboSubgroup = new wxComboBox(panel, wxID_ANY);
    for (size_t idx = 0 ; idx < NumberOf(subgroups); ++idx) comboSubgroup->AppendString(subgroups[idx].name);
    comboSubgroup->SetValidator(wxGenericValidator(&m_searchData.m_subgroupObject));
    hbox->Add(comboSubgroup, wxSizerFlags(1).Expand());
    
    hbox->AddSpacer(20);
    
    wxComboBox* comboFunctions = new wxComboBox(panel, wxID_ANY);
    for( size_t idx = 0; idx < NumberOf(subgroupFunctions); ++idx) comboFunctions->AppendString(subgroupFunctions[idx].name);
    comboFunctions->SetValidator(wxGenericValidator(&m_searchData.m_subgroupFunction));
    hbox->Add(comboFunctions, wxSizerFlags(1).Expand());
    
    sizer->Add(hbox, wxSizerFlags(1).Border(wxLEFT|wxRIGHT, 15).Expand());

    sizer->AddSpacer(5);

    wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
    vbox->Add( new wxStaticText(panel, wxID_ANY, wxT("the &following text:")) );
    vbox->Add(0, 3);
    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);

    wxTextCtrl* txtCtrl = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1));
    txtCtrl->SetValidator(wxGenericValidator(&m_searchData.m_searchText));
    hsizer->Add(txtCtrl, wxSizerFlags(1).Expand().FixedMinSize());

    vbox->Add(hsizer);
    vbox->Add(0, 3);
    wxCheckBox* checkCaseSensitivity = new wxCheckBox(panel, wxID_ANY, wxT("&Case Sensitive"));
    checkCaseSensitivity->SetValidator(wxGenericValidator(&m_searchData.m_fCaseSensitive));
    vbox->Add( checkCaseSensitivity, 1, wxEXPAND );
    
    sizer->Add(vbox, wxSizerFlags().Border(wxLEFT, 15));

    dlgSizer->Add(sizer, wxSizerFlags(0).Border(wxALL, 10).Center());
  }

  {
    wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

    wxBoxSizer* vbox1 = new wxBoxSizer(wxVERTICAL);
    vbox1->Add(new wxStaticText(panel, wxID_ANY, wxT("&Available Fields:")));
    vbox1->AddSpacer(10);
    wxListBox* lbFields = new wxListBox(panel, ID_LB_AVAILABLE_FIELDS, wxDefaultPosition, 
              wxSize(100, 200), 0, NULL, wxLB_EXTENDED);
    for (size_t idx = 0; idx < NumberOf(fieldNames); ++idx)
        if (!m_searchData.m_bsFields.test(fieldNames[idx].type))
            lbFields->Append(fieldNames[idx].name, (void*)(idx));

    vbox1->Add(lbFields, wxSizerFlags(1).Expand());
    hbox->Add(vbox1);

    hbox->AddSpacer(15);

    wxBoxSizer* buttonBox = new wxBoxSizer(wxVERTICAL);
    buttonBox->Add( new wxButton(panel, ID_SELECT_SOME, wxT(">")) );
    buttonBox->AddSpacer(5);
    buttonBox->Add( new wxButton(panel, ID_SELECT_ALL, wxT(">>")) );
    buttonBox->AddSpacer(30);
    buttonBox->Add( new wxButton(panel, ID_REMOVE_SOME, wxT("<")) );
    buttonBox->Add( new wxButton(panel, ID_REMOVE_ALL, wxT("<<")) );
    buttonBox->AddSpacer(5);
    hbox->Add(buttonBox, wxSizerFlags().Center());

    hbox->AddSpacer(15);

    wxBoxSizer* vbox2 = new wxBoxSizer(wxVERTICAL);
    vbox2->Add(new wxStaticText(panel, wxID_ANY, wxT("&Selected Fields:")));
    vbox2->AddSpacer(10);
    wxListBox* lbSelectedFields = new wxListBox(panel, ID_LB_SELECTED_FIELDS, wxDefaultPosition, 
                  wxSize(100, 200), 0, NULL, wxLB_EXTENDED);
    for (size_t idx=0; idx < NumberOf(fieldNames); ++idx)
        if (m_searchData.m_bsFields.test(fieldNames[idx].type))
            lbSelectedFields->Append(fieldNames[idx].name, (void*)(idx));

    vbox2->Add(lbSelectedFields, wxSizerFlags(1).Expand());
    hbox->Add(vbox2);

    dlgSizer->Add(hbox, wxSizerFlags(1).Expand().Border(wxALL, 10));
  }
  
  dlgSizer->Add( new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxSize(300, -1)), wxSizerFlags().Border(wxALL, 10).Center() );

//  why doesn't this work?
//  dlgSizer->Add( CreateButtonSizer(wxOK | wxCANCEL | wxHELP) );
  {
      wxBoxSizer* bbox = new wxBoxSizer(wxHORIZONTAL);
      bbox->Add(new wxButton(panel, wxID_OK, wxT("&Ok")));
      bbox->AddSpacer(20);
      bbox->Add(new wxButton(panel, wxID_CANCEL, wxT("&Cancel")));
      bbox->AddSpacer(20);
      bbox->Add(new wxButton(panel, wxID_HELP, wxT("&Help")));

      dlgSizer->Add(bbox, wxSizerFlags().Center().Border(wxBOTTOM, 10));
  }

  panel->SetSizer(dlgSizer);
  dlgSizer->Fit(this);
  dlgSizer->SetSizeHints(this);

}

void AdvancedSearchOptionsDlg::OnOk( wxCommandEvent& evt )
{
  TransferDataFromWindow();

  wxListBox* lbSelected  = wxDynamicCast(FindWindow(ID_LB_SELECTED_FIELDS), wxListBox);
  wxASSERT(lbSelected);

  //reset the selected field bits 
  m_searchData.m_bsFields.reset();
  const size_t count = lbSelected->GetCount();
  
  for (size_t idx = 0; idx < count; ++idx) {
      const size_t which = (size_t)lbSelected->GetClientData((unsigned int)idx);
      m_searchData.m_bsFields.set(fieldNames[which].type, true);
  }

  if (!m_context.IsSame(m_searchData))
      m_context.Set(m_searchData);

  //Let wxDialog handle it as well, to close the window
  evt.Skip(true);
}

void AdvancedSearchOptionsDlg::OnSelectSome( wxCommandEvent& evt )
{
  wxListBox* lbAvailable = wxDynamicCast(FindWindow(ID_LB_AVAILABLE_FIELDS), wxListBox);
  wxListBox* lbSelected  = wxDynamicCast(FindWindow(ID_LB_SELECTED_FIELDS), wxListBox);
  
  wxASSERT(lbAvailable);
  wxASSERT(lbSelected);

  wxArrayInt aSelected;
  if (lbAvailable->GetSelections(aSelected)) {
    for (size_t idx = 0; idx < aSelected.GetCount(); ++idx) {
      size_t which = (size_t)lbAvailable->GetClientData((unsigned int)(aSelected[idx] - idx));
      wxASSERT(which < NumberOf(fieldNames));
      lbAvailable->Delete((unsigned int)(aSelected[idx] - idx));
      lbSelected->Append(fieldNames[which].name, (void *)which);
    }
  }
}

void AdvancedSearchOptionsDlg::OnSelectAll( wxCommandEvent& evt )
{
  wxListBox* lbAvailable = wxDynamicCast(FindWindow(ID_LB_AVAILABLE_FIELDS), wxListBox);
  wxListBox* lbSelected  = wxDynamicCast(FindWindow(ID_LB_SELECTED_FIELDS), wxListBox);
  
  wxASSERT(lbAvailable);
  wxASSERT(lbSelected);

  while (lbAvailable->GetCount()) {
      size_t which = (size_t)lbAvailable->GetClientData(0);
      lbAvailable->Delete(0);
      lbSelected->Append(fieldNames[which].name, (void*)which);
  }
}

void AdvancedSearchOptionsDlg::OnRemoveSome( wxCommandEvent& evt )
{
  wxListBox* lbAvailable = wxDynamicCast(FindWindow(ID_LB_AVAILABLE_FIELDS), wxListBox);
  wxListBox* lbSelected  = wxDynamicCast(FindWindow(ID_LB_SELECTED_FIELDS), wxListBox);
  
  wxASSERT(lbAvailable);
  wxASSERT(lbSelected);

  wxArrayInt aSelected;
  if (lbSelected->GetSelections(aSelected)) {
    for (size_t idx = 0; idx < aSelected.GetCount(); ++idx) {
      size_t which = (size_t)lbSelected->GetClientData((unsigned int)(aSelected[idx] - idx));
      wxASSERT(which < NumberOf(fieldNames));
      lbSelected->Delete((unsigned int)(aSelected[idx] - idx));
      lbAvailable->Append(fieldNames[which].name, (void *)which);
    }
  }
}

void AdvancedSearchOptionsDlg::OnRemoveAll( wxCommandEvent& evt )
{
  wxListBox* lbAvailable = wxDynamicCast(FindWindow(ID_LB_AVAILABLE_FIELDS), wxListBox);
  wxListBox* lbSelected  = wxDynamicCast(FindWindow(ID_LB_SELECTED_FIELDS), wxListBox);
  
  wxASSERT(lbAvailable);
  wxASSERT(lbSelected);

  while (lbSelected->GetCount()) {
      size_t which = (size_t)lbSelected->GetClientData(0);
      lbSelected->Delete(0);
      lbAvailable->Append(fieldNames[which].name, (void*)which);
  }
}


////////////////////////////////////////////////////////////////////////////
// PasswordSafeSerach implementation
IMPLEMENT_CLASS( PasswordSafeSearch, wxEvtHandler )

enum {
  ID_FIND_CLOSE = 10061,
  ID_FIND_EDITBOX,
  ID_FIND_NEXT,
  ID_FIND_IGNORE_CASE,
  ID_FIND_ADVANCED_OPTIONS,
  ID_FIND_CREATE_REPORT,
  ID_FIND_CLEAR,
  ID_FIND_STATUS_AREA
};

BEGIN_EVENT_TABLE( PasswordSafeSearch, wxEvtHandler )
////@begin PasswordSafeSearch event table entries
  EVT_TEXT_ENTER( ID_FIND_EDITBOX, PasswordSafeSearch::OnDoSearch )
  EVT_TOOL( ID_FIND_CLOSE, PasswordSafeSearch::OnSearchClose )
  EVT_TOOL( ID_FIND_ADVANCED_OPTIONS, PasswordSafeSearch::OnAdvancedSearchOptions )
  EVT_TOOL( ID_FIND_IGNORE_CASE, PasswordSafeSearch::OnToggleCaseSensitivity )
  EVT_TOOL( ID_FIND_NEXT, PasswordSafeSearch::OnDoSearch )
////@end PasswordSafeSearch event table entries
END_EVENT_TABLE()


PasswordSafeSearch::PasswordSafeSearch(PasswordSafeFrame* parent) : m_toolbar(0), m_parentFrame(parent), m_fAdvancedSearch(false)
{
}

PasswordSafeSearch::~PasswordSafeSearch(void)
{
  delete m_toolbar;
  m_toolbar = 0;
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_FIND_EDITBOX
 */

void PasswordSafeSearch::OnDoSearch(wxCommandEvent& /*evt*/)
{
  wxASSERT(m_toolbar);

  wxTextCtrl* txtCtrl = wxDynamicCast(m_toolbar->FindControl(ID_FIND_EDITBOX), wxTextCtrl);
  wxASSERT(txtCtrl);

  wxString searchText = txtCtrl->GetLineText(0);
  if (m_searchContext->m_searchText != searchText)
      m_searchContext.SetSearchText(searchText);

  if (m_searchContext.IsDirty())  {
      m_searchPointer.Clear();
   
      if (!m_fAdvancedSearch)
        FindMatches(StringX(m_searchContext->m_searchText), m_searchContext->m_fCaseSensitive, m_searchPointer);
      else
        FindMatches(StringX(m_searchContext->m_searchText), m_searchContext->m_fCaseSensitive, m_searchPointer, 
                      m_searchContext->m_bsFields, m_searchContext->m_fUseSubgroups, m_searchContext->m_subgroupText,
                      subgroups[m_searchContext->m_subgroupObject].type, subgroupFunctions[m_searchContext->m_subgroupFunction].function);

      m_searchContext.Reset();
      m_searchPointer.InitIndex();
  }
  else {
      ++m_searchPointer;
  }

  UpdateView();

  // Replace the "Find" menu item under Edit menu by "Find Next" and "Find Previous"
  wxMenu* editMenu = 0;
  wxMenuItem* findItem = m_parentFrame->GetMenuBar()->FindItem(wxID_FIND, &editMenu);
  if (findItem && editMenu)  {
      //Is there a way to do this without hard-coding the insert position?
      editMenu->Insert(FIND_MENU_POSITION, ID_EDITMENU_FIND_NEXT, _("&Find next...\tF3"), _T(""), wxITEM_NORMAL);
      editMenu->Insert(FIND_MENU_POSITION+1, ID_EDITMENU_FIND_PREVIOUS, _("&Find previous...\tSHIFT+F3"), _T(""), wxITEM_NORMAL);
      editMenu->Delete(findItem);
  }
}

void PasswordSafeSearch::UpdateView()
{
  wxStaticText* statusArea = wxDynamicCast(m_toolbar->FindWindow(ID_FIND_STATUS_AREA), wxStaticText);
  wxASSERT(statusArea);

  if (!m_searchPointer.IsEmpty()) {
    m_parentFrame->SeletItem(*m_searchPointer);
    statusArea->SetLabel(m_searchPointer.GetLabel());
  }
  else {
    statusArea->SetLabel(wxT("No matches found"));
  }
}

void PasswordSafeSearch::FindNext()
{
    if (!m_searchPointer.IsEmpty()) {
      ++m_searchPointer;
      UpdateView();
    }
}

void PasswordSafeSearch::FindPrevious()
{
    if (!m_searchPointer.IsEmpty()) {
      --m_searchPointer;
      UpdateView();
    }
}


/*!
 * wxEVT_COMMAND_TOOL_CLICKED event handler for ID_FIND_CLOSE
 */

void PasswordSafeSearch::OnSearchClose(wxCommandEvent& evt)
{
  m_parentFrame->SetToolBar(NULL);
  m_toolbar->Show(false);

  wxMenu* editMenu = 0;
  wxMenuItem* findNextItem = m_parentFrame->GetMenuBar()->FindItem(ID_EDITMENU_FIND_NEXT, &editMenu);
  wxASSERT(editMenu);
  if (findNextItem)
      editMenu->Delete(findNextItem);

  wxMenuItem* findPreviousItem = m_parentFrame->GetMenuBar()->FindItem(ID_EDITMENU_FIND_PREVIOUS, 0);
  if (findPreviousItem)
      editMenu->Delete(findPreviousItem);

  editMenu->Insert(FIND_MENU_POSITION, wxID_FIND, _("&Find Entry...\tCtrl+F"), _T(""), wxITEM_NORMAL);
}

/*!
 * wxEVT_COMMAND_TOOL_CLICKED event handler for ID_FIND_ADVANCED_OPTIONS
 */
void PasswordSafeSearch::OnAdvancedSearchOptions(wxCommandEvent& evt)
{
  m_searchContext.Reset();
  AdvancedSearchOptionsDlg dlg(m_parentFrame, m_searchContext);
  m_fAdvancedSearch = (dlg.ShowModal() == wxID_OK);
  if (m_searchContext.IsDirty())
      wxMessageBox(wxT("Search is dirty.  Will search from afresh"), wxT("Password Safe Search"));
}

/*!
 * wxEVT_COMMAND_TOOL_CLICKED event handler for ID_FIND_IGNORE_CASE
 */
void PasswordSafeSearch::OnToggleCaseSensitivity(wxCommandEvent& evt)
{
    m_searchContext.SetCaseSensitivity(!m_searchContext->m_fCaseSensitive);
}


/*!
 * Creates the search bar and keeps it hidden
 */
void PasswordSafeSearch::CreateSearchBar()
{
  wxASSERT(m_toolbar == 0);

  m_toolbar = m_parentFrame->CreateToolBar(wxBORDER_NONE | wxTB_BOTTOM | wxTB_HORIZONTAL, wxID_ANY, wxT("SearchBar"));

  m_toolbar->AddTool(ID_FIND_CLOSE, wxT(""), wxBitmap(findclose), wxNullBitmap, wxITEM_NORMAL, wxT("Close Find Bar"));
  m_toolbar->AddControl(new wxTextCtrl(m_toolbar, ID_FIND_EDITBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER));
  m_toolbar->AddTool(ID_FIND_NEXT, wxT(""), wxBitmap(find), wxBitmap(find_disabled), wxITEM_NORMAL, wxT("Find Next"));
  m_toolbar->AddCheckTool(ID_FIND_IGNORE_CASE, wxT(""), wxBitmap(findcase_i), wxBitmap(findcase_s), wxT("Case Insensitive Search"));
  m_toolbar->AddTool(ID_FIND_ADVANCED_OPTIONS, wxT(""), wxBitmap(findadvanced), wxNullBitmap, wxITEM_NORMAL, wxT("Advanced Find Options"));
  m_toolbar->AddTool(ID_FIND_CREATE_REPORT, wxT(""), wxBitmap(findreport), wxNullBitmap, wxITEM_NORMAL, wxT("Create report of previous Find search"));
  m_toolbar->AddTool(ID_FIND_CLEAR, wxT(""), wxBitmap(findclear), wxNullBitmap, wxITEM_NORMAL, wxT("Clear Find"));
  m_toolbar->AddControl(new wxStaticText(m_toolbar, ID_FIND_STATUS_AREA, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY));

  if (!m_toolbar->Realize())
    wxMessageBox(wxT("SearcBar::Realize failed"), wxT("Password Safe"));
 
  m_toolbar->PushEventHandler(this);
}

/*!
 * Called when user clicks Find from Edit menu, or presses Ctrl-F
 */
void PasswordSafeSearch::Activate(void)
{
  if (!m_toolbar)
    CreateSearchBar();
  else {
    m_parentFrame->SetToolBar(m_toolbar);
    m_toolbar->Show(true);
  }

  wxASSERT(m_toolbar);

  m_toolbar->FindControl(ID_FIND_EDITBOX)->SetFocus();
}

void PasswordSafeSearch::FindMatches(const StringX& searchText, bool fCaseSensitive, SearchPointer& searchPtr)
{
  searchPtr.Clear();
  //As per original Windows code, default search is for all text fields
  CItemData::FieldBits bsFields;
  bsFields.set();

  return FindMatches(searchText, fCaseSensitive, searchPtr, bsFields, false, wxEmptyString, CItemData::END, PWSMatch::MR_INVALID);
}

bool FindNoCase( const StringX& src, const StringX& dest)
{
    StringX srcLower = src;
    ToLower(srcLower);

    StringX destLower = dest;
    ToLower(destLower);

    return destLower.find(srcLower) != StringX::npos;
}

void PasswordSafeSearch::FindMatches(const StringX& searchText, bool fCaseSensitive, SearchPointer& searchPtr,
                                       const CItemData::FieldBits& bsFields, bool fUseSubgroups, const wxString& subgroupText,
                                       CItemData::FieldType subgroupObject, PWSMatch::MatchRule subgroupFunction)
{
  if (searchText.empty())
      return;
  
  searchPtr.Clear();

  typedef StringX (CItemData::*ItemDataFuncT)() const;

  struct {
      CItemData::FieldType type;
      ItemDataFuncT        func;
  } ItemDataFields[] = {  {CItemData::GROUP,     &CItemData::GetGroup},
                          {CItemData::TITLE,     &CItemData::GetTitle},
                          {CItemData::USER,      &CItemData::GetUser},
                          {CItemData::PASSWORD,  &CItemData::GetPassword},
//                        {CItemData::NOTES,     &CItemData::GetNotes},
                          {CItemData::URL,       &CItemData::GetURL},
                          {CItemData::EMAIL,     &CItemData::GetEmail},
                          {CItemData::RUNCMD,    &CItemData::GetRunCommand},
                          {CItemData::AUTOTYPE,  &CItemData::GetAutoType},
                          {CItemData::XTIME_INT, &CItemData::GetXTimeInt},
 
                      };

  for ( ItemListConstIter itr = m_parentFrame->GetEntryIter(); itr != m_parentFrame->GetEntryEndIter(); ++itr) {
    bool found = false;
    for (size_t idx = 0; idx < NumberOf(ItemDataFields) && !found; ++idx) {
      if (bsFields.test(ItemDataFields[idx].type)) {
          const StringX str = (itr->second.*ItemDataFields[idx].func)();
          found = fCaseSensitive? str.find_first_of(searchText) != StringX::npos: FindNoCase(searchText, str);
      }
    }

    if (!found && bsFields.test(CItemData::NOTES)) {
        StringX str = itr->second.GetNotes();
        found = fCaseSensitive? str.find_first_of(searchText) != StringX::npos: FindNoCase(searchText, str);
    }

    if (!found && bsFields.test(CItemData::PWHIST)) {
    }

    if (found) {
        uuid_array_t uuid;
        itr->second.GetUUID(uuid);
        searchPtr.Add(CUUIDGen(uuid));
    }
  }
}

/////////////////////////////////////////////////
// SearchPointer class definition
SearchPointer& SearchPointer::operator++()
{ //prefix operator, to prevent copying itself
  if (!m_indices.empty()) {
    m_currentIndex++;
    if (m_currentIndex == m_indices.end()) {
      m_currentIndex = m_indices.begin();
      m_label = wxT("Search hit bottom, continuing at top");
    }
  }
  else
    m_currentIndex = m_indices.end();

  return *this;
}

SearchPointer& SearchPointer::operator--()
{ //prefix operator, to prevent copying itself
  if (!m_indices.empty()) {
    if (m_currentIndex == m_indices.begin()) {
      m_currentIndex = --m_indices.end();
      m_label = wxT("Search hit top, continuing at bottom");
    }
    else {
      m_currentIndex--;
    }
  }
  else
    m_currentIndex = m_indices.end();

  return *this;
}
