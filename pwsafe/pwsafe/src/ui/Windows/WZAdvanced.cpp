/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// CWZAdvanced.cpp : implementation file
//

#include "stdafx.h"
#include "passwordsafe.h"
#include "ThisMfcApp.h"
#include "DboxMain.h"
#include "GeneralMsgBox.h"

#include "WZAdvanced.h"
#include "WZPropertySheet.h"

#include "core/ItemData.h"
#include "core/core.h"

#include "resource.h"
#include "resource3.h"

#include <bitset>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NORMALFIELD    0x1000
#define MANDATORYFIELD 0x0800

int CWZAdvanced::dialog_lookup[] = {
  IDD_WZADVANCED,       // COMPARE
  IDD_WZADVANCEDTOP,    // MERGE (reduced dialog - top half only)
  IDD_WZADVANCED,       // SYNCH
  IDD_WZADVANCED,       // EXPORT_TEXT
  IDD_WZADVANCEDBOTTOM, // EXPORT_ENTRYTEXT (reduced dialog - bottom half only)
  IDD_WZADVANCED,       // EXPORT_XML
  IDD_WZADVANCEDBOTTOM, // EXPORT_ENTRYXML (reduced dialog - bottom half only)
  IDD_WZADVANCEDBOTTOM  // SYNCH only one entry during DB Compare
                        //  (reduced dialog - bottom half only)
};

/////////////////////////////////////////////////////////////////////////////
// CWZAdvanced dialog

IMPLEMENT_DYNAMIC(CWZAdvanced, CWZPropertyPage)

CWZAdvanced::CWZAdvanced(CWnd *pParent, UINT nIDCaption, const int nType, WZAdvanced::AdvType iIndex,
                             st_SaveAdvValues *pst_SADV)
  : CWZPropertyPage(dialog_lookup[iIndex], nIDCaption, nType), m_iIndex(iIndex), 
  m_pst_SADV(pst_SADV), m_pToolTipCtrl(NULL), m_treatwhitespaceasempty(BST_CHECKED)
{
  ASSERT(sizeof(dialog_lookup) / sizeof(int) == WZAdvanced::LAST);

  // Save pointer to my PropertySheet
  m_pWZPSH = (CWZPropertySheet *)pParent;

  if (m_pst_SADV != NULL) {
    m_bsFields = m_pst_SADV->bsFields;
    m_subgroup_name = m_pst_SADV->subgroup_name;
    m_subgroup_set = m_pst_SADV->subgroup_set;
    m_subgroup_object = m_pst_SADV->subgroup_object;
    m_subgroup_function = m_pst_SADV->subgroup_function;
    m_subgroup_case = m_pst_SADV->subgroup_case;
    m_treatwhitespaceasempty = m_pst_SADV->treatwhitespaceasempty;
  } else {
    m_bsFields.set();
    m_subgroup_name = L"";
    m_subgroup_set = m_subgroup_case = BST_UNCHECKED;
    m_treatwhitespaceasempty = BST_CHECKED;
    m_subgroup_object =  m_subgroup_function = 0;
  }

  if (m_subgroup_function < 0) {
    m_subgroup_case = BST_CHECKED;
    m_subgroup_function *= (-1);
  }

  if (m_subgroup_set == BST_UNCHECKED)
    m_subgroup_name = L"*";

  if (m_bsFields.count() == 0)
    m_bsFields.set();
}

CWZAdvanced::~CWZAdvanced()
{
  if (m_iIndex != WZAdvanced::MERGE)
    delete m_pToolTipCtrl;
}

void CWZAdvanced::DoDataExchange(CDataExchange* pDX)
{
  CWZPropertyPage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CWZAdvanced)
  if (m_iIndex != WZAdvanced::COMPARESYNCH && 
      m_iIndex != WZAdvanced::EXPORT_ENTRYTEXT &&
      m_iIndex != WZAdvanced::EXPORT_ENTRYXML) {
    DDX_Check(pDX, IDC_ADVANCED_SUBGROUP_SET, m_subgroup_set);
    DDX_Check(pDX, IDC_ADVANCED_SUBGROUP_CASE, m_subgroup_case);
    DDX_Text(pDX, IDC_ADVANCED_SUBGROUP_NAME, m_subgroup_name);
  }
  if (m_iIndex == WZAdvanced::COMPARE)
    DDX_Check(pDX, IDC_TREATWHITESPACEASEMPTY, m_treatwhitespaceasempty);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWZAdvanced, CWZPropertyPage)
  //{{AFX_MSG_MAP(CWZAdvanced)
  ON_BN_CLICKED(IDC_ADVANCED_SUBGROUP_SET, OnSetSubGroup)
  ON_BN_CLICKED(IDC_ADVANCED_SELECTSOME, OnSelectSome)
  ON_BN_CLICKED(IDC_ADVANCED_SELECTALL, OnSelectAll)
  ON_BN_CLICKED(IDC_ADVANCED_DESELECTSOME, OnDeselectSome)
  ON_BN_CLICKED(IDC_ADVANCED_DESELECTALL, OnDeselectAll)
  ON_BN_CLICKED(IDC_ADVANCED_RESET, OnReset)
  ON_NOTIFY(LVN_ITEMCHANGING, IDC_ADVANCED_SELECTED, OnSelectedItemChanging)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWZAdvanced::OnInitDialog()
{
  CWZPropertyPage::OnInitDialog();

  CString cs_text, cs_tmp;
  int iItem(-1), i;

  // Make Next button say the function
  cs_tmp.LoadString(m_pWZPSH->GetButtonID());
  m_pWZPSH->GetDlgItem(ID_WIZNEXT)->SetWindowText(cs_tmp);

  // Make sure back button is displayed
  m_pWZPSH->GetDlgItem(ID_WIZBACK)->EnableWindow(TRUE);
  m_pWZPSH->GetDlgItem(ID_WIZBACK)->ShowWindow(SW_SHOW);

  m_bsAllowedFields.reset();
  m_bsDefaultSelectedFields.reset();
  m_bsMandatoryFields.reset();

  switch (m_iIndex) {
    case WZAdvanced::COMPARE:
      cs_text.LoadString(IDS_COMPAREX);
      break;
    case WZAdvanced::MERGE:
      cs_text.LoadString(IDS_MERGEX);
      break;
    case WZAdvanced::SYNCH:
    case WZAdvanced::COMPARESYNCH:
      cs_text.LoadString(IDS_SYNCHRONIZEX);
      break;
    case WZAdvanced::EXPORT_TEXT:
      cs_text.Format(IDS_EXPORT_TEXTX);
      break;
    case WZAdvanced::EXPORT_ENTRYTEXT:
      cs_text.LoadString(IDS_EXPORT_TEXTX_SINGLE);
      break;
    case WZAdvanced::EXPORT_XML:
      cs_text.Format(IDS_EXPORT_XMLX);
      break;
    case WZAdvanced::EXPORT_ENTRYXML:
      cs_text.LoadString(IDS_EXPORT_XMLX_SINGLE);
      break;
    default:
      ASSERT(FALSE);
  }
  m_pWZPSH->SetWindowText(cs_text);

  if (m_iIndex != WZAdvanced::COMPARESYNCH &&
      m_iIndex != WZAdvanced::EXPORT_ENTRYTEXT &&
      m_iIndex != WZAdvanced::EXPORT_ENTRYXML) {
    CComboBox *cboSubgroupFunction = (CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_FUNCTION);
    if (cboSubgroupFunction->GetCount() == 0) {
      cs_text.LoadString(IDSC_EQUALS);
      iItem = cboSubgroupFunction->AddString(cs_text);
      cboSubgroupFunction->SetItemData(iItem, PWSMatch::MR_EQUALS);
      cs_text.LoadString(IDSC_DOESNOTEQUAL);
      iItem = cboSubgroupFunction->AddString(cs_text);
      cboSubgroupFunction->SetItemData(iItem, PWSMatch::MR_NOTEQUAL);
      cs_text.LoadString(IDSC_BEGINSWITH);
      iItem = cboSubgroupFunction->AddString(cs_text);
      cboSubgroupFunction->SetItemData(iItem, PWSMatch::MR_BEGINS);
      cs_text.LoadString(IDSC_DOESNOTBEGINSWITH);
      iItem = cboSubgroupFunction->AddString(cs_text);
      cboSubgroupFunction->SetItemData(iItem, PWSMatch::MR_NOTBEGIN);
      cs_text.LoadString(IDSC_ENDSWITH);
      iItem = cboSubgroupFunction->AddString(cs_text);
      cboSubgroupFunction->SetItemData(iItem, PWSMatch::MR_ENDS);
      cs_text.LoadString(IDSC_DOESNOTENDWITH);
      iItem = cboSubgroupFunction->AddString(cs_text);
      cboSubgroupFunction->SetItemData(iItem, PWSMatch::MR_NOTEND);
      cs_text.LoadString(IDSC_CONTAINS);
      iItem = cboSubgroupFunction->AddString(cs_text);
      cboSubgroupFunction->SetItemData(iItem, PWSMatch::MR_CONTAINS);
      cs_text.LoadString(IDSC_DOESNOTCONTAIN);
      iItem = cboSubgroupFunction->AddString(cs_text);
      cboSubgroupFunction->SetItemData(iItem, PWSMatch::MR_NOTCONTAIN);
    }

    for (i = 0; i < cboSubgroupFunction->GetCount(); i++) {
      if ((int)cboSubgroupFunction->GetItemData(i) == m_subgroup_function) {
        cboSubgroupFunction->SetCurSel(i);
        break;
      }
    }

    // Note: NOT SORTED by design
    CComboBox *cboSubgroupObject = (CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_OBJECT);
    if (cboSubgroupObject->GetCount () == 0) {
      cs_text.LoadString(IDS_GROUP);
      iItem = cboSubgroupObject->AddString(cs_text);
      cboSubgroupObject->SetItemData(iItem, CItemData::GROUP);
      cs_text.LoadString(IDS_GROUPTITLE);
      iItem = cboSubgroupObject->AddString(cs_text);
      cboSubgroupObject->SetItemData(iItem, CItemData::GROUPTITLE);
      cs_text.LoadString(IDS_TITLE);
      iItem = cboSubgroupObject->AddString(cs_text);
      cboSubgroupObject->SetItemData(iItem, CItemData::TITLE);
      cs_text.LoadString(IDS_USERNAME);
      iItem = cboSubgroupObject->AddString(cs_text);
      cboSubgroupObject->SetItemData(iItem, CItemData::USER);
      cs_text.LoadString(IDS_URL);
      iItem = cboSubgroupObject->AddString(cs_text);
      cboSubgroupObject->SetItemData(iItem, CItemData::URL);
      cs_text.LoadString(IDS_NOTES);
      iItem = cboSubgroupObject->AddString(cs_text);
      cboSubgroupObject->SetItemData(iItem, CItemData::NOTES);
    }

    for (i = 0; i < cboSubgroupObject->GetCount(); i++) {
      if ((int)cboSubgroupObject->GetItemData(i) == m_subgroup_object) {
        cboSubgroupObject->SetCurSel(i);
        break;
      }
    }

    BOOL bEnable = (m_subgroup_set == BST_CHECKED) ? TRUE : FALSE;
    GetDlgItem(IDC_ADVANCED_SUBGROUP_FUNCTION)->EnableWindow(bEnable);
    GetDlgItem(IDC_ADVANCED_SUBGROUP_OBJECT)->EnableWindow(bEnable);
    GetDlgItem(IDC_ADVANCED_SUBGROUP_NAME)->EnableWindow(bEnable);
    GetDlgItem(IDC_ADVANCED_SUBGROUP_CASE)->EnableWindow(bEnable);
  }

  // Merge does not allow field selection - get out now
  if (m_iIndex == WZAdvanced::MERGE)
    return TRUE;

  // m_pLC_List are those fields that aren't currently selected but could be
  // m_pLC_Selected are those fields already selected
  m_pLC_List = (CListCtrl *)GetDlgItem(IDC_ADVANCED_LIST);
  m_pLC_Selected = (CListCtrl *)GetDlgItem(IDC_ADVANCED_SELECTED);

  m_pLC_List->InsertColumn(0, L"");
  m_pLC_Selected->InsertColumn(0, L"");
  m_pLC_List->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
  m_pLC_Selected->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

  // Deal with non-text fields
  // Time fields - use Compare text and remove the quotes and leading blank

  // NOTE: ItemData field cannot be zero - so add 0x1000 to ensure this.
  // However, we add only 0x800 to mandatory fields to ensure that they sort to top of list
  switch (m_iIndex) {
    case WZAdvanced::COMPARE:
      // All these are potential comparison fields
      cs_text.LoadString(IDS_COMPCTIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_List->InsertItem(++iItem, cs_text);
      m_pLC_List->SetItemData(iItem, CItemData::CTIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::CTIME);

      cs_text.LoadString(IDS_COMPPMTIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_List->InsertItem(++iItem, cs_text);
      m_pLC_List->SetItemData(iItem, CItemData::PMTIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::PMTIME);

      cs_text.LoadString(IDS_COMPATIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_List->InsertItem(++iItem, cs_text);
      m_pLC_List->SetItemData(iItem, CItemData::ATIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::ATIME);

      cs_text.LoadString(IDS_COMPRMTIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_List->InsertItem(++iItem, cs_text);
      m_pLC_List->SetItemData(iItem, CItemData::RMTIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::RMTIME);

      cs_text.LoadString(IDS_COMPXTIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_List->InsertItem(++iItem, cs_text);
      m_pLC_List->SetItemData(iItem, CItemData::XTIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::XTIME);

      cs_text.LoadString(IDS_PASSWORDEXPIRYDATEINT);
      iItem = m_pLC_List->InsertItem(++iItem, cs_text);
      m_pLC_List->SetItemData(iItem, CItemData::XTIME_INT | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::XTIME_INT);

      cs_text.LoadString(IDS_PWPOLICY);
      iItem = m_pLC_List->InsertItem(++iItem, cs_text);
      m_pLC_List->SetItemData(iItem, CItemData::POLICY | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::POLICY);

      cs_text.LoadString(IDS_DCALONG);
      iItem = m_pLC_List->InsertItem(++iItem, cs_text);
      m_pLC_List->SetItemData(iItem, CItemData::DCA | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::DCA);

      cs_text.LoadString(IDS_PROTECTED);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::PROTECTED | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::PROTECTED);
      break;
    case WZAdvanced::SYNCH:
    case WZAdvanced::COMPARESYNCH:
    case WZAdvanced::EXPORT_TEXT:
    case WZAdvanced::EXPORT_ENTRYTEXT:
    case WZAdvanced::EXPORT_XML:
    case WZAdvanced::EXPORT_ENTRYXML:
      // All these are already selected fields
      cs_text.LoadString(IDS_COMPCTIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::CTIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::CTIME);
      m_bsDefaultSelectedFields.set(CItemData::CTIME);

      cs_text.LoadString(IDS_COMPPMTIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::PMTIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::PMTIME);
      m_bsDefaultSelectedFields.set(CItemData::PMTIME);

      cs_text.LoadString(IDS_COMPATIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::ATIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::ATIME);
      m_bsDefaultSelectedFields.set(CItemData::ATIME);

      cs_text.LoadString(IDS_COMPRMTIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::RMTIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::RMTIME);
      m_bsDefaultSelectedFields.set(CItemData::RMTIME);

      cs_text.LoadString(IDS_COMPXTIME);
      cs_text = cs_text.Mid(2, cs_text.GetLength() - 3);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::XTIME | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::XTIME);
      m_bsDefaultSelectedFields.set(CItemData::XTIME);

      cs_text.LoadString(IDS_PASSWORDEXPIRYDATEINT);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::XTIME_INT | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::XTIME_INT);
      m_bsDefaultSelectedFields.set(CItemData::XTIME_INT);

      cs_text.LoadString(IDS_PWPOLICY);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::POLICY | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::POLICY);
      m_bsDefaultSelectedFields.set(CItemData::POLICY);

      cs_text.LoadString(IDS_DCALONG);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::DCA | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::DCA);
      m_bsDefaultSelectedFields.set(CItemData::DCA);

      cs_text.LoadString(IDS_PROTECTED);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::PROTECTED | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::PROTECTED);
      m_bsDefaultSelectedFields.set(CItemData::PROTECTED);
      break;
    default:
      ASSERT(FALSE);
      break;
  }

  // Deal with text fields - all selected by default
  cs_text.LoadString(IDS_NOTES);
  iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
  m_pLC_Selected->SetItemData(iItem, CItemData::NOTES | NORMALFIELD);
  m_bsAllowedFields.set(CItemData::NOTES);
  m_bsDefaultSelectedFields.set(CItemData::NOTES);

  cs_text.LoadString(IDS_URL);
  iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
  m_pLC_Selected->SetItemData(iItem, CItemData::URL | NORMALFIELD);
  m_bsAllowedFields.set(CItemData::URL);
  m_bsDefaultSelectedFields.set(CItemData::URL);

  cs_text.LoadString(IDS_AUTOTYPE);
  iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
  m_pLC_Selected->SetItemData(iItem, CItemData::AUTOTYPE | NORMALFIELD);
  m_bsAllowedFields.set(CItemData::AUTOTYPE);
  m_bsDefaultSelectedFields.set(CItemData::AUTOTYPE);

  cs_text.LoadString(IDS_PWHISTORY);
  iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
  m_pLC_Selected->SetItemData(iItem, CItemData::PWHIST | NORMALFIELD);
  m_bsAllowedFields.set(CItemData::PWHIST);
  m_bsDefaultSelectedFields.set(CItemData::PWHIST);

  cs_text.LoadString(IDS_RUNCOMMAND);
  iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
  m_pLC_Selected->SetItemData(iItem, CItemData::RUNCMD | NORMALFIELD);
  m_bsAllowedFields.set(CItemData::RUNCMD);
  m_bsDefaultSelectedFields.set(CItemData::RUNCMD);

  cs_text.LoadString(IDS_EMAIL);
  iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
  m_pLC_Selected->SetItemData(iItem, CItemData::EMAIL | NORMALFIELD);
  m_bsAllowedFields.set(CItemData::EMAIL);
  m_bsDefaultSelectedFields.set(CItemData::EMAIL);

  // Deal with standard text fields - selected by default
  switch (m_iIndex) {
    case WZAdvanced::EXPORT_XML:
    case WZAdvanced::EXPORT_ENTRYXML:
      cs_text.LoadString(IDS_GROUP);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::GROUP | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::GROUP);
      m_bsDefaultSelectedFields.set(CItemData::GROUP);

      cs_text.LoadString(IDS_ADVANCED_TITLETEXT); // <-- Special - Mandatory field
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::TITLE | MANDATORYFIELD);
      m_bsAllowedFields.set(CItemData::TITLE);
      m_bsDefaultSelectedFields.set(CItemData::TITLE);
      m_bsMandatoryFields.set(CItemData::TITLE);

      cs_text.LoadString(IDS_USERNAME);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::USER | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::USER);
      m_bsDefaultSelectedFields.set(CItemData::USER);

      cs_text.LoadString(IDS_ADVANCED_PASSWORDTEXT); // <-- Special - Mandatory field
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::PASSWORD | MANDATORYFIELD);
      m_bsAllowedFields.set(CItemData::PASSWORD);
      m_bsDefaultSelectedFields.set(CItemData::PASSWORD);
      m_bsMandatoryFields.set(CItemData::PASSWORD);
      break;
    case WZAdvanced::COMPARE:
      cs_text.LoadString(IDS_ADVANCED_GROUPTEXT); // <-- Special - Mandatory field
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::GROUP | MANDATORYFIELD);
      m_bsAllowedFields.set(CItemData::GROUP);
      m_bsDefaultSelectedFields.set(CItemData::GROUP);
      m_bsMandatoryFields.set(CItemData::GROUP);

      cs_text.LoadString(IDS_ADVANCED_TITLETEXT); // <-- Special - Mandatory field
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::TITLE | MANDATORYFIELD);
      m_bsAllowedFields.set(CItemData::TITLE);
      m_bsDefaultSelectedFields.set(CItemData::TITLE);
      m_bsMandatoryFields.set(CItemData::TITLE);

      cs_text.LoadString(IDS_ADVANCED_USERTEXT); // <-- Special - Mandatory field
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::USER | MANDATORYFIELD);
      m_bsAllowedFields.set(CItemData::USER);
      m_bsDefaultSelectedFields.set(CItemData::USER);
      m_bsMandatoryFields.set(CItemData::USER);

      cs_text.LoadString(IDS_PASSWORD);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::PASSWORD | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::PASSWORD);
      m_bsDefaultSelectedFields.set(CItemData::PASSWORD);
      break;
    case WZAdvanced::SYNCH:
    case WZAdvanced::COMPARESYNCH:
      cs_text.LoadString(IDS_PASSWORD);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::PASSWORD | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::PASSWORD);
      m_bsDefaultSelectedFields.set(CItemData::PASSWORD);
      break;
    case WZAdvanced::EXPORT_TEXT:
    case WZAdvanced::EXPORT_ENTRYTEXT:
      cs_text.LoadString(IDS_GROUP);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::GROUP | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::GROUP);
      m_bsDefaultSelectedFields.set(CItemData::GROUP);

      cs_text.LoadString(IDS_TITLE);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::TITLE | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::TITLE);
      m_bsDefaultSelectedFields.set(CItemData::TITLE);

      cs_text.LoadString(IDS_USERNAME);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::USER | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::USER);
      m_bsDefaultSelectedFields.set(CItemData::USER);

      cs_text.LoadString(IDS_PASSWORD);
      iItem = m_pLC_Selected->InsertItem(++iItem, cs_text);
      m_pLC_Selected->SetItemData(iItem, CItemData::PASSWORD | NORMALFIELD);
      m_bsAllowedFields.set(CItemData::PASSWORD);
      m_bsDefaultSelectedFields.set(CItemData::PASSWORD);
      break;
    default:
      ASSERT(FALSE);
  }

  if (m_bsFields.count() != 0 && m_bsFields.count() != m_bsFields.size()) {
    Set(m_bsFields);
  }

  m_pLC_List->SortItems(AdvCompareFunc, NULL);
  m_pLC_Selected->SortItems(AdvCompareFunc, NULL);

  if (m_iIndex != WZAdvanced::COMPARESYNCH) {
    if (m_iIndex != WZAdvanced::COMPARE) {
      GetDlgItem(IDC_TREATWHITESPACEASEMPTY)->EnableWindow(FALSE);
      GetDlgItem(IDC_TREATWHITESPACEASEMPTY)->ShowWindow(SW_HIDE);
    } else {
      ((CButton *)GetDlgItem(IDC_TREATWHITESPACEASEMPTY))->SetCheck(BST_CHECKED);
    }
  }

  if (m_iIndex == WZAdvanced::EXPORT_ENTRYTEXT || m_iIndex == WZAdvanced::EXPORT_ENTRYXML) {
    GetDlgItem(IDC_ADVANCED_SUBGROUP_SET)->EnableWindow(FALSE);
    GetDlgItem(IDC_STATIC_WHERE)->EnableWindow(FALSE);
    GetDlgItem(IDC_STATIC_TEXT)->EnableWindow(FALSE);
  }

  m_pToolTipCtrl = new CToolTipCtrl;
  if (!m_pToolTipCtrl->Create(this, TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX)) {
    pws_os::Trace(L"Unable To create Advanced Dialog ToolTip\n");
    delete m_pToolTipCtrl;
    m_pToolTipCtrl = NULL;
    return TRUE;
  }

  // Tooltips
  EnableToolTips();

  // Activate the tooltip control.
  m_pToolTipCtrl->Activate(TRUE);
  m_pToolTipCtrl->SetMaxTipWidth(300);
  // Quadruple the time to allow reading by user
  int iTime = m_pToolTipCtrl->GetDelayTime(TTDT_AUTOPOP);
  m_pToolTipCtrl->SetDelayTime(TTDT_AUTOPOP, 4 * iTime);

  // Set the tooltip
  // Note naming convention: string IDS_xxx corresponds to control IDC_xxx
  CString cs_ToolTip;
  cs_ToolTip.LoadString(IDS_ADVANCED_SELECTSOME);
  m_pToolTipCtrl->AddTool(GetDlgItem(IDC_ADVANCED_SELECTSOME), cs_ToolTip);
  cs_ToolTip.LoadString(IDS_ADVANCED_SELECTALL);
  m_pToolTipCtrl->AddTool(GetDlgItem(IDC_ADVANCED_SELECTALL), cs_ToolTip);
  cs_ToolTip.LoadString(IDS_ADVANCED_DESELECTSOME);
  m_pToolTipCtrl->AddTool(GetDlgItem(IDC_ADVANCED_DESELECTSOME), cs_ToolTip);
  cs_ToolTip.LoadString(IDS_ADVANCED_DESELECTALL);
  m_pToolTipCtrl->AddTool(GetDlgItem(IDC_ADVANCED_DESELECTALL), cs_ToolTip);

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CWZAdvanced message handlers

void CWZAdvanced::OnReset()
{
  if (m_iIndex != WZAdvanced::MERGE) {
    Set(m_bsDefaultSelectedFields);

    m_pLC_List->SortItems(AdvCompareFunc, NULL);
    m_pLC_Selected->SortItems(AdvCompareFunc, NULL);
    m_pLC_List->Invalidate();
    m_pLC_Selected->Invalidate();
  }

  m_subgroup_name = L"";
  m_subgroup_set = m_subgroup_case = BST_UNCHECKED;
  m_treatwhitespaceasempty = BST_CHECKED;
  m_subgroup_object =  m_subgroup_function = 0;

  ((CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_OBJECT))->SetCurSel(1);  // Group/Title
  ((CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_FUNCTION))->SetCurSel(-1);

  GetDlgItem(IDC_ADVANCED_SUBGROUP_FUNCTION)->EnableWindow(FALSE);
  GetDlgItem(IDC_ADVANCED_SUBGROUP_OBJECT)->EnableWindow(FALSE);
  GetDlgItem(IDC_ADVANCED_SUBGROUP_NAME)->EnableWindow(FALSE);
  GetDlgItem(IDC_ADVANCED_SUBGROUP_CASE)->EnableWindow(FALSE);

  UpdateData(FALSE);
}

void CWZAdvanced::Set(CItemData::FieldBits bsFields)
{
  LVFINDINFO findinfo;
  CString cs_text;
  int iItem;
  DWORD_PTR dw_data;

  SecureZeroMemory(&findinfo, sizeof(LVFINDINFO));

  findinfo.flags = LVFI_PARAM;
  // Note: Mandatory fields have a ItemData value + 0x800 rather than 0x1000
  // and so will not be found and so not moved anywhere.
  for (int i = 0; i < CItemData::LAST; i++) {
    // Don't move or allow non-allowed fields
    if (!m_bsAllowedFields.test(i))
      continue;

    if (bsFields.test(i)) {
      // Selected - find entry in list of available fields and move it
      findinfo.lParam = i | NORMALFIELD;
      iItem = m_pLC_List->FindItem(&findinfo);
      if (iItem == -1)
        continue;

      cs_text = m_pLC_List->GetItemText(iItem, 0);
      dw_data = m_pLC_List->GetItemData(iItem);
      m_pLC_List->DeleteItem(iItem);
      iItem = m_pLC_Selected->InsertItem(0, cs_text);
      m_pLC_Selected->SetItemData(iItem, dw_data);
    } else {
      // Not selected - find entry in list of selected fields and move it
      findinfo.lParam = i | NORMALFIELD;
      iItem = m_pLC_Selected->FindItem(&findinfo);
      if (iItem == -1)
        continue;

      cs_text = m_pLC_Selected->GetItemText(iItem, 0);
      dw_data = m_pLC_Selected->GetItemData(iItem);
      m_pLC_Selected->DeleteItem(iItem);
      iItem = m_pLC_List->InsertItem(0, cs_text);
      m_pLC_List->SetItemData(iItem, dw_data);
    }
  }
}

LRESULT CWZAdvanced::OnWizardNext()
{
  CGeneralMsgBox gmb;
  CString cs_text;
  DWORD_PTR dw_data;

  UpdateData();
  m_bsFields.reset();

  if (m_iIndex != WZAdvanced::MERGE) {
    int num_selected = m_pLC_Selected->GetItemCount();
    int nItem(-1);

    for (int i = 0; i < num_selected; i++) {
      nItem = m_pLC_Selected->GetNextItem(nItem, LVNI_ALL);
      dw_data = LOWORD(m_pLC_Selected->GetItemData(nItem));
      m_bsFields.set(dw_data & 0xff, true);
    }

    if (m_bsFields.count() == 0) {
      CString cs_error_msg;
      switch (m_iIndex) {
        case WZAdvanced::COMPARE:
          cs_error_msg.LoadString(IDS_NOFIELDSFORCOMPARE);
          break;
        case WZAdvanced::SYNCH:
        case WZAdvanced::COMPARESYNCH:
          cs_error_msg.LoadString(IDS_NOFIELDSFORSYNCH);
          break;
        case WZAdvanced::EXPORT_TEXT:
        case WZAdvanced::EXPORT_ENTRYTEXT:
        case WZAdvanced::EXPORT_XML:
        case WZAdvanced::EXPORT_ENTRYXML:
          cs_error_msg.LoadString(IDS_NOFIELDSFOREXPORT);
          break;
        case WZAdvanced::MERGE:
          break;
        default:
          ASSERT(0);
      }
      gmb.AfxMessageBox(cs_error_msg);
      m_bsFields.set();  // note: impossible to set them all even via the advanced dialog
      return -1;
    }
  }

  if (m_iIndex != WZAdvanced::COMPARESYNCH &&
      m_iIndex != WZAdvanced::EXPORT_ENTRYTEXT &&
      m_iIndex != WZAdvanced::EXPORT_ENTRYXML) {
    if (m_subgroup_set == BST_CHECKED) {
      GetDlgItemText(IDC_ADVANCED_SUBGROUP_NAME, m_subgroup_name);
      int nObject = ((CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_OBJECT))->GetCurSel();
      if (nObject == CB_ERR) {
        gmb.AfxMessageBox(IDS_NOOBJECT);
        m_bsFields.set();  // note: impossible to set them all even via the advanced dialog
        ((CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_OBJECT))->SetFocus();
        return -1;
      }

      int nFunction = ((CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_FUNCTION))->GetCurSel();
      if (nFunction == CB_ERR) {
        gmb.AfxMessageBox(IDS_NOFUNCTION);
        m_bsFields.set();  // note: impossible to set them all even via the advanced dialog
        ((CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_FUNCTION))->SetFocus();
        return -1;
      }

      m_subgroup_object = int(((CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_OBJECT))->GetItemData(nObject));
      m_subgroup_function = int(((CComboBox *)GetDlgItem(IDC_ADVANCED_SUBGROUP_FUNCTION))->GetItemData(nFunction));
      if (m_subgroup_case == BST_CHECKED)
        m_subgroup_function *= (-1);
    }
  }

  if (m_subgroup_name == L"*")
    m_subgroup_name.Empty();

  const bool bAdvanced = m_subgroup_set == BST_CHECKED;

  const UINT nID = m_pWZPSH->GetID();
  int iAll(0);

  switch (nID) {
    case ID_MENUITEM_MERGE:
    case ID_MENUITEM_COMPARE:
    case ID_MENUITEM_SYNCHRONIZE:
    case ID_MENUITEM_EXPORT2PLAINTEXT:
    case ID_MENUITEM_EXPORT2XML:
      iAll = 1;
      break;
    case ID_MENUITEM_EXPORTENT2PLAINTEXT:
    case ID_MENUITEM_EXPORTENT2XML:
      iAll = -1;
      break;
    default:
      ASSERT(0);
      break;
  }

  if (iAll != 0) {
    OrderedItemList orderedItemList;
    CString cs_title, cs_temp;

    if (iAll == 1) {
      // Note: MakeOrderedItemList gets its members by walking the 
      // tree therefore, if a filter is active, it will ONLY export
      // those being displayed.
      m_pWZPSH->WZPSHMakeOrderedItemList(orderedItemList);
    } else {
      // Note: Only selected entry
      CItemData *pci = m_pWZPSH->WZPSHgetSelectedItem();
      orderedItemList.push_back(*pci);
    }

    // Check if there are any that meet the user's filter
    int rc = m_pWZPSH->WZPSHTestForExport(bAdvanced, (LPCWSTR)m_subgroup_name,
                     m_subgroup_object, m_subgroup_function, &orderedItemList);

    orderedItemList.clear(); // cleanup soonest

    if (rc == PWScore::NO_ENTRIES_EXPORTED) {
      CGeneralMsgBox gmb;
      cs_temp.LoadString(IDS_NO_ENTRIES_EXPORTED);
      cs_title.LoadString(IDS_TEXTEXPORTFAILED);
      gmb.MessageBox(cs_temp, cs_title, MB_OK | MB_ICONWARNING);
      return -1;
    }
  }

  m_pst_SADV->bsFields = m_bsFields;
  m_pst_SADV->subgroup_name = m_subgroup_name;
  m_pst_SADV->subgroup_set = m_subgroup_set;
  m_pst_SADV->subgroup_object = m_subgroup_object;
  m_pst_SADV->subgroup_function = m_subgroup_function;
  m_pst_SADV->subgroup_case = m_subgroup_case;
  m_pst_SADV->treatwhitespaceasempty = m_treatwhitespaceasempty;

  m_pWZPSH->SetAdvValues(m_pst_SADV);
  return CPropertyPage::OnWizardNext();
}

void CWZAdvanced::OnSetSubGroup()
{
  m_subgroup_set = ((CButton*)GetDlgItem(IDC_ADVANCED_SUBGROUP_SET))->GetCheck();

  BOOL bEnable = (m_subgroup_set == BST_CHECKED) ? TRUE : FALSE;
  GetDlgItem(IDC_ADVANCED_SUBGROUP_FUNCTION)->EnableWindow(bEnable);
  GetDlgItem(IDC_ADVANCED_SUBGROUP_OBJECT)->EnableWindow(bEnable);
  GetDlgItem(IDC_ADVANCED_SUBGROUP_NAME)->EnableWindow(bEnable);
  GetDlgItem(IDC_ADVANCED_SUBGROUP_CASE)->EnableWindow(bEnable);
}

void CWZAdvanced::OnSelectSome()
{
  CString cs_text;
  int iItem;
  DWORD_PTR dw_data;

  int num_selected = m_pLC_List->GetItemCount();
  if (num_selected == 0)
    return;

  int nItem(-1);

  for (int i = 0; i < num_selected; i++) {
    nItem = m_pLC_List->GetNextItem(nItem, LVNI_SELECTED);
    if (nItem == -1)
      continue;
    cs_text = m_pLC_List->GetItemText(nItem, 0);
    dw_data = m_pLC_List->GetItemData(nItem);
    m_pLC_List->DeleteItem(nItem);
    iItem = m_pLC_Selected->InsertItem(0, cs_text);
    m_pLC_Selected->SetItemData(iItem, dw_data);
  }

  m_pLC_List->SortItems(AdvCompareFunc, NULL);
  m_pLC_Selected->SortItems(AdvCompareFunc, NULL);
}

void CWZAdvanced::OnSelectAll()
{
  CString cs_text;
  int iItem;
  DWORD_PTR dw_data;

  int num_in_list = m_pLC_List->GetItemCount();
  if (num_in_list == 0)
    return;

  for (int i = 0; i < num_in_list; i++) {
    cs_text = m_pLC_List->GetItemText(i, 0);
    dw_data = m_pLC_List->GetItemData(i);
    iItem = m_pLC_Selected->InsertItem(0, cs_text);
    m_pLC_Selected->SetItemData(iItem, dw_data);
  }

  m_pLC_List->DeleteAllItems();
  m_pLC_Selected->SortItems(AdvCompareFunc, NULL);
}

void CWZAdvanced::OnDeselectSome()
{
  CString cs_text;
  int iItem;
  DWORD_PTR dw_data;

  int num_selected = m_pLC_Selected->GetItemCount();
  if (num_selected == 0)
    return;

  int nItem(-1);

  for (int i = 0; i < num_selected; i++) {
    nItem = m_pLC_Selected->GetNextItem(nItem, LVNI_SELECTED);
    if (nItem == -1)
      continue;

    cs_text = m_pLC_Selected->GetItemText(nItem, 0);
    dw_data = m_pLC_Selected->GetItemData(nItem);

    // Ignore mandatory fields - can't be deselected
    if (m_bsMandatoryFields.test(dw_data & 0xff))
      continue;

    m_pLC_Selected->DeleteItem(nItem);
    iItem = m_pLC_List->InsertItem(0, cs_text);
    m_pLC_List->SetItemData(iItem, dw_data);
  }

  m_pLC_List->SortItems(AdvCompareFunc, NULL);
  m_pLC_Selected->SortItems(AdvCompareFunc, NULL);
}

void CWZAdvanced::OnDeselectAll()
{
  CString cs_text;
  int iItem;
  DWORD_PTR dw_data;

  int num_selected = m_pLC_Selected->GetItemCount();
  if (num_selected == 0)
    return;

  for (int i = num_selected - 1; i >= 0; i--) {
    cs_text = m_pLC_Selected->GetItemText(i, 0);
    dw_data = m_pLC_Selected->GetItemData(i);
    // Ignore mandatory fields - can't be deselected
    if (m_bsMandatoryFields.test(dw_data & 0xff))
      continue;

    iItem = m_pLC_List->InsertItem(0, cs_text);
    m_pLC_List->SetItemData(iItem, dw_data);
    m_pLC_Selected->DeleteItem(i);
  }

  m_pLC_List->SortItems(AdvCompareFunc, NULL);
}

void CWZAdvanced::OnSelectedItemChanging(NMHDR *pNMHDR, LRESULT *pResult)
{
  // Prevent mandatory fields being deselected
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  if (m_bsMandatoryFields.test(pNMListView->lParam & 0xff) &&
      (pNMListView->uNewState & LVIS_SELECTED)) {
    pNMListView->uNewState &= ~LVIS_SELECTED;
    *pResult = TRUE;
  } else
    *pResult = FALSE;
}

// Override PreTranslateMessage() so RelayEvent() can be
// called to pass a mouse message to CPWSOptions's
// tooltip control for processing.
BOOL CWZAdvanced::PreTranslateMessage(MSG* pMsg)
{
  if (m_pToolTipCtrl != NULL)
    m_pToolTipCtrl->RelayEvent(pMsg);

  return CPropertyPage::PreTranslateMessage(pMsg);
}

int CALLBACK CWZAdvanced::AdvCompareFunc(LPARAM lParam1, LPARAM lParam2,
                                         LPARAM /* closure */)
{
  return (int)(lParam1 - lParam2);
}