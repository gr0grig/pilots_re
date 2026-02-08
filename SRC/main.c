#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- main module ---- */

int __stdcall ShowMessageBox(LPCSTR lpText, LPCSTR lpCaption)
{
  MessageBeep(0x10u);
  return MessageBoxA(g_MainWindow, lpText, lpCaption, 0x10u);
}

int __stdcall ShowErrorDialog(LPCSTR lpText)
{
  DBG_ENTER("ShowErrorDialog");
  return ShowMessageBox(lpText, "Game Maker Player");
}

int DisplayErrorMessage()
{
  DBG_ENTER("DisplayErrorMessage");
  int result; // eax

  if ( g_VideoConfig.pErrorMessage )
  {
    result = ShowErrorDialog(g_VideoConfig.pErrorMessage);
    g_VideoConfig.pErrorMessage = 0;
  }
  return result;
}

char *__stdcall CreateHelpFilePath(char *Source)
{
  char *v1; // eax

  v1 = strcpy(g_VideoConfig.helpFilePath, Source);
  *strrchr(v1, 46) = 0;
  return strcat(g_VideoConfig.helpFilePath, ".hlp");
}

BOOL CleanupHelpAndWindow()
{
  BOOL result; // eax

  if ( g_MainWindow )
  {
    WinHelpA(g_MainWindow, g_VideoConfig.helpFilePath, 2u, 0);
    return DestroyWindow(g_MainWindow);
  }
  return result;
}

int CleanupGameResources()
{
  DBG_ENTER("CleanupGameResources");
  int result; // eax

  g_MainWindow = 0;
  CleanupGameState();
  result = CloseFile();
  if ( g_VideoConfig.hPalette )
  {
    result = DeleteObject((HGDIOBJ)g_VideoConfig.hPalette);
    g_VideoConfig.hPalette = 0;
  }
  if ( g_MemoryDeviceContext )
    result = DeleteDC(g_MemoryDeviceContext);
  if ( g_BitmapHandle )
    result = DeleteObject(g_BitmapHandle);
  if ( g_VideoConfig.hGlobalMem )
  {
    GlobalFree(g_VideoConfig.hGlobalMem);
    g_VideoConfig.hGlobalMem = 0;
    g_VideoConfig.pLockedMem = 0;
    return 0;
  }
  return result;
}

LRESULT __stdcall MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{                                               // If video playback active, delegate to VideoWndProc
HCURSOR cursor;
// eax
int minHeight;
// eax
int minWidth;
// eax
struct tagRECT updateRect;
// [esp+4h] [ebp-10h] BYREF

  if ( g_VideoFlags[1] )
    return VideoWndProc(hWnd, uMsg, (HWND)wParam, (_DWORD *)lParam);
  switch ( uMsg )
  {
    case 0xFu:
      if ( GetUpdateRect(hWnd, &updateRect, 0) )// WM_PAINT: Get dirty rect, add to dirty region list, pass to DefWindowProc
      {
        AddDirtyRegion(updateRect.bottom, updateRect.right, updateRect.top, updateRect.left);
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
      }
      return 0;
    case 0x311u:
      if ( (HWND)wParam == hWnd )
        return 0;                               // WM_PALETTECHANGED: Ignore if we caused it (wParam==hWnd)
LABEL_24:
      if ( g_VideoConfig.hPalette )             // WM_QUERYNEWPALETTE / WM_PALETTECHANGED: Realize palette and invalidate if colors changed
      {
        SelectPalette(g_VideoConfig.hDC, (HPALETTE)g_VideoConfig.hPalette, 0);
        if ( RealizePalette(g_VideoConfig.hDC) )
        {
          AddDirtyRegion(g_VideoConfig.clientHeight, g_VideoConfig.clientWidth, 0, 0);
          return 1;
        }
      }
      return 0;
    case 0x30Fu:
      goto LABEL_24;
    case 0x102u:
      if ( (g_VideoConfig.pInputConfig->flags & 1) != 0 )// WM_CHAR: Store typed character if keyboard input enabled (flags bit 0)
        g_VideoConfig.charCode = wParam;
      return 0;
    case 0x100u:
      if ( (g_VideoConfig.pInputConfig->flags & 1) != 0 )// WM_KEYDOWN: Match virtual key against 8 configurable action keys + 4 special keys
      {
        if ( g_VideoConfig.pInputConfig->key0 == wParam )
        {
          g_VideoConfig.keyIndex = 0;
          goto LABEL_32;
        }
        if ( g_VideoConfig.pInputConfig->key1 == wParam )
        {
          g_VideoConfig.keyIndex = 1;
          goto LABEL_32;
        }
        if ( g_VideoConfig.pInputConfig->key2 == wParam )
        {
          g_VideoConfig.keyIndex = 2;
          goto LABEL_32;
        }
        if ( g_VideoConfig.pInputConfig->key3 == wParam )
        {
          g_VideoConfig.keyIndex = 3;
          goto LABEL_32;
        }
        if ( g_VideoConfig.pInputConfig->key4 == wParam )
        {
          g_VideoConfig.keyIndex = 4;
          goto LABEL_32;
        }
        if ( g_VideoConfig.pInputConfig->key5 == wParam )
        {
          g_VideoConfig.keyIndex = 5;
          goto LABEL_32;
        }
        if ( g_VideoConfig.pInputConfig->key6 == wParam )
        {
          g_VideoConfig.keyIndex = 6;
          goto LABEL_32;
        }
        if ( g_VideoConfig.pInputConfig->key7 == wParam )
        {
          g_VideoConfig.keyIndex = 7;
LABEL_32:
          if ( g_VideoConfig.keyIndex <= 7u && GetAsyncKeyState(16) )// If valid key (0-7) and Shift held, set bit 3 (shift modifier)
            g_VideoConfig.keyIndex |= 8u;
          g_VideoConfig.clickX = g_VideoConfig.cursorX;// Record click position as current mouse position at time of keypress
          g_VideoConfig.clickY = g_VideoConfig.cursorY;
          return 0;
        }
        if ( g_VideoConfig.pInputConfig->specialKey0 == wParam )// Special keys: map to event codes -126, -125, -113, -124 (0x82, 0x83, 0x8F, 0x84)
        {
          g_VideoConfig.eventCode = -126;
LABEL_74:
          g_VideoConfig.charCode = wParam;
          return 0;
        }
        if ( g_VideoConfig.pInputConfig->specialKey1 == wParam )
        {
          g_VideoConfig.eventCode = -125;
          goto LABEL_74;
        }
        if ( g_VideoConfig.pInputConfig->specialKey2 == wParam )
        {
          g_VideoConfig.eventCode = -113;
          goto LABEL_74;
        }
        if ( g_VideoConfig.pInputConfig->specialKey3 == wParam )
        {
          g_VideoConfig.eventCode = -124;
          goto LABEL_74;
        }
      }
      return 0;
    case 0x201u:
    case 0x204u:
      if ( (g_VideoConfig.pInputConfig->flags & 2) != 0 )// WM_LBUTTONDOWN / WM_RBUTTONDOWN: Record mouse click event (-127/0x81) with click coordinates
      {
        g_VideoConfig.eventCode = -127;
        g_VideoConfig.clickX = (unsigned __int16)lParam;
        g_VideoConfig.clickY = HIWORD(lParam);
      }
      return 0;
    case 0x202u:
      if ( (g_VideoConfig.pInputConfig->flags & 2) != 0 )// WM_LBUTTONUP: Generate event code -126 (left release), store specialKey0 as charCode
      {
        g_VideoConfig.eventCode = -126;
        g_VideoConfig.charCode = g_VideoConfig.pInputConfig->specialKey0;
        g_VideoConfig.clickX = (unsigned __int16)lParam;
        g_VideoConfig.clickY = HIWORD(lParam);
      }
      return 0;
    case 0x205u:
      if ( (g_VideoConfig.pInputConfig->flags & 2) != 0 )// WM_RBUTTONUP: Generate event code -125 (right release), store specialKey1 as charCode
      {
        g_VideoConfig.eventCode = -125;
        g_VideoConfig.charCode = g_VideoConfig.pInputConfig->specialKey1;
        g_VideoConfig.clickX = (unsigned __int16)lParam;
        g_VideoConfig.clickY = HIWORD(lParam);
      }
      return 0;
    case 0x20u:
      if ( (_WORD)lParam == 1 )                 // WM_SETCURSOR: If in client area (HTCLIENT), set game cursor; otherwise DefWindowProc
      {
        cursor = GetGameCursor();
        SetCursor(cursor);
        return 0;
      }
      return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    case 0x200u:
      if ( (g_VideoConfig.pInputConfig->flags & 2) != 0 )// WM_MOUSEMOVE: Update current mouse position and refresh cursor render object
      {
        g_VideoConfig.cursorX = (unsigned __int16)lParam;
        g_VideoConfig.cursorY = HIWORD(lParam);
        UpdateCursorPosition(HIWORD(lParam), lParam);
      }
      return 0;
    case 0x24u:
      minHeight = g_VideoConfig.borderWidth + g_VideoConfig.initialClientWidth;// WM_GETMINMAXINFO: Set min/max tracking size to window border + video dimensions
      *(_DWORD *)(lParam + 32) = g_VideoConfig.borderWidth + g_VideoConfig.initialClientWidth;
      *(_DWORD *)(lParam + 8) = minHeight;
      minWidth = g_VideoConfig.titleBarHeight + g_VideoConfig.initialClientHeight;
      *(_DWORD *)(lParam + 36) = g_VideoConfig.titleBarHeight + g_VideoConfig.initialClientHeight;
      *(_DWORD *)(lParam + 12) = minWidth;
      return 0;
    case 5u:
      g_VideoConfig.clientWidth = (unsigned __int16)lParam;// WM_SIZE: Update stored video dimensions from lParam
      g_VideoConfig.clientHeight = HIWORD(lParam);
      return 0;
    case 0x3BDu:
      MixAudioBuffers();                        // MM_WOM_DONE (0x3BD): Audio buffer done, mix next audio buffers
      return 0;
    case 0x10u:
      CleanupHelpAndWindow();                   // WM_CLOSE: Cleanup help system and window resources
      return 0;
    case 2u:
      PostQuitMessage(0);                       // WM_DESTROY: Post quit message to exit message loop
      return 0;
    default:
      return DefWindowProcA(hWnd, uMsg, wParam, lParam);
  }
}

HDC __stdcall CreateMainWindow(struct VideoConfigEx *config)
{
  DBG_ENTER("CreateMainWindow");
  int windowPosX; // esi
  int windowPosY; // edi
  int frameWidth; // ebp
  DWORD windowStyle; // ebp
  int borderWidth; // ebp
  HDC hDC; // eax
  int captionHeight; // [esp-4h] [ebp-40h]
  int frameHeight; // [esp+10h] [ebp-2Ch]
  int borderHeight; // [esp+10h] [ebp-2Ch]
  WNDCLASSA WndClass; // [esp+14h] [ebp-28h] BYREF

  windowPosX = 0x80000000;                      // Default position = CW_USEDEFAULT (0x80000000)
  windowPosY = 0x80000000;
  g_VideoConfig.clientWidth = config->width;    // Read window width/height from config struct and store in g_VideoConfig + BITMAPINFOHEADER
  g_VideoConfig.initialClientWidth = g_VideoConfig.clientWidth;
  bmi.bmiHeader.biWidth = g_VideoConfig.clientWidth;
  g_VideoConfig.clientHeight = config->height;
  g_VideoConfig.initialClientHeight = g_VideoConfig.clientHeight;
  bmi.bmiHeader.biHeight = g_VideoConfig.clientHeight;
  g_VideoConfig.screenResX = GetSystemMetrics(0);// Get full screen resolution (SM_CXSCREEN, SM_CYSCREEN)
  g_VideoConfig.screenResY = GetSystemMetrics(1);
  g_VideoConfig.borderWidth = 0;
  captionHeight = GetSystemMetrics(4);          // titleBarHeight = SM_CYCAPTION(4) - SM_CYBORDER(6) = caption without border
  g_VideoConfig.titleBarHeight = captionHeight - GetSystemMetrics(6);
  if ( LOBYTE(config->field_24) )               // Check config resizable flag at offset +36
  {
    frameWidth = GetSystemMetrics(32);          // Resizable mode: use SM_CXFRAME(32) / SM_CYFRAME(33) for thick borders
    g_VideoConfig.borderWidth = 2 * frameWidth;
    frameHeight = GetSystemMetrics(33);
    g_VideoConfig.titleBarHeight += 2 * frameHeight;
    if ( g_VideoConfig.screenResX <= g_VideoConfig.clientWidth )// Clamp window width to screen width if too large
    {
      g_VideoConfig.clientWidth = g_VideoConfig.screenResX;
      windowPosX = frameWidth - g_VideoConfig.borderWidth;
    }
    if ( g_VideoConfig.screenResY <= g_VideoConfig.clientHeight )// Clamp window height to screen height if too large
    {
      g_VideoConfig.clientHeight = g_VideoConfig.screenResY;
      windowPosY = frameHeight - g_VideoConfig.titleBarHeight;
    }
    windowStyle = 47120384;                     // windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN (0x02CF0000)
  }
  else
  {
    borderWidth = GetSystemMetrics(5);          // Fixed mode: use SM_CXBORDER(5) / SM_CYBORDER(6) for thin borders
    g_VideoConfig.borderWidth = 2 * borderWidth;
    borderHeight = GetSystemMetrics(6);
    g_VideoConfig.titleBarHeight += 2 * borderHeight;
    if ( g_VideoConfig.screenResX == g_VideoConfig.clientWidth && g_VideoConfig.screenResY == g_VideoConfig.clientHeight )// If window matches screen size exactly, offset position to hide borders
    {
      windowPosX = borderWidth - g_VideoConfig.borderWidth;
      windowPosY = borderHeight - g_VideoConfig.titleBarHeight;
    }
    windowStyle = 34209792;                     // windowStyle = WS_CLIPCHILDREN | WS_SYSMENU | WS_MINIMIZEBOX (0x020A0000)
  }
  WndClass.style = 4128;                        // Register window class: CS_OWNDC | CS_BYTEALIGNCLIENT (0x1020)
  WndClass.lpfnWndProc = MainWndProc;
  WndClass.cbClsExtra = 0;
  WndClass.cbWndExtra = 0;
  WndClass.hInstance = g_hAppInstance;
  WndClass.hIcon = LoadIconA(g_hAppInstance, (LPCSTR)0x64);// Load application icon from resource ID 100 (0x64)
  WndClass.hCursor = GetGameCursor();           // Load cursor (arrow if config allows, else hidden)
  WndClass.hbrBackground = (HBRUSH)GetStockObject(4);// Background brush = HOLLOW_BRUSH (stock object 4)
  WndClass.lpszMenuName = 0;
  WndClass.lpszClassName = ClassName;
  RegisterClassA(&WndClass);                    // Create the main window with computed style, position, and client area size
  g_MainWindow = CreateWindowExA(
                   0,
                   ClassName,
                   (LPCSTR)&config[2].field_8,
                   windowStyle,
                   windowPosX,
                   windowPosY,
                   g_VideoConfig.borderWidth + g_VideoConfig.clientWidth,
                   g_VideoConfig.titleBarHeight + g_VideoConfig.clientHeight,
                   0,
                   0,
                   g_hAppInstance,
                   0);
  { /* Fix window size: use AdjustWindowRectEx for correct client area */
    RECT rc;
    DWORD dwStyle = (DWORD)GetWindowLongA(g_MainWindow, -16); /* GWL_STYLE */
    DWORD dwExStyle = (DWORD)GetWindowLongA(g_MainWindow, -20); /* GWL_EXSTYLE */
    rc.left = 0; rc.top = 0;
    rc.right = g_VideoConfig.initialClientWidth;
    rc.bottom = g_VideoConfig.initialClientHeight;
    AdjustWindowRectEx(&rc, dwStyle, 0, dwExStyle);
    g_VideoConfig.borderWidth = (rc.right - rc.left) - g_VideoConfig.initialClientWidth;
    g_VideoConfig.titleBarHeight = (rc.bottom - rc.top) - g_VideoConfig.initialClientHeight;
    SetWindowPos(g_MainWindow, 0, 0, 0,
      rc.right - rc.left, rc.bottom - rc.top,
      0x0016); /* SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE */
  }
  hDC = GetDC(g_MainWindow);                    // Get device context for rendering
  g_VideoConfig.hDC = hDC;
  return hDC;
}

BOOL __stdcall HandleHelpRequest(DWORD *pHelpContext)
{
  DWORD *helpData; // ebx
  UINT helpCommand; // esi

  helpData = pHelpContext;
  helpCommand = 261;                            // Default: HELP_WM_HELP (0x105) - context-sensitive help via WM_HELP
  if ( *(_WORD *)pHelpContext == 63 )           // Check if help context starts with '?' (0x3F) - indicates generic help request
  {
    helpData = 0;                               // Clear dwData for HELP_CONTENTS call
    helpCommand = 3;                            // HELP_CONTENTS (3) - show help table of contents
  }
  return WinHelpA(g_MainWindow, g_VideoConfig.helpFilePath, helpCommand, (ULONG_PTR)helpData);// WinHelpA(g_MainWindow, helpFilePath, uCommand, dwData)
}

BOOL __stdcall LaunchProcessAndWait(LPSTR lpCommandLine)
{
  struct _STARTUPINFOA StartupInfo; // [esp+8h] [ebp-54h] BYREF
  struct _PROCESS_INFORMATION ProcessInformation; // [esp+4Ch] [ebp-10h] BYREF

  qmemcpy(&StartupInfo, &g_StartupInfoTemplate, sizeof(StartupInfo));// Initialize STARTUPINFO from template, then zero out reserved/optional fields
  memcpy(&ProcessInformation, &g_ProcessInfoTemplate, sizeof(ProcessInformation));
  StartupInfo.cb = 68;                          // cb = sizeof(STARTUPINFOA) = 68
  StartupInfo.lpReserved = 0;
  StartupInfo.lpReserved2 = 0;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpDesktop = 0;
  StartupInfo.dwFlags = 0;                      // dwFlags = 0: no special window/stdio settings
  return CreateProcessA(0, lpCommandLine, 0, 0, 1, 0x20u, 0, 0, &StartupInfo, &ProcessInformation)
      && WaitForSingleObject(ProcessInformation.hProcess, 0xFFFFFFFF) != -1;// CreateProcess with NORMAL_PRIORITY_CLASS, bInheritHandles=TRUE.
                                                // If successful, wait indefinitely for the child process to terminate.
}

int __stdcall SetErrorMessage(const CHAR *a1)
{
  DBG_ENTER("SetErrorMessage");
  if ( !g_VideoConfig.pErrorMessage ) {
    g_VideoConfig.pErrorMessage = a1;
    DBG_LOG("SetErrorMessage: '%s'", a1);
  }
  return 0;
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  DbgLogInit();
  DbgInstallExceptionHandler();
  DBG_ENTER("WinMain");
  DBG_LOG("hInstance=0x%08X nShowCmd=%d", (unsigned int)hInstance, nShowCmd);
  int v4; // esi
  char *v5; // eax
  DWORD Time; // ebx
  CHAR Filename[256]; // [esp+4h] [ebp-128h] BYREF
  struct tagMSG Msg; // [esp+104h] [ebp-28h] BYREF
  char inputState[4]; // [esp+120h] [ebp-Ch]
  struct tagPOINT Point; // [esp+124h] [ebp-8h] BYREF

  GetModuleFileNameA(hInstance, Filename, 0x100u);
  DBG_LOG("Module file: %s", Filename);
  g_hAppInstance = hInstance;
  g_VideoConfig.nShowCmd = nShowCmd;
  g_VideoConfig.hDC = 0;
  g_VideoConfig.hPalette = 0;
  g_VideoConfig.hGlobalMem = 0;
  g_VideoConfig.pLockedMem = 0;
  g_VideoConfig.pErrorMessage = 0;
  g_VideoConfig.frameDelayMs = 0;
  g_BitmapHandle = 0;
  g_Reserved = 0;
  g_MemoryDeviceContext = 0;
  g_VideoWindowHandle = 0;
  g_MainWindow = 0;
  g_VideoConfig.clickY = 0;
  g_VideoConfig.clickX = 0;
  g_VideoConfig.cursorY = 0;
  g_VideoConfig.cursorX = 0;
  g_VideoConfig.clientHeight = 0;
  g_VideoConfig.clientWidth = 0;
  g_VideoConfig.saveFileHandle = -1;
  g_VideoConfig.dataFileHandle = -1;
  g_VideoConfig.charCode = 14;
  g_VideoConfig.keyIndex = 14;
  g_VideoConfig.eventCode = 14;
  { strcpy(g_ExecutablePath, ".\\PILOTS.EXE");
    v5 = g_ExecutablePath;
    DBG_LOG("g_ExecutablePath = %s", g_ExecutablePath); }
  if ( InitializeGame(v4, v5) )
  {
    CreateHelpFilePath(Filename);
    while ( 1 )
    {
      while ( 1 )
      {
        while ( 1 )
        {
          while ( PeekMessageA(&Msg, 0, 0, 0, 1u) )
          {
            if ( Msg.message == 18 )
              goto LABEL_19;
            TranslateMessage(&Msg);
            DispatchMessageA(&Msg);
          }
          if ( !g_VideoConfig.pErrorMessage )
            break;
          if ( g_VideoConfig.pErrorMessage == 1 )
            goto LABEL_19;
          DisplayErrorMessage();
          CleanupHelpAndWindow();
        }
        if ( !IsIconic(g_MainWindow) )
        {
          Time = timeGetTime();
          if ( g_VideoConfig.frameDelayMs + g_LastFrameTime < Time )
            break;
        }
      }
      inputState[0] = 1;
      g_LastFrameTime = Time;
      if ( (g_VideoConfig.pInputConfig->flags & 2) != 0 )
      {
        GetCursorPos(&Point);
        ScreenToClient(g_MainWindow, &Point);
        if ( Point.x < 0 || Point.x >= g_VideoConfig.clientWidth || Point.y < 0 || Point.y >= g_VideoConfig.clientHeight )
          inputState[0] = 0;
      }
      if ( !ProcessGameFrame(
              inputState[0],
              g_VideoConfig.charCode,
              g_VideoConfig.keyIndex,
              g_VideoConfig.eventCode,
              g_VideoConfig.clickY,
              g_VideoConfig.clickX,
              g_VideoConfig.cursorY,
              g_VideoConfig.cursorX,
              g_VideoConfig.clientHeight,
              g_VideoConfig.clientWidth) )
        break;
      g_VideoConfig.charCode = 14;
      g_VideoConfig.keyIndex = 14;
      g_VideoConfig.eventCode = 14;
    }
    CleanupHelpAndWindow();
LABEL_19:
    CleanupGameResources();
    return Msg.wParam;
  }
  else
  {
    DisplayErrorMessage();
    CleanupHelpAndWindow();
    CleanupGameResources();
    return 0;
  }
}

#pragma pack(pop)
