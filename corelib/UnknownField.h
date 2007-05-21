/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */

/**
 * UnknownFieldEntry - a small struct for keeping unsupported entry
 * types across read/write of the database, in order to be compatible
 * (1) with other implementations of the published format, and
 * (2) with future versions of PasswordSafe.
 */

#ifndef __UNKNOWNFIELD_H
#define __UNKNOWNFIELD_H

// Unknown Field structure
struct UnknownFieldEntry {
  unsigned char uc_Type;
  size_t st_length;
  unsigned char * uc_pUField;

  UnknownFieldEntry() :uc_Type(0), st_length(0), uc_pUField(0) {}
  UnknownFieldEntry(unsigned char t, size_t s, unsigned char *d);
  ~UnknownFieldEntry();
  // copy c'tor and assignment operator, standard idioms
  UnknownFieldEntry(const UnknownFieldEntry &that);
  UnknownFieldEntry &operator=(const UnknownFieldEntry &that);
};
#endif /* __UNKNOWNFIELD_H */
