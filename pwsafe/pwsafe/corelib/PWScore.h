// PWScore.h
//-----------------------------------------------------------------------------

#ifndef PWScore_h
#define PWScore_h
#include <afxtempl.h> // for CList
#include "ItemData.h"
#include "MyString.h"
#include "PWSfile.h"

class PWScore {
 public:

  enum {
    SUCCESS = 0,
    FAILURE = 1,
    CANT_OPEN_FILE = -10,
    USER_CANCEL,
    WRONG_PASSWORD,
    UNKNOWN_VERSION,
    NOT_SUCCESS,
    ALREADY_OPEN,
    INVALID_FORMAT
   };

  PWScore();
  ~PWScore();

  // Following used to read/write databases
  CMyString GetCurFile() const {return m_currfile;}
  void SetCurFile(const CMyString &file) {m_currfile = file;}
  bool GetUseDefUser() const {return m_usedefuser;}
  void SetUseDefUser(bool v) {m_usedefuser = v;}
  CMyString GetDefUsername() const {return m_defusername;}
  void SetDefUsername(const CMyString &du) {m_defusername = du;}

  void ClearData();
  void NewFile(const CMyString &passkey);
  int WriteCurFile() {return WriteFile(m_currfile);}
  int WriteFile(const CMyString &filename, PWSfile::VERSION version = PWSfile::VCURRENT);
  int WriteV17File(const CMyString &filename)
    {return WriteFile(filename, PWSfile::V17);}
  int WritePlaintextFile(const CMyString &filename);
  int ImportPlaintextFile(const CMyString &ImportedPrefix, const CMyString &filename,
			  TCHAR fieldSeparator, int &numImported, int &numSkipped);
  int ImportKeePassTextFile(const CMyString &filename);
  bool FileExists(const CMyString &filename) {return PWSfile::FileExists(filename);}
  int ReadCurFile(const CMyString &passkey)
    {return ReadFile(m_currfile, passkey);}
  int ReadFile(const CMyString &filename, const CMyString &passkey);
  PWSfile::VERSION GetReadFileVersion() const {return m_ReadFileVersion;}
  int RenameFile(const CMyString &oldname, const CMyString &newname);
  int RenameCurFile(const CMyString &newSuffix);
  int CheckPassword(const CMyString &filename, CMyString &passkey);
  void ChangePassword(const CMyString & newPassword);
  bool LockFile(const CMyString &filename, CMyString &locker);
  bool IsLockedFile(const CMyString &filename) const;
  void UnlockFile(const CMyString &filename);
  
  POSITION GetFirstEntryPosition() const
    {return m_pwlist.GetHeadPosition();}
  POSITION AddEntryToTail(const CItemData &item)
    {m_changed = true; return m_pwlist.AddTail(item);}
  CItemData GetEntryAt(POSITION pos) const
    {return m_pwlist.GetAt(pos);}
  CItemData &GetEntryAt(POSITION pos)
    {return m_pwlist.GetAt(pos);}
  CItemData GetNextEntry(POSITION &pos) const
    {return m_pwlist.GetNext(pos);}
  CItemData &GetNextEntry(POSITION &pos)
    {return m_pwlist.GetNext(pos);}
  CItemData &GetTailEntry()
    {return m_pwlist.GetTail();}
  int GetNumEntries() const {return m_pwlist.GetCount();}
  void RemoveEntryAt(POSITION pos)
    {m_changed = true; m_pwlist.RemoveAt(pos);}
 // Find in m_pwlist by title and user name, exact match
  POSITION Find(const CMyString &a_group,
		const CMyString &a_title, const CMyString &a_user);

  bool IsChanged() const {return m_changed;}
  void SetChanged(bool changed) {m_changed = changed;} // use sparingly...
  void SetPassKey(const CMyString &new_passkey);

 private:
  CMyString m_currfile; // current pw db filespec
  unsigned char *m_passkey; // encrypted by session key
  unsigned int m_passkey_len; // Length of cleartext passkey
  static unsigned char m_session_key[20];
  static unsigned char m_session_salt[20];
  static unsigned char m_session_initialized;

  CMyString GetPassKey() const; // returns cleartext - USE WITH CARE
  // Following used by SetPassKey
  void EncryptPassword(const unsigned char *plaintext, int len,
		       unsigned char *ciphertext) const;
  bool m_usedefuser;
  CMyString m_defusername;
  PWSfile::VERSION m_ReadFileVersion;

  // the password database
  CList<CItemData,CItemData> m_pwlist;

  bool m_changed;
  HANDLE m_lockFileHandle;
};

#endif // PWScore_h
