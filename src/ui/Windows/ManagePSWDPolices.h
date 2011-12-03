/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#pragma once

#include "PWDialog.h"

#include "core/StringX.h"
#include "core/coredefs.h"
#include "resource.h"

// CManagePSWDPolices dialog

class DboxMain;

class CManagePSWDPolices : public CPWDialog
{
public:
  CManagePSWDPolices(CWnd* pParent = NULL);
  virtual ~CManagePSWDPolices();

  // Dialog Data
  enum { IDD = IDD_MANAGEPASSWORDPOLICIES };
  
  PSWDPolicyMap &GetPasswordPolicies() {return m_MapPSWDPLC;}

  bool IsChanged() {return m_bChanged;}

protected:
  CListCtrl m_PolicyNames;
  CListCtrl m_PolicyDetails;
  CListCtrl m_PolicyEntries;

  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual BOOL OnInitDialog();
  virtual BOOL PreTranslateMessage(MSG* pMsg);

  afx_msg void OnHelp();
  afx_msg void OnCancel();
  afx_msg void OnNew();
  afx_msg void OnEdit();
  afx_msg void OnList();
  afx_msg void OnDelete();
  afx_msg void OnPolicySelected(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnEntryDoubleClicked(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnColumnNameClick(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnColumnEntryClick(NMHDR *pNotifyStruct, LRESULT *pLResult);

  DECLARE_MESSAGE_MAP()

private:
  void UpdateNames();
  void UpdateDetails();
  void UpdateEntryList();
  static int CALLBACK SortNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK SortEntries(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  
  DboxMain *m_pDbx;
  CToolTipCtrl *m_pToolTipCtrl;

  PSWDPolicyMap m_MapPSWDPLC;
  st_PSWDPolicy m_default_st_pp;

  GTUSet m_setGTU;

  int m_iSortNamesIndex, m_iSortEntriesIndex;
  bool m_bSortNamesAscending, m_bSortEntriesAscending;

  int m_iSelectedItem;
  bool m_bChanged, m_bViewPolicy;
};
