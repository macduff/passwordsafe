// PasswordSafe.h
// main header file for the PasswordSafe application
//-----------------------------------------------------------------------------


#if defined(WIN32)
#include "stdafx.h"
#endif

/*
  jpr debug stuff
*/
#include "jprdebug.h"


/*
  eventually, this breaks off into pws_mfc.h
*/

/*
  a globally available reference to the app object, which is a whole lot
  cleaner (in my mind) than constantly calling AfxGetApp() for the same
  thing... {jpr}
*/

class ThisMfcApp;
extern ThisMfcApp app;

#include "corelib/global.h"

/*
 * This is the string to be displayed instead of the actual password, unless
 * the user chooses to see the password:
 */

extern const TCHAR *HIDDEN_PASSWORD;

//-----------------------------------------------------------------------------
// Local variables:
// mode: c++
// End:
