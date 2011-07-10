/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "PWPropertyPage.h"
#include "SecString.h"

class COptions_PropertySheet;
class DboxMain;
class PWScore;

// Database option text colour (COLORREF) - equivalent to RGB(0, 0, 128)
#define CR_DATABASE_OPTIONS 0x800000

struct st_Opt_master_data {
  UINT uicaller;   // Options, New Database, Generate

  DboxMain *pDbx;

  CSecString CurrentFile;
  CSecString UserBackupPrefix;
  CSecString UserBackupOtherLocation;
  BOOL SaveImmediately;
  BOOL BackupBeforeSave;
  BOOL BackupPrefix;
  int BackupLocation;
  int BackupSuffix;
  int MaxNumIncBackups;

  // Display Data
  BOOL AlwaysOnTop;
  BOOL ShowPasswordInEdit;
  BOOL ShowUsernameInTree;
  BOOL ShowPasswordInTree;
  BOOL ShowNotesAsTipsInViews;
  BOOL ExplorerTypeTree;
  BOOL EnableGrid;
  BOOL NotesShowInEdit;
  BOOL WordWrapNotes;
  BOOL PreExpiryWarn;
  BOOL HighlightChanges;
  int PreExpiryWarnDays;
  int TreeDisplayStatusAtOpen;
  int TrayIconColour;
  
  // Misc Data
  BOOL ConfirmDelete;
  BOOL MaintainDatetimeStamps;
  BOOL EscExits;
  int DoubleClickAction, ShiftDoubleClickAction;

  DWORD Hotkey_Value;
  BOOL Hotkey_Enabled;

  CSecString DefUsername;
  CSecString OtherBrowserLocation;
  CSecString BrowserCmdLineParms;
  CSecString OtherEditorLocation;
  CSecString Autotype;
  BOOL UseDefuser;
  BOOL QuerySetDef;
  BOOL MinAuto;

  // Password History Data
  BOOL SavePWHistory;
  int PWHistoryNumDefault;
  int PWHAction;

  // Password Policy Data
  CString Symbols;
  BOOL PWUseLowercase;
  BOOL PWUseUppercase;
  BOOL PWUseDigits;
  BOOL PWUseSymbols;
  BOOL PWUseHexdigits;
  BOOL PWEasyVision;
  BOOL PWMakePronounceable;
  int PWDefaultLength;
  int PWDigitMinLength;
  int PWLowerMinLength;
  int PWSymbolMinLength;
  int PWUpperMinLength;
  int UseOwnSymbols;

  // Security Data
  CString EraseProgram;
  CString ErasePgmCmdLineParms;
  BOOL ClearClipboardOnMinimize;
  BOOL ClearClipboardOnExit;
  BOOL LockOnMinimize;
  BOOL ConfirmCopy;
  BOOL LockOnWindowLock;
  BOOL LockOnIdleTimeout;
  BOOL CopyPswdBrowseURL;
  int IdleTimeOut;

  // Shortcut Data
  int ColWidth;
  int DefColWidth;

  // System Data
  BOOL UseSystemTray;
  BOOL HideSystemTray;
  BOOL MRUOnFileMenu;
  BOOL Startup;
  BOOL DefaultOpenRO;
  BOOL MultipleInstances;
  int MaxREItems;
  int MaxMRUItems;
};

class COptions_PropertyPage : public CPWPropertyPage
{
public:
  COptions_PropertyPage(CWnd *pParent, UINT nID, st_Opt_master_data *pOPTMD);
  virtual ~COptions_PropertyPage() {}

  virtual BOOL OnQueryCancel();

  // Retrieve DoubleClickAction or ClearClipboardOnMimimize
  // or if Hot Key set
  // Make sure no overlap with 'PP_' enum in CPWPropertyPage
  enum {PPOPT_GET_DCA = 10, PPOPT_GET_CCOM, PPOPT_HOTKEY_SET};

  DECLARE_DYNAMIC(COptions_PropertyPage)

  inline UINT &M_uicaller() {return m_OPTMD.uicaller;}
  inline DboxMain * &M_pDbx() {return m_OPTMD.pDbx;}

  // Backup Data
  inline CSecString &M_CurrentFile() {return m_OPTMD.CurrentFile;}
  inline CString &M_UserBackupPrefix() {return m_OPTMD.UserBackupPrefix;}
  inline CString &M_UserBackupOtherLocation() {return m_OPTMD.UserBackupOtherLocation;}
  inline BOOL &M_SaveImmediately() {return m_OPTMD.SaveImmediately;}
  inline BOOL &M_BackupBeforeSave() {return m_OPTMD.BackupBeforeSave;}
  inline BOOL &M_BackupPrefix() {return m_OPTMD.BackupPrefix;}
  inline int &M_BackupLocation() {return m_OPTMD.BackupLocation;}
  inline int &M_BackupSuffix() {return m_OPTMD.BackupSuffix;}
  inline int &M_MaxNumIncBackups() {return m_OPTMD.MaxNumIncBackups;}

  // Display Data
  inline BOOL &M_AlwaysOnTop() {return m_OPTMD.AlwaysOnTop;}
  inline BOOL &M_ShowPasswordInEdit() {return m_OPTMD.ShowPasswordInEdit;}
  inline BOOL &M_ShowUsernameInTree() {return m_OPTMD.ShowUsernameInTree;}
  inline BOOL &M_ShowPasswordInTree() {return m_OPTMD.ShowPasswordInTree;}
  inline BOOL &M_ShowNotesAsTipsInViews() {return m_OPTMD.ShowNotesAsTipsInViews;}
  inline BOOL &M_ExplorerTypeTree() {return m_OPTMD.ExplorerTypeTree;}
  inline BOOL &M_EnableGrid() {return m_OPTMD.EnableGrid;}
  inline BOOL &M_NotesShowInEdit() {return m_OPTMD.NotesShowInEdit;}
  inline BOOL &M_WordWrapNotes() {return m_OPTMD.WordWrapNotes;}
  inline BOOL &M_PreExpiryWarn() {return m_OPTMD.PreExpiryWarn;}
  inline BOOL &M_HighlightChanges() {return m_OPTMD.HighlightChanges;}
  inline int &M_PreExpiryWarnDays() {return m_OPTMD.PreExpiryWarnDays;}
  inline int &M_TreeDisplayStatusAtOpen() {return m_OPTMD.TreeDisplayStatusAtOpen;}
  inline int &M_TrayIconColour() {return m_OPTMD.TrayIconColour;}
  
  // Misc Data
  inline BOOL &M_ConfirmDelete() {return m_OPTMD.ConfirmDelete;}
  inline BOOL &M_MaintainDatetimeStamps() {return m_OPTMD.MaintainDatetimeStamps;}
  inline BOOL &M_EscExits() {return m_OPTMD.EscExits;}
  inline int &M_DoubleClickAction() {return m_OPTMD.DoubleClickAction;}
  inline int &M_ShiftDoubleClickAction() {return m_OPTMD.ShiftDoubleClickAction;}

  inline DWORD &M_Hotkey_Value() {return m_OPTMD.Hotkey_Value;}
  inline BOOL &M_Hotkey_Enabled() {return m_OPTMD.Hotkey_Enabled;}

  inline CSecString &M_DefUsername() {return m_OPTMD.DefUsername;}
  inline CString &M_OtherBrowserLocation() {return m_OPTMD.OtherBrowserLocation;}
  inline CString &M_BrowserCmdLineParms() {return m_OPTMD.BrowserCmdLineParms;}
  inline CString &M_OtherEditorLocation() {return m_OPTMD.OtherEditorLocation;}
  inline CString &M_Autotype() {return m_OPTMD.Autotype;}
  inline BOOL &M_UseDefUsername() {return m_OPTMD.UseDefuser;}
  inline BOOL &M_QuerySetDefUsername() {return m_OPTMD.QuerySetDef;}
  inline BOOL &M_AutotypeMinimize() {return m_OPTMD.MinAuto;}

  // Password History Data
  inline BOOL &M_SavePWHistory() {return m_OPTMD.SavePWHistory;}
  inline int &M_PWHistoryNumDefault() {return m_OPTMD.PWHistoryNumDefault;}
  inline int &M_PWHAction() {return m_OPTMD.PWHAction;}

  // Password Policy Data
  inline CString &M_Symbols() {return m_OPTMD.Symbols;}
  inline BOOL &M_PWUseLowercase() {return m_OPTMD.PWUseLowercase;}
  inline BOOL &M_PWUseUppercase() {return m_OPTMD.PWUseUppercase;}
  inline BOOL &M_PWUseDigits() {return m_OPTMD.PWUseDigits;}
  inline BOOL &M_PWUseSymbols() {return m_OPTMD.PWUseSymbols;}
  inline BOOL &M_PWUseHexdigits() {return m_OPTMD.PWUseHexdigits;}
  inline BOOL &M_PWEasyVision() {return m_OPTMD.PWEasyVision;}
  inline BOOL &M_PWMakePronounceable() {return m_OPTMD.PWMakePronounceable;}
  inline int &M_PWDefaultLength() {return m_OPTMD.PWDefaultLength;}
  inline int &M_PWDigitMinLength() {return m_OPTMD.PWDigitMinLength;}
  inline int &M_PWLowerMinLength() {return m_OPTMD.PWLowerMinLength;}
  inline int &M_PWSymbolMinLength() {return m_OPTMD.PWSymbolMinLength;}
  inline int &M_PWUpperMinLength() {return m_OPTMD.PWUpperMinLength;}
  inline int &M_UseOwnSymbols() {return m_OPTMD.UseOwnSymbols;}

  // Security Data
  inline CString &M_EraseProgram() {return m_OPTMD.EraseProgram;}
  inline CString &M_ErasePgmCmdLineParms() {return m_OPTMD.ErasePgmCmdLineParms;}
  inline BOOL &M_ClearClipboardOnMinimize() {return m_OPTMD.ClearClipboardOnMinimize;}
  inline BOOL &M_ClearClipboardOnExit() {return m_OPTMD.ClearClipboardOnExit;}
  inline BOOL &M_LockOnMinimize() {return m_OPTMD.LockOnMinimize;}
  inline BOOL &M_ConfirmCopy() {return m_OPTMD.ConfirmCopy;}
  inline BOOL &M_LockOnWindowLock() {return m_OPTMD.LockOnWindowLock;}
  inline BOOL &M_LockOnIdleTimeout() {return m_OPTMD.LockOnIdleTimeout;}
  inline BOOL &M_CopyPswdBrowseURL() {return m_OPTMD.CopyPswdBrowseURL;}
  inline int &M_IdleTimeOut() {return m_OPTMD.IdleTimeOut;}

  // Shortcut Data
  inline int &M_ColWidth() {return m_OPTMD.ColWidth;}
  inline int &M_DefColWidth() {return m_OPTMD.DefColWidth;}

  // System Data
  inline BOOL &M_UseSystemTray() {return m_OPTMD.UseSystemTray;}
  inline BOOL &M_HideSystemTray() {return m_OPTMD.HideSystemTray;}
  inline BOOL &M_MRUOnFileMenu() {return m_OPTMD.MRUOnFileMenu;}
  inline BOOL &M_Startup() {return m_OPTMD.Startup;}
  inline BOOL &M_DefaultOpenRO() {return m_OPTMD.DefaultOpenRO;}
  inline BOOL &M_MultipleInstances() {return m_OPTMD.MultipleInstances;}
  inline int &M_MaxREItems() {return m_OPTMD.MaxREItems;}
  inline int &M_MaxMRUItems() {return m_OPTMD.MaxMRUItems;}

protected:
  virtual BOOL OnInitDialog();
  COptions_PropertySheet *m_options_psh;
  st_Opt_master_data &m_OPTMD;
};
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
