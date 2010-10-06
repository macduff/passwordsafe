/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/*
* This routine processes Attachment XML using the STANDARD and UNMODIFIED
* Expat library V2.0.1 released on June 5, 2007
*
* See http://expat.sourceforge.net/
*
* Note: This is a cross-platform library and can be linked in as a
* Static library or used as a dynamic library e.g. DLL in Windows.
*
* NOTE: EXPAT is a NON-validating XML Parser.  All conformity with the
* scheam must be performed in the handlers.  Also, the concept of pre-validation
* before importing is not available.
* As per XML parsing rules, any error stops the parsing immediately.
*/

#ifndef __EATTHANDLERS_H
#define __EATTHANDLERS_H

// PWS includes
#include "../XMLAttValidation.h"
#include "../XMLAttHandlers.h"

#include "EAttValidator.h"

#include <stack>

// Expat includes
#include <expat.h>

class EAttHandlers : public XMLFileHandlers
{
public:
  EAttHandlers();
  virtual ~EAttHandlers();

  // -----------------------------------------------------------------------
  //  Handlers for the ContentHandler interface
  // -----------------------------------------------------------------------
  void XMLCALL startElement(void *userdata, const XML_Char *name,
                            const XML_Char **attrs);
  void XMLCALL endElement(void *userdata, const XML_Char *name);
  void XMLCALL characterData(void *userdata, const XML_Char *s,
                             int length);

private:
  // Local variables
  EAttValidator *m_pValidator;

  std::stack<int> m_element_datatype;
};

#endif /* __EATTHANDLERS_H */
