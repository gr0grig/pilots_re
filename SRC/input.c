#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- input module ---- */

void __cdecl ClearInputAndShutdownAudio()
{
  ClearInputBuffer();
  ShutdownMultimedia();
}

void RestoreScreenAndMusic()
{
  InitGraphics((_BYTE *)&g_CurrentScreen + 1, &g_CurrentScreen, 0, g_GraphicsConfigPtr);// Reinitialize DirectDraw graphics subsystem; saves current wave/midi volumes into g_CurrentScreen
  if ( g_CurrentMusicIndex != -1 )              // Check if background music was playing before interruption
    SwitchMusicTrack(g_CurrentMusicIndex);      // Resume the music track that was playing before video/process
}

HCURSOR GetGameCursor()
{
  if ( BYTE1(g_VideoConfig.pConfigEx->field_24) )
    return LoadCursorA(0, (LPCSTR)0x7F00);
  else
    return 0;
}

#pragma pack(pop)
