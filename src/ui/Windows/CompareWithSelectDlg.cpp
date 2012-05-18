/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file CompareWithSelectDlg.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "CompareWithSelectDlg.h"
#include "Images.h"
#include "TreeUtils.h"

#include "core/PWScore.h"
#include "core/ItemData.h"

#include "os/UUID.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace TreeUtils;

CCompareWithSelectDlg::CCompareWithSelectDlg(CItemData *pci, PWScore *pcore,
      CWnd *pParent)
  : CPWDialog(CCompareWithSelectDlg::IDD, pParent),
  m_pci(pci), m_pcore(pcore), m_pSelected(NULL)
{
  ASSERT(pci != NULL && m_pcore != NULL);

  m_group = pci->GetGroup();
  m_title = pci->GetTitle();
  m_username = pci->GetUser();
}

CCompareWithSelectDlg::~CCompareWithSelectDlg()
{
}

void CCompareWithSelectDlg::DoDataExchange(CDataExchange *pDX)
{
  CPWDialog::DoDataExchange(pDX);

  DDX_Control(pDX, IDC_ITEMTREE, m_cwItemTree);
}

BEGIN_MESSAGE_MAP(CCompareWithSelectDlg, CPWDialog)
  ON_NOTIFY(NM_CLICK, IDC_ITEMTREE, OnItemSelected)
  ON_NOTIFY(NM_DBLCLK, IDC_ITEMTREE, OnItemDblClick)
END_MESSAGE_MAP()

void CCompareWithSelectDlg::OnDestroy()
{
  // Remove image list
  m_cwItemTree.SetImageList(NULL, TVSIL_NORMAL);
  m_cwItemTree.SetImageList(NULL, TVSIL_STATE);
}

BOOL CCompareWithSelectDlg::OnInitDialog()
{
  CPWDialog::OnInitDialog();

  CString cs_text;

  GetDlgItem(IDC_GROUP)->SetWindowText(m_group);
  GetDlgItem(IDC_TITLE)->SetWindowText(m_title);
  GetDlgItem(IDC_USERNAME)->SetWindowText(m_username);

  Images *pImages = Images::GetInstance();
  
  m_cwItemTree.SetImageList(pImages->GetImageList(), TVSIL_NORMAL);
  m_cwItemTree.SetImageList(pImages->GetImageList(), TVSIL_STATE);

  // Populate TREE or LIST views
  ItemListIter listPos;
  for (listPos = m_pcore->GetEntryIter(); listPos != m_pcore->GetEntryEndIter();
       listPos++) {
    CItemData &ci = m_pcore->GetEntry(listPos);
    // Don't add shortuts our ourselves
    if (ci.GetEntryType() != CItemData::ET_SHORTCUT &&
        m_pci->GetUUID() != ci.GetUUID())
      InsertItemIntoGUITree(ci);
  }

  m_cwItemTree.SortChildren(TVI_ROOT);
  
  // Disable OK button until an entry is selected
  GetDlgItem(IDOK)->EnableWindow(FALSE);
  return TRUE;
}

void CCompareWithSelectDlg::OnItemDblClick(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  OnItemSelected(pNotifyStruct, pLResult);

  if (m_pSelected != NULL)
    EndDialog(IDOK);
}

void CCompareWithSelectDlg::OnItemSelected(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  *pLResult = 0L;

  HTREEITEM hItem(NULL);

  // TreeView

  // Seems that under Vista with Windows Common Controls V6, it is ignoring
  // the single click on the button (+/-) of a node and only processing the
  // double click, which generates a copy of whatever the user selected
  // for a double click (except that it invalid for a node!) and then does
  // the expand/collapse as appropriate.
  // This codes attempts to fix this.
  switch (pNotifyStruct->code) {
    case NM_CLICK:
    {
      // Mouseclick - Need to find the item clicked via HitTest
      TVHITTESTINFO htinfo = {0};
      CPoint point = ::GetMessagePos();
      m_cwItemTree.ScreenToClient(&point);
      htinfo.pt = point;
      m_cwItemTree.HitTest(&htinfo);
      hItem = htinfo.hItem;

      // Ignore any clicks not on an item (group or entry)
      if (hItem == NULL ||
          htinfo.flags & (TVHT_NOWHERE | TVHT_ONITEMRIGHT |
                          TVHT_ABOVE   | TVHT_BELOW |
                          TVHT_TORIGHT | TVHT_TOLEFT)) {
          GetDlgItem(IDOK)->EnableWindow(FALSE);
          return;
      }

      // If a group
      if (!IsLeaf(&m_cwItemTree, hItem)) {
        // If on indent or button
        if (htinfo.flags & (TVHT_ONITEMINDENT | TVHT_ONITEMBUTTON)) {
          m_cwItemTree.Expand(htinfo.hItem, TVE_TOGGLE);
          *pLResult = 1L; // We have toggled the group
          GetDlgItem(IDOK)->EnableWindow(FALSE);
          return;
        }
      }
      break;
    }
    case TVN_SELCHANGED:
      // Keyboard - We are given the new selected entry
      hItem = ((NMTREEVIEW *)pNotifyStruct)->itemNew.hItem;
      break;
    default:
      // No idea how we got here!
      GetDlgItem(IDOK)->EnableWindow(FALSE);
      return;
  }

  // Check it was on an item
  if (hItem != NULL && IsLeaf(&m_cwItemTree, hItem)) {
    m_pSelected = (CItemData *)m_cwItemTree.GetItemData(hItem);
  }

  HTREEITEM hti = m_cwItemTree.GetDropHilightItem();
  if (hti != NULL)
    m_cwItemTree.SetItemState(hti, 0, TVIS_DROPHILITED);

  if (m_pSelected == NULL)
    return;

  // Can't select ourselves
  if (m_pSelected->GetGroup() == m_group && m_pSelected->GetTitle() == m_title &&
      m_pSelected->GetUser() == m_username) {
    // Unselect it
    m_cwItemTree.SetItemState(hItem, 0, LVIS_SELECTED);

    m_pSelected = NULL;
    GetDlgItem(IDOK)->EnableWindow(FALSE);
  } else {
    GetDlgItem(IDOK)->EnableWindow(TRUE);
  }
}

void CCompareWithSelectDlg::InsertItemIntoGUITree(CItemData &ci)
{
  HTREEITEM ti;
  StringX treeDispString = (LPCWSTR)m_cwItemTree.MakeTreeDisplayString(ci);
  // get path, create if necessary, add title as last node
  bool bAlreadyExists;
  ti = AddGroup(&m_cwItemTree, ci.GetGroup(), bAlreadyExists);
  ti = m_cwItemTree.InsertItem(treeDispString.c_str(), ti, TVI_SORT);

  m_cwItemTree.SetItemData(ti, (DWORD_PTR)&ci);
  
  int nImage = ci.GetEntryImage();
  m_cwItemTree.SetItemImage(ti, nImage, nImage);
}

pws_os::CUUID CCompareWithSelectDlg::GetUUID()
{
  if (m_pSelected == NULL)
    return pws_os::CUUID::NullUUID();
  else
    return m_pSelected->GetUUID();
}
