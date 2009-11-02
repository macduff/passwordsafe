/*
 * Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file PWSTreeCtrl.cpp
* 
*/
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
////@end includes
#include <utility> // for make_pair
#include <vector>

#include "PWStree.h"
#include "pwsdca.h"
#include "corelib/PWSprefs.h"

////@begin XPM images
////@end XPM images


/*!
 * PWSTreeCtrl type definition
 */

IMPLEMENT_CLASS( PWSTreeCtrl, wxTreeCtrl )


/*!
 * PWSTreeCtrl event table definition
 */

BEGIN_EVENT_TABLE( PWSTreeCtrl, wxTreeCtrl )

////@begin PWSTreeCtrl event table entries
  EVT_TREE_ITEM_ACTIVATED( ID_TREECTRL, PWSTreeCtrl::OnTreectrlItemActivated )
  EVT_RIGHT_DOWN( PWSTreeCtrl::OnRightDown )

////@end PWSTreeCtrl event table entries

END_EVENT_TABLE()


// helper class to match CItemData with wxTreeItemId
class PWTreeItemData : public wxTreeItemData
{
public:
  PWTreeItemData(const CItemData &item)
  {
    item.GetUUID(m_uuid);
  }
  const uuid_array_t &GetUUID() const {return m_uuid;}
private:
  uuid_array_t m_uuid;
};

/*!
 * PWSTreeCtrl constructors
 */

PWSTreeCtrl::PWSTreeCtrl(PWScore &core) : m_core(core)
{
  Init();
}

PWSTreeCtrl::PWSTreeCtrl(wxWindow* parent, PWScore &core,
                         wxWindowID id, const wxPoint& pos,
                         const wxSize& size, long style) : m_core(core)
{
  Init();
  Create(parent, id, pos, size, style);
}


/*!
 * PWSTreeCtrl creator
 */

bool PWSTreeCtrl::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
////@begin PWSTreeCtrl creation
  wxTreeCtrl::Create(parent, id, pos, size, style);
  CreateControls();
////@end PWSTreeCtrl creation
  return true;
}


/*!
 * PWSTreeCtrl destructor
 */

PWSTreeCtrl::~PWSTreeCtrl()
{
////@begin PWSTreeCtrl destruction
////@end PWSTreeCtrl destruction
}


/*!
 * Member initialisation
 */

void PWSTreeCtrl::Init()
{
////@begin PWSTreeCtrl member initialisation
////@end PWSTreeCtrl member initialisation
}


/*!
 * Control creation for PWSTreeCtrl
 */

void PWSTreeCtrl::CreateControls()
{    
////@begin PWSTreeCtrl content construction
////@end PWSTreeCtrl content construction
}

// XXX taken from Windows PWSTreeCtrl.cpp
// XXX move to corelib
static StringX GetPathElem(StringX &path)
{
  // Get first path element and chop it off, i.e., if
  // path = "a.b.c.d"
  // will return "a" and path will be "b.c.d"
  // (assuming GROUP_SEP is '.')
  const char GROUP_SEP = '.';

  StringX retval;
  StringX::size_type N = path.find(GROUP_SEP);
  if (N == StringX::npos) {
    retval = path;
    path = _T("");
  } else {
    const StringX::size_type Len = path.length();
    retval = path.substr(0, N);
    path = path.substr(Len - N - 1);
  }
  return retval;
}

bool PWSTreeCtrl::ExistsInTree(wxTreeItemId node,
                               const StringX &s, wxTreeItemId &si)
{
  // returns true iff s is a direct descendant of node
  wxTreeItemIdValue cookie;
  wxTreeItemId ti = GetFirstChild(node, cookie);

  while (ti) {
    const wxString itemText = GetItemText(ti);
    if (itemText == s.c_str()) {
      si = ti;
      return true;
    }
    ti = GetNextSibling(ti);
  }
  return false;
}


wxTreeItemId PWSTreeCtrl::AddGroup(const StringX &group)
{
  wxTreeItemId ti = GetRootItem();
  if (!ti.IsOk())
    ti=AddRoot(wxString());

  // Add a group at the end of path
  wxTreeItemId si;
  if (!group.empty()) {
    StringX path = group;
    StringX s;
    do {
      s = GetPathElem(path);
      if (!ExistsInTree(ti, s, si)) {
        ti = AppendItem(ti, s.c_str());
        // SetItemImage(ti, CPWTreeCtrl::NODE, CPWTreeCtrl::NODE);
      } else
        ti = si;
    } while (!path.empty());
  }
  return ti;
}


wxString PWSTreeCtrl::ItemDisplayString(const CItemData &item) const
{
  PWSprefs *prefs = PWSprefs::GetInstance();
  const wxString title = item.GetTitle().c_str();
  wxString disp = title;

  if (prefs->GetPref(PWSprefs::ShowUsernameInTree)) {
    const wxString user = item.GetUser().c_str();
    if (!user.empty())
      disp += _T(" [") + user + _("]");
  }

  if (prefs->GetPref(PWSprefs::ShowPasswordInTree)) {
    const wxString passwd = item.GetPassword().c_str();
    if (!passwd.empty())
      disp += _T(" {") + passwd + _("}");
  }

  return disp;
}

wxString PWSTreeCtrl::GetPath(const wxTreeItemId &node) const
{
  wxString retval;
  std::vector<wxString> v;
  const wxTreeItemId root = GetRootItem();
  wxTreeItemId parent = GetItemParent(node);

  while (parent != root) {
    v.push_back(GetItemText(parent));
    parent = GetItemParent(parent);
  }
  std::vector<wxString>::reverse_iterator iter;
  for(iter = v.rbegin(); iter != v.rend(); iter++) {
    retval += *iter;
    if ((iter + 1) != v.rend())
      retval += _(".");
  }
  return retval;
}

void PWSTreeCtrl::UpdateItem(const CItemData &item)
{
  const wxTreeItemId node = Find(item);
  if (node.IsOk()) {
    const wxString oldGroup = GetPath(node);
    const wxString newGroup = item.GetGroup().c_str();
    if (oldGroup == newGroup) {
      const wxString disp = ItemDisplayString(item);
      SetItemText(node, disp);
    } else { // uh-oh - group's changed
      uuid_array_t uuid;
      item.GetUUID(uuid);
      // remove old item
      m_item_map.erase(CUUIDGen(uuid));
      Delete(node);
      // add new group
      AddItem(item);
    }
    Update();
  }
}


void PWSTreeCtrl::AddItem(const CItemData &item)
{
  wxTreeItemData *data = new PWTreeItemData(item);
  wxTreeItemId gnode = AddGroup(item.GetGroup());
  const wxString disp = ItemDisplayString(item);
  wxTreeItemId titem = AppendItem(gnode, disp, -1, -1, data);
  uuid_array_t uuid;
  item.GetUUID(uuid);
  m_item_map.insert(std::make_pair(CUUIDGen(uuid), titem));
}

const CItemData *PWSTreeCtrl::GetItem(const wxTreeItemId &id) const
{
  if (!id.IsOk())
    return NULL;

  PWTreeItemData *itemData = dynamic_cast<PWTreeItemData *>(GetItemData(id));
  // return if a group is selected
  if (itemData == NULL)
    return NULL;

  ItemListConstIter citer = m_core.Find(itemData->GetUUID());
  if (citer == m_core.GetEntryEndIter())
    return NULL;
  return &citer->second;

}



wxTreeItemId PWSTreeCtrl::Find(const uuid_array_t &uuid) const
{
  wxTreeItemId fail;
  CUUIDGen cuuid(uuid);
  UUIDTIMapT::const_iterator iter = m_item_map.find(cuuid);
  if (iter != m_item_map.end())
    return iter->second;
  else
    return fail;
}

wxTreeItemId PWSTreeCtrl::Find(const CItemData &item) const
{
  uuid_array_t uuid;
  item.GetUUID(uuid);
  return Find(uuid);
}

bool PWSTreeCtrl::Remove(const uuid_array_t &uuid)
{
  wxTreeItemId id = Find(uuid);
  if (id.IsOk()) {
    m_item_map.erase(CUUIDGen(uuid));
    Delete(id);
    Refresh();
    Update();
    return true;
  } else {
    return false;
  }
}


/*!
 * wxEVT_COMMAND_TREE_ITEM_ACTIVATED event handler for ID_TREECTRL
 */

void PWSTreeCtrl::OnTreectrlItemActivated( wxTreeEvent& event )
{
  const CItemData *item = GetItem(event.GetItem());
  if (item != NULL)
    PWSdca::Doit(*item);
}


/*!
 * wxEVT_RIGHT_DOWN event handler for ID_TREECTRL
 */

void PWSTreeCtrl::OnRightDown( wxMouseEvent& event )
{
////@begin wxEVT_RIGHT_DOWN event handler for ID_TREECTRL in PWSTreeCtrl.
  // Before editing this code, remove the block markers.
  wxMessageBox(_("RightClick!"));
////@end wxEVT_RIGHT_DOWN event handler for ID_TREECTRL in PWSTreeCtrl. 
}

void PWSTreeCtrl::SelectItem(const CUUIDGen & uuid)
{
  uuid_array_t uuid_array;
  uuid.GetUUID(uuid_array);
  wxTreeItemId id = Find(uuid_array);
  if (id.IsOk())
      wxTreeCtrl::SelectItem(id);
}
