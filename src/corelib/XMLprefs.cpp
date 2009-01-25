/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// XMLprefs.cpp : implementation file
//

#ifdef _WIN32
  #include <afx.h>
#endif
#include "os/typedefs.h"
#include "os/sys.h"
#include "XMLprefs.h"
#include "tinyxml/tinyxml.h"
#include "PWSprefs.h"
#include "corelib.h"
#include "StringXStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define DEBUG_XMLPREFS
#ifdef DEBUG_XMLPREFS
#include <stdio.h>
static FILE *f;
#define DOPEN() do {f = fopen("cxmlpref.log", "a+");} while (0)
#define DPRINT(x) do {fprintf x; fflush(f);} while (0)
#define DCLOSE() fclose(f)
#else
#define DOPEN()
#define DPRINT(x)
#define DCLOSE()
#endif

/////////////////////////////////////////////////////////////////////////////
// CXMLprefs

bool CXMLprefs::Lock()
{
  wstring locker(L"");
  int tries = 10;
  do {
    m_bIsLocked = PWSprefs::LockCFGFile(m_csConfigFile, locker);
    if (!m_bIsLocked) {
      pws_os::sleep(200);
    }
  } while (!m_bIsLocked && --tries > 0);
  return m_bIsLocked;
}

void CXMLprefs::Unlock()
{
  PWSprefs::UnlockCFGFile(m_csConfigFile);
  m_bIsLocked = false;
}

bool CXMLprefs::CreateXML(bool forLoad)
{
  // Call with forLoad set when about to Load, else
  // this also adds a toplevel root element
  ASSERT(m_pXMLDoc == NULL);
  m_pXMLDoc = new TiXmlDocument(m_csConfigFile.c_str());
  if (!forLoad && m_pXMLDoc != NULL) {
    TiXmlDeclaration decl(L"1.0", L"UTF-8", L"yes");
    TiXmlElement rootElem(L"Pwsafe_Settings");

    return (m_pXMLDoc->InsertEndChild(decl) != NULL &&
      m_pXMLDoc->InsertEndChild(rootElem) != NULL);
  } else
    return m_pXMLDoc != NULL;
}

bool CXMLprefs::Load()
{
  // Already loaded?
  if (m_pXMLDoc != NULL) return true;
  DOPEN();
  DPRINT((f, "Entered CXMLprefs::Load()\n"));

  bool alreadyLocked = m_bIsLocked;
  if (!alreadyLocked) {
    if (!Lock()) {
      LoadAString(m_Reason, IDSC_XMLLOCK_CFG_FAILED);
      return false;
    }
  }

  if (!CreateXML(true)) {
    LoadAString(m_Reason, IDSC_XMLCREATE_CFG_FAILED);
    return false;
  }

  bool retval = m_pXMLDoc->LoadFile();

  if (!retval) {
    // an XML load error occurred so display the reason
    Format(m_Reason, IDSC_XMLFILEERROR,
           m_pXMLDoc->ErrorDesc(), m_csConfigFile.c_str(),
           m_pXMLDoc->ErrorRow(), m_pXMLDoc->ErrorCol());
    delete m_pXMLDoc;
    m_pXMLDoc = NULL;
  } // load failed

  // if we locked it, we should unlock it...
  if (!alreadyLocked)
    Unlock();
  DPRINT((f, "Leaving CXMLprefs::Load(), retval = %s\n",
    retval ? "true" : "false"));
  DCLOSE();
  if (retval)
    m_Reason.clear();
  return retval;
}

bool CXMLprefs::Store()
{
  bool retval = false;
  bool alreadyLocked = m_bIsLocked;

  if (!alreadyLocked) {
    if (!Lock()) {
      LoadAString(m_Reason, IDSC_XMLLOCK_CFG_FAILED);
      return false;
    }
  }

  DOPEN();
  DPRINT((f, "Entered CXMLprefs::Store()\n"));
  DPRINT((f, "\tm_pXMLDoc = %p\n", m_pXMLDoc));

  // Although technically possible, it doesn't make sense
  // to create a toplevel document here, since we'd then
  // be saving an empty document.
  ASSERT(m_pXMLDoc != NULL);
  if (m_pXMLDoc == NULL) {
    LoadAString(m_Reason, IDSC_XMLCREATE_CFG_FAILED);
    retval = false;
    goto exit;
  }

  retval = m_pXMLDoc->SaveFile();
  if (!retval) {
    // Get and show error
    Format(m_Reason, IDSC_XMLFILEERROR,
           m_pXMLDoc->ErrorDesc(), m_csConfigFile.c_str(),
           m_pXMLDoc->ErrorRow(), m_pXMLDoc->ErrorCol());
  }

exit:
  // if we locked it, we should unlock it...
  if (!alreadyLocked)
    Unlock();
  DPRINT((f, "Leaving CXMLprefs::Store(), retval = %s\n",
    retval ? "true" : "false"));
  DCLOSE();
  if (retval)
    m_Reason.clear();
  return retval;
}

// get a int value
int CXMLprefs::Get(const wstring &csBaseKeyName, const wstring &csValueName, 
                   int iDefaultValue)
{
  /*
  Since XML is text based and we have no schema, just convert to a string and
  call the Get(String) method.
  */
  int iRetVal = iDefaultValue;
  wostringstream os;
  os << iDefaultValue;
  wistringstream is(Get(csBaseKeyName, csValueName, os.str()));
  is >> iRetVal;

  return iRetVal;
}

// get a string value
wstring CXMLprefs::Get(const wstring &csBaseKeyName, const wstring &csValueName, 
                       const wstring &csDefaultValue)
{
  ASSERT(m_pXMLDoc != NULL); // shouldn't be called if not loaded
  if (m_pXMLDoc == NULL) // just in case
    return csDefaultValue;

  int iNumKeys = 0;
  wstring csValue = csDefaultValue;

  // Add the value to the base key separated by a '\'
  wstring csKeyName(csBaseKeyName);
  csKeyName += L"\\";
  csKeyName += csValueName;

  // Parse all keys from the base key name (keys separated by a '\')
  wstring *pcsKeys = ParseKeys(csKeyName, iNumKeys);

  // Traverse the xml using the keys parsed from the base key name to find the correct node
  if (pcsKeys != NULL) {
    TiXmlElement *rootElem = m_pXMLDoc->RootElement();

    if (rootElem != NULL) {
      // returns the last node in the chain
      TiXmlElement *foundNode = FindNode(rootElem, pcsKeys, iNumKeys);

      if (foundNode != NULL) {
        // get the text of the node (will be the value we requested)
        csValue = wstring(foundNode->GetText());
      }
    }
    delete[] pcsKeys;
  }

  return csValue;
}

// set a int value
int CXMLprefs::Set(const wstring &csBaseKeyName, const wstring &csValueName,
                   int iValue)
{
  /*
  Since XML is text based and we have no schema, just convert to a string and
  call the SetSettingString method.
  */
  int iRetVal = 0;
  wstring csValue = L"";

  Format(csValue, L"%d", iValue);

  iRetVal = Set(csBaseKeyName, csValueName, csValue);

  return iRetVal;
}

// set a string value
int CXMLprefs::Set(const wstring &csBaseKeyName, const wstring &csValueName, 
                   const wstring &csValue)
{
  // m_pXMLDoc may be NULL if Load() not called b4 Set,
  // or if called & failed

  if (m_pXMLDoc == NULL && !CreateXML(false))
    return false;

  int iRetVal = XML_SUCCESS;
  int iNumKeys = 0;

  // Add the value to the base key separated by a '\'
  wstring csKeyName(csBaseKeyName);
  csKeyName += L"\\";
  csKeyName += csValueName;

  // Parse all keys from the base key name (keys separated by a '\')
  wstring *pcsKeys = ParseKeys(csKeyName, iNumKeys);

  // Traverse the xml using the keys parsed from the base key name to find the correct node
  if (pcsKeys != NULL) {
    TiXmlElement *rootElem = m_pXMLDoc->RootElement();

    if (rootElem != NULL) {
      // returns the last node in the chain
      TiXmlElement *foundNode = FindNode(rootElem, pcsKeys, iNumKeys, TRUE);

      if (foundNode != NULL) {
        TiXmlNode *valueNode = foundNode->FirstChild();
        if (valueNode != NULL) // replace existing value
          valueNode->SetValue(csValue.c_str());
        else {// first time set
          TiXmlText value(csValue.c_str());
          foundNode->InsertEndChild(value);
        }
      } else
        iRetVal = XML_NODE_NOT_FOUND;

    } else
      iRetVal = XML_LOAD_FAILED;

    delete [] pcsKeys;
  }
  return iRetVal;
}

// delete a key or chain of keys
bool CXMLprefs::DeleteSetting(const wstring &csBaseKeyName, const wstring &csValueName)
{
  // m_pXMLDoc may be NULL if Load() not called b4 DeleteSetting,
  // or if called & failed

  if (m_pXMLDoc == NULL && !CreateXML(false))
    return false;

  bool bRetVal = false;
  int iNumKeys = 0;
  wstring csKeyName(csBaseKeyName);

  if (!csValueName.empty()) {
    csKeyName += L"\\";
    csKeyName += csValueName;
  }

  // Parse all keys from the base key name (keys separated by a '\')
  wstring *pcsKeys = ParseKeys(csKeyName, iNumKeys);

  // Traverse the xml using the keys parsed from the base key name to find the correct node.
  if (pcsKeys != NULL) {
    TiXmlElement *rootElem = m_pXMLDoc->RootElement();

    if (rootElem != NULL) {
      // returns the last node in the chain
      TiXmlElement *foundNode = FindNode(rootElem, pcsKeys, iNumKeys);

      if (foundNode!= NULL) {
        // get the parent of the found node and use removeChild to delete the found node
        TiXmlNode *parentNode = foundNode->Parent();

        if (parentNode != NULL) {
          if (parentNode->RemoveChild(foundNode)) {
            bRetVal = TRUE;
          }
        }
      }
    }
    delete[] pcsKeys;
  }
  return bRetVal;
}

// Parse all keys from the base key name.
wstring* CXMLprefs::ParseKeys(const wstring &csFullKeyPath, int &iNumKeys)
{
  wstring* pcsKeys = NULL;

  // replace spaces with _ since xml doesn't like them
  wstring csFKP(csFullKeyPath);
  Replace(csFKP, L' ', L'_');

  if (csFKP[csFKP.length() - 1] == L'\\')
    TrimRight(csFKP, L"\\");  // remove slashes on the end

  wstring csTemp(csFKP);

  iNumKeys = Remove(csTemp, L'\\') + 1;  // get a count of slashes

  pcsKeys = new wstring[iNumKeys];  // create storage for the keys

  if (pcsKeys) {
    wstring::size_type iFind = 0, iLastFind = 0;
    int iCount = -1;

    // get all of the keys in the chain
    while (iFind != wstring::npos) {
      iFind = csFKP.find(L"\\", iLastFind);
      if (iFind != wstring::npos) {
        iCount++;
        pcsKeys[iCount] = csFKP.substr(iLastFind, iFind - iLastFind);
        iLastFind = iFind + 1;
      } else {
        // get the last key in the chain
        if (iLastFind < csFKP.length())  {
          iCount++;
          pcsKeys[iCount] = csFKP.substr(csFKP.find_last_of(L"\\")+1);
        }
      }
    }
  }
  return pcsKeys;
}

void CXMLprefs::UnloadXML()
{
  if (m_pXMLDoc != NULL) {
    delete m_pXMLDoc;
    m_pXMLDoc = NULL;
  }
}

// find a node given a chain of key names
TiXmlElement *CXMLprefs::FindNode(TiXmlElement *parentNode,
                                  wstring* pcsKeys, int iNumKeys,
                                  bool bAddNodes /*= false*/)
{
  ASSERT(m_pXMLDoc != NULL); // shouldn't be called if load failed
  if (m_pXMLDoc == NULL) // just in case
    return NULL;

  for (int i=0; i<iNumKeys; i++) {
    // find the node named X directly under the parent
    TiXmlNode *foundNode = parentNode->IterateChildren(pcsKeys[i].c_str(),
                                                       NULL);

    if (foundNode == NULL) {
      // if its not found...
      if (bAddNodes)  {  // create the node and append to parent (Set only)
        TiXmlElement elem(pcsKeys[i].c_str());
        // Add child, set parent to it for next iteration
        parentNode = parentNode->InsertEndChild(elem)->ToElement();
      } else {
        parentNode = NULL;
        break;
      }
    } else {
      // since we are traversing the nodes, we need to set the parentNode to our foundNode
      parentNode = foundNode->ToElement();
      foundNode = NULL;
    }
  }
  return parentNode;
}
