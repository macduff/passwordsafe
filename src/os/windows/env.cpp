/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/**
 * \file Windows-specific implementation of env.h
 */

#include <afx.h>
#include <Windows.h> // for GetCurrentProcessId()
#include <LMCONS.H> // for UNLEN definition
#include "../env.h"
#include <sstream>

wstring pws_os::getenv(const char *env, bool is_path)
{
  ASSERT(env != NULL);
  wstring retval;
#if _MSC_VER < 1400
  retval = getenv(env);
#else
  char* value;
  size_t requiredSize;
  getenv_s(&requiredSize, NULL, 0, env);
  if (requiredSize > 0) {
    value = new char[requiredSize];
    ASSERT(value);
    if (value != NULL) {
      getenv_s(&requiredSize, value, requiredSize, env);
      int wsize;
      wchar_t wvalue;
      char *p = value;
      do {
        wsize = mbtowc(&wvalue, p, MB_CUR_MAX);
        if (wsize <= 0)
          break;
        retval += wvalue;
        p += wsize;
        requiredSize -= wsize;
      } while (requiredSize != 1);
      delete[] value;
      if (is_path) {
        // make sure path has trailing '\'
        if (retval[retval.length()-1] != L'\\')
          retval += L"\\";
      }
    }
  }
#endif // _MSC_VER < 1400
  return retval;
}

wstring pws_os::getusername()
{
  wchar_t user[UNLEN + sizeof(wchar_t)];
  //  ulen INCLUDES the trailing blank
  DWORD ulen = UNLEN + sizeof(wchar_t);
  if (::GetUserName(user, &ulen)== FALSE) {
    user[0] = L'?';
    user[1] = L'\0';
    ulen = 2;
  }
  ulen--;
  wstring retval(user);
  return retval;
}

wstring pws_os::gethostname()
{
  //  slen EXCLUDES the trailing blank
  wchar_t sysname[MAX_COMPUTERNAME_LENGTH + sizeof(wchar_t)];
  DWORD slen = MAX_COMPUTERNAME_LENGTH + sizeof(wchar_t);
  if (::GetComputerName(sysname, &slen) == FALSE) {
    sysname[0] = L'?';
    sysname[1] = L'\0';
    slen = 1;
  }
  wstring retval(sysname);
  return retval;
}

wstring pws_os::getprocessid()
{
  wostringstream os;
  os.width(8);
  os.fill(L'0');
  os << GetCurrentProcessId();

  return os.str();
}
