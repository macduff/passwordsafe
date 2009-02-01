/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "os/env.h"
#include "os/dir.h"

#include "PWSdirs.h"

/**
* Provide directories used by application
* The functions here return values that cause the application
* to default to current behaviour, UNLESS the U3 (www.u3.com) env. vars
* are defined, in which case values corresponding to the U3 layout are used,
* as follows (corrsponding to the layout in the .u3p package):
*
* GetSafeDir()   Default database directory:
*                U3_DEVICE_DOCUMENT_PATH\My Safes\
* GetConfigDir() Location of configuration file:
*                U3_APP_DATA_PATH
*                NOTE: PWS_PREFSDIR can be set to override this!
* GetXMLDir()    Location of XML .xsd and .xsl files:
*                U3_APP_DATA_PATH\xml\
* GetHelpDir()   Location of help file(s):
*                U3_DEVICE_EXEC_PATH
* GetExeDir()    Location of executable:
*                U3_HOST_EXEC_PATH
*/

std::wstring PWSdirs::execdir;
//-----------------------------------------------------------------------------

std::wstring PWSdirs::GetOurExecDir()
{
  if (execdir.empty())
    execdir = pws_os::getexecdir();
  return execdir;
}

std::wstring PWSdirs::GetSafeDir()
{
  // returns empty string unless U3 environment detected
  std::wstring retval(pws_os::getenv("U3_DEVICE_DOCUMENT_PATH", true));
  if (!retval.empty())
    retval += L"My Safes\\";
  return retval;
}

std::wstring PWSdirs::GetConfigDir()
{
  // PWS_PREFSDIR overrides all:
  std::wstring retval(pws_os::getenv("PWS_PREFSDIR", true));
  if (retval.empty()) {
    // returns directory of executable unless U3 environment detected
    retval = pws_os::getenv("U3_APP_DATA_PATH", true);
    if (retval.empty())
      retval = GetOurExecDir();
  }
  return retval;
}

std::wstring PWSdirs::GetXMLDir()
{
  std::wstring retval(pws_os::getenv("U3_APP_DATA_PATH", true));
  if (!retval.empty())
    retval += L"\\xml\\";
  else {
    retval = GetOurExecDir();
  }
  return retval;
}

std::wstring PWSdirs::GetHelpDir()
{
  std::wstring retval(pws_os::getenv("U3_DEVICE_EXEC_PATH", true));
  if (retval.empty()) {
    retval = GetOurExecDir();
  }
  return retval;
}

std::wstring PWSdirs::GetExeDir()
{
  std::wstring retval(pws_os::getenv("U3_HOST_EXEC_PATH", true));
  if (retval.empty()) {
    retval = GetOurExecDir();
  }
  return retval;
}

void PWSdirs::Push(const std::wstring &dir)
{
  const std::wstring CurDir(pws_os::getcwd());
  if (CurDir != dir) { // minor optimization
    dirs.push(CurDir);
    pws_os::chdir(dir);
  }
}

void PWSdirs::Pop()
{
  if (!dirs.empty()) {
    pws_os::chdir(dirs.top());
    dirs.pop();
  }
}

PWSdirs::~PWSdirs()
{
  while (!dirs.empty()) {
    pws_os::chdir(dirs.top());
    dirs.pop();
  }
}
