/// file MainManage.cpp
//
// Manage-related methods of DboxMain
//-----------------------------------------------------------------------------

#include "PasswordSafe.h"

#include "ThisMfcApp.h"

#include "corelib/pwsprefs.h"

// dialog boxen
#include "DboxMain.h"

#include "PasskeyChangeDlg.h"
#include "TryAgainDlg.h"
#include "OptionsSystem.h"
#include "OptionsSecurity.h"
#include "OptionsDisplay.h"
#include "OptionsPasswordPolicy.h"
#include "OptionsPasswordHistory.h"
#include "OptionsMisc.h"
#include "OptionsBackup.h"

#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Change the master password for the database.
void
DboxMain::OnPasswordChange()
{
  if (m_IsReadOnly) // disable in read-only mode
    return;
  CPasskeyChangeDlg changeDlg(this);
  app.DisableAccelerator();
  int rc = changeDlg.DoModal();
  app.EnableAccelerator();
  if (rc == IDOK) {
    m_core.ChangePassword(changeDlg.m_newpasskey);
  }
}

void
DboxMain::OnBackupSafe()
{
  BackupSafe();
}

int
DboxMain::BackupSafe()
{
  int rc;
  PWSprefs *prefs = PWSprefs::GetInstance();
  CMyString tempname;
  CMyString currbackup =
    prefs->GetPref(PWSprefs::CurrentBackup);


  //SaveAs-type dialog box
  while (1) {
    CFileDialog fd(FALSE,
                   _T("bak"),
                   currbackup,
                   OFN_PATHMUSTEXIST|OFN_HIDEREADONLY
                   | OFN_LONGNAMES|OFN_OVERWRITEPROMPT,
                   _T("Password Safe Backups (*.bak)|*.bak||"),
                   this);
    fd.m_ofn.lpstrTitle = _T("Please Choose a Name for this Backup:");

    rc = fd.DoModal();
    if (rc == IDOK) {
      tempname = (CMyString)fd.GetPathName();
      break;
    } else
      return PWScore::USER_CANCEL;
  }

  rc = m_core.WriteFile(tempname);
  if (rc == PWScore::CANT_OPEN_FILE) {
    CMyString temp = tempname + _T("\n\nCould not open file for writing!");
    MessageBox(temp, _T("File write error."), MB_OK|MB_ICONWARNING);
    return PWScore::CANT_OPEN_FILE;
  }

  prefs->SetPref(PWSprefs::CurrentBackup, tempname);
  return PWScore::SUCCESS;
}

void
DboxMain::OnRestore()
{
  if (!m_IsReadOnly) // disable in read-only mode
    Restore();
}

int
DboxMain::Restore()
{
  int rc;
  CMyString backup, passkey, temp;
  CMyString currbackup =
    PWSprefs::GetInstance()->GetPref(PWSprefs::CurrentBackup);

  rc = SaveIfChanged();
  if (rc != PWScore::SUCCESS)
    return rc;

  //Open-type dialog box
  while (1) {
    CFileDialog fd(TRUE,
                   _T("bak"),
                   currbackup,
                   OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_LONGNAMES,
                   _T("Password Safe Backups (*.bak)|*.bak|")
				   _T("Password Safe Intermediate Backups (*.ibak)|*.ibak|")
				   _T("|"),
                   this);
    fd.m_ofn.lpstrTitle = _T("Please Choose a Backup to restore:");
    rc = fd.DoModal();
    if (rc == IDOK) {
      backup = (CMyString)fd.GetPathName();
      break;
    } else
      return PWScore::USER_CANCEL;
  }

  rc = GetAndCheckPassword(backup, passkey, GCP_NORMAL);  // OK, CANCEL, HELP
  switch (rc) {
  case PWScore::SUCCESS:
    break; // Keep going...
  case PWScore::CANT_OPEN_FILE:
    temp = backup + _T("\n\nCan't open file. Please choose another.");
    MessageBox(temp, _T("File open error."), MB_OK | MB_ICONWARNING);
  case TAR_OPEN:
    ASSERT(0);
    return PWScore::FAILURE; // shouldn't be an option here
  case TAR_NEW:
    ASSERT(0);
    return PWScore::FAILURE; // shouldn't be an option here
  case PWScore::WRONG_PASSWORD:
  case PWScore::USER_CANCEL:
    /*
      If the user just cancelled out of the password dialog,
      assume they want to return to where they were before...
    */
    return PWScore::USER_CANCEL;
  }

  // unlock the file we're leaving
  if( !m_core.GetCurFile().IsEmpty() ) {
    m_core.UnlockFile(m_core.GetCurFile());
  }

  // clear the data before restoring
  ClearData();

  rc = m_core.ReadFile(backup, passkey);
  if (rc == PWScore::CANT_OPEN_FILE) {
    temp = backup + _T("\n\nCould not open file for reading!");
    MessageBox(temp, _T("File read error."), MB_OK|MB_ICONWARNING);
    return PWScore::CANT_OPEN_FILE;
  }

  m_core.SetCurFile(_T("")); //Force a Save As...
  m_core.SetChanged(Data); //So that the restored file will be saved
#if !defined(POCKET_PC)
  m_title = _T("Password Safe - <Untitled Restored Backup>");
  app.SetTooltipText(_T("PasswordSafe"));
#endif
  ChangeOkUpdate();
  RefreshList();

  return PWScore::SUCCESS;
}

void
DboxMain::OnValidate() 
{
  CString cs_msg;
  if (!m_core.Validate(cs_msg)) {
    cs_msg = _T("Database validated - no problems found.");
			}
	AfxMessageBox(cs_msg, MB_OK);
}

void
DboxMain::OnOptions() 
{
  CPropertySheet optionsDlg(_T("Options"), this);
  COptionsDisplay         display;
  COptionsSecurity        security;
  COptionsPasswordPolicy  passwordpolicy;
  COptionsPasswordHistory passwordhistory;
  COptionsSystem          system;
  COptionsMisc            misc;
  COptionsBackup          backup;
  PWSprefs               *prefs = PWSprefs::GetInstance();
  BOOL                    prevLockOIT; // lock on idle timeout set?
  BOOL                    brc, save_hotkey_enabled;
  DWORD                   save_hotkey_value;
  /*
  **  Initialize the property pages values.
  */
  system.m_maxreitems = prefs->
    GetPref(PWSprefs::MaxREItems);
  system.m_usesystemtray = prefs->
    GetPref(PWSprefs::UseSystemTray) ? TRUE : FALSE;
  system.m_maxmruitems = prefs->
    GetPref(PWSprefs::MaxMRUItems);
  system.m_mruonfilemenu = prefs->
    GetPref(PWSprefs::MRUOnFileMenu);
  system.m_offerdeleteregistry = 
  	(prefs->GetConfigOptions() == PWSprefs::CF_FILE_RW) && prefs->GetRegistryExistence();

  display.m_alwaysontop = m_bAlwaysOnTop;
  display.m_pwshowinedit = prefs->
    GetPref(PWSprefs::ShowPWDefault) ? TRUE : FALSE;
  display.m_pwshowinlist = prefs->
    GetPref(PWSprefs::ShowPWInList) ? TRUE : FALSE;
  display.m_notesshowinedit = prefs->
    GetPref(PWSprefs::ShowNotesDefault) ? TRUE : FALSE;
#if defined(POCKET_PC)
  display.m_dcshowspassword = prefs->
    GetPref(PWSprefs::DCShowsPassword) ? TRUE : FALSE;
#endif
  // by strange coincidence, the values of the enums match the indices
  // of the radio buttons in the following :-)
  display.m_treedisplaystatusatopen = prefs->
    GetPref(PWSprefs::TreeDisplayStatusAtOpen);

  security.m_clearclipboard = prefs->
    GetPref(PWSprefs::DontAskMinimizeClearYesNo) ? TRUE : FALSE;
  security.m_lockdatabase = prefs->
    GetPref(PWSprefs::DatabaseClear) ? TRUE : FALSE;
  security.m_confirmcopy = prefs->
    GetPref(PWSprefs::DontAskQuestion) ? FALSE : TRUE;
  security.m_LockOnWindowLock = prefs->
    GetPref(PWSprefs::LockOnWindowLock) ? TRUE : FALSE;
  security.m_LockOnIdleTimeout = prevLockOIT = prefs->
    GetPref(PWSprefs::LockOnIdleTimeout) ? TRUE : FALSE;
  security.m_IdleTimeOut = prefs->
    GetPref(PWSprefs::IdleTimeout);

  passwordpolicy.m_pwlendefault = prefs->
    GetPref(PWSprefs::PWLenDefault);
  passwordpolicy.m_pwuselowercase = prefs->
    GetPref(PWSprefs::PWUseLowercase);
  passwordpolicy.m_pwuseuppercase = prefs->
    GetPref(PWSprefs::PWUseUppercase);
  passwordpolicy.m_pwusedigits = prefs->
    GetPref(PWSprefs::PWUseDigits);
  passwordpolicy.m_pwusesymbols = prefs->
    GetPref(PWSprefs::PWUseSymbols);
  passwordpolicy.m_pwusehexdigits = prefs->
    GetPref(PWSprefs::PWUseHexDigits);
  passwordpolicy.m_pweasyvision = prefs->
    GetPref(PWSprefs::PWEasyVision);

  passwordhistory.m_savepwhistory = prefs->
    GetPref(PWSprefs::SavePasswordHistory) ? TRUE : FALSE;
  passwordhistory.m_pwhistorynumdefault = prefs->
    GetPref(PWSprefs::NumPWHistoryDefault);

  misc.m_confirmdelete = prefs->
    GetPref(PWSprefs::DeleteQuestion) ? FALSE : TRUE;
  misc.m_maintaindatetimestamps = prefs->
    GetPref(PWSprefs::MaintainDateTimeStamps) ? TRUE : FALSE;
  misc.m_escexits = prefs->
    GetPref(PWSprefs::EscExits) ? TRUE : FALSE;
  // by strange coincidence, the values of the enums match the indices
  // of the radio buttons in the following :-)
  misc.m_doubleclickaction = prefs->
    GetPref(PWSprefs::DoubleClickAction);

  save_hotkey_value = misc.m_hotkey_value = 
    DWORD(prefs->GetPref(PWSprefs::HotKey));
  // Can't be enabled if not set!
  if (misc.m_hotkey_value == 0)
	  save_hotkey_enabled = misc.m_hotkey_enabled = FALSE;
  else
	  save_hotkey_enabled = misc.m_hotkey_enabled = prefs->
         GetPref(PWSprefs::HotKeyEnabled) ? TRUE : FALSE;

  misc.m_usedefuser = prefs->
    GetPref(PWSprefs::UseDefUser) ? TRUE : FALSE;
  misc.m_defusername = CString(prefs->
    GetPref(PWSprefs::DefUserName));
  misc.m_querysetdef = prefs->
    GetPref(PWSprefs::QuerySetDef) ? TRUE : FALSE;

  backup.m_saveimmediately = prefs->
    GetPref(PWSprefs::SaveImmediately) ? TRUE : FALSE;
  backup.m_backupbeforesave = prefs->
    GetPref(PWSprefs::BackupBeforeEverySave) ? TRUE : FALSE;
  backup.m_backupprefix = prefs->
    GetPref(PWSprefs::BackupPrefix);
  backup.m_userbackupprefix = CString(prefs->
    GetPref(PWSprefs::BackupPrefixValue));
  backup.m_backupsuffix = prefs->
    GetPref(PWSprefs::BackupSuffix);
  backup.m_maxnumincbackups = prefs->
    GetPref(PWSprefs::BackupMaxIncremented);
  backup.m_backuplocation = prefs->
    GetPref(PWSprefs::BackupLocation);
  backup.m_userbackupsubdirectory = CString(prefs->
    GetPref(PWSprefs::BackupSubDirectoryValue));
  backup.m_userbackupotherlocation = CString(prefs->
    GetPref(PWSprefs::BackupOtherLocationValue));

  optionsDlg.AddPage( &backup );
  optionsDlg.AddPage( &display );
  optionsDlg.AddPage( &misc );
  optionsDlg.AddPage( &passwordpolicy );
  optionsDlg.AddPage( &passwordhistory );
  optionsDlg.AddPage( &security );
  optionsDlg.AddPage( &system );

  /*
  **  Remove the "Apply Now" button.
  */
  optionsDlg.m_psh.dwFlags |= PSH_NOAPPLYNOW;

  // Disable Hotkey around this as the user may press the current key when 
  // selecting the new key!

#if !defined(POCKET_PC)
  brc = UnregisterHotKey(m_hWnd, PWS_HOTKEY_ID); // clear last - never hurts
#endif

  passwordhistory.m_pDboxMain = this;
  app.DisableAccelerator();
  int rc = optionsDlg.DoModal();
  app.EnableAccelerator();

  if (rc == IDOK)
    {
      if (system.m_deleteregistry == TRUE) {
          const CString cs_msg = _T("Please confirm that you wish to delete ALL information related to Password Safe for your logged on userid from the Windows registry on this computer!\n\n");
          rc = AfxMessageBox(cs_msg, MB_YESNO | MB_ICONSTOP);
          if (rc == IDYES) {
          	prefs->DeleteRegistryEntries();
          }
      }
      /*
      **  First save all the options.
      */
      prefs->SetPref(PWSprefs::AlwaysOnTop,
                     display.m_alwaysontop == TRUE);
      prefs->SetPref(PWSprefs::ShowPWDefault,
                     display.m_pwshowinedit == TRUE);
      prefs->SetPref(PWSprefs::ShowPWInList,
                     display.m_pwshowinlist == TRUE);
      prefs->SetPref(PWSprefs::ShowNotesDefault,
                     display.m_notesshowinedit == TRUE);                   
#if defined(POCKET_PC)
      prefs->SetPref(PWSprefs::DCShowsPassword,
                     display.m_dcshowspassword == TRUE);
#endif
      // by strange coincidence, the values of the enums match the indices
      // of the radio buttons in the following :-)
      prefs->SetPref(PWSprefs::TreeDisplayStatusAtOpen,
                     display.m_treedisplaystatusatopen);

      prefs->SetPref(PWSprefs::UseSystemTray,
                     system.m_usesystemtray == TRUE);
      prefs->SetPref(PWSprefs::MaxREItems,
                     system.m_maxreitems);
      prefs->SetPref(PWSprefs::MaxMRUItems,
                     system.m_maxmruitems);
      prefs->SetPref(PWSprefs::MRUOnFileMenu,
                     system.m_mruonfilemenu == TRUE);

      prefs->SetPref(PWSprefs::DontAskMinimizeClearYesNo,
                     security.m_clearclipboard == TRUE);
      prefs->SetPref(PWSprefs::DatabaseClear,
                     security.m_lockdatabase == TRUE);
      prefs->SetPref(PWSprefs::DontAskQuestion,
                     security.m_confirmcopy == FALSE);
      prefs->SetPref(PWSprefs::LockOnWindowLock,
                     security.m_LockOnWindowLock == TRUE);
      prefs->SetPref(PWSprefs::LockOnIdleTimeout,
                     security.m_LockOnIdleTimeout == TRUE);
      prefs->SetPref(PWSprefs::IdleTimeout,
                     security.m_IdleTimeOut);

      prefs->SetPref(PWSprefs::PWLenDefault,
                     passwordpolicy.m_pwlendefault);
      prefs->SetPref(PWSprefs::PWUseLowercase,
                     passwordpolicy.m_pwuselowercase == TRUE);
      prefs->SetPref(PWSprefs::PWUseUppercase,
                     passwordpolicy.m_pwuseuppercase == TRUE);
      prefs->SetPref(PWSprefs::PWUseDigits,
                     passwordpolicy.m_pwusedigits == TRUE);
      prefs->SetPref(PWSprefs::PWUseSymbols,
                     passwordpolicy.m_pwusesymbols == TRUE);
      prefs->SetPref(PWSprefs::PWUseHexDigits,
                     passwordpolicy.m_pwusehexdigits == TRUE);
      prefs-> SetPref(PWSprefs::PWEasyVision,
                      passwordpolicy.m_pweasyvision == TRUE);

      prefs->SetPref(PWSprefs::SavePasswordHistory,
      				 passwordhistory.m_savepwhistory == TRUE);
      if (passwordhistory.m_savepwhistory == TRUE)
          prefs->SetPref(PWSprefs::NumPWHistoryDefault,
                     passwordhistory.m_pwhistorynumdefault);

      prefs->SetPref(PWSprefs::DeleteQuestion,
                     misc.m_confirmdelete == FALSE);
      prefs->SetPref(PWSprefs::MaintainDateTimeStamps,
                     misc.m_maintaindatetimestamps == TRUE);
      prefs->SetPref(PWSprefs::EscExits,
                     misc.m_escexits == TRUE);
      // by strange coincidence, the values of the enums match the indices
      // of the radio buttons in the following :-)
      prefs->SetPref(PWSprefs::DoubleClickAction,
                     misc.m_doubleclickaction);

      // Need to update previous values as we use these variables to re-instate
      // the hotkey environment at the end whether the user changed it or not.
      prefs->SetPref(PWSprefs::HotKey,
                     misc.m_hotkey_value);
      save_hotkey_value = misc.m_hotkey_value;
	  // Can't be enabled if not set!
	  if (misc.m_hotkey_value == 0)
		  save_hotkey_enabled = misc.m_hotkey_enabled = FALSE;

      prefs->SetPref(PWSprefs::HotKeyEnabled,
                     misc.m_hotkey_enabled == TRUE);

      prefs->SetPref(PWSprefs::UseDefUser,
                     misc.m_usedefuser == TRUE);
      prefs->SetPref(PWSprefs::DefUserName,
                     misc.m_defusername);
      prefs->SetPref(PWSprefs::QuerySetDef,
                     misc.m_querysetdef == TRUE);

      prefs->SetPref(PWSprefs::SaveImmediately,
                     backup.m_saveimmediately == TRUE);
      prefs->SetPref(PWSprefs::BackupBeforeEverySave,
                     backup.m_backupbeforesave == TRUE);
      prefs->SetPref(PWSprefs::BackupPrefix,
                     backup.m_backupprefix);
      prefs->SetPref(PWSprefs::BackupPrefixValue,
                     backup.m_userbackupprefix);
      prefs->SetPref(PWSprefs::BackupSuffix,
                     backup.m_backupsuffix);
	  prefs->SetPref(PWSprefs::BackupMaxIncremented,
                     backup.m_maxnumincbackups);
      prefs->SetPref(PWSprefs::BackupLocation,
                     backup.m_backuplocation);    
      prefs->SetPref(PWSprefs::BackupSubDirectoryValue,
                     backup.m_userbackupsubdirectory);
      prefs->SetPref(PWSprefs::BackupOtherLocationValue,
                     backup.m_userbackupotherlocation);

      // JHF : no status bar under WinCE (was already so in the .h file !?!)
#if !defined(POCKET_PC)
      /* Update status bar */
      switch (misc.m_doubleclickaction) {
      	case PWSprefs::DoubleClickAutoType: statustext[SB_DBLCLICK] = IDS_STATAUTOTYPE; break;
      	case PWSprefs::DoubleClickBrowse: statustext[SB_DBLCLICK] = IDS_STATBROWSE; break;
      	case PWSprefs::DoubleClickCopyNotes: statustext[SB_DBLCLICK] = IDS_STATCOPYNOTES; break;
      	case PWSprefs::DoubleClickCopyPassword: statustext[SB_DBLCLICK] = IDS_STATCOPYPASSWORD; break;
      	case PWSprefs::DoubleClickCopyUsername: statustext[SB_DBLCLICK] = IDS_STATCOPYUSERNAME; break;
		case PWSprefs::DoubleClickViewEdit: statustext[SB_DBLCLICK] = IDS_STATVIEWEDIT; break;
		default: ASSERT(0);
      }
      m_statusBar.SetIndicators(statustext, SB_TOTAL);
	  UpdateStatusBar();
	  // Make a sunken or recessed border around the first pane
      m_statusBar.SetPaneInfo(SB_DBLCLICK, m_statusBar.GetItemID(SB_DBLCLICK), SBPS_STRETCH, NULL);
#endif

      /*
      ** Update string in database, if necessary & possible
      */
      if (prefs->IsDBprefsChanged() && !app.m_core.GetCurFile().IsEmpty() &&
          m_core.GetReadFileVersion() == PWSfile::VCURRENT) {
        // save changed preferences to file
        // Note that we currently can only write the entire file, so any changes
        // the user made to the database are also saved here
        m_core.BackupCurFile(); // try to save previous version
        if (app.m_core.WriteCurFile() != PWScore::SUCCESS)
          MessageBox(_T("Failed to save changed preferences"), AfxGetAppName());
        else
          prefs->ClearDBprefsChanged();
      }
      /*
      **  Now update the application according to the options.
      */
      m_bAlwaysOnTop = display.m_alwaysontop == TRUE;
      UpdateAlwaysOnTop();
      bool bOldShowPasswordInList = m_bShowPasswordInList;
      m_bShowPasswordInList = prefs->
        GetPref(PWSprefs::ShowPWInList);

      if (bOldShowPasswordInList != m_bShowPasswordInList)
		RefreshList();
	
      if (system.m_usesystemtray == TRUE) {
		if (app.IsIconVisible() == FALSE)
          app.ShowIcon();
	  } else { // user doesn't want to display
		if (app.IsIconVisible() == TRUE)
          app.HideIcon();
      }
      m_RUEList.SetMax(system.m_maxreitems);

      // update idle timeout values, if changed
      if (security.m_LockOnIdleTimeout != prevLockOIT)
        if (security.m_LockOnIdleTimeout == TRUE) {
          const UINT MINUTE = 60*1000;
          SetTimer(TIMER_USERLOCK, MINUTE, NULL);
        } else {
          KillTimer(TIMER_USERLOCK);
        }
      SetIdleLockCounter(security.m_IdleTimeOut);

      /*
       * Here are the old (pre 2.0) semantics:
       * The username entered in this dialog box will be added to all the entries
       * in the username-less database that you just opened. Click Ok to add the
       * username or Cancel to leave them as is.
       *
       * You can also set this username to be the default username by clicking the
       * check box.  In this case, you will not see the username that you just added
       * in the main dialog (though it is still part of the entries), and it will
       * automatically be inserted in the Add dialog for new entries.
       *
       * To me (ronys), these seem too complicated, and not useful once password files
       * have been converted to the old (username-less) format to 1.9 (with usernames).
       * (Not to mention 2.0).
       * Therefore, the username will now only be a default value to be used in new entries,
       * and in converting pre-2.0 databases.
       */

      m_core.SetDefUsername(misc.m_defusername);
      m_core.SetUseDefUser(misc.m_usedefuser == TRUE ? true : false);
    }
      // JHF no hotkeys under WinCE
#if !defined(POCKET_PC)
    // Restore hotkey as it was or as user changed it - if he/she pressed OK
    if (save_hotkey_enabled == TRUE) {
      WORD wVirtualKeyCode = WORD(save_hotkey_value & 0xffff);
      WORD mod = WORD(save_hotkey_value >> 16);
		WORD wModifiers = 0;
		// Translate between CWnd & CHotKeyCtrl modifiers
		if (mod & HOTKEYF_ALT) 
			wModifiers |= MOD_ALT; 
		if (mod & HOTKEYF_CONTROL) 
			wModifiers |= MOD_CONTROL; 
		if (mod & HOTKEYF_SHIFT) 
			wModifiers |= MOD_SHIFT; 
        brc = RegisterHotKey(m_hWnd, PWS_HOTKEY_ID,
                       UINT(wModifiers), UINT(wVirtualKeyCode));
		if (brc == FALSE)
			AfxMessageBox(IDS_NOHOTKEY, MB_OK);
      }
#endif
}

void
DboxMain::UpdatePasswordHistory(const int &iAction, const int &new_default_max)
{
	ItemList new_pwlist;
	CString cs_Msg, cs_Buffer;;
	POSITION listPos;
	int status, old_max, num_saved, len;
	int num_altered = 0;
	bool bResult = false;

	switch (iAction) {
		case 1:		// reset off
			listPos = m_core.GetFirstEntryPosition();
			while (listPos != NULL) {
				CItemData &ci = m_core.GetEntryAt(listPos);
				CMyString cs_tmp = ci.GetPWHistory();
				if (cs_tmp.GetLength() >= 5 && cs_tmp.GetAt(0) == _T('1')) {
					cs_tmp.SetAt(0, _T('0'));
					ci.SetPWHistory(cs_tmp);
					num_altered++;
				}
				new_pwlist.AddTail(ci);
				m_core.GetNextEntry(listPos);
			} // while
			if (num_altered > 0) {
				// items were changed - swap the in-core pwlist
				m_core.CopyPWList(new_pwlist);
			}
			if (num_altered == 1)
				cs_Msg = _T("1 entry has had its");
			else
				cs_Msg.Format(_T("%d entries have had their"), num_altered);
			cs_Msg += _T(" settings changed to not save password history.");
			AfxMessageBox(cs_Msg);
			bResult = true;
			break;
		case 2:		// reset on
			cs_Buffer.Format("1%02x00", new_default_max);
			listPos = m_core.GetFirstEntryPosition();
			while (listPos != NULL) {
				CItemData &ci = m_core.GetEntryAt(listPos);
				CMyString cs_tmp = ci.GetPWHistory();
				len = cs_tmp.GetLength();
				if (cs_tmp.GetLength() < 5) {
					ci.SetPWHistory(cs_Buffer);
					num_altered++;
				} else {
					if (cs_tmp.GetAt(0) == _T('0')) {
							cs_tmp.SetAt(0, _T('1'));
						ci.SetPWHistory(cs_tmp);
						num_altered++;
					}
				}
				new_pwlist.AddTail(ci);
				m_core.GetNextEntry(listPos);
			} // while
			if (num_altered > 0) {
				// items were changed - swap the in-core pwlist
				m_core.CopyPWList(new_pwlist);
			}
			if (num_altered == 1)
				cs_Msg = _T("1 entry has had its");
			else
				cs_Msg.Format(_T("%d entries have had their"), num_altered);
			cs_Msg += _T(" settings changed to save password history.");
			AfxMessageBox(cs_Msg);
			bResult = true;
			break;
		case 3:		// setmax
			cs_Buffer.Format("1%02x", new_default_max);
			listPos = m_core.GetFirstEntryPosition();
			while (listPos != NULL) {
				CItemData &ci = m_core.GetEntryAt(listPos);
				CMyString cs_tmp = ci.GetPWHistory();
				len = cs_tmp.GetLength();
				if (len >= 5) {
					TCHAR *lpszPWHistory = cs_tmp.GetBuffer(len + sizeof(TCHAR));
#if _MSC_VER >= 1400
					int iread = sscanf_s(lpszPWHistory, "%01d%02x%02x", 
						&status, &old_max, &num_saved);
#else
					int iread = sscanf(lpszPWHistory, "%01d%02x%02x",
						&status, &old_max, &num_saved);
#endif
					cs_tmp.ReleaseBuffer();
					if (iread == 3 && status == 1 && num_saved <= new_default_max) {
						cs_tmp = CMyString(cs_Buffer) + cs_tmp.Mid(3);
						ci.SetPWHistory(cs_tmp);
						num_altered++;
					}
				}
				new_pwlist.AddTail(ci);
				m_core.GetNextEntry(listPos);
			} // while
			if (num_altered > 0) {
				// items were changed - swap the in-core pwlist
				m_core.CopyPWList(new_pwlist);
			}
			if (num_altered == 1)
				cs_Msg = _T("1 entry has had its");
			else
				cs_Msg.Format(_T("%d entries have had their"), num_altered);
			cs_Msg += _T(" 'maximum saved passwords' changed to the new default.");
			AfxMessageBox(cs_Msg);
			bResult = true;
			break;
		default:
			break;
	}

	new_pwlist.RemoveAll();

	if (bResult)
		RefreshList();

	return;
}
