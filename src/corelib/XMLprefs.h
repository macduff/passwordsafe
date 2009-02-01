/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __XMLPREFS_H
#define __XMLPREFS_H

#include "os/typedefs.h"
/////////////////////////////////////////////////////////////////////////////
// CXMLprefs class
//
// This class wraps access to an XML file containing user preferences.
// Usage scenarios:
// 1. Load() followed by zero or more Get()s
// 2. Lock(), Load(), zero or more Set()s, zero or more
//    DeleteSetting()s, Store(), Unlock()
/////////////////////////////////////////////////////////////////////////////
class TiXmlDocument;
class TiXmlElement;

class CXMLprefs
{
  // Construction & Destruction
 public:
 CXMLprefs(const std::wstring &configFile)
   : m_pXMLDoc(NULL), m_csConfigFile(configFile), m_bIsLocked(false)
    {}

  ~CXMLprefs() { UnloadXML(); }

  // Implementation
 public:
  bool Load();
  bool Store();
  bool Lock();
  void Unlock();

  int Get(const std::wstring &csBaseKeyName, const std::wstring &csValueName,
          int iDefaultValue);
  std::wstring Get(const std::wstring &csBaseKeyName, const std::wstring &csValueName,
              const std::wstring &csDefaultValue);

  int Set(const std::wstring &csBaseKeyName, const std::wstring &csValueName,
          int iValue);
  int Set(const std::wstring &csBaseKeyName, const std::wstring &csValueName,
          const std::wstring &csValue);

  bool DeleteSetting(const std::wstring &csBaseKeyName, const std::wstring &csValueName);
  std::wstring getReason() const {return m_Reason;} // why something went wrong
  
  enum {XML_SUCCESS = 0, XML_LOAD_FAILED, XML_NODE_NOT_FOUND, XML_PUT_TEXT_FAILED, XML_SAVE_FAILED};

 private:
  TiXmlDocument *m_pXMLDoc;
  std::wstring m_csConfigFile;
  bool m_bIsLocked;

  std::wstring* ParseKeys(const std::wstring &csFullKeyPath, int &iNumKeys);
  bool CreateXML(bool forLoad); // forLoad will skip creation of root element
  void UnloadXML();
  TiXmlElement *FindNode(TiXmlElement *parentNode, std::wstring* pcsKeys,
                         int iNumKeys, bool bAddNodes = false);
  std::wstring m_Reason; // why something bad happenned
};

#endif /* __XMLPREFS_H */
