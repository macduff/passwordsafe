/////////////////////////////////////////////////////////////////////////////
// Name:        pwsafeapp.h
// Purpose:     
// Author:      Rony Shapiro
// Modified by: 
// Created:     Wed 14 Jan 2009 10:11:39 PM IST
// RCS-ID:      
// Copyright:   Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PWSAFEAPP_H_
#define _PWSAFEAPP_H_


/*!
 * Includes
 */

////@begin includes
#include "wx/image.h"
#include "safecombinationentry.h"
////@end includes
#include "corelib/PWScore.h"
#include "./RecentDBList.h"

/*!
 * Forward declarations
 */

////@begin forward declarations
////@end forward declarations
class wxTimer;
class PasswordSafeFrame;

/*!
 * Control identifiers
 */

////@begin control identifiers
////@end control identifiers

/*!
 * PwsafeApp class declaration
 */

class PwsafeApp: public wxApp
{    
    DECLARE_CLASS( PwsafeApp )
    DECLARE_EVENT_TABLE()

public:
    /// Constructor
    PwsafeApp();

    /// Destructor
    ~PwsafeApp();

    void Init();

    /// Initialises the application
    virtual bool OnInit();

    /// Called on exit
    virtual int OnExit();

////@begin PwsafeApp event handler declarations

////@end PwsafeApp event handler declarations

////@begin PwsafeApp member function declarations

////@end PwsafeApp member function declarations

////@begin PwsafeApp member variables
////@end PwsafeApp member variables

	void OnActivate(wxActivateEvent& actEvent);
	void OnActivityTimer(wxTimerEvent& timerEvent);

 private:
    PWScore m_core;
    wxTimer* m_activityTimer;
    PasswordSafeFrame* m_frame;
    enum { ACTIVITY_TIMER_ID = 33 } ; 
    
 public:
    CRecentDBList m_recentDatabases;
};

/*!
 * Application instance declaration 
 */

////@begin declare app
DECLARE_APP(PwsafeApp)
////@end declare app

#endif
    // _PWSAFEAPP_H_
