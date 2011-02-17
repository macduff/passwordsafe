/*
* Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
// ExpiredList.h
//-----------------------------------------------------------------------------

#ifndef __EXPIREDLIST_H
#define __EXPIREDLIST_H

#include "StringX.h"
#include "UUIDGen.h"
#include "ItemData.h"

#include <vector>

struct ExpPWEntry {
  ExpPWEntry(const CItemData &ci);
  ExpPWEntry(const ExpPWEntry &ee);
  ExpPWEntry &operator=(const ExpPWEntry &that);

  uuid_array_t uuid;
  StringX group;
  StringX title;
  StringX user;
  StringX expirylocdate;  // user's long date/time  - format displayed to user in UI
  StringX expiryexpdate;  // "YYYY/MM/DD HH:MM:SS"  - format copied to clipboard - best for sorting
  time_t expirytttXTime;
  CItemData::EntryType et; // Used to select image for display to user e.g.
                           // 'warn will expire' or 'has expired' &
                           // 'normal, aliasbase or shortcut base' entry
};

class ExpiredList: public std::vector<ExpPWEntry>
{
public:
  void Add(const CItemData &ci);
  void Update(const CItemData &ci) {Remove(ci); Add(ci);}
  void Remove(const CItemData &ci);
  ExpiredList GetExpired(const int &idays); // return a subset
};

struct ee_equal_uuid {
  ee_equal_uuid(uuid_array_t const& uuid)
  {
    memcpy(m_uuid, uuid, sizeof(uuid_array_t));
  }

  bool operator()(const ExpPWEntry &ee) const
  { return memcmp(m_uuid, ee.uuid, sizeof(uuid_array_t)) == 0; }

  uuid_array_t m_uuid;
};

#endif /* __EXPIREDLIST_H */
