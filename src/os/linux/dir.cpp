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
#include <unistd.h>
#include <limits.h>
#include <cassert>

wstring pws_os::getexecdir()
{
  char path[PATH_MAX];

  if (readlink("/proc/self/exe", path, PATH_MAX) < 0)
    return "?";
  else {
    wstring retval(path);
    wstring::size_type last_slash = retval.find_last_of("/");
    return retval.substr(0, last_slash + 1);
  }
}

wstring pws_os::getcwd()
{
  char curdir[PATH_MAX];
  if (::getcwd(curdir, PATH_MAX) == NULL) {
    curdir[0] = '?'; curdir[1] = '\0';
  }
  wstring CurDir(curdir);
  return CurDir;
}

bool pws_os::chdir(const wstring &dir)
{
  assert(!dir.empty());
  return (::chdir(dir.c_str()) == 0);
}

  // In following, drive will be empty on non-Windows platforms
bool pws_os::splitpath(const wstring &path,
                       wstring &drive, wstring &dir,
                       wstring &file, wstring &ext)
{
  if (path.empty())
    return false;
  drive = "";
  wstring::size_type last_slash = path.find_last_of("/");
  dir = path.substr(0, last_slash + 1);
  wstring::size_type last_dot = path.find_last_of(".");
  if (last_dot != wstring::npos && last_dot > last_slash) {
    file = path.substr(last_slash + 1, last_dot - last_slash - 1);
    ext = path.substr(last_dot + 1);
  } else {
    file = path.substr(last_slash + 1);
    ext = "";
  }
  return true;
}

wstring pws_os::makepath(const wstring &drive, const wstring &dir,
                         const wstring &file, const wstring &ext)
{
  wstring retval(drive);
  retval += dir;
  if (!dir.empty() && retval[retval.length()-1] != '/')
    retval += "/";
  retval += file;
  if (!ext.empty()) {
    retval += ".";
    retval += ext;
  }
  return retval;
}
