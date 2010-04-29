/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#pragma once

// ExportText.h : header file
//

#include <bitset>

/////////////////////////////////////////////////////////////////////////////
// CExportText dialog

#include "SecString.h"
#include "corelib/ItemData.h"
#include "ControlExtns.h"
#include "PWDialog.h"

class CVKeyBoardDlg;

class CExportTextDlg : public CPWDialog
{
  // Construction
public:
  CExportTextDlg(CWnd* pParent = NULL, bool bAll = true);   // standard constructor
  ~CExportTextDlg();

  const CSecString &GetPasskey() const {return m_passkey;}

  // Dialog Data
  //{{AFX_DATA(CExportTextDlg)
  enum { IDD = IDD_EXPORT_TEXT };
  CString m_defexpdelim;
  //}}AFX_DATA

  CItemData::FieldBits m_bsExport;
  CString m_subgroup_name;
  int m_subgroup_set, m_subgroup_object, m_subgroup_function;

  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CExportTextDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

  // Implementation
protected:
  virtual BOOL OnInitDialog();
  // Generated message map functions
  //{{AFX_MSG(CExportTextDlg)
  afx_msg void OnAdvanced();
  afx_msg void OnHelp();
  virtual void OnOK();
  afx_msg void OnVirtualKeyboard();
  afx_msg LRESULT OnInsertBuffer(WPARAM, LPARAM);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

private:
  void AFXAPI DDV_CheckExpDelimiter(CDataExchange* pDX,
                                    const CString &delimiter);
  CSecEditExtn *m_pctlPasskey;
  CSecString m_passkey;
  CVKeyBoardDlg *m_pVKeyBoardDlg;
  bool m_bAll;
};