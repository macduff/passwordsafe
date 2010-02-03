/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// file MainEdit.cpp
//
// Edit-related methods of DboxMain
//-----------------------------------------------------------------------------

#include "PasswordSafe.h"
#include "ThisMfcApp.h"
#include "GeneralMsgBox.h"
#include "DDSupport.h"
#include "DboxMain.h"
#include "AddEdit_PropertySheet.h"
#include "ConfirmDeleteDlg.h"
#include "QuerySetDef.h"
#include "EditShortcutDlg.h"
#include "KeySend.h"
#include "ClearQuestionDlg.h"
#include "CreateShortcutDlg.h"
#include "PasswordSubsetDlg.h"

#include "corelib/pwsprefs.h"
#include "corelib/PWSAuxParse.h"
#include "corelib/Command.h"

#include "os/dir.h"
#include "os/run.h"

#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>
#include <vector>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//Add an item
void DboxMain::OnAdd()
{
  CItemData ci;
  ci.CreateUUID();

  CAddEdit_PropertySheet add_entry_psh(IDS_ADDENTRY, this, &m_core, NULL, &ci, L""); 

  PWSprefs *prefs = PWSprefs::GetInstance();
  if (prefs->GetPref(PWSprefs::UseDefaultUser) == TRUE) {
    add_entry_psh.SetUsername(prefs->GetPref(PWSprefs::DefaultUsername).c_str());
  }

  // m_TreeViewGroup may be set by OnContextMenu, if not, try to grok it
  if (m_TreeViewGroup.empty()) {
    CItemData *pci = NULL;
    if (m_ctlItemTree.IsWindowVisible()) { // tree view
      HTREEITEM ti = m_ctlItemTree.GetSelectedItem();
      if (ti != NULL) { // if anything selected
        pci = (CItemData *)m_ctlItemTree.GetItemData(ti);
        if (pci != NULL) { // leaf selected
          m_TreeViewGroup = pci->GetGroup();
        } else { // node selected
          m_TreeViewGroup = m_ctlItemTree.GetGroup(ti);
        }
      }
    } else { // list view
      // XXX TBD - get group name of currently selected list entry
    }
  }
  add_entry_psh.SetGroup(m_TreeViewGroup);
  m_TreeViewGroup = L""; // for next time

  // Remove Apply button
  add_entry_psh.m_psh.dwFlags |= PSH_NOAPPLYNOW;

  INT_PTR rc = add_entry_psh.DoModal();

  if (rc == IDOK) {
    bool bWasEmpty = m_core.GetNumEntries() == 0;
    CSecString &sxUsername = add_entry_psh.GetUsername();

    // Initialise a copy of the DB preferences
    prefs->SetUpCopyDBprefs();
    // Get old DB preferences String value (from current preferences)
    const StringX sxOldDBPrefsString(prefs->Store());

    //Check if they wish to set a default username
    if (prefs->GetPref(PWSprefs::UseDefaultUser) == FALSE &&
        (prefs->GetPref(PWSprefs::QuerySetDef)) &&
        (!sxUsername.IsEmpty())) {
      CQuerySetDef defDlg(this);
      defDlg.m_message.Format(IDS_SETUSERNAME, (const CString&)sxUsername);
      INT_PTR rc2 = defDlg.DoModal();
      if (rc2 == IDOK) {
        // Update Copy with new values
        prefs->SetPref(PWSprefs::UseDefaultUser, true, true);
        prefs->SetPref(PWSprefs::DefaultUsername, sxUsername, true);
      }
    }

    // We can't have "no usernames displayed" in Tree if this new entry has a
    // title the same as a group in this group as they look the same.
    StringX sxGTUs;  // Not used in a single call

    if (prefs->GetPref(PWSprefs::ShowUsernameInTree) == FALSE &&
        m_core.CheckTitleSameAsGroup(&ci, sxGTUs) > 0) {
      CGeneralMsgBox gmb;
      CString cs_title, cs_msg;
      cs_title.LoadString(IDS_MUSTHAVEUSERNAMES0);
      CString cs2(MAKEINTRESOURCE(IDS_MUSTHAVEUSERNAMES1));
#ifdef NOTYET
      // XXX Update this for changed IDS_MUSTHAVEUSERNAMES3 format after Formal 3.21 release
#else /* Use this for 3.21 */
      cs_msg.Format(IDS_MUSTHAVEUSERNAMES3, cs2);
#endif /* NOTYET */
      gmb.MessageBox(cs_msg, cs_title, MB_OK);
      // Update Copy with new values
      prefs->SetPref(PWSprefs::ShowUsernameInTree, true, true);
    }

    // Set new DB preferences String value (from Copy)
    StringX sxNewDBPrefsString(prefs->Store(true));

    MultiCommands *pmulticmds = MultiCommands::Create(&m_core);
    if (sxOldDBPrefsString != sxNewDBPrefsString) {
      Command *pcmd1 = UpdateGUICommand::Create(&m_core,
                                                UpdateGUICommand::WN_UNDO,
                                                UpdateGUICommand::GUI_REFRESH_TREE);
      pmulticmds->Add(pcmd1);

      Command *pcmd2 = DBPrefsCommand::Create(&m_core, sxNewDBPrefsString);
      pmulticmds->Add(pcmd2);
    }

    DisplayInfo *pdi = new DisplayInfo;
    ci.SetDisplayInfo(pdi); // DisplayInfo values will be set later

    // Add the entry
    ci.SetStatus(CItemData::ES_ADDED);

    Command *pcmd;
    if (add_entry_psh.GetIBasedata() == 0) {
      pcmd = AddEntryCommand::Create(&m_core, ci);
    } else { // creating an alias
      uuid_array_t base_uuid;
      memcpy(base_uuid, add_entry_psh.GetBaseUUID(), sizeof(base_uuid));
      pcmd = AddEntryCommand::Create(&m_core, ci, base_uuid);
    }
    pmulticmds->Add(pcmd);

    if (sxOldDBPrefsString != sxNewDBPrefsString) {
      Command *pcmd3 = UpdateGUICommand::Create(&m_core,
                                                UpdateGUICommand::WN_EXECUTE_REDO,
                                                UpdateGUICommand::GUI_REFRESH_TREE);
      pmulticmds->Add(pcmd3);
    }
    Execute(pmulticmds); // Either AddEntry or MultiCommands

    // Update Toolbar for this new entry
    m_ctlItemList.SetItemState(pdi->list_index, LVIS_SELECTED, LVIS_SELECTED);
    m_ctlItemTree.SelectItem(pdi->tree_item);
    UpdateToolBarForSelectedItem(&ci);

    if (m_core.GetNumEntries() == 1) {
      // For some reason, when adding the first entry, it is not visible!
      m_ctlItemTree.SetRedraw(TRUE);
    }

    SortListView();
    m_ctlItemList.SetFocus();
    SetChanged(Data);

    ChangeOkUpdate();
    uuid_array_t uuid;
    ci.GetUUID(uuid);
    m_RUEList.AddRUEntry(uuid);
    // May need to update menu/toolbar if database was previously empty
    if (bWasEmpty)
      UpdateMenuAndToolBar(m_bOpen);
  }
}

//Add a shortcut
void DboxMain::OnCreateShortcut()
{
   // disable in read-only mode or nothing selected
  if (m_core.IsReadOnly() || SelItemOk() != TRUE)
    return;

  CItemData *pci = getSelectedItem();
  ASSERT(pci != NULL);

  CCreateShortcutDlg dlg_createshortcut(this, pci->GetGroup(), 
    pci->GetTitle(), pci->GetUser());

  PWSprefs *prefs = PWSprefs::GetInstance();
  if (prefs->GetPref(PWSprefs::UseDefaultUser) == TRUE) {
    dlg_createshortcut.m_username = prefs->GetPref(PWSprefs::DefaultUsername).c_str();
  }

  INT_PTR rc = dlg_createshortcut.DoModal();

  if (rc == IDOK) {
    //Check if they wish to set a default username
    if (prefs->GetPref(PWSprefs::UseDefaultUser) == FALSE &&
        (prefs->GetPref(PWSprefs::QuerySetDef)) &&
        (!dlg_createshortcut.m_username.IsEmpty())) {
      CQuerySetDef defDlg(this);
      defDlg.m_message.Format(IDS_SETUSERNAME, (const CString&)dlg_createshortcut.m_username);
      INT_PTR rc2 = defDlg.DoModal();
      if (rc2 == IDOK) {
        // Initialise a copy of the DB preferences
        prefs->SetUpCopyDBprefs();
        // Update Copy with new values
        prefs->SetPref(PWSprefs::UseDefaultUser, true);
        prefs->SetPref(PWSprefs::DefaultUsername, dlg_createshortcut.m_username, true);
        // Get old DB preferences String value (from current preferences) & 
        // new DB preferences String value (from Copy)
        const StringX sxOldDBPrefsString(prefs->Store());
        StringX sxNewDBPrefsString(prefs->Store(true));
        if (sxOldDBPrefsString != sxNewDBPrefsString) {
          // Need to integrate the following Command into others in this routine
          // Left for Rony !!! :-)
          // Command *pcmd = DBPrefsCommand::Create(&m_core, sxNewDBPrefs);
        }
      }
    }
    if (dlg_createshortcut.m_username.IsEmpty() && 
        prefs->GetPref(PWSprefs::UseDefaultUser) == TRUE)
      dlg_createshortcut.m_username = prefs->GetPref(PWSprefs::DefaultUsername).c_str();

    CreateShortcutEntry(pci, dlg_createshortcut.m_group, 
                        dlg_createshortcut.m_title, 
                        dlg_createshortcut.m_username);
  }
}
void DboxMain::CreateShortcutEntry(CItemData *pci, const StringX &cs_group,
                                   const StringX &cs_title, const StringX &cs_user)
{
  uuid_array_t base_uuid, shortcut_uuid;

  ASSERT(pci != NULL);
  pci->GetUUID(base_uuid);

  CItemData ci_temp;
  ci_temp.CreateUUID();
  ci_temp.GetUUID(shortcut_uuid);
  ci_temp.SetGroup(cs_group);
  ci_temp.SetTitle(cs_title);
  ci_temp.SetUser(cs_user);
  ci_temp.SetPassword(L"[Shortcut]");
  ci_temp.SetShortcut();

  time_t t;
  time(&t);
  ci_temp.SetCTime(t);
  ci_temp.SetXTime((time_t)0);
  ci_temp.SetStatus(CItemData::ES_ADDED);

  Execute(AddEntryCommand::Create(&m_core, ci_temp, base_uuid));

  // Update base item's graphic
  ItemListIter iter = m_core.Find(base_uuid);
  if (iter != End())
    UpdateEntryImages(iter->second);

  m_ctlItemList.SetFocus();
  SetChanged(Data);
  ChangeOkUpdate();
  m_RUEList.AddRUEntry(shortcut_uuid);
}

//Add a group (tree view only)
void DboxMain::OnAddGroup()
{
  if (m_core.IsReadOnly()) // disable in read-only mode
    return;

  if (m_ctlItemTree.IsWindowVisible()) {
    // This can be reached by right clicking over an existing group node
    // or by clicking over "whitespace".
    // If the former, add a child node to the current one
    // If the latter, add to root.
    CString cmys_text(MAKEINTRESOURCE(IDS_NEWGROUP));
    CItemData *pci = getSelectedItem();
    if (pci != NULL) {
      m_TreeViewGroup = pci->GetGroup();
    }

    if (m_TreeViewGroup.empty())
      m_TreeViewGroup = cmys_text;
    else
      m_TreeViewGroup += L"." + cmys_text;

    // If group is there - make unique
    StringX s_copy(m_TreeViewGroup);
    bool bAlreadyExists(true);
    HTREEITEM newGroup;

    int i = 1;
    do {
      newGroup = m_ctlItemTree.AddGroup(s_copy.c_str(), bAlreadyExists);
      i++;
      // Format as per Windows Exlorer "New Folder"/"New Folder (n)"
      // where if 'n' present, it starts from 2
      Format(s_copy, L"%s (%d)", m_TreeViewGroup.c_str(), i);
    } while (bAlreadyExists);

    m_ctlItemTree.SelectItem(newGroup);
    m_TreeViewGroup = L""; // for next time

    // Needed by PWTreeCtrl::OnEndLabelEdit in case user doesn't change the group name
    m_bInAddGroup = true;
    m_ctlItemTree.EditLabel(newGroup);
  }
}

void DboxMain::OnDelete()
{
  // Check preconditions, possibly prompt user for confirmation, then call Delete()
  // to do the heavy lifting.
  if (m_core.GetNumEntries() == 0) // easiest way to avoid asking stupid questions...
    return;

  bool bAskForDeleteGroupConfirmation = false;
  bool dodelete = true;
  int num_children = 0;

  // Find number of child items, ask for confirmation if > 0
  if (m_ctlItemTree.IsWindowVisible()) {
    HTREEITEM hStartItem = m_ctlItemTree.GetSelectedItem();
    if (hStartItem != NULL) {
      if (m_ctlItemTree.GetItemData(hStartItem) == NULL) {  // group node
        bAskForDeleteGroupConfirmation = true; // ALWAYS ask if deleting a group
        if (m_ctlItemTree.ItemHasChildren(hStartItem)) {
          num_children = CountChildren(hStartItem);
        }  // if has children
      }
    }
  }

  // Confirm whether to delete the item
  if (bAskForDeleteGroupConfirmation) {
    CConfirmDeleteDlg deleteDlg(this, num_children);
    INT_PTR rc = deleteDlg.DoModal();
    if (rc == IDCANCEL) {
      dodelete = false;
    }
  }

  if (dodelete)
    Delete();
}

void DboxMain::Delete()
{
  // "Top level" element delete:
  // 1. Sets up Command mechanism
  // 2. Calls group or element Delete method, as appropriate
  // 3. Executes, updates UI.

  CItemData *pci = getSelectedItem();
  Command *pcmd = NULL;

  if (pci != NULL)
    pcmd = Delete(pci); // single entry
  else if (m_ctlItemTree.IsWindowVisible()) {
    HTREEITEM ti = m_ctlItemTree.GetSelectedItem();
    // Deleting a Group
    pcmd = Delete(ti);
  }
  if (pcmd != NULL) {
    Execute(pcmd);
    if (m_bFilterActive)
      RefreshViews();
    ChangeOkUpdate();
  }
}

Command *DboxMain::Delete(const CItemData *pci)
{
  // Delete a single item of any type:
  // Normal, base, alias, shortcut...
  ASSERT(pci != NULL);

  // ConfirmDelete asks for user confirmation
  // when deleting a shortcut or alias base.
  // Otherwise it just return true
  if (m_core.ConfirmDelete(pci))
    return DeleteEntryCommand::Create(&m_core, *pci);
  else
    return NULL;
}

// Functor for find_if to find the group name associated with a HTREEITEM
struct FindGroupFromHTREEITEM {
  FindGroupFromHTREEITEM(HTREEITEM& ti) : m_ti(ti) {}
  bool operator()(std::pair<StringX, HTREEITEM> const & p) const
  {
    return (p.second  == m_ti);
  }

  HTREEITEM m_ti;
};

Command *DboxMain::Delete(HTREEITEM ti)
{
  // Delete a group
  // Create a multicommand, iterate over tree's children,
  // throw everything into multicommand and be done with it.
  // Of course, this may recurse...

  if (ti == NULL)
    return NULL; // bad bottoming out of recursion?
  if (m_ctlItemTree.IsLeaf(ti)) {
    CItemData *pci = (CItemData *)m_ctlItemTree.GetItemData(ti);
    return Delete(pci); // normal bottom out of recursion
  }

  // Here if we have a bona fida group
  ASSERT(ti != NULL && !m_ctlItemTree.IsLeaf(ti));
  MultiCommands *pmulti_cmd = MultiCommands::Create(&m_core);
  
  HTREEITEM cti = m_ctlItemTree.GetChildItem(ti);

  while (cti != NULL) {
    CItemData *pci = (CItemData *)m_ctlItemTree.GetItemData(cti);
    if (pci != NULL)
      pmulti_cmd->Add(Delete(pci));
    else
      pmulti_cmd->Add(Delete(cti)); // subgroup!

    cti = m_ctlItemTree.GetNextItem(cti, TVGN_NEXT);
  }

  m_ctlItemTree.Invalidate(); // so whole view will be refereshed

  HTREEITEM parent = m_ctlItemTree.GetParentItem(ti);
  m_ctlItemTree.DeleteItem(ti);
  m_ctlItemTree.SelectItem(parent);
  m_TreeViewGroup = L"";
  return pmulti_cmd;
}

void DboxMain::OnRename()
{
  if (m_core.IsReadOnly()) // disable in read-only mode
    return;

  // Renaming is only allowed while in Tree mode.
  if (m_ctlItemTree.IsWindowVisible()) {
    HTREEITEM hItem = m_ctlItemTree.GetSelectedItem();
    if (hItem != NULL) {
      m_ctlItemTree.EditLabel(hItem);
      if (m_bFilterActive && m_ctlItemTree.WasLabelEdited())
        RefreshViews();
    }
  }
}

void DboxMain::OnEdit()
{
  // Note that Edit is also used for just viewing - don't want to disable
  // viewing in read-only mode
  if (SelItemOk() == TRUE) {
    CItemData *pci = getSelectedItem();
    ASSERT(pci != NULL);

    if (pci->IsShortcut())
      EditShortcut(pci);
    else
      EditItem(pci);
  } else {
    // entry item not selected - perhaps here on Enter on tree item?
    // perhaps not the most elegant solution to improving non-mouse use,
    // but it works. If anyone knows how Enter/Return gets mapped to OnEdit,
    // let me know...
    CItemData *pci_node(NULL);
    if (m_ctlItemTree.IsWindowVisible()) { // tree view
      HTREEITEM ti = m_ctlItemTree.GetSelectedItem();
      if (ti != NULL) { // if anything selected
        pci_node = (CItemData *)m_ctlItemTree.GetItemData(ti);
        if (pci_node == NULL) { // node selected
          m_ctlItemTree.Expand(ti, TVE_TOGGLE);
        }
      }
    }
  }
}

bool DboxMain::EditItem(CItemData *pci, PWScore *pcore)
{
  // Note: In all but one circumstance, pcore == NULL, implying edit of an entry
  // in the current database.
  // The one except is when the user wishes to View an entry from the comparison
  // database via "CompareResultsDlg" (the Compare Database results dialog).
  // Note: In this instance, the comparison database is R/O and hence the user may
  // only View these entries and any database preferences can be obtain from the
  // copy of the header within that instance of PWScore - see below when changing the
  // default username.
  if (pcore == NULL)
    pcore = &m_core;

  CItemData ci_edit(*pci);

  // As pci may be invalidated if database is Locked while in this routine, 
  // we use a clone
  CItemData ci_original(*pci);
  pci = NULL; // Set to NULL - should use ci_original

  const UINT uicaller = pcore->IsReadOnly() ? IDS_VIEWENTRY : IDS_EDITENTRY;
  CAddEdit_PropertySheet edit_entry_psh(uicaller, this, pcore,
                                       &ci_original, &ci_edit, pcore->GetCurFile()); 

  // List might be cleared if db locked.
  // Need to take care that we handle a rebuilt list.
  bool bIsDefUserSet;
  StringX sxDefUserValue;
  PWSprefs *prefs = PWSprefs::GetInstance();
  if (pcore == &m_core) {
    // As it is us, get values from actually current 
    bIsDefUserSet = prefs->GetPref(PWSprefs::UseDefaultUser) ? TRUE : FALSE;
    sxDefUserValue = prefs->GetPref(PWSprefs::DefaultUsername);
  } else {
    // Need to get Default User value from this core's preferences stored in its header.
    // Since this core is R-O, the values in the header will not have been changed
    StringX sxDBPreferences(pcore->GetDBPreferences());
    prefs->GetDefaultUserInfo(sxDBPreferences, bIsDefUserSet, sxDefUserValue);
  }
  if (bIsDefUserSet)
    edit_entry_psh.SetDefUsername(sxDefUserValue.c_str());

  // Don't show Apply button if in R-O mode (View)
  if (uicaller == IDS_VIEWENTRY)
    edit_entry_psh.m_psh.dwFlags |= PSH_NOAPPLYNOW;

  INT_PTR rc = edit_entry_psh.DoModal();

  if (rc == IDOK && uicaller == IDS_EDITENTRY && edit_entry_psh.IsEntryModified()) {
    // Process user's changes.
    UpdateEntry(&edit_entry_psh);
    return true;
  } // rc == IDOK
  return false;
}

LRESULT DboxMain::OnApplyEditChanges(WPARAM wParam, LPARAM lParam)
{
  // Called if user does 'Apply' on the Add/Edit property sheet via
  // Windows Message WM_EDIT_APPLY
  UNREFERENCED_PARAMETER(lParam);
  CAddEdit_PropertySheet *pentry_psh = (CAddEdit_PropertySheet *)wParam;
  UpdateEntry(pentry_psh);
  return 0L;
}

void DboxMain::UpdateEntry(CAddEdit_PropertySheet *pentry_psh)
{
  // Called by EditItem on return from Edit but
  // also called if user does 'Apply' on the Add/Edit property sheet via
  // Windows Message WM_EDIT_APPLY

  PWScore *pcore = pentry_psh->GetCore();
  CItemData *pci_original = pentry_psh->GetOriginalCI();
  CItemData *pci_new = pentry_psh->GetNewCI();

  // Most of the following code handles special cases of alias/shortcut/base
  // But the common case is simply to replace the original entry
  // with a new one having the edited values and the same uuid.
  MultiCommands *pmulticmds = MultiCommands::Create(pcore);


  // We can't have "no usernames displayed" in Tree View if this new entry has a
  // title the same as a group in this group - they look the same!
  // Initialise a copy of the DB preferences
  PWSprefs *prefs = PWSprefs::GetInstance();
  prefs->SetUpCopyDBprefs();

  // Get old DB preferences String value (from current preferences)
  const StringX sxOldDBPrefsString(prefs->Store());
  StringX sxGTUs;  // Not used in a single call

  if (prefs->GetPref(PWSprefs::ShowUsernameInTree) == FALSE &&
     (pci_original->GetGroup() != pci_new->GetGroup() ||
      pci_original->GetTitle() != pci_new->GetTitle()) &&
      m_core.CheckTitleSameAsGroup(pci_new, sxGTUs) > 0) {
    CGeneralMsgBox gmb;
    CString cs_title, cs_msg;
    cs_title.LoadString(IDS_MUSTHAVEUSERNAMES0);
    CString cs2(MAKEINTRESOURCE(IDS_MUSTHAVEUSERNAMES1));
#ifdef NOTYET
      // XXX Update this for changed IDS_MUSTHAVEUSERNAMES3 format after Formal 3.21 release
#else /* Use this for 3.21 */
    cs_msg.Format(IDS_MUSTHAVEUSERNAMES3, cs2);
#endif /* NOTYET */
    gmb.MessageBox(cs_msg, cs_title, MB_OK);
    // Update Copy with new values
    prefs->SetPref(PWSprefs::ShowUsernameInTree, true, true);
  }
  
  // Set new DB preferences String value (from Copy)
  StringX sxNewDBPrefsString(prefs->Store(true));
  if (sxOldDBPrefsString != sxNewDBPrefsString) {
    Command *pcmd1 = UpdateGUICommand::Create(pcore,
                                              UpdateGUICommand::WN_UNDO,
                                              UpdateGUICommand::GUI_REFRESH_TREE);
    pmulticmds->Add(pcmd1);

    Command *pcmd2 = DBPrefsCommand::Create(pcore, sxNewDBPrefsString);
    pmulticmds->Add(pcmd2);
  }

  StringX newPassword = pci_new->GetPassword();
  uuid_array_t original_uuid = {'\0'}, original_base_uuid = {'\0'}, new_base_uuid = {'\0'};
  memcpy(new_base_uuid, pentry_psh->GetBaseUUID(), sizeof(new_base_uuid));

  ItemListIter iter;
  if (pentry_psh->GetOriginalEntrytype() == CItemData::ET_NORMAL &&
      pci_original->GetPassword() != newPassword) {
    // Original was a 'normal' entry and the password has changed
    if (pentry_psh->GetIBasedata() > 0) { // Now an alias
      Command *pcmd = AddDependentEntryCommand::Create(pcore, new_base_uuid, 
                                                       original_uuid,
                                                       CItemData::ET_ALIAS);
      pmulticmds->Add(pcmd);
      pci_new->SetPassword(L"[Alias]");
      pci_new->SetAlias();
    } else { // Still 'normal'
      pci_new->SetPassword(newPassword);
      pci_new->SetNormal();
    }
  } // Normal entry, password changed

  if (pentry_psh->GetOriginalEntrytype() == CItemData::ET_ALIAS) {
    // Original was an alias - delete it from multimap
    // RemoveDependentEntry also resets base to normal if no more dependents
    Command *pcmd = RemoveDependentEntryCommand::Create(pcore,
                                                        original_base_uuid,
                                                        original_uuid,
                                                        CItemData::ET_ALIAS);
    pmulticmds->Add(pcmd);
    if (newPassword == pentry_psh->GetBase()) {
      // Password (i.e. base) unchanged - put it back
      Command *pcmd = AddDependentEntryCommand::Create(pcore,
                                                       original_base_uuid, 
                                                       original_uuid,
                                                       CItemData::ET_ALIAS);
      pmulticmds->Add(pcmd);
    } else { // Password changed
      // Password changed so might be an alias of another entry!
      // Could also be the same entry i.e. [:t:] == [t] !
      if (pentry_psh->GetIBasedata() > 0) { // Still an alias
        Command *pcmd = AddDependentEntryCommand::Create(pcore, new_base_uuid, 
                                                         original_uuid,
                                                         CItemData::ET_ALIAS);
        pmulticmds->Add(pcmd);
        pci_new->SetPassword(L"[Alias]");
        pci_new->SetAlias();
      } else { // No longer an alias
        pci_new->SetPassword(newPassword);
        pci_new->SetNormal();
      }
    } // Password changed
  } // Alias

  if (pentry_psh->GetOriginalEntrytype() == CItemData::ET_ALIASBASE &&
      pci_original->GetPassword() != newPassword) {
    // Original was a base but might now be an alias of another entry!
    if (pentry_psh->GetIBasedata() > 0) {
      // Now an alias
      // Make this one an alias
      Command *pcmd1 = AddDependentEntryCommand::Create(pcore, new_base_uuid, 
                                                        original_uuid,
                                                        CItemData::ET_ALIAS);
      pmulticmds->Add(pcmd1);
      pci_new->SetPassword(L"[Alias]");
      pci_new->SetAlias();
      // Move old aliases across
      Command *pcmd2 = MoveDependentEntriesCommand::Create(pcore,
                                                           original_uuid, 
                                                           new_base_uuid,
                                                           CItemData::ET_ALIAS);
      pmulticmds->Add(pcmd2);
    } else { // Still a base entry but with a new password
      pci_new->SetPassword(newPassword);
      pci_new->SetAliasBase();
    }
  } // AliasBase with password changed

  // Update old base...
  iter = pcore->Find(original_base_uuid);
  if (iter != End())
    UpdateEntryImages(iter->second);

  // ... and the new base entry (only if different from the old one)
  if (CUUIDGen(new_base_uuid) != CUUIDGen(original_base_uuid)) {
    iter = pcore->Find(new_base_uuid);
    if (iter != End())
      UpdateEntryImages(iter->second);
  }

  if (pci_new->IsDependent()) {
    pci_new->SetXTime((time_t)0);
    pci_new->SetPWPolicy(L"");
  }

  pci_new->SetStatus(CItemData::ES_MODIFIED);

  Command *pcmd = EditEntryCommand::Create(pcore, *(pci_original), 
                                                  *(pci_new));
  pmulticmds->Add(pcmd);

  if (sxOldDBPrefsString != sxNewDBPrefsString) {
    Command *pcmd3 = UpdateGUICommand::Create(&m_core,
                                              UpdateGUICommand::WN_EXECUTE_REDO,
                                              UpdateGUICommand::GUI_REFRESH_TREE);
    pmulticmds->Add(pcmd3);
  }

  Execute(pmulticmds, pcore);
  SetChanged(Data);

  ChangeOkUpdate();
  // Order may have changed as a result of edit
  m_ctlItemTree.SortTree(TVI_ROOT);
  SortListView();

  // reselect entry, wherever it may be
  iter = m_core.Find(original_uuid);
  if (iter != End()) {
    DisplayInfo *pdi = (DisplayInfo *)iter->second.GetDisplayInfo();
    SelectEntry(pdi->list_index);
  }

  short sh_odca, sh_ndca;
  pci_original->GetDCA(sh_odca);
  pci_new->GetDCA(sh_ndca);
  if (sh_odca != sh_ndca)
    SetDCAText(pci_new);

  UpdateToolBarForSelectedItem(pci_new);
}

bool DboxMain::EditShortcut(CItemData *pci, PWScore *pcore)
{
  if (pcore == NULL)
    pcore = &m_core;

  // List might be cleared if db locked.
  // Need to take care that we handle a rebuilt list.
  CItemData ci_edit(*pci);
  // As pci may be invalidated if database is Locked while in this routine, 
  // we use a clone
  CItemData ci_original(*pci);
  pci = NULL; // Set to NULL - should use ci_original

  uuid_array_t entry_uuid;
  ci_original.GetUUID(entry_uuid); // for later use

  CItemData *pbci = GetBaseEntry(&ci_original);

  CEditShortcutDlg dlg_editshortcut(&ci_edit, this, pbci->GetGroup(),
                                    pbci->GetTitle(), pbci->GetUser());

  // Need to get Default User value from this core's preferences stored in its header, 
  // which are not necessarily in our PWSprefs::GetInstance !
  bool bIsDefUserSet;
  StringX sxDefUserValue, sxDBPreferences;
  sxDBPreferences = pcore->GetDBPreferences();
  PWSprefs::GetInstance()->GetDefaultUserInfo(sxDBPreferences, bIsDefUserSet,
                                              sxDefUserValue);
  if (bIsDefUserSet)
    dlg_editshortcut.m_defusername = sxDefUserValue.c_str();

  dlg_editshortcut.m_Edit_IsReadOnly = pcore->IsReadOnly();

  INT_PTR rc = dlg_editshortcut.DoModal();

  if (rc == IDOK && dlg_editshortcut.IsEntryModified()) {
    // Out with the old, in with the new
    // User cannot change a shortcut entry to anything else!
    ItemListIter listpos = Find(entry_uuid);
    ASSERT(listpos != pcore->GetEntryEndIter());
    CItemData oldElem = GetEntryAt(listpos);
    // ci_edit's displayinfo will have been deleted if
    // application "locked" (Cleared list)
    DisplayInfo *pdi_new = new DisplayInfo;
    ci_edit.SetDisplayInfo(pdi_new);
    ci_edit.SetXTime((time_t)0);
    ci_edit.SetStatus(CItemData::ES_MODIFIED);

    Command *pcmd = EditEntryCommand::Create(pcore, ci_original, ci_edit);
    Execute(pcmd, pcore);
    SetChanged(Data);

    // DisplayInfo's copied and changed, get up-to-date version
    pdi_new = static_cast<DisplayInfo *>(pcore->GetEntry(pcore->Find(entry_uuid)).GetDisplayInfo());
    rc = SelectEntry(pdi_new->list_index);

    if (rc == 0) {
      SelectEntry(m_ctlItemList.GetItemCount() - 1);
    }
    ChangeOkUpdate();

    UpdateToolBarForSelectedItem(&ci_edit);
    return true;
  } // rc == IDOK
  return false;
}

// Duplicate selected entry but make title unique
void DboxMain::OnDuplicateEntry()
{
  if (m_core.IsReadOnly()) // disable in read-only mode
    return;

  if (SelItemOk() == TRUE) {
    CItemData *pci = getSelectedItem();
    ASSERT(pci != NULL);
    DisplayInfo *pdi = (DisplayInfo *)pci->GetDisplayInfo();
    ASSERT(pdi != NULL);

    // Get information from current selected entry
    const StringX ci2_group = pci->GetGroup();
    const StringX ci2_user = pci->GetUser();
    const StringX ci2_title0 = pci->GetTitle();
    StringX ci2_title;

    // Find a unique "Title"
    ItemListConstIter listpos;
    int i = 0;
    CString s_copy;
    do {
      i++;
      s_copy.Format(IDS_COPYNUMBER, i);
      ci2_title = ci2_title0 + LPCWSTR(s_copy);
      listpos = m_core.Find(ci2_group, ci2_title, ci2_user);
    } while (listpos != m_core.GetEntryEndIter());

    // Set up new entry
    CItemData ci2(*pci);
    ci2.SetDisplayInfo(NULL);
    ci2.CreateUUID();
    ci2.SetGroup(ci2_group);
    ci2.SetTitle(ci2_title);
    ci2.SetUser(ci2_user);
    ci2.SetStatus(CItemData::ES_ADDED);

    Command *pcmd = NULL;
    if (pci->IsDependent()) {
      if (pci->IsAlias()) {
        ci2.SetAlias();
      } else { // shortcut
        ci2.SetShortcut();
      }

      CItemData *pbci = GetBaseEntry(pci);
      if (pbci != NULL) {
        uuid_array_t base_uuid;
        pbci->GetUUID(base_uuid);
        StringX cs_tmp;
        cs_tmp = L"[" +
          pbci->GetGroup() + L":" +
          pbci->GetTitle() + L":" +
          pbci->GetUser()  + L"]";
        ci2.SetPassword(cs_tmp);
        pcmd = AddEntryCommand::Create(&m_core, ci2, base_uuid);
      }
    } else { // not alias or shortcut
      ci2.SetNormal();
      pcmd = AddEntryCommand::Create(&m_core, ci2);
    }

    Execute(pcmd);

    pdi->list_index = -1; // so that InsertItemIntoGUITreeList will set new values

    uuid_array_t uuid;
    ci2.GetUUID(uuid);
    ItemListIter iter = m_core.Find(uuid);
    ASSERT(iter != m_core.GetEntryEndIter());

    InsertItemIntoGUITreeList(m_core.GetEntry(iter));
    FixListIndexes();
    SetChanged(Data);

    int rc = SelectEntry(pdi->list_index);
    if (rc == 0) {
      SelectEntry(m_ctlItemList.GetItemCount() - 1);
    }
    ChangeOkUpdate();
    m_RUEList.AddRUEntry(uuid);
  }
}

void DboxMain::OnDisplayPswdSubset()
{
  if (!SelItemOk())
    return;

  CItemData *pci = getSelectedItem();
  ASSERT(pci != NULL);

  CItemData *pci_original(pci);

  if (pci->IsDependent())
    pci = GetBaseEntry(pci);

  CPasswordSubsetDlg DisplaySubsetDlg(this, pci);

  if (DisplaySubsetDlg.DoModal() != IDCANCEL)
    UpdateAccessTime(pci_original);
}

void DboxMain::OnCopyPassword()
{
  CopyDataToClipBoard(CItemData::PASSWORD);
}

void DboxMain::OnCopyPasswordMinimize()
{
  // Do OnCopyPassword and minimize afterwards.
   CopyDataToClipBoard(CItemData::PASSWORD, true);
}

void DboxMain::OnCopyUsername()
{
  CopyDataToClipBoard(CItemData::USER);
}

void DboxMain::OnCopyNotes()
{
  CopyDataToClipBoard(CItemData::NOTES);
}

void DboxMain::OnCopyURL()
{
  CopyDataToClipBoard(CItemData::URL);
}

void DboxMain::OnCopyEmail()
{
  CopyDataToClipBoard(CItemData::EMAIL);
}

void DboxMain::OnCopyRunCommand()
{
  CopyDataToClipBoard(CItemData::RUNCMD);
}

void DboxMain::CopyDataToClipBoard(const CItemData::FieldType ft, const bool special)
{
  // Boolean 'special' flag is CItemData::FieldType 'ft' dependent
  // For example:
  //   For "CItemData::PASSWORD", "special" == true means "minimize after copy"
  if (SelItemOk() != TRUE)
    return;

  CItemData *pci = getSelectedItem();
  ASSERT(pci != NULL);

  CItemData *pci_original(pci);

  if (pci->IsShortcut() ||
      (pci->IsAlias() && ft == CItemData::PASSWORD)) {
    pci = GetBaseEntry(pci);
  }

  StringX cs_data;

  switch (ft) {
    case CItemData::PASSWORD:
    {
      //Remind the user about clipboard security
      CClearQuestionDlg clearDlg(this);
      if (clearDlg.m_dontaskquestion == FALSE &&
          clearDlg.DoModal() == IDCANCEL)
        return;
      cs_data = pci->GetPassword();
      if (special)
        ShowWindow(SW_MINIMIZE);
      break;
    }
    case CItemData::USER:
      cs_data = pci->GetUser();
      break;
    case CItemData::NOTES:
      cs_data = pci->GetNotes();
      break;
    case CItemData::URL:
    {
      StringX::size_type ipos;
      cs_data = pci->GetURL();
      ipos = cs_data.find(L"[alt]");
      if (ipos != StringX::npos)
        cs_data.replace(ipos, 5, L"");
      ipos = cs_data.find(L"[ssh]");
      if (ipos != StringX::npos)
        cs_data.replace(ipos, 5, L"");
      ipos = cs_data.find(L"{alt}");
      if (ipos != StringX::npos)
        cs_data.replace(ipos, 5, L"");
      ipos = cs_data.find(L"[autotype]");
      if (ipos != StringX::npos)
        cs_data.replace(ipos, 10, L"");
      ipos = cs_data.find(L"[xa]");
      if (ipos != StringX::npos)
        cs_data.replace(ipos, 4, L"");
      break;
    }
    case CItemData::RUNCMD:
      cs_data = pci->GetRunCommand();
      break;
    case CItemData::EMAIL:
      cs_data = pci->GetEmail();
      break;
    default:
      ASSERT(0);
  }

  SetClipboardData(cs_data);
  UpdateLastClipboardAction(ft);
  UpdateAccessTime(pci_original);
}

void DboxMain::UpdateLastClipboardAction(const int iaction)
{
  int imsg(0);
  m_lastclipboardaction = L"";
  switch (iaction) {
    case -1:
      // Clipboard cleared
      m_lastclipboardaction = L"";
      break;
    case CItemData::GROUP:
      imsg = IDS_GROUPCOPIED;
      break;
    case CItemData::TITLE:
      imsg = IDS_TITLECOPIED;
      break;
    case CItemData::USER:
      imsg = IDS_USERCOPIED;
      break;
    case CItemData::PASSWORD:
      imsg = IDS_PSWDCOPIED;
      break;
    case CItemData::NOTES:
      imsg = IDS_NOTESCOPIED;
      break;
    case CItemData::URL:
      imsg = IDS_URLCOPIED;
      break;
    case CItemData::AUTOTYPE:
      imsg = IDS_AUTOTYPECOPIED;
      break;
    case CItemData::RUNCMD:
      imsg = IDS_RUNCMDCOPIED;
      break;
    case CItemData::EMAIL:
      imsg = IDS_EMAILCOPIED;
      break;
    default:
      ASSERT(0);
      return;
  }

  if (iaction > 0) {
    wchar_t szTimeFormat[80], szTimeString[80];
    VERIFY(::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, 
                           szTimeFormat, 80 /* sizeof(szTimeFormat) / sizeof(wchar_t) */));
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, szTimeFormat,
                  szTimeString, 80 /* sizeof(szTimeString) / sizeof(wchar_t) */);
    m_lastclipboardaction.LoadString(imsg);
    m_lastclipboardaction += szTimeString;
  }
  UpdateStatusBar();
}

void DboxMain::OnShowFindToolbar()
{
  // Show Find Toolbar
  SetFindToolBar(true);
}

void DboxMain::OnClearClipboard()
{
  UpdateLastClipboardAction(-1);
  ClearClipboardData();
}

// Generate a random password.
// The generated password will be copied to the clipboard. Doing
// this leaves a problem where the user can generate a random password, have
// the password copied to the clipboard and then change the password. This could
// be avoided by putting the password into the clipboard when the entry is saved
// but that would be annoying when generating a new entry.

void DboxMain::MakeRandomPassword(StringX &password, PWPolicy &pwp, 
                                  bool bIssueMsg)
{
  password = pwp.MakeRandomPassword();
  SetClipboardData(password);
  UpdateLastClipboardAction(CItemData::PASSWORD);

  if (bIssueMsg) {
    CGeneralMsgBox gmb;
    CString cs_title, cs_msg;
    cs_title.LoadString(IDS_PASSWORDGENERATED1);
    cs_msg.Format(IDS_PASSWORDGENERATED2, password.c_str());
    gmb.MessageBox(cs_msg, cs_title, MB_OK);
  }
}

void DboxMain::PerformAutoType()
{
  OnAutoType();
}

void DboxMain::OnAutoType()
{
  CItemData *pci(NULL);
  if (m_ctlItemTree.IsWindowVisible() && m_LastFoundTreeItem != NULL) {
    pci = (CItemData *)m_ctlItemTree.GetItemData(m_LastFoundTreeItem);
    TRACE("OnAutoType: Using Tree found item\n");
  } else
  if (m_ctlItemList.IsWindowVisible() && m_LastFoundListItem >= 0) {
    pci = (CItemData *)m_ctlItemList.GetItemData(m_LastFoundListItem);
    TRACE("OnAutoType: Using List found item\n");
  } else {
    pci = getSelectedItem();
    TRACE("OnAutoType: Using Selected item\n");
  }

  if (pci == NULL)
    return;

  UpdateAccessTime(pci);

  // All code using ci must be before this AutoType since the
  // latter may trash *pci if lock-on-minimize
  AutoType(*pci);
}

void DboxMain::AutoType(const CItemData &ci)
{
  // Called from OnAutoType and OnTrayAutoType
  StringX sxgroup, sxtitle, sxuser, sxpwd, sxnotes, sxautotype;

  // Set up all the data (shortcut entry will change all of them!)
  sxgroup = ci.GetGroup();
  sxtitle = ci.GetTitle();
  sxuser = ci.GetUser();
  sxpwd = ci.GetPassword();
  sxnotes = ci.GetNotes();
  sxautotype = ci.GetAutoType();

  if (ci.IsAlias()) {
    CItemData *pbci = GetBaseEntry(&ci);
    if (pbci != NULL) {
      sxpwd = pbci->GetPassword();
    } else {
      // Problem - alias entry without a base!
      ASSERT(0);
    }
  } else if (ci.IsShortcut()) {
    CItemData *pbci = GetBaseEntry(&ci);
    if (pbci != NULL) {
      sxgroup = pbci->GetGroup();
      sxtitle = pbci->GetTitle();
      sxuser = pbci->GetUser();
      sxpwd = pbci->GetPassword();
      sxnotes = pbci->GetNotes();
      sxautotype = pbci->GetAutoType();
    } else {
      // Problem - shortcut entry without a base!
      ASSERT(0);
    }
  } // ci.IsShortcut()

  // If empty, try the database default
  if (sxautotype.empty()) {
    sxautotype = PWSprefs::GetInstance()->
              GetPref(PWSprefs::DefaultAutotypeString);

    // If still empty, take this default
    if (sxautotype.empty()) {
      // checking for user and password for default settings
      if (!sxpwd.empty()){
        if (!sxuser.empty())
          sxautotype = DEFAULT_AUTOTYPE;
        else
          sxautotype = L"\\p\\n";
      }
    }
  }

  // Rules are ("Minimize on Autotype" takes precedence):
  // 1. If "MinimizeOnAutotype" - minimize PWS during Autotype but do
  //    not restore it (previous default action - but a pain if locked
  //    in the system tray!)
  // 2. If "Always on Top" - hide PWS during Autotype and then make it
  //    "AlwaysOnTop" again, unless minimized!
  // 3. If not "Always on Top" - hide PWS during Autotype and show
  //    it again once finished - but behind other windows.
  // NOTE: If "Lock on Minimize" is set and "MinimizeOnAutotype" then
  //       the window will be locked once minimized.
  bool bMinOnAuto = PWSprefs::GetInstance()->
                    GetPref(PWSprefs::MinimizeOnAutotype) == TRUE;

  if (bMinOnAuto)
    ShowWindow(SW_MINIMIZE);
  else
    ShowWindow(SW_HIDE);

  std::vector<size_t> vactionverboffsets;
  sxautotype = PWSAuxParse::GetAutoTypeString(sxautotype,
                sxgroup, sxtitle, sxuser, sxpwd, sxnotes,
                vactionverboffsets);
  DoAutoType(sxautotype, vactionverboffsets);

  // If we minimized it, exit. If we only hid it, now show it
  if (bMinOnAuto)
    return;

  if (PWSprefs::GetInstance()->GetPref(PWSprefs::AlwaysOnTop)) {
    SetWindowPos(&wndTopMost, 0, 0, 0, 0,
                 SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
  } else {
    ShowWindow(SW_SHOW);
  }
}

void DboxMain::DoAutoType(const StringX &sx_autotype, const std::vector<size_t> &vactionverboffsets)
{
  // All parsing of AutoType command done in one place: PWSAuxParse::GetAutoTypeString
  // Except for anything involving time (\d, \w, \W) or use older method (\z)
  StringX sxtmp(L"");
  StringX sxautotype(sx_autotype);
  wchar_t curChar;
 
  bool bForceOldMethod(false), bCapsLock(false);
 
  StringX::size_type st_index = sxautotype.find(L"\\z");

  while (st_index != StringX::npos) {
    if (std::find(vactionverboffsets.begin(), vactionverboffsets.end(), st_index) !=
        vactionverboffsets.end()) {
      bForceOldMethod = true;
      break;
    }
    st_index = sxautotype.find(L"\\z", st_index + 1);
  }

  const int N = sxautotype.length();
  CKeySend ks(m_WindowsMajorVersion, m_WindowsMinorVersion, bForceOldMethod);

  // Turn off CAPSLOCK
  if (GetKeyState(VK_CAPITAL)) {
    bCapsLock = true;
    ks.SetCapsLock(false);
  }

  ks.ResetKeyboardState();

  // Stop Keyboard/Mouse Input
  TRACE(L"DboxMain::DoAutoType - BlockInput set\n");
  ::BlockInput(TRUE);

  ::Sleep(1000); // Karl Student's suggestion, to ensure focus set correctly on minimize.

  int gNumIts;
  for (int n = 0; n < N; n++){
    curChar = sxautotype[n];
    if (curChar == L'\\') {
      n++;
      if (n < N)
        curChar = sxautotype[n];

      // Only need to process fields left in there by PWSAuxParse::GetAutoTypeString
      // for later processing
      switch (curChar) {
        case L'd':
        case L'w':
        case L'W':
        { 
           if (std::find(vactionverboffsets.begin(), vactionverboffsets.end(), n - 1) ==
               vactionverboffsets.end()) {
             // Not in the list of found action verbs - treat as-is
             sxtmp += L'\\';
             sxtmp += curChar;
             break;
           }

          /*
           'd' means value is in milli-seconds, max value = 0.999s
           and is the delay between sending each character

           'w' means value is in milli-seconds, max value = 0.999s
           'W' means value is in seconds, max value = 16m 39s
           and is the wait time before sending the next character.
           Use of this field does not change any current delay value.

           User needs to understand that PasswordSafe will be unresponsive
           for the whole of this wait period!
          */

          // Delay is going to change - send what we have with old delay
          ks.SendString(sxtmp);
          // start collecting new delay
          sxtmp = L"";
          int newdelay = 0;
          gNumIts = 0;
          for (n++; n < N && (gNumIts < 3); ++gNumIts, n++) {
            if (_istdigit(sxautotype[n])) {
              newdelay *= 10;
              newdelay += (sxautotype[n] - L'0');
            } else
              break; // for loop
          }

          n--;
          // Either set new character delay time or wait specified time
          if (curChar == L'd')
            ks.SetAndDelay(newdelay);
          else
            ::Sleep(newdelay * (curChar == L'w' ? 1 : 1000));

          break; // case 'd', 'w' & 'W'
        }
        case L'z':
          if (std::find(vactionverboffsets.begin(), vactionverboffsets.end(), n - 1) ==
              vactionverboffsets.end()) {
            // Not in the list of found action verbs - treat as-is
            sxtmp += L'\\';
            sxtmp += curChar;
          }
          break;
        case L'b':
          if (std::find(vactionverboffsets.begin(), vactionverboffsets.end(), n - 1) ==
              vactionverboffsets.end()) {
            // Not in the list of found action verbs - treat as-is
            sxtmp += L'\\';
            sxtmp += curChar;
          } else {
            sxtmp += L'\b';
          }
          break;
        default:
          sxtmp += L'\\';
          sxtmp += curChar;
          break;
      }
    } else
      sxtmp += curChar;
  }
  ks.SendString(sxtmp);
  // If we turned off CAPSLOCK, put it back
  if (bCapsLock)
    ks.SetCapsLock(true);

  ::Sleep(100);

  // Reset Keyboard/Mouse Input
  TRACE(L"DboxMain::DoAutoType - BlockInput reset\n");
  ::BlockInput(FALSE);
}

void DboxMain::OnGotoBaseEntry()
{
  if (SelItemOk() == TRUE) {
    CItemData *pci = getSelectedItem();
    ASSERT(pci != NULL);

    CItemData *pbci = GetBaseEntry(pci);
    if (pbci != NULL) {
      DisplayInfo *pdi = (DisplayInfo *)pbci->GetDisplayInfo();
      SelectEntry(pdi->list_index);
      UpdateAccessTime(pci);
    }
  }
}

void DboxMain::OnEditBaseEntry()
{
  // Only for Shortcuts & Aliases
  if (SelItemOk() == TRUE) {
    CItemData *pci = getSelectedItem();
    ASSERT(pci != NULL);

    CItemData *pbci = GetBaseEntry(pci);
    if (pbci != NULL) {
       DisplayInfo *pdi = (DisplayInfo *)pbci->GetDisplayInfo();
       SelectEntry(pdi->list_index);
       EditItem(pbci); // pbci may be invalid upon return!
       UpdateAccessTime(GetBaseEntry(pci));
    }
  }
}

void DboxMain::OnRunCommand()
{
  if (SelItemOk() != TRUE)
    return;

  CItemData *pci = getSelectedItem();
  ASSERT(pci != NULL);

  CItemData *pci_original(pci);
  StringX sx_pswd;

  if (pci->IsDependent())
    sx_pswd = GetBaseEntry(pci)->GetPassword();
  else
    sx_pswd = pci->GetPassword();

  StringX sx_RunCommand, sx_Expanded_ES;
  sx_RunCommand = pci->GetRunCommand();
  if (sx_RunCommand.empty())
    return;

  std::wstring errmsg;
  StringX::size_type st_column;
  bool bURLSpecial;
  sx_Expanded_ES = PWSAuxParse::GetExpandedString(sx_RunCommand, 
                       m_core.GetCurFile(), pci, 
                       m_bDoAutoType, m_AutoType, 
                       errmsg, st_column, bURLSpecial);
  if (!errmsg.empty()) {
    CGeneralMsgBox gmb;
    CString cs_title, cs_errmsg;
    cs_title.LoadString(IDS_RUNCOMMAND_ERROR);
    cs_errmsg.Format(IDS_RUN_ERRORMSG, (int)st_column, errmsg.c_str());
    gmb.MessageBox(cs_errmsg, cs_title, MB_OK | MB_ICONQUESTION);
    return;
  }

  m_AutoType = PWSAuxParse::GetAutoTypeString(m_AutoType, pci->GetGroup(), 
                                 pci->GetTitle(), pci->GetUser(), 
                                 sx_pswd, pci->GetNotes(),
                                 m_vactionverboffsets);
  SetClipboardData(pci->GetPassword());
  UpdateLastClipboardAction(CItemData::PASSWORD);
  UpdateAccessTime(pci_original);

  // Now honour presence of [alt], {alt} or [ssh] in the url if present
  // in the RunCommand field.  Note: they are all treated the same (unlike
  // in 'Browse to'.
  StringX sxAltBrowser(PWSprefs::GetInstance()->
                         GetPref(PWSprefs::AltBrowser));

  if (bURLSpecial && !sxAltBrowser.empty()) {
    StringX sxCmdLineParms(PWSprefs::GetInstance()->
                           GetPref(PWSprefs::AltBrowserCmdLineParms));

    if (sxAltBrowser[0] != L'\'' && sxAltBrowser[0] != L'"')
      sxAltBrowser = L"\"" + sxAltBrowser + L"\"";
    if (!sxCmdLineParms.empty())
      sx_Expanded_ES = sxAltBrowser + StringX(L" ") + 
                       sxCmdLineParms + StringX(L" ") + sx_Expanded_ES;
    else
      sx_Expanded_ES = sxAltBrowser + StringX(L" ") + sx_Expanded_ES;
  }

  bool rc = m_runner.runcmd(sx_Expanded_ES, !m_AutoType.empty());
  if (!rc) {
    m_bDoAutoType = false;
    m_AutoType.clear();
    return;
  }
}

void DboxMain::AddEntries(CDDObList &in_oblist, const StringX &DropGroup)
{
  // Add Drop entries
  CItemData ci_temp;
  UUIDList possible_aliases, possible_shortcuts;
  StringX Group, Title, User;
  POSITION pos;
  wchar_t *dot;
  uuid_array_t entry_uuid;
  bool bAddToViews;

  MultiCommands *pmulticmds = MultiCommands::Create(&m_core);

  for (pos = in_oblist.GetHeadPosition(); pos != NULL; in_oblist.GetNext(pos)) {
    CDDObject *pDDObject = (CDDObject *)in_oblist.GetAt(pos);
#ifdef DEMO
    if (m_core.GetNumEntries() >= MAXDEMO)
      break;
#endif /* DEMO */
    ci_temp.Clear();
    // Only set to false if adding a shortcut where the base isn't there (yet)
    bAddToViews = true;
    pDDObject->ToItem(ci_temp);

    if (in_oblist.m_bDragNode) {
      dot = (!DropGroup.empty() && !ci_temp.GetGroup().empty()) ? L"." : L"";
      Group = DropGroup + dot + ci_temp.GetGroup();
    } else {
      Group = DropGroup;
    }

    User = ci_temp.GetUser();
    Title = GetUniqueTitle(Group, ci_temp.GetTitle(), User, IDS_DRAGNUMBER);

    ci_temp.GetUUID(entry_uuid);
    if (m_core.Find(entry_uuid) != End()) {
      // Already in use - get a new one!
      ci_temp.CreateUUID();
      ci_temp.GetUUID(entry_uuid);
    }

    ci_temp.SetGroup(Group);
    ci_temp.SetTitle(Title);

    StringX cs_tmp = ci_temp.GetPassword();

    BaseEntryParms pl;
    pl.InputType = CItemData::ET_NORMAL;

    // Potentially remove outer single square brackets as ParseBaseEntryPWD expects only
    // one set of square brackets (processing import and user edit of entries)
    if (cs_tmp.substr(0, 2) == L"[[" &&
        cs_tmp.substr(cs_tmp.length() - 2) == L"]]") {
      cs_tmp = cs_tmp.substr(1, cs_tmp.length() - 2);
      pl.InputType = CItemData::ET_ALIAS;
    }

    // Potentially remove tilde as ParseBaseEntryPWD expects only
    // one set of square brackets (processing import and user edit of entries)
    if (cs_tmp.substr(0, 2) == L"[~" &&
        cs_tmp.substr(cs_tmp.length() - 2) == L"~]") {
      cs_tmp = L"[" + cs_tmp.substr(2, cs_tmp.length() - 4) + L"]";
      pl.InputType = CItemData::ET_SHORTCUT;
    }

    m_core.ParseBaseEntryPWD(cs_tmp, pl);
    if (pl.ibasedata > 0) {
      CGeneralMsgBox gmb;
      // Password in alias/shortcut format AND base entry exists
      if (pl.InputType == CItemData::ET_ALIAS) {
        ItemListIter iter = m_core.Find(pl.base_uuid);
        ASSERT(iter != End());
        if (pl.TargetType == CItemData::ET_ALIAS) {
          // This base is in fact an alias. ParseBaseEntryPWD already found 'proper base'
          // So dropped entry will point to the 'proper base' and tell the user.
          CString cs_msg;
          cs_msg.Format(IDS_DDBASEISALIAS, Group, Title, User);
          gmb.AfxMessageBox(cs_msg, NULL, MB_OK);
        } else
        if (pl.TargetType != CItemData::ET_NORMAL && pl.TargetType != CItemData::ET_ALIASBASE) {
          // Only normal or alias base allowed as target
          CString cs_msg;
          cs_msg.Format(IDS_ABASEINVALID, Group, Title, User);
          gmb.AfxMessageBox(cs_msg, NULL, MB_OK);
          continue;
        }
        Command *pcmd = AddDependentEntryCommand::Create(&m_core, pl.base_uuid,
                                                         entry_uuid,
                                                         CItemData::ET_ALIAS);
        pmulticmds->Add(pcmd);
        ci_temp.SetPassword(L"[Alias]");
        ci_temp.SetAlias();
      } else
      if (pl.InputType == CItemData::ET_SHORTCUT) {
        ItemListIter iter = m_core.Find(pl.base_uuid);
        ASSERT(iter != End());
        if (pl.TargetType != CItemData::ET_NORMAL && pl.TargetType != CItemData::ET_SHORTCUTBASE) {
          // Only normal or shortcut base allowed as target
          CString cs_msg;
          cs_msg.Format(IDS_SBASEINVALID, Group, Title, User);
          gmb.AfxMessageBox(cs_msg, NULL, MB_OK);
          continue;
        }
        Command *pcmd = AddDependentEntryCommand::Create(&m_core,
                                                         pl.base_uuid,
                                                         entry_uuid,
                                                         CItemData::ET_SHORTCUT);
        pmulticmds->Add(pcmd);
        ci_temp.SetPassword(L"[Shortcut]");
        ci_temp.SetShortcut();
      }
    } else
    if (pl.ibasedata == 0) {
      // Password NOT in alias/shortcut format
      ci_temp.SetNormal();
    } else
    if (pl.ibasedata < 0) {
      // Password in alias/shortcut format AND base entry does not exist or multiple possible
      // base entries exit.
      // Note: As more entries are added, what was "not exist" may become "OK",
      // "no unique exists" or "multiple exist".
      // Let the code that processes the possible aliases after all have been added sort this out.
      if (pl.InputType == CItemData::ET_ALIAS) {
        possible_aliases.push_back(entry_uuid);
      } else
      if (pl.InputType == CItemData::ET_SHORTCUT) {
        possible_shortcuts.push_back(entry_uuid);
        bAddToViews = false;
      }
    }
    ci_temp.SetStatus(CItemData::ES_ADDED);
    // Add to pwlist
    Command *pcmd = AddEntryCommand::Create(&m_core, ci_temp);
    if (!bAddToViews) {
      // ONLY Add to pwlist and NOT to Tree or List views
      // After the call to AddDependentEntries for shortcuts, check if still
      // in password list and, if so, then add to Tree + List views
      pcmd->SetNoGUINotify();
    }
    pmulticmds->Add(pcmd);
  } // iteration over in_oblist

  // Now try to add aliases/shortcuts we couldn't add in previous processing
  Command *pcmdA = AddDependentEntriesCommand::Create(&m_core, possible_aliases,
                                                      NULL, CItemData::ET_ALIAS,
                                                      CItemData::PASSWORD);
  pmulticmds->Add(pcmdA);
  Command *pcmdS = AddDependentEntriesCommand::Create(&m_core,
                                                      possible_shortcuts, NULL, 
                                                      CItemData::ET_SHORTCUT,
                                                      CItemData::PASSWORD);
  pmulticmds->Add(pcmdS);
  Execute(pmulticmds);

  possible_aliases.clear();

  // Some shortcuts may have been deleted from the database as base does not exist
  // Tidy up Tree/List
  UUIDListIter paiter;
  ItemListIter iter;
  for (paiter = possible_shortcuts.begin();
       paiter != possible_shortcuts.end(); paiter++) {
    paiter->GetUUID(entry_uuid);
    iter = m_core.Find(entry_uuid);
    if (iter != End()) {
      // Still in pwlist - NOW add to Tree and List views
      InsertItemIntoGUITreeList(m_core.GetEntry(iter));
    }
  }
  possible_shortcuts.clear();

  SetChanged(Data);
  FixListIndexes();
  RefreshViews();
}

LRESULT DboxMain::OnToolBarFindMessage(WPARAM /* wParam */, LPARAM /* lParam */)
{
  // Called when user types into the Find search edit control on the Find Toolbar
  // and presses enter.
  OnToolBarFind();
  return 0L;
}

void DboxMain::OnToolBarFind()
{
  // Called when the user presses the Find button on the Find Toolbar or
  // when they press enter when the search string has focus or
  // when they press F3
  if (GetKeyState(VK_SHIFT) & 0x8000) {
    // User clicked on Toolbar button with Shift key down
    OnToolBarFindUp();
    return;
  }
  m_FindToolBar.SetSearchDirection(FIND_DOWN);
  m_FindToolBar.Find();
}

void DboxMain::OnToolBarFindUp()
{
  // Called when the user presses the Find button on the Find Toolbar or
  // when they press enter when the search string has focus or
  // when they press Shift+F3
  m_FindToolBar.SetSearchDirection(FIND_UP);
  m_FindToolBar.Find();
}