/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __FILE_H
#define __FILE_H

#include "typedefs.h"
#include <cstdio>

#include <string>
#include <vector>

namespace pws_os {
  extern bool FileExists(const std::wstring &filename);
  extern bool FileExists(const std::wstring &filename, bool &bReadOnly);
  extern bool RenameFile(const std::wstring &oldname, const std::wstring &newname);
  extern bool CopyAFile(const std::wstring &from, const std::wstring &to); // creates dirs as needed!
  extern bool DeleteAFile(const std::wstring &filename);
  extern void FindFiles(const std::wstring &filter, std::vector<std::wstring> &res);
  extern bool LockFile(const std::wstring &filename, std::wstring &locker,
                       HANDLE &lockFileHandle, int &LockCount);
  extern bool IsLockedFile(const std::wstring &filename);
  extern void UnlockFile(const std::wstring &filename,
                         HANDLE &lockFileHandle, int &LockCount);

  extern FILE *FOpen(const std::wstring &filename, const wchar_t *mode);
  extern long fileLength(FILE *fp);
  extern const wchar_t *PathSeparator; // slash for Unix, backslash for Windows
};

#endif /* __FILE_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
