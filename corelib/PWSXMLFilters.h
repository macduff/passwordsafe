/*
* Copyright (c) 2003-2008 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// PWSXMLFilters.h : header file
//

#ifndef __PWSXMLFILTERS_H
#define __PWSXMLFILTERS_H

#include <vector>
#include "PWSFilters.h"

class PWSXMLFilters
{
public:
  PWSXMLFilters(PWSFilters &mapfilters, const FilterPool fpool);
  ~PWSXMLFilters();

  bool XMLFilterProcess(const bool &bvalidation,
                        const stringT &strXMLData,
                        const stringT &strXMLFileName, 
                        const stringT &strXSDFileName);

  stringT m_strResultText;
  int m_MSXML_Version;

private:
  bool m_bValidation;
  PWSFilters &m_MapFilters;
  FilterPool m_FPool;
};

#endif /* __PWSXMLFILTERS_H */
