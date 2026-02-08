#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- audio module ---- */

int __stdcall PlayMusicTrack(int a1)
{
  int result; // eax

  result = StartMusicPlayback(0, -1, a1);
  byte_411710 = result;
  return result;
}

int __stdcall SwitchMusicTrack(int a1)
{
  if ( g_RenderingInProgress )
    return 1;
  ResetGameState();
  if ( !byte_4162B5 )
    return 1;
  g_CurrentMusicIndex = a1;
  return (char)PlayMusicTrack(*((_DWORD *)&g_MusicTrackArray->trackData + a1));
}

int __stdcall TriggerSoundSlot(int a1)
{
  if ( !g_RenderingInProgress && g_SoundEnabledFlag )
    PlaySoundEffect(1, 0, -1, (int *)g_SoundSlotArray[a1].soundDataPtr);
  return 1;
}

MMRESULT CleanupWaveOut()
{
  struct wavehdr_tag *v0; // ebx

  v0 = &pwh;
  do
    waveOutUnprepareHeader(g_hWaveOut, v0++, 0x20u);
  while ( v0 < (struct wavehdr_tag *)&g_hMidiOut );
  return waveOutClose(g_hWaveOut);
}

int __stdcall InitWaveOut(DWORD_PTR *a1)
{
  DBG_ENTER("InitWaveOut");
  MMRESULT v1; // edi
  DWORD *p_dwFlags; // ebx
  struct wavehdr_tag *v3; // ebp
  DWORD_PTR *v4; // esi
  MMRESULT v5; // eax
  WAVEFORMATEX pwfx; // [esp+10h] [ebp-18h] BYREF
  DWORD_PTR *v8; // [esp+24h] [ebp-4h]

  pwfx.wFormatTag = 1;
  pwfx.nChannels = 1;
  pwfx.nAvgBytesPerSec = 11025;
  pwfx.nSamplesPerSec = 11025;
  pwfx.nBlockAlign = 1;
  pwfx.wBitsPerSample = 8;
  pwfx.cbSize = 0;
  v1 = waveOutOpen(&g_hWaveOut, 0xFFFFFFFF, &pwfx, *a1, 0, 0x10000u);
  if ( v1 )
  {
    sndPlaySoundA(0, 0);
    v1 = waveOutOpen(&g_hWaveOut, 0xFFFFFFFF, &pwfx, *a1, 0, 0x10000u);
  }
  if ( !v1 )
  {
    if ( !waveOutGetID(g_hWaveOut, &g_WaveDeviceID) )
    {
      p_dwFlags = &pwh.dwFlags;
      v3 = &pwh;
      v4 = a1;
      v8 = a1 + 4096;
      do
      {
        *p_dwFlags = 1;
        *(p_dwFlags - 4) = (DWORD)v4;
        *(p_dwFlags - 3) = 2048;
        v5 = waveOutPrepareHeader(g_hWaveOut, v3, 0x20u);
        *(_BYTE *)p_dwFlags |= 1u;
        if ( v5 )
          break;
        p_dwFlags += 8;
        ++v3;
        v4 += 512;
      }
      while ( v8 > v4 );
      if ( !v5 )
        return 1;
    }
    CleanupWaveOut();
  }
  return 0;
}

unsigned int GetNextWaveBuffer()
{
  DWORD *p_dwFlags; // ecx

  p_dwFlags = &pwh.dwFlags;
  do
  {
    if ( (*(_BYTE *)p_dwFlags & 1) != 0 )
      return (unsigned int)((char *)p_dwFlags - (char *)&pwh.dwFlags) >> 5;
    p_dwFlags += 8;
  }
  while ( p_dwFlags < (DWORD *)&g_MidiTicks );
  waveOutReset(g_hWaveOut);
  return 0;
}

BOOL WriteWaveBuffer()
{
  unsigned __int8 v0; // al

  v0 = GetNextWaveBuffer();
  return waveOutWrite(g_hWaveOut, &pwh + v0, 0x20u) == 0;
}

BOOL ResetWave()
{
  return waveOutReset(g_hWaveOut) == 0;
}

BOOL PauseWave()
{
  return waveOutPause(g_hWaveOut) == 0;
}

BOOL ResumeWave()
{
  return waveOutRestart(g_hWaveOut) == 0;
}

int __stdcall GetWaveVolume(DWORD a1)
{
  int v1; // eax
  DWORD pdwVolume; // [esp+0h] [ebp-4h] BYREF

  pdwVolume = a1;
  waveOutGetVolume((HWAVEOUT)g_WaveDeviceID, &pdwVolume);
  if ( (unsigned __int16)pdwVolume <= HIWORD(pdwVolume) )
    v1 = HIWORD(pdwVolume);
  else
    v1 = (unsigned __int16)pdwVolume;
  return v1 >> 8;
}

int __stdcall SetWaveVolume(unsigned __int8 a1)
{
  int v1; // ebx
  int v3; // [esp+4h] [ebp-4h]

  v1 = a1 << 8;
  LOBYTE(v1) = -1;
  v3 = waveOutSetVolume((HWAVEOUT)g_WaveDeviceID, v1 | (v1 << 16)) == 0;
  if ( !a1 )
    waveOutReset(g_hWaveOut);
  return v3;
}

MMRESULT ResetMidi()
{
  if ( g_MidiTimerID )
  {
    timeKillEvent(g_MidiTimerID);
    g_MidiTimerID = 0;
  }
  return midiOutReset(g_hMidiOut);
}

int ReadMidiVarLen()
{
  int v0; // edx

  v0 = *(unsigned __int8 *)g_MidiDataPtr++;
  if ( (v0 & 0x80u) != 0 )
    return (*(unsigned __int8 *)g_MidiDataPtr++ << 7) | v0 & 0x7F;
  return v0;
}

MMRESULT ProcessMidiEvents()
{
  unsigned __int8 v0; // dl
  MMRESULT result; // eax
  DWORD v2; // ebx
  int v3; // eax
  char v4; // dl
  int v5; // ebx
  char v6; // [esp+4h] [ebp-4h]
  int v7; // [esp+5h] [ebp-3h]

  while ( 1 )
  {
    v6 = 0;
    v0 = *(_BYTE *)g_MidiDataPtr;
    if ( *(_BYTE *)g_MidiDataPtr <= 0x7Fu )
      v0 = byte_411D94;
    else
      ++g_MidiDataPtr;
    byte_411D94 = v0;
    if ( v0 != 240 && v0 != 247 )
      break;
    g_MidiDataPtr += ReadMidiVarLen();
LABEL_21:
    if ( !v6 )
    {
      v5 = ReadMidiVarLen();
      if ( v5 )
      {
        dword_411D7C += 10 * v5;
        result = timeGetTime();
        if ( result < dword_411D7C )
          return result;
      }
    }
  }
  if ( v0 != 255 )
  {
    v2 = v0 | (*(unsigned __int8 *)g_MidiDataPtr++ << 8);
    v7 = 1;
    v3 = v0 & 0xF0;
    if ( v3 == 128 || v3 == 144 )
    {
      v7 = g_MidiVolume != 0;
    }
    else if ( v3 == 192 || v3 == 208 )
    {
LABEL_19:
      if ( v7 )
        midiOutShortMsg(g_hMidiOut, v2);
      goto LABEL_21;
    }
    v4 = *(_BYTE *)g_MidiDataPtr++;
    v2 = (unsigned __int16)v2 | ((v4 & 0x7F) << 16);
    v6 = v4 & 0x80;
    goto LABEL_19;
  }
  if ( g_MidiPlaying == 0xFFFF )
    return ResetMidi();
  g_MidiLooping = g_MidiPlaying;
  g_MidiDataPtr = g_MidiDataStart;
  return g_MidiDataStart;
}

void __stdcall MidiTimerCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
  if ( g_MidiLooping )
  {
    if ( --g_MidiLooping )
      return;
    dword_411D7C = timeGetTime();
  }
  if ( timeGetTime() >= __PAIR64__(g_MidiTicks, dword_411D7C) )
  {
    ++g_MidiTicks;
    ProcessMidiEvents();
    --g_MidiTicks;
  }
}

MMRESULT CleanupMidiOut()
{
  MMRESULT result; // eax

  result = ResetMidi();
  if ( g_TimerResolution )
  {
    result = timeEndPeriod(g_TimerResolution);
    g_TimerResolution = 0;
  }
  if ( g_hMidiOut )
  {
    result = midiOutClose(g_hMidiOut);
    g_hMidiOut = 0;
  }
  return result;
}

int InitMidiOut()
{
  DBG_ENTER("InitMidiOut");
  UINT wPeriodMin; // eax
  UINT wPeriodMax; // eax
  struct timecaps_tag ptc; // [esp+0h] [ebp-8h] BYREF

  if ( !midiOutOpen(&g_hMidiOut, 0xFFFFFFFF, 0, 0, 0) )
  {
    g_TimerResolution = 0;
    g_MidiVolume = 1;
    if ( !midiOutGetID(g_hMidiOut, &g_MidiDeviceID) && !timeGetDevCaps(&ptc, 8u) )
    {
      wPeriodMin = ptc.wPeriodMin;
      if ( ptc.wPeriodMin <= 0xA )
        wPeriodMin = 10;
      if ( wPeriodMin >= ptc.wPeriodMax )
        wPeriodMax = ptc.wPeriodMax;
      else
        wPeriodMax = ptc.wPeriodMin <= 0xA ? 10 : ptc.wPeriodMin;
      g_TimerResolution = wPeriodMax;
      if ( !timeBeginPeriod(wPeriodMax) )
        return 1;
    }
    CleanupMidiOut();
  }
  return 0;
}

int __stdcall StartMidiPlayback(int a1)
{
  int result; // eax

  ResetMidi();
  g_MidiDataPtr = a1 + 4;
  if ( *(_BYTE *)(a1 + 4) == 0xF8 )
  {
    ++g_MidiDataPtr;
    ReadMidiVarLen();
    ++g_MidiDataPtr;
  }
  g_MidiDataStart = g_MidiDataPtr;
  g_MidiPlaying = 1;
  g_MidiLooping = 1;
  g_MidiTicks = 0;
  g_MidiTimerID = timeSetEvent(0xAu, g_TimerResolution, MidiTimerCallback, 0, 1u);
  result = 1;
  if ( !g_MidiTimerID )
  {
    ResetMidi();
    return 0;
  }
  return result;
}

char StopMidiPlayback()
{
  ResetMidi();
  return 1;
}

int __stdcall GetMidiVolume(DWORD a1)
{
  int v1; // eax
  DWORD pdwVolume; // [esp+0h] [ebp-4h] BYREF

  pdwVolume = a1;
  midiOutGetVolume((HMIDIOUT)g_MidiDeviceID, &pdwVolume);
  if ( (unsigned __int16)pdwVolume <= HIWORD(pdwVolume) )
    v1 = HIWORD(pdwVolume);
  else
    v1 = (unsigned __int16)pdwVolume;
  return v1 >> 8;
}

int __stdcall SetMidiVolume(unsigned __int8 a1)
{
  int v1; // ebx
  int v3; // [esp+4h] [ebp-4h]

  g_MidiVolume = a1;
  v1 = a1 << 8;
  LOBYTE(v1) = -1;
  v3 = midiOutSetVolume((HMIDIOUT)g_MidiDeviceID, v1 | (v1 << 16)) == 0;
  if ( !a1 )
    midiOutReset(g_hMidiOut);
  return v3;
}

char __cdecl MultimediaStub_NoArgs()
{
  return 1;
}

char __stdcall MultimediaStub_OneArg(int param)
{
  return 1;
}

char __stdcall MultimediaStub_StartMidi(int param)
{
  return 1;
}

HGLOBAL WinNT_CleanupMultimedia()
{
  HGLOBAL result; // eax

  if ( g_pGraphicsFunc )
    result = (HGLOBAL)g_pGraphicsFunc(0, 513, 0);
  if ( g_pUnregisterFunc )
    result = (HGLOBAL)g_pUnregisterFunc(g_hAppInstance);
  if ( g_hGraphicsDLL )
    result = (HGLOBAL)FreeLibrary(g_hGraphicsDLL);
  if ( g_hGraphicsMemory )
  {
    result = GlobalFree(g_hGraphicsMemory);
    g_hGraphicsMemory = 0;
  }
  return result;
}

int WinNT_WriteWaveBuffer()
{
  return g_pGraphicsFunc(0, 514, 0);
}

int WinNT_ResetWave()
{
  return g_pGraphicsFunc(0, 515, 0);
}

int WinNT_GetNextWaveBuffer()
{
  return g_pGraphicsFunc(0, 517, 0);
}

int WinNT_PauseWave()
{
  return g_pGraphicsFunc(0, 518, 0);
}

int WinNT_ResumeWave()
{
  return g_pGraphicsFunc(0, 519, 0);
}

int __stdcall WinNT_SetWaveVolume(unsigned __int8 a1)
{
  return g_pGraphicsFunc(0, a1, 0);
}

int __stdcall WinNT_StartMidiPlayback(_DWORD *Src)
{
  int result; // eax

  if ( g_hGraphicsMemory )
  {
    memcpy(g_hGraphicsMemory, Src, *Src + 4);
    return g_pGraphicsFunc(g_hGraphicsMemory, 520, 0);
  }
  return result;
}

int WinNT_StopMidiPlayback()
{
  return g_pGraphicsFunc(0, 521, 0);
}

int __stdcall WinNT_SetMidiVolume(unsigned __int8 a1)
{
  return g_pGraphicsFunc(0, a1 + 256, 0);
}

char __stdcall InitGraphics_WinNT(int a1)
{
  DBG_ENTER("InitGraphics_WinNT");
  HMODULE LibraryA; // eax

  LibraryA = LoadLibraryA("GMW.DLL");
  g_hGraphicsDLL = LibraryA;
  if ( LibraryA )
  {
    g_pRegisterFunc = (int (__stdcall *)(_DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD, _DWORD))GetProcAddress(
                                                                                                   LibraryA, "UTRegister");
    if ( g_pRegisterFunc )
    {
      g_pUnregisterFunc = (int (__stdcall *)(_DWORD))GetProcAddress(g_hGraphicsDLL, "UTUnRegister");
      if ( g_pUnregisterFunc )
      {
        if ( g_pRegisterFunc(g_hAppInstance, "GMW.DLL", 0, 2, &g_pGraphicsFunc, 0, 0) )
        {
          if ( g_pGraphicsFunc(a1, 512, 0) )
          {
            g_hGraphicsMemory = GlobalAlloc(0, 0xF000u);
            if ( g_hGraphicsMemory )
            {
              g_pfnWriteWaveBuffer = WinNT_WriteWaveBuffer;
              g_pfnResetWave = WinNT_ResetWave;
              g_pfnGetWaveVolume = GetCurrentScreenMode;
              g_pfnSetWaveVolume = (int (*)(int))WinNT_SetWaveVolume;
              g_pfnGetNextWaveBuffer = WinNT_GetNextWaveBuffer;
              g_pfnPauseWave = WinNT_PauseWave;
              g_pfnResumeWave = WinNT_ResumeWave;
              g_pfnStartMidiPlayback = (int (*)(int))WinNT_StartMidiPlayback;
              g_pfnStopMidiPlayback = WinNT_StopMidiPlayback;
              g_pfnGetMidiVolume = GetMaxScreenMode;
              g_pfnSetMidiVolume = (int (*)(int))WinNT_SetMidiVolume;
              g_pfnCleanupMultimedia = (int (*)())WinNT_CleanupMultimedia;
              return 1;
            }
          }
          WinNT_CleanupMultimedia();
        }
      }
    }
  }
  return 0;
}

int CleanupMultimedia()
{
  DBG_ENTER("CleanupMultimedia");
  CleanupWaveOut();
  return CleanupMidiOut();
}

char __stdcall InitDirectDraw_Win95(DWORD_PTR *pHwnd)
{
  DBG_ENTER("InitDirectDraw_Win95");
  int audioInitResult; // [esp-4h] [ebp-4h]

  audioInitResult = (unsigned __int8)InitWaveOut(pHwnd);
  if ( (unsigned __int8)InitMidiOut() | audioInitResult )
  {
    g_pfnWriteWaveBuffer = WriteWaveBuffer;
    g_pfnResetWave = ResetWave;
    g_pfnGetWaveVolume = (_DWORD (*)())GetWaveVolume;
    g_pfnSetWaveVolume = (int (*)(int))SetWaveVolume;
    g_pfnGetNextWaveBuffer = (int (*)())GetNextWaveBuffer;
    g_pfnPauseWave = PauseWave;
    g_pfnResumeWave = ResumeWave;
    g_pfnStartMidiPlayback = (int (*)())StartMidiPlayback;
    g_pfnStopMidiPlayback = (int (*)())StopMidiPlayback;
    g_pfnGetMidiVolume = (_DWORD (*)())GetMidiVolume;
    g_pfnSetMidiVolume = (int (*)(int))SetMidiVolume;
    g_pfnCleanupMultimedia = CleanupMultimedia;
    return 1;
  }
  else
  {
    CleanupMultimedia();
    return 0;
  }
}

void __stdcall CopyAudioBufferAndPadSilence(unsigned int byteSize, const void *srcBuffer, void *dstBuffer)
{
  qmemcpy(dstBuffer, srcBuffer, 4 * (byteSize >> 2));// Copy srcBuffer -> dstBuffer (DWORD-aligned fast copy)
  if ( byteSize >> 2 != 512 )                   // 0x80808080 = silence pattern for unsigned 8-bit PCM (128 per sample)
    memset((char *)dstBuffer + 4 * (byteSize >> 2), 0x80u, 4 * (512 - (byteSize >> 2)));// Pad remaining buffer with silence
}

int __stdcall BlendPixelBuffersSaturated(unsigned int a1, int *a2, int *a3)
{
  unsigned int v5; // ecx
  int v6; // edx
  int v7; // sf
  int v8; // of
  int v9; // eax
  int v10; // sf
  char v11; // ah
  __int16 v12; // dx
  char v13; // dh
  int v14; // sf
  int v15; // eax
  int v16; // sf
  char v17; // ah
  int result; // eax

  v5 = a1 >> 2;
  do
  {
    v9 = *a2++;
    v6 = *a3;
    LOBYTE(v9) = v9 + 0x80;
    LOBYTE(v6) = *a3 + 0x80;
    BYTE1(v9) += 0x80;
    BYTE1(v6) = BYTE1(*a3) + 0x80;
    v8 = __OFADD__((_BYTE)v6, (_BYTE)v9);
    v7 = (char)(v6 + v9) < 0;
    LOBYTE(v9) = v6 + v9;
    if ( v8 )
    {
      if ( v7 )
        LOBYTE(v9) = -16;
      else
        LOBYTE(v9) = 0;
    }
    else
    {
      LOBYTE(v9) = v9 + 0x80;
    }
    v8 = __OFADD__(BYTE1(v6), BYTE1(v9));
    v10 = (char)(BYTE1(v6) + BYTE1(v9)) < 0;
    v11 = BYTE1(v6) + BYTE1(v9);
    if ( v8 )
    {
      if ( v10 )
        BYTE1(v9) = -16;
      else
        BYTE1(v9) = 0;
    }
    else
    {
      BYTE1(v9) = v11 + 0x80;
    }
    v15 = __ROL4__(v9, 16);
    v12 = __ROL4__(v6, 16);
    LOBYTE(v15) = v15 + 0x80;
    LOBYTE(v12) = v12 + 0x80;
    BYTE1(v15) += 0x80;
    v13 = HIBYTE(v12) + 0x80;
    v8 = __OFADD__((_BYTE)v12, (_BYTE)v15);
    v14 = (char)(v12 + v15) < 0;
    LOBYTE(v15) = v12 + v15;
    if ( v8 )
    {
      if ( v14 )
        LOBYTE(v15) = -16;
      else
        LOBYTE(v15) = 0;
    }
    else
    {
      LOBYTE(v15) = v15 + 0x80;
    }
    v8 = __OFADD__(v13, BYTE1(v15));
    v16 = (char)(v13 + BYTE1(v15)) < 0;
    v17 = v13 + BYTE1(v15);
    if ( v8 )
    {
      if ( v16 )
        BYTE1(v15) = -16;
      else
        BYTE1(v15) = 0;
    }
    else
    {
      BYTE1(v15) = v17 + 0x80;
    }
    result = __ROL4__(v15, 16);
    --v5;
    *a3++ = result;
  }
  while ( v5 );
  return result;
}

char __stdcall InitGraphics(_BYTE *pMaxScreen, _BYTE *pCurrentScreen, int unused, unsigned __int8 graphicsConfig)
{
  DBG_ENTER("InitGraphics");
  signed int windowsVersion; // ebx
  char initResult; // al
  int *bufferPtr; // ebx

  windowsVersion = GetVersion();
  g_hMainWindow[0] = (DWORD_PTR)g_MainWindow;
  if ( windowsVersion >= 0 || (unsigned __int8)windowsVersion >= 4u )
    initResult = InitDirectDraw_Win95(g_hMainWindow);
  else
    initResult = InitGraphics_WinNT((int)g_hMainWindow);
  if ( initResult )
  {
    *pCurrentScreen = g_pfnGetWaveVolume();
    *pMaxScreen = g_pfnGetMidiVolume();
    g_GraphicsConfig = graphicsConfig;
    g_GraphicsState1 = 0;
    g_GraphicsState2 = 0;
    bufferPtr = g_GraphicsBuffers;
    do
    {
      *bufferPtr = 0;
      bufferPtr += 4;
    }
    while ( &g_GraphicsBuffers[4 * g_GraphicsConfig] > bufferPtr );
  }
  return 1;
}

int __stdcall StartMusicPlayback(int a1, int a2, int param)
{
  return ((int (__stdcall *)(int))g_pfnStartMidiPlayback)(param);
}

char __stdcall PlaySoundEffect(char a1, char a2, char a3, int *a4)
{
  int v4; // ebx
  int v5; // ecx
  int v6; // edx
  int v7; // ebx

  v4 = g_GraphicsState2;
  do
  {
    if ( !g_GraphicsBuffers[4 * v4] )
      break;
    if ( ++v4 >= (unsigned int)g_GraphicsConfig )
      v4 = 0;
  }
  while ( v4 != g_GraphicsState2 );
  g_GraphicsState2 = v4;
  v5 = *a4;
  LOBYTE(v5) = *a4 & 0xFC;
  v6 = 4 * v4;
  dword_411DB8[v6] = v5;
  g_GraphicsBuffers[v6] = (int)(a4 + 1);
  dword_411DB4[v6] = (int)(a4 + 1);
  byte_411DBD[v6 * 4] = a2;
  byte_411DBC[v6 * 4] = a3;
  byte_411DBE[v6 * 4] = a1;
  if ( a1 >= 0 )
  {
    if ( g_GraphicsState1 )
      v7 = 0;
    else
      v7 = 2;
  }
  else
  {
    v7 = 1;
  }
  if ( v7 == 2 )
  {
    MixAudioBuffers();
  }
  else if ( v7 != 1 )
  {
    return 1;
  }
  MixAudioBuffers();
  return 1;
}

int __stdcall SetMusicVolume(int a1)
{
  return ((int (__stdcall *)(int))g_pfnSetMidiVolume)(a1);
}

int __stdcall SetSoundVolume(int volume)
{
  int result; // eax

  LOBYTE(result) = ((int (__stdcall *)(int))g_pfnSetWaveVolume)(volume);
  return result;
}

char __cdecl StopMidiMusic()
{
  return g_pfnStopMidiPlayback();
}

char ClearInputBuffer()
{
  int *v0; // ecx

  v0 = g_GraphicsBuffers;
  do
  {
    *v0 = 0;
    v0 += 4;
  }
  while ( &g_GraphicsBuffers[4 * g_GraphicsConfig] > v0 );
  return g_pfnResetWave();
}

unsigned __int8 MixAudioBuffers()
{
  int v0; // edi
  unsigned __int8 result; // al
  int *v2; // ebp
  int *v3; // ebx
  unsigned int v4; // esi
  int v5; // zf
  int *v6; // [esp-8h] [ebp-18h]

  v0 = 0;
  result = g_pfnGetNextWaveBuffer();
  v2 = (int *)&g_hMainWindow[512 * result];
  v3 = g_GraphicsBuffers;
  do
  {
    if ( *v3 )
    {
      v4 = v3[2];
      if ( v4 > 0x800 )
        v4 = 2048;
      v6 = (int *)v3[1];
      if ( v0 )
        result = BlendPixelBuffersSaturated(v4, v6, v2);
      else
        CopyAudioBufferAndPadSilence(v4, v6, (char *)v2);
      v5 = v3[2] == v4;
      v3[2] -= v4;
      if ( v5 )
        *v3 = 0;
      else
        v3[1] += v4;
      ++v0;
    }
    v3 += 4;
  }
  while ( &g_GraphicsBuffers[4 * g_GraphicsConfig] > v3 );
  g_GraphicsState1 = v0;
  if ( v0 )
    return g_pfnWriteWaveBuffer();
  return result;
}

char __cdecl PauseWavePlayback()
{
  return g_pfnPauseWave();
}

char __cdecl ResumeWavePlayback()
{
  return g_pfnResumeWave();
}

void __cdecl ShutdownMultimedia()
{
  DBG_ENTER("ShutdownMultimedia");
  g_pfnCleanupMultimedia();
  g_pfnStartMidiPlayback = (int (*)(int))MultimediaStub_StartMidi;// Reset all multimedia function pointers to no-op stubs
  g_pfnResumeWave = MultimediaStub_NoArgs;
  g_pfnPauseWave = (int (*)())MultimediaStub_NoArgs;
  g_pfnGetNextWaveBuffer = (_DWORD (*)())MultimediaStub_NoArgs;
  g_pfnGetMidiVolume = (_DWORD (*)())MultimediaStub_NoArgs;
  g_pfnGetWaveVolume = (_DWORD (*)())MultimediaStub_NoArgs;
  g_pfnStopMidiPlayback = (int (*)())MultimediaStub_NoArgs;
  g_pfnResetWave = (_DWORD (*)())MultimediaStub_NoArgs;
  g_pfnWriteWaveBuffer = (_DWORD (*)())MultimediaStub_NoArgs;
  g_pfnSetMidiVolume = (int (*)(int))MultimediaStub_OneArg;
  g_pfnSetWaveVolume = (int (*)(int))MultimediaStub_OneArg;
  g_pfnCleanupMultimedia = nullsub_6;           // Set cleanup handler itself to nullsub (idempotent)
}

#pragma pack(pop)
