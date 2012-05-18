/*
* Copyright (c) 2003-2012 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#pragma once

// CPWToolBar

#include <map>

typedef std::map<UINT, UINT> ID2ImageMap;
typedef ID2ImageMap::iterator ID2ImageMapIter;

class CPWToolBar : public CToolBar
{
  DECLARE_DYNAMIC(CPWToolBar)

public:
  CPWToolBar();
  virtual ~CPWToolBar();

  void Init(const int NumBits, const bool bRefresh = false);
  void LoadDefaultToolBar(const int toolbarMode);
  void CustomizeButtons(CString csButtonNames);
  void ChangeImages(const int toolbarMode);
  void Reset();

  CString GetButtonString();
  int GetBrowseURLImageIndex() {return m_iBrowseURL_BM_offset;}
  int GetSendEmailImageIndex() {return m_iSendEmail_BM_offset;}
  void MapControlIDtoImage(ID2ImageMap &IDtoImages);
  void SetBitmapBackground(CBitmap &bm, const COLORREF newbkgrndColour);
  void RefreshImages();

protected:
  //{{AFX_MSG(CPWToolBar)
  afx_msg void OnToolBarGetButtonInfo(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnToolBarQueryInsert(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnToolBarQueryDelete(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnToolBarQueryInfo(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnToolBarReset(NMHDR *pNotifyStruct, LRESULT *pLResult);
  afx_msg void OnDestroy();
  //}}AFX_MSG

  DECLARE_MESSAGE_MAP()

private:
  enum MainToolBar {
    // Default Main Toobar
    MAIN_TB_NEW, MAIN_TB_OPEN, MAIN_TB_CLOSE, MAIN_TB_SAVE,
    MAIN_TB_SEPARATOR0, 
    MAIN_TB_COPYPASSWORD, MAIN_TB_COPYUSERNAME, MAIN_TB_COPYNOTESFLD, MAIN_TB_CLEARCLIPBOARD,
    MAIN_TB_SEPARATOR1,
    MAIN_TB_AUTOTYPE, MAIN_TB_BROWSEURL,
    MAIN_TB_SEPARATOR2,
    MAIN_TB_ADD, MAIN_TB_EDIT,
    MAIN_TB_SEPARATOR4,
    MAIN_TB_DELETEENTRY, MAIN_TB_SEPARATOR5, MAIN_TB_EXPANDALL, MAIN_TB_COLLAPSEALL,
    MAIN_TB_SEPARATOR6,
    MAIN_TB_OPTIONS,
    MAIN_TB_SEPARATOR8,
    MAIN_TB_HELP,
    // Allowed additional buttons on Main Toolbar
    MAIN_TB_EXPORT2PLAINTEXT, MAIN_TB_EXPORT2XML, MAIN_TB_IMPORT_PLAINTEXT, MAIN_TB_IMPORT_XML,
    MAIN_TB_SAVEAS, MAIN_TB_COMPARE, MAIN_TB_MERGE, MAIN_TB_SYNCHRONIZE,
    MAIN_TB_UNDO, MAIN_TB_REDO, MAIN_TB_PASSWORDSUBSET, MAIN_TB_BROWSEURLPLUS,
    MAIN_TB_RUNCOMMAND, MAIN_TB_SENDEMAIL, MAIN_TB_LISTTREE, MAIN_TB_SHOWFINDTOOLBAR,
    MAIN_TB_TOOLBUTTON_VIEWREPORTS, MAIN_TB_APPLYFILTER, MAIN_TB_CLEARFILTER, MAIN_TB_EDITFILTER,
    MAIN_TB_MANAGEFILTERS, MAIN_TB_ADDGROUP, MAIN_TB_PSWD_POLICIES,
    MAIN_TB_LAST};

  enum OtherToolBar {
    OTHER_TB_PROPERTIES, OTHER_TB_GROUPENTER, OTHER_TB_DUPLICATEENTRY, OTHER_TB_CHANGEFONTMENU,
    OTHER_TB_CHANGETREEFONT, OTHER_TB_CHANGEPSWDFONT, OTHER_TB_REPORT_COMPARE, OTHER_TB_REPORT_FIND,
    OTHER_TB_REPORT_IMPORTTEXT, OTHER_TB_REPORT_IMPORTXML, OTHER_TB_REPORT_MERGE, OTHER_TB_REPORT_SYNCHRONIZE,
    OTHER_TB_REPORT_VALIDATE, OTHER_TB_CHANGECOMBO, OTHER_TB_BACKUPSAFE, OTHER_TB_RESTORESAFE, OTHER_TB_EXIT,
    OTHER_TB_ABOUT, OTHER_TB_TRAYUNLOCK, OTHER_TB_TRAYLOCK, OTHER_TB_REPORTSMENU, OTHER_TB_MRUENTRY,
    OTHER_TB_EXPORTMENU, OTHER_TB_IMPORTMENU, OTHER_TB_CREATESHORTCUT, OTHER_TB_CUSTOMIZETOOLBAR,
    OTHER_TB_VKEYBOARDFONT, OTHER_TB_COMPVIEWEDIT, OTHER_TB_COPY_TO_ORIGINAL, OTHER_TB_EXPORTENT2PLAINTEXT,
    OTHER_TB_EXPORTENT2XML, OTHER_TB_DUPLICATEGROUP, OTHER_TB_REPORT_EXPORTTEXT, OTHER_TB_REPORT_EXPORTXML,
    OTHER_TB_COPYALL_TO_ORIGINAL, OTHER_TB_SYNCHRONIZEALL,
    OTHER_TB_LAST};

 typedef struct ToolbarDefinitions {
      UINT ID;
      UINT CLASSIC_BMP;
      UINT NEW_BMP;
      UINT NEW_DISABLED_BMP;
  };
  
  static const ToolbarDefinitions m_MainToolBarDefinitions[MAIN_TB_LAST];
  static const ToolbarDefinitions m_OtherToolbarDefinitions[OTHER_TB_LAST];

  enum ImageType {CLASSIC_TYPE = 1, NEW_TYPE = 2};
  void SetupImageList(const ToolbarDefinitions *pTBDefs, const ImageType iType, 
                      const int num_entries, const int nImageList);

  static const CString m_csMainButtons[];

  // 1st = Classic; 2nd = New 8; 3rd = New 32;
  CImageList m_ImageLists[3];
  // 1st = New 8; 2nd = New 32;
  CImageList m_DisabledImageLists[2];

  CString m_csDefaultButtonString;
  TBBUTTON *m_pOriginalTBinfo;

  int m_iMaxNumButtons, m_iNum_Bitmaps, m_iNumDefaultButtons, m_NumBits;
  int m_toolbarMode, m_bitmode;
  bool m_bIsDefault;
  int m_iBrowseURL_BM_offset, m_iSendEmail_BM_offset;
};
