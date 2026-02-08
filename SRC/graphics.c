#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- graphics module ---- */

int GetCurrentScreenMode()
{
  return g_pGraphicsFunc(0, 516, 0);
}

int GetMaxScreenMode()
{
  return g_pGraphicsFunc(0, 522, 0);
}

char InitDisplay()
{
  DBG_ENTER("InitDisplay");
  DBG_LOG("InitDisplay: screenRes=%dx%d client=%dx%d pConfigEx=0x%X",
    g_VideoConfig.screenResX, g_VideoConfig.screenResY,
    g_VideoConfig.clientWidth, g_VideoConfig.clientHeight,
    (unsigned)g_VideoConfig.pConfigEx);
  if ( g_VideoConfig.screenResX >= g_VideoConfig.clientWidth && g_VideoConfig.screenResY >= g_VideoConfig.clientHeight )
  {
    ProcessMessages();
    if ( !PlayIntroVideo(
            g_VideoConfig.pConfigEx->videoWindowX,
            g_VideoConfig.pConfigEx->videoWindowY,
            g_VideoConfig.pConfigEx->videoWindowWidth,
            g_VideoConfig.pConfigEx->videoWindowHeight,
            0) )
    {
      ShowWindow(g_MainWindow, g_VideoConfig.nShowCmd);
      UpdateWindow(g_MainWindow);
      SendMessageA(g_MainWindow, 0x30Fu, 0, 0);
    }
    return 1;
  }
  else
  {
    SetErrorMessage("Game REQUIRES larger screen.");
    return 0;
  }
}

BOOL __stdcall InitializeVideoSystem(void *a1, struct VideoConfigEx *a2, struct VideoDataBuffer *a3)
{
  DBG_ENTER("InitializeVideoSystem");
  HMODULE ModuleHandleA; // eax
  FARPROC ProcAddress; // eax
  int v5; // eax
  void *pLockedMem; // eax

  g_VideoConfig.pDataBuffer = a3;
  g_VideoConfig.pConfigEx = a2;
  DBG_LOG("InitVidSys: pConfigEx=0x%X (&dword_4161F0=0x%X)",
    (unsigned)g_VideoConfig.pConfigEx, (unsigned)&dword_4161F0);
  g_VideoConfig.pInputConfig = a1;
  if ( !g_MainWindow )
    CreateMainWindow(a2);
  if ( !g_Reserved )
  {
    ModuleHandleA = GetModuleHandleA("GDI32.DLL");
    if ( ModuleHandleA )
    {
      ProcAddress = GetProcAddress(ModuleHandleA, aCreatedibsecti);
      if ( ProcAddress )
      {
        g_BitmapHandle = (HGDIOBJ)((int (__stdcall *)(HDC, BITMAPINFO *, int, int *, _DWORD, _DWORD))ProcAddress)(
                                    g_VideoConfig.hDC,
                                    &bmi,
                                    1,
                                    &g_Reserved,
                                    0,
                                    0);
        if ( g_Reserved )
        {
          g_MemoryDeviceContext = CreateCompatibleDC(g_VideoConfig.hDC);
          SelectObject(g_MemoryDeviceContext, g_BitmapHandle);
        }
      }
    }
  }
  v5 = g_Reserved;
  a3->buffer = (void *)g_Reserved;
  if ( v5 )
    a3->field_0 -= a3->size;
  a3->field_0 -= 80000;
  DBG_LOG("InitVidSys: field_0=%d size=%d g_Reserved=0x%X", a3->field_0, a3->size, g_Reserved);
  if ( !g_VideoConfig.pLockedMem )
  {
    g_VideoConfig.hGlobalMem = GlobalAlloc(2u, a3->field_0);
    g_VideoConfig.pLockedMem = GlobalLock(g_VideoConfig.hGlobalMem);
    DBG_LOG("InitVidSys: hGlobalMem=0x%X pLockedMem=0x%X", (unsigned int)g_VideoConfig.hGlobalMem, (unsigned int)g_VideoConfig.pLockedMem);
  }
  pLockedMem = g_VideoConfig.pLockedMem;
  a3->field_4 = (int)g_VideoConfig.pLockedMem;
  return pLockedMem != 0;
}

char __stdcall SetFrameRate(int frameRate)
{
  g_VideoConfig.frameDelayMs = 0;
  if ( (_BYTE)frameRate )
    g_VideoConfig.frameDelayMs = 1000 / (unsigned __int8)frameRate;
  return 1;
}

BOOL __stdcall SetPalette(char a1, char a2, int cEntries, BYTE *a4)
{
  PALETTEENTRY *palPalEntry; // esi
  int i; // edi
  int j; // ebx
  int k; // ebx
  HBRUSH StockObject; // eax
  int m; // edx
  BYTE *v10; // eax
  PALETTEENTRY *v11; // esi
  int n; // edx
  BYTE peRed; // bl
  struct { WORD palVersion; WORD palNumEntries; PALETTEENTRY palPalEntry[256]; } plpal;
  PALETTEENTRY *v16_ptr; /* alias for &plpal.palPalEntry[246] */
  HGDIOBJ h; // [esp+410h] [ebp-18h]
  HGDIOBJ ho; // [esp+414h] [ebp-14h]
  RECT rc; // [esp+418h] [ebp-10h] BYREF

  palPalEntry = plpal.palPalEntry;
  v16_ptr = &plpal.palPalEntry[246];
  if ( g_VideoConfig.clientWidth && g_VideoConfig.clientHeight )
  {
    if ( a2 )
    {
      ho = CreatePen(0, 1, *(_DWORD *)&aCreatedibsecti[4 * a2 + 15]);
      h = SelectObject(g_VideoConfig.hDC, ho);
      for ( i = 0; i < 8; ++i )
      {
        for ( j = i; j < g_VideoConfig.clientWidth; j += 8 )
        {
          MoveToEx(g_VideoConfig.hDC, j, 0, 0);
          LineTo(g_VideoConfig.hDC, j, g_VideoConfig.clientHeight);
        }
        for ( k = i; k < g_VideoConfig.clientHeight; k += 8 )
        {
          MoveToEx(g_VideoConfig.hDC, 0, k, 0);
          LineTo(g_VideoConfig.hDC, g_VideoConfig.clientWidth, k);
        }
      }
      SelectObject(g_VideoConfig.hDC, h);
      DeleteObject(ho);
    }
    else
    {
      rc.left = 0;
      rc.top = 0;
      rc.right = g_VideoConfig.clientWidth;
      rc.bottom = g_VideoConfig.clientHeight;
      StockObject = (HBRUSH)GetStockObject(4);
      FillRect(g_VideoConfig.hDC, &rc, StockObject);
    }
  }
  plpal.palVersion = 768;
  plpal.palNumEntries = cEntries;
  if ( cEntries > 0 )
  {
    for ( m = 0; m < cEntries; ++m )
    {
      palPalEntry->peRed = *a4;
      palPalEntry->peGreen = a4[1];
      v10 = a4 + 2;
      a4 += 3;
      palPalEntry->peBlue = *v10;
      palPalEntry->peFlags = 4;
      ++palPalEntry;
    }
  }
  if ( a1 )
  {
    GetSystemPaletteEntries(g_VideoConfig.hDC, 0, 0xAu, plpal.palPalEntry);
    GetSystemPaletteEntries(g_VideoConfig.hDC, 0xF6u, 0xAu, v16_ptr);
  }
  if ( g_VideoConfig.hPalette )
    DeleteObject((HGDIOBJ)g_VideoConfig.hPalette);
  g_VideoConfig.hPalette = (int)CreatePalette((LOGPALETTE *)&plpal);
  if ( g_MemoryDeviceContext )
  {
    v11 = plpal.palPalEntry;
    if ( cEntries > 0 )
    {
      for ( n = 0; n < cEntries; ++n )
      {
        peRed = v11->peRed;
        v11->peRed = v11->peBlue;
        v11->peBlue = peRed;
        ++v11;
      }
    }
    SetDIBColorTable(g_MemoryDeviceContext, 0, cEntries, (const RGBQUAD *)plpal.palPalEntry);
  }
  if ( g_MainWindow )
    SendMessageA(g_MainWindow, 0x30Fu, 0, 0);
  return g_VideoConfig.hPalette != 0;
}

int __stdcall BlitRectToScreen(int bmp_height, int cy, int cx, int y, int x, void *lpBits)
{                                               // Prefer BitBlt through compatible memory DC (DIB section path)
  if ( g_MemoryDeviceContext )
return BitBlt(g_VideoConfig.hDC, x, y, cx, cy, g_MemoryDeviceContext, x, y, 0xCC0020u);
// BitBlt: src and dst coordinates are identical (1:1 copy)
  else
return StretchDIBits(g_VideoConfig.hDC, x, y, cx, cy, x, bmp_height - y - cy, cx, cy, lpBits, &bmi, 1u, 0xCC0020u);
// StretchDIBits fallback: adjust srcY for bottom-up bitmap (bmp_height - y - cy)
}

char __stdcall BlitScreenWithDissolve(
        char use_dissolve,
        int bmp_height,
        int unused,
        int rect_height,
        int rect_width,
        int rect_y,
        int rect_x,
        void *lpBits)
{                                               // Early out if window has zero dimensions
int tileX;
// esi
int i;
// ebx
int passIndex;
// [esp+10h] [ebp-Ch]
int *pPatternY;
// [esp+14h] [ebp-8h]
int *pPatternX;
// [esp+18h] [ebp-4h]

  if ( g_VideoConfig.clientWidth && g_VideoConfig.clientHeight )
  {                                             // Dissolve mode: iterate 16 passes with different tile offsets
    if ( use_dissolve )
    {
      passIndex = 0;                            // 16 dissolve passes, each using a different (x,y) offset from pattern tables
      pPatternY = (int *)&g_DissolvePatternY;
      pPatternX = (int *)&g_DissolvePatternX;
      do
      {                                         // Iterate X across screen in 64-pixel steps starting at pattern offset
        for ( tileX = *pPatternX; tileX < g_VideoConfig.clientWidth; tileX += 64 )
        {                                       // Iterate Y across screen in 64-pixel steps starting at pattern offset
          for ( i = *pPatternY; i < g_VideoConfig.clientHeight; i += 64 )
          {
            BlitRectToScreen(bmp_height, 16, 16, i, tileX, lpBits);// Blit one 16x16 tile to screen
            GdiFlush();                         // Flush GDI to make each tile visible immediately (dissolve animation)
          }
        }
        ++pPatternY;
        ++pPatternX;
        ++passIndex;
      }
      while ( passIndex < 16 );
    }
    else
    {
BlitRectToScreen(bmp_height, rect_height, rect_width, rect_y, rect_x, lpBits);
// Non-dissolve: single rectangle blit
    }
  }
  return 1;
}

BOOL FlushGDI(void)
{
  return GdiFlush();
}

char __stdcall PumpWindowMessages(DWORD *timestamp)
{
  char result; // al
  struct tagMSG Msg; // [esp+0h] [ebp-1Ch] BYREF

  if ( !*timestamp )
    g_VideoConfig.eventCode = 14;
  if ( PeekMessageA(&Msg, 0, 0, 0, 1u) )
  {
    while ( Msg.message != 18 )
    {
      TranslateMessage(&Msg);
      DispatchMessageA(&Msg);
      if ( !PeekMessageA(&Msg, 0, 0, 0, 1u) )
        goto LABEL_7;
    }
    g_VideoConfig.pErrorMessage = 1;
    return -125;
  }
  else
  {
LABEL_7:
    *timestamp = timeGetTime();
    result = g_VideoConfig.eventCode;
    g_VideoConfig.eventCode = 14;
  }
  return result;
}

#pragma pack(pop)
