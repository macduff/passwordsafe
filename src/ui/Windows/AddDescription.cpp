/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// AddDescription.cpp : implementation file
//

#include "stdafx.h"
#include "AddDescription.h"

// CAddDescription dialog

IMPLEMENT_DYNAMIC(CAddDescription, CPWDialog)

CAddDescription::CAddDescription(CWnd* pParent, const CString filename)
	: CPWDialog(CAddDescription::IDD, pParent), m_filename(filename)
{
}

CAddDescription::~CAddDescription()
{
}

void CAddDescription::DoDataExchange(CDataExchange* pDX)
{
	CPWDialog::DoDataExchange(pDX);

  DDX_Text(pDX, IDC_STATIC_ATTACHMENTNAME, m_filename);
  DDX_Text(pDX, IDC_OPTIONALDESCRIPTION, m_description);
}

BEGIN_MESSAGE_MAP(CAddDescription, CPWDialog)
  ON_BN_CLICKED(IDOK, OnOK)
END_MESSAGE_MAP()

// CAddDescription message handlers

void CAddDescription::OnOK()
{
  // TODO: Add your control notification handler code here
  CPWDialog::OnOK();
}
