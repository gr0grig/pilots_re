#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- render module ---- */

int __stdcall RemoveAndFreeRenderObject(RenderObject *render_object)
{
  if ( (render_object->flags & 0x80u) != 0 )
    AddObjectToDirtyRegions((int)render_object);
  return FreeRenderObject(render_object);
}

int __stdcall RemoveChildTextObjects(int parent_object_addr)
{
  int result; // eax
  int parent_index; // esi
  RenderObject *current_object; // ebx

  if ( (*(_BYTE *)(parent_object_addr + 3) & 2) != 0 )
  {
    *(_BYTE *)(parent_object_addr + 3) &= ~2u;
    result = ((int)g_RenderObjectsStart - parent_object_addr) / 16;
    parent_index = result;
    for ( current_object = g_RenderObjectsStart; current_object > g_RenderObjectsEnd; --current_object )
    {
      if ( (current_object->flags & 0xE3) == 0xE1 && current_object->parentIndex == parent_index )
        result = RemoveAndFreeRenderObject(current_object);
    }
  }
  return result;
}

int __stdcall RemoveRenderObjectByIndex(int a1)
{
  int result; // eax

  result = a1;
  if ( a1 != 0xFFFF )
    return RemoveAndFreeRenderObject(&g_RenderObjectsStart[-a1]);
  return result;
}

void __stdcall UpdateRenderObjectCache(RenderObject *object)
{
  RenderObject *v1; // ebx
  RenderObject *v2; // esi
  int v3; // edi

  if ( object != g_LastCheckedRenderObject )
    return;
  v1 = 0;
  v2 = g_RenderObjectsStart;
  v3 = 0;
  if ( g_RenderObjectsStart <= g_RenderObjectsEnd )
    goto LABEL_9;
  while ( 1 )
  {
    if ( v2 < object )
      ++v3;
    if ( (v2->flags & 3) != 3 || LOBYTE(g_SpriteSlotArray[v2->slotIndex].baseData) != 3 )
      goto LABEL_13;
    if ( v3 )
      break;
    if ( !v1 )
      v1 = v2;
LABEL_13:
    if ( --v2 <= g_RenderObjectsEnd )
      goto LABEL_9;
  }
  v1 = v2;
LABEL_9:
  g_LastCheckedRenderObject = v1;
}

int ReexecuteCurrentSpriteScript()
{
  int gameFlags; // eax
  int spriteIndex; // edi
  SpriteSlot *spriteSlot; // esi
  RenderObject *renderObj; // ebx

  gameFlags = g_GameFlags1;
  spriteIndex = *(unsigned __int8 *)(g_GameFlags1 + 2);
  if ( spriteIndex != 254 )                     // 0xFE = no active sprite/screen transition, skip
  {
    spriteSlot = &g_SpriteSlotArray[spriteIndex];
    renderObj = g_RenderObjectsStart;
    while ( (renderObj->flags & 0xC1) != 0x81
         || renderObj->parentIndex != 0xFFFF
         || renderObj->animLink != *(unsigned __int16 *)(g_GameFlags1 + 6) )// Search for render object: flags==0x81 (active+type1), no parent, matching animation slot from g_GameFlags1+6
    {
      if ( --renderObj <= g_RenderObjectsEnd )
        goto LABEL_7;
    }
    RemoveAndFreeRenderObject(renderObj);       // Remove the old render object before re-executing script
LABEL_7:
    ExecuteScriptWithContext(
      *(_DWORD *)(g_GameFlags1 + 3),
      *(_DWORD *)(g_GameFlags1 + 7),
      *(_DWORD *)(g_GameFlags1 + 6),
      0,
      0xFFFF,
      0,
      (int)spriteSlot,
      (BYTE *)spriteSlot->scriptPtr);
    gameFlags = g_GameFlags1;
    *(_WORD *)(g_GameFlags1 + 2) = -3842;       // Mark as processed: screenIndex=0xFE (inactive), upper byte=0xF0
  }
  return gameFlags;
}

int __stdcall RemoveAndCreateTileObject(char removeStandalone, int tileY, int tileX)
{
  int tileMapIndex; // eax
  unsigned __int8 *tileEntryPtr; // ebp
  RenderObject *renderObj; // ebx
  RenderObject *compositeObj; // esi
  unsigned __int16 renderInfo; // ax
  char removalDone; // [esp+10h] [ebp-Ch]
  SpriteSlot *spriteSlot; // [esp+14h] [ebp-8h]
  __int16 packedTileCoords; // [esp+18h] [ebp-4h]

  tileMapIndex = tileX + (tileY << g_TileMapShift);// tileMapIndex = (tileY << g_TileMapShift) + tileX
  tileEntryPtr = (unsigned __int8 *)(g_TileMapBuffer + 2 * tileMapIndex);// tileEntryPtr = &g_TileMapBuffer[tileMapIndex * 2] — each entry is 2 bytes: [spriteIndex, direction+flags]
  if ( *tileEntryPtr == 0xFE )
    return tileMapIndex;                        // 0xFE = empty tile, nothing to do
  spriteSlot = &g_SpriteSlotArray[*tileEntryPtr];
  renderObj = g_RenderObjectsStart;
  compositeObj = 0;
  removalDone = 0;
  LOBYTE(packedTileCoords) = tileX;             // Pack tile coords: lo=tileX, hi=tileY for matching against parentIndex/animLink
  HIBYTE(packedTileCoords) = tileY;
  while ( 1 )
  {                                             // Check if render object is active (bit0=1)
    if ( (renderObj->flags & 1) != 0 )
    {                                           // bit1=0: standalone object; bit1=1: composite (has children)
      if ( (renderObj->flags & 2) == 0 )
      {
        if ( !removeStandalone
          || renderObj->animLink != packedTileCoords
          || renderObj->parentIndex != 0xFFFF
          || (renderObj->flags & 0x40) != 0 )
        {
          goto LABEL_13;                        // removeStandalone: also remove non-composite objects matching tile coords (skip if flag=0x40: persistent)
        }
        RemoveAndFreeRenderObject(renderObj);
        if ( removalDone )
          goto LABEL_14;
        goto LABEL_12;
      }
      if ( renderObj->parentIndex == packedTileCoords )
        break;                                  // Composite object whose parentIndex matches packed tile coords — full teardown
    }
LABEL_13:
    if ( --renderObj <= g_RenderObjectsEnd )
      goto LABEL_14;
  }
  RemoveRenderObjectByIndex(renderObj->posY);   // Remove child render objects referenced by posY and posX
  if ( renderObj->posY != renderObj->posX )
    RemoveRenderObjectByIndex(renderObj->posX);
  if ( (renderObj->flags & 8) != 0 )            // bit3: object owns allocated memory block in spriteDataPtr
    FreeMemoryBlock((int)renderObj->spriteDataPtr);
  compositeObj = renderObj;
  RemoveChildTextObjects((int)renderObj);       // Remove any text overlay child objects
  FreeRenderObject(renderObj);
  UpdateRenderObjectCache(renderObj);
  if ( removeStandalone && !removalDone )
  {
LABEL_12:
    ++removalDone;
    goto LABEL_13;
  }
LABEL_14:
  if ( compositeObj )                           // If composite object was found, save its render layer back to tile map entry
  {
    renderInfo = compositeObj->renderInfo;
    HIBYTE(renderInfo) &= 0xF0u;                // Mask: keep upper 4 bits of HIBYTE (render layer) only
    *(_WORD *)tileEntryPtr = renderInfo;
  }
  return ExecuteScriptWithContext(
           *(_DWORD *)(tileEntryPtr + 1),
           tileY,
           tileX,
           0,
           0xFFFF,
           0,
           (int)spriteSlot,
           (BYTE *)spriteSlot->scriptPtr);
}

__int16 __stdcall CreateTileObject(int a1, int a2, __int16 a3)
{
  __int16 *v3; // ebp
  AnimationSlot *v4; // eax
  _BYTE *frameModificationData; // esi
  int v6; // ebx
  SpriteSlot *v7; // esi
  int v8; // ebx
  uint16_t *v9; // edi
  RenderObject *v10; // eax
  unsigned int v11; // ecx
  __int16 result; // ax
  int v13; // [esp+10h] [ebp-Ch]
  int v14; // [esp+10h] [ebp-Ch]
  size_t Size; // [esp+14h] [ebp-8h]

  v3 = (__int16 *)(g_TileMapBuffer + 2 * (a2 + (a1 << g_TileMapShift)));
  LOBYTE(v13) = a3;
  if ( (a3 & 0x100) != 0 )
  {
    v4 = &g_AnimationSlotArray[(unsigned __int8)a3];
    frameModificationData = v4->frameModificationData;
    v13 = *((unsigned __int8 *)v4->frameSelectionData
          + ((*(unsigned __int8 *)v4->frameSelectionData * (unsigned int)HIWORD(g_RandomSeed)) >> 16)
          + 1);
    g_RandomSeed *= 1103515245;
    g_RandomSeed += 12345;
    if ( frameModificationData )
    {
      v6 = (int)HIBYTE(a3) >> 4;
      HIBYTE(a3) = frameModificationData[v13];
      g_TileMatchHandlers[v6]((int)&a3);
    }
  }
  else if ( (unsigned __int8)a3 == 254 )
  {
    RemoveAndCreateTileObject(1, a1, a2);
    result = a3;
    *v3 = a3;
    return result;
  }
  g_MoveDirectionHandlers[(int)HIBYTE(a3) >> 4]((int)&a3);
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  LOBYTE(a3) = v13;
  v7 = &g_SpriteSlotArray[(unsigned __int8)v13];
  if ( LOBYTE(v7->baseData) )
  {
    RemoveAndCreateTileObject(0, a1, a2);
    v10 = AllocRenderObject();
    v8 = (int)v10;
    v10->flags = HIBYTE(a3) & 0xF0 | 3;
    v10->slotIndex = a3;
    LOBYTE(v10->animLink) = 0;
    HIBYTE(v10->animLink) = BYTE2(v7->baseData);
    LOBYTE(v10->parentIndex) = a2;
    HIBYTE(v10->parentIndex) = a1;
    v10->posY = -1;
    v10->posX = -1;
    v10->renderInfo = *v3;
    if ( (unsigned int)v10 < g_GameFlags1 )
      HIBYTE(v10->renderInfo) |= 1u;
    v11 = HIBYTE(v7->baseData) + 1;
    Size = v11;
    if ( v11 <= 4 )
    {
      *(_DWORD *)&v10->spriteDataPtr = 0;
      v9 = &v10->spriteDataPtr;
    }
    else
    {
      v9 = (uint16_t *)AllocFromMemoryPool(v11);
      *(_DWORD *)(v8 + 12) = v9;
      ClearBuffer(Size, v9);
      *(_BYTE *)v8 |= 8u;
    }
    v14 = ((int)g_RenderObjectsStart - v8) / 16;
    if ( LOBYTE(v7->baseData) == 3 && !g_LastCheckedRenderObject )
      g_LastCheckedRenderObject = v8;
  }
  else
  {
    RemoveAndCreateTileObject(1, a1, a2);
    v8 = 0;
    v14 = 0xFFFF;
    v9 = 0;
  }
  *v3 = a3;
  return ExecuteScriptWithContext(SHIBYTE(a3), a1, a2, (int)v9, v14, v8, (int)v7, (int)v7->dataPtr);
}

char __stdcall MoveObjectOnTileMap(int a1)
{
  char v1; // bl
  __int16 v2; // cx
  _WORD *v3; // esi
  char result; // al

  if ( HIWORD(a1) )
  {
    g_CurrentTileX_Wrapped = (g_ScreenWidth + g_CurrentTileX_Wrapped + SBYTE2(a1)) % g_ScreenWidth;
    g_CurrentTileY_Wrapped = (g_ScreenHeight + g_CurrentTileY_Wrapped + SHIBYTE(a1)) % g_ScreenHeight;
    v1 = *(_BYTE *)(g_GameFlags1 + 3) & 0xF;
    v2 = *(_WORD *)(g_GameFlags1 + 2);
    HIBYTE(v2) &= 0xF0u;
    *(_WORD *)(g_TileMapBuffer + 2 * (g_PreviousTileX + (g_PreviousTileY << g_TileMapShift))) = v2;
    RemoveAndCreateTileObject(0, g_CurrentTileY_Wrapped, g_CurrentTileX_Wrapped);
    *(_BYTE *)(g_GameFlags1 + 6) = g_CurrentTileX_Wrapped;
    *(_BYTE *)(g_GameFlags1 + 7) = g_CurrentTileY_Wrapped;
    v3 = (_WORD *)(g_TileMapBuffer + 2 * (g_CurrentTileX_Wrapped + (g_CurrentTileY_Wrapped << g_TileMapShift)));
    *(_WORD *)(g_GameFlags1 + 2) = *v3;
    *(_BYTE *)(g_GameFlags1 + 3) |= v1;
    *v3 = *(unsigned __int8 *)(g_GameFlags1 + 1) | ((*(_BYTE *)g_GameFlags1 & 0xF0) << 8);
    g_TileMapModified = 1;
  }
  result = BYTE1(a1) & 0xF0;
  if ( (BYTE1(a1) & 0xF0) != g_CurrentDirection )
  {
    g_CurrentDirection = BYTE1(a1) & 0xF0;
    *(_BYTE *)g_GameFlags1 &= 0xFu;
    *(_BYTE *)g_GameFlags1 |= g_CurrentDirection;
    result = g_CurrentDirection;
    *(_BYTE *)(g_TileMapBuffer + 2 * (g_CurrentTileX_Wrapped + (g_CurrentTileY_Wrapped << g_TileMapShift)) + 1) = g_CurrentDirection;
    g_TileMapModified = 1;
  }
  return result;
}

char UpdateFrameRate()
{
  GameLoopData *v0; // ebx
  unsigned __int8 v1; // al

  v0 = g_GameLoopDataPtr;                       // Load g_GameLoopDataPtr to work with game loop timing data
  if ( (_BYTE)video_recursion_counter == 14 )
  {
    BYTE2(g_GameLoopDataPtr->flags_or_id) = 0;
    HIBYTE(v0->flags_or_id) = 0;
  }
  else
  {
    HIBYTE(g_GameLoopDataPtr->flags_or_id) = video_recursion_counter;// Set video_recursion_counter field
    BYTE2(v0->flags_or_id) = 1;                 // Set update_flag = 1
  }
  if ( LOBYTE(v0->flags_or_id) )
    return 0;
  ++*(_DWORD *)&v0->frame_rate;                 // Increment frame_counter
  if ( !BYTE1(v0->flags_or_id) )
    BYTE1(v0->flags_or_id) = 1;                 // Initialize frame_rate to 1 if zero
  if ( BYTE1(v0->flags_or_id) > 0x32u )
    BYTE1(v0->flags_or_id) = 50;                // Cap frame_rate at 50
  v1 = BYTE1(v0->flags_or_id);
  if ( v1 != g_DefaultFrameRate )
  {
    g_DefaultFrameRate = BYTE1(v0->flags_or_id);
    SetFrameRate(v1);
  }
  return 1;
}

int __stdcall ProcessInputAndRender(
        char inputFlag,
        unsigned __int8 eventCode,
        int coordY,
        int coordX,
        int altCoordY,
        int altCoordX)
{
  RenderObject *v6; // esi
  int v8; // ebp
  RenderObject *v9; // ebx
  SpriteSlot *v10; // edi
  int baseData_low; // ecx
  int result; // eax
  char baseData; // [esp+10h] [ebp-Ch]
  unsigned __int8 v14; // [esp+11h] [ebp-Bh]
  unsigned __int8 v15; // [esp+12h] [ebp-Ah]
  int ya; // [esp+14h] [ebp-8h]

  v6 = 0;
  v14 = 0;
  v15 = -1;
  switch ( eventCode )
  {
    case 0x8Fu:
      UpdateRenderObjectCache(g_LastCheckedRenderObject);
      break;
    case 0x82u:
      v14 = -112;
      break;
    case 0x83u:
      v14 = -96;
      break;
    case 0xEu:
      coordX = altCoordX;
      coordY = altCoordY;
      break;
  }
  if ( inputFlag != 14 )
    v14 |= inputFlag | 0x40;
  v8 = g_CameraOffsetX + coordX;
  ya = g_CameraOffsetY + coordY;
  v9 = g_RenderObjectsStart;
  if ( g_RenderObjectsStart > g_RenderObjectsEnd )
  {
    while ( (v9->flags & 3) != 3 )
    {
LABEL_26:
      if ( --v9 <= g_RenderObjectsEnd )
        goto LABEL_27;
    }
    v10 = &g_SpriteSlotArray[v9->slotIndex];
    baseData_low = LOBYTE(v10->baseData);
    if ( baseData_low == 1 )
    {
      LOBYTE(v9->animLink) = v14;
      goto LABEL_22;
    }
    if ( baseData_low != 2 )
    {
      if ( baseData_low != 3 )
      {
LABEL_22:
        if ( (!v6 || HIBYTE(v9->animLink) <= v15) && CheckObjectHitTest(ya, v8, v9) )
        {
          v6 = v9;
          baseData = (char)v10->baseData;
          v15 = HIBYTE(v9->animLink);
        }
        goto LABEL_26;
      }
      if ( v9 != g_LastCheckedRenderObject )
      {
        LOBYTE(v9->animLink) = 0;
        goto LABEL_22;
      }
    }
    LOBYTE(v9->animLink) = v14 & 0x4F;
    goto LABEL_22;
  }
LABEL_27:
  if ( !v6 )
  {
    g_CurrentTileX = v8 / g_TileWidth;
    g_CurrentTileY = ya / g_TileHeight;
    HIBYTE(video_recursion_counter) = *(_BYTE *)(g_TileMapBuffer
                                               + 2 * (v8 / g_TileWidth + ((ya / g_TileHeight) << g_TileMapShift)));
    goto LABEL_44;
  }
  HIBYTE(video_recursion_counter) = baseData;
  if ( baseData == 2 )
  {
    if ( eventCode != 130 && eventCode != 131 )
    {
      if ( eventCode == 14 )
      {
        v14 |= 0x10u;
      }
      else if ( eventCode == 129 )
      {
        v14 |= 0x20u;
      }
    }
  }
  else
  {
    if ( baseData != 3 || v14 != 144 && v14 != 160 )
      goto LABEL_42;
    g_LastCheckedRenderObject = v6;
  }
  LOBYTE(v6->animLink) = v14;
LABEL_42:
  g_CurrentTileX = LOBYTE(v6->parentIndex);
  g_CurrentTileY = HIBYTE(v6->parentIndex);
LABEL_44:
  g_CurrentInput = eventCode;
  result = eventCode;
  if ( eventCode == 130 || eventCode == 131 )
  {
    g_PreviousInput = eventCode;
    HIBYTE(g_PreviousScreen) = HIBYTE(video_recursion_counter);
    g_SelectedTileX = g_CurrentTileX;
    g_SelectedTileY = g_CurrentTileY;
    return g_CurrentTileY;
  }
  else
  {
    if ( eventCode == 129 )
      g_PreviousInput = 14;
    g_CurrentInput = 14;
  }
  return result;
}

int __stdcall StepValueTowardTarget(unsigned __int8 target, _BYTE *current)
{
  int currentVal; // eax
  int delta; // ebx
  int stepSize; // [esp+4h] [ebp-4h]

  currentVal = (unsigned __int8)*current;       // Read current value and compute delta = target - current
  delta = target - currentVal;
  if ( target != currentVal )
  {
    stepSize = 255 / (unsigned __int8)g_DefaultFrameRate;// Positive delta: step up by 255/frameRate (clamped to delta if smaller)
    if ( delta >= 0 )
    {
      if ( stepSize > delta )
LABEL_6:
        LOBYTE(stepSize) = target - currentVal;
    }
    else
    {
      stepSize = -255 / (unsigned __int8)g_DefaultFrameRate;// Negative delta: step down by -255/frameRate (clamped to delta if closer)
      if ( stepSize < delta )
        goto LABEL_6;
    }
    *current += stepSize;                       // Apply step to current value
  }
  return target - currentVal;                   // Return remaining distance (0 = reached target)
}

int UpdateScreenSections()
{
  int result; // eax

  if ( StepValueTowardTarget(g_PreviousScreen, (_BYTE *)&g_GraphicsConfigPtr + 1) )
    SetSoundVolume(*(int *)((char *)&g_GraphicsConfigPtr + 1));
  result = StepValueTowardTarget(BYTE1(g_PreviousScreen), (_BYTE *)&g_GraphicsConfigPtr + 2);
  if ( result )
    return SetMusicVolume(*(int *)((char *)&g_GraphicsConfigPtr + 2));
  return result;
}

char ProcessNormalEvent()
{
  uint16_t prevLink; // si
  RenderObject *next_object; // ebx
  int nextLink; // esi
  uint16_t prev_object_link; // si
  RenderObject *next_object_linked; // ebx
  SpriteSlot *sprite_slot; // eax
  int v7; // edx
  char direction_offset; // di
  unsigned int direction_index; // esi
  int v10; // eax
  RenderObject *current_object; // [esp+10h] [ebp-Ch]
  int **sprite_command_list; // [esp+14h] [ebp-8h]

  current_object = *(RenderObject **)&g_InputState[3];
  if ( !*(_DWORD *)&g_InputState[3] )
    current_object = g_RenderObjectsStart;
  if ( current_object <= g_RenderObjectsEnd )
    goto LABEL_15;
  while ( 1 )
  {
    if ( (current_object->flags & 3) != 3 )
    {
      if ( !g_SpecialMode && (current_object->flags & 0x83) == 0x81 && current_object->parentIndex == 0xFFFF )
        UpdateSpriteAnimation(1, 0, (int)current_object);
      goto LABEL_48;
    }
    if ( !g_SpecialMode
      || ((1 << (current_object->slotIndex & 7))
        & *(unsigned __int8 *)(((int)current_object->slotIndex >> 3) + g_SpecialMode)) != 0 )
    {
      if ( (current_object->renderInfo & 0x100) != 0 )
      {
        HIBYTE(current_object->renderInfo) &= ~1u;
        goto LABEL_48;
      }
      if ( (current_object->flags & 4) != 0 )
      {
        if ( !UpdateSpriteAnimation(
                current_object->posY != current_object->posX,
                1,
                (int)&g_RenderObjectsStart[-current_object->posY]) )
        {
          if ( current_object == *(RenderObject **)&g_InputState[3] )
            goto LABEL_15;
          goto LABEL_48;
        }
        prevLink = current_object->posX;
        current_object->posY = prevLink;
        if ( prevLink != 0xFFFF )
        {
          next_object = &g_RenderObjectsStart[-prevLink];
          next_object->flags |= 0x80u;
          LOBYTE(next_object->animLink) = current_object->parentIndex;
          HIBYTE(next_object->animLink) = HIBYTE(current_object->parentIndex);
          CalculateObjectPosition((int)next_object);
          AddObjectToDirtyRegions((int)next_object);
        }
        current_object->flags &= ~4u;
      }
      else
      {
        nextLink = current_object->posY;
        if ( nextLink != 0xFFFF )
        {
          if ( UpdateSpriteAnimation(1, 0, (int)&g_RenderObjectsStart[-nextLink]) )
          {
            prev_object_link = current_object->posX;
            current_object->posY = prev_object_link;
            if ( prev_object_link != 0xFFFF )
            {
              next_object_linked = &g_RenderObjectsStart[-prev_object_link];
              next_object_linked->flags |= 0x80u;
              LOBYTE(next_object_linked->animLink) = current_object->parentIndex;
              HIBYTE(next_object_linked->animLink) = HIBYTE(current_object->parentIndex);
              CalculateObjectPosition((int)next_object_linked);
              AddObjectToDirtyRegions((int)next_object_linked);
            }
          }
        }
      }
      g_GameFlags1 = (int)current_object;
      g_ResourceID2 = g_RenderObjectsStart - current_object;
      sprite_slot = &g_SpriteSlotArray[current_object->slotIndex];
      g_GameFlags2 = (int)sprite_slot;
      v7 = (current_object->flags & 8) != 0
         ? *(_DWORD *)&current_object->spriteDataPtr
         : (int)&current_object->spriteDataPtr;
      g_CurrentObjectData = v7;
      sprite_command_list = (int **)sprite_slot->subObjectArray;
      g_RenderComplete = 0;
      if ( sprite_command_list )
        break;
    }
LABEL_48:
    if ( --current_object <= g_RenderObjectsEnd )
      goto LABEL_15;
  }
  while ( 2 )
  {
    direction_offset = 0;
    g_CurrentDirection = *(_BYTE *)g_GameFlags1 & 0xF0;
    if ( (unsigned __int8)g_CurrentDirection == 128 )
    {
      direction_offset = 1;
    }
    else if ( g_CurrentDirection != 64 )
    {
      if ( g_CurrentDirection != 32 )
        goto LABEL_31;
      goto LABEL_30;
    }
    ++direction_offset;
LABEL_30:
    ++direction_offset;
LABEL_31:
    direction_index = 0;
    while ( 1 )
    {
      if ( ((1 << direction_index) & *(unsigned __int8 *)(g_GameFlags2 + 1)) != 0 )
      {
        g_SpriteDirectionIndex = (direction_offset + (direction_index & 3)) & 3;
        if ( direction_index > 3 )
          g_SpriteDirectionIndex += 4;
        g_TileProcessor = g_SpriteTransformTable[g_SpriteDirectionIndex];
        g_ContinueRendering = 0;
        v10 = RenderSprite(0, *sprite_command_list);
        if ( v10 )
          break;
      }
      if ( (int)++direction_index >= 8 )
        goto LABEL_40;
    }
    if ( v10 != 1 )
    {
      if ( !*(_DWORD *)&g_InputState[3] )
      {
        UpdateRenderObjectCache((SpriteSlot *)current_object);
        goto LABEL_48;
      }
      goto LABEL_53;
    }
    if ( g_ContinueRendering )
    {
      direction_index = 0;
LABEL_40:
      if ( *(char *)*sprite_command_list >= 0 )
      {
        ++sprite_command_list;
        continue;
      }
      if ( !direction_index )
      {
        *(_DWORD *)&g_InputState[3] = current_object;
        goto LABEL_15;
      }
    }
    break;
  }
  if ( !*(_DWORD *)&g_InputState[3] )
    goto LABEL_48;
LABEL_53:
  *(_DWORD *)&g_InputState[3] = 0;
LABEL_15:
  g_GameFlags1 = 0;
  g_ResourceID2 = 0xFFFF;
  return 1;
}

char __stdcall ProcessSpecialEvent(char keyCode)
{
  RenderObject *current_object; // ebx
  RenderObject *parent_object; // edx

  for ( current_object = g_RenderObjectsStart; current_object > g_RenderObjectsEnd; --current_object )
  {
    if ( (current_object->flags & 0x87) == 0x81 )
    {
      if ( current_object->slotIndex + 1 == LOBYTE(current_object->renderInfo) && current_object->parentIndex != 0xFFFF )
      {
        parent_object = &g_RenderObjectsStart[-current_object->parentIndex];
        LOBYTE(current_object->animLink) = parent_object->parentIndex;
        HIBYTE(current_object->animLink) = HIBYTE(parent_object->parentIndex);
      }
      UpdateSpriteAnimation(0, 0, current_object);
    }
  }
  if ( (unsigned __int8)video_recursion_counter == 32
    || (unsigned __int8)video_recursion_counter == 13
    || (unsigned __int8)video_recursion_counter != 14 && keyCode == 14 )
  {
    return ProcessTextInput(video_recursion_counter);
  }
  else
  {
    return 1;
  }
}

char __stdcall HandleGameEvent(int coordY, int coordX)
{
  int v2; // ebx

  if ( g_DisableCursor )
    goto LABEL_8;
  v2 = LOBYTE(g_CursorObject.renderInfo);
  if ( LOBYTE(g_CursorObject.renderInfo) > 1u )
  {
    AddObjectToDirtyRegions(&g_CursorObject);
    if ( ++g_CursorObject.slotIndex != v2 )
    {
      *(_DWORD *)&g_CursorObject.spriteDataPtr += 8;
      goto LABEL_6;
    }
    if ( (g_CursorObject.flags & 4) == 0 )
    {
      g_CursorObject.slotIndex = 0;
      *(_DWORD *)&g_CursorObject.spriteDataPtr -= 8 * v2 - 8;
      goto LABEL_6;
    }
    goto LABEL_11;
  }
  if ( (g_CursorObject.flags & 4) != 0 )
  {
LABEL_11:
    InitGraphicsMode(1, 0);
    goto LABEL_6;
  }
  if ( (__int16)g_CursorObject.posX != coordX || (__int16)g_CursorObject.posY != coordY || !g_InputState[1] )
  {
    AddObjectToDirtyRegions(&g_CursorObject);
LABEL_6:
    g_CursorObject.posX = coordX;
    g_CursorObject.posY = coordY;
    if ( g_InputState[1] )
      AddObjectToDirtyRegions(&g_CursorObject);
LABEL_8:
    g_GameStateFlag = 1;
    return 1;
  }
  return 1;
}

char RenderDirtyRegions()
{
  RenderObject *i; // edx
  int v1; // eax
  int *p_render_objects_array; // esi
  int v3; // ebx
  char *v4; // edi
  int v5; // ecx
  int v6; // eax
  char *v7; // edi
  int v8; // eax
  int v9; // ebx
  int v10; // esi
  int v11; // edi
  int v12; // ecx
  int v13; // ecx
  int v14; // ecx
  int v15; // ecx
  int v16; // edx
  int v17; // ecx
  int v19; // [esp-1Ch] [ebp-1074h]
  int v20; // [esp-18h] [ebp-1070h]
  int v21; // [esp-Ch] [ebp-1064h]
  int v22; // [esp-8h] [ebp-1060h]
  unsigned __int16 *SpriteData; // [esp-4h] [ebp-105Ch]
  _DWORD *dirty_region; // [esp+Ch] [ebp-104Ch]
  int object_index; // [esp+10h] [ebp-1048h]
  int object_count; // [esp+14h] [ebp-1044h]
  int render_objects_array[1024]; // [esp+18h] [ebp-1040h] BYREF
  _DWORD *next_dirty_region; // [esp+1018h] [ebp-40h]
  int clip_left; // [esp+101Ch] [ebp-3Ch]
  int clip_right; // [esp+1020h] [ebp-38h]
  int clip_top; // [esp+1024h] [ebp-34h]
  int clip_bottom; // [esp+1028h] [ebp-30h]
  unsigned __int16 *sprite_header; // [esp+102Ch] [ebp-2Ch]
  int sprite_width; // [esp+1030h] [ebp-28h]
  int sprite_right; // [esp+1034h] [ebp-24h]
  int sprite_height; // [esp+1038h] [ebp-20h]
  int sprite_bottom; // [esp+103Ch] [ebp-1Ch]
  char render_flags; // [esp+1040h] [ebp-18h]
  int v40; // [esp+1044h] [ebp-14h]
  int v41; // [esp+1048h] [ebp-10h]
  int *current_object_ptr; // [esp+104Ch] [ebp-Ch]
  int v43; // [esp+1050h] [ebp-8h]
  char *v44; // [esp+1054h] [ebp-4h]

  dirty_region = (_DWORD *)g_DirtyRegionsList;
  if ( g_DirtyRegionsList )
  {
    object_count = 0;
    for ( i = g_RenderObjectsStart; i > g_RenderObjectsEnd; --i )
    {
      if ( (i->flags & 0x83) == 0x81 )
      {
        v1 = object_count++;
        *(render_objects_array + v1) = (int)i;
      }
    }
    if ( object_count )
    {
      p_render_objects_array = render_objects_array;
      v41 = 1;
      v43 = object_count + 1;
      v44 = (char *)&render_objects_array[1];
      do
      {
        v3 = v41;
        if ( v41 < object_count )
        {
          v4 = v44;
          do
          {
            v5 = *p_render_objects_array;
            if ( *(_BYTE *)(*p_render_objects_array + 3) < *(_BYTE *)(*(_DWORD *)v4 + 3) )
            {
              *p_render_objects_array = *(_DWORD *)v4;
              *(_DWORD *)v4 = v5;
            }
            v4 += 4;
            ++v3;
          }
          while ( v3 < object_count );
        }
        ++p_render_objects_array;
        v44 += 4;
        ++v41;
      }
      while ( v43 > v41 );
    }
    if ( !g_DisableCursor && g_InputState[1] )
    {
      v6 = object_count++;
      *(render_objects_array + v6) = (int)&g_CursorObject;
    }
    do
    {
      next_dirty_region = (_DWORD *)dirty_region[4];
      clip_left = *dirty_region - g_CameraOffsetX;
      if ( clip_left < 0 )
        clip_left = 0;
      clip_right = dirty_region[2] - g_CameraOffsetX;
      if ( clip_right > g_MouseX )
        clip_right = g_MouseX;
      if ( clip_right > clip_left )
      {
        clip_top = dirty_region[1] - g_CameraOffsetY;
        if ( clip_top < 0 )
          clip_top = 0;
        clip_bottom = dirty_region[3] - g_CameraOffsetY;
        if ( clip_bottom > g_MouseY )
          clip_bottom = g_MouseY;
        if ( clip_bottom > clip_top )
        {
          CopyRectToBackbuffer(
            clip_bottom,
            clip_right,
            clip_top,
            clip_left,
            *(_DWORD *)(g_SnapshotBasePtr + 12),
            *(_DWORD *)(g_SnapshotBasePtr + 8),
            -clip_top,
            -clip_left,
            g_SnapshotBasePtr + 24);
          object_index = 0;
          if ( object_count > 0 )
          {
            current_object_ptr = render_objects_array;
            do
            {
              v7 = (char *)*current_object_ptr;
              v8 = *(_DWORD *)(*current_object_ptr + 12);
              sprite_header = *(unsigned __int16 **)v8;
              v9 = *((__int16 *)v7 + 4) - g_CameraOffsetX;
              v10 = *((__int16 *)v7 + 5) - g_CameraOffsetY;
              render_flags = *v7;
              if ( (render_flags & 0x40) != 0 )
              {
                v9 -= *(__int16 *)(v8 + 4);
                v10 -= *(__int16 *)(v8 + 6);
              }
              if ( v9 < clip_right && v10 < clip_bottom )
              {
                sprite_width = *sprite_header;
                sprite_right = v9 + sprite_width;
                if ( v9 + sprite_width > clip_left )
                {
                  sprite_height = sprite_header[1];
                  sprite_bottom = v10 + sprite_height;
                  if ( v10 + sprite_height > clip_top )
                  {
                    v11 = v10 - clip_top;
                    SpriteData = LoadSpriteData(sprite_header);
                    v12 = v9 - clip_left;
                    v40 = v9 - clip_left;
                    if ( v9 - clip_left >= 0 )
                      v12 = 0;
                    v22 = v12;
                    if ( v11 >= 0 )
                      v13 = 0;
                    else
                      v13 = v11;
                    v21 = v13;
                    if ( v40 >= 0 )
                      v14 = v9;
                    else
                      v14 = clip_left;
                    v20 = v14;
                    if ( v11 >= 0 )
                      v15 = v10;
                    else
                      v15 = clip_top;
                    v19 = v15;
                    if ( clip_right < sprite_right )
                      v16 = clip_right;
                    else
                      v16 = sprite_right;
                    v17 = sprite_bottom;
                    if ( clip_bottom < sprite_bottom )
                      v17 = clip_bottom;
                    if ( (render_flags & 8) != 0 )
                    {
                      if ( (render_flags & 0x10) != 0 )
                        DrawSpriteFlippedHV(v17, v16, v19, v20, sprite_height, sprite_width, v21, v22, (int)SpriteData);
                      else
                        DrawSpriteFlippedH(v17, v16, v19, v20, sprite_height, sprite_width, v21, v22, (int)SpriteData);
                    }
                    else if ( (render_flags & 0x10) != 0 )
                    {
                      DrawSpriteFlippedV(v17, v16, v19, v20, sprite_height, sprite_width, v21, v22, (int)SpriteData);
                    }
                    else
                    {
                      DrawSpriteTransparent(v17, v16, v19, v20, sprite_height, sprite_width, v21, v22, (int)SpriteData);
                    }
                  }
                }
              }
              ++current_object_ptr;
              ++object_index;
            }
            while ( object_index < object_count );
          }
          BlitDirtyRegion(g_InputState[0], clip_bottom, clip_right, clip_top, clip_left);
        }
      }
      FreeMemoryBlock((int)dirty_region);
      dirty_region = next_dirty_region;
    }
    while ( next_dirty_region );
    FlushGDI();
    g_DirtyRegionsList = 0;
    g_InputState[0] = 0;
  }
  return 1;
}

int __stdcall ClearDirtyRegionsAndInit(char a1)
{
  int *v1; // ebx
  int *v2; // esi

  v1 = (int *)g_DirtyRegionsList;
  if ( g_DirtyRegionsList )
  {
    do
    {
      v2 = (int *)v1[4];
      if ( a1 )
        BlitDirtyRegion(0, v1[3], v1[2], v1[1], *v1);
      FreeMemoryBlock((int)v1);
      v1 = v2;
    }
    while ( v2 );
  }
  FlushGDI();
  g_DirtyRegionsList = (int)v1;
  HIBYTE(g_PreviousScreen) = -1;
  g_PreviousInput = 14;
  g_SpecialMode = 0;
  g_RandomSeed = 1103515245 * time(0) + 12345;
  g_LastCheckedRenderObject = 0;
  UpdateRenderObjectCache(0); return 0;
}

int __stdcall RenderSprite(int useAbsoluteCoords, int *spriteData)
{
  unsigned __int64 v2; // rax
  int *dataPtr; // edi
  int rangeVal; // eax
  int rangeValCopy; // ecx
  AnimationSlot *animSlot; // ebp
  _BYTE *frameModData; // ebx
  unsigned __int8 frameBit; // cl
  char savedMoveFlags; // bl
  int choiceGroupIdx; // ebp
  int *cmdPtr; // edi
  unsigned int randomIndex; // eax
  int cmdResult; // eax
  int rangeCount; // esi
  unsigned int skipCount; // esi
  int cmdResult2; // eax
  unsigned int pickIndex; // esi
  unsigned int pickCounter; // ecx
  unsigned int pickDecrement; // esi
  unsigned int coinFlip; // esi
  int cmdResult3; // eax
  int choiceKey; // esi
  int cmdResult4; // eax
  int pad; // [esp+Ch] [ebp-44h]
  int rangeBase; // [esp+10h] [ebp-40h] BYREF
  _DWORD rangeArray[5]; // [esp+14h] [ebp-3Ch] BYREF
  int spriteFlags; // [esp+28h] [ebp-28h] BYREF
  BYTE *tileEntry; // [esp+2Ch] [ebp-24h] BYREF
  unsigned int groupHeader; // [esp+30h] [ebp-20h]
  int *groupEndPtr; // [esp+34h] [ebp-1Ch]
  int commandCount; // [esp+38h] [ebp-18h]
  int groupIndex; // [esp+3Ch] [ebp-14h]
  __int16 tileMapValue; // [esp+40h] [ebp-10h]
  unsigned __int8 matchResult; // [esp+42h] [ebp-Eh]
  int savedRenderLayer; // [esp+44h] [ebp-Ch]
  _DWORD *rangeMinPtr; // [esp+48h] [ebp-8h]
  int *rangeMaxPtr; // [esp+4Ch] [ebp-4h]

  commandCount = 0;
  if ( useAbsoluteCoords )                      // === SETUP PHASE: Initialize context for absolute coordinate mode ===
  {
    g_GameFlags1 = 0;
    g_ResourceID2 = 0xFFFF;
    g_GameFlags2 = 0;
    g_SavedState = 0;
    g_ResourceID1 = 0xFFFF;
    g_CurrentTileY_Wrapped = 0;
    g_CurrentTileX_Wrapped = 0;
    g_PreviousTileY = 0;
    g_PreviousTileX = 0;
    g_CurrentDirection = 16;
    g_TileProcessor = (MoveDirectionHandler)TransformDir_Identity;
    g_SpriteCommandHandler = ProcessAbsoluteTileCoords;
    g_CurrentObjectData = 0;
  }
  else
  {
    g_CurrentTileX_Wrapped = *(unsigned __int8 *)(g_GameFlags1 + 6);// === SETUP PHASE: Initialize context for relative coordinate mode ===
    g_PreviousTileX = g_CurrentTileX_Wrapped;
    g_CurrentTileY_Wrapped = *(unsigned __int8 *)(g_GameFlags1 + 7);
    g_PreviousTileY = g_CurrentTileY_Wrapped;
    g_SpriteCommandHandler = ProcessRelativeTileCoords;
  }
  spriteFlags = *spriteData;                    // === Read sprite header flags ===
  LODWORD(v2) = 4;
  dataPtr = spriteData + 1;
  if ( (spriteFlags & 1) != 0 )                 // --- Bit 0: Precondition script. Evaluate and abort if returns 0 ---
  {
    LODWORD(v2) = EvaluateScriptBytecode(v2, (BYTE *)*dataPtr);
    if ( !(_DWORD)v2 )
      return 0;
    dataPtr = spriteData + 2;
  }
  if ( (spriteFlags & 2) == 0 )
    goto LABEL_34;                              // --- Bit 1: Tile pattern matching phase ---
  groupIndex = -1;
LABEL_9:
  groupHeader = *dataPtr++;                     // === PATTERN GROUP LOOP: Read group header (lo16=entry count, hi8=type, bit16=last group flag) ===
  groupEndPtr = &dataPtr[(unsigned __int16)groupHeader];
  if ( HIBYTE(groupHeader) == 4 )               // Type 4: accumulating range group - track min/max across groups
  {
    if ( ++groupIndex )
    {
      rangeVal = 2 * groupIndex;
      rangeValCopy = *(&pad + 2 * groupIndex);
      rangeArray[rangeVal] = rangeValCopy;
      rangeArray[rangeVal - 1] = rangeValCopy;
    }
    else
    {
      rangeArray[0] = 0;
      rangeBase = 0;
    }
  }
  else
  {
    groupIndex = -1;
  }
  rangeMinPtr = &rangeArray[2 * groupIndex - 1];
  rangeMaxPtr = &rangeArray[2 * groupIndex];
  while ( 1 )
  {
    tileEntry = (BYTE *)*dataPtr++;             // === PATTERN ENTRY LOOP: Read packed tile entry and match against tilemap ===
    g_TileProcessor(&tileEntry);                // Transform tile coordinates (absolute or relative)
    if ( useAbsoluteCoords )                    // Read tile map value at computed coords
    {
      tileMapValue = *(_WORD *)(g_TileMapBuffer + 2 * (BYTE2(tileEntry) + (HIBYTE(tileEntry) << g_TileMapShift)));
    }
    else
    {
      HIDWORD(v2) = (((g_ScreenHeight + g_CurrentTileY_Wrapped + SHIBYTE(tileEntry)) % g_ScreenHeight) << g_TileMapShift)
                  + (g_ScreenWidth + g_CurrentTileX_Wrapped + SBYTE2(tileEntry)) % g_ScreenWidth;
      tileMapValue = *(_WORD *)(g_TileMapBuffer + 2 * HIDWORD(v2));
    }
    matchResult = 0;
    if ( (BYTE1(tileEntry) & 1) != 0 )          // Bit 0 of entry flags: use animation-based matching (bitmask check)
    {
      if ( (unsigned __int8)tileMapValue == 254 )
        goto LABEL_26;                          // 0xFE = empty tile → no match
      HIDWORD(v2) = 1 << (tileMapValue & 7);    // Check if tile's sprite index is set in animation slot's frameBitmask
      animSlot = &g_AnimationSlotArray[(unsigned __int8)tileEntry];
      if ( (BYTE4(v2) & *((_BYTE *)animSlot->frameBitmask + ((unsigned __int8)tileMapValue >> 3))) == 0 )
        goto LABEL_26;
      frameModData = animSlot->frameModificationData;// If frameModificationData exists, apply frame modification to flags
      if ( frameModData )
      {
        frameBit = BYTE1(tileEntry);
        BYTE1(tileEntry) = frameModData[(unsigned __int8)tileMapValue] | BYTE1(tileEntry) & 0xF;
        g_TileMatchHandlers[(int)frameBit >> 4]((int)&tileEntry);
      }
    }
    else if ( (unsigned __int8)tileEntry != (unsigned __int8)tileMapValue )
    {
      goto LABEL_26;                            // Simple match: entry sprite index must equal tile sprite index
    }
    if ( (BYTE1(tileEntry) & 0xF0 & HIBYTE(tileMapValue)) != 0 )// Check direction/layer overlap between entry flags and tile flags
      matchResult = 2;
LABEL_26:
    LODWORD(v2) = matchResult ^ BYTE1(tileEntry) & 2;// XOR matchResult with mismatch flag → determine if condition failed
    if ( (_DWORD)v2 )                           // Condition failed: check if we've exhausted group or need to bail
    {
      if ( dataPtr == groupEndPtr && groupIndex >= 0 )
      {
        LODWORD(v2) = *rangeMinPtr;
        if ( *rangeMinPtr == *rangeMaxPtr )
          return 0;
      }
      goto LABEL_63;
    }
    LODWORD(v2) = BYTE1(tileEntry) & 0xC;       // Condition succeeded: check continuation mode (bits 2-3 of entry flags)
    if ( (BYTE1(tileEntry) & 0xC) == 0 )        // Mode 0: all entries matched → advance to next group
    {
      dataPtr = groupEndPtr;
      goto LABEL_9;
    }
    if ( (_DWORD)v2 == 4 )
      return 0;                                 // Mode 1 (bits=4): early success → return 0 (pattern matched, no commands)
    if ( (_DWORD)v2 == 8 )
      break;                                    // Mode 2 (bits=8): pattern matched → skip remaining groups, proceed to commands
    HIDWORD(v2) = rangeMaxPtr;                  // Mode 3 (bits=0xC): record matched tile value in g_RandomChoiceArray for later random selection
    rangeCount = *rangeMaxPtr;
    g_RandomChoiceArray[rangeCount] = HIWORD(tileEntry);
    *(_DWORD *)HIDWORD(v2) = rangeCount + 1;
LABEL_63:
    if ( dataPtr == groupEndPtr )
    {
      if ( (groupHeader & 0x10000) != 0 )
        goto LABEL_34;
      goto LABEL_9;
    }
  }
  for ( ; (groupHeader & 0x10000) == 0; LODWORD(v2) = groupHeader )
  {
    groupHeader = *groupEndPtr;
    groupEndPtr += (unsigned __int16)groupHeader + 1;
  }
  dataPtr = groupEndPtr;                        // === MOVEMENT PHASE (bit 2) ===
LABEL_34:
  g_TileMapModified = 0;
  if ( (spriteFlags & 4) != 0 )                 // --- Bit 2: Process movement on tilemap ---
  {
    savedMoveFlags = BYTE1(spriteFlags);        // Save original movement flags before tile processor modifies them
    g_TileProcessor(&spriteFlags);
    g_MoveDirectionHandlers[(int)BYTE1(spriteFlags) >> 4]((int)&spriteFlags);// Dispatch direction handler based on upper 4 bits of movement flags
    g_RandomSeed *= 1103515245;                 // Advance LCG random seed
    g_RandomSeed += 12345;
    BYTE1(spriteFlags) = savedMoveFlags & 0xF | BYTE1(spriteFlags) & 0xF0;
    LOBYTE(v2) = MoveObjectOnTileMap(spriteFlags);
  }
  *((_BYTE*)&g_PreviousScreen + 2) = 0;
  if ( (spriteFlags & 8) != 0 )                 // --- Bit 3: Execute script bytecode ---
  {                                             // Save current render layer before script execution
    if ( g_GameFlags1 )
    {
      LODWORD(v2) = g_GameFlags1;
      LOBYTE(v2) = *(_BYTE *)(g_GameFlags1 + 5);
      LOBYTE(savedRenderLayer) = v2;
    }
    EvaluateScriptBytecode(v2, (BYTE *)*dataPtr++);
    if ( !*((_BYTE*)&g_PreviousScreen + 2)
      && !g_TileMapModified
      && g_GameFlags1
      && *(_BYTE *)(g_GameFlags1 + 5) != (_BYTE)savedRenderLayer
      && *(_WORD *)(g_GameFlags1 + 10) != 0xFFFF )// If render layer changed during script and overlay exists, mark for redraw
    {
      AddObjectToDirtyRegionByIndex(*(unsigned __int16 *)(g_GameFlags1 + 10));
    }
  }
  if ( !*((_BYTE*)&g_PreviousScreen + 2) && g_TileMapModified )// If tilemap was modified during script, update object position
    UpdateObjectPosition(g_GameFlags1);
  if ( (spriteFlags & 0x10) != 0 )              // --- Bit 4: Command dispatch phase with randomization ---
  {
    choiceGroupIdx = -1;
    groupHeader = *dataPtr;
    cmdPtr = dataPtr + 1;
    for ( groupEndPtr = &cmdPtr[(unsigned __int16)groupHeader]; ; groupEndPtr += (unsigned __int16)groupHeader + 1 )
    {
      switch ( HIBYTE(groupHeader) )
      {
        case 0u:
          randomIndex = ((BYTE1(spriteFlags) & 3) * (unsigned int)HIWORD(g_RandomSeed)) >> 16;// Type 0: Three-way random (0=first group, 1=second group, 2=skip first then use second)
          if ( randomIndex == 2 )
          {
            groupEndPtr += (unsigned __int16)*groupEndPtr + 1;
          }
          else if ( randomIndex != 1 )
          {
            goto LABEL_55;
          }
          cmdPtr = groupEndPtr + 1;
          groupEndPtr += (unsigned __int16)*groupEndPtr + 1;
LABEL_55:
          g_RandomSeed *= 1103515245;
          g_RandomSeed += 12345;
          do
          {
            cmdResult = g_SpriteCommandHandler(*cmdPtr);
            commandCount += cmdResult;
            ++cmdPtr;
          }
          while ( cmdPtr != groupEndPtr );
          return commandCount + 1;
        case 1u:
          skipCount = (HIWORD(g_RandomSeed) * (unsigned int)(unsigned __int16)groupHeader) >> 16;// Type 1: Random skip - pick random entry count, execute that many
          g_RandomSeed *= 1103515245;
          g_RandomSeed += 12345;
          do
          {
            if ( skipCount-- )
            {
              cmdResult2 = g_SpriteCommandHandler(*cmdPtr);
              commandCount += cmdResult2;
            }
            ++cmdPtr;
          }
          while ( cmdPtr != groupEndPtr );
          goto LABEL_70;
        case 2u:
          pickIndex = (HIWORD(g_RandomSeed) * (unsigned int)(unsigned __int16)groupHeader) >> 16;// Type 2: Random pick - pick one random entry by index
          g_RandomSeed *= 1103515245;
          g_RandomSeed += 12345;
          pickCounter = pickIndex;
          pickDecrement = pickIndex - 1;
          if ( pickCounter )
          {
            do
              ++cmdPtr;
            while ( pickDecrement-- );
          }
          goto LABEL_82;
        case 3u:
          do
          {
            coinFlip = (2 * (unsigned int)HIWORD(g_RandomSeed)) >> 16;// Type 3: Coin flip per entry - 50% chance to execute each
            g_RandomSeed *= 1103515245;
            g_RandomSeed += 12345;
            if ( coinFlip )
            {
              cmdResult3 = g_SpriteCommandHandler(*cmdPtr);
              commandCount += cmdResult3;
            }
            ++cmdPtr;
          }
          while ( cmdPtr != groupEndPtr );
          goto LABEL_70;
        default:
          ++choiceGroupIdx;                     // Type 4+: Choice-based selection using g_RandomChoiceArray from pattern matching phase
          choiceKey = (unsigned __int16)g_RandomChoiceArray[(rangeArray[2 * choiceGroupIdx - 1]
                                                           + (unsigned int)HIWORD(g_RandomSeed)
                                                           * (rangeArray[2 * choiceGroupIdx]
                                                            - rangeArray[2 * choiceGroupIdx - 1])) >> 16] << 16;
          g_RandomSeed *= 1103515245;
          g_RandomSeed += 12345;
          break;
      }
      while ( (*cmdPtr & 0xFFFF0000) != choiceKey )
      {
        if ( ++cmdPtr == groupEndPtr )
          goto LABEL_70;
      }
LABEL_82:
      cmdResult4 = g_SpriteCommandHandler(*cmdPtr);
      commandCount += cmdResult4;
LABEL_70:
      if ( (groupHeader & 0x10000) != 0 )
        break;
      cmdPtr = groupEndPtr + 1;
      groupHeader = *groupEndPtr;
    }
  }
  return commandCount + 1;
}

int __stdcall AddDirtyRegion(int a1, int a2, int a3, int a4)
{
  _DWORD *v4; // esi
  int result; // eax
  _DWORD *v6; // ebx
  _DWORD *v7; // edi
  _DWORD *v8; // esi
  int v9; // ecx
  int v10; // ecx
  int v11; // ecx
  _DWORD *v12; // ecx
  _DWORD *v13; // esi

  v4 = (_DWORD *)g_DirtyRegionsList;
  if ( !g_DirtyRegionsList )
  {
    result = AllocFromMemoryPool(0x14u);
    v13 = (_DWORD *)result;
    g_DirtyRegionsList = result;
LABEL_45:
    *v13 = a4;
    v13[1] = a3;
    v13[2] = a2;
    v13[3] = a1;
    v13[4] = 0;
    return result;
  }
  if ( *(_DWORD *)g_DirtyRegionsList > a2
    || *(_DWORD *)(g_DirtyRegionsList + 4) > a1
    || *(_DWORD *)(g_DirtyRegionsList + 8) < a4
    || *(_DWORD *)(g_DirtyRegionsList + 12) < a3 )
  {
    while ( 1 )
    {
      v12 = (_DWORD *)v4[4];
      if ( !v12 )
        break;
      v4 = (_DWORD *)v4[4];
      if ( *v12 <= a2 && v4[1] <= a1 && v4[2] >= a4 && v4[3] >= a3 )
        goto LABEL_6;
    }
    result = AllocFromMemoryPool(0x14u);
    v4[4] = result;
    v13 = (_DWORD *)result;
    goto LABEL_45;
  }
LABEL_6:
  result = 0;
  if ( *v4 > a4 )
  {
    *v4 = a4;
    result = 1;
  }
  if ( v4[2] < a2 )
  {
    v4[2] = a2;
    ++result;
  }
  if ( v4[1] > a3 )
  {
    v4[1] = a3;
    ++result;
  }
  if ( v4[3] < a1 )
  {
    v4[3] = a1;
    ++result;
  }
  if ( result )
  {
    while ( 1 )
    {
      v6 = (_DWORD *)g_DirtyRegionsList;
      v7 = 0;
      while ( 1 )
      {
        v8 = (_DWORD *)v6[4];
        if ( v8 )
          break;
LABEL_35:
        v7 = v6;
        v6 = (_DWORD *)v6[4];
        if ( !v6 )
          return result;
      }
      result = (int)(v6 + 2);
      while ( *v8 > *(_DWORD *)result || v8[1] > v6[3] || v8[2] < *v6 || v8[3] < v6[1] )
      {
        v8 = (_DWORD *)v8[4];
        if ( !v8 )
          goto LABEL_35;
      }
      if ( *v6 < *v8 )
        *v8 = *v6;
      v9 = v6[2];
      if ( v9 > v8[2] )
        v8[2] = v9;
      v10 = v6[1];
      if ( v10 < v8[1] )
        v8[1] = v10;
      v11 = v6[3];
      if ( v11 > v8[3] )
        v8[3] = v11;
      if ( v7 )
        v7[4] = v6[4];
      else
        g_DirtyRegionsList = v6[4];
      result = (int)FreeMemoryBlock((int)v6);
    }
  }
  return result;
}

int __stdcall UpdateRenderObjectBounds(RenderObject *a1)
{
  int result; // eax

  g_RenderObjectsEnd = a1;
  result = ((int)a1 - g_MemoryPoolCurrent) / 4 - 4;
  *(_DWORD *)g_MemoryPoolCurrent = result;
  g_MemoryPoolAvailable = result;
  return result;
}

int __stdcall CalculateObjectPosition(RenderObject *object)
{
  __int16 *spriteDataPtr; // ebp
  int v2; // eax
  int parentIndex; // edx
  int v4; // ebx
  int v5; // ecx
  int v6; // eax
  int v7; // ecx
  int result; // eax
  unsigned __int16 *v9; // [esp+10h] [ebp-18h]
  int v10; // [esp+14h] [ebp-14h]
  __int16 v11; // [esp+18h] [ebp-10h]
  int animLink_low; // [esp+20h] [ebp-8h]
  int animLink_high; // [esp+24h] [ebp-4h]

  spriteDataPtr = (__int16 *)object->spriteDataPtr;
  v9 = *(unsigned __int16 **)spriteDataPtr;
  animLink_low = LOBYTE(object->animLink);
  v10 = g_TileWidth * animLink_low;
  animLink_high = HIBYTE(object->animLink);
  v2 = g_TileHeight * animLink_high;
  v11 = g_TileHeight * animLink_high;
  parentIndex = object->parentIndex;
  if ( parentIndex != 0xFFFF )
  {
    v2 = (int)&g_RenderObjectsStart[-parentIndex];
    if ( (*(_BYTE *)v2 & 4) != 0 )
    {
      v4 = object->slotIndex + 1;
      v10 += v4 * g_TileWidth * (*(unsigned __int8 *)(v2 + 6) - animLink_low) / LOBYTE(object->renderInfo);
      v2 = v4 * g_TileHeight * (*(unsigned __int8 *)(v2 + 7) - animLink_high) / LOBYTE(object->renderInfo);
      v11 += v2;
    }
  }
  LOWORD(v2) = v10;
  if ( (object->flags & 8) != 0 )
    v5 = *v9 - g_TileWidth - spriteDataPtr[2];
  else
    v5 = spriteDataPtr[2];
  v6 = v2 - v5;
  object->posX = v6;
  LOWORD(v6) = v11;
  if ( (object->flags & 0x10) != 0 )
    v7 = v9[1] - g_TileHeight - spriteDataPtr[3];
  else
    v7 = spriteDataPtr[3];
  result = v6 - v7;
  object->posY = result;
  return result;
}

int __stdcall AddObjectToDirtyRegions(RenderObject *object)
{
  __int16 *v1; // eax
  int prevLink; // esi
  int nextLink; // edi

  v1 = *(__int16 **)&object->spriteDataPtr;
  prevLink = (__int16)object->posX;
  nextLink = (__int16)object->posY;
  if ( (object->flags & 0x40) != 0 )
  {
    prevLink -= v1[2];
    nextLink -= v1[3];
  }
  return AddDirtyRegion(
           nextLink + *(unsigned __int16 *)(*(_DWORD *)v1 + 2),
           prevLink + **(unsigned __int16 **)v1,
           nextLink,
           prevLink);
}

int __stdcall AddObjectToDirtyRegionByIndex(int a1)
{
  return AddObjectToDirtyRegions((int)&g_RenderObjectsStart[-a1]);
}

int __stdcall SetObjectSpriteByDirection(char a1, RenderObject *object, int a3)
{
  int v3; // esi

  object->flags &= 0xE7u;
  object->slotIndex = 0;
  if ( *(_BYTE *)(a3 + 2) != 1 )
  {
    switch ( g_CurrentDirection )
    {
      case 16:
        v3 = 1;
        if ( g_CurrentTileY_Wrapped != g_PreviousTileY || (*(_BYTE *)(a3 + 1) & 8) == 0 )
          goto LABEL_42;
        break;
      case 32:
        v3 = 3;
        if ( g_CurrentTileY_Wrapped > g_PreviousTileY )
        {
          v3 = 2;
          goto LABEL_42;
        }
        if ( g_CurrentTileY_Wrapped < g_PreviousTileY )
        {
          v3 = 4;
          if ( (*(_BYTE *)(a3 + 1) & 4) != 0 )
          {
            v3 = 2;
            object->flags |= 0x10u;
          }
          goto LABEL_42;
        }
        if ( (*(_BYTE *)(a3 + 1) & 8) == 0 || g_CurrentTileX_Wrapped != g_PreviousTileX )
        {
LABEL_42:
          object->spriteDataPtr = *(void **)(*(_DWORD *)(a3 + 4) + 4 * v3);
          goto LABEL_43;
        }
        break;
      case 64:
        v3 = 5;
        if ( g_CurrentTileY_Wrapped != g_PreviousTileY || (*(_BYTE *)(a3 + 1) & 8) == 0 )
        {
          if ( (*(_BYTE *)(a3 + 1) & 4) != 0 )
          {
            v3 = 1;
            object->flags |= 0x10u;
          }
          goto LABEL_42;
        }
        break;
      default:
        v3 = 7;
        if ( g_CurrentTileY_Wrapped == g_PreviousTileY )
        {
          if ( (*(_BYTE *)(a3 + 1) & 8) == 0 || g_CurrentTileX_Wrapped != g_PreviousTileX )
          {
            if ( (*(_BYTE *)(a3 + 1) & 2) != 0 )
            {
              v3 = 3;
              object->flags |= 8u;
            }
            goto LABEL_42;
          }
          break;
        }
        if ( g_CurrentTileY_Wrapped > g_PreviousTileY )
        {
          v3 = 8;
          if ( (*(_BYTE *)(a3 + 1) & 2) == 0 )
            goto LABEL_42;
LABEL_39:
          v3 = 2;
          object->flags |= 8u;
          goto LABEL_42;
        }
        v3 = 6;
        if ( (*(_BYTE *)(a3 + 1) & 4) == 0 )
        {
          if ( (*(_BYTE *)(a3 + 1) & 2) != 0 )
          {
            v3 = 4;
            object->flags |= 8u;
          }
          goto LABEL_42;
        }
        v3 = 8;
        object->flags |= 0x10u;
        if ( (*(_BYTE *)(a3 + 1) & 2) != 0 )
          goto LABEL_39;
        goto LABEL_42;
    }
    v3 = 0;
    goto LABEL_42;
  }
  object->spriteDataPtr = *(void **)(a3 + 4);
  if ( (unsigned __int8)g_CurrentDirection == 128 )
  {
    if ( (*(_BYTE *)(a3 + 1) & 2) != 0 )
      object->flags |= 8u;
  }
  else if ( g_CurrentDirection == 64 && (*(_BYTE *)(a3 + 1) & 4) != 0 )
  {
    object->flags |= 0x10u;
  }
LABEL_43:
  if ( a1 )
  {
    object->flags |= 0x40u;
  }
  else
  {
    LOBYTE(object->animLink) = g_PreviousTileX;
    HIBYTE(object->animLink) = g_PreviousTileY;
    CalculateObjectPosition(object);
  }
  return AddObjectToDirtyRegions(object);
}

FontSlot *__stdcall FindFontSlotByPointer(int a1)
{
  void *v1; // esi
  FontSlot *result; // eax
  void **p_fontData; // edx

  v1 = *(void **)(a1 + 12);
  result = &g_FontSlotArray[dword_416184 - 1];
  if ( v1 < result->fontData )
  {
    p_fontData = &result->fontData;
    do
    {
      p_fontData -= 2;
      --result;
    }
    while ( *p_fontData > v1 );
  }
  return result;
}

int __stdcall UpdateObjectPosition(int a1)
{
  int result; // eax
  int v2; // esi
  RenderObject *v3; // ebx
  int FontSlotByPointer; // edi

  result = a1;
  v2 = *(unsigned __int16 *)(a1 + 10);
  if ( v2 != 0xFFFF )
  {
    v3 = &g_RenderObjectsStart[-v2];
    FontSlotByPointer = FindFontSlotByPointer((int)v3);
    AddObjectToDirtyRegions((int)v3);
    if ( g_PreviousTileX != g_CurrentTileX_Wrapped || g_PreviousTileY != g_CurrentTileY_Wrapped )
      *(_BYTE *)a1 |= 4u;
    return SetObjectSpriteByDirection(0, v3, FontSlotByPointer);
  }
  return result;
}

int __stdcall CheckObjectHitTest(int y, int x, RenderObject *object)
{
  int posY; // edx
  RenderObject *v4; // ebx
  unsigned __int16 *v5; // edx
  int result; // al

  posY = object->posY;
  result = 0;
  if ( posY != 0xFFFF )
  {
    v4 = &g_RenderObjectsStart[-posY];
    if ( (__int16)v4->posX <= x && (__int16)v4->posY <= y )
    {
      v5 = *(unsigned __int16 **)v4->spriteDataPtr;
      if ( *v5 + (__int16)v4->posX >= x && v5[1] + (__int16)v4->posY >= y )
        return 1;
    }
  }
  return result;
}

void *__stdcall ClearBuffer(size_t Size, void *buffer)
{
  return memset(buffer, 0, Size);
}

BOOL AlwaysTrue()
{
  return 1;
}

#pragma pack(pop)
