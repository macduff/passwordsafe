/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#pragma once

// RUEList.h
//-----------------------------------------------------------------------------

#include <deque>
#include <vector>
#include "corelib/ItemData.h"
#include "corelib/StringX.h"
#include "corelib/PWScore.h"
#include "corelib/UUIDGen.h"

using namespace std;

//-----------------------------------------------------------------------------

/*
* CRUEList is a class that contains the recently used entries
*/

// Following is Most Recent Entry field separator for dynamic menu:
#define MRE_FS L"\xbb"

// Recent Entry structure for m_RUEList
struct RUEntry {
  RUEntry() {}
  RUEntry(const uuid_array_t &RUEuuid);
  bool operator() (const RUEntry &); // for find_if
  uuid_array_t RUEuuid;
};

struct RUEntryStringImage {
  StringX string;
  int image;
};

// identifies menu owner-draw data as mine
const LONG RUEMENUITEMID = MAKELONG(MAKEWORD('R', 'U'),MAKEWORD('E', 'M'));

// private struct: one of these for each owner-draw menu item
struct CRUEItemData {
  long     magicNum;      // magic number identifying me
  int      nImage;        // index of button image in image list
  CRUEItemData()          { magicNum = RUEMENUITEMID; }
  BOOL     IsRUEID()      { return magicNum == RUEMENUITEMID; }
};

typedef deque<RUEntry> RUEList;
typedef RUEList::iterator RUEListIter;
typedef RUEList::const_iterator RUEListConstIter;

class DboxMain;

class CRUEList
{
public:
  // Construction/Destruction/operators
  CRUEList();
  ~CRUEList() {}

  CRUEList& operator=(const CRUEList& second);

  // Data retrieval
  size_t GetCount() const {return m_RUEList.size();}
  size_t GetMax() const {return m_maxentries;}
  bool GetAllMenuItemStrings(vector<RUEntryStringImage> &) const;
  bool GetPWEntry(size_t, CItemData &); // NOT const!

  // Data setting
  void SetMax(size_t);
  void ClearEntries() {m_RUEList.clear();}
  bool AddRUEntry(const uuid_array_t &);
  bool DeleteRUEntry(size_t);
  bool DeleteRUEntry(const uuid_array_t &);

private:
  PWScore &m_core;    // Dboxmain's m_core (which = app.m_core!)
  size_t m_maxentries;
  RUEList m_RUEList;  // Recently Used Entry History List
  DboxMain *m_pDbx;
};

//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
