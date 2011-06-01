/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// Properties.cpp : implementation file
//

#include "stdafx.h"
#include "Properties.h"
#include "NumUtilities.h"
#include "core/StringXStream.h" // for ostringstreamT

// CProperties dialog

IMPLEMENT_DYNAMIC(CProperties, CPWDialog)

BEGIN_MESSAGE_MAP(CProperties, CPWDialog)
  ON_BN_CLICKED(IDOK, &CPWDialog::OnOK)
END_MESSAGE_MAP()

BOOL CProperties::OnInitDialog()
{
  GetDlgItem(IDC_DATABASENAME)->SetWindowText(m_dbp.database.c_str());
  GetDlgItem(IDC_DATABASEFORMAT)->SetWindowText(m_dbp.databaseformat.c_str());
  GetDlgItem(IDC_NUMGROUPS)->SetWindowText(m_dbp.numgroups.c_str());
  GetDlgItem(IDC_NUMENTRIES)->SetWindowText(m_dbp.numentries.c_str());
  GetDlgItem(IDC_SAVEDON)->SetWindowText(m_dbp.whenlastsaved.c_str());
  GetDlgItem(IDC_SAVEDBY)->SetWindowText(m_dbp.wholastsaved.c_str());
  GetDlgItem(IDC_SAVEDAPP)->SetWindowText(m_dbp.whatlastsaved.c_str());
  GetDlgItem(IDC_FILEUUID)->SetWindowText(m_dbp.file_uuid.c_str());
  GetDlgItem(IDC_UNKNOWNFIELDS)->SetWindowText(m_dbp.unknownfields.c_str());

  CString tmp1, tmp2, tmp3, tmp4, tmp5;;
  tmp1.Format(_T("%d"), m_dbp.num_att);
  if (m_dbp.num_att == 0) {
    tmp2 = tmp3 = tmp4 = L"n/a";
  } else {
    double dblVar;
    wchar_t wcbuffer[40];
    dblVar = (m_dbp.totalunc + 1023.0) / 1024.0;
    PWSNumUtil::DoubleToLocalizedString(::GetThreadLocale(), dblVar, 1,
                                        wcbuffer, sizeof(wcbuffer) / sizeof(wchar_t));
    wcscat_s(wcbuffer, sizeof(wcbuffer) / sizeof(wchar_t), L" KB");
    tmp2 = wcbuffer;

    dblVar = (m_dbp.totalcmp + 1023.0) / 1024.0;;
    PWSNumUtil::DoubleToLocalizedString(::GetThreadLocale(), dblVar, 1,
                                        wcbuffer, sizeof(wcbuffer) / sizeof(wchar_t));
    wcscat_s(wcbuffer, sizeof(wcbuffer) / sizeof(wchar_t), L" KB");
    tmp3 = wcbuffer;

    dblVar = (m_dbp.largestunc + 1023.0) / 1024.0;;
    PWSNumUtil::DoubleToLocalizedString(::GetThreadLocale(), dblVar, 1,
                                        wcbuffer, sizeof(wcbuffer) / sizeof(wchar_t));
    wcscat_s(wcbuffer, sizeof(wcbuffer) / sizeof(wchar_t), L" KB");
    tmp4 = wcbuffer;

    dblVar = (m_dbp.largestcmp + 1023.0) / 1024.0;;
    PWSNumUtil::DoubleToLocalizedString(::GetThreadLocale(), dblVar, 1,
                                        wcbuffer, sizeof(wcbuffer) / sizeof(wchar_t));
    wcscat_s(wcbuffer, sizeof(wcbuffer) / sizeof(wchar_t), L" KB");
    tmp4 += L" / ";
    tmp4 += wcbuffer;
  }

  GetDlgItem(IDC_NUMATTACHMENTS)->SetWindowText(tmp1);
  GetDlgItem(IDC_TOTALUNCMP)->SetWindowText(tmp2);
  GetDlgItem(IDC_TOTALCMP)->SetWindowText(tmp3);
  GetDlgItem(IDC_ATTLARGEST)->SetWindowText(tmp4);

  return TRUE;
}
