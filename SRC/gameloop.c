#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- gameloop module ---- */

int __stdcall ExecuteScriptWithContext(
        char direction,
        unsigned __int8 tile_y,
        unsigned __int8 tile_x,
        int object_data,
        int resource_id,
        int game_flags1,
        int game_flags2,
        BYTE *script_ptr)
{
  unsigned __int64 v8; // rax
  int saved_object_data; // ebx
  int saved_tile_x; // esi
  int saved_tile_y; // edi
  int result; // eax
  int saved_game_flags2; // [esp+Ch] [ebp-18h]
  int saved_game_flags1; // [esp+10h] [ebp-14h]
  int saved_resource_id; // [esp+14h] [ebp-10h]
  int saved_prev_tile_x; // [esp+18h] [ebp-Ch]
  int saved_prev_tile_y; // [esp+1Ch] [ebp-8h]
  char saved_direction; // [esp+20h] [ebp-4h]

  if ( script_ptr )
  {
    saved_game_flags2 = g_GameFlags2;
    saved_game_flags1 = g_GameFlags1;
    HIDWORD(v8) = g_ResourceID2;
    saved_resource_id = g_ResourceID2;
    saved_object_data = g_CurrentObjectData;
    saved_tile_x = g_CurrentTileX_Wrapped;
    saved_tile_y = g_CurrentTileY_Wrapped;
    saved_prev_tile_x = g_PreviousTileX;
    saved_prev_tile_y = g_PreviousTileY;
    saved_direction = g_CurrentDirection;
    g_GameFlags2 = game_flags2;
    g_GameFlags1 = game_flags1;
    g_ResourceID2 = resource_id;
    g_CurrentObjectData = object_data;
    g_PreviousTileX = tile_x;
    g_CurrentTileX_Wrapped = tile_x;
    LODWORD(v8) = tile_y;
    g_PreviousTileY = tile_y;
    g_CurrentTileY_Wrapped = tile_y;
    LOBYTE(v8) = direction;
    g_CurrentDirection = direction;
    EvaluateScriptBytecode(v8, script_ptr);
    g_CurrentDirection = saved_direction;
    g_PreviousTileY = saved_prev_tile_y;
    g_PreviousTileX = saved_prev_tile_x;
    g_CurrentTileY_Wrapped = saved_tile_y;
    g_CurrentTileX_Wrapped = saved_tile_x;
    g_CurrentObjectData = saved_object_data;
    g_ResourceID2 = saved_resource_id;
    g_GameFlags1 = saved_game_flags1;
    g_GameFlags2 = saved_game_flags2;
    return saved_game_flags2;
  }
  return result;
}

char __stdcall InitGraphicsMode(int updateDirtyRegions, int fontIndex)
{
  DBG_ENTER("InitGraphicsMode");
  FontSlot *v2; // edx
  void *fontData; // eax

  if ( !g_DisableCursor )
  {
    if ( updateDirtyRegions && *(_DWORD *)&g_CursorObject.spriteDataPtr )
      AddObjectToDirtyRegions(&g_CursorObject);
    g_CurrentFontIndex = fontIndex;
    v2 = &g_FontSlotArray[fontIndex];
    g_CursorObject.flags = -63;
    LOBYTE(g_CursorObject.renderInfo) = v2->attributes;
    g_CursorObject.slotIndex = 0;
    g_CursorObject.posY = 0;
    g_CursorObject.posX = 0;
    if ( v2->type == 1 )
      fontData = v2->fontData;
    else
      fontData = *(void **)v2->fontData;
    *(_DWORD *)&g_CursorObject.spriteDataPtr = fontData;
  }
  return 1;
}

BOOL __stdcall InitGameState(int gameOpcode)
{
  DBG_ENTER("InitGameState");
  g_RandomSeed = 1103515245 * time(0) + 12345;
  g_ResourceID1 = 0xFFFF;
  g_ResourceID2 = 0xFFFF;
  g_GameFlags1 = 0;
  g_GameFlags2 = 0;
  g_SavedState = 0;
  g_CustomDataPtr3 = 0;
  g_CustomDataPtr2 = 0;
  g_CustomDataPtr1 = 0;
  *((_BYTE*)&video_recursion_counter + 1) = 0; *((_BYTE*)&video_recursion_counter + 2) = 0;
  g_GameStateFlag = 0;
  g_FirstRun = 1;
  g_CurrentScreen = 0;
  LOWORD(g_PreviousScreen) = 0;
  g_InputState[1] = 0;
  return GameMainLoop(gameOpcode, 0);
}

void GameState_Reset()
{
  DBG_ENTER("GameState_Reset");
  g_GameStateFlag = 0;
}

int InitRenderingEngine()
{
  DBG_ENTER("InitRenderingEngine");
  int result; // eax

  result = 1103515245 * time(0);
  g_RandomSeed = result + 12345;
  g_LastCheckedRenderObject = 0;
  g_DirtyRegionsList = 0;
  return result;
}

__declspec(noreturn) void LongjmpRestart()
{
  DBG_ENTER("LongjmpRestart");
  longjmp(g_JumpBuffer, 1);
}

int __stdcall ProcessAbsoluteTileCoords(int a1)
{
  g_TileProcessor(&a1);
  CreateTileObject(SHIBYTE(a1), SBYTE2(a1), a1);
  return 0;
}

BOOL __stdcall ProcessRelativeTileCoords(int a1)
{
  g_TileProcessor(&a1);
  CreateTileObject(
    (g_ScreenHeight + g_CurrentTileY_Wrapped + SHIBYTE(a1)) % g_ScreenHeight,
    (g_ScreenWidth + g_CurrentTileX_Wrapped + SBYTE2(a1)) % g_ScreenWidth,
    a1);
  return HIWORD(a1) == 0;
}

BOOL __stdcall InitializeGame(int gameOpcode, char *dataFilePath)
{
  DBG_ENTER("InitializeGame");
  char *copiedPath; // eax
  BOOL result; // eax

  copiedPath = strcpy(g_DataFilePath, dataFilePath);
  result = 0;
  if ( LoadDataFile(copiedPath) )
  {
    if ( InitGameState(gameOpcode) )
    {
      if ( InitDisplay() )
      {
        if ( SetFrameRate(*(int *)((char *)&g_GameLoopDataPtr->flags_or_id + 1)) )
        {
          if ( AlwaysTrue() )
          {
            if ( InitGraphics((_BYTE *)&g_CurrentScreen + 1, &g_CurrentScreen, 0, g_GraphicsConfigPtr) )
            {
              LOWORD(g_PreviousScreen) = g_CurrentScreen;
              if ( CheckGameState() )
                return 1;
            }
          }
        }
      }
    }
  }
  return result;
}

BOOL __stdcall ProcessGameFrame(
        char inputState,
        unsigned __int8 inputCode,
        char inputFlag,
        unsigned __int8 eventCode,
        int coordY,
        int coordX,
        int altCoordY,
        int altCoordX,
        int mouseY,
        int mouseX)
{
  if ( _setjmp3(g_JumpBuffer, 0) || !g_ErrorFlag )
  {
    eventCode = 14;
    inputFlag = 14;
    inputCode = 14;
  }
  g_MouseX = mouseX;
  g_MouseY = mouseY;
  LOBYTE(video_recursion_counter) = inputCode;
  g_InputState[1] = inputState;
  ProcessInputAndRender(inputFlag, eventCode, coordY, coordX, altCoordY, altCoordX);
  UpdateScreenSections();
  if ( !UpdateFrameRate() )
    return 0;
  if ( BYTE2(video_recursion_counter) )
  {
    if ( ProcessSpecialEvent(eventCode) )
      goto LABEL_10;
    return 0;
  }
  if ( !ProcessNormalEvent() )
    return 0;
LABEL_10:
  while ( g_SpecialMode )
  {
    LOBYTE(video_recursion_counter) = 14;
    if ( !UpdateFrameRate() )
      return 0;
    if ( BYTE2(video_recursion_counter) )
    {
      if ( !ProcessSpecialEvent(eventCode) )
        return 0;
    }
    else if ( !ProcessNormalEvent() )
    {
      return 0;
    }
  }
  return HandleGameEvent(altCoordY, altCoordX) && RenderDirtyRegions();
}

char RestoreGraphicsAfterVideo()
{
  BYTE *v1; // [esp-4h] [ebp-4h]

  v1 = (BYTE *)(g_SnapshotBasePtr + *(_DWORD *)(g_SnapshotBasePtr + 16) + 24);
  g_InputState[0] = g_DisplayMode[0];
  SetPalette(1, g_DisplayMode[0], 256, v1);
  AddDirtyRegion(dword_416174, dword_416170, 0, 0);
  return 1;
}

char __stdcall UpdateCursorPosition(unsigned __int16 a1, unsigned __int16 a2)
{
  char result; // al

  if ( !g_DisableCursor && !g_SpecialMode && !*((_BYTE*)&video_recursion_counter + 1) && g_CursorObject.spriteDataPtr )
  {
    if ( g_GameStateFlag )
    {
      g_InputState[1] = 1;
      AddObjectToDirtyRegions(&g_CursorObject);
      g_CursorObject.posX = a2;
      g_CursorObject.posY = a1;
      AddObjectToDirtyRegions(&g_CursorObject);
      return RenderDirtyRegions();
    }
  }
  return result;
}

int CleanupGameState()
{
  DBG_ENTER("CleanupGameState");
  int v0; // eax
  int v1; // eax

  ClearInputBuffer();
  v0 = StopMidiMusic();
  video_enabled_flag = 1;
  byte_4162B5 = 1;
  g_SoundEnabledFlag = 1;
  g_ErrorFlag = 1;
  g_FirstRun = 1;
  LOBYTE(v0) = g_CurrentScreen;
  BYTE1(g_GraphicsConfigPtr) = g_CurrentScreen;
  v1 = SetSoundVolume(v0);
  LOBYTE(v1) = HIBYTE(g_CurrentScreen);
  BYTE2(g_GraphicsConfigPtr) = HIBYTE(g_CurrentScreen);
  SetMusicVolume(v1);
  ValidateGameData(dword_4162F4);
  ShutdownMultimedia();
  nullsub_4();
  nullsub_5();
  GameState_Reset();
  return CloseFile_Wrapper();
}

int Error_InsufficientMemory()
{
  DBG_ENTER("Error_InsufficientMemory");
  SetErrorMessage(aInsufficientMe);
  return 0;
}

void ClearRenderAndMemoryPoolState()
{
  g_RenderObjectsEnd = 0;
  g_RenderObjectsStart = 0;
  g_MemoryPoolCurrent = 0;
  g_MemoryPoolStart = 0;
  g_MemoryPoolAvailable = 0;
}

char __stdcall GameOpcodeDispatcher(
        int opcodeData,
        int dataOffset,
        int arrayIdx,
        int subIdx,
        int slotIdx,
        int opcodeType)
{
  int v6; // eax
  ScreenSnapshot *snapshot; // eax
  int tileMapPtr; // eax
  int tileCount; // ebx
  _WORD *tilePtr; // ecx
  _DWORD *subObjData; // ebx
  int scriptOffset; // esi
  FontSlot *fontSlot; // edx
  FontSlot *fontSlot2; // eax
  _DWORD *glyphArray; // ebx
  int groupEntry; // ecx

  v6 = opcodeType;
  if ( opcodeType == OPCODE_SET_FRAMERATE )
  {
    g_GameLoopDataPtr = (GameLoopData *)opcodeData;// OPCODE_SET_FRAMERATE: Store game loop data ptr and apply default frame rate
    LOBYTE(v6) = g_DefaultFrameRate;
    *(_BYTE *)(opcodeData + 1) = g_DefaultFrameRate;
    SetFrameRate(v6);
  }
  else if ( opcodeType != OPCODE_UPDATE_FRAME && opcodeType != OPCODE_UNKNOWN_19 )
  {
    switch ( opcodeType )
    {
      case OPCODE_INIT_SCREEN_SNAPSHOT:
        snapshot = &g_ScreenSnapshotArray[slotIdx];// OPCODE_INIT_SCREEN_SNAPSHOT: Initialize snapshot slot with current scene pointers
        snapshot->compressedData = (void *)g_SnapshotBasePtr;
        snapshot->sceneDataPtr = (void *)g_CurrentScenePtr;
        snapshot->decompressedData = 0;
        snapshot->objectCount = 0;
        if ( g_ActiveSnapshotIndex == -1 && *(int *)opcodeData < 0 )// If no snapshot active yet and opcodeData flags are negative (default flag), set this slot as active
          g_ActiveSnapshotIndex = slotIdx;
        break;
      case OPCODE_CAPTURE_SNAPSHOT:
        if ( !g_DemoMode )
        {
          tileMapPtr = g_TileMapBuffer;
          tileCount = g_ScreenHeight << g_TileMapShift;
          do
          {
            tilePtr = (_WORD *)tileMapPtr;      // Fill entire tilemap with 0xF0FE (empty marker + layer mask)
            tileMapPtr += 2;
            *tilePtr = -3842;
            --tileCount;
          }
          while ( tileCount );
          g_RenderingInProgress = 1;
          RenderSprite(1, (int *)opcodeData);
          g_RenderingInProgress = 0;
          CaptureScreenSnapshot(slotIdx);
        }
        break;                                  // OPCODE_CAPTURE_SNAPSHOT: Clear tilemap to 0xF0FE, render sprite in absolute mode, then capture
      case OPCODE_INIT_SPRITE_SLOT:
        InitSlotFromTemplate(&g_SpriteSlotArray[slotIdx]);// OPCODE_INIT_SPRITE_SLOT: Copy template data into sprite slot
        break;
      case OPCODE_SET_SPRITE_DATA_PTR:
        g_SpriteSlotArray[slotIdx].dataPtr = (void *)(dataOffset + opcodeData);// OPCODE_SET_SPRITE_DATA_PTR: Set sprite data pointer (base + offset)
        break;
      case OPCODE_SET_SPRITE_SCRIPT_PTR:
        g_SpriteSlotArray[slotIdx].scriptPtr = (void *)(dataOffset + opcodeData);// OPCODE_SET_SPRITE_SCRIPT_PTR: Set sprite script pointer (base + offset)
        break;
      case OPCODE_SET_SPRITE_SUBOBJECT_ARRAY:
        g_SpriteSlotArray[slotIdx].subObjectArray = (void **)opcodeData;// OPCODE_SET_SPRITE_SUBOBJECT_ARRAY: Set pointer to sub-object pointer array
        break;
      case OPCODE_SET_SUBOBJECT_PTR:
        g_SpriteSlotArray[slotIdx].subObjectArray[subIdx] = (void *)opcodeData;// OPCODE_SET_SUBOBJECT_PTR: Set individual sub-object pointer
        break;
      case OPCODE_SET_SUBOBJECT_DATA_PTR:
        *((_DWORD *)g_SpriteSlotArray[slotIdx].subObjectArray[subIdx] + 1) = dataOffset + opcodeData;// OPCODE_SET_SUBOBJECT_DATA_PTR: Set data ptr inside sub-object (field at offset +4)
        break;
      case OPCODE_SET_SUBOBJECT_SCRIPT_PTR:
        subObjData = g_SpriteSlotArray[slotIdx].subObjectArray[subIdx];// OPCODE_SET_SUBOBJECT_SCRIPT_PTR: Walk past sprite flags and pattern groups to find script ptr slot, then set it
        scriptOffset = 1;                       // scriptOffset starts at 1 (after header dword)
        if ( (*subObjData & 1) != 0 )           // If bit0 (precondition): skip 1 extra dword (precondition script ptr)
          scriptOffset = 2;
        if ( (*(_BYTE *)subObjData & 2) != 0 )  // If bit1 (pattern matching): walk past all pattern groups to find script field
        {
          do
          {
            groupEntry = subObjData[scriptOffset];// Walk group: read header, advance by lo16(entry count)+1, loop until bit16 (last group) set
            scriptOffset += (unsigned __int16)groupEntry + 1;
          }
          while ( (groupEntry & 0x10000) == 0 );
        }
        subObjData[scriptOffset] = dataOffset + opcodeData;// Finally set the script pointer at the computed offset
        break;
      case OPCODE_SET_ANIMATION_FRAME_BITMASK:
        g_AnimationSlotArray[slotIdx].frameBitmask = (void *)opcodeData;// OPCODE_SET_ANIMATION_FRAME_BITMASK: Set bitmask for animation frame matching
        break;
      case OPCODE_SET_ANIMATION_FRAME_SELECTION:
        g_AnimationSlotArray[slotIdx].frameSelectionData = (void *)opcodeData;// OPCODE_SET_ANIMATION_FRAME_SELECTION: Set frame selection lookup table
        break;
      case OPCODE_SET_ANIMATION_FRAME_MODIFICATION:
        g_AnimationSlotArray[slotIdx].frameModificationData = (void *)opcodeData;// OPCODE_SET_ANIMATION_FRAME_MODIFICATION: Set frame modification data for tile match
        break;
      case OPCODE_INIT_FONT_SLOT:
        InitSlotFromTemplate(&g_FontSlotArray[slotIdx]);// OPCODE_INIT_FONT_SLOT: Initialize font from template + extract render flags
        g_FontRenderFlags = *((_BYTE *)g_TemplateDataBuffer + 1) & 0x80;
        DBG_LOG("OPCODE_INIT_FONT_SLOT: slot=%d fontData=%p flags=%d type=%d", slotIdx, g_FontSlotArray[slotIdx].fontData, g_FontSlotArray[slotIdx].flags, g_FontSlotArray[slotIdx].type);
        break;
      case OPCODE_SET_FONT_DATA:
        g_FontSlotArray[slotIdx].fontData = (void *)opcodeData;// OPCODE_SET_FONT_DATA: Set font data pointer directly
        DBG_LOG("OPCODE_SET_FONT_DATA: slot=%d fontData=%p", slotIdx, g_FontSlotArray[slotIdx].fontData);
        break;
      case OPCODE_SET_FONT_DATA_OR_ARRAY:
        fontSlot = &g_FontSlotArray[slotIdx];   // OPCODE_SET_FONT_DATA_OR_ARRAY: Set font data or sub-font array entry depending on font type
        DBG_LOG("OPCODE_SET_FONT_DATA_OR_ARRAY: slot=%d opcodeData=%p fontSlot=%p fontData_before=%p flags=%d type=%d", slotIdx, (void*)opcodeData, fontSlot, fontSlot->fontData, fontSlot->flags, fontSlot->type);
        if ( fontSlot->flags || fontSlot->type <= 1u )
          fontSlot->fontData = (void *)opcodeData;
        else
          *((_DWORD *)fontSlot->fontData + subIdx) = opcodeData;
        DBG_LOG("OPCODE_SET_FONT_DATA_OR_ARRAY: fontData_after=%p", fontSlot->fontData);
        break;
      case OPCODE_SET_FONT_GLYPH_DATA:
        fontSlot2 = &g_FontSlotArray[slotIdx];  // OPCODE_SET_FONT_GLYPH_DATA: Set glyph sprite data for specific character index
        glyphArray = fontSlot2->fontData;
        DBG_LOG("OPCODE_SET_FONT_GLYPH_DATA: slot=%d fontSlot=%p fontData=%p arrayIdx=%d opcodeData=%p g_TemplateDataBuffer=%p", slotIdx, fontSlot2, fontSlot2->fontData, arrayIdx, (void*)opcodeData, (void*)(int)g_TemplateDataBuffer);
        if ( !fontSlot2->flags && fontSlot2->type > 1u )// If font type > 1 and no flags: fontData is array of sub-font pointers, index by subIdx
          glyphArray = (_DWORD *)glyphArray[subIdx];
        glyphArray[2 * arrayIdx] = opcodeData;
        break;
      case OPCODE_INIT_SOUND_SLOT:
        InitSlotFromTemplate(&g_SoundSlotArray[slotIdx]);// OPCODE_INIT_SOUND_SLOT: Initialize sound slot from template
        break;
      case OPCODE_SET_SOUND_DATA:
        g_SoundSlotArray[slotIdx].soundDataPtr = (void *)opcodeData;// OPCODE_SET_SOUND_DATA: Set sound data pointer
        break;
      case OPCODE_SET_MUSIC_TRACK_DATA:
        *((_DWORD *)&g_MusicTrackArray->trackData + slotIdx) = opcodeData;// OPCODE_SET_MUSIC_TRACK_DATA: Set music track data for track slot
        break;
      case OPCODE_SET_OVERLAY_SPRITE_DATA:
        g_OverlaySlotArray[slotIdx].spriteDataPtr = (int *)opcodeData;// OPCODE_SET_OVERLAY_SPRITE_DATA: Set overlay slot's sprite data pointer
        break;
      case OPCODE_SET_OVERLAY_COORDS:
        g_OverlaySlotArray[slotIdx].coords = (OverlayCoords *)opcodeData;// OPCODE_SET_OVERLAY_COORDS: Set overlay slot's coordinate data
        break;
      case OPCODE_SET_GLOBAL_PTR_1:
        g_CustomDataPtr1 = (int *)opcodeData;// OPCODE_SET_GLOBAL_PTR_1: Store custom global data pointer 1
        break;
      case OPCODE_SET_GLOBAL_PTR_2:
        g_CustomDataPtr2 = (_DWORD *)opcodeData;// OPCODE_SET_GLOBAL_PTR_2: Store custom global data pointer 2
        break;
      case OPCODE_SET_GLOBAL_PTR_3:
        g_CustomDataPtr3 = (int *)opcodeData;// OPCODE_SET_GLOBAL_PTR_3: Store custom global data pointer 3
        break;
    }
  }
  return 1;
}

BOOL __stdcall GameMainLoop(int opcodeType, int startResourceId)
{
  DBG_ENTER("GameMainLoop");
  unsigned int currentResourceId; // edi
  int snapIdx; // eax
  char loopResult; // al
  char *pDataBuffer; // ecx
  size_t actualDataSize; // ebx
  size_t requiredDataSize; // ecx
  int opcodeParam; // [esp+10h] [ebp-4Ch] BYREF
  int arg1; // [esp+14h] [ebp-48h] BYREF
  int arg2; // [esp+18h] [ebp-44h] BYREF
  int arg3; // [esp+1Ch] [ebp-40h] BYREF
  size_t jumpTarget; // [esp+20h] [ebp-3Ch] BYREF
  #pragma pack(push, 1)
  struct { char buf[52]; unsigned int id; } _stkLayout;
  #pragma pack(pop)
  char *dataBuffer = _stkLayout.buf;
  /* nextResourceId is now _stkLayout.id — accessed via macro below */
  unsigned int *_pNextResId = &_stkLayout.id;

  currentResourceId = 1;
  InitFileReadBuffer(0);                        // Initialize file reading buffer
  if ( !g_FirstRun && !ValidateGameData(dword_4162F4) || !LoadResourceById(1u) )// Validate game data and load first resource
    return 0;
  byte_411698 = 1;
  g_CurrentSnapshotIndex = -1;
  g_ActiveSnapshotIndex = -1;
  g_TotalSnapshotCount = 0;
  dword_4162D4 = startResourceId;
  g_CustomDataPtr3 = 0;
  g_CustomDataPtr2 = 0;
  g_CustomDataPtr1 = 0;
  g_MaxDeferredLoadSize = 0;
  g_CachedSpriteOffset = 0;
  g_SpriteDataBuffer = 0;
  ResetGameState();
  ClearInputBuffer();
  _stkLayout.id = startResourceId + 2;
  while ( 1 )
  {                                             // Main opcode processing loop
    while ( g_CurrentOpcode )
    {                                           // Switch on current opcode type
      switch ( g_CurrentOpcode )
      {
        case 1:
          DecodeVLQ((char *)&arg1);             // Opcode 1: Decode arg1
          break;
        case 2:
          DecodeVLQ((char *)&arg2);             // Opcode 2: Decode arg2
          break;
        case 3:
          DecodeVLQ((char *)&arg3);             // Opcode 3: Decode arg3
          break;
        case 5:
          ReadJumpOffset(5, &jumpTarget);       // Opcode 5: Read jump offset
          if ( opcodeType == 20 )
            *(_DWORD *)(dword_416338 + 4 * opcodeParam) = jumpTarget;
          break;
        default:
          if ( (unsigned __int8)g_CurrentOpcode == 255 )
          {                                     // Opcode 255: Execute game command
            if ( !GameOpcodeDispatcher(opcodeDataPtr, arg3, arg2, arg1, opcodeParam, opcodeType) || !ReadByte() )
              return 0;
          }
          else if ( g_CurrentOpcode == 4 )
          {                                     // Opcode 4: Process based on opcodeType
            switch ( opcodeType )
            {
              case 15:
                if ( !g_FirstRun || g_DemoMode )// Type 15: Read text/data chunk
                  pDataBuffer = dataBuffer;
                else
                  pDataBuffer = (char *)&unk_4162A0;
                DBG_LOG("case15: BEFORE pDataBuffer=%p dataBuffer=%p _stkLayout.id=%u(0x%08X)", pDataBuffer, dataBuffer, _stkLayout.id, _stkLayout.id);
                ReadCompressedData(pDataBuffer);
                DBG_LOG("case15: AFTER g_TemplateDataSize=%u g_DecompressedDataSize=%u _stkLayout.id=%u(0x%08X)", (unsigned)g_TemplateDataSize, (unsigned)g_DecompressedDataSize, _stkLayout.id, _stkLayout.id);
                continue;
              case 16:
                if ( !ProcessSpriteData() )     // Type 16: Process sprite animation
                  return 0;
                continue;
              case 17:
                if ( opcodeParam != startResourceId )
                  goto LABEL_47;
                InitializeFrameBuffers();       // Type 17: Update game frame
                continue;
              case 24:
                if ( byte_411698 )              // Type 24: Initialize game subsystems
                {
                  byte_411698 = 0;
                  if ( g_MaxDeferredLoadSize )
                  {
                    g_SpriteDataBuffer = g_TemplateDataBuffer;
                    AllocMemoryChunk(g_MaxDeferredLoadSize);
                  }
                  InitRenderingEngine();
                  SetupMemoryContext();
                  g_TemplateDataBuffer = (void *)AllocOpcodeBuffer();
                }
                g_CurrentScenePtr = GetCurrentFileOffset();
                ++g_TotalSnapshotCount;
                goto LABEL_47;
              default:
LABEL_47:
                if (opcodeType >= 0x40 && opcodeType <= 0x43) DBG_LOG("LABEL_47: opcodeType=0x%X param=%d g_TemplateDataBuffer=%p g_FontSlotArray=%p", opcodeType, opcodeParam, (void*)(int)g_TemplateDataBuffer, g_FontSlotArray);
                opcodeDataPtr = (int)g_TemplateDataBuffer;
                ReadCompressedData((char *)g_TemplateDataBuffer);
                if ( !GameOpcodeDispatcher((int)g_TemplateDataBuffer, arg3, arg2, arg1, opcodeParam, opcodeType) )
                  return 0;
                if ( opcodeType == OPCODE_SET_FONT_GLYPH_DATA )// Type 67: Handle memory allocation for opcodes
                {
                  if ( g_FontRenderFlags )
                  {
                    actualDataSize = g_TemplateDataSize - 4;
                    requiredDataSize = g_TemplateDataSize - 4;
                    if ( g_DecompressedDataSize )
                    {
                      actualDataSize = g_DecompressedDataSize;
                      requiredDataSize = g_TemplateDataSize;
                    }
                    if ( requiredDataSize > g_MaxDeferredLoadSize )
                      g_MaxDeferredLoadSize = requiredDataSize;
                    *((_DWORD *)g_TemplateDataBuffer + 1) = 'Disk';// Special case: Create DeferredLoadInfo structure for lazy font glyph loading. g_TemplateDataBuffer is cast to DeferredLoadInfo*. Structure: {firstDword, magic='Disk' (0x4469736B), fileOffset, decompressedSize}
                    *((_DWORD *)g_TemplateDataBuffer + 2) = GetCurrentFileOffset() - actualDataSize - 1;
                    *((_DWORD *)g_TemplateDataBuffer + 3) = g_DecompressedDataSize;
                    g_TemplateDataSize = 16;
                  }
LABEL_63:
                  g_TemplateDataBuffer = (void *)AllocMemoryChunk(g_TemplateDataSize);
                  continue;
                }
                if ( opcodeType != OPCODE_UPDATE_FRAME
                  && opcodeType != OPCODE_INIT_SPRITE_SLOT
                  && opcodeType != OPCODE_INIT_FONT_SLOT
                  && opcodeType != OPCODE_INIT_SOUND_SLOT
                  && opcodeType != OPCODE_INIT_SCREEN_SNAPSHOT
                  && opcodeType != OPCODE_CAPTURE_SNAPSHOT )
                {
                  goto LABEL_63;
                }
                break;
            }
          }
          else
          {
            arg3 = 0;
            arg2 = 0;
            arg1 = 0;
            opcodeParam = 0;
            opcodeType = g_CurrentOpcode & 0x7F;
            if ( (signed char)g_CurrentOpcode < 0 ) {
              DBG_LOG("GameMainLoop_ELSE: opcode=0x%02X opcodeType=%d (no param)", (unsigned char)g_CurrentOpcode, opcodeType);
              ReadByte();
            } else {
              DecodeVLQ((char *)&opcodeParam);
              DBG_LOG("GameMainLoop_ELSE: opcode=0x%02X opcodeType=0x%X opcodeParam=%d", (unsigned char)g_CurrentOpcode, opcodeType, opcodeParam);
            }
          }
          break;
      }
    }
    if ( currentResourceId != 1 )
      break;
    currentResourceId = _stkLayout.id;
    if ( _stkLayout.id > g_MaxResourceIndex ) {
      DBG_LOG("GameMainLoop: nextResourceId %u > max %u, skipping to cleanup",
        _stkLayout.id, g_MaxResourceIndex);
      break;
    }
    if ( !LoadResourceById(_stkLayout.id) )
      return 0;
  }
  CleanupGameLoop();
  if ( g_DemoMode )
    return 1;
  if ( !InitGraphicsMode(0, 0) || !SaveGameState(dword_4162F4) )
    return 0;
  snapIdx = g_ActiveSnapshotIndex;
  if ( g_ActiveSnapshotIndex == -1 )
    snapIdx = 0;
  return ExecuteGameLoop(0, snapIdx) != 0;
}

char __stdcall ExecuteGameLoop(char skipCapture, int snapIdx)
{
  DBG_LOG("ExecuteGameLoop: skipCapture=%d snapIdx=%d g_ScreenSnapshotArray=%p g_CurrentSnapshotIndex=%d", (int)skipCapture, snapIdx, g_ScreenSnapshotArray, g_CurrentSnapshotIndex);                                               // If there's an active snapshot and not skipping capture, save current state first
  int row; // ebx
  int srcOffset; // esi
  RenderObject *objDataPtr; // ebp
  int objectCount; // eax
  int remainingObjects; // ebx
  RenderObject *srcObjPtr; // esi
  RenderObject *destObjPtr; // eax
  ScreenSnapshot *snapshot; // [esp+10h] [ebp-4h]

  if ( g_CurrentSnapshotIndex != -1 && !skipCapture )
    CaptureScreenSnapshot(g_CurrentSnapshotIndex);
  g_CurrentSnapshotIndex = snapIdx;       // Update current snapshot index
  snapshot = &g_ScreenSnapshotArray[snapIdx];// Get pointer to snapshot structure
  DBG_LOG("ExecuteGameLoop: snapshot=%p sceneDataPtr=%p compressedData=%p decompressedData=%p objCount=%d",
    snapshot, snapshot->sceneDataPtr, snapshot->compressedData, snapshot->decompressedData, snapshot->objectCount);
  if ( SeekFileWithBuffer(0, (int)snapshot->sceneDataPtr) && ReadCompressedData((char *)snapshot->compressedData) )// Seek to snapshot file position and decompress data
  {
    row = 0;                                    // PHASE 1: RESTORE SCREEN - Initialize row counter
    srcOffset = 0;
    do
    {
      memcpy(
        (void *)(g_TileMapBuffer + 2 * (row << g_TileMapShift)),
        (char *)snapshot->decompressedData + g_ScreenWidth * srcOffset,
        2 * g_ScreenWidth);                     // Copy each row from snapshot buffer to tile map (2 bytes per pixel)
      srcOffset += 2;
      ++row;
    }
    while ( row < g_ScreenHeight );
    objDataPtr = (RenderObject *)((char *)snapshot->decompressedData + 2 * g_ScreenPixelCount);// PHASE 2: RESTORE OBJECTS - Position pointer after screen data to start of objects
    objectCount = snapshot->objectCount;        // Get number of objects to restore
    remainingObjects = objectCount - 1;
    if ( objectCount )
    {
      do
      {
        srcObjPtr = objDataPtr++;               // Point to source object in snapshot (16 bytes)
        destObjPtr = AllocRenderObject();       // Allocate new render object slot
        *(_DWORD *)&destObjPtr->flags = *(_DWORD *)&srcObjPtr->flags;// Copy object data: 4 DWORDs (16 bytes) from snapshot to allocated slot
        srcObjPtr = (RenderObject *)((char *)srcObjPtr + 4);
        *(_DWORD *)&destObjPtr->animLink = *(_DWORD *)&srcObjPtr->flags;
        srcObjPtr = (RenderObject *)((char *)srcObjPtr + 4);
        *(_DWORD *)&destObjPtr->posX = *(_DWORD *)&srcObjPtr->flags;
        *(_DWORD *)&destObjPtr->spriteDataPtr = *(_DWORD *)&srcObjPtr->animLink;
      }
      while ( remainingObjects-- );
    }
    snapshot->objectCount = 0;                  // Clear object count after restoration
    FreeMemoryBlock((int)snapshot->decompressedData);// PHASE 3: CLEANUP - Free snapshot buffer
    ClearDirtyRegionsAndInit(0);                // Clear dirty regions and reinitialize game state
    if ( !skipCapture )                         // If not skipping capture, restore graphics (after video playback)
      RestoreGraphicsAfterVideo();
  }
}

#pragma pack(pop)
