/// \file PWCharPool.h
//-----------------------------------------------------------------------------

#ifndef PWCharPool_h
#define PWCharPool_h

#include <afxtempl.h> // for CList
#include "MyString.h"

//-----------------------------------------------------------------------------

typedef enum
{
   PWC_UNKNOWN,
   PWC_LOWER,
   PWC_UPPER,
   PWC_DIGIT,
   PWC_SYMBOL
} PWCHARTYPE;

//-----------------------------------------------------------------------------

class CPasswordCharBlock
{
public:
   CPasswordCharBlock();
   CPasswordCharBlock(const CPasswordCharBlock &);
   void           SetStr(const TCHAR* str);
   void           SetLength(size_t len);
   void           SetType(PWCHARTYPE type);
   const TCHAR*   GetStr(void) const;
   PWCHARTYPE     GetType(void) const;
   size_t         GetLength(void) const;

   CPasswordCharBlock& operator=(const CPasswordCharBlock& second);

protected:
   PWCHARTYPE     m_type;
   size_t         m_length;
   const TCHAR*   m_str; // externally allocated/deallocated, never copied
};

//-----------------------------------------------------------------------------

class CPasswordCharPool
{
public:
   CPasswordCharPool::CPasswordCharPool(UINT pwlen,
					BOOL uselowercase, BOOL useuppercase,
					BOOL usedigits, BOOL usesymbols,
					BOOL easyvision);
   char     GetRandomChar(PWCHARTYPE& type) const;
   size_t   GetLength(void) const;
   CMyString MakePassword() const;


private:
   static const TCHAR std_lowercase_chars[];
   static const TCHAR std_uppercase_chars[];
   static const TCHAR std_digit_chars[];
   static const TCHAR std_symbol_chars[];
   static const TCHAR easyvision_lowercase_chars[];
   static const TCHAR easyvision_uppercase_chars[];
   static const TCHAR easyvision_digit_chars[];
   static const TCHAR easyvision_symbol_chars[];

   static const size_t std_lowercase_len;
   static const size_t std_uppercase_len;
   static const size_t std_digit_len;
   static const size_t std_symbol_len;
   static const size_t easyvision_lowercase_len;
   static const size_t easyvision_uppercase_len;
   static const size_t easyvision_digit_len;
   static const size_t easyvision_symbol_len;

   CList<CPasswordCharBlock,CPasswordCharBlock> m_pool;
   UINT m_length;
   // Following state vars set by ctor, used by MakePassword()
   const UINT m_pwlen;
   const BOOL m_uselowercase;
   const BOOL m_useuppercase;
   const BOOL m_usedigits;
   const BOOL m_usesymbols;
};

#endif

