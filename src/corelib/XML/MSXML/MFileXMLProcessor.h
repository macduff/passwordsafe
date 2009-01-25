/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// MFileXMLProcessor.h : header file
//

#ifndef __MFILEXMLPROCESSOR_H
#define __MFILEXMLPROCESSOR_H

#include "../../UnknownField.h"
#include "../../UUIDGen.h"
#include "os/typedefs.h"

#include <vector>

using namespace std;

typedef vector<CUUIDGen> UUIDList;

class PWScore;

class MFileXMLProcessor
{
public:
  MFileXMLProcessor(PWScore *core, UUIDList *possible_aliases, UUIDList *possible_shortcuts);
  ~MFileXMLProcessor();

  bool Process(const bool &bvalidation, const wstring &ImportedPrefix,
    const wstring &strXMLFileName, const wstring &strXSDFileName,
    int &nITER, int &nRecordsWithUnknownFields, UnknownFieldList &uhfl);

  wstring getResultText() {return m_strResultText;}
  int getNumEntriesValidated() {return m_numEntriesValidated;}
  int getNnumEntriesImported() {return m_numEntriesImported;}
  bool getIfDatabaseHeaderErrors() {return m_bDatabaseHeaderErrors;}
  bool getIfRecordHeaderErrors() {return m_bRecordHeaderErrors;}

private:
  PWScore *m_xmlcore;
  UUIDList *m_possible_aliases;
  UUIDList *m_possible_shortcuts;
  wstring m_strResultText;
  int m_numEntriesValidated, m_numEntriesImported, m_MSXML_Version;
  wchar_t m_delimiter;
  bool m_bDatabaseHeaderErrors, m_bRecordHeaderErrors;
  bool m_bValidation;
};

#endif /* __MFILEXMLPROCESSOR_H */
