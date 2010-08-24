/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
/// \file AddDescription.h
//-----------------------------------------------------------------------------

#pragma once

#include "resource.h"

// CAddDescription dialog

class CAddDescription : public CDialog
{
	DECLARE_DYNAMIC(CAddDescription)

public:
	CAddDescription(CWnd* pParent, const CString filename);   // standard constructor
	virtual ~CAddDescription();

// Dialog Data
	enum { IDD = IDD_ADD_DESCRIPTION };
  CString GetDescription() {return m_description;}

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  afx_msg void OnBnClickedOk();

	DECLARE_MESSAGE_MAP()

private:
  CString m_filename, m_description;
};
