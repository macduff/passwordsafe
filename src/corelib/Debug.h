/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __DEBUG_H
#define __DEBUG_H
// Debug.h
// Functions that are used mainly (or only) for debugging.
//-----------------------------------------------------------------------------

#include "os/typedefs.h"

namespace PWSDebug {
  // Opens a messagebox with text of last system error, titlebar
  // is csFunction
  void IssueError(const std::wstring &csFunction);
/*
  Produce a printable version of memory dump (hex + ascii)

  parameters:
    pmemory  - pointer to memory to format
    length  - length to format
    cs_prefix - prefix each line with this
    maxnum  - maximum characters dumped per line

  return:
    wstring containing output buffer
*/
  void HexDump(unsigned char *pmemory, const int length,
               const std::wstring &cs_prefix = L"", const int maxnum = 16);
};
#endif /* __DEBUG_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
