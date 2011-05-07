/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// Everything in zlib is char even if the project is Unicode!!!!!!

#include "../typedefs.h"
#include "../zdebug.h"
#include "../core/Util.h"

#if defined(_DEBUG) || defined(DEBUG)

// TRACE replacement - only need this Debug mode
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

// See discussion on CONVERT_GLIBC_FORMATSPECS in core/StringX.cpp
#if defined(__GNUC__)  && (defined(UNICODE) || defined(_UNICODE))
#define CONVERT_GLIBC_FORMATSPECS
#endif


// Debug output - Same usage as MFC TRACE
void pws_os::zTrace(char *lpszFormat, ...)
{
  openlog("pwsafe:", LOG_PID|LOG_PERROR, LOG_USER);
  va_list args;
  va_start(args, lpszFormat);

  int num_required, num_written;

  num_required = GetStringBufSizeA(lpszFormat, args);
  va_end(args);  //after using args we should reset list
  va_start(args, lpszFormat);

  char *szbuffer = new char[num_required];
  num_written = vsnprintf(szbuffer, num_required, lpszFormat, args);
  assert(num_required == num_written+1);
  szbuffer[num_required-1] = '\0';

  syslog(LOG_DEBUG, "%s", szbuffer);

  delete[] szbuffer;
  closelog();

  va_end(args);
}

void pws_os::zTrace0(char *lpszFormat)
{
  openlog("pwsafe:", LOG_PID|LOG_PERROR, LOG_USER);

  syslog(LOG_DEBUG, "%s", lpszFormat);

  closelog();
}

void pws_os::zTracec(char c)
{
  openlog("pwsafe:", LOG_PID, LOG_USER);

  syslog(LOG_DEBUG, c);

  closelog();
}
#else   /* _DEBUG || DEBUG */
void pws_os::zTrace(char * , ...)
{
//  Do nothing in non-Debug mode
}
void pws_os::zTrace0(char * )
{
//  Do nothing in non-Debug mode
}
void pws_os::zTracec(char )
{
//  Do nothing in non-Debug mode
}
#endif  /* _DEBUG || DEBUG */
