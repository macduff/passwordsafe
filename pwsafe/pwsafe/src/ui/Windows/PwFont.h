/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
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

struct PWFonts {
  PWFonts() : m_pCurrentFont(NULL), m_pModifiedFont(NULL) {}
  ~PWFonts() {delete m_pModifiedFont;}
  void SetUpFont(CWnd *pWnd, CFont *pfont);
  CFont *m_pCurrentFont;  // Do NOT delete - done in DboxMain
  CFont *m_pModifiedFont;
  static const COLORREF MODIFIED_COLOR;
};

//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
