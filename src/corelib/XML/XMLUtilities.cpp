/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifdef USE_XML_LIBRARY

#include "XMLUtilities.h"
#include "../Util.h"

#if USE_XML_LIBRARY == MSXML
wchar_t * ProcessAttributes(
                  /* [in]  */ ISAXAttributes __RPC_FAR *pAttributes,
                  /* [in]  */ const wchar_t *lpName,
                  /* [out] */ bool &berror)
{
  // Note 1: Caller needs to trash and delete the value returned.
  // Note 2: This ONLY processes the attributes to find ONE value.
  // Needs to be enhanced if we ever need more (which we do not currently)
  int iAttribs(0);
  berror = false;
  pAttributes->getLength(&iAttribs);
  for (int i = 0; i < iAttribs; i++) {
    const wchar_t *QName, *Value;
    int QName_length, Value_length;

    pAttributes->getQName(i, &QName, &QName_length);

    wchar_t szQName = new wchar_t[QName_length + 1];
#if (_MSC_VER >= 1400)
    wcsncpy_s(szQName, QName_length + 1, QName, QName_length);
#else
    wcsncpy(szQName, QName, QName_length);
#endif  // _MSC_VER >= 1400
    szQName[QName_length] = L'\0';

    pAttributes->getValue(i, &Value, &Value_length);
    wchar_t szValue = new wchar_t[Value_length + 1];
#if (_MSC_VER >= 1400)
    wcsncpy_s(szValue, Value_length + 1, Value, Value_length);
#else
    wcsncpy(szValue, Value, Value_length);
#endif  // _MSC_VER >= 1400
    szValue[Value_length] = L'\0';

    if (wcscmp(szQName, lpName) == 0) {
      delete [] szQName;
      return szValue;
    }
  }

  delete [] szQName;
  delete [] szValue;
  return NULL;
}
#endif /* USE_XML_LIBRARY == MSXML */

#endif /* USE_XML_LIBRARY */
