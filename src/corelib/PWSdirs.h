/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef __PWSDIRS_H
#define __PWSDIRS_H

// PWSdirs.h
// Provide directories used by application
//
// Note that GetConfigDir will return value of environment var
// PWS_PREFSDIR if defined.
//
//-----------------------------------------------------------------------------

#include "os/typedefs.h"
#include <stack>

class PWSdirs
{
public:
  PWSdirs() {} // only need to create an object for push/pop
  PWSdirs(const std::wstring &dir) {Push(dir);} // convenience: create & push
  ~PWSdirs(); // does a repeated Pop, so we're back where we started

  static std::wstring GetSafeDir(); // default database location
  static std::wstring GetConfigDir(); // pwsafe.cfg location
  static std::wstring GetXMLDir(); // XML .xsd .xsl files
  static std::wstring GetHelpDir(); // help file(s)
  static std::wstring GetExeDir(); // location of executable

  void Push(const std::wstring &dir); // cd to dir after saving current dir
  void Pop(); // cd to last dir, nop if stack empty

private:
  static std::wstring GetOurExecDir();
  static std::wstring execdir;
  std::stack<std::wstring> dirs;
};

#endif /* __PWSDIRS_H */
