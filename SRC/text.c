#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- text module ---- */

int __stdcall RenderOverlayWithContext(int **a1)
{
  int v1; // ebx
  int v2; // esi
  int v3; // edi
  int result; // eax
  int v5; // [esp+Ch] [ebp-2Ch]
  int v6; // [esp+10h] [ebp-28h]
  int v7; // [esp+14h] [ebp-24h]
  int v8; // [esp+18h] [ebp-20h]
  int v9; // [esp+1Ch] [ebp-1Ch]
  int v10; // [esp+20h] [ebp-18h]
  char v11; // [esp+24h] [ebp-14h]
  int (__stdcall *v12)(_DWORD); // [esp+28h] [ebp-10h]
  int (__stdcall *v13)(_DWORD); // [esp+2Ch] [ebp-Ch]
  int v14; // [esp+30h] [ebp-8h]
  unsigned __int8 v15; // [esp+34h] [ebp-4h]

  v5 = g_GameFlags1;
  v6 = g_ResourceID2;
  v7 = g_GameFlags2;
  v1 = g_SavedState;
  v2 = g_ResourceID1;
  v3 = g_PreviousTileX;
  v8 = g_PreviousTileY;
  v9 = g_CurrentTileX_Wrapped;
  v10 = g_CurrentTileY_Wrapped;
  v11 = g_CurrentDirection;
  v12 = g_TileProcessor;
  v13 = g_SpriteCommandHandler;
  v14 = g_CurrentObjectData;
  v15 = *((_BYTE*)&g_PreviousScreen + 2);
  result = RenderSprite(1, *a1);
  *((_BYTE*)&g_PreviousScreen + 2) = v15;
  g_CurrentObjectData = v14;
  g_SpriteCommandHandler = v13;
  g_TileProcessor = v12;
  g_CurrentDirection = v11;
  g_CurrentTileY_Wrapped = v10;
  g_CurrentTileX_Wrapped = v9;
  g_PreviousTileY = v8;
  g_PreviousTileX = v3;
  g_ResourceID1 = v2;
  g_SavedState = v1;
  g_GameFlags2 = v7;
  g_ResourceID2 = v6;
  g_GameFlags1 = v5;
  return result;
}

int __stdcall RenderOverlaySlot(int a1)
{
  return RenderOverlayWithContext(&g_OverlaySlotArray[a1].spriteDataPtr);
}

int __stdcall CreateFontRenderObject(char is_overlay, unsigned __int16 posY, unsigned __int16 posX, int font_index)
{
  FontSlot *fontSlot; // esi
  RenderObject *newObj; // eax
  RenderObject *renderObj; // ebx
  __int16 parentIdx; // cx
  int objOffset; // ebp
  int primarySlotIdx; // edi
  int secondarySlotIdx; // edx
  int v12; // [esp+10h] [ebp-8h]
  RenderObject *v13; // [esp+14h] [ebp-4h]

  fontSlot = &g_FontSlotArray[font_index];      // Look up the font slot by index
  newObj = AllocRenderObject();                 // Allocate a new render object from the pool
  renderObj = newObj;
  newObj->flags |= 0x80u;                       // Set bit 7 (0x80): marks object as active/visible
  if ( (fontSlot->baseCharOffset & 1) != 0 )    // If font has mirrored flag (bit 0 of baseCharOffset), set flip flag (0x04)
    newObj->flags |= 4u;
  LOBYTE(newObj->renderInfo) = fontSlot->attributes;// Copy font attributes into renderInfo low byte
  parentIdx = -1;                               // parentIndex: -1 for overlay objects, g_ResourceID2 for tile objects
  if ( !is_overlay )
    parentIdx = g_ResourceID2;
  newObj->parentIndex = parentIdx;
  newObj->posX = posX;                          // Set object position from parameters
  newObj->posY = posY;
  if ( is_overlay )                             // Overlay path: set animLink fields to -1 (no animation link)
  {
    HIBYTE(newObj->animLink) = -1;
    LOBYTE(newObj->animLink) = -1;
    HIBYTE(newObj->renderInfo) = *(_BYTE *)(g_GameFlags1 + 5);
  }
  else if ( g_GameFlags1 )                      // Tile path with active game flags: manage primary/secondary render slots
  {
    objOffset = g_RenderObjectsStart - newObj;  // Compute offset of new object relative to render objects base
    primarySlotIdx = *(unsigned __int16 *)(g_GameFlags1 + 8);// Read primary slot index (offset +8) and secondary slot index (offset +10)
    secondarySlotIdx = *(unsigned __int16 *)(g_GameFlags1 + 10);
    v12 = secondarySlotIdx;
    if ( secondarySlotIdx != 0xFFFF )           // If secondary slot is valid (not 0xFFFF), recycle the old object
    {
      v13 = &g_RenderObjectsStart[-secondarySlotIdx];
      AddObjectToDirtyRegions(v13);             // Mark old secondary object as dirty for re-rendering
      v13->flags &= ~0x80u;                     // Clear active flag on old secondary object
      if ( v12 != primarySlotIdx )              // Free old secondary if it differs from primary (avoid double-free)
        FreeRenderObject(v13);
    }
    *(_WORD *)(g_GameFlags1 + 10) = objOffset;  // Store new object offset as the new secondary slot
    if ( (renderObj->flags & 4) == 0 )          // If no flip flag, also update the primary slot
    {                                           // Free old primary object if valid
      if ( primarySlotIdx != 0xFFFF )
        FreeRenderObject(&g_RenderObjectsStart[-primarySlotIdx]);
      *(_WORD *)(g_GameFlags1 + 8) = objOffset; // Store new object as primary slot
    }
    HIBYTE(renderObj->renderInfo) = *(_BYTE *)(g_GameFlags1 + 5);// Copy render layer from game flags byte 5 into renderInfo high byte
    if ( g_CurrentTileX_Wrapped != g_PreviousTileX || g_CurrentTileY_Wrapped != g_PreviousTileY )// If tile position changed since last frame, set movement dirty flag (bit 2) on game flags
      *(_BYTE *)g_GameFlags1 |= 4u;
  }
  else
  {
    HIBYTE(newObj->renderInfo) = *(_BYTE *)(g_GameFlags2 + 2);// No game flags: use g_GameFlags2 byte 2 as render layer
  }
  DBG_LOG("CreateFontRenderObject: font_index=%d fontSlot=%p fontData=%p type=%d flags=%d", font_index, fontSlot, fontSlot->fontData, fontSlot->type, fontSlot->flags);
  if ( !fontSlot->fontData ) {
    DBG_LOG("ERROR: fontSlot->fontData is NULL for font_index=%d! g_FontSlotArray=%p", font_index, g_FontSlotArray);
  }
  SetObjectSpriteByDirection(is_overlay, renderObj, (int)fontSlot);// Configure sprite frame based on current direction and font slot data
  return 1;
}

int __stdcall RenderOverlayWithCoords(int a1)
{
  OverlaySlot *v1; // esi
  OverlayCoords *coords; // ebx

  v1 = &g_OverlaySlotArray[a1];
  coords = v1->coords;
  if ( coords )
  {
    CreateFontRenderObject(1, coords->y, coords->x, coords->index);
    while ( !coords->reserved[1] )
    {
      coords = (OverlayCoords *)((char *)coords + 8);
      CreateFontRenderObject(1, coords->y, coords->x, coords->index);
    }
  }
  return RenderOverlayWithContext(&v1->spriteDataPtr);
}

RenderObject *__stdcall RenderTextCharacter(uint16_t render_y, uint16_t *render_x_ptr, int font_index, int character)
{
  FontSlot *font_slot; // esi
  RenderObject *char_render_object; // edi
  unsigned __int16 **char_sprite_data; // ebx

  font_slot = &g_FontSlotArray[font_index];
  char_render_object = AllocRenderObject();
  char_render_object->flags |= 0xE0u;
  char_render_object->slotIndex = 0;
  LOBYTE(char_render_object->renderInfo) = 1;
  HIBYTE(char_render_object->renderInfo) = *(_BYTE *)(g_GameFlags1 + 5);
  char_render_object->animLink = -1;
  char_render_object->parentIndex = g_ResourceID2;
  char_render_object->posX = *render_x_ptr;
  char_render_object->posY = render_y;
  char_sprite_data = (unsigned __int16 **)((char *)font_slot->fontData + 8 * (character - font_slot->baseCharOffset));
  char_render_object->spriteDataPtr = char_sprite_data;
  *(_DWORD *)render_x_ptr += **char_sprite_data - *((__int16 *)char_sprite_data + 2);
  AddObjectToDirtyRegions(char_render_object);
  return char_render_object;
}

int __stdcall PrepareTextDisplay(int a1)
{
  HideTextInput();
  *(_BYTE *)(g_GameFlags1 + 3) |= 2u;
  return a1;
}

int __stdcall DeleteTextCharacters(int char_count)
{
  int char_object; // ebx
  int result; // eax

  do
  {
    char_object = g_TextCharacterObjects[--g_TextInputLength];
    g_TextRenderX -= ***(unsigned __int16 ***)(char_object + 12) - *(__int16 *)(*(_DWORD *)(char_object + 12) + 4);
    result = RemoveAndFreeRenderObject((RenderObject *)char_object);
    --char_count;
  }
  while ( char_count );
  return result;
}

char __stdcall ProcessTextInput(unsigned __int8 keyCode)
{
  unsigned __int8 *offset_ptr; // ecx
  RenderObject *char_object; // eax
  int char_index; // ecx

  g_GameFlags1 = g_SavedGameFlags1;
  g_ResourceID2 = g_SavedResourceID2;
  g_GameFlags2 = g_SavedGameFlags2;
  while ( 1 )
  {
    while ( 1 )
    {
      while ( 1 )
      {
        if ( keyCode == 8 )
        {
          if ( g_TextInputActive )
          {
            if ( g_TextInputLength )
              DeleteTextCharacters(1);
            return 1;
          }
          goto LABEL_37;
        }
        if ( keyCode != 27 )
          break;
        if ( !g_TextInputActive )
          goto LABEL_37;
        if ( g_TextInputLength )
        {
          DeleteTextCharacters(g_TextInputLength);
          return 1;
        }
LABEL_14:
        if ( *((_BYTE*)&video_recursion_counter + 2) )
        {
          *((_BYTE*)&video_recursion_counter + 2) = 0;
          return HideTextInput();
        }
        *((_BYTE*)&video_recursion_counter + 2) = 1;
        g_TextInputActive = 0;
        keyCode = *(_BYTE *)g_TextCommandPtr++;
      }
      if ( !keyCode )
        goto LABEL_14;
      if ( keyCode != 13 )
        break;
      if ( !g_TextInputActive )
        goto LABEL_37;
      g_TextInputBuffer[g_TextInputLength] = 0;
      switch ( g_TextInputMode & 7 )
      {
        case 0:
          *g_TextInputTargetPtr = atoi(g_TextInputBuffer);
          break;
        case 1:
          strcpy(*(char **)g_TextInputTargetPtr, g_TextInputBuffer);
          break;
        case 2:
          strcpy(g_TextInputTargetPtr, g_TextInputBuffer);
          break;
        case 3:
          *(_DWORD *)g_TextInputTargetPtr = atoi(g_TextInputBuffer);
          break;
        case 4:
          **(_DWORD **)g_TextInputTargetPtr = atoi(g_TextInputBuffer);
          break;
        default:
          break;
      }
      g_TextInputActive = 0;
      keyCode = *(_BYTE *)g_TextCommandPtr++;
    }
    if ( keyCode == 15 )
    {
      g_TextInputMode = *(_BYTE *)g_TextCommandPtr++;
      g_TextInputMaxLength = *(unsigned __int8 *)g_TextCommandPtr++;
      if ( (g_TextInputMode & 0x70) == 0 )
      {
        g_TextInputTargetPtr = (char *)g_GlobalVarsPtr;
        goto LABEL_30;
      }
      if ( (g_TextInputMode & 0x70) == 0x10 )
        break;
    }
LABEL_37:
    if ( g_TextInputActive )
    {
      if ( g_TextInputLength < (unsigned int)g_TextInputMaxLength )
      {
        if ( *(_BYTE *)(g_TextCharRangePtr + 1) > keyCode )
          keyCode = tolower(keyCode);
        if ( *(_BYTE *)(g_TextCharRangePtr + 2) < keyCode )
          keyCode = toupper(keyCode);
        if ( *(_BYTE *)(g_TextCharRangePtr + 1) <= keyCode
          && *(_BYTE *)(g_TextCharRangePtr + 2) >= keyCode
          && (!g_TextInputNumericMode || isdigit(keyCode) != 0) )
        {
          g_TextInputBuffer[g_TextInputLength] = keyCode;
          char_object = RenderTextCharacter(g_TextRenderY, (uint16_t *)&g_TextRenderX, g_TextFontIndex, keyCode);
          char_index = g_TextInputLength++;
          g_TextCharacterObjects[char_index] = (int)char_object;
        }
      }
      return 1;
    }
    RenderTextCharacter(g_TextRenderY, (uint16_t *)&g_TextRenderX, g_TextFontIndex, keyCode);
    keyCode = *(_BYTE *)g_TextCommandPtr++;
  }
  g_TextInputTargetPtr = (char *)g_CurrentObjectData;
LABEL_30:
  if ( (signed char)g_TextInputMode >= 0 )
  {
    offset_ptr = (unsigned __int8 *)g_TextCommandPtr++;
    g_TextInputTargetPtr += *offset_ptr;
  }
  else
  {
    g_TextInputTargetPtr += *(_DWORD *)g_TextCommandPtr;
    g_TextCommandPtr += 4;
  }
  switch ( g_TextInputMode & 7 )
  {
    case 0:
    case 3:
    case 4:
      g_TextInputNumericMode = 1;
      break;
    case 1:
    case 2:
      g_TextInputNumericMode = 0;
      break;
    default:
      break;
  }
  g_TextInputLength = 0;
  g_TextInputActive = 1;
  return 1;
}

int __stdcall SetupTextDisplay(int a1, int a2, int a3, int a4)
{
  if ( *((_BYTE*)&video_recursion_counter + 2) )
    return 0;
  g_TextCommandPtr = PrepareTextDisplay(a4);
  g_TextRenderX = a2;
  g_TextRenderY = a1;
  g_TextFontIndex = a3;
  g_TextCharRangePtr = (int)&g_FontSlotArray[a3];
  g_SavedGameFlags1 = g_GameFlags1;
  g_SavedResourceID2 = g_ResourceID2;
  g_SavedGameFlags2 = g_GameFlags2;
  return ProcessTextInput(0);
}

int __stdcall ProcessOverlayText(int a1, int a2)
{
  return SetupTextDisplay(
           g_OverlaySlotArray[a2].coords->y,
           g_OverlaySlotArray[a2].coords->x,
           g_OverlaySlotArray[a2].coords->index,
           a1);
}

int __stdcall RenderFormattedText(uint16_t renderY, int renderX, int fontIndex, int textDataPtr)
{
  unsigned __int64 acc; // rax
  _DWORD *textPtr; // esi
  unsigned __int8 *fmtPtr; // esi
  int sourceType; // ecx
  unsigned __int8 *nativeFuncIdPtr; // edx
  int nativeResult; // eax
  int dataBase; // edi
  const char **varDataPtr; // edi
  int scriptEvalResult; // eax
  char *bufIter; // ebx
  char *curChar; // edx
  unsigned __int8 *literalCharPtr; // edx
  size_t maxLen; // [esp+Ch] [ebp-104h]
  char fmtBuffer; // [esp+10h] [ebp-100h] BYREF
  char *scriptVarOffset; // [esp+11h] [ebp-FFh]
  char scriptTerminator; // [esp+15h] [ebp-FBh]

  LODWORD(acc) = PrepareTextDisplay(textDataPtr);
  textPtr = (_DWORD *)acc;
  while ( *(_BYTE *)textPtr )
  {
    if ( *(_BYTE *)textPtr == 15 )
    {
      fmtPtr = (unsigned __int8 *)textPtr + 1;
      LOBYTE(acc) = *fmtPtr++;                  // fmtDescriptor: bits[6:4]=source, bit[7]=offset_size, bits[2:0]=data_type
      maxLen = *fmtPtr;
      textPtr = fmtPtr + 1;
      sourceType = acc & 0x70;
      if ( sourceType == 32 )
      {
        nativeFuncIdPtr = (unsigned __int8 *)textPtr;
        textPtr = (_DWORD *)((char *)textPtr + 1);
        nativeResult = g_NativeFuncTable[*nativeFuncIdPtr]();
        LODWORD(acc) = itoa(nativeResult, &fmtBuffer, 10);
      }
      else
      {
        if ( (acc & 0x70) == 0 )
        {
          dataBase = g_GlobalVarsPtr;
          goto LABEL_10;
        }
        if ( sourceType == 16 )
        {
          dataBase = g_CurrentObjectData;
LABEL_10:
          if ( (acc & 0x80u) == 0LL )           // bit7=0: 1-byte offset, bit7=1: 4-byte offset
          {
            HIDWORD(acc) = textPtr;
            textPtr = (_DWORD *)((char *)textPtr + 1);
            varDataPtr = (const char **)((unsigned __int8)*(_BYTE *)HIDWORD(acc) + dataBase);// dataType = descriptor & 7: 0=sbyte, 1=strptr, 2=inline_str, 3=dword_as_int, 4=deref_dword, 5=script_eval
          }
          else
          {
            varDataPtr = (const char **)(*textPtr++ + dataBase);
          }
          switch ( acc & (FMT_SCRIPT_EVAL|FMT_INLINE_STR) )
          {
            case FMT_SBYTE:
              LODWORD(acc) = itoa(*(char *)varDataPtr, &fmtBuffer, 0xA);
              break;
            case FMT_STRPTR:
              LODWORD(acc) = strncpy(&fmtBuffer, *varDataPtr, maxLen);
              break;
            case FMT_INLINE_STR:
              LODWORD(acc) = strncpy(&fmtBuffer, (const char *)varDataPtr, maxLen);
              break;
            case FMT_DWORD_INT:
              LODWORD(acc) = itoa((int)*varDataPtr, &fmtBuffer, 0xA);
              break;
            case FMT_DEREF_DWORD:
              LODWORD(acc) = itoa(*(_DWORD *)*varDataPtr, &fmtBuffer, 0xA);
              break;
            case FMT_SCRIPT_EVAL:
              fmtBuffer = 0x35;                 // Build inline script: [OP_CALL_INDIRECT(0x35)][varOffset][OP_RETURN(0x00)] to evaluate script function at varDataPtr
              scriptVarOffset = (char *)varDataPtr - g_GlobalVarsPtr;// scriptVarOffset = varDataPtr - g_GlobalVarsPtr (relative offset for OP_CALL_INDIRECT)
              scriptTerminator = 0;             // scriptTerminator = OP_RETURN (0x00)
              scriptEvalResult = EvaluateScriptBytecode(acc, (BYTE *)&fmtBuffer);
              LODWORD(acc) = itoa(scriptEvalResult, &fmtBuffer, 10);
              break;
            default:
              break;
          }
          *(&fmtBuffer + maxLen) = 0;           // Null-terminate formatted buffer at maxLen
          bufIter = &fmtBuffer;
          if ( fmtBuffer )
          {
            do
            {
              curChar = bufIter++;              // Render each character of formatted buffer
              LODWORD(acc) = RenderTextCharacter(renderY, (uint16_t *)&renderX, fontIndex, (unsigned __int8)*curChar);
            }
            while ( *bufIter );
          }
        }
      }
    }
    else
    {
      literalCharPtr = (unsigned __int8 *)textPtr;// literalCharPtr: pointer to plain text character to render
      textPtr = (_DWORD *)((char *)textPtr + 1);
      LODWORD(acc) = RenderTextCharacter(renderY, (uint16_t *)&renderX, fontIndex, *literalCharPtr);
    }
  }
  return 1;
}

int __stdcall sub_407618(int a1, int a2)
{
  return RenderFormattedText(g_TileHeight * g_CurrentTileY_Wrapped, g_TileWidth * g_CurrentTileX_Wrapped, a2, a1);
}

int __stdcall RenderOverlayText(int textDataPtr, int a2)
{
  return RenderFormattedText(
           g_OverlaySlotArray[a2].coords->y,
           g_OverlaySlotArray[a2].coords->x,
           g_OverlaySlotArray[a2].coords->index,
           textDataPtr);
}

int HideTextInput()
{
  RemoveChildTextObjects(g_GameFlags1);
  return 1;
}

#pragma pack(pop)
