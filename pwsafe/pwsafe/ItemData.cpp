/// \file ItemData.cpp
//-----------------------------------------------------------------------------

//in add, Ok is ok the first time

#include "stdafx.h"
#include "PasswordSafe.h"

#include <io.h>
#include <math.h>

#include "Util.h"
#include "Blowfish.h"
#include "sha1.h"

#include "ThisMfcApp.h"

#include "ItemData.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------
//More complex constructor
CItemData::CItemData(const CMyString &name,
                     const CMyString &password,
                     const CMyString &password2,  //DK 
                     const CMyString &password3,  //DK 
                     const CMyString &notes)
{
   InitStuff();
   SetName(name);
   SetPassword(password);
   SetPassword(password2, 2);  //DK 
   SetPassword(password3, 3);  //DK 
   SetNotes(notes);
}

CItemData::CItemData(const CItemData &stuffhere)
{
   m_nLength = stuffhere.m_nLength;
   m_pwLength = stuffhere.m_pwLength;
   m_pwLength2 = stuffhere.m_pwLength2;  //DK 
   m_pwLength3 = stuffhere.m_pwLength3;  //DK 
   m_notesLength = stuffhere.m_notesLength;

   m_name = new unsigned char[GetBlockSize(m_nLength)];
   m_nameValid = TRUE;
   memcpy(m_name, stuffhere.m_name, GetBlockSize(m_nLength));
	
   m_password = new unsigned char[GetBlockSize(m_pwLength)];
   m_pwValid = TRUE;
   memcpy(m_password, stuffhere.m_password, GetBlockSize(m_pwLength));

   if (m_pwLength2 > 0)  //DK 
   {  //DK 
	   m_password2 = new unsigned char[GetBlockSize(m_pwLength2)];  //DK 
		m_pwValid2 = TRUE;  //DK 
		memcpy(m_password2, stuffhere.m_password2, GetBlockSize(m_pwLength2));  //DK 
   }  //DK 
   else  //DK 
   {  //DK 
	   m_password2 = NULL;  //DK 
		m_pwValid2 = FALSE;  //DK 
   }  //DK 

   if (m_pwLength3 > 0)  //DK 
   {  //DK 
	   m_password3 = new unsigned char[GetBlockSize(m_pwLength3)];  //DK 
		m_pwValid3 = TRUE;  //DK 
		memcpy(m_password3, stuffhere.m_password3, GetBlockSize(m_pwLength3));  //DK 
   }  //DK 
   else  //DK 
   {  //DK 
	   m_password3 = NULL;  //DK 
		m_pwValid3 = FALSE;  //DK 
   }  //DK 

   m_notes = new unsigned char[GetBlockSize(m_notesLength)];
   m_notesValid = TRUE;
   memcpy(m_notes, stuffhere.m_notes, GetBlockSize(m_notesLength));

   memcpy((char*)m_salt, (char*)stuffhere.m_salt, SaltLength);
   m_saltValid = stuffhere.m_saltValid;
}

//Returns a plaintext name
BOOL
CItemData::GetName(CMyString &name) const
{
   return DecryptData(m_name, m_nLength, m_nameValid, &name);
}

CMyString
CItemData::GetName() const
{
   CMyString ret;
   (void) DecryptData(m_name, m_nLength, m_nameValid, &ret);
   return ret;
}


//Returns a plaintext password
BOOL
CItemData::GetPassword(CMyString &password, const int n) const  //DK 
{
	switch (n)  //DK 
   {  //DK 
   	case 1:  //DK 
   		return DecryptData(m_password, m_pwLength, m_pwValid, &password);
		break;  //DK 
   	case 2:  //DK 
		return DecryptData(m_password2, m_pwLength2, m_pwValid2, &password);  //DK 
		break;  //DK 
   	case 3:  //DK 
   		return DecryptData(m_password3, m_pwLength3, m_pwValid3, &password);  //DK 
		break;  //DK 
   }  //DK 
	return 0;
}

CMyString
CItemData::GetPassword(const int n) const  //DK 
{
   CMyString ret = "";
	switch (n)  //DK 
   {  //DK 
   	case 1:  //DK 
   		(void) DecryptData(m_password, m_pwLength, m_pwValid, &ret);
		break;  //DK 
   	case 2:  //DK 
		(void) DecryptData(m_password2, m_pwLength2, m_pwValid2, &ret);  //DK 
		break;  //DK 
   	case 3:  //DK 
   		(void) DecryptData(m_password3, m_pwLength3, m_pwValid3, &ret);  //DK 
		break;  //DK 
   }  //DK 
   return ret;
}

//Returns a plaintext notes
BOOL
CItemData::GetNotes(CMyString &notes) const
{
   return DecryptData(m_notes, m_notesLength, m_notesValid, &notes);
}

CMyString
CItemData::GetNotes() const
{
   CMyString ret;
   (void) DecryptData(m_notes, m_notesLength, m_notesValid, &ret);
   return ret;
}


//Encrypts a plaintext name and stores it in m_name
BOOL
CItemData::SetName(const CMyString &name)
{
   return EncryptData(name, &m_name, &m_nLength, m_nameValid);
}

//Encrypts a plaintext password and stores it in m_password
BOOL
CItemData::SetPassword(const CMyString &password, const int n)  //DK 
{
   switch (n)  //DK 
   {  //DK 
   	case 1:  //DK 
			return EncryptData(password, &m_password, &m_pwLength, m_pwValid);
			break;  //DK 
   	case 2:  //DK 
			return EncryptData(password, &m_password2, &m_pwLength2, m_pwValid2);  //DK 
			break;  //DK 
   	case 3:  //DK 
			return EncryptData(password, &m_password3, &m_pwLength3, m_pwValid3);  //DK 
			break;  //DK
   }  //DK 
   return 0;
}

//Encrypts a plaintext password and stores it in m_password
BOOL
CItemData::SetPassword(const CMyString &password)
{

   return EncryptData(password, &m_password, &m_pwLength, m_pwValid);

}

//Encrypts plaintext notes and stores them in m_notes
BOOL
CItemData::SetNotes(const CMyString &notes)
{
   return EncryptData(notes, &m_notes, &m_notesLength, m_notesValid);
}

//Deletes stuff
CItemData::~CItemData()
{
   if (m_nameValid == TRUE)
   {
      delete [] m_name;
      m_nameValid = FALSE;
   }
   if (m_pwValid == TRUE)
   {
      delete [] m_password;
      m_pwValid = FALSE;
   }
   if (m_pwValid2 == TRUE)  //DK 
   {  //DK 
      delete [] m_password2;  //DK 
      m_pwValid2 = FALSE;  //DK 
   }  //DK 
   if (m_pwValid3 == TRUE)  //DK 
   {  //DK 
      delete [] m_password3;  //DK 
      m_pwValid3 = FALSE;  //DK 
   }  //DK 
   if (m_notesValid == TRUE)
   {
      delete [] m_notes;
      m_notesValid = FALSE;
   }
}

//Encrypts the thing in plain to the variable cipher - alloc'd here
BOOL
CItemData::EncryptData(const CMyString &plain,
                       unsigned char **cipher,
                       int *cLength,
                       BOOL &valid)
{
  const LPCSTR plainstr = (const LPCSTR)plain; // use of CString::operator LPCSTR
  int result = EncryptData((const unsigned char*)plainstr,
                            plain.GetLength(),
                            cipher,
                            cLength,
                            valid);
   return result;
}


BlowFish *
CItemData::MakeBlowFish() const
{
  ASSERT(m_saltValid);
  LPCSTR passstr = LPCSTR(app.m_passkey);

  return ::MakeBlowFish((const unsigned char *)passstr, app.m_passkey.GetLength(),
			m_salt, SaltLength);
}



BOOL
CItemData::EncryptData(const unsigned char *plain,
                       int plainlength,
                       unsigned char **cipher,
                       int *cLength,
                       BOOL &valid)
{
  // Note that the m_salt member is set here, and read in DecryptData,
  // hence this can't be const, but DecryptData can

   if (valid == TRUE)
   {
      delete [] *cipher;
      valid = FALSE;
   }
	
   //Figure out the length of the ciphertext (round for blocks)
   *cLength = plainlength;
   int BlockLength = GetBlockSize(*cLength);

   *cipher = new unsigned char[BlockLength];
   if (*cipher == NULL)
      return FALSE;
   valid = TRUE;
   int x;

   if (m_saltValid == FALSE)
   {
      for (x=0;x<SaltLength;x++)
         m_salt[x] = newrand();
      m_saltValid = TRUE;
   }

   BlowFish *Algorithm = MakeBlowFish();

   unsigned char *tempmem = new unsigned char[BlockLength];
   // invariant: BlockLength >= plainlength
   memcpy((char*)tempmem, (char*)plain, plainlength);

   //Fill the unused characters in with random stuff
   for (x=plainlength; x<BlockLength; x++)
      tempmem[x] = newrand();

   //Do the actual encryption
   for (x=0; x<BlockLength; x+=8)
      Algorithm->Encrypt(tempmem+x, *cipher+x);

   delete Algorithm;
   delete [] tempmem;

   return TRUE;
}

//This is always used for preallocated data - not elegant, but who cares
BOOL
CItemData::DecryptData(const unsigned char *cipher,
                       int cLength,
                       BOOL valid,
                       unsigned char *plain,
                       int plainlength) const
{

	if (valid == FALSE) // check here once instead of in each caller to DecryptData
		return FALSE;

   int BlockLength = GetBlockSize(cLength);

   BlowFish *Algorithm = MakeBlowFish();
	
   unsigned char *tempmem = new unsigned char[BlockLength];

   int x;
   for (x=0;x<BlockLength;x+=8)
      Algorithm->Decrypt(cipher+x, tempmem+x);

   delete Algorithm;

   for (x=0;x<cLength;x++)
      if (x<plainlength)
         plain[x] = tempmem[x];

   delete [] tempmem;

   return TRUE;
}

//Decrypts the thing pointed to by cipher into plain
BOOL
CItemData::DecryptData(const unsigned char *cipher,
                       int cLength,
                       BOOL valid,
                       CMyString *plain) const
{
	*plain = "";  //DK
  if (valid == FALSE) // check here once instead of in each caller to DecryptData
    return FALSE;

   int BlockLength = GetBlockSize(cLength);
	
   unsigned char *plaintxt = (unsigned char*)plain->GetBuffer(BlockLength+1);

   BlowFish *Algorithm = MakeBlowFish();
   int x;
   for (x=0;x<BlockLength;x+=8)
      Algorithm->Decrypt(cipher+x, plaintxt+x);

   delete Algorithm;

   //ReleaseBuffer does a strlen, so 0s will be truncated
   for (x=cLength;x<BlockLength;x++)
      plaintxt[x] = 0;
   plaintxt[BlockLength] = 0;

   plain->ReleaseBuffer();

   return TRUE;
}


//Called by the constructors
void CItemData::InitStuff()
{
   m_nLength = 0;
   m_pwLength = 0;
   m_pwLength2 = 0;  //DK 
   m_pwLength3 = 0;  //DK 
   m_notesLength = 0;
	
   m_name = NULL;
   m_nameValid = FALSE;

   m_password = NULL;	
   m_password2 = NULL;  //DK 
   m_password3 = NULL;  //DK 
   m_pwValid = FALSE;
   m_pwValid2 = FALSE;  //DK 
   m_pwValid3 = FALSE;  //DK 

   m_notes = NULL;
   m_notesValid = FALSE;

   m_saltValid = FALSE;
}


//Returns the number of bytes of 8 byte blocks needed to store 'size' bytes
int
CItemData::GetBlockSize(int size) const
{
   return (int)ceil((double)size/8.0) * 8;
}

CItemData&
CItemData::operator=(const CItemData &second)
{
   //Check for self-assignment
   if (&second != this)
   {
      if (m_nameValid == TRUE)
      {
         delete [] m_name;
         m_nameValid = FALSE;
      }
      if (m_pwValid == TRUE)
      {
         delete [] m_password;
         m_pwValid = FALSE;
      }
      if (m_pwValid2 == TRUE)  //DK 
      {  //DK 
         delete [] m_password2;  //DK 
         m_pwValid2 = FALSE;  //DK 
      }  //DK 
      if (m_pwValid3 == TRUE)  //DK 
      {  //DK 
         delete [] m_password3;  //DK 
         m_pwValid3 = FALSE;  //DK 
      }  //DK 
      if (m_notesValid == TRUE)
      {
         delete [] m_notes;
         m_notesValid = FALSE;
      }
		
      m_nLength = second.m_nLength;
      m_pwLength = second.m_pwLength;
      m_pwLength2 = second.m_pwLength2;  //DK 
      m_pwLength3 = second.m_pwLength3;  //DK 
      m_notesLength = second.m_notesLength;

      m_name = new unsigned char[GetBlockSize(m_nLength)];
      m_nameValid = TRUE;
      memcpy(m_name, second.m_name, GetBlockSize(m_nLength));
		
      m_password = new unsigned char[GetBlockSize(m_pwLength)];
      m_pwValid = TRUE;
      memcpy(m_password, second.m_password, GetBlockSize(m_pwLength));

		if (m_pwLength2 > 0)  //DK
		{
			m_password2 = new unsigned char[GetBlockSize(m_pwLength2)];  //DK
			m_pwValid2 = TRUE;  //DK
			memcpy(m_password2, second.m_password2, GetBlockSize(m_pwLength2));  //DK
		}
		else
		{
			m_password2 = NULL;  //DK
			m_pwValid2 = FALSE;  //DK
		}

		if (m_pwLength3 > 0)  //DK
		{
			m_password3 = new unsigned char[GetBlockSize(m_pwLength3)];  //DK
			m_pwValid3 = TRUE;  //DK
			memcpy(m_password3, second.m_password3, GetBlockSize(m_pwLength3));  //DK
		}
		else
		{
			m_password3 = NULL;  //DK
			m_pwValid3 = FALSE;  //DK
		}

      m_notes = new unsigned char[GetBlockSize(m_notesLength)];
      m_notesValid = TRUE;
      memcpy(m_notes, second.m_notes, GetBlockSize(m_notesLength));

      memcpy((char*)m_salt, (char*)second.m_salt, SaltLength);
      m_saltValid = second.m_saltValid;
   }

   return *this;
}

//TODO: "General System Fault. Please sacrifice a goat 
//and two chickens to continue."

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
