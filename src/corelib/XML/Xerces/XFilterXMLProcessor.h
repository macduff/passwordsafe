/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/*
* This routine processes Filter XML using the STANDARD and UNMODIFIED
* Xerces library V3.0.0 released on September 29, 2008
*
* See http://xerces.apache.org/xerces-c/
*
* Note: This is a cross-platform library and can be linked in as a
* Static library or used as a dynamic library e.g. DLL in Windows.
* To use the static version, the following pre-processor statement
* must be defined: XERCES_STATIC_LIBRARY
*
*/

/*
* NOTE: Xerces characters are ALWAYS in UTF-16 (may or may not be wchar_t 
* depending on platform).
* Non-unicode builds will need convert any results from parsing the XML
* document from UTF-16 to ASCII.
*/

#ifndef __XFILTERXMLPROCESSOR_H
#define __XFILTERXMLPROCESSOR_H

#include "XFilterSAX2Handlers.h"

// PWS includes
#include "../../StringX.h"
#include "../../PWSFilters.h"
#include "../../Proxy.h"

#include <stdlib.h>
#include <string>

// Xerces includes
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#if defined(XERCES_NEW_IOSTREAMS)
#include <iostream>
#else
#include <iostream.h>
#endif

class XFilterXMLProcessor
{
public:
  XFilterXMLProcessor(PWSFilters &mapfilters, const FilterPool fpool, Asker *pAsker);
  ~XFilterXMLProcessor();

  bool Process(const bool &bvalidation,
               const StringX &strXMLData,
               const std::wstring &strXMLFileName, 
               const std::wstring &strXSDFileName);

  std::wstring getResultText() {return m_strResultText;}

private:
  Asker *m_pAsker;
  PWSFilters &m_MapFilters;
  FilterPool m_FPool;
  std::wstring m_strResultText;
  bool m_bValidation;
};

#endif /* __XFILTERXMLPROCESSOR_H */
