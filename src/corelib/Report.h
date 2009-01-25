/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef __REPORT_H
#define __REPORT_H

// Create an action report file

#ifdef _WIN32
  #include "afx.h"
#endif
#include "os/typedefs.h"
#include "StringXStream.h"

class CReport
{
public:
  CReport() {}
  ~CReport() {}

  void StartReport(LPCWSTR tcAction, const wstring &csDataBase);
  void EndReport();
  void WriteLine(const wstring &cs_line, bool bCRLF = true);
  void WriteLine(const LPWSTR &tc_line, bool bCRLF = true);
  void WriteLine();
  bool SaveToDisk();
  StringX GetString() {return m_osxs.rdbuf()->str();}

private:
  woStringXStream m_osxs;
  wstring m_cs_filename;
  int m_imode;
  wstring m_tcAction;
  wstring m_csDataBase;
};

#endif /* __REPORT_H */
