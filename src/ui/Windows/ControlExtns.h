/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

#include "SecString.h"        // for CSecEditExtn
#include "InfoDisplay.h"      // for Listbox Tooltips
#include <vector>             // for Listbox Tooltips & EditExtn menus

// ControlExtns.h : header file
// Extensions to standard Static, Edit, ListBox and Combobox Controls

// Pick a number at the end of the WM_USER range
#define EM_SELECTALL (WM_APP - 1)

class CStaticExtn : public CStatic
{
  // Construction
public:
  CStaticExtn();
  virtual ~CStaticExtn();

  void SetColour(COLORREF cfUser)
  {m_bUserColour = true; m_cfUser = cfUser;}
  void ResetColour()
  {m_bUserColour = false;}
  void SetBkColour(COLORREF cfBkUser);
  void ResetBkColour();

  void FlashBkgnd(COLORREF cfFlashColour);
  void SetHighlight(bool bHighlight, COLORREF cfHighlightColour)
  {m_bHighlight = bHighlight; m_cfHighlightColour = cfHighlightColour;}

  inline int IsFlashing() {return m_iFlashing;}
  inline bool GetColourState()  {return m_bUserColour;}
  inline bool IsHighlighted() {return m_bHighlight;}
  inline bool IsMouseInWindow() {return m_bMouseInWindow;}
  inline COLORREF GetFlashColour() {return m_cfFlashColour;}
  inline COLORREF GetHighlightColour() {return m_cfHighlightColour;}
  inline COLORREF GetUserColour() {return m_cfUser;}

protected:
  //{{AFX_MSG(CStaticExtn)
  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
  afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
  afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
  int m_iFlashing;
  COLORREF m_cfUser, m_cfOldColour, m_cfFlashColour, m_cfHighlightColour, m_cfBkUser;
  bool m_bUserColour, m_bMouseInWindow, m_bHighlight, m_bUserBkColour;
  CBrush m_brBkUser;
};

struct st_context_menu {
  UINT_PTR message_number;
  std::wstring menu_string;
  int flags;
};

class CEditExtn : public CEdit
{
  // Construction
public:
  CEditExtn(COLORREF focusColor = (RGB(222, 255, 222))); // light green
  CEditExtn(std::vector<st_context_menu> vmenu_items, 
            COLORREF focusColor = (RGB(222, 255, 222))); //light green
  virtual ~CEditExtn();

  void ChangeColour() {m_bIsFocused = TRUE;}
  void UpdateState(const int message_number, const BOOL new_state);

  DWORD GetSel();
  void GetSel(int &nStartChar, int &nEndChar);
  void SetSel(DWORD dwSelection, BOOL bNoScroll = FALSE);
  void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll = FALSE);

protected:
  //{{AFX_MSG(CEditExtn)
  afx_msg void OnSetFocus(CWnd* pOldWnd);
  afx_msg void OnKillFocus(CWnd* pNewWnd);
  afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

  // Attributes
private:
  BOOL m_bIsFocused;

  CBrush m_brInFocus;
  CBrush m_brNoFocus;
  const COLORREF m_crefInFocus;

  int m_lastposition, m_nStartChar, m_nEndChar;
  std::vector<st_context_menu> m_vmenu_items;
};

// Following is meant for sensitive information that you really don't
// want to be in memory more than necessary, such as master passwords
// We use a CSecEditExtn::Impl class member not for security, but to
// avoid #including stuff here that really shouldn't be of interest to
// users of these classes

class CSecEditExtn : public CEditExtn
{
public:
  CSecEditExtn();
  CSecEditExtn(std::vector<st_context_menu> vmenu_items);
  virtual ~CSecEditExtn();

  // Overriding virtuals doesn't work, due to defective
  // implementation of DDX_Text. Grr.
  void DoDDX(CDataExchange *pDX, CSecString &str);
  void SetSecure(bool on_off); // on by default
  bool GetSecure() const {return m_secure;}
  CSecString GetSecureText() const;
  void SetSecureText(const CSecString &str);

protected:
  //{{AFX_MSG(CSecEditExtn)
  afx_msg void OnUpdate();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP();

private:
  void OnSecureUpdate();
  struct Impl;
  Impl *m_pImpl;
  bool m_secure;
  bool m_in_recursion;
};

class CComboBoxExtn;

class CListBoxExtn : public CListBox
{
  // Construction
public:
  CListBoxExtn();
  virtual ~CListBoxExtn();

  void ChangeColour() {m_bIsFocused = TRUE;}
  void ActivateToolTips();
  void SetCombo(CComboBoxExtn *pCombo) {m_pCombo = pCombo;}

  // Generated message map functions
protected:
  //{{AFX_MSG(CListBoxExtn)
  afx_msg void OnSetFocus(CWnd* pOldWnd);
  afx_msg void OnKillFocus(CWnd* pNewWnd);
  afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnMouseMove(UINT nFlags, CPoint point);
  afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

  // Attributes
private:
  BOOL m_bIsFocused;

  CBrush m_brInFocus;
  CBrush m_brNoFocus;

  bool ShowToolTip(int nItem, const bool bVisible);

  CComboBoxExtn *m_pCombo;
  CInfoDisplay *m_pLBToolTips;
  UINT_PTR m_nHoverLBTimerID, m_nShowLBTimerID;
  CPoint m_HoverLBPoint;
  int m_HoverLBnItem;
  bool m_bUseToolTips, m_bMouseInWindow;
};

class CComboBoxExtn : public CComboBox
{
  // Construction
public:
  CComboBoxExtn();
  virtual ~CComboBoxExtn();

  void SetToolTipStrings(std::vector<CSecString> vtooltips);
  CSecString GetToolTip(int nItem)
  {return m_vtooltips[nItem];}

  CEditExtn m_edit;
  CListBoxExtn m_listbox;
  void ChangeColour();

protected:
  //{{AFX_MSG(CComboBoxExtn)
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg void OnDestroy();
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
  bool m_bUseToolTips;
  std::vector<CSecString> m_vtooltips;
};

/////////////////////////////////////////////////////////////////////////////
// CSymbolEdit

class CSymbolEdit : public CEdit
{
  // Construction
public:
  CSymbolEdit() {}

protected:
  //{{AFX_MSG(CSymbolEdit)
  afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg LRESULT OnPaste(WPARAM wParam, LPARAM lParam);
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()
};
