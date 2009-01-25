/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef __XMLUTILITIES_H
#define __XMLUTILITIES_H

#include "../XMLDefs.h"

#ifdef USE_XML_LIBRARY

#if USE_XML_LIBRARY == MSXML
extern wchar_t * ProcessAttributes(
                     /* [in]  */ ISAXAttributes __RPC_FAR *pAttributes,
                     /* [in]  */ wchar_t *lpName,
                     /* [out] */ bool &berror);
#endif /* USE_XML_LIBRARY == MSXML */

#endif /* USE_XML_LIBRARY */

#endif /* __XMLUTILITIES_H */
