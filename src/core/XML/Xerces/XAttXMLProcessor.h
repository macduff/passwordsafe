/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
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

#ifndef __XATTXMLPROCESSOR_H
#define __XATTXMLPROCESSOR_H

// PWS includes
#include "XAttSAX2Handlers.h"

#include "../../UUIDGen.h"
#include "../../Command.h"

#include "os/typedefs.h"

#include <stdlib.h>
#include <string.h>
#include <vector>

// Xerces includes
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>

#if defined(XERCES_NEW_IOSTREAMS)
#include <iostream>
#else
#include <iostream.h>
#endif

class PWScore;
class PWSAttfile;

class XAttXMLProcessor
{
public:
  XAttXMLProcessor(PWScore *pcore, MultiCommands *p_multicmds,
                   CReport *prpt);
  ~XAttXMLProcessor();

  bool Process(const bool &bvalidation,
               const stringT &strXMLFileName, const stringT &strXSDFileName,
               PWSAttfile *pimport);

  stringT getXMLErrors() {return m_strXMLErrors;}
  stringT getSkippedList() {return m_strSkippedList;}

  int getNumAttachmentsValidated() {return m_numAttachmentsValidated;}
  int getNumAttachmentsImported() {return m_numAttachmentsImported;}
  int getNumAttachmentsSkipped() {return m_numAttachmentsSkipped;}

private:
  PWScore *m_pXMLcore;
  MultiCommands *m_pmulticmds;
  CReport *m_prpt;
  PWSAttfile *m_pimport;

  stringT m_strXMLErrors, m_strSkippedList;
  int m_numAttachmentsValidated, m_numAttachmentsImported, m_numAttachmentsSkipped;
  bool m_bValidation;
};

#endif /* __XATTXMLPROCESSOR_H */
