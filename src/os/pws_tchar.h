/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef _PWS_wchar_t_H

/**
 * Use Windows' wchar.h for Windows build,
 * roll our own for others.
 *
 */

#ifdef _WIN32
#include <wchar.h>
#else
#define _gmtime64_s(ts64, tm64) gmtime64_r(tm64, ts64)
#define _mkgmtime32(ts) mktime(ts)
#include <wctype.h>
#include <wchar.h>
#include "linux/pws_time.h"
#define _wcsicmp(s1, s2) wcscasecmp(s1, s2)
#define _wasctime_s(s, N, st) pws_os::asctime(s, N, st)
#define _vsctprintf(fmt, args) vswprintf(NULL, 0, fmt, args)
#define _vstprintf_s(str, size, fmt, args) vswprintf(str, size, fmt, args)
#include "linux/pws_str.h"
#define _wtoi(s) pws_os::wctoi(s)
#define _wtof(s) pws_os::wctof(s)
#endif /* _WIN32 */

#endif /* _PWS_wchar_t_H */
