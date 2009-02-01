/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __DIR_H
#define __DIR_H

#include "typedefs.h"

namespace pws_os {
  extern std::wstring getexecdir(); // path of executable
  extern std::wstring getcwd();
  extern bool chdir(const std::wstring &dir);
  // In following, drive will be empty on non-Windows platforms
  extern bool splitpath(const std::wstring &path,
                        std::wstring &drive, std::wstring &dir,
                        std::wstring &file, std::wstring &ext);
  extern std::wstring makepath(const std::wstring &drive, const std::wstring &dir,
                               const std::wstring &file, const std::wstring &ext);
};

#endif /* __DIR_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
