/// \file EditDlg.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "PasswordSafe.h"

#include "ThisMfcApp.h"
#include "DboxMain.h"
#include "EditDlg.h"
#include "PwFont.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void CEditDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Text(pDX, IDC_NOTES, (CString &)m_notes);
   DDX_Text(pDX, IDC_PASSWORD, (CString &)m_password);
   DDX_Text(pDX, IDC_PASSWORD2, (CString &)m_password2);  //DK
   DDX_Text(pDX, IDC_PASSWORD3, (CString &)m_password3);  //DK
   DDX_Text(pDX, IDC_USERNAME, (CString &)m_username);
   DDX_Text(pDX, IDC_TITLE, (CString &)m_title);
}


BEGIN_MESSAGE_MAP(CEditDlg, CDialog)
   ON_BN_CLICKED(IDC_SHOWPASSWORD, OnShowpassword)
   ON_BN_CLICKED(IDHELP, OnHelp)
   ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_RANDOM, OnRandom)
END_MESSAGE_MAP()


void CEditDlg::OnShowpassword() 
{
   UpdateData(TRUE);

   CMyString wndName;
   GetDlgItem(IDC_SHOWPASSWORD)->GetWindowText(wndName);

   if (wndName == "&Show Password")
   {
      ShowPassword();
   }
   else if (wndName == "&Hide Password")
   {
      m_realpassword = m_password;
	   m_realpassword2 = m_password2;  //DK
	   m_realpassword3 = m_password3;  //DK
      HidePassword();
   }
   else
      AfxMessageBox("Error in retrieving window text");

   UpdateData(FALSE);
}


void
CEditDlg::OnOK() 
{
   UpdateData(TRUE);

   /*
    *  If the password is shown it may have been edited,
    *  so save the current text.
    */

   if (! m_isPwHidden)
   {
      m_realpassword = m_password;
	   m_realpassword2 = m_password2;  //DK
	   m_realpassword3 = m_password3;  //DK
   }
      
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

   DboxMain* pParent = (DboxMain*) GetParent();
   ASSERT(pParent != NULL);

   POSITION listindex = pParent->Find(m_title, m_username);
   /*
    *  If there is a matching entry in our list, and that
    *  entry is not the same one we started editing, tell the
    *  user to try again.
    */
   if ((listindex != NULL) &&
       (m_listindex != listindex))
   {
      CMyString temp =
         "An item with Title \""
         + m_title + "\" and User Name \"" + m_username
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


void CEditDlg::OnCancel() 
{
   app.m_pMainWnd = NULL;
   CDialog::OnCancel();
}


BOOL CEditDlg::OnInitDialog() 
{
   CDialog::OnInitDialog();
 
   SetPasswordFont(GetDlgItem(IDC_PASSWORD));
   SetPasswordFont(GetDlgItem(IDC_PASSWORD2));  //DK
   SetPasswordFont(GetDlgItem(IDC_PASSWORD3));  //DK

   if (app.GetProfileInt("", "showpwdefault", FALSE) == TRUE)
   {
      ShowPassword();
   }
   else
   {
      HidePassword();
   }
   UpdateData(FALSE);
   return TRUE;
}


void CEditDlg::ShowPassword(void)
{
   m_password = m_realpassword;
   m_password2 = m_realpassword2;  //DK
   m_password3 = m_realpassword3;  //DK
   m_isPwHidden = false;
   GetDlgItem(IDC_SHOWPASSWORD)->SetWindowText("&Hide Password");
   GetDlgItem(IDC_PASSWORD)->EnableWindow(TRUE);
   GetDlgItem(IDC_PASSWORD2)->EnableWindow(TRUE);  //DK
   GetDlgItem(IDC_PASSWORD3)->EnableWindow(TRUE);  //DK
}


void CEditDlg::HidePassword(void)
{
   m_password = HIDDEN_PASSWORD;
   m_password2 = HIDDEN_PASSWORD;  //DK
   m_password3 = HIDDEN_PASSWORD;  //DK
   m_isPwHidden = true;
   GetDlgItem(IDC_SHOWPASSWORD)->SetWindowText("&Show Password");
   GetDlgItem(IDC_PASSWORD)->EnableWindow(FALSE);
   GetDlgItem(IDC_PASSWORD2)->EnableWindow(FALSE);  //DK
   GetDlgItem(IDC_PASSWORD3)->EnableWindow(FALSE);  //DK
}


void CEditDlg::OnRandom() 
{
   DboxMain* pParent = (DboxMain*) GetParent();
   ASSERT(pParent != NULL);
   CMyString temp = pParent->GetPassword();

   UpdateData(TRUE);
   CMyString msg;
   int nResponse;
 
   //Ask if something's there
   if (m_password != "")
   {
      msg =
         "The randomly generated password is: \""
         + temp
         + "\" \n(without the quotes). Would you like to use it?";
      nResponse = MessageBox(msg, 
                             AfxGetAppName(),
                             MB_ICONEXCLAMATION|MB_YESNO);
   }
   else 
      nResponse = IDYES;

   if (nResponse == IDYES)
   {
      m_realpassword = temp;

      CMyString wndName;
      GetDlgItem(IDC_SHOWPASSWORD)->GetWindowText(wndName);

      if (wndName == "&Show Password")
      {
	m_password = HIDDEN_PASSWORD;
      }
      else if (wndName == "&Hide Password")
      {
         m_password = m_realpassword;
      }
      UpdateData(FALSE);
   }
   else if (nResponse == IDNO)
   {
   }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
