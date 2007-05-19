/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */

// UnknownRecordField.h

#pragma once

#include "ItemData.h"
#include "ItemField.h"
#include <vector>

// Unknown Record Field structure
struct UnknownRecordFieldEntry {
  unsigned char uc_Type;
  size_t st_Length;
  CItemField if_UField;

  UnknownRecordFieldEntry(unsigned char type) :uc_Type(0), st_Length(0), if_UField(type) {}

  // copy c'tor and assignment operator, standard idioms
  UnknownRecordFieldEntry(const UnknownRecordFieldEntry &that)
    : uc_Type(that.uc_Type), st_Length(that.st_Length), if_UField(that.if_UField) {}


  UnknownRecordFieldEntry &operator=(const UnknownRecordFieldEntry &that)
    { if (this != &that) {
        uc_Type = that.uc_Type;
        st_Length = that.st_Length;
        if_UField = that.if_UField;
      }
      return *this;
    }
};

typedef std::vector<UnknownRecordFieldEntry> UnknownRecordFieldList;