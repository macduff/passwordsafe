/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// Command.cpp
//-----------------------------------------------------------------------------

#include "CommandInterface.h"
#include "Command.h"
#include "PWSprefs.h"
#include "return_codes.h"

#include <algorithm>

// ------------------------------------------------
// Base class: Command
// ------------------------------------------------

/*

The base class provides Save/restore state functions to enable Undo/Redo to be
correctly performed.

All GUI functions should ONLY use Command-derived classes to update any values in
the core.  None should be updated directly.

The MultiCommands derived class allows multiple commands to be lumped together
as one unit of work (in terms of Execute, Undo & Redo)

All other derived classes are aptly named to indicate their function.

There are 2 special derived classes:
1. UpdateGUICommand - which calls the GUI to allow it to update what the user sees
after any change.  For MultiCommands, it is normal to turn off GUI notification
during the execution of the individual commands and only notify the GUI at the 
end (e.g. after importing a lot of entries or after undoing the importing of these
entries).  Flags can be set on when to do this notification.

2. GUICommand - which allows the GUI to add GUI related commands to a MultiCommand
unit of work.

*/

using pws_os::CUUID;

Command::Command(CommandInterface *pcomInt)
:  m_pcomInt(pcomInt), m_bNotifyGUI(true), m_RC(0), m_bState(false)
{
}

Command::~Command()
{
}

// Save/restore state
void Command::SaveState()
{
  m_bSaveDBChanged = m_pcomInt->IsChanged();
  m_bUniqueGTUValidated = m_pcomInt->GetUniqueGTUValidated();
  m_saved_vNodes_Modified = m_pcomInt->GetVNodesModified();
}

void Command::RestoreState()
{
  m_pcomInt->SetDBChanged(m_bSaveDBChanged);
  m_pcomInt->SetUniqueGTUValidated(m_bUniqueGTUValidated);
  m_pcomInt->SetVNodesModified(m_saved_vNodes_Modified);
}

// ------------------------------------------------
// MultiCommands
// ------------------------------------------------

MultiCommands::MultiCommands(CommandInterface *pcomInt)
  : Command(pcomInt)
{
}

MultiCommands::~MultiCommands()
{
  std::vector<Command *>::iterator cmd_Iter;

  for (cmd_Iter = m_vpcmds.begin(); cmd_Iter != m_vpcmds.end(); cmd_Iter++) {
    delete (*cmd_Iter);
  }
}

int MultiCommands::Execute(const bool bRedo)
{
  std::vector<Command *>::iterator cmd_Iter;

  for (cmd_Iter = m_vpcmds.begin(); cmd_Iter != m_vpcmds.end(); cmd_Iter++) {
    int rc(-1);
    if (*cmd_Iter != NULL) {
      rc = (*cmd_Iter)->Execute(bRedo);
    }
    m_vRCs.push_back(rc);
  }

  m_bState = true;
  return 0;
}

void MultiCommands::Undo()
{
  std::vector<Command *>::reverse_iterator cmd_rIter;

  for (cmd_rIter = m_vpcmds.rbegin(); cmd_rIter != m_vpcmds.rend(); cmd_rIter++) {
    if (*cmd_rIter != NULL)
      (*cmd_rIter)->Undo();
  }

  m_bState = false;
}

void MultiCommands::Add(Command *pcmd)
{
  ASSERT(pcmd != NULL);
  m_vpcmds.push_back(pcmd);
}

void MultiCommands::Insert(Command *pcmd)
{
  // VERY INEFFICIENT - use sparingly to add commands at the front of the
  // multi-command vector
  ASSERT(pcmd != NULL);
  m_vpcmds.insert(m_vpcmds.begin(), pcmd);
}

bool MultiCommands::Remove(Command *pcmd)
{
  ASSERT(pcmd != NULL);
  std::vector<Command *>::iterator cmd_Iter;

  cmd_Iter = find(m_vpcmds.begin(), m_vpcmds.end(), pcmd);
  if (cmd_Iter != m_vpcmds.end()) {
    delete (*cmd_Iter);
    m_vpcmds.erase(cmd_Iter);
    return true;
  } else
    return false;
}

bool MultiCommands::Remove()
{
  if (!m_vpcmds.empty()) {
    delete m_vpcmds.back();
    m_vpcmds.pop_back();
    return true;
  } else
    return false;
}

bool MultiCommands::GetRC(Command *pcmd, int &rc)
{
  std::vector<Command *>::iterator cmd_Iter;

  cmd_Iter = find(m_vpcmds.begin(), m_vpcmds.end(), pcmd);
  if (cmd_Iter != m_vpcmds.end()) {
    rc = m_vRCs[cmd_Iter - m_vpcmds.begin()];
    return true;
  } else {
    rc = 0;
    return false;
  }
}

bool MultiCommands::GetRC(const size_t ncmd, int &rc)
{
  if (ncmd <= 0 || ncmd > m_vRCs.size()) {
    rc = 0;
    return false;
  } else {
    rc = m_vRCs[ncmd - 1];
    return true;
  }
}

void MultiCommands::ResetSavedState(bool bNewDBState)
{
  Command::ResetSavedState(bNewDBState);
  std::vector<Command *>::iterator cmd_Iter;
  for (cmd_Iter = m_vpcmds.begin(); cmd_Iter != m_vpcmds.end(); cmd_Iter++) {
    (*cmd_Iter)->ResetSavedState(bNewDBState);
  }
}

// ------------------------------------------------
// UpdateGUICommand
// ------------------------------------------------

UpdateGUICommand::UpdateGUICommand(CommandInterface *pcomInt,
                                   ExecuteFn When, GUI_Action ga)
  : Command(pcomInt), m_When(When), m_ga(ga)
{
}

int UpdateGUICommand::Execute(const bool /*bRedo*/)
{
  if (m_When == WN_EXECUTE || m_When == WN_EXECUTE_REDO || 
      m_When == WN_REDO || m_When == WN_ALL) {
    m_pcomInt->NotifyGUINeedsUpdating(m_ga, CUUID::NullUUID());
  }
  return 0;
}

void UpdateGUICommand::Undo()
{
  if (m_When == WN_UNDO || m_When == WN_ALL) {
    m_pcomInt->NotifyGUINeedsUpdating(m_ga, CUUID::NullUUID());
  }
}

// ------------------------------------------------
// DBPrefsCommand
// ------------------------------------------------

DBPrefsCommand::DBPrefsCommand(CommandInterface *pcomInt, StringX &sxDBPrefs)
  : Command(pcomInt), m_sxNewDBPrefs(sxDBPrefs)
{
  m_bOldState = PWSprefs::GetInstance()->IsDBprefsChanged();
  m_sxOldDBPrefs = PWSprefs::GetInstance()->Store();
}

int DBPrefsCommand::Execute(const bool /*bRedo*/)
{
  PWSprefs::GetInstance()->Load(m_sxNewDBPrefs);
  if (!m_pcomInt->IsReadOnly())
    m_pcomInt->SetDBPrefsChanged(m_pcomInt->HaveHeaderPreferencesChanged(m_sxNewDBPrefs));

  if (m_bNotifyGUI) {
    m_pcomInt->NotifyGUINeedsUpdating(UpdateGUICommand::GUI_DB_PREFERENCES_CHANGED,
                                      CUUID::NullUUID());
  }

  m_bState = true;
  return 0;
}

void DBPrefsCommand::Undo()
{
  PWSprefs::GetInstance()->Load(m_sxOldDBPrefs);
  if (!m_pcomInt->IsReadOnly())
    m_pcomInt->SetDBPrefsChanged(m_bOldState);

  if (m_bNotifyGUI) {
    m_pcomInt->NotifyGUINeedsUpdating(UpdateGUICommand::GUI_DB_PREFERENCES_CHANGED,
                                      CUUID::NullUUID());
  }

  m_bState = false;
}

// ------------------------------------------------
// AddEntryCommand
// ------------------------------------------------

// Normal or Base entry
AddEntryCommand::AddEntryCommand(CommandInterface *pcomInt, const CItemData &ci, 
                                 const Command *pcmd, const ATRVector *pvNewATRecords)
  : Command(pcomInt), m_ci(ci)
{
  ASSERT(!ci.IsDependent()); // use other c'tor for dependent entries!

  if (pvNewATRecords != NULL)
    m_vNewATRecords = *pvNewATRecords;

  m_entry_uuid = m_ci.GetUUID();

  if (pcmd != NULL)
    m_bNotifyGUI = pcmd->GetGUINotify();
}

// Alias or Shortcut entry (Shortcuts do not have attachments)
AddEntryCommand::AddEntryCommand(CommandInterface *pcomInt, const CItemData &ci,
                                 const CUUID &base_uuid, const Command *pcmd,
                                 const ATRVector *pvNewATRecords)
  : Command(pcomInt), m_ci(ci), m_base_uuid(base_uuid)
{
  if (pvNewATRecords != NULL)
    m_vNewATRecords = *pvNewATRecords;

  m_entry_uuid = m_ci.GetUUID();

  if (pcmd != NULL)
    m_bNotifyGUI = pcmd->GetGUINotify();
}

AddEntryCommand::~AddEntryCommand()
{
}

int AddEntryCommand::Execute(const bool /*bRedo*/)
{
  SaveState();
  
  if (m_pcomInt->IsReadOnly())
    return 0;

  // Add entry
  m_pcomInt->DoAddEntry(m_ci);

  // Only add attachments on Edit - not Add - so no code here

  m_pcomInt->AddChangedNodes(m_ci.GetGroup());

  if (m_ci.IsDependent()) {
    m_pcomInt->DoAddDependentEntry(m_base_uuid, m_ci.GetUUID(),
                                   m_ci.GetEntryType());
  }

  if (m_bNotifyGUI) {
    m_pcomInt->NotifyGUINeedsUpdating(UpdateGUICommand::GUI_ADD_ENTRY,
                                      m_ci.GetUUID());
  }

  time_t tttXTime;
  m_ci.GetXTime(tttXTime);
  if (tttXTime != time_t(0)) {
    m_pcomInt->AddExpiryEntry(m_ci);
  } 

  m_bState = true;
  return 0;
}

int AddEntryCommand::Redo()
{
  int irc = Execute(true);

  // Just unmark for deletion
  m_pcomInt->UnMarkAllAttachmentsForDeletion(m_entry_uuid);
  return irc;
}

void AddEntryCommand::Undo()
{
  // Just mark for deletion - don't actually remove them yet
  m_pcomInt->MarkAllAttachmentsForDeletion(m_entry_uuid);

  DeleteEntryCommand dec(m_pcomInt, m_ci, this);
  dec.Execute();

  if (m_ci.IsDependent()) {
    m_pcomInt->DoRemoveDependentEntry(m_base_uuid, m_ci.GetUUID(),
                                      m_ci.GetEntryType());
  }

  RestoreState();
  m_bState = false;
}

// ------------------------------------------------
// DeleteEntryCommand
// ------------------------------------------------

DeleteEntryCommand::DeleteEntryCommand(CommandInterface *pcomInt,
                                       const CItemData &ci, const Command *pcmd)
  : Command(pcomInt), m_ci(ci), m_dependents(0)
{
   if (ci.IsShortcut())
     m_entry_uuid = CUUID::NullUUID();
   else
     m_entry_uuid = m_ci.GetUUID();

  if (pcmd != NULL) {
    m_bNotifyGUI = pcmd->GetGUINotify();
  }

  if (ci.IsNormal()) {
    m_base_uuid = CUUID::NullUUID();
  } else {
    const CUUID uuid = ci.GetUUID();
    // If ci is not a normal entry, gather the related entry
    // info for undo
    if (ci.IsDependent()) {
      // For aliases or shortcuts, we just need the uuid of the base entry
      const ItemMap &imap = (ci.IsAlias() ? pcomInt->GetAlias2BaseMap() :
                             pcomInt->GetShortcuts2BaseMap());
      m_base_uuid = imap.find(uuid)->second;
    } else if (ci.IsBase()) {
      /**
       * When a shortcut base is deleted, we need to save all
       * the shortcuts referencing it, as they too are deleted.
       * When an alias base is deleted, we need the uuids of all its
       * dependents, to change their passwords back upon undo
       * To save code, we just keep the entire entry, same as shortcuts
      */
      const ItemMMap &immap = 
        ci.IsShortcutBase() ? pcomInt->GetBase2ShortcutsMmap() : pcomInt->GetBase2AliasesMmap();
      ItemMMapConstIter iter;
      for (iter = immap.lower_bound(uuid);
           iter != immap.upper_bound(uuid); iter++) {
        const CUUID dep_uuid(iter->second);
        ItemListIter itemIter = pcomInt->Find(dep_uuid);
        ASSERT(itemIter != pcomInt->GetEntryEndIter());
        if (itemIter != pcomInt->GetEntryEndIter())
          m_dependents.push_back(itemIter->second);
      } // for all dependents
    } // IsBase
  } // !IsNormal
}

DeleteEntryCommand::~DeleteEntryCommand()
{
}

int DeleteEntryCommand::Execute(const bool /*bRedo*/)
{
  SaveState();


  if (m_pcomInt->IsReadOnly())
    return 0;

  if (m_bNotifyGUI) {
    m_pcomInt->NotifyGUINeedsUpdating(UpdateGUICommand::GUI_DELETE_ENTRY,
                                      m_ci.GetUUID());
  }

  m_pcomInt->MarkAllAttachmentsForDeletion(m_entry_uuid);
  m_pcomInt->DoDeleteEntry(m_ci);
  m_pcomInt->AddChangedNodes(m_ci.GetGroup());
  m_pcomInt->RemoveExpiryEntry(m_ci);

  m_bState = true;
  return 0;
}

void DeleteEntryCommand::Undo()
{
  CUUID uuid = m_ci.GetUUID();

  if (m_ci.IsDependent()) {
    Command *pcmd = AddEntryCommand::Create(m_pcomInt, m_ci, m_base_uuid, this);
    pcmd->Execute();
    delete pcmd;
  } else {
    AddEntryCommand undo(m_pcomInt, m_ci, this);
    undo.Execute();
    if (m_ci.IsShortcutBase()) { // restore dependents
      for (std::vector<CItemData>::iterator iter = m_dependents.begin();
           iter != m_dependents.end(); iter++) {
        Command *pcmd = AddEntryCommand::Create(m_pcomInt, *iter, uuid);
        pcmd->Execute();
        delete pcmd;
      }
    } else if (m_ci.IsAliasBase()) {
      // Undeleting an alias base means making all the dependents refer to the alias
      // again. Perhaps the easiest approach is to delete the existing entries
      // and create new aliases.
      for (std::vector<CItemData>::iterator iter = m_dependents.begin();
           iter != m_dependents.end(); iter++) {
        DeleteEntryCommand delExAlias(m_pcomInt, *iter, this);
        delExAlias.Execute(); // out with the old...
        Command *pcmd = AddEntryCommand::Create(m_pcomInt, *iter, uuid, this);
        pcmd->Execute(); // in with the new!
        delete pcmd;
      }
    }
  }
  m_pcomInt->UnMarkAllAttachmentsForDeletion(m_entry_uuid);

  RestoreState();

  m_bState = false;
}

// ------------------------------------------------
// EditEntryCommand
// ------------------------------------------------

EditEntryCommand::EditEntryCommand(CommandInterface *pcomInt,
                                   const CItemData &old_ci,
                                   const CItemData &new_ci,
                                   const ATRVector *pvNewATRecords,
                                   const ATRVector *pvATRecords)
  : Command(pcomInt), m_old_ci(old_ci), m_new_ci(new_ci)
{
  // We're only supposed to operate on entries
  // with same uuids, and possibly different fields
  ASSERT(m_old_ci.GetUUID() == m_new_ci.GetUUID());
  m_entry_uuid = m_old_ci.GetUUID();

  if (pvNewATRecords != NULL)
    m_vNewATRecords = *pvNewATRecords;
  if (pvATRecords != NULL)
    m_vATRecords = *pvATRecords;

  m_pcomInt->GetAttachments(m_entry_uuid, m_vOriginalATRecords);
}

EditEntryCommand::~EditEntryCommand()
{
}

int EditEntryCommand::Execute(const bool bRedo)
{
  SaveState();

  int rc = 0;

  if (m_pcomInt->IsReadOnly())
    return rc;

  m_pcomInt->DoReplaceEntry(m_old_ci, m_new_ci);

  m_pcomInt->AddChangedNodes(m_old_ci.GetGroup());
  m_pcomInt->AddChangedNodes(m_new_ci.GetGroup());

  if (m_bNotifyGUI) {
    const CUUID entry_uuid = m_old_ci.GetUUID();
    // if the group's changed, refresh the entire tree, otherwise, just the field
    UpdateGUICommand::GUI_Action gac = (m_old_ci.GetGroup() != m_new_ci.GetGroup()) ?
      UpdateGUICommand::GUI_REFRESH_TREE : UpdateGUICommand::GUI_REFRESH_ENTRYFIELD;
    m_pcomInt->NotifyGUINeedsUpdating(gac, entry_uuid);
  }

  // Delete/change some old attachments
  for (size_t i = 0; i < m_vATRecords.size(); i++) {
    if ((m_vATRecords[i].uiflags & ATT_ATTACHMENT_DELETED) == ATT_ATTACHMENT_DELETED) {
      m_pcomInt->MarkAttachmentForDeletion(m_vATRecords[i]);
    }
    if ((m_vATRecords[i].uiflags & ATT_ATTACHMENT_FLGCHGD) == ATT_ATTACHMENT_FLGCHGD) {
      m_pcomInt->ChangeAttachment(m_vATRecords[i]);
    }
  }

  // Only add attachments if first execute - not on Redo as deleted attachments
  // are not removed from the file until the associated database is closed
  if (!bRedo && !m_vNewATRecords.empty()) {
    m_pcomInt->AddAttachments(m_vNewATRecords);
    rc = m_pcomInt->XWriteAttachmentFile(false);
  }

  if (rc != PWSRC::SUCCESS)
    return rc;

  m_bState = true;
  return rc;
}

int EditEntryCommand::Redo()
{
  int irc = Execute(true);

  // Just unmark new attachments for deletion
  for (size_t i = 0; i < m_vNewATRecords.size(); i++) {
    m_pcomInt->UnMarkAttachmentForDeletion(m_vNewATRecords[i]);
  }

  // Need to Redo other changes
  return irc;
}

void EditEntryCommand::Undo()
{
  if (m_pcomInt->IsReadOnly())
    return;

  m_pcomInt->DoReplaceEntry(m_new_ci, m_old_ci);

  if (m_bNotifyGUI) {
    const CUUID entry_uuid = m_old_ci.GetUUID();
    // if the group's changed, refresh the entire tree, otherwise, just the field
    UpdateGUICommand::GUI_Action gac = (m_old_ci.GetGroup() != m_new_ci.GetGroup()) ?
      UpdateGUICommand::GUI_REFRESH_TREE : UpdateGUICommand::GUI_REFRESH_ENTRYFIELD;
    m_pcomInt->NotifyGUINeedsUpdating(gac, entry_uuid);
  }

  // Just mark added attachments for deletion - don't actually remove them yet
  for (size_t i = 0; i < m_vNewATRecords.size(); i++) {
    m_pcomInt->MarkAttachmentForDeletion(m_vNewATRecords[i]);
  }

  // Need to restore other changes
  RestoreState();
  m_bState = false;
}

// ------------------------------------------------
// UpdateEntryCommand
// ------------------------------------------------

UpdateEntryCommand::UpdateEntryCommand(CommandInterface *pcomInt,
                                       const CItemData &ci,
                                       CItemData::FieldType ftype,
                                       const StringX &value)
  : Command(pcomInt), m_ftype(ftype), m_value(value)
{
  m_entry_uuid = ci.GetUUID();
  m_old_status = ci.GetStatus();
  m_old_value = ci.GetFieldValue(m_ftype);
}

void UpdateEntryCommand::Doit(const CUUID &entry_uuid,
                              CItemData::FieldType ftype,
                              const StringX &value,
                              CItemData::EntryStatus es,
                              UpdateGUICommand::ExecuteFn efn)
{
  if (m_pcomInt->IsReadOnly())
    return;

  ItemListIter pos = m_pcomInt->Find(entry_uuid);
  if (pos != m_pcomInt->GetEntryEndIter()) {
    if (ftype != CItemData::PASSWORD)
      pos->second.SetFieldValue(ftype, value);
    else {
      if (efn == UpdateGUICommand::WN_EXECUTE_REDO) {
        m_oldpwhistory = pos->second.GetPWHistory();
        pos->second.GetXTime(m_tttoldXtime);
        pos->second.UpdatePassword(value);
      } else {
        pos->second.SetPassword(value);
        pos->second.SetXTime(m_tttoldXtime);
        pos->second.SetPWHistory(m_oldpwhistory);
      }
    }
    if (ftype == CItemData::PASSWORD ||
        ftype == CItemData::XTIME)
      m_pcomInt->UpdateExpiryEntry(pos->second);

    pos->second.SetStatus(es);
    m_pcomInt->SetDBChanged(true, false);
    m_pcomInt->AddChangedNodes(pos->second.GetGroup());
  }
}

int UpdateEntryCommand::Execute(const bool /*bRedo*/)
{
  SaveState();

  Doit(m_entry_uuid, m_ftype, m_value, CItemData::ES_MODIFIED, UpdateGUICommand::WN_EXECUTE_REDO);

  if (m_bNotifyGUI)
    m_pcomInt->NotifyGUINeedsUpdating(UpdateGUICommand::GUI_REFRESH_ENTRYFIELD,
                                      m_entry_uuid, m_ftype);

  if (m_ftype == CItemData::XTIME)
    m_pcomInt->UpdateExpiryEntry(m_entry_uuid, m_ftype, m_value);

  m_bState = true;
  return 0;
}

void UpdateEntryCommand::Undo()
{
  Doit(m_entry_uuid, m_ftype, m_old_value, m_old_status, UpdateGUICommand::WN_UNDO);
  RestoreState();

  if (m_bNotifyGUI)
    m_pcomInt->NotifyGUINeedsUpdating(UpdateGUICommand::GUI_REFRESH_ENTRYFIELD,
                                      m_entry_uuid, m_ftype);
  m_bState = false;
}

// ------------------------------------------------
// UpdatePasswordCommand
// ------------------------------------------------

UpdatePasswordCommand::UpdatePasswordCommand(CommandInterface *pcomInt,
                                             CItemData &ci,
                                             const StringX sxNewPassword)
  : Command(pcomInt), m_sxNewPassword(sxNewPassword)
{
  m_entry_uuid = ci.GetUUID();
  m_old_status = ci.GetStatus();
  m_sxOldPassword = ci.GetPassword();
  m_sxOldPWHistory = ci.GetPWHistory();
  ci.GetXTime(m_tttOldXTime);
}

int UpdatePasswordCommand::Execute(const bool /*bRedo*/)
{
  SaveState();

  if (m_pcomInt->IsReadOnly())
    return 0;

  ItemListIter pos = m_pcomInt->Find(m_entry_uuid);
  if (pos != m_pcomInt->GetEntryEndIter()) {
    pos->second.UpdatePassword(m_sxNewPassword);
    time_t tttNewXTime;
    pos->second.GetXTime(tttNewXTime);
    if (m_tttOldXTime != tttNewXTime) {
      m_pcomInt->UpdateExpiryEntry(pos->second);
    }
    pos->second.SetStatus(CItemData::ES_MODIFIED);
    m_pcomInt->SetDBChanged(true, false);
    m_pcomInt->AddChangedNodes(pos->second.GetGroup());
  }

  if (m_bNotifyGUI)
    m_pcomInt->NotifyGUINeedsUpdating(UpdateGUICommand::GUI_REFRESH_ENTRYPASSWORD,
                                      m_entry_uuid);

  m_bState = true;
  return 0;
}

void UpdatePasswordCommand::Undo()
{
  if (m_pcomInt->IsReadOnly())
    return;

  ItemListIter pos = m_pcomInt->Find(m_entry_uuid);
  if (pos != m_pcomInt->GetEntryEndIter()) {
    pos->second.SetPassword(m_sxOldPassword);
    pos->second.SetPWHistory(m_sxOldPWHistory);
    pos->second.SetStatus(m_old_status);
    pos->second.SetXTime(m_tttOldXTime);
  }

  RestoreState();

  if (m_bNotifyGUI)
    m_pcomInt->NotifyGUINeedsUpdating(UpdateGUICommand::GUI_REFRESH_ENTRYPASSWORD,
                                      m_entry_uuid);
  m_bState = false;
}

// ------------------------------------------------
// AddDependentEntryCommand
// ------------------------------------------------

AddDependentEntryCommand::AddDependentEntryCommand(CommandInterface *pcomInt,
                                                   const CUUID &base_uuid,
                                                   const CUUID &entry_uuid,
                                                   const CItemData::EntryType type)
  : Command(pcomInt), m_base_uuid(base_uuid),
    m_entry_uuid(entry_uuid), m_type(type)
{
}

int AddDependentEntryCommand::Execute(const bool /*bRedo*/)
{
  SaveState();

  if (m_pcomInt->IsReadOnly())
    return 0;

  m_pcomInt->DoAddDependentEntry(m_base_uuid, m_entry_uuid, m_type);
  m_bState = true;
  return 0;
}

void AddDependentEntryCommand::Undo()
{
  if (m_pcomInt->IsReadOnly())
    return;

  m_pcomInt->DoRemoveDependentEntry(m_base_uuid, m_entry_uuid, m_type);
  RestoreState();
  m_bState = false;
}

// ------------------------------------------------
// AddDependentEntriesCommand
// ------------------------------------------------

AddDependentEntriesCommand::AddDependentEntriesCommand(CommandInterface *pcomInt,
                                                       UUIDVector &dependentslist,
                                                       CReport *pRpt,
                                                       CItemData::EntryType type,
                                                       int iVia)
  : Command(pcomInt), m_dependentslist(dependentslist), m_pRpt(pRpt),
    m_type(type), m_iVia(iVia)
{
  m_pmapDeletedItems = new ItemList;
  m_pmapSaveStatus = new SaveTypePWMap;
}

AddDependentEntriesCommand::~AddDependentEntriesCommand()
{
  delete m_pmapDeletedItems;
  delete m_pmapSaveStatus;
}

int AddDependentEntriesCommand::Execute(const bool /*bRedo*/)
{
  SaveState();

  if (m_type == CItemData::ET_ALIAS) {
    m_saved_base2aliases_mmap = m_pcomInt->GetBase2AliasesMmap();
    m_saved_alias2base_map = m_pcomInt->GetAlias2BaseMap();
  } else { // if !alias, assume shortcut
    m_saved_base2shortcuts_mmap = m_pcomInt->GetBase2ShortcutsMmap();
    m_saved_shortcut2base_map = m_pcomInt->GetShortcuts2BaseMap();
  }

  if (m_pcomInt->IsReadOnly())
    return 0;

  int rc =  m_pcomInt->DoAddDependentEntries(m_dependentslist, m_pRpt,
                                             m_type, m_iVia,
                                             m_pmapDeletedItems, m_pmapSaveStatus);
  m_bState = true;
  return rc;
}

void AddDependentEntriesCommand::Undo()
{
  if (m_pcomInt->IsReadOnly())
    return;

  m_pcomInt->UndoAddDependentEntries(m_pmapDeletedItems, m_pmapSaveStatus);
  if (m_type == CItemData::ET_ALIAS) {
    m_pcomInt->SetBase2AliasesMmap(m_saved_base2aliases_mmap);
    m_pcomInt->SetAlias2BaseMap(m_saved_alias2base_map);
  } else { // if !alias, assume shortcut
    m_pcomInt->SetBase2ShortcutsMmap(m_saved_base2shortcuts_mmap);
    m_pcomInt->SetShortcuts2BaseMap(m_saved_shortcut2base_map);
  }

  RestoreState();
  m_bState = false;
}

// ------------------------------------------------
// RemoveDependentEntryCommand
// ------------------------------------------------

RemoveDependentEntryCommand::RemoveDependentEntryCommand(CommandInterface *pcomInt,
                                                         const CUUID &base_uuid,
                                                         const CUUID &entry_uuid,
                                                         const CItemData::EntryType type)
  : Command(pcomInt), m_base_uuid(base_uuid),
    m_entry_uuid(entry_uuid), m_type(type)
{
}

int RemoveDependentEntryCommand::Execute(const bool /*bRedo*/)
{
  SaveState();

  if (m_pcomInt->IsReadOnly())
    return 0;

  m_pcomInt->DoRemoveDependentEntry(m_base_uuid, m_entry_uuid, m_type);
  m_bState = true;
  return 0;
}

void RemoveDependentEntryCommand::Undo()
{
  if (m_pcomInt->IsReadOnly())
    return;

  m_pcomInt->DoAddDependentEntry(m_base_uuid, m_entry_uuid, m_type);

  RestoreState();
  m_bState = false;
}

// ------------------------------------------------
// MoveDependentEntriesCommand
// ------------------------------------------------

MoveDependentEntriesCommand::MoveDependentEntriesCommand(CommandInterface *pcomInt,
                                                         const CUUID &from_baseuuid,
                                                         const CUUID &to_baseuuid,
                                                         const CItemData::EntryType type)
  : Command(pcomInt), m_from_baseuuid(from_baseuuid),
    m_to_baseuuid(to_baseuuid), m_type(type)
{
}

int MoveDependentEntriesCommand::Execute(const bool /*bRedo*/)
{
  SaveState();

  if (m_pcomInt->IsReadOnly())
    return 0;

  m_pcomInt->DoMoveDependentEntries(m_from_baseuuid, m_to_baseuuid, m_type);
  m_bState = true;
  return 0;
}

void MoveDependentEntriesCommand::Undo()
{
  if (m_pcomInt->IsReadOnly())
    return;

  m_pcomInt->DoMoveDependentEntries(m_to_baseuuid, m_from_baseuuid, m_type);

  RestoreState();
  m_bState = false;
}

// ------------------------------------------------
// UpdatePasswordHistoryCommand
// ------------------------------------------------

UpdatePasswordHistoryCommand::UpdatePasswordHistoryCommand(CommandInterface *pcomInt,
                                                           const int iAction,
                                                           const int new_default_max)
 : Command(pcomInt), m_iAction(iAction), m_new_default_max(new_default_max)
{}

int UpdatePasswordHistoryCommand::Execute(const bool /*bRedo*/)
{
  SaveState();


  if (m_pcomInt->IsReadOnly())
    return 0;

  int rc = m_pcomInt->DoUpdatePasswordHistory(m_iAction, m_new_default_max,
                                              m_mapSavedHistory);
  m_bState = true;
  return rc;
}

void UpdatePasswordHistoryCommand::Undo()
{
  if (m_pcomInt->IsReadOnly())
    return;

  m_pcomInt->UndoUpdatePasswordHistory(m_mapSavedHistory);

  RestoreState();
  m_bState = false;
}
