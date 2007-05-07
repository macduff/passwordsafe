/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */
#pragma once

#include <Afxcmn.h>
#include "DropTarget.h"
#include "DropSource.h"
#include "corelib/ItemData.h"
#include "corelib/MyString.h"
#include "DDSupport.h"

class CTVTreeCtrl : public CTreeCtrl, public CDropTarget, public CDropSource
{
public:
  CTVTreeCtrl();
  ~CTVTreeCtrl();

  BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
    DROPEFFECT dropEffect, CPoint point);
  DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject,
    DWORD dwKeyState, CPoint point);
  DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
    DWORD dwKeyState, CPoint point);

   void operator delete(void* p)
        { CDropTarget::operator delete(p); }

   // indices of bitmaps in ImageList
  enum {NODE=0, LEAF=1, EXPIRED_LEAF = 2, WARNEXPIRED_LEAF = 3};
  enum {REQUEST_INFO = 0, SEND_INFO = 1};

  void DeleteWithParents(HTREEITEM hItem); // if a parent node becomes a leaf
  void DeleteFromSet(HTREEITEM hItem);
  CString GetGroup(HTREEITEM hItem); // get group path to hItem
  HTREEITEM AddGroup(const CString &path);
  bool IsLeafNode(HTREEITEM hItem);
  void RestoreExpanded();
  void ClearExpanded(); // use when items will be invalid
  void OnCollapseAll();
  void OnExpandAll();
  void SetDboxPointer(void *parent) {m_parent = parent;}
  void SetListPointer(CListCtrl *pList) {m_pctlItemList = pList;}
  HTREEITEM GetNextTreeItem(HTREEITEM hItem);
  void BeginLabelEdit(NMHDR *pNotifyStruct, LRESULT * &pLResult);
  void EndLabelEdit(NMHDR *pNotifyStruct, LRESULT * &pLResult);
  void ExpandCollapse(NMHDR *pNotifyStruct, LRESULT * &pLresult);

 protected:
  virtual void CompleteMove();

  //{{AFX_MSG(CTVTreeCtrl)
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
  afx_msg void OnDestroy();
  //}}AFX_MSG

  BOOL PreTranslateMessage(MSG* pMsg);

  DECLARE_MESSAGE_MAP()

private:
  CImageList *m_pimagelist;
  void *m_parent;
  void *m_expandedItems; // Internally this is a SetTreeItem_t, don't want to include stl file here...

  bool m_isRestoring; // don't repopulate m_expandedItems in restore

  void SetNewStyle(long lStyleMask, BOOL bSetBits);
  bool TransferItem(HTREEITEM hitem, HTREEITEM hNewParent);
  bool CopyItem(HTREEITEM hitem, HTREEITEM hNewParent);
  bool IsChildNodeOf(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent);
  void UpdateLeafsGroup(HTREEITEM hItem, CString prefix);
  void CollapseBranch(HTREEITEM hItem);
  void GetGroupEntriesData(CDDObList &out_oblist, HTREEITEM hItem);
  void GetEntryData(CDDObList &out_oblist, CItemData *ci);
  bool CollectData(BYTE * &out_buffer, long &outLen);
  bool ProcessData(BYTE *in_buffer, const long &inLen, const CMyString DropGroup);
  void DecryptReceivedData(BYTE * &in_buffer, const long &inLen,
                           unsigned char * &out_buffer, long &outLen);
  void EncryptSendingData(unsigned char * &in_buffer, const long &inLen,
                          BYTE * &out_buffer, long &outLen);
  CDropSource m_TCDropSource;
  CDropTarget m_TCDropTarget;
  CImageList* m_pDragImage;
  HTREEITEM m_iItem;

protected:
  CListCtrl *m_pctlItemList;
  HTREEITEM m_hitemDrag;
  HTREEITEM m_hitemDrop;
  unsigned char m_sending_classname[40];
  int m_nDragPathLen;
  unsigned int  m_uiSendingSession;
  unsigned int  m_uiReceivingSession;
  int m_calls;
};
