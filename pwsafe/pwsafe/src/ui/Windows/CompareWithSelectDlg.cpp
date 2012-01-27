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

#include "core/PWScore.h"
#include "core/ItemData.h"

#include "os/UUID.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCompareWithSelectDlg::CCompareWithSelectDlg(CItemData *pci, PWScore *pcore,
      bool bShowTree, CWnd *pParent)
  : CPWDialog(CCompareWithSelectDlg::IDD, pParent),
  m_pci(pci), m_pcore(pcore), m_pSelected(NULL), m_bShowTree(bShowTree), m_pImageList(NULL)
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

  DDX_Control(pDX, IDC_ITEMLIST, m_cwItemList);
  DDX_Control(pDX, IDC_ITEMTREE, m_cwItemTree);
}

BEGIN_MESSAGE_MAP(CCompareWithSelectDlg, CPWDialog)
  ON_WM_DESTROY()
  ON_NOTIFY(NM_CLICK, IDC_ITEMLIST, OnListItemSelected)
  ON_NOTIFY(NM_CLICK, IDC_ITEMTREE, OnTreeItemSelected)
END_MESSAGE_MAP()

void CCompareWithSelectDlg::OnDestroy()
{
  if (m_bShowTree) {
    // Remove image list
    m_cwItemTree.SetImageList(NULL, TVSIL_NORMAL);
    m_cwItemTree.SetImageList(NULL, TVSIL_STATE);

    m_pImageList->DeleteImageList();
    delete m_pImageList;
  }
}

BOOL CCompareWithSelectDlg::OnInitDialog()
{
  CPWDialog::OnInitDialog();

  CString cs_text;

  GetDlgItem(IDC_GROUP)->SetWindowText(m_group);
  GetDlgItem(IDC_TITLE)->SetWindowText(m_title);
  GetDlgItem(IDC_USERNAME)->SetWindowText(m_username);

  if (m_bShowTree) {
   // Init stuff for tree view
    CBitmap bitmap;
    BITMAP bm;

    // Change all pixels in this 'grey' to transparent
    const COLORREF crTransparent = RGB(192, 192, 192);

    bitmap.LoadBitmap(IDB_NODE);
    bitmap.GetBitmap(&bm);

    m_pImageList = new CImageList();
    // Number (12) corresponds to number in CCWTreeCtrl public enum
    BOOL status = m_pImageList->Create(bm.bmWidth, bm.bmHeight,
                                       ILC_MASK | ILC_COLORDDB,
                                       CCWTreeCtrl::NUM_IMAGES, 0);
    ASSERT(status != 0);

    // Order of LoadBitmap() calls matches CCWTreeCtrl public enum
    //bitmap.LoadBitmap(IDB_NODE); - already loaded above to get width
    m_pImageList->Add(&bitmap, crTransparent);
    bitmap.DeleteObject();
    UINT bitmapResIDs[] = {IDB_NODE,
      IDB_NORMAL, IDB_NORMAL_WARNEXPIRED, IDB_NORMAL_EXPIRED,
      IDB_ABASE, IDB_ABASE_WARNEXPIRED, IDB_ABASE_EXPIRED,
      IDB_ALIAS,
      IDB_SBASE, IDB_SBASE_WARNEXPIRED, IDB_SBASE_EXPIRED,
      IDB_SHORTCUT,
    };

    for (int i = 1; i < sizeof(bitmapResIDs) / sizeof(bitmapResIDs[0]); i++) {
      bitmap.LoadBitmap(bitmapResIDs[i]);
      m_pImageList->Add(&bitmap, crTransparent);
      bitmap.DeleteObject();
    }

    m_cwItemTree.SetImageList(m_pImageList, TVSIL_NORMAL);
    m_cwItemTree.SetImageList(m_pImageList, TVSIL_STATE);
  }

  // Insert List column if displaying list
  if (!m_bShowTree) {
    CString cs_text;
    cs_text.LoadString(IDS_GROUP);
    m_cwItemList.InsertColumn(0, cs_text);
    cs_text.LoadString(IDS_TITLE);
    m_cwItemList.InsertColumn(1, cs_text);
    cs_text.LoadString(IDS_USERNAME);
    m_cwItemList.InsertColumn(2, cs_text);
    GetDlgItem(IDC_ITEMTREE)->EnableWindow(FALSE);
    GetDlgItem(IDC_ITEMTREE)->ShowWindow(SW_HIDE);

    m_cwItemList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
  } else {
    GetDlgItem(IDC_ITEMLIST)->EnableWindow(FALSE);
    GetDlgItem(IDC_ITEMLIST)->ShowWindow(SW_HIDE);
  }

  // Populate Tree or List views
  ItemListIter listPos;
  for (listPos = m_pcore->GetEntryIter(); listPos != m_pcore->GetEntryEndIter();
       listPos++) {
    CItemData &ci = m_pcore->GetEntry(listPos);
    // Don't add shortuts our ourselves
    if (ci.GetEntryType() != CItemData::ET_SHORTCUT &&
        m_pci->GetUUID() != ci.GetUUID())
      InsertItemIntoGUITreeList(ci, -1);
  }

  if (m_bShowTree) {
    m_cwItemTree.SortChildren(TVI_ROOT);
  } else {
    m_cwItemList.SortItems(CompareFunc, CItemData::GROUP);
    m_cwItemList.SetColumnWidth(0, LVSCW_AUTOSIZE);
    m_cwItemList.SetColumnWidth(1, LVSCW_AUTOSIZE);
    m_cwItemList.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
  }

  // Disable OK button until an entry is selected
  GetDlgItem(IDOK)->EnableWindow(FALSE);
  return TRUE;
}

void CCompareWithSelectDlg::OnListItemSelected(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  OnItemSelected(pNotifyStruct, pLResult);
}

void CCompareWithSelectDlg::OnTreeItemSelected(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  OnItemSelected(pNotifyStruct, pLResult);
}

void CCompareWithSelectDlg::OnItemSelected(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  *pLResult = 0L;

  HTREEITEM hItem(NULL);
  int iItem(-1);

  if (m_bShowTree) {
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
        if (!m_cwItemTree.IsLeaf(hItem)) {
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
    if (hItem != NULL && m_cwItemTree.IsLeaf(hItem)) {
      m_pSelected = (CItemData *)m_cwItemTree.GetItemData(hItem);
    }

    HTREEITEM hti = m_cwItemTree.GetDropHilightItem();
    if (hti != NULL)
      m_cwItemTree.SetItemState(hti, 0, TVIS_DROPHILITED);
  } else {
    // ListView
    switch (pNotifyStruct->code) {
      case NM_CLICK:
      {
        NMITEMACTIVATE *pNMIA = reinterpret_cast<NMITEMACTIVATE *>(pNotifyStruct);
        LVHITTESTINFO htinfo = {0};
        CPoint point(pNMIA->ptAction);
        htinfo.pt = point;
        iItem = m_cwItemList.SubItemHitTest(&htinfo);

        // Ignore any clicks not on an item
        if (iItem == -1 ||
            htinfo.flags & (LVHT_NOWHERE |
                            LVHT_ABOVE   | LVHT_BELOW |
                            LVHT_TORIGHT | LVHT_TOLEFT)) {
            GetDlgItem(IDOK)->EnableWindow(FALSE);
            return;
        }
        break;
      }
      case LVN_KEYDOWN:
      {
        NMLVKEYDOWN *pNMLVKD = reinterpret_cast<NMLVKEYDOWN *>(pNotifyStruct);
        iItem = m_cwItemList.GetNextItem(-1, LVNI_SELECTED);
        int nCount = m_cwItemList.GetItemCount();
        if (pNMLVKD->wVKey == VK_DOWN)
          iItem = (iItem + 1) % nCount;
        if (pNMLVKD->wVKey == VK_UP)
          iItem = (iItem - 1 + nCount) % nCount;
        break;
      }
      default:
        // No idea how we got here!
        GetDlgItem(IDOK)->EnableWindow(FALSE);
        return;
    }
    if (iItem != -1) {
      // -1 if nothing selected, e.g., empty list
      m_pSelected = (CItemData *)m_cwItemList.GetItemData(iItem);
    }
  }

  if (m_pSelected == NULL)
    return;

  // Can't select ourselves
  if (m_pSelected->GetGroup() == m_group && m_pSelected->GetTitle() == m_title &&
      m_pSelected->GetUser() == m_username) {
    // Unselect it
    if (m_bShowTree)
      m_cwItemTree.SetItemState(hItem, 0, LVIS_SELECTED);
    else
      m_cwItemList.SetItemState(iItem, 0, LVIS_SELECTED);

    m_pSelected = NULL;
    GetDlgItem(IDOK)->EnableWindow(FALSE);
  } else {
    GetDlgItem(IDOK)->EnableWindow(TRUE);
  }
}

void CCompareWithSelectDlg::InsertItemIntoGUITreeList(CItemData &ci, int iIndex)
{
  int iResult = iIndex;
  if (iResult < 0) {
    iResult = m_cwItemList.GetItemCount();
  }

  if (!m_bShowTree) {
    // Insert the column data - group/title/username
    iResult = m_cwItemList.InsertItem(iResult, ci.GetGroup().c_str());
    m_cwItemList.SetItemText(iResult, 1, ci.GetTitle().c_str());
    m_cwItemList.SetItemText(iResult, 2, ci.GetUser().c_str());

    m_cwItemList.SetItemData(iResult, (DWORD_PTR)&ci);
  } else {
    HTREEITEM ti;
    StringX treeDispString = (LPCWSTR)m_cwItemTree.MakeTreeDisplayString(ci);
    // get path, create if necessary, add title as last node
    ti = m_cwItemTree.AddGroup(ci.GetGroup().c_str());
    ti = m_cwItemTree.InsertItem(treeDispString.c_str(), ti, TVI_SORT);

    m_cwItemTree.SetItemData(ti, (DWORD_PTR)&ci);

    int nImage = m_cwItemTree.GetEntryImage(ci);
    m_cwItemTree.SetItemImage(ti, nImage, nImage);
  }
}

pws_os::CUUID CCompareWithSelectDlg::GetUUID()
{
  if (m_pSelected == NULL)
    return pws_os::CUUID::NullUUID();
  else
    return m_pSelected->GetUUID();
}

int CALLBACK CCompareWithSelectDlg::CompareFunc(LPARAM lParam1, LPARAM lParam2,
                                                LPARAM lParmSort)
{
  // closure is "this" of the calling DboxMain, from which we use:
  // m_iTypeSortColumn to determine which column is getting sorted
  // which is by column data TYPE and NOT by column index!
  // m_bSortAscending to determine the direction of the sort (duh)

  int nTypeSortColumn = abs(lParmSort);
  int bSortAscending = lParmSort > 0;

  CItemData *pLHS = (CItemData *)lParam1;
  CItemData *pRHS = (CItemData *)lParam2;
  StringX group1, group2;

  int iResult;
  switch (nTypeSortColumn) {
    case CItemData::GROUP:
      group1 = pLHS->GetGroup();
      group2 = pRHS->GetGroup();
      if (group1.empty())  // root?
        group1 = L"\xff";
      if (group2.empty())  // root?
        group2 = L"\xff";
      iResult = CompareNoCase(group1, group2);
      if (iResult == 0) {
        iResult = CompareNoCase(pLHS->GetTitle(), pRHS->GetTitle());
        if (iResult == 0) {
          iResult = CompareNoCase(pLHS->GetUser(), pRHS->GetUser());
        }
      }
      break;
    case CItemData::TITLE:
      iResult = CompareNoCase(pLHS->GetTitle(), pRHS->GetTitle());
      if (iResult == 0) {
        iResult = CompareNoCase(pLHS->GetUser(), pRHS->GetUser());
      }
      break;
    case CItemData::USER:
      iResult = CompareNoCase(pLHS->GetUser(), pRHS->GetUser());
      if (iResult == 0) {
        iResult = CompareNoCase(pLHS->GetTitle(), pRHS->GetTitle());
      }
      break;
    default:
      iResult = 0; // should never happen - just keep compiler happy
      ASSERT(FALSE);
  }

  if (!bSortAscending) {
    iResult *= -1;
  }

  return iResult;
}