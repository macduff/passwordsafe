/// \file ThisMfcApp.cpp
/// \brief App object of MFC version of Password Safe
//-----------------------------------------------------------------------------

#include "PasswordSafe.h"
#include "corelib/PwsPlatform.h"

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
#include "DboxMain.h"

#include "CryptKeyEntry.h"


//-----------------------------------------------------------------------------
/*
  The one and only ThisMfcApp object.  In the MFC world, creating
  this global object is what starts the application.
*/

ThisMfcApp app;

//-----------------------------------------------------------------------------

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
	m_pMRU( NULL )
{
   srand((unsigned)time(NULL));

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
	if (*(unsigned int*)buf != ii)
	{
		AfxMessageBox(_T("Password Safe has been compiled as ") ENDIANNESS
			_T(" but CPU is really ") ENDIANNESS2 _T("\n")
			_T("You may not be able to open files or saved files may be incompatible with other platforms."));
	}
#endif
}


ThisMfcApp::~ThisMfcApp()
{
#ifndef POCKET_PC
	if ( m_pMRU )
	{
		m_pMRU->WriteList();
		delete m_pMRU;
	}
#endif
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
  AfxMessageBox(_T("Usage: PasswordSafe [password database]\n")
		_T("or PasswordSafe [-e|-d] filename"));
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
  if (fp == NULL)
    {
      CString text;
      text = "A fatal error occured: ";

      if (errno==EACCES)
	text += "Given path is a directory or file is read-only";
      else if (errno==EEXIST)
	text += "The filename already exists.";
      else if (errno==EINVAL)
	text += "Invalid oflag or shflag argument.";
      else if (errno==EMFILE)
	text += "No more file handles available.";
      else if (errno==ENOENT)
	text += "File or path not found.";
      text += "\nProgram will terminate.";

      CString title = "Password Safe - " + fn;
      AfxGetMainWnd()->MessageBox(text, title, MB_ICONEXCLAMATION|MB_OK);
    }
#endif
}

static BOOL EncryptFile(const CString &fn, const CMyString &passwd)
{
  unsigned int len;
  unsigned char* buf;

#if defined(UNICODE)
  FILE *in = _wfopen(fn, _T("rb"));
#else
  FILE *in = fopen(fn, _T("rb"));
#endif
  if (in != NULL) {
    len = fileLength(in);
    buf = new unsigned char[len];

    fread(buf, 1, len, in);
    fclose(in);
  } else {
    ErrorMessages(fn, in);
    return FALSE;
  }

  CString out_fn = fn;
  out_fn += CIPHERTEXT_SUFFIX;

#if defined(UNICODE)
  FILE *out = _wfopen(out_fn, _T("wb"));
#else
  FILE *out = fopen(out_fn, _T("wb"));
#endif
  if (out != NULL) {
#ifdef KEEP_FILE_MODE_BWD_COMPAT
	fwrite( &len, 1, sizeof(len), out);
#else
    for (int i=0; i < 8; i++)
      app.m_randstuff[i] = newrand();

    // miserable bug - have to fix this way to avoid breaking existing files
    app.m_randstuff[8] = app.m_randstuff[9] = '\0';
    GenRandhash(passwd,
		app.m_randstuff,
		app.m_randhash);
   fwrite(app.m_randstuff, 1,  8, out);
   fwrite(app.m_randhash,  1, 20, out);
#endif // KEEP_FILE_MODE_BWD_COMPAT
		
    unsigned char thesalt[SaltLength];
    for (int x=0;x<SaltLength;x++)
      thesalt[x] = newrand();
    fwrite(thesalt, 1, SaltLength, out);
		
    unsigned char ipthing[8];
    for (x=0;x<8;x++)
      ipthing[x] = newrand();
    fwrite(ipthing, 1, 8, out);

    LPCSTR pwd = LPCSTR(passwd);
    _writecbc(out, buf, len,
	      (unsigned char *)pwd, passwd.GetLength(),
	      thesalt, SaltLength,
	      ipthing);
		
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

#if defined(UNICODE)
  FILE *in = _wfopen(fn, _T("rb"));
#else
  FILE *in = fopen(fn, _T("rb"));
#endif
  if (in != NULL) {
      unsigned char salt[SaltLength];
      unsigned char ipthing[8];

#ifdef KEEP_FILE_MODE_BWD_COMPAT
      fread(&len, 1, sizeof(len), in); // XXX portability issue
#else
      fread(app.m_randstuff, 1, 8, in);
      app.m_randstuff[8] = app.m_randstuff[9] = '\0'; // ugly bug workaround
      fread(app.m_randhash, 1, 20, in);

      unsigned char temphash[20]; // HashSize
      GenRandhash(passwd,
		  app.m_randstuff,
		  temphash);
      if (0 != memcmp((char*)app.m_randhash,
		      (char*)temphash,
		      20)) // HashSize
	{
	  fclose(in);
	  AfxMessageBox(_T("Incorrect password"));
	  return FALSE;
	}
#endif // KEEP_FILE_MODE_BWD_COMPAT
      buf = NULL; // allocated by _readcbc - see there for apologia

      fread(salt,    1, SaltLength, in);
      fread(ipthing, 1, 8,          in);
      LPCSTR pwd = LPCSTR(passwd);
      if (_readcbc(in, buf, len,
		   (unsigned char *)pwd, passwd.GetLength(),
		   salt, SaltLength, ipthing) == 0) {
	delete[] buf; // if not yet allocated, delete[] NULL, which is OK
	return FALSE;
      }
		
      fclose(in);
    } else {
      ErrorMessages(fn, in);
      return FALSE;
    }

  int suffix_len = strlen(CIPHERTEXT_SUFFIX);
  int filepath_len = fn.GetLength();

  CString out_fn = fn;
  out_fn = out_fn.Left(filepath_len - suffix_len);

#if defined(UNICODE)
  FILE *out = _wfopen(out_fn, _T("wb"));
#else
  FILE *out = fopen(out_fn, _T("wb"));
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

BOOL
ThisMfcApp::InitInstance()
{
   /*
    * It's always best to start at the beginning.  [Glinda, Witch of the North]
	*/
	
	/*
	this pulls 'Counterpane Systems' out of the string table, verifies
	that it exists (actually, only verifies that *some* IDS_COMPANY
	string exists -- it could be 'Microsoft' for all we know), and then
	instructs the app to use the registry instead of .ini files.  The
	path ends up being
	
	  HKEY_CURRENT_USER\Software\(companyname)\(appname)\(sectionname)\(valuename)
	  
		Assuming the open-source version of this is going to become less
		Counterpane-centric, I expect this may change, but if it does, an
		automagic migration ought to happen. -- {jpr}
	*/
#if defined(POCKET_PC)
	SHInitExtraControls();
#endif

	CMyString companyname;
	VERIFY(companyname.LoadString(IDS_COMPANY) != 0);
	SetRegistryKey(companyname);
#ifndef POCKET_PC	
   int	nMRUItems = GetProfileInt(_T(PWS_REG_OPTIONS), _T("maxmruitems"), 4);
   m_pMRU = new CRecentFileList( 0, _T("MRU"), _T("Safe%d"), nMRUItems );;
	m_pMRU->ReadList();
#endif	
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
			fn.TrimLeft();

			StripFileQuotes( fn );

			if (fn.IsEmpty() || !CheckFile(fn)) {
				Usage();
				return FALSE;
			}
			
			CMyString passkey;
			if (args[1] == 'e' || args[1] == 'E' || args[1] == 'd' || args[1] == 'D') {
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
			switch (args[1]) {
			case 'e': case 'E': // do encrpytion
				status = EncryptFile(fn, passkey);
				if (!status) {
	   AfxMessageBox(_T("Encryption failed"));
				}
				break;
			case 'd': case 'D': // do decryption
				status = DecryptFile(fn, passkey);
				if (!status) {
					// nothing to do - DecryptFile displays its own error messages
				}
				break;
			default:
				Usage();
				return FALSE;
			} // switch
			return FALSE;
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
	//	application, rather than start the application's message pump.
	
	return FALSE;
}

#if !defined(POCKET_PC)
// Removes quotation marks from file name parameters
// as in "file with spaces.dat"
void
ThisMfcApp::StripFileQuotes( CString& strFilename )
{
	const char* szFilename	= strFilename;
	int			nLen		= strFilename.GetLength();

	// if the filenames starts with a quote...
	if ( *szFilename == '\"' )
	{
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
ThisMfcApp::ProcessMessageFilter(int code,
                                 LPMSG lpMsg)
{
   if (code < 0)
      CWinApp::ProcessMessageFilter(code, lpMsg);
	
   if (m_bUseAccelerator &&
	   m_maindlg != NULL
       && m_ghAccelTable != NULL)
   {
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
   ::HtmlHelp(NULL,
              "pwsafe.chm::/html/pws_intro.htm",
              HH_DISPLAY_TOPIC, 0);
#endif
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
