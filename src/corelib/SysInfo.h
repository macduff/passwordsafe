/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __SYSINFO_H
#define __SYSINFO_H

// SysInfo.h
//-----------------------------------------------------------------
//
// Singleton class to provide system-specific information,
// such as hostname, username, pid
//
// As in Unix, we support the concept of an "effective" v.s. "real"
// name. The "real" is read from the system API, and is immutable.
// The "effecitve" is initially set to be == the real value, but
// may be overridden by the relevant SetEffective*() member function.
//-----------------------------------------------------------------
#include "os/typedefs.h"

class SysInfo
{
public:
  static SysInfo *GetInstance(); // singleton
  static void DeleteInstance();

  static bool IsUnderU3();

  void SetEffectiveUser(const wstring &u) {m_euser = u;}
  void SetEffectiveHost(const wstring &h) {m_esysname = h;}

  const wstring &GetRealUser() const {return m_ruser;}
  const wstring &GetRealHost() const {return m_rsysname;}
  const wstring &GetEffectiveUser() const {return m_euser;}
  const wstring &GetEffectiveHost() const {return m_esysname;}
  const wstring &GetCurrentPID() const {return m_ProcessID;}

private:
  SysInfo();
  ~SysInfo() {};

  static SysInfo *self;

  wstring m_ruser, m_rsysname;
  wstring m_euser, m_esysname;
  wstring m_ProcessID;
};
#endif /* __SYSINFO_H */
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
