/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// ItemData.h
//-----------------------------------------------------------------------------

#ifndef __ITEMDATA_H
#define __ITEMDATA_H

#include "Util.h"
#include "Match.h"
#include "ItemField.h"
#include "PWSprefs.h"
#include "PWPolicy.h"
#include "os/UUID.h"
#include "StringX.h"

#include <time.h> // for time_t
#include <bitset>
#include <vector>
#include <string>

typedef std::vector<CItemField> UnknownFields;
typedef UnknownFields::const_iterator UnknownFieldsConstIter;

//-----------------------------------------------------------------------------

/*
* CItemData is a class that contains the data present in a password entry
*
* 'Name' is the pre-2.x field, that had both the entry title and the
* username rolled-in together, separated by SPLTCHR (defined in util.h).
* In 2.0 and later, this field is unused, and the title and username
* are stored in separate fields.
*
* What makes this class interesting is that all fields are kept encrypted
* from the moment of construction, and are decrypted by the appropriate
* accessor (Get* member function).
*
* All this is to protect the data in memory, and has nothing to do with
* how the records are written to disk.
*/

class BlowFish;

struct DisplayInfoBase
{
  // Following used by display methods of the GUI
  DisplayInfoBase() {}
  virtual ~DisplayInfoBase() {}
  virtual DisplayInfoBase *clone() const = 0; // virtual c'tor idiom
};

class CItemData
{
public:
  // field types, per formatV{2,3}.txt. Any value > 0xff is internal only!
  enum FieldType {
    START = 0x00, GROUPTITLE = 0x00 /* reusing depreciated NAME for Group.Title combination */,
    NAME = 0x00, UUID = 0x01, GROUP = 0x02, TITLE = 0x03, USER = 0x04, NOTES = 0x05,
    PASSWORD = 0x06, CTIME = 0x07, PMTIME = 0x08, ATIME = 0x09, XTIME = 0x0a,
    RESERVED = 0x0b /* cannot use */, RMTIME = 0x0c, URL = 0x0d, AUTOTYPE = 0x0e,
    PWHIST = 0x0f, POLICY = 0x10, XTIME_INT = 0x11, RUNCMD = 0x12, DCA = 0x13,
    EMAIL = 0x14, PROTECTED = 0x15, SYMBOLS = 0x16, SHIFTDCA = 0x17,
    POLICYNAME = 0x18,
    LAST,        // Start of unknown fields!
    END = 0xff,
    // Internal fields only - used in filters
    ENTRYSIZE = 0x100, ENTRYTYPE = 0x101, ENTRYSTATUS  = 0x102, PASSWORDLEN = 0x103,
    // 'UNKNOWNFIELDS' should be last
    UNKNOWNFIELDS = 0x104};

  // SubGroup Object - same as FieldType

  // Status returns from "ProcessInputRecordField"
  enum {SUCCESS = 0, FAILURE, END_OF_FILE = 8};

  // Entry type (note: powers of 2)
  enum EntryType {ET_INVALID      = -1,
                  ET_NORMAL       =  0, 
                  ET_ALIASBASE    =  1, ET_ALIAS    = 2, 
                  ET_SHORTCUTBASE =  4, ET_SHORTCUT = 8,
                  ET_LAST};

  // Entry status (note: powers of 2)
  // A status can (currently) have values:
  //   0 (normal), 1 (added), 2 (modified) or 4 (deleted).
  enum EntryStatus {ES_INVALID      = -1,
                    ES_CLEAN        =  0,
                    ES_ADDED        =  1,  // Added    but not yet saved to disk copy
                    ES_MODIFIED     =  2,  // Modified but not yet saved to disk copy
                    ES_DELETED      =  4,  // Deleted  but not yet removed from disk copy
                    ES_LAST};

  // Flags if error found during validate of the entry
  enum  {VF_OK              =  0,
         VF_BAD_UUID        =  1,
         VF_EMPTY_TITLE     =  2,
         VF_EMPTY_PASSWORD  =  4,
         VF_NOT_UNIQUE_GTU  =  8,
         VF_BAD_PSWDHISTORY = 16};

  // a bitset for indicating a subset of an item's fields: 
  typedef std::bitset<LAST> FieldBits;

  static void SetSessionKey(); // call exactly once per session

  static bool IsTextField(unsigned char t);

  //Construction
  CItemData();

  CItemData(const CItemData& stuffhere);

  ~CItemData();

  // Convenience: Get the name associated with FieldType
  static stringT FieldName(FieldType ft);
  // Convenience: Get the untranslated (English) name of a FieldType
  static stringT EngFieldName(FieldType ft);

  //Data retrieval
  StringX GetName() const; // V17 - deprecated - replaced by GetTitle & GetUser
  StringX GetTitle() const; // V20
  StringX GetUser() const; // V20
  StringX GetPassword() const;
  size_t GetPasswordLength() const {return GetField(m_Password).length();}
  StringX GetNotes(TCHAR delimiter = 0) const;
  void GetUUID(uuid_array_t &) const; // V20
  const pws_os::CUUID GetUUID() const; // V20 - see comment in .cpp re return type
  StringX GetGroup() const; // V20
  StringX GetURL() const; // V30
  StringX GetAutoType() const; // V30
  StringX GetATime() const {return GetTime(ATIME, PWSUtil::TMC_ASC_UNKNOWN);}  // V30
  StringX GetCTime() const {return GetTime(CTIME, PWSUtil::TMC_ASC_UNKNOWN);}  // V30
  StringX GetXTime() const {return GetTime(XTIME, PWSUtil::TMC_ASC_UNKNOWN);}  // V30
  StringX GetPMTime() const {return GetTime(PMTIME, PWSUtil::TMC_ASC_UNKNOWN);}  // V30
  StringX GetRMTime() const {return GetTime(RMTIME, PWSUtil::TMC_ASC_UNKNOWN);}  // V30
  StringX GetATimeL() const {return GetTime(ATIME, PWSUtil::TMC_LOCALE);}  // V30
  StringX GetCTimeL() const {return GetTime(CTIME, PWSUtil::TMC_LOCALE);}  // V30
  StringX GetXTimeL() const {return GetTime(XTIME, PWSUtil::TMC_LOCALE_DATE_ONLY);}  // V30
  StringX GetPMTimeL() const {return GetTime(PMTIME, PWSUtil::TMC_LOCALE);}  // V30
  StringX GetRMTimeL() const {return GetTime(RMTIME, PWSUtil::TMC_LOCALE);}  // V30
  StringX GetATimeN() const {return GetTime(ATIME, PWSUtil::TMC_ASC_NULL);}  // V30
  StringX GetCTimeN() const {return GetTime(CTIME, PWSUtil::TMC_ASC_NULL);}  // V30
  StringX GetXTimeN() const {return GetTime(XTIME, PWSUtil::TMC_ASC_NULL);}  // V30
  StringX GetPMTimeN() const {return GetTime(PMTIME, PWSUtil::TMC_ASC_NULL);}  // V30
  StringX GetRMTimeN() const {return GetTime(RMTIME, PWSUtil::TMC_ASC_NULL);}  // V30
  StringX GetATimeExp() const {return GetTime(ATIME, PWSUtil::TMC_EXPORT_IMPORT);}  // V30
  StringX GetCTimeExp() const {return GetTime(CTIME, PWSUtil::TMC_EXPORT_IMPORT);}  // V30
  StringX GetXTimeExp() const {return GetTime(XTIME, PWSUtil::TMC_EXPORT_IMPORT);}  // V30
  StringX GetPMTimeExp() const {return GetTime(PMTIME, PWSUtil::TMC_EXPORT_IMPORT);}  // V30
  StringX GetRMTimeExp() const {return GetTime(RMTIME, PWSUtil::TMC_EXPORT_IMPORT);}  // V30
  StringX GetATimeXML() const {return GetTime(ATIME, PWSUtil::TMC_XML);}  // V30
  StringX GetCTimeXML() const {return GetTime(CTIME, PWSUtil::TMC_XML);}  // V30
  StringX GetXTimeXML() const {return GetTime(XTIME, PWSUtil::TMC_XML);}  // V30
  StringX GetPMTimeXML() const {return GetTime(PMTIME, PWSUtil::TMC_XML);}  // V30
  StringX GetRMTimeXML() const {return GetTime(RMTIME, PWSUtil::TMC_XML);}  // V30
  //  These populate the time structure instead of giving a character string
  void GetATime(time_t &t) const {GetTime(ATIME, t);}  // V30
  void GetCTime(time_t &t) const {GetTime(CTIME, t);}  // V30
  void GetXTime(time_t &t) const {GetTime(XTIME, t);}  // V30
  void GetPMTime(time_t &t) const {GetTime(PMTIME, t);}  // V30
  void GetRMTime(time_t &t) const {GetTime(RMTIME, t);}  // V30
  void GetXTimeInt(int32 &xint) const; // V30
  StringX GetXTimeInt() const; // V30
  StringX GetPWHistory() const;  // V30
  void GetPWPolicy(PWPolicy &pwp) const;
  StringX GetPWPolicy() const;
  StringX GetRunCommand() const;
  void GetDCA(short &iDCA, const bool bShift = false) const;
  StringX GetDCA(const bool bShift = false) const;
  void GetShiftDCA(short &iDCA) const {GetDCA(iDCA, true);}
  StringX GetShiftDCA() const {return GetDCA(true);}
  StringX GetEmail() const;
  StringX GetProtected() const;
  void GetProtected(unsigned char &ucprotected) const;
  bool IsProtected() const;
  StringX GetSymbols() const;
  StringX GetPolicyName() const;

  StringX GetFieldValue(const FieldType &ft) const;

  // GetPlaintext returns all fields separated by separator, if delimiter is != 0, then
  // it's used for multi-line notes and to replace '.' within the Title field.
  StringX GetPlaintext(const TCHAR &separator, const FieldBits &bsExport,
                       const TCHAR &delimiter, const CItemData *pcibase) const;
  std::string GetXML(unsigned id, const FieldBits &bsExport, TCHAR m_delimiter,
                     const CItemData *pcibase, bool bforce_normal_entry) const;
  void GetUnknownField(unsigned char &type, size_t &length,
                       unsigned char * &pdata,
                       const unsigned int &num) const;
  void GetUnknownField(unsigned char &type, size_t &length,
                       unsigned char * &pdata,
                       const UnknownFieldsConstIter &iter) const;
  void SetUnknownField(const unsigned char &type, const size_t &length,
                       const unsigned char * &ufield);
  size_t NumberUnknownFields() const
  {return m_URFL.size();}
  void ClearUnknownFields()
  {return m_URFL.clear();}
  UnknownFieldsConstIter GetURFIterBegin() const {return m_URFL.begin();}
  UnknownFieldsConstIter GetURFIterEnd() const {return m_URFL.end();}

  void CreateUUID(); // V20 - generate UUID for new item
  void SetName(const StringX &name,
               const StringX &defaultUsername); // V17 - deprecated - replaced by SetTitle & SetUser
  void SetTitle(const StringX &title, TCHAR delimiter = 0);
  void SetUser(const StringX &user); // V20
  void SetPassword(const StringX &password);
  void UpdatePassword(const StringX &password); // use when password changed!
  void SetNotes(const StringX &notes, TCHAR delimiter = 0);
  void SetUUID(const uuid_array_t &uuid); // V20
  void SetUUID(const pws_os::CUUID &uuid) {SetUUID(*uuid.GetARep());}
  void SetGroup(const StringX &group); // V20
  void SetURL(const StringX &url); // V30
  void SetAutoType(const StringX &autotype); // V30
  void SetATime() {SetTime(ATIME);}  // V30
  void SetATime(time_t t) {SetTime(ATIME, t);}  // V30
  bool SetATime(const stringT &time_str) {return SetTime(ATIME, time_str);}  // V30
  void SetCTime() {SetTime(CTIME);}  // V30
  void SetCTime(time_t t) {SetTime(CTIME, t);}  // V30
  bool SetCTime(const stringT &time_str) {return SetTime(CTIME, time_str);}  // V30
  void SetXTime() {SetTime(XTIME);}  // V30
  void SetXTime(time_t t) {SetTime(XTIME, t);}  // V30
  bool SetXTime(const stringT &time_str) {return SetTime(XTIME, time_str);}  // V30
  void SetPMTime() {SetTime(PMTIME);}  // V30
  void SetPMTime(time_t t) {SetTime(PMTIME, t);}  // V30
  bool SetPMTime(const stringT &time_str) {return SetTime(PMTIME, time_str);}  // V30
  void SetRMTime() {SetTime(RMTIME);}  // V30
  void SetRMTime(time_t t) {SetTime(RMTIME, t);}  // V30
  bool SetRMTime(const stringT &time_str) {return SetTime(RMTIME, time_str);}  // V30
  void SetXTimeInt(int &xint); // V30
  bool SetXTimeInt(const stringT &xint_str); // V30
  void SetPWHistory(const StringX &PWHistory);  // V30
  void SetPWPolicy(const PWPolicy &pwp);
  bool SetPWPolicy(const stringT &cs_pwp);
  void SetRunCommand(const StringX &cs_RunCommand);
  void SetDCA(const short &iDCA, const bool bShift = false);
  bool SetDCA(const stringT &cs_DCA, const bool bShift = false);
  void SetShiftDCA(const short &iDCA) {SetDCA(iDCA, true);}
  bool SetShiftDCA(const stringT &cs_DCA) {return SetDCA(cs_DCA, true);}
  void SetEmail(const StringX &sx_email);
  void SetProtected(const bool &bOnOff);
  void SetSymbols(const StringX &sx_symbols);
  void SetPolicyName(const StringX &sx_PolicyName);

  void SetFieldValue(const FieldType &ft, const StringX &value);

  CItemData& operator=(const CItemData& second);
  // Following used by display methods - we just keep it handy
  DisplayInfoBase *GetDisplayInfo() const {return m_display_info;}
  void SetDisplayInfo(DisplayInfoBase *di) {delete m_display_info; m_display_info = di;}
  void Clear();

  // check record for mandatory fields, silently fix if missing
  bool ValidateEntry(int &flags);
  bool ValidatePWHistory(); // return true if OK, false if there's a problem

  bool IsExpired() const;
  bool WillExpire(const int numdays) const;

  // Predicate to determine if item matches given criteria
  bool Matches(const stringT &stValue, int iObject, 
               int iFunction) const;  // string values
  bool Matches(int num1, int num2, int iObject,
               int iFunction) const;  // integer values
  bool Matches(time_t time1, time_t time2, int iObject,
               int iFunction) const;  // time values
  bool Matches(short dca, int iFunction, const bool bShift = false) const;  // DCA values
  bool Matches(EntryType etype, int iFunction) const;  // Entrytype values
  bool Matches(EntryStatus estatus, int iFunction) const;  // Entrystatus values

  bool IsGroupEmpty() const {return m_Group.IsEmpty();}
  bool IsUserEmpty() const {return m_User.IsEmpty();}
  bool IsNotesEmpty() const {return m_Notes.IsEmpty();}
  bool IsURLEmpty() const {return m_URL.IsEmpty();}
  bool IsRunCommandEmpty() const {return m_RunCommand.IsEmpty();}
  bool IsEmailEmpty() const {return m_email.IsEmpty();}
  bool IsPolicyEmpty() const {return m_PolicyName.IsEmpty();}

  bool IsGroupSet() const                     { return !m_Group.IsEmpty();        }
  bool IsUserSet() const                      { return !m_User.IsEmpty();         }
  bool IsNotesSet() const                     { return !m_Notes.IsEmpty();        }
  bool IsURLSet() const                       { return !m_URL.IsEmpty();          }
  bool IsRunCommandSet() const                { return !m_RunCommand.IsEmpty();   }
  bool IsEmailSet() const                     { return !m_email.IsEmpty();        }
  bool IsUUIDSet() const                      { return !m_UUID.IsEmpty();         }
  bool IsTitleSet() const                     { return !m_Title.IsEmpty();        }
  bool IsPasswordSet() const                  { return !m_Password.IsEmpty();     }
  bool IsCreationTimeSet() const              { return !m_tttCTime.IsEmpty();     }
  bool IsModificationTimeSet() const          { return !m_tttPMTime.IsEmpty();    }
  bool IsLastAccessTimeSet() const            { return !m_tttATime.IsEmpty();     }
  bool IsExpiryDateSet() const                { return !m_tttXTime.IsEmpty();     }
  bool IsRecordModificationTimeSet() const    { return !m_tttRMTime.IsEmpty();    }
  bool IsAutoTypeSet() const                  { return !m_AutoType.IsEmpty();     }
  bool IsPasswordHistorySet() const           { return !m_PWHistory.IsEmpty();    }
  bool IsPasswordPolicySet() const            { return !m_PWPolicy.IsEmpty();     }
  bool IsPasswordExpiryIntervalSet() const    { return !m_XTimeInterval.IsEmpty();}
  bool IsDCASet() const                       { return !m_DCA.IsEmpty();          }
  bool IsProtectionSet() const                { return !m_protected.IsEmpty();    }
  bool IsSymbolsSet() const                   { return !m_symbols.IsEmpty();      }
  bool IsPolicyNameSet() const                { return !m_PolicyName.IsEmpty();   }
    
  void SerializePlainText(std::vector<char> &v,
                          const CItemData *pcibase = NULL) const;
  bool DeSerializePlainText(const std::vector<char> &v);
  bool SetField(int type, const unsigned char *data, size_t len);

  EntryType GetEntryType() const {return m_entrytype;}

  bool IsNormal() const {return (m_entrytype == ET_NORMAL);}
  bool IsAliasBase() const {return (m_entrytype == ET_ALIASBASE);}
  bool IsShortcutBase() const {return (m_entrytype == ET_SHORTCUTBASE);}
  bool IsAlias() const {return (m_entrytype == ET_ALIAS);}
  bool IsShortcut() const {return (m_entrytype == ET_SHORTCUT);}
  bool IsBase() const {return IsAliasBase() || IsShortcutBase();}
  bool IsDependent() const {return IsAlias() || IsShortcut();}

  void SetEntryType(EntryType et) {m_entrytype = et;}
  void SetNormal() {m_entrytype = ET_NORMAL;}
  void SetAliasBase() {m_entrytype = ET_ALIASBASE;}
  void SetShortcutBase() {m_entrytype = ET_SHORTCUTBASE;}
  void SetAlias() {m_entrytype = ET_ALIAS;}
  void SetShortcut() {m_entrytype = ET_SHORTCUT;}

  EntryStatus GetStatus() const
  {return m_entrystatus;}
  void ClearStatus()
  {m_entrystatus = ES_CLEAN;}
  void SetStatus(const EntryStatus es)
  {m_entrystatus = es;}

  bool IsURLEmail() const
  {return GetURL().find(_T("mailto:")) != StringX::npos;}

  size_t GetSize();
  void GetSize(size_t &isize) const;

private:
  CItemField m_Name;
  CItemField m_Title;
  CItemField m_User;
  CItemField m_Password;
  CItemField m_Notes;
  CItemField m_UUID;
  CItemField m_Group;
  CItemField m_URL;
  CItemField m_AutoType;
  CItemField m_tttATime;  // last 'A'ccess time
  CItemField m_tttCTime;  // 'C'reation time
  CItemField m_tttXTime;  // password e'X'iry time
  CItemField m_tttPMTime; // last 'P'assword 'M'odification time
  CItemField m_tttRMTime; // last 'R'ecord 'M'odification time
  CItemField m_PWHistory;
  CItemField m_XTimeInterval;
  CItemField m_RunCommand;
  CItemField m_DCA;
  CItemField m_ShiftDCA;
  CItemField m_email;
  CItemField m_protected;
  // Password Policy stuff: Either m_PWPolicy (+ optionally m_symbols) is not empty
  // or m_PolicyName is not empty. Both cannot be set. All can be empty.
  CItemField m_PWPolicy;  // string encoding of item-specific password policy
  CItemField m_symbols;   // string of item-specific password symbols
  CItemField m_PolicyName; // named non-default password policy for this item

  // Save unknown record fields on read to put back on write unchanged
  UnknownFields m_URFL;

  EntryType m_entrytype;
  EntryStatus m_entrystatus;

  // random key for storing stuff in memory, just to remove dependence
  // on passphrase
  static bool IsSessionKeySet;
  static unsigned char SessionKey[64];
  //The salt value
  unsigned char m_salt[SaltLength];
  // Following used by display methods - we just keep it handy
  DisplayInfoBase *m_display_info;

  // move from pre-2.0 name to post-2.0 title+user
  void SplitName(const StringX &name,
                 StringX &title, StringX &username);
  StringX GetTime(int whichtime, PWSUtil::TMC result_format) const; // V30
  void GetTime(int whichtime, time_t &t) const; // V30
  void SetTime(const int whichtime); // V30
  void SetTime(const int whichtime, time_t t); // V30
  bool SetTime(const int whichtime, const stringT &time_str); // V30

  // Create local Encryption/Decryption object
  BlowFish *MakeBlowFish(bool noData = false) const;
  // Laziness is a Virtue:
  StringX GetField(const CItemField &field) const;
  void GetField(const CItemField &field, unsigned char *value,
                size_t &length) const;
  void GetUnknownField(unsigned char &type, size_t &length,
                       unsigned char * &pdata, const CItemField &item) const;
  void SetField(CItemField &field, const StringX &value);
  void SetField(CItemField &field, const unsigned char *value,
                size_t length);
  void UpdatePasswordHistory(); // used by UpdatePassword()
};

inline bool CItemData::IsTextField(unsigned char t)
{
  return !(t == UUID ||
    t == CTIME     || t == PMTIME || t == ATIME || t == XTIME || t == RMTIME ||
    t == XTIME_INT ||
    t == RESERVED  || t == DCA    || t == SHIFTDCA || t == PROTECTED ||
    t >= LAST);
}
#endif /* __ITEMDATA_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
