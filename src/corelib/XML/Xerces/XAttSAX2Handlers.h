/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/*
* This routine processes Attachment XML using the STANDARD and UNMODIFIED
* Xerces library V3.1.1 released on April 27, 2010
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

#ifndef __XATTSAX2HANDLERS_H
#define __XATTSAX2HANDLERS_H

#include "../XMLAttValidation.h"
#include "../XMLAttHandlers.h"

#include "XAttValidator.h"

// PWS includes
#include "../../StringX.h"
#include "../../ItemData.h"
#include "../../UUIDGen.h"
#include "../../PWScore.h"

// Xerces includes
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>


XERCES_CPP_NAMESPACE_USE

class XAttSAX2Handlers : public DefaultHandler, public XMLAttHandlers
{
public:
  XAttSAX2Handlers();
  virtual ~XAttSAX2Handlers();

  // -----------------------------------------------------------------------
  //  Handlers for the SAX ContentHandler interface
  // -----------------------------------------------------------------------
  void startElement(const XMLCh* const uri, const XMLCh* const localname,
                    const XMLCh* const qname, const Attributes& attrs);
  void characters(const XMLCh* const chars, const XMLSize_t length);
  void ignorableWhitespace(const XMLCh* const chars, const XMLSize_t length);
  void endElement(const XMLCh* const uri,
                  const XMLCh* const localname,
                  const XMLCh* const qname);
  void setDocumentLocator(const Locator *const locator) {m_pLocator = locator;}
  void startDocument();

  // -----------------------------------------------------------------------
  //  Handlers for the SAX ErrorHandler interface
  // -----------------------------------------------------------------------
  void warning(const SAXParseException& exc);
  void error(const SAXParseException& exc);
  void fatalError(const SAXParseException& exc);

  stringT getValidationResult() {return m_strValidationResult;}

private:
  void FormatError(const SAXParseException& e, const int type);

  // Local variables
  XAttValidator *m_pValidator;

  const Locator *m_pLocator;

  stringT m_strValidationResult;
  bool m_bErrorsFound;
};

#endif /*__XATTSAX2HANDLERS_H */
