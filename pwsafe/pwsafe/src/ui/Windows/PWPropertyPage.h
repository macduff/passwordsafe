/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

/*
* PasswordSafe-specific base class for property pages
*
* All property pages shown in the Options Property sheet
* should be derived from this class.
* The sole raisone d'etre for this class is the virtual
* abstract GetHelpName() member function,
* which is used to ensure that the correct help page
* is displayed by ThisMfcApp::OnHelp()
* Truly a workaround for M'soft dain bramage.
*/

#pragma once

class CPWPropertyPage : public CPropertyPage
{
public:
  CPWPropertyPage(UINT nID) : CPropertyPage(nID) {}
  virtual ~CPWPropertyPage() {}
  virtual const wchar_t *GetHelpName() const = 0;

  // Following override to reset idle timeout on any event
  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
 
  enum {PP_DATA_CHANGED = 0, PP_UPDATE_VARIABLES};

  DECLARE_DYNAMIC(CPWPropertyPage)
};
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
