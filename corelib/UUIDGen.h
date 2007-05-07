/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */
// UUIDGen.h
// Silly class for generating UUIDs
// Each instance has its own unique value, 
// which can be accessed as an array of bytes or as a human-readable
// ASCII string.
//

#ifndef __UUIDGEN_H
#define __UUIDGEN_H
#include <TCHAR.H>

typedef unsigned char uuid_array_t[16];
typedef TCHAR uuid_str_t[37]; //"204012e6-600f-4e01-a5eb-515267cb0d50"

#include "PwsPlatform.h"

class CUUIDGen {
 public:
  CUUIDGen(); // UUID generated at creation time
  CUUIDGen(const uuid_array_t &); // for storing an existing UUID
  ~CUUIDGen();
  void GetUUID(uuid_array_t &) const;
  void GetUUIDStr(uuid_str_t &str) const;
 private:
  UUID uuid;
};

#endif /* __UUIDGEN_H */
