/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/**
 * \file Windows-specific implementation of dir.h
 */

#include <afx.h>
#include <Windows.h>
#include "../dir.h"
#include <direct.h>

std::wstring pws_os::getexecdir()
{
  // returns the directory part of ::GetModuleFileName()
  wchar_t acPath[MAX_PATH + 1];

  if (GetModuleFileName(NULL, acPath, MAX_PATH + 1) != 0) {
    // guaranteed file name of at least one character after path '\'
    *(wcsrchr(acPath, L'\\') + 1) = L'\0';
  } else {
    acPath[0] = L'\\'; acPath[1] = L'\0';
  }
  return std::wstring(acPath);
}

std::wstring pws_os::getcwd()
{
  wchar_t *curdir = _wgetcwd(NULL, 512); // NULL means 512 doesn't matter
  std::wstring CurDir(curdir);
  free(curdir);
  return CurDir;
}

bool pws_os::chdir(const std::wstring &dir)
{
  ASSERT(!dir.empty());
  return (_tchdir(dir.c_str()) == 0);
}

  // In following, drive will be empty on non-Windows platforms
bool pws_os::splitpath(const std::wstring &path,
                       std::wstring &drive, std::wstring &dir,
                       std::wstring &file, std::wstring &ext)
{
  wchar_t tdrv[_MAX_DRIVE];
  wchar_t tdir[_MAX_DIR];
  wchar_t tname[_MAX_FNAME];
  wchar_t text[_MAX_EXT];

  memset(tdrv, 0, sizeof(tdrv));
  memset(tdir, 0, sizeof(tdir));
  memset(tname, 0, sizeof(tname));
  memset(text, 0, sizeof(text));

  if (_wsplitpath_s(path.c_str(), tdrv, tdir, tname, text) == 0) {
    drive = tdrv;
    dir = tdir;
    file = tname;
    ext = text;
    return true;
  } else
    return false;
}

std::wstring pws_os::makepath(const std::wstring &drive, const std::wstring &dir,
                              const std::wstring &file, const std::wstring &ext)
{
  std::wstring retval;
  wchar_t path[_MAX_PATH];

  memset(path, 0, sizeof(path));
  if (_wmakepath_s(path, drive.c_str(), dir.c_str(),
                   file.c_str(), ext.c_str()) == 0)
    retval = path;
  return retval;
}
