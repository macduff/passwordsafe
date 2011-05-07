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

#if defined(_DEBUG) || defined(DEBUG)

// TRACE replacement - only need this Debug mode
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

// Debug output - Same usage as MFC TRACE
void pws_os::zTrace(char *lpszFormat, ...)
{
  openlog("pwsafe:", LOG_PID, LOG_USER);
  va_list args;
  va_start(args, lpszFormat);

  int num_required, num_written;
  num_required = _vscprintf(lpszFormat, args);
  char *szbuffer = new char[num_required + 1];
  num_written = vsprintf(szbuffer, num_required, lpszFormat, args);
  assert(num_required == num_written);
  szbuffer[num_required] = '\0';
  syslog(LOG_DEBUG, szbuffer);

  delete[] szbuffer;
  closelog();

  va_end(args);
}

void pws_os::zTrace0(LPCTSTR lpszFormat)
{
  openlog("pwsafe:", LOG_PID, LOG_USER);

  syslog(LOG_DEBUG, lpszFormat);

  closelog();
}

void pws_os::zTracec(char c)
{
  openlog("pwsafe:", LOG_PID, LOG_USER);

  syslog(LOG_DEBUG, c);

  closelog();
}
#else   /* _DEBUG || DEBUG */
void pws_os::zTrace(LPCTSTR , ...)
{
//  Do nothing in non-Debug mode
}
void pws_os::zTrace0(LPCTSTR )
{
//  Do nothing in non-Debug mode
}
void pws_os::zTracec(char )
{
//  Do nothing in non-Debug mode
}
#endif  /* _DEBUG || DEBUG */
