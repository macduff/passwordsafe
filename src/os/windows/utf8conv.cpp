/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/**
 * \file Windows-specific implementation of utf8conv.h
 */

#include <Windows.h>
#include "../typedefs.h"
#include "../utf8conv.h"

#include <locale.h>

class Startup {
public:
  Startup() {
    wchar_t *sl = _wsetlocale(LC_ALL, L"");
    if (sl == NULL)
      throw "Couldn't initialize locale - bailing out";
  }
};

static Startup startup;

size_t pws_os::wcstombs(char *dst, size_t maxdstlen,
                        const wchar_t *src, size_t srclen, bool isUTF8)
{
  UINT codePage = isUTF8 ? CP_UTF8 : CP_ACP;
  
  if (dst == NULL || maxdstlen == 0) {
    dst = NULL; maxdstlen = 0; // resolve ambiguity
  }

  return WideCharToMultiByte(codePage, 0,
                             src, srclen, dst, maxdstlen,
                             NULL, NULL);
}

size_t pws_os::mbstowcs(wchar_t *dst, size_t maxdstlen,
                        const char *src, size_t srclen, bool isUTF8)
{
  UINT codePage;
  DWORD flags;

  if (isUTF8) {
    codePage = CP_UTF8; flags = 0;
  } else {
    codePage = CP_ACP; flags = MB_PRECOMPOSED;
  }

  if (dst == NULL || maxdstlen == 0) {
    dst = NULL; maxdstlen = 0; // resolve ambiguity
  }
  return MultiByteToWideChar(codePage, flags,
                             src, srclen,
                             dst, maxdstlen);
}
