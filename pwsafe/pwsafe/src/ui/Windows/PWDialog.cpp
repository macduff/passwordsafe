/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "ThisMfcApp.h"
#include "DboxMain.h"
#include "PWDialog.h"

#include <algorithm>
#include <functional>

#if defined(POCKET_PC)
#error "TBD - define proper Dialog base class for PPC"
#endif

extern const wchar_t *EYE_CATCHER;

static CPWDialogTracker the_tracker;
CPWDialogTracker *CPWDialog::sm_tracker = &the_tracker; // static member

IMPLEMENT_DYNAMIC(CPWDialog, CDialog)

LRESULT CPWDialog::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  CWnd *pParent = GetParent();
  while (pParent != NULL) {
    DboxMain *pDbx = dynamic_cast<DboxMain *>(pParent);
    if (pDbx != NULL && pDbx->m_eye_catcher != NULL &&
        wcscmp(pDbx->m_eye_catcher, EYE_CATCHER) == 0) {
      pDbx->ResetIdleLockCounter(message);
      break;
    } else
      pParent = pParent->GetParent();
  }
  if (pParent == NULL)
    pws_os::Trace(L"CPWDialog::WindowProc - couldn't find DboxMain ancestor\n");
  return CDialog::WindowProc(message, wParam, lParam);
}

INT_PTR CPWDialog::DoModal()
{
  bool bAccEn = app.IsAcceleratorEnabled();
  if (bAccEn)
    app.DisableAccelerator();

  GetDialogTracker()->AddOpenDialog(this);
  INT_PTR rc = CDialog::DoModal();
  GetDialogTracker()->RemoveOpenDialog(this);

  if (bAccEn)
    app.EnableAccelerator();

  return rc;
}


CPWDialogTracker *CPWDialog::GetDialogTracker()
{
  return sm_tracker;
}

CPWDialogTracker::CPWDialogTracker()
{
}

CPWDialogTracker::~CPWDialogTracker()
{
}

bool CPWDialogTracker::AnyOpenDialogs() const
{
  bool retval;
  m_mutex.Lock();
  retval = !m_dialogs.empty();
  m_mutex.Unlock();
  return retval;
}

void CPWDialogTracker::AddOpenDialog(CWnd *dlg)
{
  m_mutex.Lock();
  m_dialogs.push_back(dlg);
  m_mutex.Unlock();
}

void CPWDialogTracker::RemoveOpenDialog(CWnd *dlg)
{
  m_mutex.Lock();
  m_dialogs.remove(dlg);
  m_mutex.Unlock();
}

void CPWDialogTracker::Apply(void (*f)(CWnd *))
{
  // we operate on a copy of the list of dialogs,
  // to avoid deadlocks and other nastiness
  std::list<CWnd *> dialogs;
  m_mutex.Lock();
  dialogs = m_dialogs;
  m_mutex.Unlock();
  std::for_each(dialogs.begin(), dialogs.end(), std::ptr_fun(f));
}
