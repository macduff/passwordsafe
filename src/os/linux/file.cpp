/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/**
 * \file Linux-specific implementation of file.h
 */
 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cassert>
#include <fstream>

#include <dirent.h>
#include <fnmatch.h>
#include <malloc.h> // for free

#include "../file.h"
#include "../env.h"
#include "corelib.h"
#include "StringXStream.h"

const wchar_t *pws_os::PathSeparator = L"/";

bool pws_os::FileExists(const std::wstring &wfilename)
{
  // minor sanity checking
  if (wfilename.empty())
    return false;

  struct stat statbuf;
  size_t N = wcstombs(NULL, wfilename.c_str(), 0) + 1;
  assert(N > 0);
  char *fn = new char[N];
  wcstombs(fn, wfilename.c_str(), N);
  int status = ::stat(fn, &statbuf);
  delete [] fn;
  return (status == 0);
}

bool pws_os::FileExists(const std::wstring &wfilename, bool &bReadOnly)
{
  // minor sanity checking
  if (wfilename.empty())
    return false;

  bool retval = false;
  bReadOnly = false;
  size_t N = wcstombs(NULL, wfilename.c_str(), 0) + 1;
  assert(N > 0);
  char *fn = new char[N];
  wcstombs(fn, wfilename.c_str(), N);

  if (::access(fn, R_OK) == 0) {
    bReadOnly = (::access(fn, W_OK) != 0);
    retval = true;
  }
  delete [] fn;
  return retval;
}

bool pws_os::RenameFile(const std::wstring &woldname, const std::wstring &wnewname)
{
  // minor sanity checking
  if (woldname.empty() || wnewname.empty())
    return false;

  size_t oldN = wcstombs(NULL, woldname.c_str(), 0) + 1;
  assert(oldN > 0);
  char *oldfn = new char[oldN];
  wcstombs(oldfn, woldname.c_str(), oldN);

  size_t newN = wcstombs(NULL, wnewname.c_str(), 0) + 1;
  assert(newN > 0);
  char *newfn = new char[newN];
  wcstombs(newfn, wnewname.c_str(), newN);

  int status = ::rename(oldfn, newfn);

  delete [] oldfn;
  delete [] newfn;
  return (status == 0);
}

bool pws_os::CopyAFile(const std::wstring &wfrom, const std::wstring &wto)
{
  // minor sanity checking
  if (wfrom.empty() || wto.empty())
    return false;

  // can we read the source?
  size_t fromsize = wcstombs(NULL, wfrom.c_str(), 0) + 1;
  assert(fromsize > 0);
  char *szfrom = new char[fromsize];
  wcstombs(szfrom, wfrom.c_str(), fromsize);
  if (::access(szfrom, R_OK) != 0) {
    delete [] szfrom;
    return false;
  }

  // creates dirs as needed
  size_t tosize = wcstombs(NULL, wto.c_str(), 0) + 1;
  assert(tosize > 0);
  char *szto = new char[tosize];
  wcstombs(szto, wto.c_str(), tosize);
  std::string cto(szto);
  std::string::size_type start = (cto[0] == '/') ? 1 : 0;
  std::string::size_type stop;
  do {
    stop = cto.find_first_of("/", start);
    if (stop != string::npos)
      ::mkdir(cto.substr(start, stop).c_str(), 0700); // fail if already there - who cares?
    start = stop + 1;
  } while (stop != string::npos);

  std::ifstream src(szfrom, ios_base::in | ios_base::binary);
  std::ofstream dst(cto.c_str(), ios_base::out | ios_base::binary);
  const size_t BUFSIZE = 2048;
  char buf[BUFSIZE];
  size_t readBytes;

  do {
    src.read(buf, BUFSIZE);
    readBytes = src.gcount();
    dst.write(buf, readBytes);
  } while(readBytes != 0);

  delete [] szfrom;
  delete [] szto;
  return true;
}

bool pws_os::DeleteAFile(const std::wstring &wfilename)
{
  size_t fnsize = wcstombs(NULL, wfilename.c_str(), 0) + 1;
  assert(fnsize > 0);
  char *szfn = new char[fnsize];
  wcstombs(szffn, wfilename.c_str(), fnsize);

  bool rc = ::unlink(szffn);
  delete [] szffn;
  return rc;
}

static std::string filterString;

static int filterFunc(const struct dirent *de)
{
  return fnmatch(filterString.c_str(), de->d_name, 0) == 0;
}

void pws_os::FindFiles(const std::wstring &wfilter, std::vector<std::wstring> &res)
{
  // filter is a full path with a filter file name.
  // start by splitting it up
  size_t fltsize = wcstombs(NULL, wfilter.c_str(), 0) + 1;
  assert(fltsize > 0);
  assert(fltsize > 0);
  char *szfilter = new char[fltsize];
  wcstombs(szfilter, wfilter.c_str(), fltsize);

  std::string cfilter(szfilter);
  std::string dir;
  std::string::size_type last_slash = filter.find_last_of("/");

  if (last_slash != wstring::npos) {
    dir = filter.substr(0, last_slash);
    filterString = filter.substr(last_slash + 1);
  } else {
    dir = ".";
    filterString = filter;
  }
  delete [] szfilter;
  res.clear();
  struct dirent **namelist;
  int nMatches = scandir(dir.c_str(), &namelist,
                         filterFunc, alphasort);
  if (nMatches <= 0)
    return;

  while (nMatches-- != 0) {
    res.push_back(wstring(namelist[nMatches]->d_name));
    free(namelist[nMatches]);
  }
  free(namelist);
}

static std::wstring GetLockFileName(const std::wstring &filename)
{
  assert(!filename.empty());
  // derive lock filename from filename
  std::wstring retval(filename, 0, filename.find_last_of(L'.'));
  retval += L".plk";
  return retval;
}

bool pws_os::LockFile(const std::wstring &wfilename, std::wstring &locker, 
                      HANDLE &lockFileHandle, int &LockCount)
{
  const std::wstring wlock_filename = GetLockFileName(wfilename);
  std::wstring s_locker;
  size_t lfs = wcstombs(NULL, wlock_filename.c_str(), wlock_filename.length()) + 1;
  assert(lfs > 0);
  char *lfn = new char[lfs];
  wcstombs(lfn, lock_filename.c_str(), wlock_filename.length());
  int fh = open(lfn, (O_CREAT | O_EXCL | O_WRONLY),
                     (S_IREAD | S_IWRITE));
  delete [] lfn;

  if (fh == -1) { // failed to open exclusively. Already locked, or ???
    switch (errno) {
      case EACCES:
        // Tried to open read-only file for writing, or file's
        // sharing mode does not allow specified operations, or given path is directory
        LoadAString(locker, IDSC_NOLOCKACCESS);
        break;
      case EEXIST: // filename already exists
      {
        // read locker data ("user@machine:nnnnnnnn") from file
        std::wistringstream is(lock_filename);
        std::wstring lockerStr;
        if (is >> lockerStr) {
          locker = lockerStr;
        }
        break;
      } // EEXIST block
      case EINVAL: // Invalid oflag or pmode argument
        LoadAString(locker, IDSC_INTERNALLOCKERROR);
        break;
      case EMFILE: // No more file handles available (too many open files)
        LoadAString(locker, IDSC_SYSTEMLOCKERROR);
        break;
      case ENOENT: //File or path not found
        LoadAString(locker, IDSC_LOCKFILEPATHNF);
        break;
      default:
        LoadAString(locker, IDSC_LOCKUNKNOWNERROR);
        break;
    } // switch (errno)
    return false;
  } else { // valid filehandle, write our info
    int numWrit;
    const std::wstring user = pws_os::getusername();
    const std::wstring host = pws_os::gethostname();
    const std::wstring pid = pws_os::getprocessid();

    numWrit = write(fh, user.c_str(), user.length() * sizeof(wchar_t));
    numWrit += write(fh, L"@", sizeof(wchar_t));
    numWrit += write(fh, host.c_str(), host.length() * sizeof(wchar_t));
    numWrit += write(fh, L":", sizeof(wchar_t));
    numWrit += write(fh, pid.c_str(), pid.length() * sizeof(wchar_t));
    ASSERT(numWrit > 0);
    close(fh);
    return true;
  }
}

void pws_os::UnlockFile(const std::wstring &wfilename,
                        HANDLE &lockFileHandle, int &LockCount)
{
  std::wstring wlock_filename = GetLockFileName(wfilename);
  size_t lfs = wcstombs(NULL, wlock_filename.c_str(), wlock_filename.length()) + 1;
  assert(lfs > 0);
  char *lfn = new char[lfs];
  wcstombs(lfn, wlock_filename.c_str(), lock_filename.length());
  unlink(lfn);
  delete [] lfn;
}

bool pws_os::IsLockedFile(const std::wstring &filename)
{
  const std::wstring lock_filename = GetLockFileName(filename);
  return pws_os::FileExists(lock_filename);
}

FILE *pws_os::FOpen(const std::wstring &wfilename, const wchar_t *wmode)
{
  size_t fnsize = wcstombs(NULL, wfilename.c_str(), 0) + 1;
  assert(fnsize > 0);
  char *szfilename = new char[fnsize];
  wcstombs(szfilename, wfilename.c_str(), fnsize);

  size_t modesize = wcstombs(NULL, wmode, 0) + 1;
  assert(modesize > 0);
  char *szmode = new char[modesize];
  wcstombs(szmode, wmode, modesize);

  FILE *file = ::fopen(szfilename, szmode);
  delete [] szfilename;
  delete [] szmode;
  return file;
}

long pws_os::fileLength(FILE *fp)
{
  int fd = fileno(fp);
  if (fd == -1)
    return -1L;

  struct stat st;
  if (fstat(fd, &st) == -1)
    return -1L;

  return st.st_size;
}
