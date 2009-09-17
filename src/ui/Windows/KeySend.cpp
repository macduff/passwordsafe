/*
* Copyright (c) 2003-2009 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#include "KeySend.h"

CKeySend::CKeySend(DWORD dwWindowsMajorVersion, DWORD dwWindowsMinorVersion)
 : m_delay(10)
{
  // We want to use keybd_event (OldSendChar) for Win2K & older,
  // SendInput (NewSendChar) for newer versions.
  m_isOldOS = ((dwWindowsMajorVersion <= 4) ||
               (dwWindowsMajorVersion == 5 && dwWindowsMinorVersion == 0));

  // get the locale of the current thread.
  // we are assuming that all window and threading in the 
  // current users desktop have the same locale.
  m_hlocale = GetKeyboardLayout(0);
}

CKeySend::~CKeySend(void)
{
}

void CKeySend::SendString(const StringX &data)
{
  for (StringX::const_iterator iter = data.begin();
       iter != data.end(); iter++)
    SendChar(*iter);
}

void CKeySend::SendChar(wchar_t c)
{
  if (m_isOldOS)
    OldSendChar(c);
  else
    NewSendChar(c);
}

void CKeySend::NewSendChar(wchar_t c)
{
  UINT status;
  INPUT input[2];
  input[0].ki.time = input[1].ki.time = 0; //probably needed
  input[0].ki.dwExtraInfo = input[1].ki.dwExtraInfo = 0; //probably not
  
  input[0].type = input[1].type = INPUT_KEYBOARD;
  switch (c) {
    case L'\t':
    case L'\r':
      input[0].ki.wVk = c == L'\t' ? VK_TAB : VK_RETURN;
      input[0].ki.wScan = 0;
      input[0].ki.dwFlags = 0;
      break;
    default:
      input[0].ki.wVk = 0;
      input[0].ki.wScan = c;
      input[0].ki.dwFlags = KEYEVENTF_UNICODE;
      break;
  }
  // add the key-up event
  input[1].ki = input[0].ki;
  input[1].ki.dwFlags |= KEYEVENTF_KEYUP;

  status = ::SendInput(2, input, sizeof(INPUT));
  if (status != 2)
    TRACE(L"CKeySend::SendChar: SendInput failed status=%d\n", status);
  ::Sleep(m_delay);
}

void CKeySend::OldSendChar(wchar_t c)
{
  BOOL shiftDown = false; //assume shift key is up at start.
  BOOL ctrlDown = false;
  BOOL altDown = false;
  SHORT keyScanCode = VkKeyScanEx(c, m_hlocale);
  // high order byte of keyscancode indicates if SHIFT, CTRL etc keys should be down 
  if (keyScanCode & 0x100) {
    shiftDown=true;      
    //send a shift down
    keybd_event(VK_SHIFT, (BYTE) MapVirtualKeyEx(VK_SHIFT, 0, m_hlocale), 0, 3); //Fixes bug #1208955
  } 

  if (keyScanCode & 0x200) {
    ctrlDown=true;       
    //send a ctrl down
    keybd_event(VK_CONTROL, (BYTE) MapVirtualKeyEx(VK_CONTROL, 0, m_hlocale), KEYEVENTF_EXTENDEDKEY, 0); 
  } 

  if (keyScanCode & 0x400) {
    altDown=true; 
    //send a alt down
    keybd_event(VK_MENU, (BYTE) MapVirtualKeyEx(VK_MENU, 0, m_hlocale), KEYEVENTF_EXTENDEDKEY, 0);    
  } 

  // the lower order byte has the key scan code we need.
  keyScanCode =(SHORT)(keyScanCode & 0xFF);

  keybd_event((BYTE)keyScanCode, (BYTE) MapVirtualKeyEx(keyScanCode, 0, m_hlocale), 0, 0);      
  keybd_event((BYTE)keyScanCode, (BYTE) MapVirtualKeyEx(keyScanCode, 0, m_hlocale), KEYEVENTF_KEYUP, 0);    

  if (shiftDown) {
    //send a shift up
    keybd_event(VK_SHIFT, (BYTE) MapVirtualKeyEx(VK_SHIFT, 0, m_hlocale), KEYEVENTF_KEYUP, 3); //Fixes bug #1208955
    shiftDown=false;
  }

  if (ctrlDown) {
    //send a ctrl up
    keybd_event(VK_CONTROL, (BYTE) MapVirtualKeyEx(VK_CONTROL, 0, m_hlocale),
                KEYEVENTF_KEYUP |KEYEVENTF_EXTENDEDKEY, 0); 
    ctrlDown=false;
  } 

  if (altDown) {
    //send a alt up
    keybd_event(VK_MENU, (BYTE) MapVirtualKeyEx(VK_MENU, 0, m_hlocale), KEYEVENTF_KEYUP |KEYEVENTF_EXTENDEDKEY, 0); 
    altDown=false;       
  } 
  ::Sleep(m_delay);
}

static void newSendVK(WORD vk)
{
  UINT status;
  INPUT input[2];
  input[0].ki.time = input[1].ki.time = 0; //probably needed
  input[0].ki.dwExtraInfo = input[1].ki.dwExtraInfo = 0; //probably not
  input[0].type = input[1].type = INPUT_KEYBOARD;
  input[0].ki.wVk = input[1].ki.wVk = vk;
  input[0].ki.dwFlags = 0;
  input[1].ki.dwFlags = KEYEVENTF_KEYUP;
  status = ::SendInput(2, input, sizeof(INPUT));
  if (status != 2)
    TRACE(L"newSendVK: SendInput failed status=%d\n", status);
}

void CKeySend::ResetKeyboardState()
{
  // We need to make sure that the Control Key is still not down. 
  // It will be down while the user presses ctrl-T the shortcut for autotype.

  BYTE keys[256];

  GetKeyboardState((LPBYTE)&keys);

  while((keys[VK_CONTROL] & 0x80) != 0) {
    // VK_CONTROL is down so send a key down and an key up...
    if (m_isOldOS) {
      keybd_event(VK_CONTROL, (BYTE)MapVirtualKeyEx(VK_CONTROL, 0, m_hlocale),
                  KEYEVENTF_EXTENDEDKEY, 0);
      keybd_event(VK_CONTROL, (BYTE) MapVirtualKeyEx(VK_CONTROL, 0, m_hlocale),
                  KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    } else {
      newSendVK(VK_CONTROL); // Send Ctrl keydown/keyup via SendInput
    }

    //now we let the messages be processed by the applications to set the keyboard state
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
      // so there is a message process it.
      if (!AfxGetThread()->PumpMessage())
        break;
    }

    ::Sleep(10);
    SecureZeroMemory(keys, sizeof(keys));
    GetKeyboardState((LPBYTE)&keys);
  } // while
}

// SetAndDelay allows users to put \d500\d10 in autotype and
// the it will cause a delay of half a second then subsequent
// key stokes will be delayed by 10 ms 
// thedavecollins 2004-08-05

void CKeySend::SetAndDelay(int d) {
  SetDelay(d);
  ::Sleep(m_delay);
}

void CKeySend::SetDelay(int d) {
  m_delay = d;
}

void CKeySend::SetCapsLock(const bool bState)
{
  BYTE keyState[256];

  GetKeyboardState((LPBYTE)&keyState);
  if ((bState && !(keyState[VK_CAPITAL] & 0x01)) ||
      (!bState && (keyState[VK_CAPITAL] & 0x01))) {
    if (m_isOldOS) {
      // Simulate a key press
      keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
      // Simulate a key release
      keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
    } else {
      newSendVK(VK_CAPITAL); // Send CapLock keydown/keyup via SendInput
    }
  }

  MSG msg;
  while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
    // so there is a message process it.
    if (!AfxGetThread()->PumpMessage())
      break;
  }
}
