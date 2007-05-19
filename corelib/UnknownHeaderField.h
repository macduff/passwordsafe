/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */

// UnknownHeaderField.h

#pragma once

#include "Util.h"

// Unknown Field structure
struct UnknownHeaderFieldEntry {
  unsigned char uc_Type;
  size_t st_Length;
  unsigned char * uc_pUField;

  UnknownHeaderFieldEntry() :uc_Type(0), st_Length(0), uc_pUField(NULL) {}
  ~UnknownHeaderFieldEntry()
    {
      if (st_Length > 0 && uc_pUField != NULL) {
        trashMemory((void *)uc_pUField, st_Length);
        delete[] uc_pUField;
        st_Length = 0;
        uc_pUField = NULL;
      }
    }

  // copy c'tor and assignment operator, standard idioms
  UnknownHeaderFieldEntry(const UnknownHeaderFieldEntry &that)
    : uc_Type(that.uc_Type), st_Length(that.st_Length)
  {
    if (that.st_Length > 0 && that.uc_pUField != NULL) {
      uc_pUField =  new unsigned char[st_Length + sizeof(TCHAR)];
      memset(uc_pUField, 0x00, st_Length + sizeof(TCHAR));
      memcpy(uc_pUField, that.uc_pUField, st_Length);
    } else {
      st_Length = 0;
      uc_pUField = NULL;
    }
  }

  UnknownHeaderFieldEntry &operator=(const UnknownHeaderFieldEntry &that)
    { if (this != &that) {
        uc_Type = that.uc_Type;
        st_Length = that.st_Length;
        if (st_Length > 0 && that.uc_pUField != NULL) {
          uc_pUField =  new unsigned char[st_Length + sizeof(TCHAR)];
          memset(uc_pUField, 0x00, st_Length + sizeof(TCHAR));
          memcpy(uc_pUField, that.uc_pUField, st_Length);
        } else {
          st_Length = 0;
          uc_pUField = NULL;
        }
      }
      return *this;
    }
};

typedef std::vector<UnknownHeaderFieldEntry> UnknownHeaderFieldList;