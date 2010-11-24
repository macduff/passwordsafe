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

#include "../XMLDefs.h"    // Required if testing "USE_XML_LIBRARY"

#if USE_XML_LIBRARY == XERCES

// PWS includes
#include "XAttXMLProcessor.h"
#include "XAttSAX2Handlers.h"
#include "XSecMemMgr.h"

#include "../../ItemData.h"
#include "../../core.h"
#include "../../PWScore.h"
#include "../../UnknownField.h"
#include "../../PWSprefs.h"

#include <sys/types.h>
#include <sys/stat.h>

// Xerces includes
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/framework/XMLGrammarDescription.hpp>

#if defined(XERCES_NEW_IOSTREAMS)
#include <fstream>
#else
#include <fstream.h>
#endif

#include "./XMLChConverter.h"

XAttXMLProcessor::XAttXMLProcessor(PWScore *pcore,
                                   MultiCommands *p_multicmds,
                                   CReport *prpt)
  : m_pXMLcore(pcore), 
    m_pmulticmds(p_multicmds), m_prpt(prpt)
{
}

XAttXMLProcessor::~XAttXMLProcessor()
{
}

// ---------------------------------------------------------------------------
bool XAttXMLProcessor::Process(const bool &bvalidation,
                               const stringT &strXMLFileName, const stringT &strXSDFileName,
                               PWSAttfile *pimport)
{
  USES_XMLCH_STR
  
  bool bErrorOccurred = false;
  stringT cs_validation;
  LoadAString(cs_validation, IDSC_XMLVALIDATION);
  stringT cs_import;
  LoadAString(cs_import, IDSC_XMLIMPORT);
  stringT strResultText(_T(""));
  m_bValidation = bvalidation;  // Validate or Import
  m_pimport = pimport;

  XSecMemMgr sec_mm;

  // Initialize the XML4C2 system
  try
  {
    XMLPlatformUtils::Initialize(XMLUni::fgXercescDefaultLocale, 0, 0, &sec_mm);
  }
  catch (const XMLException& toCatch)
  {
#ifdef UNICODE
    strResultText = stringT(_X2ST(toCatch.getMessage()));
#else
    char *szData = XMLString::transcode(toCatch.getMessage());
    strResultText = stringT(szData);
    XMLString::release(&szData);
#endif
    return false;
  }

#ifdef UNICODE
  const XMLCh* xmlfilename = _W2X(strXMLFileName.c_str());
  const XMLCh* schemafilename = _W2X(strXSDFileName.c_str());
#else
  const XMLCh* xmlfilename = XMLString::transcode(strXMLFileName.c_str());
  const XMLCh* schemafilename = XMLString::transcode(strXSDFileName.c_str());
#endif

  //  Create a SAX2 parser object.
  SAX2XMLReader* pSAX2Parser = XMLReaderFactory::createXMLReader(&sec_mm);

  // Set non-default features
  pSAX2Parser->setFeature(XMLUni::fgSAX2CoreNameSpacePrefixes, true);
  pSAX2Parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
  pSAX2Parser->setFeature(XMLUni::fgXercesDynamic, false);
  pSAX2Parser->setFeature(XMLUni::fgXercesSchemaFullChecking, true);
  pSAX2Parser->setFeature(XMLUni::fgXercesLoadExternalDTD, false);
  pSAX2Parser->setFeature(XMLUni::fgXercesSkipDTDValidation, true);
 
  // Set properties
  pSAX2Parser->setProperty(XMLUni::fgXercesScannerName,
                          (void *)XMLUni::fgSGXMLScanner);
  pSAX2Parser->setInputBufferSize(4096);

  // Set schema file name (also via property)
  pSAX2Parser->setProperty(XMLUni::fgXercesSchemaExternalNoNameSpaceSchemaLocation,
                           (void *)schemafilename);

  // Create SAX handler object and install it on the pSAX2Parser, as the
  // document and error pSAX2Handler.
  XAttSAX2Handlers * pSAX2Handler = new XAttSAX2Handlers;
  pSAX2Parser->setContentHandler(pSAX2Handler);
  pSAX2Parser->setErrorHandler(pSAX2Handler);

  pSAX2Handler->SetVariables(m_bValidation ? NULL : m_pXMLcore, m_bValidation, 
                             m_pmulticmds, m_pimport, m_prpt);

  try {
    // Let's begin the parsing now
    pSAX2Parser->parse(xmlfilename);
  }
  catch (const OutOfMemoryException&) {
    LoadAString(strResultText, IDCS_XERCESOUTOFMEMORY);
    bErrorOccurred = true;
  }
  catch (const XMLException& e) {
#ifdef UNICODE
    strResultText = stringT(_X2ST(e.getMessage()));
#else
    char *szData = XMLString::transcode(e.getMessage());
    strResultText = stringT(szData);
    XMLString::release(&szData);
#endif
    bErrorOccurred = true;
  }

  catch (...) {
    LoadAString(strResultText, IDCS_XERCESEXCEPTION);
    bErrorOccurred = true;
  }

  if (pSAX2Handler->getIfErrors() || bErrorOccurred) {
    bErrorOccurred = true;
    strResultText = pSAX2Handler->getValidationResult();
    Format(m_strXMLErrors, IDSC_XERCESPARSEERROR, 
           m_bValidation ? cs_validation.c_str() : cs_import.c_str(), 
           strResultText.c_str());
  } else {
    if (m_bValidation) {
      m_strXMLErrors = pSAX2Handler->getValidationResult();
      m_numEntriesValidated = pSAX2Handler->getNumEntries();
    } else {
      // Get numbers (may have been modified by AddEntries
      m_numAttachmentsImported = pSAX2Handler->m_numAttachments;
      m_numAttachmentsSkipped = pSAX2Handler->getNumSkipped();

      // Get lists
      m_strXMLErrors = pSAX2Handler->getXMLErrors();
      m_strSkippedList = pSAX2Handler->getSkippedList();
    }
  }

#ifndef UNICODE
  XMLString::release((XMLCh **)&xmlfilename);
  XMLString::release((XMLCh **)&schemafilename);
#endif

  //  Delete the pSAX2Parser itself.  Must be done prior to calling Terminate, below.
  delete pSAX2Parser;
  delete pSAX2Handler;

  USES_XMLCH_STR_END
  
  // And call the termination method
  XMLPlatformUtils::Terminate();

  return !bErrorOccurred;
}

#endif /* USE_XML_LIBRARY == XERCES */
