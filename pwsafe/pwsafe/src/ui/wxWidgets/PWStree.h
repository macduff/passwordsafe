/*
 * Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */
/** \file
* 
*/

#ifndef _PWSTREECTRL_H_
#define _PWSTREECTRL_H_


/*!
 * Includes
 */

////@begin includes
#include "wx/treectrl.h"
////@end includes
#include "corelib/ItemData.h"
#include "corelib/PWScore.h"
#include "corelib/UUIDGen.h"
#include <map>

/*!
 * Forward declarations
 */

////@begin forward declarations
class PWSTreeCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_TREECTRL 10061
#define SYMBOL_PWSTREECTRL_STYLE wxTR_EDIT_LABELS|wxTR_HAS_BUTTONS |wxTR_HIDE_ROOT|wxTR_SINGLE
#define SYMBOL_PWSTREECTRL_IDNAME ID_TREECTRL
#define SYMBOL_PWSTREECTRL_SIZE wxSize(100, 100)
#define SYMBOL_PWSTREECTRL_POSITION wxDefaultPosition
////@end control identifiers

typedef std::map<CUUIDGen, wxTreeItemId, CUUIDGen::ltuuid> UUIDTIMapT;

/*!
 * PWSTreeCtrl class declaration
 */

class PWSTreeCtrl: public wxTreeCtrl
{    
  DECLARE_CLASS( PWSTreeCtrl )
  DECLARE_EVENT_TABLE()

public:
  /// Constructors
  PWSTreeCtrl(PWScore &core);
  PWSTreeCtrl(wxWindow* parent, PWScore &core, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS);

  /// Creation
  bool Create(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS);

  /// Destructor
  ~PWSTreeCtrl();

  /// Initialises member variables
  void Init();

  /// Creates the controls and sizers
  void CreateControls();

////@begin PWSTreeCtrl event handler declarations

  /// wxEVT_COMMAND_TREE_ITEM_ACTIVATED event handler for ID_TREECTRL
  void OnTreectrlItemActivated( wxTreeEvent& event );

  /// wxEVT_RIGHT_DOWN event handler for ID_TREECTRL
  void OnRightClick( wxTreeEvent& event );

  /// wxEVT_CHAR event handler for ID_TREECTRL
  void OnChar( wxKeyEvent& event );

////@end PWSTreeCtrl event handler declarations
  void OnGetToolTip( wxTreeEvent& event ); // Added manually

////@begin PWSTreeCtrl member function declarations

////@end PWSTreeCtrl member function declarations
  void Clear() {DeleteAllItems(); m_item_map.clear();} // consistent name w/PWSgrid
  void AddItem(const CItemData &item);
  void UpdateItem(const CItemData &item);
  CItemData *GetItem(const wxTreeItemId &id) const;
  wxTreeItemId Find(const uuid_array_t &uuid) const;
  wxTreeItemId Find(const CItemData &item) const;
  bool Remove(const uuid_array_t &uuid); // only remove from tree, not from m_core
  void SelectItem(const CUUIDGen& uuid);

 private:
  bool ExistsInTree(wxTreeItemId node,
                    const StringX &s, wxTreeItemId &si);
  wxTreeItemId AddGroup(const StringX &group);
  wxString ItemDisplayString(const CItemData &item) const;
  wxString GetPath(const wxTreeItemId &node) const;
////@begin PWSTreeCtrl member variables
////@end PWSTreeCtrl member variables
  PWScore &m_core;
  UUIDTIMapT m_item_map; // given a uuid, find the tree item pronto!
};

#endif
  // _PWSTREECTRL_H_
