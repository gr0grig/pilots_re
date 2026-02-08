#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- native_funcs module ---- */

int Native_EnableContinueRendering()
{
  g_ContinueRendering = 1;
  return 1;
}

BOOL Native_IsOverlaySlotEmpty()
{
  return *(_WORD *)(g_GameFlags1 + 10) == 0xFFFF;
}

BOOL __stdcall Native_CheckCurrentFontSlot(int a1)
{
  int v1; // ebx

  v1 = *(unsigned __int16 *)(g_GameFlags1 + 8);
  return v1 != 0xFFFF && ((int)FindFontSlotByPointer((int)&g_RenderObjectsStart[-v1]) - (int)g_FontSlotArray) / 8 == a1;
}

BOOL Native_IsInputModeActive()
{
  return (*(_BYTE *)(g_GameFlags1 + 4) & 0x90) == 16;
}

BOOL Native_IsClickModeActive()
{
  return (*(_BYTE *)(g_GameFlags1 + 4) & 0xA0) == 32;
}

BOOL __stdcall Native_CheckInputFlags(int a1)
{
  return (*(_BYTE *)(g_GameFlags1 + 4) & 0xB0) == a1;
}

BOOL __stdcall Native_CheckStateFlags(int a1)
{
  return (*(_BYTE *)(g_GameFlags1 + 4) & 0x4F) == a1;
}

int __stdcall Native_CheckDirectionMatch(char a1)
{
  if ( (*(_BYTE *)(g_GameFlags1 + 4) & 0x40) != 0 && (*(_BYTE *)(g_GameFlags1 + 4) & 8) == (a1 & 8) )
    return CheckRotatedDirectionMatch(a1 & 7, *(_BYTE *)(g_GameFlags1 + 4) & 7);
  else
    return 0;
}

BOOL __stdcall Native_CheckRenderLayer(int a1)
{
  return *(unsigned __int8 *)(g_GameFlags1 + 5) == a1;
}

BOOL Native_IsScreenTransition()
{
  return *(_BYTE *)(g_GameFlags1 + 2) == 0xFE;
}

BOOL __stdcall Native_CheckScreenId(int a1)
{
  return *(unsigned __int8 *)(g_GameFlags1 + 2) == a1;
}

int __stdcall CheckAnimationFrameAvailable(int a1)
{
  return *((unsigned __int8 *)g_AnimationSlotArray[a1].frameBitmask + (*(unsigned __int8 *)(g_GameFlags1 + 2) >> 3))
       & (1 << (*(_BYTE *)(g_GameFlags1 + 2) & 7));
}

int __stdcall Native_CheckVideoCounter(_BYTE *a1)
{
  _BYTE *v1; // ecx

  v1 = a1;
  if ( !*a1 )
    return 0;
  while ( *v1 != (_BYTE)video_recursion_counter )
  {
    if ( !*++v1 )
      return 0;
  }
  return 1;
}

__declspec(noreturn) void __stdcall Native_RestoreGameSnapshot(int snapIdx)
{
  ExecuteGameLoop(0, snapIdx);
  LongjmpRestart();
}

int __stdcall Native_RenderAtCurrentTile(int a1)
{
  BYTE2(g_PreviousScreen) = 1;
  return CreateFontRenderObject(0, g_TileHeight * g_CurrentTileY_Wrapped, g_TileWidth * g_CurrentTileX_Wrapped, a1);
}

int __stdcall Native_MeasureTextAtTile(int a1, int a2)
{
  return SetupTextDisplay(g_TileHeight * g_CurrentTileY_Wrapped, g_TileWidth * g_CurrentTileX_Wrapped, a2, a1);
}

int __stdcall Native_SetRenderLayer(char a1)
{
  int v1; // edi
  int v2; // esi
  int v3; // esi
  RenderObject *v4; // edi

  v1 = g_GameFlags1;
  if ( *(_BYTE *)(g_GameFlags1 + 5) != a1 )
  {
    *(_BYTE *)(g_GameFlags1 + 5) = a1;
    v2 = *(unsigned __int16 *)(v1 + 8);
    if ( v2 != 0xFFFF )
      HIBYTE(g_RenderObjectsStart[-v2].renderInfo) = a1;
    v3 = *(unsigned __int16 *)(v1 + 10);
    if ( v3 != 0xFFFF )
    {
      v4 = &g_RenderObjectsStart[-v3];
      HIBYTE(v4->renderInfo) = a1;
      AddObjectToDirtyRegions(v4);
    }
  }
  return 1;
}

int Native_ResetInputState()
{
  ReexecuteCurrentSpriteScript();
  return 1;
}

int __stdcall Native_RandomizeAnimSlot0(int a1)
{
  SelectRandomAnimationFrame(0, a1);
  return 1;
}

int __stdcall Native_RandomizeAnimSlot1(int a1)
{
  SelectRandomAnimationFrame(1, a1);
  return 1;
}

int Native_RemoveOverlayObject()
{
  int result; // eax
  int v1; // ebx

  result = g_GameFlags1;
  v1 = *(unsigned __int16 *)(g_GameFlags1 + 10);
  if ( v1 != 0xFFFF )
  {
    *(_WORD *)(g_GameFlags1 + 8) = -1;
    *(_WORD *)(g_GameFlags1 + 10) = -1;
    return RemoveAndFreeRenderObject(&g_RenderObjectsStart[-v1]);
  }
  return result;
}

int __stdcall Native_InitGraphicsWithFont(int fontIndex)
{
  return InitGraphicsMode(1, fontIndex);
}

int Native_InitGraphicsDefault()
{
  return InitGraphicsMode(1, 0);
}

int Native_CalcLayerFromHeight()
{
  *(_BYTE *)(g_GameFlags1 + 5) = g_ScreenHeight - *(_BYTE *)(g_GameFlags1 + 7);
  return 1;
}

int __stdcall Native_GetRenderLayer(_BYTE *a1)
{
  *a1 = *(_BYTE *)(g_GameFlags1 + 5);
  return 1;
}

int __stdcall Native_TransitionEffect(int a1)
{
  int result; // eax

  switch ( a1 )
  {
    case 3:
      result = PathfindByTileType(-1, -1, 14);
      break;
    case 4:
      result = PathfindByTileType(-1, -2, 14);
      break;
    case 5:
      result = PathfindByTileType(-2, -2, 14);
      break;
    case 6:
      result = PathfindByTileType(-1, -1, -126);
      break;
    case 7:
      result = PathfindByTileType(-1, -2, -126);
      break;
    case 8:
      result = PathfindByTileType(-2, -2, -126);
      break;
    case 9:
      result = PathfindByTileType(-1, -1, -125);
      break;
    case 10:
      result = PathfindByTileType(-1, -2, -125);
      break;
    case 11:
      result = PathfindByTileType(-2, -2, -125);
      break;
    default:
      return result;
  }
  return result;
}

int __stdcall Native_TransitionEffectEx(int a1, char a2)
{
  int result; // eax

  switch ( a1 )
  {
    case 1:
      result = PathfindByTileType(-1, a2, 0);
      break;
    case 2:
      result = PathfindByTileType(-2, a2, 0);
      break;
    case 3:
      result = PathfindByTileType(-1, a2, 14);
      break;
    case 4:
      result = PathfindByTileType(-2, a2, 14);
      break;
    case 5:
      result = PathfindByTileType(a2, a2, 14);
      break;
    case 6:
      result = PathfindByTileType(-1, a2, -126);
      break;
    case 7:
      result = PathfindByTileType(-2, a2, -126);
      break;
    case 8:
      result = PathfindByTileType(a2, a2, -126);
      break;
    case 9:
      result = PathfindByTileType(-1, a2, -125);
      break;
    case 10:
      result = PathfindByTileType(-2, a2, -125);
      break;
    case 11:
      result = PathfindByTileType(a2, a2, -125);
      break;
    default:
      return result;
  }
  return result;
}

int __stdcall Native_FadeEffect(int a1, unsigned __int8 a2)
{
  int result; // eax

  switch ( a1 )
  {
    case 1:
      result = PathfindWithBitmask(0xFFu, a2, 0);
      break;
    case 2:
      result = PathfindWithBitmask(0xFEu, a2, 0);
      break;
    case 3:
      result = PathfindWithBitmask(0xFFu, a2, 0xEu);
      break;
    case 4:
      result = PathfindWithBitmask(0xFEu, a2, 0xEu);
      break;
    case 5:
      result = PathfindWithBitmask(a2, a2, 0xEu);
      break;
    case 6:
      result = PathfindWithBitmask(0xFFu, a2, 0x82u);
      break;
    case 7:
      result = PathfindWithBitmask(0xFEu, a2, 0x82u);
      break;
    case 8:
      result = PathfindWithBitmask(a2, a2, 0x82u);
      break;
    case 9:
      result = PathfindWithBitmask(0xFFu, a2, 0x83u);
      break;
    case 10:
      result = PathfindWithBitmask(0xFEu, a2, 0x83u);
      break;
    case 11:
      result = PathfindWithBitmask(a2, a2, 0x83u);
      break;
    default:
      return result;
  }
  return result;
}

BOOL __stdcall Native_CheckInputKeyExact(int a1)
{
  return g_RenderComplete && a1 == g_PathDirection1;
}

BOOL __stdcall Native_CheckInputKey2Exact(int a1)
{
  return g_RenderComplete && a1 == g_PathDirection2;
}

BOOL __stdcall Native_CheckInputKeyMatch(int a1)
{
  return g_RenderComplete && CheckRotatedDirectionMatch(a1, g_PathDirection1);
}

BOOL __stdcall Native_CheckInputKey2Match(int a1)
{
  return g_RenderComplete && CheckRotatedDirectionMatch(a1, g_PathDirection2);
}

int __stdcall Native_CreateTileAction(int a1)
{
  __int16 v1; // bx
  int v3; // [esp+4h] [ebp-4h]

  if ( g_RenderComplete )
  {
    if ( a1 )
    {
      if ( a1 == 1 )
      {
        HIBYTE(v1) = g_CurrentDirection;
        LOBYTE(v1) = -2;
        CreateTileObject(g_PathStartY, g_PathStartX, v1);
      }
      else if ( a1 == 2 )
      {
        BYTE1(v3) = g_CurrentDirection;
        BYTE2(v3) = g_PathStartX - g_PathTargetX;
        HIBYTE(v3) = g_PathStartY - g_PathTargetY;
        MoveObjectOnTileMap(v3);
      }
    }
    else
    {
      g_RenderComplete = 0;
      g_PreviousInput = -1;
      g_CurrentInput = -1;
      HIBYTE(g_PreviousScreen) = -1;
    }
  }
  return 1;
}

int __stdcall Native_CreateTileObject(char a1)
{
  __int16 v1; // bx

  if ( g_RenderComplete )
  {
    HIBYTE(v1) = g_CurrentDirection;
    LOBYTE(v1) = a1;
    CreateTileObject(g_PathStartY, g_PathStartX, v1);
  }
  return 1;
}

int __stdcall Native_CreateTileObjectMirrored(char a1)
{
  __int16 v1; // bx

  if ( g_RenderComplete )
  {
    HIBYTE(v1) = g_CurrentDirection | 1;
    LOBYTE(v1) = a1;
    CreateTileObject(g_PathStartY, g_PathStartX, v1);
  }
  return 1;
}

BOOL __stdcall Native_TestGameFlag(char a1)
{
  return (unsigned __int8)(a1 & *(_BYTE *)g_GameFlags1) != 0;
}

char *__stdcall Native_CopyString(char *Source, char *Destination)
{
  return strcpy(Destination, Source);
}

BOOL __fastcall Native_GetSystemFlag(char a1, int a2, int a3)
{
  switch ( a3 )
  {
    case 0:
      a1 = g_ErrorFlag;
      break;
    case 1:
      a1 = g_SoundEnabledFlag;
      break;
    case 2:
      a1 = BYTE1(g_PreviousScreen);
      break;
    case 3:
      a1 = video_enabled_flag;
      break;
    default:
      return a1 != 0;
  }
  return a1 != 0;
}

int __stdcall Native_SetSystemFlag(int a1)
{
  switch ( a1 )
  {
    case 0:
    case 1:
      g_ErrorFlag = a1;
      break;
    case 2:
      g_SoundEnabledFlag = 0;
      LOBYTE(g_PreviousScreen) = 0;
      break;
    case 3:
      g_SoundEnabledFlag = 1;
      LOBYTE(g_PreviousScreen) = g_CurrentScreen;
      break;
    case 4:
      BYTE1(g_PreviousScreen) = 0;
      break;
    case 5:
      BYTE1(g_PreviousScreen) = HIBYTE(g_CurrentScreen);
      break;
    case 6:
    case 7:
      video_enabled_flag = a1 - 6;
      break;
    default:
      return 1;
  }
  return 1;
}

int Native_ClearSpecialMode()
{
  g_SpecialMode = 0;
  return 1;
}

int __stdcall Native_SetTimerCallback(ULONG_PTR dwData)
{
  return HandleHelpRequest(dwData);
}

int __stdcall Native_SetPaletteEntry(int a1, _BYTE *a2)
{
  int result; // eax

  LOBYTE(result) = *a2;
  byte_4162C8[a1] = *a2;
  return (unsigned __int8)result;
}

unsigned int __stdcall Native_GetRandom(int a1)
{
  unsigned int v1; // ecx

  v1 = (a1 * (unsigned int)HIWORD(g_RandomSeed)) >> 16;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return v1;
}

int __stdcall Native_PlayVideo(char *videoPath)
{
  char v1; // bl
  int v3; // [esp+4h] [ebp-50h] BYREF
  int v4; // [esp+8h] [ebp-4Ch] BYREF
  int v5; // [esp+Ch] [ebp-48h] BYREF
  int v6; // [esp+10h] [ebp-44h] BYREF
  char Source[64]; // [esp+14h] [ebp-40h] BYREF

  ClearInputAndShutdownAudio();
  v6 = 0;
  v5 = 0;
  v4 = 0;
  v3 = 0;
  sscanf(videoPath, "%s %d %d %d %d", Source, &v3, &v4, &v5, &v6);
  v1 = PlayIntroVideo(v6, v5, v4, v3, Source);
  RestoreScreenAndMusic();
  return v1;
}

int __stdcall Native_ExecuteCommand(LPSTR lpCommandLine)
{
  char v1; // bl

  ClearInputAndShutdownAudio();
  v1 = LaunchProcessAndWait(lpCommandLine);
  RestoreScreenAndMusic();
  return v1;
}

#pragma pack(pop)
