/// \file PWCharPool.h
//-----------------------------------------------------------------------------

#ifndef PWCharPool_h
#define PWCharPool_h

#include "PasswordSafe.h"

#include "ThisMfcApp.h"
#include "resource.h"


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
   CPasswordCharBlock::CPasswordCharBlock();
   void           SetStr(const TCHAR* str);
   void           SetLength(size_t len);
   void           SetType(PWCHARTYPE type);
   const TCHAR*   GetStr(void);
   PWCHARTYPE     GetType(void);
   size_t         GetLength(void);

   CPasswordCharBlock& operator=(const CPasswordCharBlock& second);

protected:
   PWCHARTYPE     m_type;
   size_t         m_length;
   const TCHAR*   m_str;
};

//-----------------------------------------------------------------------------

class CPasswordCharPool
{
public:
   CPasswordCharPool::CPasswordCharPool();
   void     SetPool(BOOL easyvision, BOOL uselower, BOOL useupper, BOOL usedigits, BOOL usesymbols);
   char     GetRandomChar(PWCHARTYPE* type);
   size_t   GetLength(void);


private:
   const TCHAR *std_lowercase_chars;
   const TCHAR *std_uppercase_chars;
   const TCHAR *std_digit_chars;
   const TCHAR *std_symbol_chars;
   const TCHAR *easyvision_lowercase_chars;
   const TCHAR *easyvision_uppercase_chars;
   const TCHAR *easyvision_digit_chars;
   const TCHAR *easyvision_symbol_chars;

   size_t std_lowercase_len;
   size_t std_uppercase_len;
   size_t std_digit_len;
   size_t std_symbol_len;
   size_t easyvision_lowercase_len;
   size_t easyvision_uppercase_len;
   size_t easyvision_digit_len;
   size_t easyvision_symbol_len;

   CList<CPasswordCharBlock,CPasswordCharBlock> m_pool;
   UINT m_length;
};

#endif

