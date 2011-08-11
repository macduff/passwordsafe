/*
 * Copyright (c) 2003-2011 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */
/** \file ComparisonGridTable.h
* 
*/

#ifndef _COMPARISONGRIDTABLE_H_
#define _COMPARISONGRIDTABLE_H_

#include <wx/grid.h>

#include "../../core/DBCompareData.h"
#include "../../core/UIinterface.h"

#define CurrentBackgroundColor    *wxWHITE
#define ComparisonBackgroundColor *wxWHITE


struct SelectionCriteria;
class PWScore;

class ComparisonGrid: public wxGrid
{
public:
  ComparisonGrid(wxWindow* parent, wxWindowID id);
  wxPen GetRowGridLinePen(int row);
};


class ComparisonGridTable: public wxGridTableBase, public UIInterFace
{
public:
  ComparisonGridTable(SelectionCriteria* criteria);
  ~ComparisonGridTable();

  //virtual overrides
  int GetNumberCols();
  void SetValue(int row, int col, const wxString& value);
  wxString GetColLabelValue(int col);

  //common to all derived classes
  void AutoSizeField(CItemData::FieldType ft);
  int FieldToColumn(CItemData::FieldType ft);
  CItemData::FieldType ColumnToField(int col);

  //derived classes must override these

  //should return wxNOT_FOUND on error
  virtual int GetItemRow(const pws_os::CUUID& uuid) = 0;
  virtual pws_os::CUUID GetSelectedItemId(bool readOnly) = 0;

protected:
  SelectionCriteria*      m_criteria;
  typedef bool (CItemData::*AvailableFunction)() const;
  typedef struct {
    CItemData::FieldType ft;
    AvailableFunction available;
  } ColumnData;
  ColumnData*   m_colFields;

  //UIinterface overrides
public:
  virtual void DatabaseModified(bool bChanged);
  virtual void UpdateGUI(UpdateGUICommand::GUI_Action ga,
                         const pws_os::CUUID &entry_uuid,
                         CItemData::FieldType ft = CItemData::START,
                         bool bUpdateGUI = true);
  virtual void GUISetupDisplayInfo(CItemData &ci);
  virtual void GUIRefreshEntry(const CItemData &ci);
  virtual void UpdateWizard(const stringT &s);
};

///////////////////////////////////////////////////////////////
//UniSafeCompareGridTable
//
//Class to handle display of comparison results which only invole a single 
//safe (uses only a single core)
class UniSafeCompareGridTable: public ComparisonGridTable
{
  typedef pws_os::CUUID st_CompareData::*uuid_ptr;
  
  CompareData*            m_compData;
  PWScore*                m_core;
  wxGridCellAttr          *m_gridAttr;
  uuid_ptr                m_uuidptr;
public:
  
  UniSafeCompareGridTable(SelectionCriteria* criteria, 
                          CompareData* data,
                          PWScore* core,
                          uuid_ptr pu,
                          const wxColour& backgroundColour);
  
  //virtual overrides
  int GetNumberRows();
  bool IsEmptyCell(int row, int col);
  wxString GetValue(int row, int col);
  wxGridCellAttr* GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind);
  
  //virtual override from ComparisonGridTable
  int GetItemRow(const pws_os::CUUID& uuid);
  virtual pws_os::CUUID GetSelectedItemId(bool readOnly);
};

///////////////////////////////////////////////////////////////////
//MultiSafeCompareGridTable
//
//Class to handle display of comparison results which invole two
//safes (uses both the cores in comparison result)
class MultiSafeCompareGridTable: public ComparisonGridTable
{
  
  CompareData*            m_compData;
  PWScore*                m_currentCore;
  PWScore*                m_otherCore;
  wxGridCellAttr          *m_currentAttr, *m_comparisonAttr;
  
public:
  
  MultiSafeCompareGridTable(SelectionCriteria* criteria, 
                               CompareData* data,
                               PWScore* current,
                               PWScore* other);
  
  //virtual overrides
  int GetNumberRows();
  bool IsEmptyCell(int row, int col);
  wxString GetValue(int row, int col);
  wxString GetRowLabelValue(int row);
  wxGridCellAttr* GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind);

  //virtual override from ComparisonGridTable
  int GetItemRow(const pws_os::CUUID& uuid);
  virtual pws_os::CUUID GetSelectedItemId(bool readOnly);
private:
  
  PWScore* GetRowCore(int row);

};

#endif
