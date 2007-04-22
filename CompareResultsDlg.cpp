/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */

/// CompareResultsDlg.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "ThisMfcApp.h"
#include "DboxMain.h"  // For WM_VIEW_COMPARE_RESULT
#include "CompareResultsDlg.h"
#include "corelib/PWScore.h"
#include "resource3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CCompareResultsDlg, CDialog)

//-----------------------------------------------------------------------------
CCompareResultsDlg::CCompareResultsDlg(CWnd* pParent,
  CompareData &OnlyInCurrent, CompareData &OnlyInComp, CompareData &Conflicts)
  : CDialog(CCompareResultsDlg::IDD, pParent),
  m_OnlyInCurrent(OnlyInCurrent), m_OnlyInComp(OnlyInComp), m_Conflicts(Conflicts),
  m_bSortAscending(true), m_iSortedColumn(-1)
{
}

BOOL CCompareResultsDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  m_LCResults.GetHeaderCtrl()->SetDlgCtrlID(IDC_RESULTLISTHDR);

  DWORD dwExtendedStyle = m_LCResults.GetExtendedStyle();
  dwExtendedStyle |= LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT;
  m_LCResults.SetExtendedStyle(dwExtendedStyle);

  CString cs_header;
  cs_header.LoadString(IDS_ORIGINALDB);
  m_LCResults.InsertColumn(CURRENT, cs_header);
  cs_header.LoadString(IDS_COMPARISONDB);
  m_LCResults.InsertColumn(COMPARE, cs_header);
  cs_header.LoadString(IDS_GROUP);
  m_LCResults.InsertColumn(GROUP, cs_header);
  cs_header.LoadString(IDS_TITLE);
  m_LCResults.InsertColumn(TITLE, cs_header);
  cs_header.LoadString(IDS_USERNAME);
  m_LCResults.InsertColumn(USER, cs_header);
  cs_header.LoadString(IDS_PASSWORD);
  m_LCResults.InsertColumn(PASSWORD, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_NOTES);
  m_LCResults.InsertColumn(NOTES, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_URL);
  m_LCResults.InsertColumn(URL, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_AUTOTYPE);
  m_LCResults.InsertColumn(AUTOTYPE, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_PWHIST );
  m_LCResults.InsertColumn(PWHIST, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_CREATED);
  m_LCResults.InsertColumn(CTIME, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_LASTACCESSED);
  m_LCResults.InsertColumn(ATIME, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_PASSWORDEXPIRYDATE);
  m_LCResults.InsertColumn(LTIME, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_PASSWORDMODIFIED);
  m_LCResults.InsertColumn(PMTIME, cs_header, LVCFMT_CENTER);
  cs_header.LoadString(IDS_LASTMODIFIED);
  m_LCResults.InsertColumn(RMTIME, cs_header, LVCFMT_CENTER);

  m_LCResults.SetItemCount(m_OnlyInCurrent.size() +
                           m_OnlyInComp.size() +
                           m_Conflicts.size());

  int i, iItem = 0;
  CompareData::iterator cd_iter;

	if (m_OnlyInCurrent.size() > 0) {
    for (cd_iter = m_OnlyInCurrent.begin(); cd_iter != m_OnlyInCurrent.end(); cd_iter++) {
      st_CompareData &st_data = *cd_iter;

      m_LCResults.InsertItem(iItem, _T("Y"));
      m_LCResults.SetItemText(iItem, COMPARE, _T("-"));
      m_LCResults.SetItemText(iItem, GROUP, st_data.group);
      m_LCResults.SetItemText(iItem, TITLE, st_data.title);
      m_LCResults.SetItemText(iItem, USER, st_data.user);
      for (i = USER + 1; i < LAST; i++)
        m_LCResults.SetItemText(iItem, i, _T("-"));

      st_data.listindex = iItem;
      m_LCResults.SetItemData(iItem, (DWORD)&st_data);
      iItem++;
		}
	}

	if (m_OnlyInComp.size() > 0) {
    for (cd_iter = m_OnlyInComp.begin(); cd_iter != m_OnlyInComp.end(); cd_iter++) {
      st_CompareData &st_data = *cd_iter;

      m_LCResults.InsertItem(iItem, _T("-"));
      m_LCResults.SetItemText(iItem, COMPARE, _T("Y"));
      m_LCResults.SetItemText(iItem, GROUP, st_data.group);
      m_LCResults.SetItemText(iItem, TITLE, st_data.title);
      m_LCResults.SetItemText(iItem, USER, st_data.user);
      for (i = USER + 1; i < LAST; i++)
        m_LCResults.SetItemText(iItem, i, _T("-"));

      st_data.listindex = iItem;
      m_LCResults.SetItemData(iItem, (DWORD)&st_data);
      iItem++;
		}
	}

	if (m_Conflicts.size() > 0) {
    for (cd_iter = m_Conflicts.begin(); cd_iter != m_Conflicts.end(); cd_iter++) {
      st_CompareData &st_data = *cd_iter;

      m_LCResults.InsertItem(iItem, _T("Y"));
      m_LCResults.SetItemText(iItem, COMPARE, _T("Y"));
      m_LCResults.SetItemText(iItem, GROUP, st_data.group);
      m_LCResults.SetItemText(iItem, TITLE, st_data.title);
      m_LCResults.SetItemText(iItem, USER, st_data.user);
  	  m_LCResults.SetItemText(iItem, PASSWORD, st_data.bsDiffs.test(CItemData::PASSWORD) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, NOTES, st_data.bsDiffs.test(CItemData::NOTES) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, URL, st_data.bsDiffs.test(CItemData::URL) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, AUTOTYPE, st_data.bsDiffs.test(CItemData::AUTOTYPE) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, PWHIST, st_data.bsDiffs.test(CItemData::PWHIST) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, CTIME, st_data.bsDiffs.test(CItemData::CTIME) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, ATIME, st_data.bsDiffs.test(CItemData::ATIME) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, LTIME, st_data.bsDiffs.test(CItemData::LTIME) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, PMTIME, st_data.bsDiffs.test(CItemData::PMTIME) ? _T("X") : _T("-"));
  	  m_LCResults.SetItemText(iItem, RMTIME, st_data.bsDiffs.test(CItemData::RMTIME) ? _T("X") : _T("-"));

      st_data.listindex = iItem;
      m_LCResults.SetItemData(iItem, (DWORD)&st_data);
      iItem++;
		}
	}

  m_LCResults.SetRedraw(FALSE);
  for (i = 0; i < LAST; i++) {
    m_LCResults.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
    int header_width = m_LCResults.GetColumnWidth(i);
    m_LCResults.SetColumnWidth(i, LVSCW_AUTOSIZE);
    int data_width = m_LCResults.GetColumnWidth(i);
    if (header_width > data_width)
      m_LCResults.SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
  }
  m_LCResults.SetRedraw(TRUE);
  m_LCResults.Invalidate();

  // setup status bar for gripper only
  if (m_statusBar.CreateEx(this, SBARS_SIZEGRIP)) {
    statustext[0] = IDS_STATCOMPANY;
    m_statusBar.SetIndicators(statustext, 1);
    CString s;
    s.Format(IDS_COMPARERESULTS, 
      m_OnlyInCurrent.size(), m_OnlyInComp.size(), m_Conflicts.size());
    m_statusBar.SetPaneText(0, s, TRUE);
    m_statusBar.SetPaneInfo(0, m_statusBar.GetItemID(0), SBPS_STRETCH, NULL);
    m_statusBar.UpdateWindow();
  } else {
    TRACE(_T("Could not create status bar\n"));
  }

  // Put on StatusBar
  CRect rcClientStart;
  CRect rcClientNow;
  GetClientRect(rcClientStart);
  RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
  /*RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0,
                 reposQuery, rcClientNow); */

  // Arrange all the controls - needed for resizeable dialog
  CWnd *pwndListCtrl = GetDlgItem(IDC_RESULTLIST);
  CWnd *pwndOKButton = GetDlgItem(IDOK);
  CWnd *pwndCPYButton = GetDlgItem(IDC_COPYTOCLIPBOARD);

  CRect sbRect, ctrlRect, dlgRect;
  int xleft, ytop;

  GetClientRect(&dlgRect); 
  m_DialogMinWidth = dlgRect.Width();
  m_DialogMinHeight = dlgRect.Height();

  m_statusBar.GetWindowRect(&sbRect);

  pwndListCtrl->GetWindowRect(&ctrlRect);
  ScreenToClient(&ctrlRect);

  m_cxBSpace = dlgRect.Size().cx - ctrlRect.Size().cx;
  m_cyBSpace = dlgRect.Size().cy - ctrlRect.Size().cy;
  m_cySBar = sbRect.Size().cy;

  pwndListCtrl->SetWindowPos(NULL, NULL, NULL,
                        dlgRect.Size().cx - (2 * ctrlRect.TopLeft().x),
                        dlgRect.Size().cy - m_cyBSpace,
                        SWP_NOMOVE | SWP_NOZORDER);

  GetWindowRect(&dlgRect); 
  pwndCPYButton->GetWindowRect(&ctrlRect);
  xleft = (m_DialogMinWidth / 4) - (ctrlRect.Width() / 2);
  ytop = dlgRect.Height() - m_cyBSpace/2 - m_cySBar;

  pwndCPYButton->SetWindowPos(NULL, xleft, ytop, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);

  pwndOKButton->GetWindowRect(&ctrlRect);
  xleft = (3 * m_DialogMinWidth / 4) - (ctrlRect.Width() / 2);

  pwndOKButton->SetWindowPos(NULL, xleft, ytop, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);

  GetDlgItem(IDC_COMPAREORIGINALDB)->SetWindowText(m_cs_Filename1);
  GetDlgItem(IDC_COMPARECOMPARISONDB)->SetWindowText(m_cs_Filename2);
  return TRUE;
}

void CCompareResultsDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_RESULTLIST, m_LCResults);
}

BEGIN_MESSAGE_MAP(CCompareResultsDlg, CDialog)
  ON_WM_SIZE()
  ON_WM_GETMINMAXINFO()
  ON_NOTIFY(NM_DBLCLK, IDC_RESULTLIST, OnItemDoubleClick)
  ON_BN_CLICKED(ID_HELP, OnHelp)
  ON_BN_CLICKED(IDOK, OnOK)
  ON_BN_CLICKED(IDC_COPYTOCLIPBOARD, OnCopyToClipboard)
  ON_NOTIFY(HDN_ITEMCLICK, IDC_RESULTLISTHDR, OnColumnClick)
END_MESSAGE_MAP()

void
CCompareResultsDlg::OnCancel()
{
  CDialog::OnCancel();
}

void
CCompareResultsDlg::OnOK()
{
  CDialog::OnOK();
}

void
CCompareResultsDlg::OnHelp()
{
  CString cs_HelpTopic = app.GetHelpFileName() + _T("::/html/compare_results.html");
  HtmlHelp(DWORD_PTR((LPCTSTR)cs_HelpTopic), HH_DISPLAY_TOPIC);
}

void
CCompareResultsDlg::OnItemDoubleClick( NMHDR* /* pNMHDR */, LRESULT *pResult)
{
  int row, column, colwidth0;

  row = m_LCResults.GetNextItem(-1, LVNI_SELECTED);

  if (row == -1)
    return;

  CPoint pt;
  pt = ::GetMessagePos();
  ScreenToClient(&pt);

  colwidth0 = m_LCResults.GetColumnWidth(0);

  if (pt.x <= colwidth0)
    column = 0;
  else if  (pt.x <= (colwidth0 + m_LCResults.GetColumnWidth(1)))
    column = 1;
  else
    column = LAST;

  if (column < 2) {
    st_CompareData *st_data;
    st_data = (st_CompareData *)m_LCResults.GetItemData(row);
    int pos_index = st_data->index;
    if (column == pos_index || pos_index == -1) {
      POSITION pos = (column == 0) ? st_data->pos1 : st_data->pos2;
      ::SendMessage(AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
                    WM_VIEW_COMPARE_RESULT, (WPARAM)pos, (LPARAM)pos_index);
    }
  }

  *pResult = 0;
}

void
CCompareResultsDlg::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
  NMHEADER *pNMHeaderCtrl  = (NMHEADER *)pNMHDR;

  // Get column index to CItemData value
  int isortcolumn = pNMHeaderCtrl->iItem;

  if (m_iSortedColumn == isortcolumn) {
    m_bSortAscending = !m_bSortAscending;
  } else {
    m_iSortedColumn = isortcolumn;
    m_bSortAscending = true;
  }

  m_LCResults.SortItems(CRCompareFunc, (LPARAM)this);

  // Reset item index
  for (int i = 0; i < m_LCResults.GetItemCount(); i++) {
    st_CompareData *st_data = (st_CompareData *)m_LCResults.GetItemData(i);
    st_data->listindex = i;
  }

#if (WINVER < 0x0501)  // These are already defined for WinXP and later
#define HDF_SORTUP 0x0400
#define HDF_SORTDOWN 0x0200
#endif

  HDITEM hdi;
  hdi.mask = HDI_FORMAT;

  CHeaderCtrl *pHDRCtrl;

  pHDRCtrl = m_LCResults.GetHeaderCtrl();
  pHDRCtrl->GetItem(isortcolumn, &hdi);
  // Turn off all arrows
  hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
  // Turn on the correct arrow
  hdi.fmt |= ((m_bSortAscending == TRUE) ? HDF_SORTUP : HDF_SORTDOWN);
  pHDRCtrl->SetItem(isortcolumn, &hdi);

  *pResult = TRUE;
}

  /*
   * Compare function used by m_LCResults.SortItems()
   * "The comparison function must return a negative value if the first item should precede
   * the second, a positive value if the first item should follow the second, or zero if
   * the two items are equivalent."
   */
int CALLBACK CCompareResultsDlg::CRCompareFunc(LPARAM lParam1, LPARAM lParam2,
				   LPARAM lParamSort)
{

  // m_bSortAscending to determine the direction of the sort (duh)

  CCompareResultsDlg *self = (CCompareResultsDlg *)lParamSort;
  int nSortColumn = self->m_iSortedColumn;
  st_CompareData *LHS_st_data = (st_CompareData *)lParam1;
  st_CompareData *RHS_st_data = (st_CompareData *)lParam2;
  int LHS_Index = LHS_st_data->listindex;
  int RHS_Index = RHS_st_data->listindex;

  CString LHS_ItemText = self->m_LCResults.GetItemText(LHS_Index, nSortColumn);
  CString RHS_ItemText = self->m_LCResults.GetItemText(RHS_Index, nSortColumn);

  int iResult = LHS_ItemText.CompareNoCase(RHS_ItemText);

  if (!self->m_bSortAscending) {
    iResult *= -1;
  }
  return iResult;
}

void
CCompareResultsDlg::OnCopyToClipboard()
{
  CompareData::iterator cd_iter;
  CString resultStr(_T(""));
  CString buffer;

	if (m_OnlyInCurrent.size() > 0) {
		buffer.Format(IDS_COMPAREENTRIES1, m_cs_Filename1);
		resultStr += buffer;
    for (cd_iter = m_OnlyInCurrent.begin(); cd_iter != m_OnlyInCurrent.end();
         cd_iter++) {
      const st_CompareData &st_data = *cd_iter;

			buffer.Format(IDS_COMPARESTATS, st_data.group, st_data.title, st_data.user);
			resultStr += buffer;
		}
		resultStr += _T("\n");
	}

	if (m_OnlyInComp.size() > 0) {
		buffer.Format(IDS_COMPAREENTRIES2, m_cs_Filename2);
		resultStr += buffer;
    for (cd_iter = m_OnlyInComp.begin(); cd_iter != m_OnlyInComp.end();
         cd_iter++) {
      const st_CompareData &st_data = *cd_iter;

      buffer.Format(IDS_COMPARESTATS, st_data.group, st_data.title, st_data.user);
			resultStr += buffer;
		}
		resultStr += _T("\n");
	}

	if (m_Conflicts.size() > 0) {
		buffer.Format(IDS_COMPAREBOTHDIFF2, m_cs_Filename1, m_cs_Filename2);
		resultStr += buffer;

		const CString csx_password(MAKEINTRESOURCE(IDS_COMPPASSWORD));
		const CString csx_notes(MAKEINTRESOURCE(IDS_COMPNOTES));
		const CString csx_url(MAKEINTRESOURCE(IDS_COMPURL));
		const CString csx_autotype(MAKEINTRESOURCE(IDS_COMPAUTOTYPE));
		const CString csx_ctime(MAKEINTRESOURCE(IDS_COMPCTIME));
		const CString csx_pmtime(MAKEINTRESOURCE(IDS_COMPPMTIME));
		const CString csx_atime(MAKEINTRESOURCE(IDS_COMPATIME));
		const CString csx_ltime(MAKEINTRESOURCE(IDS_COMPLTIME));
		const CString csx_rmtime(MAKEINTRESOURCE(IDS_COMPRMTIME));
		const CString csx_pwhistory(MAKEINTRESOURCE(IDS_COMPPWHISTORY));

    for (cd_iter = m_Conflicts.begin(); cd_iter != m_Conflicts.end();
         cd_iter++) {
      const st_CompareData &st_data = *cd_iter;

      buffer.Format(IDS_COMPARESTATS2, st_data.group, st_data.title, st_data.user);
			resultStr += buffer;

 			if (st_data.bsDiffs.test(CItemData::PASSWORD)) resultStr += csx_password;
			if (st_data.bsDiffs.test(CItemData::NOTES)) resultStr += csx_notes;
			if (st_data.bsDiffs.test(CItemData::URL)) resultStr += csx_url;
			if (st_data.bsDiffs.test(CItemData::AUTOTYPE)) resultStr += csx_autotype;
			if (st_data.bsDiffs.test(CItemData::CTIME)) resultStr += csx_ctime;
			if (st_data.bsDiffs.test(CItemData::PMTIME)) resultStr += csx_pmtime;
			if (st_data.bsDiffs.test(CItemData::ATIME)) resultStr += csx_atime;
			if (st_data.bsDiffs.test(CItemData::LTIME)) resultStr += csx_ltime;
			if (st_data.bsDiffs.test(CItemData::RMTIME)) resultStr += csx_rmtime;
			if (st_data.bsDiffs.test(CItemData::PWHIST)) resultStr += csx_pwhistory;
		}
	}

	app.SetClipboardData(resultStr);
}

void
CCompareResultsDlg::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);

  CWnd *pwndListCtrl = GetDlgItem(IDC_RESULTLIST); 
  CWnd *pwndODBText = GetDlgItem(IDC_COMPAREORIGINALDB);
  CWnd *pwndCDBText = GetDlgItem(IDC_COMPARECOMPARISONDB);
  CWnd *pwndCPY = GetDlgItem(IDC_COPYTOCLIPBOARD); 
  CWnd *pwndOK = GetDlgItem(IDOK); 

  if (!IsWindow(pwndListCtrl->GetSafeHwnd())) 
    return;

  CRect ctrlRect, dlgRect;
  CPoint pt_top, pt;

  GetWindowRect(&dlgRect);

  // Allow the database names window width to grow/shrink (not height)
  pwndODBText->GetWindowRect(&ctrlRect);
  pt_top.x = ctrlRect.left;
  pt_top.y = ctrlRect.top;
  ScreenToClient(&pt_top);
  pwndODBText->MoveWindow(pt_top.x, pt_top.y,
                    cx - pt_top.x - 5,
                    ctrlRect.Height(),
                    TRUE);

  GetDlgItem(IDC_COMPAREORIGINALDB)->SetWindowText(m_cs_Filename1);

  pwndCDBText->GetWindowRect(&ctrlRect);
  pt_top.x = ctrlRect.left;
  pt_top.y = ctrlRect.top;
  ScreenToClient(&pt_top);
  pwndCDBText->MoveWindow(pt_top.x, pt_top.y,
                    cx - pt_top.x - 5,
                    ctrlRect.Height(),
                    TRUE);

  GetDlgItem(IDC_COMPARECOMPARISONDB)->SetWindowText(m_cs_Filename2);

  // Allow ListCtrl to grow/shrink but leave room for the buttons underneath!
  pwndListCtrl->GetWindowRect(&ctrlRect);
  pt_top.x = ctrlRect.left;
  pt_top.y = ctrlRect.top;
  ScreenToClient(&pt_top);

  pwndListCtrl->MoveWindow(pt_top.x, pt_top.y,
                        cx - (2 * pt_top.x),
                        cy - m_cyBSpace,
                        TRUE); 

  // Keep buttons in the bottom area
  int xleft, ytop;
  pwndCPY->GetWindowRect(&ctrlRect);
  xleft = (cx / 4) - (ctrlRect.Width() / 2);
  ytop = dlgRect.Height() - m_cyBSpace / 2 - m_cySBar;

  pwndCPY->SetWindowPos(NULL, xleft, ytop, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);

  pwndOK->GetWindowRect(&ctrlRect);
  xleft = (3 * cx / 4) - (ctrlRect.Width() / 2);

  pwndOK->SetWindowPos(NULL, xleft, ytop, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);

  m_statusBar.GetWindowRect(&ctrlRect);
  pt_top.x = ctrlRect.left;
  pt_top.y = ctrlRect.top;
  ScreenToClient(&pt_top);

  m_statusBar.MoveWindow(pt_top.x, cy - ctrlRect.Height(),
                        cx - (2 * pt_top.x),
                        ctrlRect.Height(),
                        TRUE); 
}

void CCompareResultsDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI) 
{
  CWnd::OnGetMinMaxInfo(lpMMI);

  lpMMI->ptMinTrackSize = CPoint(m_DialogMinWidth, m_DialogMinHeight);
}
