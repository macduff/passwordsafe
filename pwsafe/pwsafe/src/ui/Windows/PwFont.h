/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#pragma once

// PwFont.h
//-----------------------------------------------------------------------------

void GetPasswordFont(LOGFONT *plogfont);
void SetPasswordFont(LOGFONT *plogfont);
void ApplyPasswordFont(CWnd* pDlgItem);
void GetDefaultPasswordFont(LOGFONT &lf);
void DeletePasswordFont();
void ExtractFont(const CString& str, LOGFONT &logfont);

//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
