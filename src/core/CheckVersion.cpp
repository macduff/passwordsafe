/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/// \file CheckVersion.cpp
//-----------------------------------------------------------------------------

/*
 * The latest version information is in
 * http://pwsafe.org/latest.xml
 *
 * And is of the form:
 * <VersionInfo>
 *  <Product name=PasswordSafe variant=PC major=3 minor=10 build=2 rev=1710 />
 *  <Product name=PasswordSafe variant=PPc major=1 minor=9 build=2
 *    rev=100 />
 *  <Product name=PasswordSafe variant=U3 major=3 minor=10 build=2
 *    rev=1710 />
 *  <Product name=PasswordSafe variant=Linux major=3 minor=10 build=2
 *    rev=1710 />
 * </VersionInfo>
 *
 * Note: The "rev" is the svn commit number. Not using it (for now),
 *       as I think it's too volatile.
 */

#include "CheckVersion.h"
#include "tinyxml/tinyxml.h"
#include "SysInfo.h"
#include "StringX.h" // for Format()

static bool SafeCompare(const TCHAR *v1, const TCHAR *v2)
{
  return (v1 != NULL && v2 != NULL && stringT(v1) == v2);
}

CheckVersion::CheckStatus
CheckVersion::CheckLatestVersion(const stringT &xml, stringT &latest) const
{
  // Parse the file we just retrieved
  TiXmlDocument doc; 
  if (doc.Parse(xml.c_str()) == NULL)
    return CANT_READ;
  TiXmlNode *pRoot = doc.FirstChildElement();

  if (!pRoot || !SafeCompare(pRoot->Value(), _T("VersionInfo")))
    return CANT_READ;

  TiXmlNode *pProduct = 0;
  while((pProduct = pRoot->IterateChildren(pProduct)) != NULL) {
    if (SafeCompare(pProduct->Value(), _T("Product"))) {
      TiXmlElement *pElem = pProduct->ToElement();
      if (pElem == NULL)
        return CANT_READ;
      const TCHAR *prodName = pElem->Attribute(_T("name"));
      if (SafeCompare(prodName, _T("PasswordSafe"))) {
        const TCHAR *pVariant = pElem->Attribute(_T("variant"));
        if (pVariant == NULL) continue;
        const stringT variant(pVariant);
        // Determine which variant is relevant for us
        if ((SysInfo::IsUnderU3() && variant == _T("U3")) ||
            (SysInfo::IsLinux() && variant == _T("Linux")) ||
            (!SysInfo::IsUnderU3() && !SysInfo::IsLinux() &&
             variant == _T("PC"))) {
            int xmajor(0), xminor(0), xbuild(0), xrevision(0);
            pElem->QueryIntAttribute(_T("major"), &xmajor);
            pElem->QueryIntAttribute(_T("minor"), &xminor);
            pElem->QueryIntAttribute(_T("build"), &xbuild);
            pElem->QueryIntAttribute(_T("rev"), &xrevision);
            // Not using svn rev info - too volatile
            if ((xmajor > m_nMajor) ||
              (xmajor == m_nMajor && xminor > m_nMinor) ||
              (xmajor == m_nMajor && xminor == m_nMinor &&
              xbuild > m_nBuild)
              ) {
                if (xbuild == 0) { // hide build # if zero (formal release)
                  Format(latest, _T("PasswordSafe V%d.%02d (%d)"),
                         xmajor, xminor, xrevision);
                } else {
                  Format(latest, _T("PasswordSafe V%d.%02d.%02d (%d)"),
                         xmajor, xminor, xbuild, xrevision);
                }
                return NEWER_AVAILABLE;
            }
            return UP2DATE;
        } // handled our variant
      } // Product name == PasswordSafe
    } // Product element
  } // IterateChildren
  return CANT_READ;
}
