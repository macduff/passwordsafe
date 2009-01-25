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
#include <vector>

using namespace std;

namespace pws_os {
  extern bool FileExists(const wstring &filename);
  extern bool FileExists(const wstring &filename, bool &bReadOnly);
  extern bool RenameFile(const wstring &oldname, const wstring &newname);
  extern bool CopyAFile(const wstring &from, const wstring &to); // creates dirs as needed!
  extern bool DeleteAFile(const wstring &filename);
  extern void FindFiles(const wstring &filter, vector<wstring> &res);
  extern bool LockFile(const wstring &filename, wstring &locker,
                       HANDLE &lockFileHandle, int &LockCount);
  extern bool IsLockedFile(const wstring &filename);
  extern void UnlockFile(const wstring &filename,
                         HANDLE &lockFileHandle, int &LockCount);

  extern FILE *FOpen(const wstring &filename, const wchar_t *mode);
  extern long fileLength(FILE *fp);
  extern const wchar_t *PathSeparator; // slash for Unix, backslash for Windows
};

#endif /* __FILE_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
