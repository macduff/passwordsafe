/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/**
 * \file Linux-specific implementation of dir.h
 */
 
#include "../dir.h"
#include "../env.h"

#include <unistd.h>
#include <limits.h>
#include <cassert>
#include <stdlib.h>

std::wstring pws_os::getexecdir()
{
  char path[PATH_MAX];

  if (readlink("/proc/self/exe", path, PATH_MAX) < 0)
    return std::wstring(L"?");
  else {
    std::wstring retval = towc(path);
    std::wstring::size_type last_slash = retval.find_last_of(L"/");
    return retval.substr(0, last_slash + 1);
  }
}

std::wstring pws_os::getcwd()
{
  char curdir[PATH_MAX] = { 0 };
  if (::getcwd(curdir, PATH_MAX) == NULL) {
    curdir[0] = '?';
    curdir[1] = '\0';
  }
  return towc(curdir);
}

bool pws_os::chdir(const std::wstring &wdir)
{
  assert(!wdir.empty());

  size_t N = wcstombs(NULL, wdir.c_str(), 0) + 1;
  assert(N > 0);
  char *szdir = new char[N];
  wcstombs(szdir, wdir.c_str(), N);

  bool rc = (::chdir(szdir) == 0);
  delete [] szdir;
  return rc;
}

  // In following, drive will be empty on non-Windows platforms
bool pws_os::splitpath(const std::wstring &path,
                       std::wstring &drive, std::wstring &dir,
                       std::wstring &file, std::wstring &ext)
{
  if (path.empty())
    return false;

  drive = L"";
  std::wstring::size_type last_slash = path.find_last_of(L"/");
  dir = path.substr(0, last_slash + 1);
  std::wstring::size_type last_dot = path.find_last_of(L".");
  if (last_dot != std::wstring::npos && last_dot > last_slash) {
    file = path.substr(last_slash + 1, last_dot - last_slash - 1);
    ext = path.substr(last_dot + 1);
  } else {
    file = path.substr(last_slash + 1);
    ext = L"";
  }
  return true;
}

std::wstring pws_os::makepath(const std::wstring &drive, const std::wstring &dir,
                              const std::wstring &file, const std::wstring &ext)
{
  std::wstring retval(drive);
  retval += dir;
  if (!dir.empty() && retval[retval.length() - 1] != L'/')
    retval += L"/";
  retval += file;
  if (!ext.empty()) {
    retval += L".";
    retval += ext;
  }
  return retval;
}
