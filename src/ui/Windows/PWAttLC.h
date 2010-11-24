/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "CoolMenu.h"

#include "core/Itemdata.h"
#include "core/attachments.h"

// ATT_ERASURE_REQUIRED = ATT_ERASEPGMEXISTS | ATT_ERASEONDBCLOSE
#define ATT_ERASURE_REQUIRED    0x60

// Extract attachment via context menu on Attachment CListCtrl - ALSO DEFINED IN DBOXMAIN.H
#define PWS_MSG_EXTRACT_ATTACHMENT      (WM_APP + 70)

// Export attachment via context menu on Attachment CListCtrl - ALSO DEFINED IN DBOXMAIN.H
#define PWS_MSG_EXPORT_ATTACHMENT       (WM_APP + 71)

// Change attachment via context menu on Attachment CListCtrl - ALSO DEFINED IN DBOXMAIN.H
#define PWS_MSG_CHANGE_ATTACHMENT       (WM_APP + 72)

// Update AddEdit_Attachments that the user has changed an entry's flags - ALSO DEFINED IN DBOXMAIN.H
#define PWS_MSG_ATTACHMENT_FLAG_CHANGED (WM_APP + 79)

class CPWAttDlg;

class CPWAttLC : public CListCtrl
{
public:
  enum LCType {NEW = 0, EXTRACT, EXISTING, VIEW, NUMTYPES};

  CPWAttLC();
  ~CPWAttLC();

  friend class CAddEdit_Attachments;

  void Init(const LCType lct, CWnd *pWnd, const bool bReadOnly = false);
  
  void AddAttachments(ATRExVector &vatrex);
  void AddAttachments(ATRVector &vatr, const bool bInitialize = false);
  void AddNewAttachment(const size_t &num, ATRecord &atr);
  void GetAttachmentFlags(const size_t num, BYTE &flags, BYTE &uiflags)
  {flags = m_vATRecords[num].flags; uiflags = m_vATRecords[num].uiflags;}
  void GetNewAttachmentFlags(const size_t num, BYTE &flags)
  {flags = m_vATRecords[num].flags;}
  int m_iItem;

protected:
  WCHAR *m_pwchTip;
  char *m_pchTip;

  CCoolMenuManager m_menuManager;

  BOOL PreTranslateMessage(MSG* pMsg);
  INT_PTR OnToolHitTest(CPoint point, TOOLINFO * pTI) const;

  //{{AFX_MSG(CPWAttLC)
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnCustomDraw(NMHDR *pNotifyStruct, LRESULT *pResult);
  afx_msg BOOL OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnHdnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnHdnEndDrag(NMHDR *pNMHDR, LRESULT *pResult);
  afx_msg void OnHdnEndTrack(NMHDR *pNMHDR, LRESULT *pResult);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
  void DrawImage(CDC *pDC, CRect &rect, int nImage);
  void RecalcHeaderTips();

  void RemoveAttachment();
  void AddAttachment(const size_t &num, ATRecordEx &atrex);
  void AddAttachment(const size_t &num, ATRecord &atr, const wchar_t * szGroup = NULL, 
                     const wchar_t * szTitle = NULL, const wchar_t * szUser = NULL);

  CWnd *m_pParent, *m_pWnd;
  CHeaderCtrl *m_pHeaderCtrl;
  CImageList *m_pCheckImageList;
  CToolTipCtrl *m_pToolTipCtrl;

  // Needed to make the row height bigger
  CImageList m_imageList;

  ATRExVector m_vATRecordsEx;
  ATRVector m_vATRecords;
  COLORREF m_crWindowText;
  int m_fwidth, m_lwidth, m_rowheight;
  int m_numattachments;
  bool *m_pbColTT, m_bReadOnly;
  LCType m_lct;
};
