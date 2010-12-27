/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#pragma once

// ConfirmDeleteDlg.h
//-----------------------------------------------------------------------------
#include "core/PwsPlatform.h"

#include "PWDialog.h"
class CConfirmDeleteDlg : public CPWDialog
{
public:
  CConfirmDeleteDlg(CWnd* pParent = NULL, const size_t numchildren = 0, size_t numatts = 0,
      const StringX sxGroup = L"", const StringX sxTitle = L"",
      const StringX sxUser = L"");

private:
  // Dialog Data
  //{{AFX_DATA(CConfirmDeleteDlg)
  enum { IDDENT = IDD_CONFIRMDELETE_DIALOGENT,
         IDDGRP = IDD_CONFIRMDELETE_DIALOGGRP};
  bool m_dontaskquestion;
  size_t m_numchildren, m_numatts;
  //}}AFX_DATA

  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CConfirmDeleteDlg)

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

  // Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CConfirmDeleteDlg)
  virtual BOOL OnInitDialog();
  virtual void OnCancel();
  virtual void OnOK();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

private:
  StringX m_sxGroup, m_sxTitle, m_sxUser;
};

//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
