/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// MFilterSAX2Filters.cpp : implementation file
//

#include "../XMLDefs.h"

#if USE_XML_LIBRARY == MSXML

#include "MFilterSAX2Handlers.h"
#include <msxml6.h>

#include "../../util.h"
#include "../../UUIDGen.h"
#include "../../corelib.h"
#include "../../PWSfileV3.h"
#include "../../PWSFilters.h"
#include "../../PWSprefs.h"
#include "../../VerifyFormat.h"
#include "../../Match.h"

#include <map>
#include <algorithm>

typedef std::vector<std::wstring>::const_iterator vciter;
typedef std::vector<std::wstring>::iterator viter;

// Stop warnings about unused formal parameters!
#pragma warning(disable : 4100)

//  -----------------------------------------------------------------------
//  MFilterSAX2ErrorHandler Methods
//  -----------------------------------------------------------------------
MFilterSAX2ErrorHandler::MFilterSAX2ErrorHandler()
  : bErrorsFound(FALSE), m_strValidationResult(L"")
{
  m_refCnt = 0;
}

MFilterSAX2ErrorHandler::~MFilterSAX2ErrorHandler()
{
}

long __stdcall MFilterSAX2ErrorHandler::QueryInterface(const struct _GUID &riid,void ** ppvObject)
{
  *ppvObject = NULL;
  if (riid == IID_IUnknown ||riid == __uuidof(ISAXContentHandler))
  {
    *ppvObject = static_cast<ISAXErrorHandler *>(this);
  }

  if (*ppvObject)
  {
    AddRef();
    return S_OK;
  }
  else return E_NOINTERFACE;
}

unsigned long __stdcall MFilterSAX2ErrorHandler::AddRef()
{
  return ++m_refCnt; // NOT thread-safe
}

unsigned long __stdcall MFilterSAX2ErrorHandler::Release()
{
  --m_refCnt; // NOT thread-safe
  if (m_refCnt == 0) {
    delete this;
    return 0; // Can't return the member of a deleted object.
  }
  else return m_refCnt;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ErrorHandler::error(struct ISAXLocator * pLocator,
                                                         const wchar_t * pwchErrorMessage,
                                                         HRESULT hrErrorCode )
{
  wchar_t szErrorMessage[MAX_PATH*2] = {0};
  wchar_t szFormatString[MAX_PATH*2] = {0};
  int iLineNumber, iCharacter;

#if (_MSC_VER >= 1400)
  wcscpy_s(szErrorMessage, MAX_PATH * 2, pwchErrorMessage);
#else
  wcscpy(szErrorMessage, pwchErrorMessage);
#endif
  pLocator->getLineNumber(&iLineNumber);
  pLocator->getColumnNumber(&iCharacter);

  std::wstring cs_format;
  LoadAString(cs_format, IDSC_MSXMLSAXGENERROR);

#if (_MSC_VER >= 1400)
  _stprintf_s(szFormatString, MAX_PATH * 2, cs_format.c_str(),
              hrErrorCode, iLineNumber, iCharacter, szErrorMessage);
#else
  _stprintf(szFormatString, cs_format.c_str(),
              hrErrorCode, iLineNumber, iCharacter, szErrorMessage);
#endif

  m_strValidationResult += szFormatString;

  bErrorsFound = TRUE;

  return S_OK;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ErrorHandler::fatalError(struct ISAXLocator * pLocator,
                                                          const wchar_t * pwchErrorMessage,
                                                          HRESULT hrErrorCode )
{
  return S_OK;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ErrorHandler::ignorableWarning(struct ISAXLocator * pLocator,
                                                                const wchar_t * pwchErrorMessage,
                                                                HRESULT hrErrorCode )
{
  return S_OK;
}

//  -----------------------------------------------------------------------
//  MFilterSAX2ContentHandler Methods
//  -----------------------------------------------------------------------
MFilterSAX2ContentHandler::MFilterSAX2ContentHandler()
{
  m_refCnt = 0;
  m_strElemContent.clear();
  m_pSchema_Version = NULL;
  m_iXMLVersion = -1;
  m_iSchemaVersion = -1;
  m_pAsker = NULL;
}

//  -----------------------------------------------------------------------
MFilterSAX2ContentHandler::~MFilterSAX2ContentHandler()
{
}

long __stdcall MFilterSAX2ContentHandler::QueryInterface(const struct _GUID &riid,void ** ppvObject)
{
  *ppvObject = NULL;
  if (riid == IID_IUnknown ||riid == __uuidof(ISAXContentHandler)) {
    *ppvObject = static_cast<ISAXContentHandler *>(this);
  }

  if (*ppvObject) {
    AddRef();
    return S_OK;
  }
  else return E_NOINTERFACE;
}

unsigned long __stdcall MFilterSAX2ContentHandler::AddRef()
{
  return ++m_refCnt; // NOT thread-safe
}

unsigned long __stdcall MFilterSAX2ContentHandler::Release()
{
  --m_refCnt; // NOT thread-safe
  if (m_refCnt == 0) {
    delete this;
    return 0; // Can't return the member of a deleted object.
  }
  else return m_refCnt;
}

//  -----------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::startDocument ( )
{
  m_strImportErrors = L"";
  m_bentrybeingprocessed = false;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::putDocumentLocator (struct ISAXLocator * pLocator )
{
  return S_OK;
}

//  ---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::startElement(
  /* [in] */ const wchar_t __RPC_FAR *pwchNamespaceUri,
  /* [in] */ int cchNamespaceUri,
  /* [in] */ const wchar_t __RPC_FAR *pwchLocalName,
  /* [in] */ int cchLocalName,
  /* [in] */ const wchar_t __RPC_FAR *pwchRawName,
  /* [in] */ int cchRawName,
  /* [in] */ ISAXAttributes __RPC_FAR *pAttributes)
{
  wchar_t szCurElement[MAX_PATH + 1] = {0};

#if (_MSC_VER >= 1400)
  wcsncpy_s(szCurElement, MAX_PATH + 1, pwchRawName, cchRawName);
#else
  wcsncpy(szCurElement, pwchRawName, cchRawName);
#endif

  if (m_bValidation && wcscmp(szCurElement, L"filters") == 0) {
    int iAttribs = 0;
    if (m_pSchema_Version == NULL) {
      LoadAString(m_strImportErrors, IDSC_MISSING_SCHEMA_VER);
      return E_FAIL;
    }

    m_iSchemaVersion = _wtoi(*m_pSchema_Version);
    if (m_iSchemaVersion <= 0) {
      LoadAString(m_strImportErrors, IDSC_INVALID_SCHEMA_VER);
      return E_FAIL;
    }

    pAttributes->getLength(&iAttribs);
    for (int i = 0; i < iAttribs; i++) {
      wchar_t szQName[MAX_PATH + 1] = {0};
      wchar_t szValue[MAX_PATH + 1] = {0};
      const wchar_t *QName, *Value;
      int QName_length, Value_length;

      pAttributes->getQName(i, &QName, &QName_length);
      pAttributes->getValue(i, &Value, &Value_length);
#if (_MSC_VER >= 1400)
      wcsncpy_s(szQName, MAX_PATH + 1, QName, QName_length);
      wcsncpy_s(szValue, MAX_PATH + 1, Value, Value_length);
#else
      wcsncpy(szQName, QName, QName_length);
      wcsncpy(szValue, Value, Value_length);
#endif
      if (QName_length == 7 && memcmp(szQName, L"version", 7 * sizeof(wchar_t)) == 0) {
        m_iXMLVersion = _wtoi(szValue);
      }
    }
  }

  if (m_bValidation || wcscmp(szCurElement, L"filters") == 0)
    return S_OK;

  bool bfilter = (wcscmp(szCurElement, L"filter") == 0);
  bool bfilter_entry = (wcscmp(szCurElement, L"filter_entry") == 0);

   if (bfilter) {
    cur_filter = new st_filters;
  }

  if (bfilter_entry) {
    cur_filterentry = new st_FilterRow;
    cur_filterentry->Empty();
    cur_filterentry->bFilterActive = true;
    m_bentrybeingprocessed = true;
  }

  if (bfilter || bfilter_entry) {
    // Process the attributes we need.
    int iAttribs = 0;
    pAttributes->getLength(&iAttribs);
    for (int i = 0; i < iAttribs; i++) {
      wchar_t szQName[MAX_PATH + 1] = {0};
      wchar_t szValue[MAX_PATH + 1] = {0};
      const wchar_t *QName, *Value;
      int QName_length, Value_length;

      pAttributes->getQName(i, &QName, &QName_length);
      pAttributes->getValue(i, &Value, &Value_length);
#if (_MSC_VER >= 1400)
      wcsncpy_s(szQName, MAX_PATH + 1, QName, QName_length);
      wcsncpy_s(szValue, MAX_PATH + 1, Value, Value_length);
#else
      wcsncpy(szQName, QName, QName_length);
      wcsncpy(szValue, Value, Value_length);
#endif

      if (bfilter && QName_length == 10 && memcmp(szQName, L"filtername", 10 * sizeof(wchar_t)) == 0)
        cur_filter->fname = szValue;

      if (bfilter_entry && QName_length == 6 && memcmp(szQName, L"active", 6 * sizeof(wchar_t)) == 0) {
        if (Value_length == 2 && memcmp(szValue, L"no", 2 * sizeof(wchar_t)) == 0)
          cur_filterentry->bFilterActive = false;
      }
    }
  }

  m_strElemContent = L"";

  return S_OK;
}

//  ---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::characters(
  /* [in] */ const wchar_t __RPC_FAR *pwchChars,
  /* [in] */ int cchChars)
{
  if (m_bValidation)
    return S_OK;

  wchar_t* szData = new wchar_t[cchChars + 2];

#if (_MSC_VER >= 1400)
  wcsncpy_s(szData, cchChars + 2, pwchChars, cchChars);
#else
  wcsncpy(szData, pwchChars, cchChars);
#endif

  szData[cchChars]=0;
  m_strElemContent += szData;

  delete [] szData;
  szData = NULL;

  return S_OK;
}

//  -----------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::endElement (
  const wchar_t * pwchNamespaceUri,
  int cchNamespaceUri,
  const wchar_t * pwchLocalName,
  int cchLocalName,
  const wchar_t * pwchQName,
  int cchQName)
{
  wchar_t szCurElement[MAX_PATH + 1] = {0};

#if (_MSC_VER >= 1400)
  wcsncpy_s(szCurElement, MAX_PATH + 1, pwchQName, cchQName);
#else
  wcsncpy(szCurElement, pwchQName, cchQName);
#endif

  if (m_bValidation && wcscmp(szCurElement, L"filters") == 0) {
    // Check that the XML file version is present and that
    // a. it is less than or equal to the Filter schema version
    // b. it is less than or equal to the version supported by this PWS
    if (m_iXMLVersion < 0) {
      LoadAString(m_strImportErrors, IDSC_MISSING_XML_VER);
      return E_FAIL;
    }
    if (m_iXMLVersion > m_iSchemaVersion) {
      Format(m_strImportErrors,
             IDSC_INVALID_XML_VER1, m_iXMLVersion, m_iSchemaVersion);
      return E_FAIL;
    }
    if (m_iXMLVersion > PWS_XML_FILTER_VERSION) {
      Format(m_strImportErrors,
             IDSC_INVALID_XML_VER2, m_iXMLVersion, PWS_XML_FILTER_VERSION);
      return E_FAIL;
    }
  }

  if (m_bValidation) {
    return S_OK;
  }

  if (wcscmp(szCurElement, L"filter") == 0) {
    INT_PTR rc = IDYES;
    st_Filterkey fk;
    fk.fpool = m_FPool;
    fk.cs_filtername = cur_filter->fname;
    if (m_MapFilters->find(fk) != m_MapFilters->end()) {
      std::wstring question;
      Format(question, IDSC_FILTEREXISTS, cur_filter->fname.c_str());
      if (m_pAsker == NULL || !(*m_pAsker)(question)) {
        m_MapFilters->erase(fk);
      }
    }
    if (rc == IDYES) {
      m_MapFilters->insert(PWSFilters::Pair(fk, *cur_filter));
    }
    delete cur_filter;
    return S_OK;
  }

  else if (wcscmp(szCurElement, L"filter_entry") == 0) {
    if (m_type == DFTYPE_MAIN) {
      cur_filter->num_Mactive++;
      cur_filter->vMfldata.push_back(*cur_filterentry);
    } else if (m_type == DFTYPE_PWHISTORY) {
      cur_filter->num_Hactive++;
      cur_filter->vHfldata.push_back(*cur_filterentry);
    } else if (m_type == DFTYPE_PWPOLICY) {
      cur_filter->num_Pactive++;
      cur_filter->vPfldata.push_back(*cur_filterentry);
    }
    delete cur_filterentry;
  }

  else if (wcscmp(szCurElement, L"grouptitle") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_STRING;
    cur_filterentry->ftype = FT_GROUPTITLE;
  }

  else if (wcscmp(szCurElement, L"group") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_STRING;
    cur_filterentry->ftype = FT_GROUP;
  }

  else if (wcscmp(szCurElement, L"title") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_STRING;
    cur_filterentry->ftype = FT_TITLE;
  }

  else if (wcscmp(szCurElement, L"user") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_STRING;
    cur_filterentry->ftype = FT_USER;
  }

  else if (wcscmp(szCurElement, L"password") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_PASSWORD;
    cur_filterentry->ftype = FT_PASSWORD;
  }

  else if (wcscmp(szCurElement, L"url") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_STRING;
    cur_filterentry->ftype = FT_URL;
  }

  else if (wcscmp(szCurElement, L"autotype") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_STRING;
    cur_filterentry->ftype = FT_AUTOTYPE;
  }

  else if (wcscmp(szCurElement, L"notes") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_STRING;
    cur_filterentry->ftype = FT_NOTES;
  }

  else if (wcscmp(szCurElement, L"create_time") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_DATE;
    cur_filterentry->ftype = FT_CTIME;
  }

  else if (wcscmp(szCurElement, L"password_modified_time") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_DATE;
    cur_filterentry->ftype = FT_PMTIME;
  }

  else if (wcscmp(szCurElement, L"last_access_time") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_DATE;
    cur_filterentry->ftype = FT_ATIME;
  }

  else if (wcscmp(szCurElement, L"expiry_time") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_DATE;
    cur_filterentry->ftype = FT_XTIME;
  }

  else if (wcscmp(szCurElement, L"record_modified_time") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_DATE;
    cur_filterentry->ftype = FT_RMTIME;
  }

  else if (wcscmp(szCurElement, L"password_expiry_interval") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_INTEGER;
    cur_filterentry->ftype = FT_XTIME_INT;
  }

  else if (wcscmp(szCurElement, L"entrytype") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_ENTRYTYPE;
    cur_filterentry->ftype = FT_ENTRYTYPE;
  }

  else if (wcscmp(szCurElement, L"unknownfields") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->ftype = FT_UNKNOWNFIELDS;
  }

  else if (wcscmp(szCurElement, L"password_history") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_PWHIST;
    cur_filterentry->ftype = FT_PWHIST;
  }

  else if (wcscmp(szCurElement, L"history_present") == 0) {
    m_type = DFTYPE_PWHISTORY;
    cur_filterentry->mtype = PWSMatch::MT_BOOL;
    cur_filterentry->ftype = HT_PRESENT;
  }

  else if (wcscmp(szCurElement, L"history_active") == 0) {
    m_type = DFTYPE_PWHISTORY;
    cur_filterentry->mtype = PWSMatch::MT_BOOL;
    cur_filterentry->ftype = HT_ACTIVE;
  }

  else if (wcscmp(szCurElement, L"history_number") == 0) {
    m_type = DFTYPE_PWHISTORY;
    cur_filterentry->mtype = PWSMatch::MT_INTEGER;
    cur_filterentry->ftype = HT_NUM;
  }

  else if (wcscmp(szCurElement, L"history_maximum") == 0) {
    m_type = DFTYPE_PWHISTORY;
    cur_filterentry->mtype = PWSMatch::MT_INTEGER;
    cur_filterentry->ftype = HT_MAX;
  }

  else if (wcscmp(szCurElement, L"history_changedate") == 0) {
    m_type = DFTYPE_PWHISTORY;
    cur_filterentry->mtype = PWSMatch::MT_DATE;
    cur_filterentry->ftype = HT_CHANGEDATE;
  }

  else if (wcscmp(szCurElement, L"history_passwords") == 0) {
    m_type = DFTYPE_PWHISTORY;
    cur_filterentry->mtype = PWSMatch::MT_PASSWORD;
    cur_filterentry->ftype = HT_PASSWORDS;
  }

  else if (wcscmp(szCurElement, L"password_policy") == 0) {
    m_type = DFTYPE_MAIN;
    cur_filterentry->mtype = PWSMatch::MT_POLICY;
    cur_filterentry->ftype = FT_POLICY;
  }

  else if (wcscmp(szCurElement, L"policy_present") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_BOOL;
    cur_filterentry->ftype = PT_PRESENT;
  }

  else if (wcscmp(szCurElement, L"policy_length") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_INTEGER;
    cur_filterentry->ftype = PT_LENGTH;
  }

  else if (wcscmp(szCurElement, L"policy_number_lowercase") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_INTEGER;
    cur_filterentry->ftype = PT_LOWERCASE;
  }

  else if (wcscmp(szCurElement, L"policy_number_uppercase") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_INTEGER;
    cur_filterentry->ftype = PT_UPPERCASE;
  }

  else if (wcscmp(szCurElement, L"policy_number_digits") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_INTEGER;
    cur_filterentry->ftype = PT_DIGITS;
  }

  else if (wcscmp(szCurElement, L"policy_number_symbols") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_INTEGER;
    cur_filterentry->ftype = PT_SYMBOLS;
  }

  else if (wcscmp(szCurElement, L"policy_easyvision") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_BOOL;
    cur_filterentry->ftype = PT_EASYVISION;
  }

  else if (wcscmp(szCurElement, L"policy_pronounceable") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_BOOL;
    cur_filterentry->ftype = PT_PRONOUNCEABLE;
  }

  else if (wcscmp(szCurElement, L"policy_hexadecimal") == 0) {
    m_type = DFTYPE_PWPOLICY;
    cur_filterentry->mtype = PWSMatch::MT_BOOL;
    cur_filterentry->ftype = PT_HEXADECIMAL;
  }

  else if (wcscmp(szCurElement, L"rule") == 0) {
    ToUpper(m_strElemContent);
    if (m_strElemContent == L"EQ")
      cur_filterentry->rule = PWSMatch::MR_EQUALS;
    else if (m_strElemContent == L"NE")
      cur_filterentry->rule = PWSMatch::MR_NOTEQUAL;
    else if (m_strElemContent == L"AC")
      cur_filterentry->rule = PWSMatch::MR_ACTIVE;
    else if (m_strElemContent == L"IA")
      cur_filterentry->rule = PWSMatch::MR_INACTIVE;
    else if (m_strElemContent == L"PR")
      cur_filterentry->rule = PWSMatch::MR_PRESENT;
    else if (m_strElemContent == L"NP")
      cur_filterentry->rule = PWSMatch::MR_NOTPRESENT;
    else if (m_strElemContent == L"SE")
      cur_filterentry->rule = PWSMatch::MR_SET;
    else if (m_strElemContent == L"NS")
      cur_filterentry->rule = PWSMatch::MR_NOTSET;
    else if (m_strElemContent == L"IS")
      cur_filterentry->rule = PWSMatch::MR_IS;
    else if (m_strElemContent == L"NI")
      cur_filterentry->rule = PWSMatch::MR_ISNOT;
    else if (m_strElemContent == L"BE")
      cur_filterentry->rule = PWSMatch::MR_BEGINS;
    else if (m_strElemContent == L"NB")
      cur_filterentry->rule = PWSMatch::MR_NOTBEGIN;
    else if (m_strElemContent == L"EN")
      cur_filterentry->rule = PWSMatch::MR_ENDS;
    else if (m_strElemContent == L"ND")
      cur_filterentry->rule = PWSMatch::MR_NOTEND;
    else if (m_strElemContent == L"CO")
      cur_filterentry->rule = PWSMatch::MR_CONTAINS;
    else if (m_strElemContent == L"NC")
      cur_filterentry->rule = PWSMatch::MR_NOTCONTAIN;
    else if (m_strElemContent == L"BT")
      cur_filterentry->rule = PWSMatch::MR_BETWEEN;
    else if (m_strElemContent == L"LT")
      cur_filterentry->rule = PWSMatch::MR_LT;
    else if (m_strElemContent == L"LE")
      cur_filterentry->rule = PWSMatch::MR_LE;
    else if (m_strElemContent == L"GT")
      cur_filterentry->rule = PWSMatch::MR_GT;
    else if (m_strElemContent == L"GE")
      cur_filterentry->rule = PWSMatch::MR_GE;
    else if (m_strElemContent == L"BF")
      cur_filterentry->rule = PWSMatch::MR_BEFORE;
    else if (m_strElemContent == L"AF")
      cur_filterentry->rule = PWSMatch::MR_AFTER;
    else if (m_strElemContent == L"EX")
      cur_filterentry->rule = PWSMatch::MR_EXPIRED;
    else if (m_strElemContent == L"WX")
      cur_filterentry->rule = PWSMatch::MR_WILLEXPIRE;
  }

  else if (wcscmp(szCurElement, L"logic") == 0) {
    if (m_strElemContent == L"or")
      cur_filterentry->ltype = LC_OR;
    else
      cur_filterentry->ltype = LC_AND;
  }

  else if (wcscmp(szCurElement, L"string") == 0) {
    cur_filterentry->fstring = m_strElemContent;
  }

  else if (wcscmp(szCurElement, L"case") == 0) {
    cur_filterentry->fcase = _wtoi(m_strElemContent.c_str()) != 0;
  }

  else if (wcscmp(szCurElement, L"warn") == 0) {
    cur_filterentry->fnum1 = _wtoi(m_strElemContent.c_str());
  }

  else if (wcscmp(szCurElement, L"num1") == 0) {
    cur_filterentry->fnum1 = _wtoi(m_strElemContent.c_str());
  }

  else if (wcscmp(szCurElement, L"num2") == 0) {
    cur_filterentry->fnum1 = _wtoi(m_strElemContent.c_str());
  }

  else if (wcscmp(szCurElement, L"date1") == 0) {
    time_t t(0);
    if (VerifyXMLDateString(m_strElemContent.c_str(), t) &&
        (t != (time_t)-1))
      cur_filterentry->fdate1 = t;
    else
    cur_filterentry->fdate1 = (time_t)0;
  }

  else if (wcscmp(szCurElement, L"date2") == 0) {
    time_t t(0);
    if (VerifyXMLDateString(m_strElemContent.c_str(), t) &&
        (t != (time_t)-1))
      cur_filterentry->fdate2 = t;
    else
      cur_filterentry->fdate1 = (time_t)0;
  }

  else if (wcscmp(szCurElement, L"type") == 0) {
    if (m_strElemContent == L"normal")
      cur_filterentry->etype = CItemData::ET_NORMAL;
    else if (m_strElemContent == L"alias")
      cur_filterentry->etype = CItemData::ET_ALIAS;
    else if (m_strElemContent == L"shortcut")
      cur_filterentry->etype = CItemData::ET_SHORTCUT;
    else if (m_strElemContent == L"aliasbase")
      cur_filterentry->etype = CItemData::ET_ALIASBASE;
    else if (m_strElemContent == L"shortcutbase")
      cur_filterentry->etype = CItemData::ET_SHORTCUTBASE;
  } else if (!(wcscmp(szCurElement, L"test") == 0 ||
               wcscmp(szCurElement, L"filters") == 0))
    ASSERT(0);

  return S_OK;
}

//  ---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::endDocument ( )
{
  return S_OK;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::startPrefixMapping (
  const wchar_t * pwchPrefix,
  int cchPrefix,
  const wchar_t * pwchUri,
  int cchUri )
{
  return S_OK;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::endPrefixMapping (
  const wchar_t * pwchPrefix,
  int cchPrefix )
{
  return S_OK;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::ignorableWhitespace (
  const wchar_t * pwchChars,
  int cchChars )
{
  return S_OK;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::processingInstruction (
  const wchar_t * pwchTarget,
  int cchTarget,
  const wchar_t * pwchData,
  int cchData )
{
  return S_OK;
}

HRESULT STDMETHODCALLTYPE MFilterSAX2ContentHandler::skippedEntity (
  const wchar_t * pwchName,
  int cchName )
{
  return S_OK;
}

#endif /* USE_XML_LIBRARY == MSXML */
