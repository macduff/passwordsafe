/// \file ThisMfcApp.cpp
/// \brief App object of MFC version of Password Safe
//-----------------------------------------------------------------------------

#include "PasswordSafe.h"

#include "corelib/PWSrand.h"
#include "corelib/sha256.h"
#include "corelib/sha1.h"

#if defined(POCKET_PC)
  #include "pocketpc/PocketPC.h"
  #include "pocketpc/resource.h"
#else
  #include <errno.h>
  #include <io.h>
  #include "resource.h"
#endif

#include "ThisMfcApp.h"
#include "corelib/Util.h"
#include "corelib/BlowFish.h"
#include "DboxMain.h"

#include "CryptKeyEntry.h"
#include "PWSRecentFileList.h"
#include "corelib/PWSprefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(ThisMfcApp, CWinApp)
//   ON_COMMAND(ID_HELP, CWinApp::OnHelp)
   ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

ThisMfcApp::ThisMfcApp() :
#if defined(POCKET_PC)
	m_bUseAccelerator( false ),
#else
	m_bUseAccelerator( true ),
#endif
	m_pMRU( NULL ), m_TrayLockedState(LOCKED), m_TrayIcon(NULL), m_csDefault_Browser(""),
	m_HotKeyPressed(false), m_companyname(_T("Counterpane Systems"))
{
  // {kjp} Temporary until I'm sure that PwsPlatform.h configures the endianness properly
#if defined(POCKET_PC)
  // Double check that *_ENDIAN has been correctly set!
#if defined(PWS_LITTLE_ENDIAN)
  unsigned char	buf[4]	= { 1, 0, 0, 0 };
  unsigned int	ii		= 1;
#define ENDIANNESS	_T("little endian")
#define ENDIANNESS2	_T("big endian")
#elif defined(PWS_BIG_ENDIAN)
  unsigned char	buf[4]	= { 0, 0, 0, 1 };
  unsigned int	ii		= 1;
#define ENDIANNESS	_T("big endian")
#define ENDIANNESS2	_T("little endian")
#endif
  if (*(unsigned int*)buf != ii) {
    AfxMessageBox(_T("Password Safe has been compiled as ") ENDIANNESS
                  _T(" but CPU is really ") ENDIANNESS2 _T("\n")
                  _T("You may not be able to open files or saved files may be incompatible with other platforms."));
  }
#endif

  CoInitialize(NULL);
}

ThisMfcApp::~ThisMfcApp()
{
  delete m_TrayIcon;
  if (m_pMRU != NULL) {
  	WriteMRU(PWSprefs::GetInstance()->GetConfigOptions());
    delete m_pMRU;
  }

  delete m_mainmenu;

  // Note: deleting PWSprefs instance first reformats the XML config file
  PWSprefs::DeleteInstance();
  PWSrand::DeleteInstance();
  CoUninitialize();

  /*
    apparently, with vc7, there's a CWinApp::HtmlHelp - I'd like
    to see the docs, someday.  In the meantime, force with :: syntax
  */

#if !defined(POCKET_PC)
  ::HtmlHelp(NULL, NULL, HH_CLOSE_ALL, 0);
#endif
}

#if !defined(POCKET_PC)
static void Usage()
{
  AfxMessageBox(_T("Usage: PasswordSafe [-r|-v] [password database]\n")
                _T("or PasswordSafe [-e|-d] filename\n")
				_T("or PasswordSafe [-s]\n\n")
				_T(" where:\n")
                _T("\t-r = open read-only\n")
				_T("\t-v = validate & repair\n")
				_T("\t-e/d = encrypt/decrypt file\n")
				_T("\t-s = open silently i.e. no open database")
                );
}

// tests if file exists, returns empty string if so, displays error message if not
static BOOL CheckFile(const CString &fn)
{
  DWORD status = ::GetFileAttributes(fn);
  CString ErrMess;

  if (status == -1) {
    ErrMess = _T("Could not access file: ");
    ErrMess += fn;
  } else if (status & FILE_ATTRIBUTE_DIRECTORY) {
    ErrMess = fn;
    ErrMess += _T(" is a directory");
  }

  if (ErrMess.IsEmpty()) {
    return TRUE;
  } else {
    AfxMessageBox(ErrMess);
    return FALSE;
  }
}

//Complain if the file has not opened correctly

static void
ErrorMessages(const CString &fn, FILE *fp)
{
#if !defined(POCKET_PC)
  if (fp == NULL) {
    CString text;
    text = _T("A fatal error occured: ");

    if (errno==EACCES)
      text += _T("Given path is a directory or file is read-only");
    else if (errno==EEXIST)
      text += _T("The filename already exists.");
    else if (errno==EINVAL)
      text += _T("Invalid oflag or shflag argument.");
    else if (errno==EMFILE)
      text += _T("No more file handles available.");
    else if (errno==ENOENT)
      text += _T("File or path not found.");
    text += _T("\nProgram will terminate.");

    CString title = _T("Password Safe - ") + fn;
    AfxGetMainWnd()->MessageBox(text, title, MB_ICONEXCLAMATION|MB_OK);
  }
#endif
}

static BOOL EncryptFile(const CString &fn, const CMyString &passwd)
{
  unsigned int len;
  unsigned char* buf;

  FILE *in;
#if _MSC_VER >= 1400
  _tfopen_s(&in, fn, _T("rb"));
#else
  in = _tfopen(fn, _T("rb"));
#endif
  if (in != NULL) {
    len = PWSUtil::fileLength(in);
    buf = new unsigned char[len];

    fread(buf, 1, len, in);
    fclose(in);
  } else {
    ErrorMessages(fn, in);
    return FALSE;
  }

  CString out_fn = fn;
  out_fn += CIPHERTEXT_SUFFIX;

  FILE *out;
#if _MSC_VER >= 1400
  _tfopen_s(&out, out_fn, _T("wb"));
#else
  out = _tfopen(out_fn, _T("wb")); 
#endif
  if (out != NULL) {
#ifdef KEEP_FILE_MODE_BWD_COMPAT
    fwrite( &len, 1, sizeof(len), out);
#else
    unsigned char randstuff[StuffSize];
    unsigned char randhash[SHA1::HASHLEN];   // HashSize
    PWSrand::GetInstance()->GetRandomData( randstuff, 8 );
    // miserable bug - have to fix this way to avoid breaking existing files
    randstuff[8] = randstuff[9] = '\0';
    GenRandhash(passwd,
                randstuff,
                randhash);
    fwrite(randstuff, 1,  8, out);
    fwrite(randhash,  1, sizeof(randhash), out);
#endif // KEEP_FILE_MODE_BWD_COMPAT
		
    unsigned char thesalt[SaltLength];
    PWSrand::GetInstance()->GetRandomData( thesalt, SaltLength );
    fwrite(thesalt, 1, SaltLength, out);
		
    unsigned char ipthing[8];
    PWSrand::GetInstance()->GetRandomData( ipthing, 8 );
    fwrite(ipthing, 1, 8, out);

    LPCSTR pwd = LPCSTR(passwd);
    Fish *fish = BlowFish::MakeBlowFish((unsigned char *)pwd, passwd.GetLength(),
                                        thesalt, SaltLength);
    _writecbc(out, buf, len, (unsigned char)0, fish, ipthing);
    delete fish;
    fclose(out);

  } else {
    ErrorMessages(out_fn, out);
    delete [] buf;
    return FALSE;
  }
  delete[] buf;
  return TRUE;
}

static BOOL DecryptFile(const CString &fn, const CMyString &passwd)
{
  unsigned int len;
  unsigned char* buf;

  FILE *in;
#if _MSC_VER >= 1400
  _tfopen_s(&in, fn, _T("rb"));
#else
  in = _tfopen(fn, _T("rb")); 
#endif
  if (in != NULL) {
    unsigned char salt[SaltLength];
    unsigned char ipthing[8];
    unsigned char randstuff[StuffSize];
    unsigned char randhash[SHA1::HASHLEN];

#ifdef KEEP_FILE_MODE_BWD_COMPAT
    fread(&len, 1, sizeof(len), in); // XXX portability issue
#else
    fread(randstuff, 1, 8, in);
    randstuff[8] = randstuff[9] = '\0'; // ugly bug workaround
    fread(randhash, 1, sizeof(randhash), in);

    unsigned char temphash[SHA1::HASHLEN];
    GenRandhash(passwd,
                randstuff,
                temphash);
    if (0 != memcmp((char*)randhash,
                    (char*)temphash, SHA1::HASHLEN)) {
      fclose(in);
      AfxMessageBox(_T("Incorrect password"));
      return FALSE;
    }
#endif // KEEP_FILE_MODE_BWD_COMPAT
    buf = NULL; // allocated by _readcbc - see there for apologia

    fread(salt,    1, SaltLength, in);
    fread(ipthing, 1, 8,          in);
    LPCSTR pwd = LPCSTR(passwd);
    unsigned char dummyType;

    Fish *fish = BlowFish::MakeBlowFish((unsigned char *)pwd, passwd.GetLength(),
                                        salt, SaltLength);
    if (_readcbc(in, buf, len,dummyType, fish, ipthing) == 0) {
      delete fish;
      delete[] buf; // if not yet allocated, delete[] NULL, which is OK
      return FALSE;
    }
    delete fish;
    fclose(in);
  } else {
    ErrorMessages(fn, in);
    return FALSE;
  }

  int suffix_len = strlen(CIPHERTEXT_SUFFIX);
  int filepath_len = fn.GetLength();

  CString out_fn = fn;
  out_fn = out_fn.Left(filepath_len - suffix_len);

#if _MSC_VER >= 1400
  FILE *out;
  _tfopen_s(&out, out_fn, _T("wb"));
#else
  FILE *out = _tfopen(out_fn, _T("wb")); 
#endif
  if (out != NULL) {
    fwrite(buf, 1, len, out);
    fclose(out);
  } else
    ErrorMessages(out_fn, out);

  delete[] buf; // allocated by _readcbc
  return TRUE;
}
#endif

int
ThisMfcApp::ExitInstance() 
{ 
  CWinApp::ExitInstance(); 
  return 0; 
} 

BOOL
ThisMfcApp::InitInstance()
{
  /*
   * It's always best to start at the beginning.  [Glinda, Witch of the North]
   */
	
#if defined(POCKET_PC)
  SHInitExtraControls();
#endif

  /*
    this instructs the app to use the registry instead of .ini files.  The
    path ends up being
	
    HKEY_CURRENT_USER\Software\(companyname)\(appname)\(sectionname)\(valuename)
	  
    Assuming the open-source version of this is going to become less
    Counterpane-centric, I expect this may change, but if it does, an
    automagic migration ought to happen. -- {jpr}

    Of course, this is legacy, and will go away once the registry is fully replaced
    by the in-database preference storage. -- ronys
  */
  SetRegistryKey(m_companyname);

  // MUST (indirectly) create PWSprefs first
  // Ensures all things like saving locations etc. are set up.
  PWSprefs *prefs = PWSprefs::GetInstance(&m_core);

  CMenu* new_popupmenu = NULL;
  const int iConfigOptions = prefs->GetConfigOptions();

	if (iConfigOptions != PWSprefs::CF_NONE) {
		int	nMRUItems = prefs->GetPref(PWSprefs::MaxMRUItems);

		m_mruonfilemenu = prefs->GetPref(PWSprefs::MRUOnFileMenu);

		m_clipboard_set = false;

		m_mainmenu = new CMenu;
		m_mainmenu->LoadMenu(IDR_MAINMENU);
		new_popupmenu = new CMenu;

		// Look for "File" menu.
		int pos = FindMenuItem(m_mainmenu, _T("&File"));
		if (pos == -1) // E.g., in non-English versions
			pos = 0; // best guess...

		CMenu* file_submenu = m_mainmenu->GetSubMenu(pos);
		if (file_submenu != NULL)	// Look for "Close Database"
			pos = FindMenuItem(file_submenu, ID_MENUITEM_CLOSE);
		else
			pos = -1;

		if (nMRUItems > 0) {
			if (pos > -1) {
				int irc;
				// Create New Popup Menu
				new_popupmenu->CreatePopupMenu();

				if (!m_mruonfilemenu) {	// MRU entries in popup menu
					// Insert Item onto new popup
					irc = new_popupmenu->InsertMenu( 0, MF_BYPOSITION, ID_FILE_MRU_ENTRY1, "Recent" );
					ASSERT(irc != 0);

					// Insert Popup onto main menu
					irc = file_submenu->InsertMenu( pos + 2, MF_BYPOSITION | MF_POPUP, (UINT) new_popupmenu->m_hMenu,
																				 "&Recent Safes" );
					ASSERT(irc != 0);
				} else {	// MRU entries inline
					irc = file_submenu->InsertMenu( pos + 2, MF_BYPOSITION, ID_FILE_MRU_ENTRY1, "Recent" );
					ASSERT(irc != 0);
				}

				m_pMRU = new CPWSRecentFileList( 0, _T("MRU"), _T("Safe%d"), nMRUItems );
				ReadMRU(iConfigOptions);
				// If new config file, copy the existing ones over now.
				if (iConfigOptions == PWSprefs::CF_FILE_RW_NEW)
					WriteMRU(iConfigOptions);
			}
		} else {
			if (pos > -1) {
				int irc;
				// Remove extra separator
				irc = file_submenu->RemoveMenu(pos + 1, MF_BYPOSITION);
				ASSERT( irc != 0);
				// Remove Clear MRU menu item.
				irc = file_submenu->RemoveMenu(ID_MENUITEM_CLEAR_MRU, MF_BYCOMMAND);
				ASSERT( irc != 0);
			}
		}
	}

  DboxMain dbox(NULL);
	
  /*
   * Command line processing:
   * Historically, it appears that if a filename was passed as a commadline argument,
   * the application would prompt the user for the password, and the encrypt or decrypt
   * the named file, based on the file's suffix. Ugh.
   *
   * What I'll do is as follows:
   * If a file is given in the command line, it is used as the database, overriding the
   * registry value. This will allow the user to have several databases, say, one for work
   * and one for personal use, and to set up a different shortcut for each.
   *
   * I think I'll keep the old functionality, but activate it with a "-e" or "-d" flag. (ronys)
   * {kjp} ... and I've removed all of it from the Pocket PC build.
   */
	
#if !defined(POCKET_PC)
  if (m_lpCmdLine[0] != '\0') {
    CString args = m_lpCmdLine;
		
    if (args[0] != _T('-')) {
      StripFileQuotes( args );
			
      if (CheckFile(args)) {
        dbox.SetCurFile(args);
      } else {
        return FALSE;
      }
    } else { // here if first char of arg is '-'
      // first, let's check that there's a second arg
      CString fn = args.Right(args.GetLength()-2);
      fn.Trim();

      if (!fn.IsEmpty())
        StripFileQuotes( fn );

      const int UC_arg1(toupper(args[1]));
	  // The following arguements require the database name and it exists!
      if ((UC_arg1 == 'R' || UC_arg1 == 'E' || UC_arg1 == 'D' || UC_arg1 == 'V')
		   && (fn.IsEmpty() || CheckFile(fn) == FALSE)) {
        Usage();
        return FALSE;
      }

	  if (fn.IsEmpty())
		  fn = (CString)PWSprefs::GetInstance()->GetPref(PWSprefs::CurrentFile);

      CMyString passkey;
      if (UC_arg1 == 'E' || UC_arg1 == 'D') {
        // get password from user if valid flag given. If note, default below will
        // pop usage message
        CCryptKeyEntry dlg(NULL);
        int nResponse = dlg.DoModal();
				
        if (nResponse==IDOK) {
          passkey = dlg.m_cryptkey1;
        } else {
          return FALSE;
        }
      }
      BOOL status;
      dbox.SetReadOnly(false);
      switch (UC_arg1) {
      case 'E': // do encrpytion
        status = EncryptFile(fn, passkey);
        if (!status) {
          AfxMessageBox(_T("Encryption failed"));
        }
        return TRUE;
      case 'D': // do decryption
        status = DecryptFile(fn, passkey);
        if (!status) {
          // nothing to do - DecryptFile displays its own error messages
        }
        return TRUE;
      case 'R':
        dbox.SetReadOnly(true);
        dbox.SetCurFile(fn);
        break;
      case 'S':
        dbox.SetStartSilent(true);
        dbox.SetCurFile(_T(""));
        break;
      case 'V':
        dbox.SetValidate(true);
        dbox.SetCurFile(fn);
        break;
      default:
        Usage();
        return FALSE;
      } // switch
    } // else
  } // m_lpCmdLine[0] != '\0';
#endif
	
  /*
   * normal startup
   */

  /*
    Here's where PWS currently does DboxMain, which in turn will do
    the initial PasskeyEntry (the one that looks like a splash screen).
    This makes things very hard to control.
    The app object (here) should instead do the initial PasskeyEntry,
    and, if successful, move on to DboxMain.  I think. {jpr}
  */
  m_maindlg = &dbox;
  m_pMainWnd = m_maindlg;

  // JHF : no tray icon and menu for PPC
#if !defined(POCKET_PC)
  //HICON stIcon = app.LoadIcon(IDI_TRAY);
  //ASSERT(stIcon != NULL);
  m_LockedIcon = app.LoadIcon(IDI_LOCKEDICON);
  m_UnLockedIcon = app.LoadIcon(IDI_UNLOCKEDICON);
  m_ClosedIcon = app.LoadIcon(IDI_TRAY);
  m_TrayIcon = new CSystemTray(NULL, WM_ICON_NOTIFY, _T("PasswordSafe"),
                               m_LockedIcon, dbox.m_RUEList,
                               WM_ICON_NOTIFY, IDR_POPTRAY);
  m_TrayIcon->SetTarget(&dbox);

#endif

  // Set up an Accelerator table
#if !defined(POCKET_PC)
  m_ghAccelTable = LoadAccelerators(AfxGetInstanceHandle(),
                                    MAKEINTRESOURCE(IDR_ACCS));
#endif
  //Run dialog
  (void) dbox.DoModal();
	
  /*
    note that we don't particularly care what the response was
  */
	
  // Since the dialog has been closed, return FALSE so that we exit the
  // application, rather than start the application's message pump.
  delete new_popupmenu;

  return FALSE;
}

void
ThisMfcApp::AddToMRU(const CString &pszFilename)
{
	if (m_pMRU == NULL)
		return;

	CString csMRUFilename(pszFilename);
	csMRUFilename.Trim();
	/* Implemented own CRecentFileList to get around MS problem - see code in
	   PWSRecentFileList.cpp */
	if (!csMRUFilename.IsEmpty())
		m_pMRU->Add(csMRUFilename);
}

void
ThisMfcApp::ClearMRU()
{
	if (m_pMRU == NULL || 
	    PWSprefs::GetInstance()->GetConfigOptions() == PWSprefs::CF_NONE)
		return;

	int numMRU = m_pMRU->GetSize();
	for (int i = numMRU; i > 0; i--)
		m_pMRU->Remove(i - 1);

	WriteMRU(PWSprefs::GetInstance()->GetConfigOptions());

	// Can't get the MRU list on the menu to tidy up automatically
	// Do it manually!
	CWnd* pMain = AfxGetMainWnd();

	CMenu* xmainmenu = pMain->GetMenu();

	// Look for "File" menu.
	int pos = FindMenuItem(xmainmenu, _T("&File"));
	if (pos == -1) // E.g., in non-English versions
		pos = 0; // best guess...

	CMenu* xfile_submenu = xmainmenu->GetSubMenu(pos);
	if (xfile_submenu != NULL)  // Look for MRU first entry
		pos = FindMenuItem(xfile_submenu, ID_FILE_MRU_ENTRY1);
	else
		return;

	if (pos > -1) {
		// Recent databases are on the main File menu
		for (int nID = numMRU; nID > 1; nID--)
			xfile_submenu->RemoveMenu(ID_FILE_MRU_ENTRY1 + nID - 1, MF_BYCOMMAND);

		return;
	}

	// Recent databases are on the popup menu off the main File menu
	CMenu* xpopupmenu = xfile_submenu->GetSubMenu(3);
	if (xpopupmenu != NULL)  // Look for MRU first entry
		pos = FindMenuItem(xpopupmenu, ID_FILE_MRU_ENTRY1);
	else
		return;

	if (pos > -1) {
		// Recent databases are on the main File menu
		for (int nID = numMRU; nID > 1; nID--)
			xfile_submenu->RemoveMenu(ID_FILE_MRU_ENTRY1 + nID - 1, MF_BYCOMMAND);

		return;
	}
}

void 
ThisMfcApp::WriteMRU(const int &iconfig)
{
	switch (iconfig) {
		case PWSprefs::CF_REGISTRY:
			m_pMRU->WriteList();
			break;
		case PWSprefs::CF_FILE_RW:
		case PWSprefs::CF_FILE_RW_NEW:
			{
				int num_MRU = m_pMRU->GetSize();
				CString csMRUFilename, csSubkey;
				PWSprefs::GetInstance()->SetKeepXMLLock(true);
				// Write out ones in use
				for (int i = 0; i < num_MRU; i++) {
					csMRUFilename = (*m_pMRU)[i];
					csMRUFilename.Trim();
					csSubkey.Format("Safe%02d", i + 1);
					PWSprefs::GetInstance()->WriteMRUToXML(csSubkey, csMRUFilename);
				}
				// Remove any not in use
				const int max_MRU = ID_FILE_MRU_ENTRYMAX - ID_FILE_MRU_ENTRY1;
				for (int i = num_MRU; i < max_MRU; i++) {
					csSubkey.Format("Safe%02d", i + 1);
					PWSprefs::GetInstance()->DeleteMRUFromXML(csSubkey);
				}
				PWSprefs::GetInstance()->SetKeepXMLLock(false);
			}
			break;
		case PWSprefs::CF_FILE_RO:
		case PWSprefs::CF_NONE:
		default:
			break;	
	}
}

void 
ThisMfcApp::ReadMRU(const int &iconfig)
{
	switch (iconfig) {
		case PWSprefs::CF_REGISTRY:
		case PWSprefs::CF_FILE_RW_NEW:
			m_pMRU->ReadList();
			break;
		case PWSprefs::CF_FILE_RW:
		case PWSprefs::CF_FILE_RO:
			{
				int	nMRUItems = PWSprefs::GetInstance()->GetPref(PWSprefs::MaxMRUItems);
				CString csSubkey;
				PWSprefs::GetInstance()->SetKeepXMLLock(true);
				for (int i = nMRUItems; i > 0; i--) {
					csSubkey.Format("Safe%02d", i);
					const CString csMRUFilename = (PWSprefs::GetInstance()->
						ReadMRUFromXML(csSubkey));
					AddToMRU(csMRUFilename);
				}
				PWSprefs::GetInstance()->SetKeepXMLLock(false);
			}
			break;
		case PWSprefs::CF_NONE:
		default:
			break;
	}
}

void
ThisMfcApp::SetSystemTrayState(STATE s)
{
  // need to protect against null m_TrayIcon due to
  // tricky initialization order
  if (m_TrayIcon != NULL && s != m_TrayLockedState) {
    m_TrayLockedState = s;
	HICON hIcon(m_LockedIcon);
	switch (s) {
		case LOCKED:
			hIcon = m_LockedIcon;
			break;
		case UNLOCKED:
			hIcon = m_UnLockedIcon;
			break;
		case CLOSED:
			hIcon = m_ClosedIcon;
			break;
		default:
			break;
	}
	m_TrayIcon->SetIcon(hIcon);
  }
}

void
ThisMfcApp::SetClipboardData(const CMyString &data)
{
	m_clipboard_set = PWSUtil::ToClipboard(data, m_clipboard_digest, m_pMainWnd->m_hWnd);
}

void
ThisMfcApp::ClearClipboardData()
{
  // Clear the clipboard IFF its value is the same as last set by this app.
  if (!m_clipboard_set)
    return;
		
	m_clipboard_set = PWSUtil::ClearClipboard(m_clipboard_digest, m_pMainWnd->m_hWnd);
}

#if !defined(POCKET_PC)
// Removes quotation marks from file name parameters
// as in "file with spaces.pws"
void
ThisMfcApp::StripFileQuotes( CString& strFilename )
{
  const char* szFilename	= strFilename;
  int			nLen		= strFilename.GetLength();

  // if the filenames starts with a quote...
  if ( *szFilename == '\"' ) {
    // remove leading quote
    ++szFilename;
    --nLen;

    // trailing quote is optional, remove if present
    if ( szFilename[nLen - 1] == '\"' )
      --nLen;

    strFilename = CString( szFilename, nLen );
  }
}
#endif

#if !defined(POCKET_PC)
//Copied from Knowledge Base article Q100770
//But not for WinCE {kjp}
BOOL
ThisMfcApp::ProcessMessageFilter(int code, LPMSG lpMsg)
{
  if (code < 0)
    CWinApp::ProcessMessageFilter(code, lpMsg);
	
  if (m_bUseAccelerator &&
      m_maindlg != NULL
      && m_ghAccelTable != NULL) {
    if (::TranslateAccelerator(m_maindlg->m_hWnd, m_ghAccelTable, lpMsg))
      return TRUE;
  }
  return CWinApp::ProcessMessageFilter(code, lpMsg);
}
#endif


void
ThisMfcApp::OnHelp()
{
#if defined(POCKET_PC)
  CreateProcess( _T("PegHelp.exe"), _T("pws_ce_help.html#mainhelp"), NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL );
#else
  /*
   * Following mess because CPropertySheet is too smart for its own
   * good. The "Help" button there is mapped to the App framework,
   * and that's that...
   */
  CString title;
  CWnd *wnd = CWnd::GetCapture();
  if (wnd == NULL)
    wnd = CWnd::GetFocus();
  if (wnd != NULL)
    wnd = wnd->GetParent();
  if (wnd != NULL) {
    wnd->GetWindowText(title);
  }
  if (title != _T("Options"))
    ::HtmlHelp(wnd->m_hWnd,
               "pwsafe.chm",
               HH_DISPLAY_TOPIC, 0);
  else
    ::HtmlHelp(NULL,
               "pwsafe.chm::/html/display_tab.html",
               HH_DISPLAY_TOPIC, 0);

#endif
}

// FindMenuItem() will find a menu item string from the specified
// popup menu and returns its position (0-based) in the specified 
// popup menu. It returns -1 if no such menu item string is found.
int ThisMfcApp::FindMenuItem(CMenu* Menu, LPCTSTR MenuString)
{
  ASSERT(Menu);
  ASSERT(::IsMenu(Menu->GetSafeHmenu()));

  int count = Menu->GetMenuItemCount();
  for (int i = 0; i < count; i++) {
    CString str;
    if (Menu->GetMenuString(i, str, MF_BYPOSITION) &&
        (strcmp(str, MenuString) == 0))
      return i;
  }

  return -1;
}

// FindMenuItem() will find a menu item ID from the specified
// popup menu and returns its position (0-based) in the specified 
// popup menu. It returns -1 if no such menu item string is found.
int ThisMfcApp::FindMenuItem(CMenu* Menu, int MenuID)
{
  ASSERT(Menu);
  ASSERT(::IsMenu(Menu->GetSafeHmenu()));

  int count = Menu->GetMenuItemCount();
  int id;
   
  for (int i = 0; i < count; i++) {
    id = Menu->GetMenuItemID(i);  // id = 0 for Separator; 1 for popup
    if ( id > 1 && id == MenuID)         
      return i;
  }

  return -1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
