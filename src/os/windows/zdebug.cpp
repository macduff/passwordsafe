/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/**
 * \file Windows-specific implementation of debug.h
 */
 
// Everything in zlib is char even if the project is Unicode!!!!!!

// TRACE replacement
#include "../typedefs.h"
#include "../zdebug.h"
#include "../../core/util.h"
#include <wtypes.h>

#if defined(_DEBUG) || defined(DEBUG)

#include <stdio.h>

void zTrace(char *lpszFormat, ...)
{
  va_list args;
  va_start(args, lpszFormat);

  std::string sTimeStamp;
  PWSUtil::GetTimeStampA(sTimeStamp);

  char szBuffer[512];
  int nBuf = _vsnprintf(szBuffer, sizeof(szBuffer),
                         lpszFormat, args);
  _ASSERT(nBuf > -1);

  const char szErrorMsg[] = "zTrace buffer overflow\n";
  const size_t N = strlen(nBuf > -1 ? szBuffer : szErrorMsg) + sTimeStamp.length() + 2;
  char *szDebugString = new char[N];

#if (_MSC_VER >= 1400)
  strcpy_s(szDebugString, N, sTimeStamp.c_str());
  strcat_s(szDebugString, N, " ");
  strcat_s(szDebugString, N, nBuf > -1 ? szBuffer : szErrorMsg);
#else
  strcpy(szDebugString, sTimeStamp.c_str());
  strcat(szDebugString, " ");
  strcat(szDebugString, nBuf > -1 ? szBuffer : szErrorMsg);
#endif

  std::string temp = szDebugString;
  stringT xtemp;
  xtemp.assign(temp.begin(), temp.end());
  ::OutputDebugString(xtemp.c_str());
  delete [] szDebugString;

  va_end(args);
}

void zTrace0(char *lpszFormat)
{
  std::string sTimeStamp;
  PWSUtil::GetTimeStampA(sTimeStamp);

  const size_t N = strlen(lpszFormat) + sTimeStamp.length() + 2;
  char *szDebugString = new char[N];

#if (_MSC_VER >= 1400)
  strcpy_s(szDebugString, N, sTimeStamp.c_str());
  strcat_s(szDebugString, N, " ");
  strcat_s(szDebugString, N, lpszFormat);
#else
  strcpy(szDebugString, sTimeStamp.c_str());
  strcat(szDebugString, " ");
  strcat(szDebugString, lpszFormat);
#endif

  std::string temp = szDebugString;
  stringT xtemp;
  xtemp.assign(temp.begin(), temp.end());
  ::OutputDebugString(xtemp.c_str());
  delete [] szDebugString;
}

void zTracec(char c)
{
  char c2[] = {'\0', '\0'};
  c2[0] = c;
  std::string temp = c2;
  stringT xtemp;
  xtemp.assign(temp.begin(), temp.end());
  ::OutputDebugString(xtemp.c_str());
}

#else   /* _DEBUG || DEBUG */
void zTrace(char * , ...)
{
//  Do nothing in non-Debug mode
}
void zTrace0(char *)
{
//  Do nothing in non-Debug mode
}
void zTracec(char)
{
//  Do nothing in non-Debug mode
}
#endif  /* _DEBUG || DEBUG */
