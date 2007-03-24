#pragma once

#include "resource.h"
#include "ColumnChooserLC.h"

// CColumnChooserDlg dialog

class CColumnChooserDlg : public CDialog
{
  DECLARE_DYNAMIC(CColumnChooserDlg)

private:
    //using CDialog::Create

public:
  CColumnChooserDlg(CWnd* pParent = NULL);   // standard constructor
  virtual ~CColumnChooserDlg();
  BOOL Create(UINT nID, CWnd *parent);

  // Dialog Data
  //{{AFX_DATA(CColumnChooserDlg)
  enum { IDD = IDD_COLUMNCHOOSER };
  CColumnChooserLC m_ccListCtrl;
  //}}AFX_DATA

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual void PostNcDestroy();
  BOOL OnInitDialog();

  //{{AFX_DATA(CColumnChooserDlg)
  afx_msg void OnDestroy();
  //}}AFX_DATA

  DECLARE_MESSAGE_MAP()

public:
};
