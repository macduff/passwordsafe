// file PWScore.cpp
//-----------------------------------------------------------------------------
#pragma warning(push,3) // sad that VC6 cannot cleanly compile standard headers
#include <fstream> // for WritePlaintextFile
#include <iostream>
#include <string>
#include <vector>
#pragma warning(pop)
#pragma warning(disable : 4786)
using namespace std;

#include <LMCONS.H> // for UNLEN definition
#include <io.h> // low level file routines for locking
#include <fcntl.h> // constants _O_* for above
#include <sys/stat.h> // constants _S_* for above

#include "PWScore.h"
#include "BlowFish.h"
#include "PWSprefs.h"

unsigned char PWScore::m_session_key[20]; unsigned char
PWScore::m_session_salt[20]; unsigned char
PWScore::m_session_initialized = false;

PWScore::PWScore() : m_currfile(_T("")), m_changed(false),
		     m_usedefuser(false), m_defusername(_T("")),
		     m_ReadFileVersion(PWSfile::UNKNOWN_VERSION),
		     m_passkey(NULL), m_passkey_len(0),
		     m_lockFileHandle(INVALID_HANDLE_VALUE)
{

  if (!PWScore::m_session_initialized)
  {
	srand((unsigned)time(NULL));
	CItemData::SetSessionKey(); // per-session initialization
  GetRandomData(m_session_key, sizeof(m_session_key) );
  GetRandomData(m_session_salt, sizeof(m_session_salt) );

	PWScore::m_session_initialized = true;
  }

}

PWScore::~PWScore()
{
  if (m_passkey_len > 0) {
    trashMemory(m_passkey, ((m_passkey_len + 7)/8)*8);
    delete[] m_passkey;
  }
}

void
PWScore::ClearData(void)
{
  if (m_passkey_len > 0) {
    trashMemory(m_passkey, ((m_passkey_len + 7)/8)*8);
    delete[] m_passkey;
    m_passkey_len = 0;
  }
  //Composed of ciphertext, so doesn't need to be overwritten
  m_pwlist.RemoveAll();
}

void
PWScore::NewFile(const CMyString &passkey)
{
   ClearData();
   SetPassKey(passkey);
   m_changed = false;
}

int
PWScore::WriteFile(const CMyString &filename, PWSfile::VERSION version)
{
  PWSfile out(filename, GetPassKey());

  int status;

  // preferences are kept in header, which is written in OpenWriteFile,
  // so we need to update the prefernce string here
  out.SetPrefString(PWSprefs::GetInstance()->Store());

  status = out.OpenWriteFile(version);

  if (status != PWSfile::SUCCESS)
    return CANT_OPEN_FILE;

  CItemData temp;
  POSITION listPos = m_pwlist.GetHeadPosition();
  while (listPos != NULL)
    {
      temp = m_pwlist.GetAt(listPos);
      out.WriteRecord(temp);
      m_pwlist.GetNext(listPos);
    }
  out.CloseFile();

  m_changed = FALSE;
  m_ReadFileVersion = version; // needed when saving a V17 as V20 1st time [871893]

  return SUCCESS;
}

int
PWScore::WritePlaintextFile(const CMyString &filename)
{
  ofstream of(filename);

  if (!of)
    return CANT_OPEN_FILE;

  CItemData temp;
  POSITION listPos = m_pwlist.GetHeadPosition();

  while (listPos != NULL)
    {
      temp = m_pwlist.GetAt(listPos);
      of << (const char *)temp.GetPlaintext('\t') << endl;
      m_pwlist.GetNext(listPos);
    }
  of.close();

  return SUCCESS;
}

int
PWScore::WritePlaintextFile(const CMyString &filename, const char delimiter)
{
  ofstream of(filename);

  if (!of)
    return CANT_OPEN_FILE;

  CItemData temp;
  POSITION listPos = m_pwlist.GetHeadPosition();

  while (listPos != NULL)
  {
      temp = m_pwlist.GetAt(listPos);
      of << (const char *)temp.GetPlaintext('\t', delimiter) << endl;
      m_pwlist.GetNext(listPos);
  }
  of.close();

  return SUCCESS;
}

/*
int
PWScore::WriteXMLFile(const CMyString &filename)
{
  ofstream of(filename);

  if (!of)
    return CANT_OPEN_FILE;

  of << "<?xml version=\"1.0\">" << endl;
  of << "<passwordsafe>" << endl;
  POSITION listPos = m_pwlist.GetHeadPosition();
  while (listPos != NULL)
    {
      CItemData temp = m_pwlist.GetAt(listPos);

      of << "  <entry>" << endl;
      // TODO: need to handle entity escaping of values.
      of << "    <group>" << temp.GetGroup() << "</group>" << endl;
      of << "    <title>" << temp.GetTitle() << "</title>" << endl;
      of << "    <username>" << temp.GetUser() << "</username>" << endl;
      of << "    <password>" << temp.GetPassword() << "</password>" << endl;
      of << "    <notes>" << temp.GetNotes() << "</notes>" << endl;
      of << "  </entry>" << endl;

      m_pwlist.GetNext(listPos);
    }
  of << "</passwordsafe>" << endl;
  of.close();

  return SUCCESS;
}
*/

int
PWScore::ImportPlaintextFile(const CMyString &ImportedPrefix, const CMyString &filename,
			     TCHAR fieldSeparator, TCHAR delimiter, int &numImported, int &numSkipped)
{
  ifstream ifs(filename);
  numImported = numSkipped = 0;

  if (!ifs)
    return CANT_OPEN_FILE;

  for (;;)
    {
      // read a single line.
      string linebuf;
      if (!getline(ifs, linebuf, '\n')) break;

      // remove MS-DOS linebreaks, if needed.
      if (!linebuf.empty() && *(linebuf.end() - 1) == '\r') {
	linebuf.resize(linebuf.size() - 1);
      }

      // tokenize into separate elements
      vector<string> tokens;
      for (int startpos = 0; ; )
      {
	int nextchar = linebuf.find_first_of(fieldSeparator, startpos);
	if (nextchar >= 0 && tokens.size() < 3) {
	  tokens.push_back(linebuf.substr(startpos, nextchar - startpos));
	  startpos = nextchar + 1;
	} else {
	  // Here for last field, which is Notes. Notes may be double-quoted, and
	  // if they are, they may span more than one line.
	  string note(linebuf.substr(startpos));
	  unsigned int first_quote = note.find_first_of('\"');
	  unsigned int last_quote = note.find_last_of('\"');
	  if (first_quote == last_quote && first_quote != string::npos) {
	    //there was exactly one quote, meaning that we've a multi-line Note
	    bool noteClosed = false;
	    do {
	  					if (!getline(ifs, linebuf, '\n'))
	  					{
		  ifs.close(); // file ends before note closes
		  return (numImported > 0) ? SUCCESS : INVALID_FORMAT;
	      }
	      // remove MS-DOS linebreaks, if needed.
	      			if (!linebuf.empty() && *(linebuf.end() - 1) == '\r')
	      			{
		linebuf.resize(linebuf.size() - 1);
	      }
	      note += "\r\n";
	      note += linebuf;
	      unsigned int fq = linebuf.find_first_of('\"');
	      unsigned int lq = linebuf.find_last_of('\"');
	      noteClosed = (fq == lq && fq != string::npos);
	    } while (!noteClosed);
	  } // multiline note processed
	  tokens.push_back(note);
	  break;
	}
      }
      if (tokens.size() != 4) {
	numSkipped++; // malformed entry
	continue; // try to process next records
      }

      // Start initializing the new record.
      CItemData temp;
      temp.CreateUUID();
      temp.SetUser(CMyString(tokens[1].c_str()));
      temp.SetPassword(CMyString(tokens[2].c_str()));

      // The group and title field are concatenated.
      const string &grouptitle = tokens[0];
      int lastdot = grouptitle.find_last_of('.');
      if (lastdot > 0)
      {
      	CMyString newgroup(ImportedPrefix.IsEmpty() ?
			   "" : ImportedPrefix + ".");
	newgroup += grouptitle.substr(0, lastdot).c_str();
	temp.SetGroup(newgroup);
	temp.SetTitle(grouptitle.substr(lastdot + 1).c_str());
      } else {
	temp.SetGroup(ImportedPrefix);
	temp.SetTitle(grouptitle.c_str());
      }

      // The notes field begins and ends with a double-quote, with
      // no special escaping of any other internal characters.
      string quotedNotes = tokens[3];
      if (!quotedNotes.empty() &&
	  *quotedNotes.begin() == '\"' &&
	  *(quotedNotes.end() - 1) == '\"')
        {
	  quotedNotes = quotedNotes.substr(1, quotedNotes.size() - 2);
      	if (delimiter == '\0') {
	  temp.SetNotes(CMyString(quotedNotes.c_str()));
		} else {
			temp.SetNotes(CMyString(quotedNotes.c_str()), delimiter);
		}
        }

      AddEntryToTail(temp);
      numImported++;
    } // file processing for (;;) loop
  ifs.close();

  return SUCCESS;
}

int PWScore::CheckPassword(const CMyString &filename, CMyString& passkey)
{
  PWSfile f(filename, passkey);

  int status = f.CheckPassword();

  switch (status) {
  case PWSfile::SUCCESS:
    return SUCCESS;
  case PWSfile::CANT_OPEN_FILE:
    return CANT_OPEN_FILE;
  case PWSfile::WRONG_PASSWORD:
    return WRONG_PASSWORD;
  default:
    ASSERT(0);
    return status; // should never happen
  }
}

int
PWScore::ReadFile(const CMyString &a_filename,
                   const CMyString &a_passkey)
{
   //That passkey had better be the same one that came from CheckPassword(...)

   PWSfile in(a_filename, a_passkey);

  int status;

  m_ReadFileVersion = in.GetFileVersion();

  if (m_ReadFileVersion == PWSfile::UNKNOWN_VERSION)
    return UNKNOWN_VERSION;

  status = in.OpenReadFile(m_ReadFileVersion);

  if (status != PWSfile::SUCCESS)
    return CANT_OPEN_FILE;

  // prepare handling of pre-2.0 DEFUSERCHR conversion
  if (m_ReadFileVersion == PWSfile::V17)
    in.SetDefUsername(m_defusername);
  else // for 2.0 & later, get pref string (possibly empty)
    PWSprefs::GetInstance()->Load(in.GetPrefString());

   ClearData(); //Before overwriting old data, but after opening the file...

   SetPassKey(a_passkey);

   CItemData temp;

   status = in.ReadRecord(temp);

   while (status == PWSfile::SUCCESS)
   {
      m_pwlist.AddTail(temp);
      status = in.ReadRecord(temp);
   }

   in.CloseFile();

   return SUCCESS;
}

int PWScore::RenameFile(const CMyString &oldname, const CMyString &newname)
{
  return PWSfile::RenameFile(oldname, newname);
}


int PWScore::BackupCurFile()
{
  // renames CurFile to CurFile~
  CString newname(GetCurFile());
  newname += TCHAR('~');
  return PWSfile::RenameFile(GetCurFile(), newname);
}


void PWScore::ChangePassword(const CMyString &newPassword)
{

  SetPassKey(newPassword);
  m_changed = TRUE;
}


// Finds stuff based on title & user fields only
POSITION
PWScore::Find(const CMyString &a_group,const CMyString &a_title, const CMyString &a_user)
{
   POSITION listPos = m_pwlist.GetHeadPosition();
   CMyString group, title, user;

   while (listPos != NULL)
   {
     const CItemData &item = m_pwlist.GetAt(listPos);
      group = item.GetGroup();
      title = item.GetTitle();
      user = item.GetUser();
      if (group == a_group && title == a_title && user == a_user)
         break;
      else
         m_pwlist.GetNext(listPos);
   }

   return listPos;
}

void PWScore::EncryptPassword(const unsigned char *plaintext, int len,
			      unsigned char *ciphertext) const
{
  // ciphertext is ((len +7)/8)*8 bytes long
  BlowFish *Algorithm = BlowFish::MakeBlowFish(m_session_key,
                                               sizeof(m_session_key),
                                               m_session_salt,
                                               sizeof(m_session_salt));
  int BlockLength = ((len + 7)/8)*8;
  unsigned char curblock[8];

  for (int x=0;x<BlockLength;x+=8) {
    int i;
    if ((len == 0) ||
	((len%8 != 0) && (len - x < 8))) {
      //This is for an uneven last block
      memset(curblock, 0, 8);
      for (i = 0; i < len %8; i++)
	curblock[i] = plaintext[x + i];
      } else
	for (i = 0; i < 8; i++)
	  curblock[i] = plaintext[x + i];
      Algorithm->Encrypt(curblock, curblock);
      memcpy(ciphertext + x, curblock, 8);
   }
   trashMemory(curblock, 8);
  delete Algorithm;
}

void PWScore::SetPassKey(const CMyString &new_passkey)
{
  // if changing, clear old
  if (m_passkey_len > 0) {
    trashMemory(m_passkey, ((m_passkey_len + 7)/8)*8);
    delete[] m_passkey;
  }

  m_passkey_len = new_passkey.GetLength();

  int BlockLength = ((m_passkey_len + 7)/8)*8;
  m_passkey = new unsigned char[BlockLength];
  LPCTSTR plaintext = LPCTSTR(new_passkey);
  EncryptPassword((const unsigned char *)plaintext, m_passkey_len,
		  m_passkey);
}

CMyString PWScore::GetPassKey() const
{
  CMyString retval(_T(""));
  if (m_passkey_len > 0) {
    const unsigned int BS = BlowFish::BLOCKSIZE;
    unsigned int BlockLength = ((m_passkey_len + (BS-1))/BS)*BS;
    BlowFish *Algorithm = BlowFish::MakeBlowFish(m_session_key,
                                                 sizeof(m_session_key),
                                                 m_session_salt,
                                                 sizeof(m_session_salt));
    unsigned char curblock[BS];

    for (unsigned int x = 0; x < BlockLength; x += BS) {
      unsigned int i;
      for (i = 0; i < BS; i++)
        curblock[i] = m_passkey[x + i];
      Algorithm->Decrypt(curblock, curblock);
      for (i = 0; i < BS; i++)
        if (x + i < m_passkey_len)
          retval += curblock[i];
    }
    trashMemory(curblock, sizeof(curblock));
    delete Algorithm;
  }
  return retval;
}

/*
  Thought this might be useful to others...
  I made the mistake of using another password safe for a while...
  Glad I came back before it was too late, but I still needed to bring in those passwords.

  The format of the source file is from doing an export to TXT file in keepass.
  I tested it using my password DB from KeePass.

  There are two small things: if you have a line that is enclosed by square brackets in the
  notes, it will stop processing.  Also, it adds a single, extra newline character to any notes
  that is imports.  Both are pretty easy things to live with.

  --jah
*/

int
PWScore::ImportKeePassTextFile(const CMyString &filename)
{
  static const char *ImportedPrefix = { "ImportedKeePass" };
  ifstream ifs(filename);

  if (!ifs) {
    return CANT_OPEN_FILE;
  }

  string linebuf;

  string group;
  string title;
  string user;
  string passwd;
  string notes;

  // read a single line.
  if (!getline(ifs, linebuf, '\n') || linebuf.empty()) {
    return INVALID_FORMAT;
  }

  // the first line of the keepass text file contains a few garbage characters
  linebuf = linebuf.erase(0, linebuf.find("["));

  int pos = -1;
  for (;;) {
    if (!ifs)
      break;
    notes.erase();

    // this line should always be a title contained in []'s
    if (*(linebuf.begin()) != '[' || *(linebuf.end() - 1) != ']') {
      return INVALID_FORMAT;
    }

    // set the title: line pattern: [<group>]
    title = linebuf.substr(linebuf.find("[") + 1, linebuf.rfind("]") - 1).c_str();

    // set the group: line pattern: Group: <user>
    if (!getline(ifs, linebuf, '\n') || (pos = linebuf.find("Group: ")) == -1) {
      return INVALID_FORMAT;
    }
    group = ImportedPrefix;
    if (!linebuf.empty()) {
      group.append(".");
      group.append(linebuf.substr(pos + 7));
    }

    // set the user: line pattern: UserName: <user>
    if (!getline(ifs, linebuf, '\n') || (pos = linebuf.find("UserName: ")) == -1) {
      return INVALID_FORMAT;
    }
    user = linebuf.substr(pos + 10);

    // set the url: line pattern: URL: <url>
    if (!getline(ifs, linebuf, '\n') || (pos = linebuf.find("URL: ")) == -1) {
      return INVALID_FORMAT;
    }
    if (!linebuf.substr(pos + 5).empty()) {
      notes.append(linebuf.substr(pos + 5));
      notes.append("\r\n\r\n");
    }

    // set the password: line pattern: Password: <passwd>
    if (!getline(ifs, linebuf, '\n') || (pos = linebuf.find("Password: ")) == -1) {
      return INVALID_FORMAT;
    }
    passwd = linebuf.substr(pos + 10);

    // set the first line of notes: line pattern: Notes: <notes>
    if (!getline(ifs, linebuf, '\n') || (pos = linebuf.find("Notes: ")) == -1) {
      return INVALID_FORMAT;
    }
    notes.append(linebuf.substr(pos + 7));

    // read in any remaining new notes and set up the next record
    for (;;) {
      // see if we hit the end of the file
      if (!getline(ifs, linebuf, '\n')) {
	break;
      }

      // see if we hit a new record
      if (linebuf.find("[") == 0 && linebuf.rfind("]") == linebuf.length() - 1) {
	break;
      }

      notes.append("\r\n");
      notes.append(linebuf);
    }

    // Create & append the new record.
    CItemData temp;
    temp.CreateUUID();
    temp.SetTitle(title.empty() ? group.c_str() : title.c_str());
    temp.SetGroup(group.c_str());
    temp.SetUser(user.empty() ? " " : user.c_str());
    temp.SetPassword(passwd.empty() ? " " : passwd.c_str());
    temp.SetNotes(notes.empty() ? "" : notes.c_str());

    AddEntryToTail(temp);
  }
  ifs.close();

  // TODO: maybe return an error if the full end of the file was not reached?

  return SUCCESS;
}

/*
 * The file lock/unlock functions were first implemented (in 2.08)
 * with Posix semantics (using open(_O_CREATE|_O_EXCL) to detect
 * an existing lock.
 * This fails to check liveness of the locker process, specifically,
 * if a user just turns of her PC, the lock file will remain.
 * So, I'm keeping the Posix code under idef POSIX_FILE_LOCK,
 * and re-implementing using the Win32 API, whose semantics
 * supposedly protect against this scenario.
 * Thanks to Frank (xformer) for discussion on the subject.
 */

static void GetLockFileName(const CMyString &filename,
			    CMyString &lock_filename)
{
  ASSERT(!filename.IsEmpty());
  // derive lock filename from filename
  if (filename.GetLength() > 3)
    lock_filename = CMyString(filename.Left(filename.GetLength()-3));
  else
    lock_filename = filename;
  lock_filename += _T("plk");
}

bool PWScore::LockFile(const CMyString &filename, CMyString &locker)
{
  CMyString lock_filename;
  GetLockFileName(filename, lock_filename);
#ifdef POSIX_FILE_LOCK
  int fh = _open(lock_filename, (_O_CREAT | _O_EXCL | _O_WRONLY),
		 (_S_IREAD | _S_IWRITE));

  if (fh == -1) { // failed to open exclusively. Already locked, or ???
    switch (errno) {
    case EACCES:
      // Tried to open read-only file for writing, or file�s
      // sharing mode does not allow specified operations, or given path is directory
      locker = _T("Cannot create lock file - no permission in directory?");
      break;
    case EEXIST: // filename already exists
      {
	// read locker data ("user@machine") from file
	TCHAR lockerStr[UNLEN + MAX_COMPUTERNAME_LENGTH + sizeof(TCHAR)*2];
	int fh2 = _open(lock_filename, _O_RDONLY);
	if (fh2 == -1) {
	  locker = _T("Unable to determine locker?");
	} else {
	  int bytesRead = _read(fh2, lockerStr, sizeof(lockerStr)-1);
	  _close(fh2);
	  if (bytesRead > 0) {
	    lockerStr[bytesRead] = TCHAR('\0');
	    locker = lockerStr;
	  } else { // read failed for some reason
	    locker = _T("Unable to read locker?");
	  } // read info from lock file
	} // open lock file for read
      } // EEXIST block
      break;
    case EINVAL: // Invalid oflag or pmode argument
      locker = _T("Internal error: Invalid oflag or pmode argument");
      break;
    case EMFILE: // No more file handles available (too many open files)
      locker = _T("System error: No morefile handles available");
      break;
    case ENOENT: //File or path not found
      locker = _T("File or path not found");
      break;
    default:
      locker = _T("Internal error: Unexpected errno");
      break;
    } // switch (errno)
    return false;
  } else { // valid filehandle, write our info
    TCHAR user[UNLEN+1];
    TCHAR sysname[MAX_COMPUTERNAME_LENGTH+1];
    DWORD len;
    len = sizeof(user);
    if (::GetUserName(user, &len)== FALSE) {
      user[0] = TCHAR('?'); user[1] = TCHAR('\0');
    }
    len = sizeof(sysname);
    if (::GetComputerName(sysname, &len) == FALSE) {
      sysname[0] = TCHAR('?'); sysname[1] = TCHAR('\0');
    }
    int numWrit;
    numWrit = _write(fh, user, _tcslen(user)*sizeof(TCHAR));
    numWrit += _write(fh, _T("@"), _tcslen("@")*sizeof(TCHAR));
    numWrit += _write(fh, sysname, _tcslen(sysname)*sizeof(TCHAR));
    ASSERT(numWrit > 0);
    _close(fh);
    return true;
  }
#else
  // Use Win32 API for locking - supposedly better at
  // detecting dead locking processes
  if (m_lockFileHandle != INVALID_HANDLE_VALUE) {
    // here if we've open another (or same) dbase previously,
    // need to unlock it. A bit inelegant...
    // If app was minimized and ClearData() called, we've a small
    // potential for a TOCTTOU issue here. Worse case, lock
    // will fail.
    UnlockFile(GetCurFile());
  }
  m_lockFileHandle = ::CreateFile(LPCTSTR(lock_filename),
				  GENERIC_WRITE,
				  FILE_SHARE_READ,
				  NULL,
				  CREATE_ALWAYS, // rely on share to fail if exists!
				  FILE_ATTRIBUTE_NORMAL| FILE_FLAG_WRITE_THROUGH,
				  NULL);
  if (m_lockFileHandle == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    switch (error) {
    case ERROR_SHARING_VIOLATION: // already open by a live process
      {
 	// read locker data ("user@machine") from file
	TCHAR lockerStr[UNLEN + MAX_COMPUTERNAME_LENGTH + sizeof(TCHAR)*2];
	// flags here counter (my) intuition, but see
	// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/base/creating_and_opening_files.asp
	HANDLE h2 = ::CreateFile(LPCTSTR(lock_filename),
				 GENERIC_READ, FILE_SHARE_WRITE,
				 NULL, OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL, NULL);
	if (h2 == INVALID_HANDLE_VALUE) {
	  locker = _T("Unable to determine locker?");
	} else {
	  DWORD bytesRead;
	  (void)::ReadFile(h2, lockerStr, sizeof(lockerStr)-1,
					&bytesRead, NULL);
	  CloseHandle(h2);
	  if (bytesRead > 0) {
	    lockerStr[bytesRead] = TCHAR('\0');
	    locker = lockerStr;
	  } else { // read failed for some reason
	    locker = _T("Unable to read locker?");
	  } // read info from lock file
	} // open lock file for read
      } // ERROR_SHARING_VIOLATION block
      break;
    default:
      locker = _T("Cannot create lock file - no permission in directory?");
      break;
    } // switch (error)
    return false;
  } else { // valid filehandle, write our info
    TCHAR user[UNLEN+1];
    TCHAR sysname[MAX_COMPUTERNAME_LENGTH+1];
    DWORD len;
    len = sizeof(user);
    if (::GetUserName(user, &len)== FALSE) {
      user[0] = TCHAR('?'); user[1] = TCHAR('\0');
    }
    len = sizeof(sysname);
    if (::GetComputerName(sysname, &len) == FALSE) {
      sysname[0] = TCHAR('?'); sysname[1] = TCHAR('\0');
    }
    DWORD numWrit, sumWrit;
    BOOL write_status;
    write_status = ::WriteFile(m_lockFileHandle, user,
			       _tcslen(user)*sizeof(TCHAR),
			       &sumWrit, NULL);
    write_status = ::WriteFile(m_lockFileHandle,
			       _T("@"), _tcslen("@")*sizeof(TCHAR),
			       &numWrit, NULL);
    sumWrit += numWrit;
    write_status += ::WriteFile(m_lockFileHandle,
				sysname, _tcslen(sysname)*sizeof(TCHAR),
				&numWrit, NULL);
    sumWrit += numWrit;
    ASSERT(sumWrit > 0);
    return true;
  }
#endif // POSIX_FILE_LOCK
}

void PWScore::UnlockFile(const CMyString &filename)
{
#ifdef POSIX_FILE_LOCK
  CMyString lock_filename;
  GetLockFileName(filename, lock_filename);
  _unlink(lock_filename);
#else
  // Use Win32 API for locking - supposedly better at
  // detecting dead locking processes
  if (m_lockFileHandle != INVALID_HANDLE_VALUE) {
    CMyString lock_filename;
    GetLockFileName(filename, lock_filename);
    CloseHandle(m_lockFileHandle);
    m_lockFileHandle = INVALID_HANDLE_VALUE;
    DeleteFile(LPCTSTR(lock_filename));
  }
#endif // POSIX_FILE_LOCK
}

bool PWScore::IsLockedFile(const CMyString &filename) const
{
  CMyString lock_filename;
  GetLockFileName(filename, lock_filename);
#ifdef POSIX_FILE_LOCK
  return PWSfile::FileExists(lock_filename);
#else
  // under this scheme, we need to actually try to open the file to determine
  // if it's locked.
  HANDLE h = CreateFile(LPCTSTR(lock_filename),
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING, // don't create one!
			FILE_ATTRIBUTE_NORMAL| FILE_FLAG_WRITE_THROUGH,
			NULL);
  if (h == INVALID_HANDLE_VALUE) {
    DWORD error = GetLastError();
    if (error == ERROR_SHARING_VIOLATION)
      return true;
    else
      return false; // couldn't open it, probably doesn't exist.
  } else {
    CloseHandle(h); // here if exists but lockable.
    return false;
  }
#endif // POSIX_FILE_LOCK
}

// GetUniqueGroups - Creates an array of all group names, with no duplicates.
void PWScore::GetUniqueGroups(CStringArray &aryGroups)
{
  aryGroups.RemoveAll();
  POSITION listPos = GetFirstEntryPosition();
  while (listPos != NULL) {
    CItemData &ci = GetEntryAt(listPos);
    CString strThisGroup = ci.GetGroup();
    // Is this group already in the list?
    bool bAlreadyInList=false;
    for(int igrp=0; igrp<aryGroups.GetSize(); igrp++) {
      if(aryGroups[igrp] == strThisGroup) {
	bAlreadyInList = true;
	break;
      }
    }
    if(!bAlreadyInList) aryGroups.Add(strThisGroup);
    GetNextEntry(listPos);
  }
}
