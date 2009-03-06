// pws_at.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

#include "pws_at.h"
#include <stdio.h>

/*
 In PWS:

 #include "pws_autotype/pws_at.h"

 ON_REGISTERED_MESSAGE(app.m_uiWH_SHELL, On...)

 LRESULT DboxMain::On...(WPARAM wParam, LPARAM lParam);

*/

#ifdef _DEBUG
#pragma data_seg(".pws_atD")
static HWND hWndServer = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.pws_atD,rws")
#else
#pragma data_seg(".pws_at")
static HWND hWndServer = NULL;
#pragma data_seg()
#pragma comment(linker, "/section:.pws_at,rws")
#endif

HMODULE m_hInstance(NULL);
HHOOK   m_shl_hook(NULL);
UINT    m_uiWH_SHELL(0);

static LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam);

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD   ul_reason_for_call,
                      LPVOID  lpReserved)
{
  switch (ul_reason_for_call)	{
    case DLL_PROCESS_ATTACH:
      // A process is loading the DLL.
      m_hInstance = hModule;
      m_uiWH_SHELL = RegisterWindowMessage(UNIQUE_PWS_SHELL);
      break;
    case DLL_THREAD_ATTACH:
      // A process is creating a new thread.
      break;
    case DLL_THREAD_DETACH:
      // A thread exits normally.
      break;
    case DLL_PROCESS_DETACH:
      // A process unloads the DLL.
      if (hWndServer != NULL)
        AT_HK_UnInitialise(hWndServer);
      break;
  }

  return TRUE;
}

__declspec(dllexport) BOOL AT_HK_Initialise(HWND hWnd)
{
  // Calling process: Initialise(m_hWnd)
  if (hWndServer != NULL)
    return FALSE;

  m_shl_hook = SetWindowsHookEx(WH_SHELL,
                            (HOOKPROC)ShellProc,
                            m_hInstance,
                            0);

  if (m_shl_hook != NULL) {
    hWndServer = hWnd;
    return TRUE;
  }

  return FALSE;
}

__declspec(dllexport) BOOL AT_HK_UnInitialise(HWND hWnd)
{
  // UnInitialise(m_hWnd)
  if (hWndServer != hWnd || hWnd == NULL)
    return FALSE;

  BOOL unhooked = UnhookWindowsHookEx(m_shl_hook);

  if (unhooked) {
    m_shl_hook = NULL;
    hWndServer = NULL;
    return TRUE;
  }

  return FALSE;
}

static LRESULT CALLBACK ShellProc(int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode == HSHELL_WINDOWACTIVATED) {
    DWORD dwProcessId(0);
    // wParam == handle to the activated window
    GetWindowThreadProcessId((HWND)wParam, &dwProcessId);
    PostMessage(hWndServer, m_uiWH_SHELL, (WPARAM)dwProcessId, (LPARAM)wParam);
  }

  return CallNextHookEx(m_shl_hook, nCode, wParam, lParam);
}
