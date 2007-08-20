/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license.php
 */

#include "DDSupport.h"
#include "corelib/MyString.h"
#include "corelib/util.h"
#include "corelib/UUIDGen.h"

void CDDObject::DDSerialize(CSMemFile &outDDmemfile)
{
  m_item.DDSerialize(outDDmemfile);
}

void CDDObject::DDUnSerialize(CSMemFile &inDDmemfile)
{
  m_item.DDUnSerialize(inDDmemfile);
}

void CDDObList::DDSerialize(CSMemFile &outDDmemfile)
{
  // NOTE:  Do not call the base class!
  int nCount;
  POSITION Pos;
  CDDObject* pDDObject;

  nCount = GetCount();

  outDDmemfile.Write((void *)&nCount, sizeof(nCount));
  outDDmemfile.Write((void *)&m_bDragNode, sizeof(bool));

  Pos = GetHeadPosition();
  while (Pos != NULL) {
    pDDObject = (CDDObject *)GetNext(Pos);
    pDDObject->DDSerialize(outDDmemfile);
  }
}

void CDDObList::DDUnSerialize(CSMemFile &inDDmemfile)
{
  ASSERT(GetCount() == 0);
  int n, nCount;
  CDDObject* pDDObject;

  inDDmemfile.Read((void *)&nCount, sizeof(nCount));
  inDDmemfile.Read((void *)&m_bDragNode, sizeof(bool));

  for (n = 0; n < nCount; n++) {
    pDDObject = new CDDObject();
    pDDObject->DDUnSerialize(inDDmemfile);
    AddTail(pDDObject);
  }
}
