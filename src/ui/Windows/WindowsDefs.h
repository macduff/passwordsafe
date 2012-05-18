/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

// Windows definitions cf. coredefs.h

#include "core/StringX.h"
#include "core/coredefs.h"

#include <map>

enum ViewType {LIST = 0, TREE = 1, EXPLORER = 2};

enum SplitterRow {INVALID_ROW = -1, TOP = 0, BOTTOM = 1};

class CItemData;

// Structure for access to entry in List View
struct st_PWLV_lParam {
  // HTREEITEM of this group's entry in the corresponding CTreeView entry.
  HTREEITEM hItem;
  // Pointer to entry, if it is an entry in ListView or NULL if a group.
  CItemData *pci;

 st_PWLV_lParam() : hItem(NULL), pci(NULL) {}

  st_PWLV_lParam(const st_PWLV_lParam &that)
  : hItem(that.hItem), pci(that.pci)
  {}

  st_PWLV_lParam &operator=(const st_PWLV_lParam &that)
  {
    if (this != &that) {
      hItem = that.hItem;
      pci = that.pci;
    }
    return *this;
  }
};

// Path_Compare is defined and documented in core/coredefs.h

typedef std::map<StringX, HTREEITEM, Path_Compare> PathMap;
typedef PathMap::iterator PathMapIter;
typedef PathMap::const_iterator PathMapConstIter;

// Custom message event used for system tray handling
#define PWS_MSG_ICON_NOTIFY             (WM_APP + 10)

// To catch post Header drag
#define PWS_MSG_HDR_DRAG_COMPLETE       (WM_APP + 20)
#define PWS_MSG_CCTOHDR_DD_COMPLETE     (WM_APP + 21)
#define PWS_MSG_HDRTOCC_DD_COMPLETE     (WM_APP + 22)

// Process Compare Result Dialog click/menu functions
#define PWS_MSG_COMPARE_RESULT_FUNCTION (WM_APP + 30)
#define PWS_MSG_COMPARE_RESULT_ALLFNCTN (WM_APP + 31)

// Equivalent one from Expired Password dialog
#define PWS_MSG_EXPIRED_PASSWORD_EDIT   (WM_APP + 32)

// Edit/Add extra context menu messages
#define PWS_MSG_CALL_EXTERNAL_EDITOR    (WM_APP + 40)
#define PWS_MSG_EXTERNAL_EDITOR_ENDED   (WM_APP + 41)
#define PWS_MSG_EDIT_WORDWRAP           (WM_APP + 42)
#define PWS_MSG_EDIT_SHOWNOTES          (WM_APP + 43)
#define PWS_MSG_EDIT_APPLY              (WM_APP + 44)
#define PWS_MSG_CALL_NOTESZOOMIN        (WM_APP + 45)
#define PWS_MSG_CALL_NOTESZOOMOUT       (WM_APP + 46)

// Simulate Ctrl+F from Find Toolbar "enter"
#define PWS_MSG_TOOLBAR_FIND            (WM_APP + 50)

// Perform Drag Autotype
#define PWS_MSG_DRAGAUTOTYPE            (WM_APP + 55)
#define PWS_MSG_EXPLORERAUTOTYPE        (WM_APP + 56)

// Update current filters whilst SetFilters dialog is open
#define PWS_MSG_EXECUTE_FILTERS         (WM_APP + 60)

// Wizard thread ended notification
#define PWS_MSG_WIZARD_EXECUTE_THREAD_ENDED (WM_APP + 65)

// Message to get Virtual Keyboard buffer.
#define PWS_MSG_INSERTBUFFER            (WM_APP + 70)

/*
  Timer related values (note - all documented her but some defined only where needed.
*/

// Timer event number used to by PupText.
#define TIMER_PUPTEXT             0x03
// Timer event number used to check if the workstation is locked
#define TIMER_LOCKONWTSLOCK       0x04
// Timer event number used to support lock on user-defined idle timeout
#define TIMER_LOCKDBONIDLETIMEOUT 0x05
// Timer event number used to support Find in PWListCtrl when icons visible
#define TIMER_FIND                0x06
// Timer event number used to support display of notes in LIST & TREE controls
#define TIMER_ND_HOVER            0x07
#define TIMER_ND_SHOWING          0x08
// Timer event number used to support DragBar
#define TIMER_DRAGBAR             0x09
// Timer event numbers used to by ControlExtns for ListBox tooltips.
#define TIMER_LB_HOVER            0x0A
#define TIMER_LB_SHOWING          0x0B 
// Timer event numbers used by StatusBar for tooltips.
#define TIMER_SB_HOVER            0x0C
#define TIMER_SB_SHOWING          0x0D 
// Timer event for daily expired entries check
#define TIMER_EXPENT              0x0E

// Definition of a minute in milliseconds
#define MINUTE 60000
// How ofter should idle timeout timer check:
#define IDLE_CHECK_RATE 2
#define IDLE_CHECK_INTERVAL (MINUTE/IDLE_CHECK_RATE)

// DragBar time interval 
#define TIMER_DRAGBAR_TIME 100

/*
HOVER_TIME_xx       The length of time the pointer must remain stationary
                    within a tool's bounding rectangle before the tool tip
                    window appears.
                    xx = ND - Notes displat
                    xx = LB - Tooltips in ComboBoxes and ListBoxes
                    xx = SB - Tooltips in StatusBar

TIMEINT_xx_SHOWING The length of time the tool tip window remains visible
                   if the pointer is stationary within a tool's bounding
                   rectangle.
*/
#define HOVER_TIME_ND      2000
#define TIMEINT_ND_SHOWING 5000

#define HOVER_TIME_LB      1000
#define TIMEINT_LB_SHOWING 5000

#define HOVER_TIME_SB      1000
#define TIMEINT_SB_SHOWING 5000

// Hotkey value ID
#define PWS_HOTKEY_ID      5767
