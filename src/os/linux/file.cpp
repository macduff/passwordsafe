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

using namespace std;

const wchar_t *pws_os::PathSeparator = L"/";

bool pws_os::FileExists(const wstring &filename)
{
  struct stat statbuf;
  int status;
  size_t N = wcstombs(NULL, filename.c_str(), 0) + 1;
  char *fn = new char[N];
  wcstombs(fn, filename.c_str(), N);
  status = ::stat(fn, &statbuf);
  delete[] fn;
  return (status == 0);
}

bool pws_os::FileExists(const wstring &filename, bool &bReadOnly)
{
  int status;
  bool retval = false;
  bReadOnly = false;
  size_t N = wcstombs(NULL, filename.c_str(), 0) + 1;
  char *fn = new char[N];
  wcstombs(fn, filename.c_str(), N);
  status = ::access(fn, R_OK);
  if (status == 0) {
    bReadOnly = (::access(fn, W_OK) != 0);
    retval = true;
  }
  delete[] fn;
  return retval;
}

bool pws_os::RenameFile(const wstring &oldname, const wstring &newname)
{
  int status;
  size_t oldN = wcstombs(NULL, oldname.c_str(), 0) + 1;
  char *oldfn = new char[oldN];
  wcstombs(oldfn, oldname.c_str(), oldN);
  size_t newN = wcstombs(NULL, newname.c_str(), 0) + 1;
  char *newfn = new char[newN];
  wcstombs(newfn, newname.c_str(), newN);
  status = ::rename(oldfn, newfn);
  delete[] oldfn;
  delete[] newfn;
  return (status == 0);
}

bool pws_os::CopyAFile(const wstring &from, const wstring &to)
{
  // can we read the source?
  if (::access(from.c_str(), R_OK) != 0)
    return false;
  // creates dirs as needed
  wstring::size_type start = (to[0] == '/') ? 1 : 0;
  wstring::size_type stop;
  do {
    stop = to.find_first_of("/", start);
    if (stop != wstring::npos)
      ::mkdir(to.substr(start, stop).c_str(), 0700); // fail if already there - who cares?
    start = stop + 1;
  } while (stop != wstring::npos);
  ifstream src(from.c_str(), ios_base::in|ios_base::binary);
  ofstream dst(to.c_str(), ios_base::out|ios_base::binary);
  const size_t BUFSIZE = 2048;
  char buf[BUFSIZE];
  size_t readBytes;

  do {
    src.read(buf, BUFSIZE);
    readBytes = src.gcount();
    dst.write(buf, readBytes);
  } while(readBytes != 0);
  return true;
}

bool pws_os::DeleteAFile(const wstring &filename)
{
  return ::unlink(filename.c_str());
}

static wstring filterString;

static int filterFunc(const struct dirent *de)
{
  return fnmatch(filterString.c_str(), de->d_name, 0) == 0;
}

void pws_os::FindFiles(const wstring &filter, vector<wstring> &res)
{
  // filter is a full path with a filter file name.
  // start by splitting it up
  wstring dir;
  wstring::size_type last_slash = filter.find_last_of("/");
  if (last_slash != wstring::npos) {
    dir = filter.substr(0, last_slash);
    filterString = filter.substr(last_slash + 1);
  } else {
    dir = ".";
    filterString = filter;
  }
  res.clear();
  struct dirent **namelist;
  int nMatches = scandir(dir.c_str(), &namelist,
                         filterFunc, alphasort);
  if (nMatches <= 0)
    return;
  while (nMatches-- != 0) {
    res.push_back(namelist[nMatches]->d_name);
    free(namelist[nMatches]);
  }
  free(namelist);
}

static wstring GetLockFileName(const wstring &filename)
{
  assert(!filename.empty());
  // derive lock filename from filename
  wstring retval(filename, 0, filename.find_last_of(L'.'));
  retval += L".plk";
  return retval;
}

bool pws_os::LockFile(const wstring &filename, wstring &locker, 
                      HANDLE &lockFileHandle, int &LockCount)
{
  const wstring lock_filename = GetLockFileName(filename);
  wstring s_locker;
  size_t lfs = wcstombs(NULL, lock_filename.c_str(), lock_filename.length()) + 1;
  char *lfn = new char[lfs];
  wcstombs(lfn, lock_filename.c_str(), lock_filename.length());
  int fh = open(lfn, (O_CREAT | O_EXCL | O_WRONLY),
                 (S_IREAD | S_IWRITE));
  delete[] lfn;

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
          wistringstream is(lock_filename);
          wstring lockerStr;
          if (is >> lockerStr) {
            locker = lockerStr;
          }
      } // EEXIST block
        break;
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
    const wstring user = pws_os::getusername();
    const wstring host = pws_os::gethostname();
    const wstring pid = pws_os::getprocessid();

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

void pws_os::UnlockFile(const wstring &filename,
                        HANDLE &lockFileHandle, int &LockCount)
{
  wstring lock_filename = GetLockFileName(filename);
  size_t lfs = wcstombs(NULL, lock_filename.c_str(), lock_filename.length()) + 1;
  char *lfn = new char[lfs];
  wcstombs(lfn, lock_filename.c_str(), lock_filename.length());
  unlink(lfn);
  delete[] lfn;
}

bool pws_os::IsLockedFile(const wstring &filename)
{
  const wstring lock_filename = GetLockFileName(filename);
  return pws_os::FileExists(lock_filename);
}

FILE *pws_os::FOpen(const wstring &filename, const wchar_t *mode)
{
  return ::fopen(filename.c_str(), mode);
}

long pws_os::fileLength(FILE *fp)
{
  int fd = fileno(fp);
  if (fd == -1)
    return -1;
  struct stat st;
  if (fstat(fd, &st) == -1)
    return -1;
  return st.st_size;
}
