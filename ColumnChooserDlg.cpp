// ColumnChooserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ColumnChooserDlg.h"

// CColumnChooserDlg dialog

IMPLEMENT_DYNAMIC(CColumnChooserDlg, CDialog)

CColumnChooserDlg::CColumnChooserDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CColumnChooserDlg::IDD, pParent)
{
}

CColumnChooserDlg::~CColumnChooserDlg()
{
}

void CColumnChooserDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_CPLIST, m_ccListCtrl);
}

BEGIN_MESSAGE_MAP(CColumnChooserDlg, CDialog)
  //{{AFX_MSG_MAP(CColumnChooserDlg)
  ON_WM_DESTROY()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

// CColumnChooserDlg message handlers

BOOL CColumnChooserDlg::Create(UINT nID, CWnd *parent)
{
  return CDialog::Create(nID, parent);
}

BOOL CColumnChooserDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Initialise DropTarget
  m_ccListCtrl.Initialize(&m_ccListCtrl);

  return TRUE;
}

void CColumnChooserDlg::PostNcDestroy()
{
  delete this;
}

void CColumnChooserDlg::OnDestroy()
{
  // Delete all items
  m_ccListCtrl.DeleteAllItems();

  // Stop Drag & Drop OLE
  m_ccListCtrl.Terminate();
}
