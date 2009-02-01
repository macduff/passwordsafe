/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "Report.h"
#include "Util.h"
#include "Debug.h"
#include "corelib.h"
#include "os/dir.h"
#include "os/file.h"
#include "os/utf8conv.h"
#include "StringX.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include <sstream>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
const wchar_t *CRLF = L"\r\n";

/*
  It writes a header record and a "Start Report" record.
*/
void CReport::StartReport(LPCWSTR tcAction, const std::wstring &csDataBase)
{
  m_osxs.str(L"");

  m_tcAction = tcAction;
  m_csDataBase = csDataBase;

  std::wstring cs_title;
  Format(cs_title, IDSC_REPORT_TITLE1, tcAction,
                  PWSUtil::GetTimeStamp());
  WriteLine();
  WriteLine(cs_title);
  Format(cs_title, IDSC_REPORT_TITLE2, csDataBase.c_str());
  WriteLine(cs_title);
  WriteLine();
  LoadAString(cs_title, IDSC_START_REPORT);
  WriteLine(cs_title);
  WriteLine();
}

static bool isFileUnicode(const std::wstring &fname)
{
  char *fn = NULL;
  size_t fnlen = 0;
  fnlen = pws_os::wcstombs(fn, fnlen, fname.c_str(), fname.length()) + 1;
  fn = new char[fnlen];
  fnlen = pws_os::wcstombs(fn, fnlen, fname.c_str(), fname.length());
  std::ifstream is(fn);
  delete[] fn;
  unsigned char buffer[] = {0x00, 0x00};
  const unsigned char BOM[] = {0xff, 0xfe};
  if (!is.read((char *)buffer, sizeof(buffer)))
    return false;
  return (buffer[0] == BOM[0] && buffer[1] == BOM[1]);
}

/*
  SaveToDisk creates a new file of name "<tcAction>_Report.txt" e.g. "Merge_Report.txt"
  in the same directory as the current database or appends to this file if it already exists.
*/
bool CReport::SaveToDisk()
{
  FILE *fd;

  std::wstring path(m_csDataBase);
  std::wstring drive, dir, file, ext;
  if (!pws_os::splitpath(path, drive, dir, file, ext)) {
    PWSDebug::IssueError(L"StartReport: Finding path to database");
    return false;
  }

  Format(m_cs_filename, IDSC_REPORTFILENAME,
         drive.c_str(), dir.c_str(), m_tcAction.c_str());

  if ((fd = pws_os::FOpen(m_cs_filename, L"a+b")) == NULL) {
    PWSDebug::IssueError(L"StartReport: Opening log file");
    return false;
  }

  // **** MOST LIKELY ACTION ****
  // If file is new/emtpy AND we are UNICODE, write BOM, as some text editors insist!

  // **** LEAST LIKELY ACTIONS as it requires the user to use both U & NU versions ****
  // Text editors really don't like files with both UNICODE and ASCII characters, so -
  // If we are UNICODE and file is not, convert file to UNICODE before appending
  // If we are not UNICODE but file is, convert file to ASCII before appending

  bool bFileIsUnicode = isFileUnicode(m_cs_filename);

  const unsigned int iBOM = 0xFEFF;
  if (pws_os::fileLength(fd) == 0) {
    // File is empty - write BOM
    putwc(iBOM, fd);
  } else {
    if (!bFileIsUnicode) {
      // Convert ASCII contents to UNICODE

      // Close original first
      fclose(fd);

      // Open again to read
      FILE *f_in = pws_os::FOpen(m_cs_filename, L"rb");

      // Open new file
      std::wstring cs_out = m_cs_filename + L".tmp";
      FILE *f_out = pws_os::FOpen(cs_out, L"wb");

      // Write BOM
      putwc(iBOM, f_out);

      UINT nBytesRead;
      unsigned char inbuffer[4096];
      wchar_t outwbuffer[4096];

      // Now copy
      do {
        nBytesRead = fread(inbuffer, sizeof(inbuffer), 1, f_in);

        if (nBytesRead > 0) {
          size_t len = pws_os::mbstowcs(outwbuffer, 4096, (const char *)inbuffer, nBytesRead);
          if (len != 0)
            fwrite(outwbuffer, sizeof(outwbuffer[0])*len, 1, f_out);
        } else
          break;

      } while(nBytesRead > 0);

      // Close files
      fclose(f_in);
      fclose(f_out);

      // Swap them
      pws_os::RenameFile(cs_out, m_cs_filename);

      // Re-open file
      if ((fd = pws_os::FOpen(m_cs_filename, L"ab")) == NULL) {
        PWSDebug::IssueError(L"StartReport: Opening log file");
        return false;
      }
    }
  }
  StringX sx = m_osxs.rdbuf()->str();
  fwrite((void *)sx.c_str(), sizeof(BYTE), sx.length() * sizeof(wchar_t), fd);
  fclose(fd);

  return true;
}

// Write a record with(default) or without a CRLF
void CReport::WriteLine(const std::wstring &cs_line, bool bCRLF)
{
  m_osxs << cs_line.c_str();
  if (bCRLF) {
    m_osxs << CRLF;
  }
}

// Write a record with(default) or without a CRLF
void CReport::WriteLine(const LPWSTR &tc_line, bool bCRLF)
{
  m_osxs << tc_line;
  if (bCRLF) {
    m_osxs << CRLF;
  }
}

// Write a new line
void CReport::WriteLine()
{
  m_osxs << CRLF;
}

/*
  EndReport writes a "End Report" record and closes the report file.
*/
void CReport::EndReport()
{
  WriteLine();
  std::wstring cs_title;
  LoadAString(cs_title, IDSC_END_REPORT1);
  WriteLine(cs_title);
  LoadAString(cs_title, IDSC_END_REPORT2);
  WriteLine(cs_title);

  m_osxs.flush();
}
