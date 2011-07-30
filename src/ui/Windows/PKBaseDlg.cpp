/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "stdafx.h"
#include "PKBaseDlg.h"
#include "resource.h"
#include "os/env.h"

CPKBaseDlg::CPKBaseDlg(int id, CWnd *pParent)
  : CPWDialog(id, pParent), m_passkey(L""), m_yubi(new Yubi(this))
{
  m_pctlPasskey = new CSecEditExtn;
  if (pws_os::getenv("PWS_PW_MODE", false) == L"NORMAL")
    m_pctlPasskey->SetSecure(false);
}

CPKBaseDlg::~CPKBaseDlg()
{
  delete m_yubi;
  delete m_pctlPasskey;
}

void CPKBaseDlg::yubiInserted(void)
{
  GetDlgItem(IDC_YUBIKEY_BTN)->EnableWindow(TRUE);
  m_yubi_status.SetWindowText(_T("Click, then activate your YubiKey"));
}

void CPKBaseDlg::yubiRemoved(void)
{
  GetDlgItem(IDC_YUBIKEY_BTN)->EnableWindow(FALSE);
  m_yubi_status.SetWindowText(_T("Please insert your YubiKey"));
}

void CPKBaseDlg::yubiCompleted(ycRETCODE rc)
{
  m_yubi_status.ShowWindow(SW_SHOW);
  m_yubi_timeout.ShowWindow(SW_HIDE);
  switch (rc) {
  case ycRETCODE_OK:
    m_yubi_timeout.SetPos(0);
    m_yubi_status.SetWindowText(_T(""));
    TRACE(_T("yubiCompleted(ycRETCODE_OK)"));
    // Get hmac, process it, synthesize OK event
    m_yubi->RetrieveHMACSha1(m_passkey);
    // The returned hash is the passkey
    ProcessPhrase();
    // If we returned from above, reset status:
    m_yubi_status.SetWindowText(_T("Click, then activate your YubiKey"));
    break;
  case ycRETCODE_NO_DEVICE:
    // device removed while waiting?
    TRACE(_T("yubiCompleted(ycRETCODE_NO_DEVICE)\n"));
    m_yubi_status.SetWindowText(_T("Error: YubiKey removed"));
    break;
  case ycRETCODE_TIMEOUT:
    // waited, no user input
    TRACE(_T("yubiCompleted(ycRETCODE_TIMEOUT)\n"));
    m_yubi_status.SetWindowText(_T("YubiKey timed out"));
    break;
  case ycRETCODE_MORE_THAN_ONE:
    TRACE(_T("yubiCompleted(ycRETCODE_MORE_THAN_ONE)\n"));
    m_yubi_status.SetWindowText(_T("More than one YubiKey detected"));
    break;
  case ycRETCODE_REENTRANT_CALL:
    TRACE(_T("yubiCompleted(ycRETCODE_REENTRANT_CALL)\n"));
    m_yubi_status.SetWindowText(_T("Internal error: Reentrant call"));
    break;
  case ycRETCODE_FAILED:
    TRACE(_T("yubiCompleted(ycRETCODE_FAILED)\n"));
    m_yubi_status.SetWindowText(_T("YubiKey timed out: Click to try again"));
    break;
  default:
    // Generic error message
    TRACE(_T("yubiCompleted(%d)\n"), rc);
    m_yubi_status.SetWindowText(_T("Internal error: Unknown return code"));
    break;
  }
}

void CPKBaseDlg::yubiWait(WORD seconds)
{
  // Update progress bar
  m_yubi_timeout.SetPos(seconds);
  TRACE(_T("CPKBaseDlg::yubiWait(%d)\n"), seconds);
}
