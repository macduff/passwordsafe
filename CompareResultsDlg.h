/*
 * Copyright (c) 2003-2007 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */
#pragma once

/// CompareResultsDlg.h
//-----------------------------------------------------------------------------

#include "PWDialog.h"
#include "corelib/ItemData.h"
#include "corelib/MyString.h"
#include "corelib/PWScore.h"
#include "corelib/Report.h"
#include "corelib/uuidgen.h"

#ifdef _DEBUG
#include <bitset>
#include <string>
#endif

// The following structure is needed for compare when record is in
// both databases (indatabase = -1) but there are differences
// Subset used when record is in only one (indatabase = 0 or 1)
// If entries made equal by copying, indatabase set to -1.
struct st_CompareData {
  uuid_array_t uuid0;  // original DB
  uuid_array_t uuid1;  // comparison DB
  CItemData::FieldBits bsDiffs;  // list of items compared
  CMyString group;
  CMyString title;
  CMyString user;
  int id;  // # in the appropriate list: "Only in Original", "Only in Comparison" or in "Both with Differences"
  int indatabase;  // see enum below
  int listindex;  // list index in CompareResultsDlg list control
  bool unknflds0;  // original DB
  bool unknflds1;  // comparison DB

  st_CompareData()
    : bsDiffs(0), group(_T("")), title(_T("")), user(_T("")),
      id(0), indatabase(0), listindex(0),
      unknflds0(false), unknflds1(false)
  {
    memset(uuid0, 0x00, sizeof(uuid_array_t));
    memset(uuid1, 0x00, sizeof(uuid_array_t));
  }

  st_CompareData(const st_CompareData &that)
    : bsDiffs(that.bsDiffs), group(that.group), title(that.title), user(that.user),
      id(that.id), indatabase(that.indatabase), listindex(that.listindex),
      unknflds0(that.unknflds0), unknflds1(that.unknflds1)
  {
    memcpy(uuid0, that.uuid0, sizeof(uuid_array_t));
    memcpy(uuid1, that.uuid1, sizeof(uuid_array_t));
  }

  st_CompareData &operator=(const st_CompareData &that)
  {
    if (this != &that) {
      memcpy(uuid0, that.uuid0, sizeof(uuid_array_t));
      memcpy(uuid1, that.uuid1, sizeof(uuid_array_t));
      bsDiffs = that.bsDiffs;
      group = that.group;
      title = that.title;
      user = that.user;
      id = that.id;
      indatabase = that.indatabase;
      listindex = that.listindex;
      unknflds0 = that.unknflds0;
      unknflds1 = that.unknflds1;
    }
    return *this;
  }

#ifdef _DEBUG
  void Dump()
  {
    char uuid0_buffer[33], uuid1_buffer[33];
#if _MSC_VER >= 1400
    sprintf_s(uuid0_buffer, 33,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
            uuid0[0],  uuid0[1],  uuid0[2],  uuid0[3],
            uuid0[4],  uuid0[5],  uuid0[6],  uuid0[7],
            uuid0[8],  uuid0[9],  uuid0[10], uuid0[11],
            uuid0[12], uuid0[13], uuid0[14], uuid0[15]);
    sprintf_s(uuid1_buffer, 33,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
            uuid1[0],  uuid1[1],  uuid1[2],  uuid1[3],
            uuid1[4],  uuid1[5],  uuid1[6],  uuid1[7],
            uuid1[8],  uuid1[9],  uuid1[10], uuid1[11],
            uuid1[12], uuid1[13], uuid1[14], uuid1[15]);
#else
    sprintf(uuid0_buffer,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
          uuid0[0],  uuid0[1],  uuid0[2],  uuid0[3],
          uuid0[4],  uuid0[5],  uuid0[6],  uuid0[7],
          uuid0[8],  uuid0[9],  uuid0[10], uuid0[11],
          uuid0[12], uuid0[13], uuid0[14], uuid0[15]);
    sprintf(uuid1_buffer,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
          uuid1[0],  uuid1[1],  uuid1[2],  uuid1[3],
          uuid1[4],  uuid1[5],  uuid1[6],  uuid1[7],
          uuid1[8],  uuid1[9],  uuid1[10], uuid1[11],
          uuid1[12], uuid1[13], uuid1[14], uuid1[15]);
#endif
    uuid0_buffer[32] = '\0';
    uuid1_buffer[32] = '\0';
    std::string sDiffs;
    sDiffs =  bsDiffs.to_string();
    CString csDiffs(sDiffs.c_str());
    CString cs_InDatabase;
    if (indatabase == -1)
      cs_InDatabase = _T("BOTH");
    else if (indatabase == 0)
      cs_InDatabase = _T("CURRENT");
    else if (indatabase == 1)
      cs_InDatabase = _T("COMPARE");
    else
      cs_InDatabase = _T("ERROR");

    TRACE(_T("\nst_CompareData:\n"));
    TRACE("uuid0: %s\n", uuid0_buffer);
    TRACE("uuid1: %s\n", uuid1_buffer);
    TRACE(_T("bsDiffs: %s\n"), csDiffs);
    TRACE(_T("Group: %s; Title: %s; User: %s\n"), group, title, user);
    TRACE(_T("id: %d; indatabase: %s; listindex: %d; unknflds0: %c; unknflds1: %c\n"),
             id, cs_InDatabase, listindex, unknflds0 ? _T('T') : _T('F'), unknflds1 ? _T('T') : _T('F'));
  }
#else
  void Dump() {}
#endif
};

struct equal_id
{
   equal_id(int const& id) : m_id(id) {}
   bool operator()(st_CompareData const& rdata) const
   {
     return (rdata.id == m_id);
   }

   int m_id;
};

// Vector of entries passed from DboxMain::Compare to CompareResultsDlg
// Used for "Only in Original DB", "Only in Comparison DB" and
// in "Both with Differences"
typedef std::vector<st_CompareData> CompareData;

// The following structure is needed for compare to send back data
// to allow copying, viewing and editing of entries
struct st_CompareInfo {
  PWScore *pcore0;  // original DB
  PWScore *pcore1;  // comparison DB
  uuid_array_t uuid0;  // original DB
  uuid_array_t uuid1;  // comparison DB
  int  clicked_column;

#ifdef _DEBUG
  void Dump()
  {
    char uuid0_buffer[33], uuid1_buffer[33];
#if _MSC_VER >= 1400
    sprintf_s(uuid0_buffer, 33,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
            uuid0[0],  uuid0[1],  uuid0[2],  uuid0[3],
            uuid0[4],  uuid0[5],  uuid0[6],  uuid0[7],
            uuid0[8],  uuid0[9],  uuid0[10], uuid0[11],
            uuid0[12], uuid0[13], uuid0[14], uuid0[15]);
    sprintf_s(uuid1_buffer, 33,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
            uuid1[0],  uuid1[1],  uuid1[2],  uuid1[3],
            uuid1[4],  uuid1[5],  uuid1[6],  uuid1[7],
            uuid1[8],  uuid1[9],  uuid1[10], uuid1[11],
            uuid1[12], uuid1[13], uuid1[14], uuid1[15]);
#else
    sprintf(uuid0_buffer,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
          uuid0[0],  uuid0[1],  uuid0[2],  uuid0[3],
          uuid0[4],  uuid0[5],  uuid0[6],  uuid0[7],
          uuid0[8],  uuid0[9],  uuid0[10], uuid0[11],
          uuid0[12], uuid0[13], uuid0[14], uuid0[15]);
    sprintf(uuid1_buffer,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", 
          uuid1[0],  uuid1[1],  uuid1[2],  uuid1[3],
          uuid1[4],  uuid1[5],  uuid1[6],  uuid1[7],
          uuid1[8],  uuid1[9],  uuid1[10], uuid1[11],
          uuid1[12], uuid1[13], uuid1[14], uuid1[15]);
#endif
    uuid0_buffer[32] = '\0';
    uuid1_buffer[32] = '\0';
    TRACE(_T("\nst_CompareInfo:\n"));
    TRACE("uuid0: %s\n", uuid0_buffer);
    TRACE("uuid1: %s\n", uuid1_buffer);
    TRACE(_T("pcore0: %p; pcore1: %p; clicked column: %d\n"), pcore0, pcore1, clicked_column);
  }
#else
  void Dump() {}
#endif
};

class CCompareResultsDlg : public CPWDialog
{
  DECLARE_DYNAMIC(CCompareResultsDlg)

  // Construction
public:
  CCompareResultsDlg(CWnd* pParent,
                     CompareData &OnlyInCurrent,
                     CompareData &OnlyInComp,
                     CompareData &Conflicts,
                     CompareData &Identical,
                     CItemData::FieldBits &bsFields,
                     PWScore *pcore0, PWScore *pcore1,
                     CReport *rpt);

  // st_CompareInfo Functions
  enum {EDIT = 0, VIEW, COPY_TO_ORIGINALDB, COPY_TO_COMPARISONDB};

  // Column indices
  // IDENTICAL means CURRENT + COMPARE but identical
  // BOTH means CURRENT + COMPARE but with differences
  enum {IDENTICAL = -2, BOTH = -1 , CURRENT = 0, COMPARE, 
        GROUP, TITLE, USER, PASSWORD, NOTES, URL, AUTOTYPE, PWHIST, 
        CTIME, ATIME, LTIME, PMTIME, RMTIME,
        LAST};

  // Dialog Data
  //{{AFX_DATA(CCompareResultsDlg)
  enum { IDD = IDD_COMPARE_RESULTS };
  CListCtrl m_LCResults;
  int m_iSortedColumn;
  bool m_bSortAscending;
  CMyString m_cs_Filename1, m_cs_Filename2;
  int m_ShowIdenticalEntries;
  //}}AFX_DATA

  bool m_bOriginalDBReadOnly, m_bComparisonDBReadOnly;
  bool m_OriginalDBChanged, m_ComparisonDBChanged;
  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CCompareResultsDlg)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

private:
  static int CALLBACK CRCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

  // Implementation
protected:

  UINT statustext[1];
  CStatusBar m_statusBar;
  bool CopyLeftOrRight(const bool bCopyLeft);
  void UpdateStatusBar();
  bool ProcessFunction(const int ifunction, st_CompareData *st_data);
  void GetReportData(CString &data);
  st_CompareData * GetCompareData(const DWORD dwItemData);

  virtual BOOL OnInitDialog();
  // Generated message map functions
  //{{AFX_MSG(CCompareResultsDlg)
  virtual void OnCancel();
  virtual void OnOK();
  afx_msg void OnHelp();
  afx_msg void OnShowIdenticalEntries();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
  afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnItemDoubleClick(NMHDR* pNotifyStruct, LRESULT* result);
  afx_msg void OnItemRightClick(NMHDR* pNotifyStruct, LRESULT* result);
  afx_msg void OnCompareViewEdit();
  afx_msg void OnCompareCopyToOriginalDB();
  afx_msg void OnCompareCopyToComparisonDB();
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
	CompareData m_OnlyInCurrent;
	CompareData m_OnlyInComp;
	CompareData m_Conflicts;
  CompareData m_Identical;
  CItemData::FieldBits m_bsFields;

	PWScore *m_pcore0, *m_pcore1;
  CReport *m_prpt;

  size_t m_numOnlyInCurrent, m_numOnlyInComp, m_numConflicts, m_numIdentical;
  int m_cxBSpace, m_cyBSpace, m_cySBar;
  int m_DialogMinWidth, m_DialogMinHeight;
  int m_DialogMaxWidth, m_DialogMaxHeight;
  int m_row, m_column;
  int m_nCols;
};
