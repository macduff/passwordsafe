/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */

#include "SMemFile.h"
#include "util.h"
#include "UTF8Conv.h"

/*
 * Override normal CMemFile to allow the contents to be trashed
 * during Realloc and Free.  Otherwise identical to CMemFile
 */

BYTE* CSMemFile::Alloc(SIZE_T nBytes)
{
  BYTE* lpNewMem = (BYTE *)malloc(nBytes);

  if (lpNewMem == NULL) {
    m_size = 0;
    TRACE(_T("SMemfile:Alloc Size=%d FAILED\n"), nBytes);
    return NULL;
  }

  TRACE(_T("SMemfile:Alloc Size=%d\n"), nBytes);
  m_size = nBytes;
  return lpNewMem;
}

BYTE* CSMemFile::Realloc(BYTE* lpOldMem, SIZE_T nBytes)
{
  if (nBytes == 0) {
    trashMemory(lpOldMem, m_size);
    Free(lpOldMem);
    m_size = 0;
    return NULL;
  }

  size_t old_size = _msize((void *)lpOldMem);
  ASSERT(m_size == old_size);
  BYTE* lpNewMem = (BYTE *)malloc(nBytes);

  if (lpNewMem == NULL) {
    TRACE(_T("SMemfile:Realloc Old size=%d, New Size=%d FAILED\n"), old_size, nBytes);
    trashMemory(lpOldMem, old_size);
    free(lpOldMem);
    m_size = 0;
    return NULL;
  }

  memcpy((void *)lpNewMem, (void *)lpOldMem, old_size);
  trashMemory(lpOldMem, old_size);
  free(lpOldMem);

  TRACE(_T("SMemfile:Realloc Old size=%d, New Size=%d\n"), old_size, nBytes);
  m_size = nBytes;
  return lpNewMem;
}

void CSMemFile::Free(BYTE* lpMem)
{
  size_t mem_size = _msize((void *)lpMem);
  ASSERT(m_size == mem_size);
  m_size = 0;
  TRACE(_T("SMemfile:Free Memory at %p, Size=%d\n"), lpMem, mem_size);
  if (lpMem == NULL)
    return;

  if (mem_size != 0)
    trashMemory(lpMem, mem_size);

  free(lpMem);
}

size_t
CSMemFile::WriteField(unsigned char type, const CString &data)
{
  CUTF8Conv utf8conv;
  bool status;
  const unsigned char *utf8;
  int utf8Len;
  status = utf8conv.ToUTF8(data, utf8, utf8Len);
  if (!status)
    TRACE(_T("ToUTF8(%s) failed\n"), data);
  return WriteField(type, utf8, (unsigned int)utf8Len);
}

size_t
CSMemFile::WriteField(unsigned char type, const unsigned char *data,
                      int length)
{
  Write((void *)&type, sizeof(type));
  Write((void *)&length, sizeof(length));
  Write(data, length);
  return (sizeof(type) + sizeof(length) + length);
}

size_t
CSMemFile::ReadField(unsigned char &type, unsigned char* &data,
                     int &length)
{
  int numread;
  numread = Read((void *)&type, sizeof(type));
  ASSERT(numread == sizeof(type));
  numread = Read((void *)&length, sizeof(length));
  ASSERT(numread == sizeof(length));
  data = new unsigned char[length + 1];
  numread = Read(data, length);
  ASSERT(numread == length);
  data[length] = '\0';
  return (sizeof(type) + sizeof(length) + length);
}
