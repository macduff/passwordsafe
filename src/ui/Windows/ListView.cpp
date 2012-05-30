/*
 *Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
 *All rights reserved. Use of the code is allowed under the
 *Artistic License 2.0 terms, as specified in the LICENSE file
 *distributed with this code, or available from
 *http://www.opensource.org/licenses/artistic-license-2.0.php
*/

// ListView.cpp : implementation of the CPWListView class
//

#include "stdafx.h"

#include "WindowsDefs.h"

#include "ListView.h"
#include "TreeView.h"
#include "DboxMain.h"
#include "DDSupport.h"
#include "Images.h"
#include "TreeUtils.h"
#include "ThisMfcApp.h"
#include "GeneralMsgBox.h"

#include "core/PWSprefs.h"
#include "core/ItemData.h"

#include "os/UUID.h"

#include "resource.h"
#include "resource3.h"

#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using pws_os::CUUID;
using namespace TreeUtils;
using namespace std;

extern wchar_t GROUP_SEP;
extern StringX sxDot;

BEGIN_MESSAGE_MAP(CLVEdit, CEdit)
  //{{AFX_MSG_MAP(CLVEdit)
  ON_WM_CHAR()
  ON_WM_NCDESTROY()
  ON_WM_KILLFOCUS()
  ON_WM_CREATE()
  ON_WM_WINDOWPOSCHANGED()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CLVEdit::CLVEdit(CWnd *pParent, int nItem, int nSubItem, CSecString csLabel)
  : m_x(0), m_y(0), m_nItem(nItem), m_nSubItem(nSubItem),
  m_csLabel(csLabel), m_bVK_ESCAPE(false)
{
  m_pPWListView = static_cast<CPWListView *>(pParent);
  m_pDbx = static_cast<DboxMain *>(m_pPWListView->GetParent());
}

void CLVEdit::OnWindowPosChanged(WINDOWPOS *lpwndpos)
{
  lpwndpos->x = m_x;

  CEdit::OnWindowPosChanging(lpwndpos);
}

BOOL CLVEdit::PreTranslateMessage(MSG *pMsg)
{
  if (pMsg->message == WM_KEYDOWN &&
      (pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN)) {
    if (pMsg->wParam == VK_ESCAPE)
      m_bVK_ESCAPE = true;

    m_pPWListView->SetFocus();
    return TRUE;
  }

  return CEdit::PreTranslateMessage(pMsg);
}

void CLVEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  CEdit::OnChar(nChar, nRepCnt, nFlags);

  // Resize edit control if needed
  // Get text extent
  CString str;
  GetWindowText(str);

  CWindowDC dc(this);
  CFont *pFont = GetParent()->GetFont();
  CFont *pFontDC = dc.SelectObject(pFont);
  CSize size = dc.GetTextExtent(str);

  dc.SelectObject(pFontDC);
  size.cx += 5; // add some extra buffer

  // Get client rect
  CRect rect, rcParent;
  GetClientRect(&rect);
  m_pPWListView->GetClientRect(&rcParent);

  // Transform rect to parent coordinates
  ClientToScreen(&rect);
  m_pPWListView->ScreenToClient(&rect);

  // Check whether control needs to be resized
  // and whether there is space to grow
  if (size.cx > rect.Width())  {
    if (size.cx + rect.left < rcParent.right)
      rect.right = rect.left + size.cx;
    else
      rect.right = rcParent.right;
    MoveWindow(&rect);
  }
}

void CLVEdit::OnNcDestroy()
{
  CEdit::OnNcDestroy();

  delete this;
}

void CLVEdit::OnKillFocus(CWnd* pNewWnd)
{
  CEdit::OnKillFocus(pNewWnd);

  CString str;
  GetWindowText(str);

  // Send Notification to parent of ListView ctrl
  LV_DISPINFO lvDispInfo;
  lvDispInfo.hdr.hwndFrom = GetParent()->m_hWnd;
  lvDispInfo.hdr.idFrom = GetDlgCtrlID();
  lvDispInfo.hdr.code = LVN_ENDLABELEDIT;
  lvDispInfo.item.mask = LVIF_TEXT;
  lvDispInfo.item.iItem = m_nItem;
  lvDispInfo.item.iSubItem = m_nSubItem;
  lvDispInfo.item.pszText = m_bVK_ESCAPE ? NULL : LPTSTR((LPCTSTR)str);
  lvDispInfo.item.cchTextMax = str.GetLength();

  m_pDbx->SendMessage(WM_NOTIFY, m_pPWListView->GetDlgCtrlID(), (LPARAM)&lvDispInfo);

  DestroyWindow();
}

int CLVEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

  if (CEdit::OnCreate(lpCreateStruct) == -1)
    return -1;

  CFont *font = m_pPWListView->GetFont();
  SetFont(font);
  SetWindowText(m_csLabel);
  SetFocus();
  SetSel(0, -1);
  return 0;
}

// CPWListView

IMPLEMENT_DYNCREATE(CPWListView, CListView)

BEGIN_MESSAGE_MAP(CPWListView, CListView)
  //{{AFX_MSG_MAP(CPWListView)
  ON_WM_CREATE()
  ON_WM_DESTROY()
  ON_WM_CONTEXTMENU()
  ON_WM_SETFOCUS()
  ON_WM_SIZE()
  ON_WM_VSCROLL()

  ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndLabelEdit)
  ON_NOTIFY(HDN_ITEMCLICK, 0, OnHeaderClicked)
  ON_NOTIFY_REFLECT(NM_DBLCLK, OnItemDoubleClick)
  ON_NOTIFY_REFLECT(NM_CLICK, OnItemClick)
  ON_COMMAND(ID_MENUITEM_RENAME, OnRename)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPWListView construction/destruction

CPWListView::CPWListView()
  : m_pParent(NULL), m_pListCtrl(NULL), m_pTreeView(NULL), m_pDbx(NULL),
  m_pOtherListView(NULL), m_pOtherListCtrl(NULL),
  m_iSortedColumn(1), m_bSortAscending(true), m_bInitDone(false),
  m_bAccEn(false), m_bInRename(false), m_this_row(INVALID_ROW),
  m_bDisplayingFoundEntries(false)
{
  // m_sxCurrentPath must not be empty at start-up - otherwise root not added to history
  m_sxCurrentPath = L"\xff";
}

CPWListView::~CPWListView()
{
}

BOOL CPWListView::PreCreateWindow(CREATESTRUCT &cs)
{
  return CListView::PreCreateWindow(cs);
}

BOOL CPWListView::PreTranslateMessage(MSG *pMsg)
{
  // When an item is being edited make sure the edit control
  // receives certain important key strokes
  if (m_pListCtrl->GetEditControl()) {
    ::TranslateMessage(pMsg);
    ::DispatchMessage(pMsg);
    return TRUE; // DO NOT process further
  }

  if (!m_bInRename) {
    // Process User's AutoType shortcut
    if (m_pDbx != NULL && m_pDbx->CheckPreTranslateAutoType(pMsg))
      return TRUE;

    // Process User's Delete shortcut
    if (m_pDbx != NULL && m_pDbx->CheckPreTranslateDelete(pMsg))
      return TRUE;

    // Process user's Rename shortcut
    if (m_pDbx != NULL && m_pDbx->CheckPreTranslateRename(pMsg)) {
      //  Send via main window to ensure it isn't an Edit in place
      PostMessage(WM_COMMAND, ID_MENUITEM_RENAME);
      return TRUE;
    }
  }

  // Check whether its enter key on a selected item to cause it to expand/collapse
  if (m_pListCtrl != NULL && m_pListCtrl->GetSelectedCount() == 1 &&
      pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
    // Check whether the currently selected item is
    POSITION pos = m_pListCtrl->GetFirstSelectedItemPosition();
    if (pos != NULL) {
      int iIndex = m_pListCtrl->GetNextSelectedItem(pos);
      st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(iIndex);
      ASSERT(pLP != NULL);
      if (pLP->pci == NULL) {
        // It is a group - get partner to do the work
        StringX sxEntry = m_pListCtrl->GetItemText(iIndex, 1);
        m_pTreeView->SetFocus();
        m_pTreeView->OnListViewFolderSelected(sxEntry, iIndex, true);
        m_pListCtrl->SetFocus();
        return TRUE;
      }
    }
  }

  // Let the parent class do its thing
  return CListView::PreTranslateMessage(pMsg);
}

void CPWListView::Initialize()
{
  m_pListCtrl = &GetListCtrl();

  m_pListCtrl->ModifyStyle(NULL, LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS);

  DWORD dwExStyle = m_pListCtrl->GetExtendedStyle();
  dwExStyle |= LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES;
  m_pListCtrl->SetExtendedStyle(dwExStyle);

  Images *pImages = Images::GetInstance();

  CImageList *pil = pImages->GetImageList();
  ASSERT(pil);

  m_pListCtrl->SetImageList(pil, LVSIL_NORMAL);
  m_pListCtrl->SetImageList(pil, LVSIL_SMALL);

  SetUpHeader();

  m_bInitDone = true;
}

void CPWListView::SetUpHeader()
{
  CHeaderCtrl *pHeaderCtrl = m_pListCtrl->GetHeaderCtrl();
  CString cs_header(L" ");
  HDITEM hdi;
  hdi.mask = HDI_LPARAM;
  
  const int num_columns = pHeaderCtrl->GetItemCount();

  if (m_bDisplayingFoundEntries) {
    // 3 columns - image, group, title[username]
    
    if (num_columns == 3)
      return;
      
    for (int i = 0; i < num_columns; i++) {
      pHeaderCtrl->DeleteItem(0);
    }
    
    //cs_header.LoadString(IDS_ICON);
    m_pListCtrl->InsertColumn(0, cs_header);
    hdi.lParam = CItemData::UUID;
    pHeaderCtrl->SetItem(0, &hdi);

    cs_header.LoadString(IDS_GROUP);
    m_pListCtrl->InsertColumn(1, cs_header);
    hdi.lParam = CItemData::GROUP;
    pHeaderCtrl->SetItem(1, &hdi);
    
    cs_header.LoadString(IDS_TITLEUSER);
    m_pListCtrl->InsertColumn(2, cs_header);
    hdi.lParam = CItemData::TITLE;
    pHeaderCtrl->SetItem(2, &hdi);

    m_pListCtrl->SetColumnWidth(0, LVSCW_AUTOSIZE);
    m_pListCtrl->SetColumnWidth(1, LVSCW_AUTOSIZE);
    m_pListCtrl->SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
  } else {
    // 2 columns - image group/title[username]
    
    if (num_columns == 2)
      return;
      
    for (int i = 0; i < num_columns; i++) {
      pHeaderCtrl->DeleteItem(0);
    }

    // None at all - add them
    //cs_header.LoadString(IDS_ICON);
    m_pListCtrl->InsertColumn(0, cs_header);
    hdi.lParam = CItemData::UUID;
    pHeaderCtrl->SetItem(0, &hdi);
      
    cs_header.LoadString(IDS_GROUPTITLEUSER);
    m_pListCtrl->InsertColumn(1, cs_header);
    hdi.lParam = CItemData::TITLE;
    pHeaderCtrl->SetItem(1, &hdi);

    m_pListCtrl->SetColumnWidth(0, LVSCW_AUTOSIZE);
    m_pListCtrl->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
  }
}

void CPWListView::SetOtherView(const SplitterRow &i_this_row, CPWListView *pOtherListView)
{
  // Set what row we are and info on the other row
  m_this_row = i_this_row;

  m_pOtherListView = pOtherListView;
  m_pOtherListCtrl = &pOtherListView->GetListCtrl();
}

// CPWListView message handlers

int CPWListView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CListView::OnCreate(lpCreateStruct) == -1)
    return -1;

  if (!m_bInitDone)
    Initialize();

  m_pParent = (CPWSplitterWnd *)GetParent();

  return 0;
}

void CPWListView::OnDestroy()
{
  // Remove image list
  m_pListCtrl->SetImageList(NULL, LVSIL_NORMAL);
  m_pListCtrl->SetImageList(NULL, LVSIL_SMALL);
}

void CPWListView::OnContextMenu(CWnd *pWnd, CPoint screen)
{
  m_pDbx->PostMessage(WM_CONTEXTMENU, (WPARAM)pWnd->m_hWnd,
                      MAKELPARAM(screen.x, screen.y));
}

void CPWListView::OnSize(UINT nType, int cx, int cy)
{
  // stop editing if resizing
  if (GetFocus() != this )
    SetFocus();

  CListView::OnSize(nType, cx, cy);
}

void CPWListView::OnSetFocus(CWnd *pOldWnd)
{
  CListView::OnSetFocus(pOldWnd);
  if (m_pDbx == NULL)
    return;

  m_pDbx->SetCurrentRow(m_this_row);
  m_pDbx->UpdateCurrentPath(m_sxCurrentPath);
}

void CPWListView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
  if (GetFocus() != this)
    SetFocus();

  CListView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CPWListView::OnRename()
{
  // Can't rename if R-O or if more than one selected
  if (m_pDbx->IsDBReadOnly() && m_pListCtrl->GetSelectedCount() != 1)
    return;

  // Get selected item
  POSITION pos = m_pListCtrl->GetFirstSelectedItemPosition();
  if (pos != NULL)
    m_edit_item = m_pListCtrl->GetNextSelectedItem(pos);
  else
    return;

  if (m_edit_item == -1)
    return;

  // Don't allow rename of protect entries
  st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(m_edit_item);
  if (pLP->pci != NULL && pLP->pci->IsProtected())
    return;

  // Don't allow if can't make visible for editing
  if (!m_pListCtrl->EnsureVisible(m_edit_item, TRUE))
   return;

  m_bInRename = true;

  // Disable accelerators that would interfere with editing
  m_bAccEn = app.IsAcceleratorEnabled();
  if (m_bAccEn)
    app.DisableAccelerator();

  CRect rect;
  int offset = 0;
  m_pListCtrl->GetSubItemRect(m_edit_item, 1, LVIR_BOUNDS, rect);

  // Now scroll if we need to expose the column
  CRect rcClient;
  GetClientRect(rcClient);
  if (offset + rect.left < 0 || offset + rect.left > rcClient.right) {
    CSize size(offset + rect.left,0);
    m_pListCtrl->Scroll(size);
    rect.left -= size.cx;
  }
  rect.left += offset;
  rect.right = rect.left + m_pListCtrl->GetColumnWidth(1);
  if (rect.right > rcClient.right)
    rect.right = rcClient.right;

  // In case we have to revert:
  m_sxOldText = StringX(m_pListCtrl->GetItemText(m_edit_item, 1));

  CRect subrect;
  m_pListCtrl->GetSubItemRect(m_edit_item, 1,
                                LVIR_BOUNDS, subrect);

  DWORD dwStyle = ES_LEFT | WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL;
  CLVEdit *pLVEdit = new CLVEdit(this, m_edit_item, 1, m_sxOldText);

  pLVEdit->Create(dwStyle, rect, this, IDC_LISTVIEW_EDITCTRL);

  // Move edit control text 1 pixel to the right of org label,
  // as Windows does it...
  pLVEdit->m_x = subrect.left + 6;

  // Hide subitem text so it don't show if we delete some
  // text in the edit control
  // OnPaint handles other issues also regarding this
  m_pListCtrl->GetSubItemRect(m_edit_item, 1, LVIR_LABEL, rect);
  CDC *pDC = GetDC();
  CBrush br = ::GetSysColor(COLOR_WINDOW);
  pDC->FillRect(rect, &br);
  ReleaseDC(pDC);

  m_bEditLabelCompleted = false;
}

bool CPWListView::IsLeaf(int iIndex) const
{
  if (iIndex < 0 || iIndex > m_pListCtrl->GetItemCount() - 1)
    return false;

  st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(iIndex);
  return (pLP->pci != NULL);
}

void CPWListView::ClearEntries()
{
  m_pListCtrl->SetRedraw(FALSE);

  const int nCount = m_pListCtrl->GetItemCount();
  for (int i = 0; i < nCount; i++) {
    st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(0);
    delete pLP;
    m_pListCtrl->DeleteItem(0);
  }

  m_pListCtrl->SetRedraw(TRUE);
  UpdateWindow();
}

void CPWListView::DisplayEntries(const StringX sxRealPath, const bool bUpdateHistory)
{
  pws_os::Trace(L"CPWListView::DisplayEntries: sxRealPath=%s\n", sxRealPath.c_str());

  m_bDisplayingFoundEntries = false;
  SetUpHeader();

  m_pListCtrl->SetRedraw(FALSE);

  // Delete all previous entries and associated ItemData
  const int nCount = m_pListCtrl->GetItemCount();
  for (int i = 0; i < nCount; i++) {
    st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(0);
    delete pLP;
    m_pListCtrl->DeleteItem(0);
  }

  // Get all subgroups of this group
  PathMapIter iter;
  PathMap mapGroups2Item;
  m_pTreeView->GetGroupMap(sxRealPath, mapGroups2Item);

  int iIndex = 0;
  StringX sxRealPathDot = sxRealPath + sxDot;
  size_t len = sxRealPath.length();
  size_t len_root = m_pDbx->GetRoot().length();

  // Add only the immediate groups of this group
  for (iter = mapGroups2Item.begin(); iter != mapGroups2Item.end(); iter++) {
    // Ignore root (database name)
    if (iter->first.length() == len_root)
      continue;

    StringX sxNode = iter->first.substr(len_root + 1);
    if (len != 0) {
      if (sxNode.substr(0, len + 1) != sxRealPathDot ||
          sxNode == sxRealPath) {
        continue;
      }
      sxNode = sxNode.substr(len + 1);
    }

    // If this is a lower group - ignore
    if (sxNode.find_first_of(L'.', 1) != StringX::npos)
      continue;

    // Insert the group
    StringX sxGroup;
    if (len != 0)
      sxGroup = sxRealPath + sxDot + sxNode;
    else
      sxGroup = sxNode;

    const bool bIsEmpty = m_pDbx->IsEmptyGroup(sxGroup);
    iIndex = m_pListCtrl->InsertItem(++iIndex, NULL,
                      bIsEmpty ? CItemData::EI_EMPTY_GROUP : CItemData::EI_GROUP);
    ASSERT(iIndex != -1);
    m_pListCtrl->SetItemText(iIndex, 1, sxNode.c_str());

    // Get new ItemData
    st_PWLV_lParam *pLP = new st_PWLV_lParam;
    pLP->hItem = iter->second;
    pLP->pci = NULL;

    // Set it
    m_pListCtrl->SetItemData(iIndex, (DWORD_PTR)pLP);
  }

  // Now get this groups' entries
  vector<CUUID> vGroupEntries;
  m_pDbx->GetGroupEntries(sxRealPath, &vGroupEntries);

  HTREEITEM hGroup(NULL);
  if (!sxRealPath.empty()) {
    iter = mapGroups2Item.find(m_pDbx->GetRoot() + sxDot + sxRealPath);
    if (iter != mapGroups2Item.end())
      hGroup = iter->second;
    else
      ASSERT(0);
  } else
    hGroup = m_pTreeView->GetTreeCtrl().GetRootItem();

  // Show all of the children of hStartItem
  for (size_t i = 0; i < vGroupEntries.size(); i++) {
    ItemListIter iter = m_pDbx->Find(vGroupEntries[i]);
    ASSERT(iter != m_pDbx->End());

    CItemData *pci = &iter->second;

    // Shouldn't be a group!
    if (pci == NULL)
      continue;

    // First column is an image
    int nImage = pci->GetEntryImage();
    iIndex = m_pListCtrl->InsertItem(++iIndex, NULL, nImage);
    ASSERT(iIndex != -1);

    // Since these are all entries, pci is not NULL, so no need to worry about getting
    // details from entry
    StringX sxTitleUser;
    Format(sxTitleUser,L"%s [%s]", pci->GetTitle().c_str(), pci->GetUser().c_str());

    if (pci->IsProtected())
      sxTitleUser += L" #";

    m_pListCtrl->SetItemText(iIndex, 1, sxTitleUser.c_str());


    st_PWLV_lParam *pLP = new st_PWLV_lParam;
    pLP->hItem = hGroup; // Add HTREEITEM of this entries group in the TreeView
    pLP->pci = pci;      // Save pointer to entry's data

    m_pListCtrl->SetItemData(iIndex, (DWORD_PTR)pLP);
  }

  m_pListCtrl->SetColumnWidth(0, LVSCW_AUTOSIZE);
  m_pListCtrl->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);

  // Now sort it
  m_pListCtrl->SortItemsEx(CompareFunction, (DWORD_PTR)this);

  m_pListCtrl->SetRedraw(TRUE);

  if (bUpdateHistory && m_sxCurrentPath != sxRealPath) {
    m_pDbx->UpdateBackwardsForwards(m_this_row, sxRealPath);
    m_pDbx->UpdateStatusBar();
  }
  m_sxCurrentPath = sxRealPath;
  m_pDbx->UpdateCurrentPath(m_sxCurrentPath);

  UpdateWindow();
}

void CPWListView::DisplayFoundEntries(const vector<CUUID> &vFoundEntries)
{
  m_bDisplayingFoundEntries = true;
  SetUpHeader();

  m_pListCtrl->SetRedraw(FALSE);

  // Delete all previous entries and associated ItemData
  const int nCount = m_pListCtrl->GetItemCount();
  for (int i = 0; i < nCount; i++) {
    st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(0);
    delete pLP;
    m_pListCtrl->DeleteItem(0);
  }

  int iIndex(0);
  for (size_t i = 0; i < vFoundEntries.size(); i++) {
    ItemListIter iter = m_pDbx->Find(vFoundEntries[i]);
    ASSERT(iter != m_pDbx->End());

    CItemData *pci = &iter->second;

    // Shouldn't be a group!
    if (pci == NULL)
      continue;

    // First column is an image
    int nImage = pci->GetEntryImage();
    iIndex = m_pListCtrl->InsertItem(++iIndex, NULL, nImage);
    ASSERT(iIndex != -1);

    // Since these are all entries, pci is not NULL, so no need to worry about getting
    // details from entry
    StringX sxGroup, sxTitleUser;
    sxGroup = pci->GetGroup();
    Format(sxTitleUser,L"%s [%s]", pci->GetTitle().c_str(), pci->GetUser().c_str());

    if (pci->IsProtected())
      sxTitleUser += L" #";

    m_pListCtrl->SetItemText(iIndex, 1, sxGroup.c_str());
    m_pListCtrl->SetItemText(iIndex, 2, sxTitleUser.c_str());


    st_PWLV_lParam *pLP = new st_PWLV_lParam;
    pLP->hItem = NULL;
    pLP->pci = pci;      // Save pointer to entry's data

    m_pListCtrl->SetItemData(iIndex, (DWORD_PTR)pLP);
  }

  m_pListCtrl->SetColumnWidth(0, LVSCW_AUTOSIZE);
  m_pListCtrl->SetColumnWidth(1, LVSCW_AUTOSIZE);
  m_pListCtrl->SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);

  // Now sort it
  // Initial sort
  m_iSortedColumn = -1;
  m_pListCtrl->SortItemsEx(CompareFoundEntriesFunction, (DWORD_PTR)this);
  m_iSortedColumn = 1;

  m_pListCtrl->SetRedraw(TRUE);
  
  StringX sxSearch;
  LoadAString(sxSearch, IDS_SEARCH);
  Remove(sxSearch, L'&');
  m_pDbx->UpdateBackwardsForwards(m_this_row, sxSearch);
  m_pDbx->UpdateStatusBar();
}

BOOL CPWListView::SelectItem(const pws_os::CUUID &uuid, const BOOL &MakeVisible)
{
  // Find entry with this CUUID
  const int num_displayed = m_pListCtrl->GetItemCount();
  bool bFound(false);
  int iIndex;

  for (iIndex = 0; iIndex < num_displayed; iIndex++) {
    st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(iIndex);
    if (pLP == NULL || pLP->pci == NULL)
      continue;
    
    if (pLP->pci->GetUUID() == uuid) {
      bFound = true;
      break;
    }
  }

  // Not found????
  if (!bFound)
    return FALSE;

  // Now select it
  BOOL retval = m_pListCtrl->SetItemState(iIndex,
                                          LVIS_FOCUSED | LVIS_SELECTED,
                                          LVIS_FOCUSED | LVIS_SELECTED);

  if (MakeVisible) {
    m_pListCtrl->EnsureVisible(iIndex, FALSE);
  }
  m_pListCtrl->Invalidate();
  
  return retval;
}

#if (WINVER < 0x0501)  // These are already defined for WinXP and later
#define HDF_SORTUP             0x0400
#define HDF_SORTDOWN           0x0200
#endif

void CPWListView::OnHeaderClicked(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  HD_NOTIFY *phdn = reinterpret_cast<HD_NOTIFY *>(pNotifyStruct);

  if (phdn->iButton == 0) {
    // User clicked on header using left mouse button
    if (phdn->iItem == m_iSortedColumn)
      m_bSortAscending = !m_bSortAscending;
    else
      m_bSortAscending = true;

    m_iSortedColumn = phdn->iItem;
    if (m_bDisplayingFoundEntries)
      m_pListCtrl->SortItemsEx(CompareFoundEntriesFunction, (DWORD_PTR)this);
    else
      m_pListCtrl->SortItemsEx(CompareFunction, (DWORD_PTR)this);

    HDITEM HeaderItem;
    HeaderItem.mask = HDI_FORMAT;
    m_pListCtrl->GetHeaderCtrl()->GetItem(m_iSortedColumn, &HeaderItem);
    // Turn off all arrows
    HeaderItem.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
    // Turn on the correct arrow
    HeaderItem.fmt |= (m_bSortAscending ? HDF_SORTUP : HDF_SORTDOWN);
    m_pListCtrl->GetHeaderCtrl()->SetItem(m_iSortedColumn, &HeaderItem);
  }

 *pLResult = 0;
}

int CALLBACK CPWListView::CompareFunction(LPARAM lParam1, LPARAM lParam2,
                                          LPARAM lParamSort)
{
  // Called by SortItemsEx - the 'Ex' means that lParam1 & lParam2 are the
  // current indices of the entries being compared in this ListCtrl and
  // NOT the ItemData for these entries!
  CPWListView *self = (CPWListView *)lParamSort;
  CListCtrl &myListCtrl = self->GetListCtrl();
  int nSortColumn = self->m_iSortedColumn;
  const int iLHS = static_cast<int>(lParam1);
  const int iRHS = static_cast<int>(lParam2);

  st_PWLV_lParam *pLP1 = (st_PWLV_lParam *)myListCtrl.GetItemData(iLHS);
  st_PWLV_lParam *pLP2 = (st_PWLV_lParam *)myListCtrl.GetItemData(iRHS);
  ASSERT(pLP1 != NULL && pLP2 != NULL);
  CItemData *pLHS_PCI = pLP1->pci;
  CItemData *pRHS_PCI = pLP2->pci;

  // Note: Column 0 = Image, 1 = Group or Title [user]
  int iResult(0);
  switch (nSortColumn) {
    case 0: // Images - Groups first!
      if (pLHS_PCI == NULL && pRHS_PCI == NULL) {
        // Both groups
        StringX sxText1 = myListCtrl.GetItemText(iLHS, 1);
        StringX sxText2 = myListCtrl.GetItemText(iRHS, 1);
        iResult = CompareNoCase(sxText1, sxText2);
      } else
      if (pLHS_PCI != NULL && pRHS_PCI != NULL) {
        // Both entries
        if (pLHS_PCI->GetEntryType() != pRHS_PCI->GetEntryType())
          iResult = (pLHS_PCI->GetEntryType() < pRHS_PCI->GetEntryType()) ? -1 : 1;
      } else
      if (pLHS_PCI == NULL && pRHS_PCI != NULL) {
        // Only left is a group - groups first
        iResult = -1;
      } else
      if (pLHS_PCI != NULL && pRHS_PCI == NULL) {
        // Only right is a group - groups first
        iResult = 1;
      }
      break;
    case 1: // Text
      if (pLHS_PCI == NULL && pRHS_PCI == NULL) {
        // Both groups
        StringX sxText1 = myListCtrl.GetItemText(iLHS, 1);
        StringX sxText2 = myListCtrl.GetItemText(iRHS, 1);
        iResult = CompareNoCase(sxText1, sxText2);
      } else
      if (pLHS_PCI != NULL && pRHS_PCI != NULL) {
        // Both entries
        iResult = CompareNoCase(pLHS_PCI->GetTitle(), pRHS_PCI->GetTitle());
        if (iResult == 0) {
          iResult = CompareNoCase(pLHS_PCI->GetUser(), pRHS_PCI->GetUser());
        }
      } else
      if (pLHS_PCI == NULL && pRHS_PCI != NULL) {
       // Only left is a group - groups first
        iResult = -1;
      } else
      if (pLHS_PCI != NULL && pRHS_PCI == NULL) {
        // Only right is a group - groups first
        iResult = 1;
      }
      break;
    default:
      ASSERT(0);
  }

  if (!self->m_bSortAscending) {
    iResult *= -1;
  }

  return iResult;
}

int CALLBACK CPWListView::CompareFoundEntriesFunction(LPARAM lParam1, LPARAM lParam2,
                                                      LPARAM lParamSort)
{
  // Called by SortItemsEx - the 'Ex' means that lParam1 & lParam2 are the
  // current indices of the entries being compared in this ListCtrl and
  // NOT the ItemData for these entries!
  CPWListView *self = (CPWListView *)lParamSort;
  CListCtrl &myListCtrl = self->GetListCtrl();
  int nSortColumn = self->m_iSortedColumn;
  const int iLHS = static_cast<int>(lParam1);
  const int iRHS = static_cast<int>(lParam2);

  st_PWLV_lParam *pLP1 = (st_PWLV_lParam *)myListCtrl.GetItemData(iLHS);
  st_PWLV_lParam *pLP2 = (st_PWLV_lParam *)myListCtrl.GetItemData(iRHS);
  ASSERT(pLP1 != NULL && pLP2 != NULL);
  CItemData *pLHS_PCI = pLP1->pci;
  CItemData *pRHS_PCI = pLP2->pci;

  // Note: Column 0 = Image, 1 = Group, 2 = Title [user]
  int iResult(0);
  switch (nSortColumn) {
    case -1:
    {
      // Initial sort - Groups
      // Non-root entries first
      StringX sxText1 = myListCtrl.GetItemText(iLHS, 1);
      StringX sxText2 = myListCtrl.GetItemText(iRHS, 1);
      if (sxText1.empty() && !sxText2.empty())
        iResult = 1;
      else
      if (sxText2.empty() && !sxText1.empty())
        iResult = -1;
      else
        iResult = CompareNoCase(sxText1, sxText2);

      // Then Title
      if (iResult == 0)
        iResult = CompareNoCase(pLHS_PCI->GetTitle(), pRHS_PCI->GetTitle());

      // Then Username
      if (iResult == 0)
        iResult = CompareNoCase(pLHS_PCI->GetUser(), pRHS_PCI->GetUser());
      break;
    }
    case 0: // Images - no Groups!
      // Both entries
      if (pLHS_PCI->GetEntryType() != pRHS_PCI->GetEntryType())
        iResult = (pLHS_PCI->GetEntryType() < pRHS_PCI->GetEntryType()) ? -1 : 1;
      break;
    case 1: // Groups
    {
      // Non-root groups first
      StringX sxText1 = myListCtrl.GetItemText(iLHS, 1);
      StringX sxText2 = myListCtrl.GetItemText(iRHS, 1);
      if (sxText1.empty() && !sxText2.empty())
        iResult = 1;
      else
      if (sxText2.empty() && !sxText1.empty())
        iResult = -1;
      else
        iResult = CompareCase(sxText1, sxText2);
      break;
    }
    case 2: // Title [User]
      // Both entries
      iResult = CompareNoCase(pLHS_PCI->GetTitle(), pRHS_PCI->GetTitle());
      if (iResult == 0)
        iResult = CompareNoCase(pLHS_PCI->GetUser(), pRHS_PCI->GetUser());
      break;
    default:
      ASSERT(0);
  }

  if (!self->m_bSortAscending) {
    iResult *= -1;
  }

  return iResult;
}

void CPWListView::OnItemClick(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
 *pLResult = 0L;

 NMLISTVIEW *pNMListView = (NMLISTVIEW *)pNotifyStruct;

 if (pNMListView->iItem != -1)
   m_pListCtrl->SetItemState(pNMListView->iItem,
                             LVIS_FOCUSED | LVIS_SELECTED,
                             LVIS_FOCUSED | LVIS_SELECTED);

  // Ignore click if multiple entries selected (LIST view only)
  if ( m_pListCtrl->GetSelectedCount() != 1)
    return;

  // Now set we have processed the event
 *pLResult = 1L;

  st_PWLV_lParam *pLP(NULL);
  int iIndex(-1);
  POSITION pos = m_pListCtrl->GetFirstSelectedItemPosition();
  if (pos) {
    iIndex = m_pListCtrl->GetNextSelectedItem(pos);
    pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(iIndex);
    ASSERT(pLP != NULL);
  }

  if (iIndex == -1)
    return;

  if (pLP->pci != NULL) {
    m_pDbx->UpdateToolBarForSelectedItem(pLP->pci);
    m_pDbx->SetDCAText(pLP->pci);
  }
}

void CPWListView::DeleteEntries()
{
  // Don't do anything if we are the top row - we shouldn't be called
  if (m_this_row == TOP)
    return;

  ClearEntries();
}

void CPWListView::PopulateEntries()
{
  // Don't do anything if we are the top row - we shouldn't be called
  if (m_this_row == TOP)
    return;

  m_pListCtrl->SetRedraw(FALSE);

  // Clear everything just in case
  ClearEntries();

  // As we are the bottom row, we use the current state of entries in the
  // top row
  int iIndex(0);

  LVITEM lvi = {0};
  lvi.mask = LVIF_IMAGE | LVIF_PARAM;

  for (int i = 0; i < m_pOtherListCtrl->GetItemCount(); i++) {
    lvi.iItem = i;
    m_pOtherListCtrl->GetItem(&lvi);

    // Set new data
    const st_PWLV_lParam *pLP0 = (st_PWLV_lParam *)lvi.lParam;
    st_PWLV_lParam *pLP = new st_PWLV_lParam;
    pLP->hItem = pLP0->hItem;
    pLP->pci = pLP0->pci;
    lvi.lParam = (LPARAM)pLP;

    // Insert it
    iIndex = m_pListCtrl->InsertItem(&lvi);
    ASSERT(iIndex != -1);

    // Get & then set text in 2nd column (first column == image)
    CString cs_text = m_pOtherListCtrl->GetItemText(iIndex, 1);
    m_pListCtrl->SetItemText(iIndex, 1, cs_text);
  }

  // Set sort to use default column (1) and sort ascending - but save these values first
  bool bSaveSortAscending = m_bSortAscending;
  int iSaveSortedColumn = m_iSortedColumn;
  m_bSortAscending = true;
  m_iSortedColumn = 1;

  // Now sort it
  m_pListCtrl->SortItemsEx(CompareFunction, (DWORD_PTR)this);

  // Restore settings
  m_bSortAscending = bSaveSortAscending;
  m_iSortedColumn = iSaveSortedColumn;

  m_pListCtrl->SetRedraw(TRUE);
}

CItemData *CPWListView::GetFullPath(const int iItem, StringX &sx_FullPath)
{
  pws_os::Trace(L"CPWListView::GetFullPath: sx_FullPath=%s\n", sx_FullPath.c_str());
  sx_FullPath = L"";

  if (iItem < 0 || iItem > m_pListCtrl->GetItemCount() - 1) {
    return NULL;
  }

  // Is this an entry?  If so, we can return valid CItemData
  st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(iItem);

  if (pLP->pci != NULL) {
    sx_FullPath = pLP->pci->GetGroup();
    return pLP->pci;
  }

  // OK - a group.
  sx_FullPath = m_sxCurrentPath + m_sxCurrentPath.empty() ? L"" : sxDot;
  sx_FullPath += m_pListCtrl->GetItemText(iItem, 1);
  return NULL;
}

void CPWListView::OnItemDoubleClick(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
 *pLResult = 0L;

 NMLISTVIEW *pNMListView = (NMLISTVIEW *)pNotifyStruct;

 if (pNMListView->iItem != -1)
   m_pListCtrl->SetItemState(pNMListView->iItem,
                             LVIS_FOCUSED | LVIS_SELECTED,
                             LVIS_FOCUSED | LVIS_SELECTED);

  // Ignore double-click if multiple entries selected (LIST view only)
  if ( m_pListCtrl->GetSelectedCount() != 1)
    return;

  // Now set we have processed the event
 *pLResult = 1L;

  st_PWLV_lParam *pLP(NULL);
  int iIndex(-1);
  POSITION pos = m_pListCtrl->GetFirstSelectedItemPosition();
  ASSERT(pos != NULL);

  iIndex = m_pListCtrl->GetNextSelectedItem(pos);
  pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(iIndex);
  ASSERT(pLP != NULL);

  if (iIndex == -1)
    return;

  CItemData *pci = pLP->pci;
  // Don't do anything if can't get the data or a Group
  if (pci == NULL) {
    StringX sxEntry = m_pListCtrl->GetItemText(iIndex, 1);

    m_pTreeView->SetFocus();
    m_pTreeView->OnListViewFolderSelected(sxEntry, iIndex, true);
    m_pListCtrl->SetFocus();
    return;
  }

  // It is an entry
  if (pci->IsShortcut()) {
    pci = m_pDbx->GetBaseEntry(pci);
  }

  short iDCA;
  const bool m_bShiftKey = ((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000);
  pci->GetDCA(iDCA, m_bShiftKey);

  if (iDCA < PWSprefs::minDCA || iDCA > PWSprefs::maxDCA)
    iDCA = (short)PWSprefs::GetInstance()->GetPref(m_bShiftKey ?
              PWSprefs::ShiftDoubleClickAction : PWSprefs::DoubleClickAction);

  CItemData::FieldType ft = CItemData::UUID;
  switch (iDCA) {
    case PWSprefs::DoubleClickAutoType:
      ((CWnd *)m_pDbx)->PostMessage(WM_NOTIFY, PWS_MSG_EXPLORERAUTOTYPE, (WPARAM)pci);
      break;
    case PWSprefs::DoubleClickBrowse:
      m_pDbx->DoBrowse(false, false, pci);
      break;
    case PWSprefs::DoubleClickBrowsePlus:
      m_pDbx->DoBrowse(true, false, pci);
      break;
    case PWSprefs::DoubleClickCopyNotes:
      ft = CItemData::NOTES;
      break;
    case PWSprefs::DoubleClickCopyPassword:
      ft = CItemData::PASSWORD;
      break;
    case PWSprefs::DoubleClickCopyUsername:
      ft = CItemData::USER;
      break;
    case PWSprefs::DoubleClickCopyPasswordMinimize:
      m_pDbx->CopyDataToClipBoard(CItemData::PASSWORD, pci, true);
      break;
    case PWSprefs::DoubleClickViewEdit:
      if (pci->IsShortcut())
         m_pDbx->EditShortcut(pci);
      else
         m_pDbx->EditItem(pci);
      break;
    case PWSprefs::DoubleClickRun:
      m_pDbx->DoRunCommand(pci);
      break;
    case PWSprefs::DoubleClickSendEmail:
      m_pDbx->DoBrowse(false, false, pci);
      break;
    default:
      ASSERT(0);
  }

  if (ft != CItemData::UUID)
    m_pDbx->CopyDataToClipBoard(ft, pci);
}

void CPWListView::OnEndLabelEdit(NMHDR *pNotifyStruct, LRESULT *pLResult)
{
  if (m_pDbx->IsDBReadOnly())
    return; // don't edit in read-only mode

  *pLResult = 0;
  bool bOK(false);
  CItemData *pci(NULL);

  NMLVDISPINFO *plvinfo = reinterpret_cast<NMLVDISPINFO *>(pNotifyStruct);
  LV_ITEM *plvItem = &plvinfo->item;

  //plvItem->pszText is NULL if editing canceled
  if (plvItem->pszText != NULL) {
    CSecString cs_NewText = plvItem->pszText;

    PWScore *pcore = (PWScore *)m_pDbx->GetCore();
    MultiCommands *pmulticmds = MultiCommands::Create(pcore);

    st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(plvItem->iItem);
    pci = pLP->pci;
    bool bIsEntry = (pci != NULL);
    StringX sxNewText, sxNewTitle, sxNewUser, sxNewPassword;

    if (bIsEntry) {
      // Check new text is legal - i.e. Title not empty
      if (!SplitLeafText(cs_NewText, sxNewTitle, sxNewUser, sxNewPassword))
        goto exit;

      // Generate new format
      Format(sxNewText,L"%s [%s]", sxNewTitle.c_str(), sxNewUser.c_str());
      if (pci->IsProtected())
        sxNewText += L" #";
    } else
      sxNewText = (LPCWSTR)cs_NewText;

    // Verify doesn't already exist.
    for (int i = 0; i < m_pListCtrl->GetItemCount(); i++) {
      if (m_pListCtrl->GetItemText(i, 1) == sxNewText.c_str())
        goto exit;
    }

    // OK valid, we can change the entry
    if (bIsEntry) {
      // Update Leaf
      if (sxNewTitle != pci->GetTitle()) {
        pmulticmds->Add(UpdateEntryCommand::Create(pcore, *pci,
                                                   CItemData::TITLE, sxNewTitle));
      }

      if (sxNewUser != pci->GetUser()) {
        pmulticmds->Add(UpdateEntryCommand::Create(pcore, *pci,
                                                   CItemData::USER, sxNewUser));
      }
    } else {
      // Update Group
      // Check it doesn't contain a dot!
      if (sxNewText.find_first_of(L'.') != StringX::npos)
        goto exit;

      // We refresh the TREE and EXPLORER views
      Command *pcmd1 = UpdateGUICommand::Create(pcore,
                                                UpdateGUICommand::WN_UNDO,
                                                UpdateGUICommand::GUI_REFRESH_TREE);
      pmulticmds->Add(pcmd1);

      // Update Group
      StringX sxOldPath, sxNewPath;
      if (m_sxCurrentPath.empty()) {
        sxOldPath = m_sxOldText;
        sxNewPath = sxNewText;
      } else {
        sxOldPath = m_sxCurrentPath + sxDot + m_sxOldText;
        sxNewPath = m_sxCurrentPath + sxDot + sxNewText;
      }

      pmulticmds->Add(RenameGroupCommand::Create(pcore, sxOldPath, sxNewPath));

      // We refresh the view
      Command *pcmd2 = UpdateGUICommand::Create(pcore,
                                                UpdateGUICommand::WN_EXECUTE_REDO,
                                                UpdateGUICommand::GUI_REFRESH_TREE);
      pmulticmds->Add(pcmd2);
    }

    // Do it
    m_pDbx->Execute(pmulticmds);

    // Update list item
    m_pListCtrl->SetItemText(plvItem->iItem, 1, plvItem->pszText);

    // If the other pane is in the same group - we need to tell them
    if (m_pOtherListView != NULL &&
        m_sxCurrentPath == m_pOtherListView->GetCurrentPath()) {
      // We are - tell them
      m_pOtherListView->UpdateEntry(m_sxOldText, sxNewText);
    }

    // Mark database as modified
    m_pDbx->SetChanged(DboxMain::Data);
    m_pDbx->ChangeOkUpdate();

    bOK = true;
  }

exit:
  if (!bOK)
    m_pListCtrl->SetItemText(plvItem->iItem, 1, m_sxOldText.c_str());

  m_bEditLabelCompleted = true;

  if (m_bAccEn)
    app.EnableAccelerator();

  m_bInRename = false;
}

void CPWListView::UpdateEntry(StringX &sxOldGTU, StringX &sxNewGTU)
{
  for (int i = 0; i < m_pListCtrl->GetItemCount(); i++) {
    if (m_pListCtrl->GetItemText(i, 1) == sxOldGTU.c_str()) {
      m_pListCtrl->SetItemText(i, 1, sxNewGTU.c_str());
      m_pListCtrl->Update(i);
      break;
    }
  }
}

void CPWListView::SetEntryText(const CItemData *pci, const StringX &sxnewText)
{
  for (int i = 0; i < m_pListCtrl->GetItemCount(); i++) {
    st_PWLV_lParam *pLP = (st_PWLV_lParam *)m_pListCtrl->GetItemData(i);
    if (pLP->pci == pci) {
      m_pListCtrl->SetItemText(i, 1, sxnewText.c_str());
      m_pListCtrl->Update(i);
      break;
    }
  }
}
