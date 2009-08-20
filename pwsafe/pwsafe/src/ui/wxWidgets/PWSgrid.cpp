/*
 * Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */

/** \file pwsgrid.cpp
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
#include "PWSgrid.h"
#include "pwsdca.h"
//#include "../../corelib/PWScore.h"

////@begin XPM images
////@end XPM images


/*!
 * PWSGrid type definition
 */

IMPLEMENT_CLASS( PWSGrid, wxGrid )


/*!
 * PWSGrid event table definition
 */

BEGIN_EVENT_TABLE( PWSGrid, wxGrid )

////@begin PWSGrid event table entries
  EVT_GRID_CELL_RIGHT_CLICK( PWSGrid::OnCellRightClick )
  EVT_GRID_CELL_LEFT_DCLICK( PWSGrid::OnLeftDClick )

////@end PWSGrid event table entries

END_EVENT_TABLE()

//Notification from PWSCore when the password list is modified
void OnPasswordListModified(LPARAM lParam)
{
  reinterpret_cast<PWSGrid*>(lParam)->OnPasswordListModified();
}

/*!
 * PWSGrid constructors
 */

PWSGrid::PWSGrid(PWScore &core) : m_core(core)
{
  const bool bAdded = m_core.RegisterOnListModified(&::OnPasswordListModified, 
                                             reinterpret_cast<LPARAM>(this));
  wxCHECK_RET(bAdded, _S("Could not added callback from PWSCore to PWSGrid"));
  Init();
}

PWSGrid::PWSGrid(wxWindow* parent, PWScore &core,
                 wxWindowID id, const wxPoint& pos,
                 const wxSize& size, long style) : m_core(core)
{
  const bool bAdded = m_core.RegisterOnListModified(&::OnPasswordListModified, 
                                             reinterpret_cast<LPARAM>(this));
  wxCHECK_RET(bAdded, _S("Could not added callback from PWSCore to PWSGrid"));
  Init();
  Create(parent, id, pos, size, style);
}


/*!
 * PWSGrid creator
 */

bool PWSGrid::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
////@begin PWSGrid creation
  wxGrid::Create(parent, id, pos, size, style);
  //CreateControls();
////@end PWSGrid creation
  return true;
}


/*!
 * PWSGrid destructor
 */

PWSGrid::~PWSGrid()
{
////@begin PWSGrid destruction
////@end PWSGrid destruction
}


/*!
 * Member initialisation
 */

void PWSGrid::Init()
{
////@begin PWSGrid member initialisation
////@end PWSGrid member initialisation
}


/*!
 * Control creation for PWSGrid
 */

void PWSGrid::CreateControls()
{    
////@begin PWSGrid content construction
////@end PWSGrid content construction
  CreateGrid(0, 2, wxGrid::wxGridSelectRows);
  SetColLabelValue(0, _("Title"));
  SetColLabelValue(1, _("User"));
  SetRowLabelSize(0);
  int w,h;
  GetClientSize(&w, &h);
  int cw = w/2; // 2 = number of columns
  SetColSize(0, cw);
  SetColSize(1, cw);
}

void PWSGrid::OnPasswordListModified()
{
  m_row_map.clear();
  m_uuid_map.clear();
 
  ItemListConstIter iter;
  int row = 0;
  for (iter = m_core.GetEntryIter();
       iter != m_core.GetEntryEndIter();
       iter++) {
    AddItem(iter->second, row);
    row++;
  }
}

/*!
 * Should we show tooltips?
 */

bool PWSGrid::ShowToolTips()
{
  return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PWSGrid::GetBitmapResource( const wxString& name )
{
  // Bitmap retrieval
////@begin PWSGrid bitmap retrieval
  wxUnusedVar(name);
  return wxNullBitmap;
////@end PWSGrid bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PWSGrid::GetIconResource( const wxString& name )
{
  // Icon retrieval
////@begin PWSGrid icon retrieval
  wxUnusedVar(name);
  return wxNullIcon;
////@end PWSGrid icon retrieval
}

/*
//This function should be deleted.  It has been replaced by DeleteAllItems()
void PWSGrid::Clear()
{
  int N = GetNumberRows();
  if (N > 0)
    DeleteRows(0, N);
  m_row_map.clear();
  m_uuid_map.clear();
}
*/


void PWSGrid::DisplayItem(const CItemData &item, int row)
{
  wxString title = item.GetTitle().c_str();
  wxString user = item.GetUser().c_str();
  SetCellValue(row, 0, title);
  SetCellValue(row, 1, user);
}


void PWSGrid::AddItem(const CItemData &item, int row)
{
  DisplayItem(item, row);
  uuid_array_t uuid;
  item.GetUUID(uuid);
  m_row_map.insert(std::make_pair(row, CUUIDGen(uuid)));
  m_uuid_map.insert(std::make_pair(CUUIDGen(uuid), row));
}

void PWSGrid::UpdateItem(const CItemData &item)
{  
  uuid_array_t uuid;
  item.GetUUID(uuid);
  UUIDRowMapT::iterator iter = m_uuid_map.find(CUUIDGen(uuid));
  if (iter != m_uuid_map.end()) {
    int row = iter->second;
    DisplayItem(item, row);
  }
}

void PWSGrid::Remove(const uuid_array_t &uuid)
{
  UUIDRowMapT::iterator iter = m_uuid_map.find(CUUIDGen(uuid));
  if (iter != m_uuid_map.end()) {
    int row = iter->second;
    m_row_map.erase(row);
    m_uuid_map.erase(CUUIDGen(uuid));
    DeleteRows(row);
  }  
}

/*!
 * Returns the number of elements in its own books
 */
size_t PWSGrid::GetNumItems() const
{
	wxASSERT(m_uuid_map.size() == m_core.GetNumEntries());
    wxASSERT(m_row_map.size() == m_core.GetNumEntries());
	//return m_uuid_map.size();
	return m_core.GetNumEntries();
}

/*!
 * callback from PWSGridTable.  Removes the item(s) from its own books
 * as well as from PWSCore.  The display will be updated by an event
 * generated by PWSGridTable once this function returns.
 */
void PWSGrid::DeleteItems(int row, size_t numItems)
{
  for (size_t N = 0; N < numItems; ++N) {
    RowUUIDMapT::iterator iter = m_row_map.find(row);
    if (iter != m_row_map.end()) {
      UUIDRowMapT::iterator iter_uuid = m_uuid_map.find(iter->second);
      if (iter_uuid != m_uuid_map.end()) {
        uuid_array_t uuid;
        iter_uuid->first.GetUUID(uuid);
        ItemListIter citer = m_core.Find(uuid);
        if (citer != m_core.GetEntryEndIter())
          m_core.RemoveEntryAt(citer);
        m_uuid_map.erase(iter_uuid);
      }
      m_row_map.erase(iter);    
    }
  }
}

/*!
 * callback from PWSGridTable.  Removes all items from its own books
 * as well as from PWSCore.  The items from display grid will be 
 * removed by an event generated by PWSGridTable once this function
 * returns
 */
void PWSGrid::DeleteAllItems()
{
  m_uuid_map.clear();
  m_row_map.clear();
  m_core.ClearData();
}

/*!
 * wxEVT_GRID_CELL_RIGHT_CLICK event handler for ID_LISTBOX
 */

void PWSGrid::OnCellRightClick( wxGridEvent& event )
{
////@begin wxEVT_GRID_CELL_RIGHT_CLICK event handler for ID_LISTBOX in PWSGrid.
  // Before editing this code, remove the block markers.
  wxMessageBox(_("I'm right-clicked!"));
////@end wxEVT_GRID_CELL_RIGHT_CLICK event handler for ID_LISTBOX in PWSGrid. 
}

const CItemData *PWSGrid::GetItem(int row) const
{
  if (row < 0 || row > const_cast<PWSGrid *>(this)->GetNumberRows())
    return NULL;
  RowUUIDMapT::const_iterator iter = m_row_map.find(row);
  if (iter != m_row_map.end()) {
    uuid_array_t uuid;
    iter->second.GetUUID(uuid);
    ItemListConstIter citer = m_core.Find(uuid);
    if (citer == m_core.GetEntryEndIter())
      return NULL;
    return &citer->second;
  }
  return NULL;
}


/*!
 * wxEVT_GRID_CELL_LEFT_DCLICK event handler for ID_LISTBOX
 */

void PWSGrid::OnLeftDClick( wxGridEvent& event )
{
  const CItemData *item = GetItem(event.GetRow());
  if (item != NULL)
    PWSdca::Doit(*item);
}
