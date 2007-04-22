/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */
#pragma once

/// CompareResultsDlg.h
//-----------------------------------------------------------------------------

#include "afxwin.h"
#include "corelib/ItemData.h"
#include "corelib/MyString.h"

// The following structure needed for compare when record is in
// both databases but there are differences
struct st_CompareData {
  POSITION pos1;
  POSITION pos2;
  CItemData::FieldBits bsDiffs;
  CMyString group;
  CMyString title;
  CMyString user;
  int index;
  int listindex;
};

typedef std::vector<st_CompareData> CompareData;

// Column indices
enum {CURRENT = 0, COMPARE, GROUP, TITLE, USER, PASSWORD, NOTES, URL,
      AUTOTYPE, PWHIST, CTIME, ATIME, LTIME, PMTIME, RMTIME,
      LAST};

class CCompareResultsDlg : public CDialog
{
  DECLARE_DYNAMIC(CCompareResultsDlg)

  // Construction
public:
  CCompareResultsDlg(CWnd* pParent,
                     CompareData &OnlyInCurrent,
                     CompareData &OnlyInComp,
                     CompareData &Conflicts);

  // Dialog Data
  //{{AFX_DATA(CCompareResultsDlg)
  enum { IDD = IDD_COMPARE_RESULTS };
  CListCtrl m_LCResults;
  int m_iSortedColumn;
  bool m_bSortAscending;
  CMyString m_cs_Filename1, m_cs_Filename2;

  //}}AFX_DATA

  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CCompareResultsDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

private:
  static int CALLBACK CRCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

  // Implementation
protected:

  UINT statustext[1];
  CStatusBar m_statusBar;

  virtual BOOL OnInitDialog();
  // Generated message map functions
  //{{AFX_MSG(CCompareResultsDlg)
  virtual void OnCancel();
  virtual void OnOK();
  afx_msg void OnHelp();
  afx_msg void OnCopyToClipboard();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
  afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnItemDoubleClick(NMHDR* pNotifyStruct, LRESULT* result);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
	CompareData m_OnlyInCurrent;
	CompareData m_OnlyInComp;
	CompareData m_Conflicts;

  int m_cxBSpace, m_cyBSpace, m_cySBar;
  int m_DialogMinWidth, m_DialogMinHeight;
};
//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:

