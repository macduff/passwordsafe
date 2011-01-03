/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file AttProgressDlg.h
//-----------------------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "PWDialog.h"
#include "resource.h"
#include "AttThreadParms.h"

// CAttProgressDlg dialog

#define ATTPRG_UPDATE_GUI          (WM_APP + 100)
#define ATTPRG_THREAD_ENDED        (WM_APP + 101)

class CAttProgressDlg : public CPWDialog
{
  DECLARE_DYNAMIC(CAttProgressDlg)

public:
  CAttProgressDlg(CWnd* pParent, bool *pbAttachmentCancel = NULL,
                  bool *pbStopVerify = NULL, ATThreadParms *pthdpms = NULL);

  void SetThread(CWinThread *pAttThread)
  {m_pAttThread = pAttThread;}

// Dialog Data
  enum { IDD = IDD_ATTPROGRESS };
  CProgressCtrl m_progress;

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual BOOL OnInitDialog();
  afx_msg void OnStopVerify();
  afx_msg void OnCancel();
  afx_msg void OnPauseResume();
  LRESULT OnUpdateProgress(WPARAM wparam, LPARAM );
  LRESULT OnThreadFinished(WPARAM , LPARAM );

  DECLARE_MESSAGE_MAP()

private:
  DboxMain *m_pDbx;
  CWinThread *m_pAttThread;

  CString m_function, m_owner, m_filename, m_description;
  bool *m_pbStopVerify, *m_pbAttachmentCancel, m_bPause;
  ATThreadParms *m_pthdpms;
};
