/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef _TYPEDEFS_H
/**
* Silly wrapper to abstract away the difference between a Unicode
* (wchar_t) and non-Unicode (char) string, as well as
* Linux/Windows portability.
*
*/

#include <string>

using namespace std;

#ifdef _WIN32
#include <wchar.h>

typedef char             int8;
typedef short            int16;
typedef int              int32;
typedef __int64          int64;

typedef unsigned int     uint;
typedef unsigned char    uint8;
typedef unsigned short   uint16;
typedef unsigned int     uint32;
typedef unsigned __int64 uint64;
typedef unsigned __int64 ulong64;
typedef unsigned long    ulong32;

typedef void             *HANDLE;

#else /* !defined(_WIN32) */
#include <sys/types.h>
#include "linux/pws_time.h"

typedef int8_t           int8;
typedef int16_t          int16;
typedef int32_t          int32;
typedef int64_t          int64;

typedef u_int8_t         uint8;
typedef u_int16_t        uint16;
typedef u_int32_t        uint32;
typedef u_int64_t        uint64;

typedef int              errno_t;

// mimic Microsoft conventional typdefs:
typedef wchar_t         *LPWSTR;
typedef const wchar_t   *LPWTSTR;
typedef bool             BOOL;
typedef unsigned char    BYTE;
typedef unsigned short   WORD;
typedef unsigned long    WORD;
typedef long             LPARAM;
typedef unsigned int     UINT;
typedef int              HANDLE;

#define LOWORD(ul) (WORD(DWORD(ul) & 0xffff))
#define HIWORD(ul) (WORD(DWORD(ul) >> 16))
#define INVALID_HANDLE_VALUE HANDLE (-1)

// assorted conveniences:
#define ASSERT(p) assert(p)
#define VERIFY(p) if (!(p)) TRACE("VERIFY Failed")
#define TRACE(...) // nothing, for now...
#define TRACE0(...) // nothing, for now...
#ifndef TRUE
  #define TRUE true
#endif
#ifndef FALSE
  #define FALSE false
#endif
#endif /* _WIN32 */
          
#endif /* _TYPEDEFS_H */
