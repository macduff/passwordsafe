/// \file AddDlg.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "PasswordSafe.h"
#include "MainDlg.h"
#include "AddDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------
CAddDlg::CAddDlg(CWnd* pParent)
   : CDialog(CAddDlg::IDD, pParent)
{
   m_password = "";
   m_notes = "";
   m_username = "";
   m_title = "";
}


void CAddDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_PASSWORD, m_password.m_mystring);
   DDX_Text(pDX, IDC_NOTES, m_notes.m_mystring);
   DDX_Text(pDX, IDC_USERNAME, m_username.m_mystring);
   DDX_Text(pDX, IDC_TITLE, m_title.m_mystring);
}


BEGIN_MESSAGE_MAP(CAddDlg, CDialog)
   ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_RANDOM, OnRandom)
END_MESSAGE_MAP()


void
CAddDlg::OnCancel() 
{
   app.m_pMainWnd = NULL;
   CDialog::OnCancel();
}


void
CAddDlg::OnOK() 
{
   UpdateData(TRUE);

   //Check that data is valid
   if (m_title == "")
   {
      AfxMessageBox("This entry must have a title.");
      ((CEdit*)GetDlgItem(IDC_TITLE))->SetFocus();
      return;
   }
   if (m_password == "")
   {
      AfxMessageBox("This entry must have a password.");
      ((CEdit*)GetDlgItem(IDC_PASSWORD))->SetFocus();
      return;
   }
   //End check

   CMyString temptitle;
   MakeName(temptitle, m_title, m_username);
   CPasswordSafeDlg *pParent = (CPasswordSafeDlg*)GetParent();
   ASSERT(pParent != NULL);
   if (pParent->Find(temptitle) != NULL)
   {
      CMyString temp =
         "An item with the title \""
         + temptitle
         + "\" already exists.";
      AfxMessageBox(temp);
      ((CEdit*)GetDlgItem(IDC_TITLE))->SetSel(MAKEWORD(-1, 0));
      ((CEdit*)GetDlgItem(IDC_TITLE))->SetFocus();
   }
   else
   {
      app.m_pMainWnd = NULL;
      CDialog::OnOK();
   }
}


void CAddDlg::OnHelp() 
{
   WinHelp(0x2008E, HELP_CONTEXT);
}


void CAddDlg::OnRandom() 
{
   CMyString temp = "";
   for (int x=0; x<8; x++)
      temp += GetRandAlphaNumChar();

   UpdateData(TRUE);
	
   int nResponse;
   if (m_password == "")
      nResponse = IDYES;
   else
   {
      CMyString msg;
      msg = "The randomly generated password is: \""
         + temp
         + "\" (without\nthe quotes). Would you like to use it?";
      nResponse = MessageBox(msg, AfxGetAppName(),
                             MB_ICONEXCLAMATION|MB_YESNO);
   }

   if (nResponse == IDYES)
   {
      m_password = temp;
      UpdateData(FALSE);
   }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
