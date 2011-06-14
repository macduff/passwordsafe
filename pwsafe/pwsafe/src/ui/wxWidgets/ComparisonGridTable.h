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

#define CurrentBackgroundColor    wxColour(244, 244, 244)
#define ComparisonBackgroundColor wxColour(225, 225, 225)


struct SelectionCriteria;
class PWScore;

class ComparisonGrid: public wxGrid
{
public:
  ComparisonGrid(wxWindow* parent, wxWindowID id);
  wxPen GetRowGridLinePen(int row);
};



class ComparisonGridTable: public wxGridTableBase
{
public:
  ComparisonGridTable(SelectionCriteria* criteria);
  ~ComparisonGridTable();

  //virtual overrides
  int GetNumberCols();
  void SetValue(int row, int col, const wxString& value);
  wxString GetColLabelValue(int col);

protected:
  SelectionCriteria*      m_criteria;
  typedef bool (CItemData::*AvailableFunction)() const;
  typedef struct {
    CItemData::FieldType ft;
    AvailableFunction available;
  } ColumnData;
  ColumnData*   m_colFields;
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

private:
  
  PWScore* GetRowCore(int row);

};

#endif
