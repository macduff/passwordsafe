/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/**
 * \file Linux-specific implementation of env.h
 */

#include <sstream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>

#include "../env.h"

static std::wstring towc(const char *val)
{
  std::wstring retval;
  assert(val != NULL);
  int len = strlen(val);
  int wsize;
  const char *p = val;
  wchar_t wvalue;
  do {
    wsize = mbtowc(&wvalue, p, MB_CUR_MAX);
    if (wsize <= 0)
      break;
    retval += wvalue;
    p += wsize;
    len -= wsize;
  } while (len != 1);
  return retval;
}

std::wstring pws_os::getenv(const char *env, bool is_path)
{
  assert(env != NULL);
  std::wstring retval;
  char *value = getenv(env);
  if (value != NULL) {
    retval = towc(value);
    if (is_path) {
      // make sure path has trailing '\'
      if (retval[retval.length()-1] != L'/')
        retval += L"/";
    } // is_path
  } // value != NULL
  return retval;
}

std::wstring pws_os::getusername()
{
  std::wstring retval;
  const char *user = getlogin();
  if (user == NULL)
    user = "?";
  retval = towc(user);
  return retval;
}

std::wstring pws_os::gethostname()
{
  std::wstring retval;
  char name[HOST_NAME_MAX];
  if (::gethostname(name, HOST_NAME_MAX) != 0) {
    assert(0);
    name[0] = '?'; name[1] = '\0';
  }
  retval = towc(name);
  return retval;
}

std::wstring pws_os::getprocessid()
{
  std::wostringstream os;
  os.width(8);
  os.fill(L'0');
  os << getpid();

  return os.str();
}
