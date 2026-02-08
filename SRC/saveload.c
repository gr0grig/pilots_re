#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- saveload module ---- */

_BYTE *__stdcall XorCryptData(int cryptOffset, int size, _BYTE *data)
{
  _BYTE *result; // eax

  do
  {
    ++cryptOffset;
    result = data++;
    *result ^= byte_410298[cryptOffset & 0x1F];
    --size;
  }
  while ( size );
  return result;
}

char __stdcall ReadCustomSaveData(int *data_desc)
{
  int v1; // ebx
  unsigned int *v2; // esi
  int v3; // edi
  int v4; // ebp
  _BYTE *data; // [esp+10h] [ebp-8h]

  if ( !data_desc )
    return 1;
  v1 = *data_desc;
  v2 = (unsigned int *)&data_desc[2 * *data_desc + 2];
  while ( 1 )
  {
    v2 -= 2;
    --v1;
    v3 = data_desc[2 * v1 + 1];
    data = (_BYTE *)(g_GlobalVarsPtr + v3);
    v4 = *v2;
    if ( !ReadFromFile(*v2, (void *)(g_GlobalVarsPtr + v3)) )
      break;
    XorCryptData(v3, v4, data);
    if ( !v1 )
      return 1;
  }
  return 0;
}

char __stdcall WriteCustomSaveData(int *data_desc)
{
  int v1; // ebp
  int v2; // ebx
  _BYTE *v3; // edi
  unsigned int v4; // esi
  int *v6; // [esp+14h] [ebp-4h]

  if ( !data_desc )
    return 1;
  v1 = *data_desc;
  v6 = &data_desc[2 * *data_desc + 2];
  while ( 1 )
  {
    v6 -= 2;
    --v1;
    v2 = data_desc[2 * v1 + 1];
    v3 = (_BYTE *)(g_GlobalVarsPtr + v2);
    v4 = *v6;
    XorCryptData(v2, *v6, (_BYTE *)(g_GlobalVarsPtr + v2));
    if ( !WriteToFile(v4, v3) )
      break;
    XorCryptData(v2, v4, v3);
    if ( !v1 )
      return 1;
  }
  return 0;
}

void *__stdcall ClearCustomSaveRegions(_DWORD *data_desc)
{                                               // Null descriptor: nothing to clear
size_t *entryPtr;
// ebx
void *ret;
// eax

  if ( data_desc )
  {
    entryPtr = &data_desc[2 * *data_desc + 2];  // Point to end of descriptor entries (past last size field)
    do
    {
      entryPtr -= 2;                            // Walk backwards: entryPtr[-1] = offset, entryPtr[0] = size
      ret = ClearBuffer(*entryPtr, (void *)(g_GlobalVarsPtr + *(entryPtr - 1)));// ClearBuffer(size, g_GlobalVarsPtr + offset) — zero this region
    }
    while ( data_desc + 2 != entryPtr );
  }
  return ret;
}

BOOL ReadSaveHeader()
{
  return ReadFromFile(0x34u, &unk_4162A0);
}

int WriteSaveHeader()
{
  WriteToFile(8u, &g_SaveMagic1);
  WriteToFile(0x34u, &unk_4162A0);
  WriteCustomSaveData(g_CustomDataPtr1);
  return WriteCustomSaveData(g_CustomDataPtr2);
}

BOOL __stdcall WriteDword(unsigned int value)
{
  return WriteToFile(4u, &value);
}

int __stdcall FixupObjectPointers(int base_adjust, int object_count, char *object_data)
{
  int result; // eax
  int i; // ecx

  result = object_count;
  for ( i = object_count - 1; result; --i )
  {
    if ( (*object_data & 2) == 0 )
      *((_DWORD *)object_data + 3) += base_adjust;
    object_data += 16;
    result = i;
  }
  return result;
}

char __stdcall SaveGameState(int save_slot)
{
  DBG_ENTER("SaveGameState");
  _DWORD DstBuf[2]; // [esp+4h] [ebp-8h] BYREF

  if ( !(unsigned __int8)OpenSaveFile(save_slot, (int)&unk_4162A0, g_DataFilePath) )
  { DBG_LOG("SaveGameState: OpenSaveFile FAILED"); return 0; }
  DBG_LOG("SaveGameState: g_FirstRun=%d", g_FirstRun);
  while ( g_FirstRun )
  {
    DBG_LOG("SaveGameState: reading 8-byte magic header");
    if ( ReadFromFile(8u, DstBuf) && DstBuf[0] == 1028871741 && DstBuf[1] == -2147483626 )
    {
      ReadSaveHeader();
      *(_WORD *)((char *)&g_GraphicsConfigPtr + 1) = *(_WORD *)&g_PreviousScreen;
      offset = TellSaveFile();
      ReadCustomSaveData(g_CustomDataPtr1);
      ReadCustomSaveData(g_CustomDataPtr2);
      ClearCustomSaveRegions(g_CustomDataPtr2);
      g_FirstRun = 0;
      goto LABEL_11;
    }
    DBG_LOG("SaveGameState: magic mismatch or read fail, truncating");
    TruncateFile();
    CloseSaveFile();
    ValidateGameData(save_slot);
    if ( !(unsigned __int8)OpenSaveFile(save_slot, (int)&unk_4162A0, g_DataFilePath) )
    { DBG_LOG("SaveGameState: re-open FAILED"); return 0; }
  }
  DBG_LOG("SaveGameState: seeking to offset=%d", offset);
  SeekSaveFile(offset);
  ReadCustomSaveData(g_CustomDataPtr1);
  ReadCustomSaveData(g_CustomDataPtr2);
LABEL_11:
  CloseSaveFile();
  DBG_LOG("SaveGameState: SUCCESS");
  return 1;
}

char __stdcall ValidateGameData(int a1)
{
  if ( !(unsigned __int8)OpenSaveFile(a1, (int)&unk_4162A0, g_DataFilePath) )
    return 0;
  if ( g_FirstRun )
  {
    dword_4162B0 = 0;
    WriteSaveHeader();
  }
  else
  {
    SeekSaveFile(offset);
    WriteCustomSaveData(g_CustomDataPtr1);
    WriteCustomSaveData(g_CustomDataPtr2);
  }
  CloseSaveFile();
  return 1;
}

int __stdcall CheckSaveFileExists(int a1)
{
  return DeleteSaveFile(a1, (int)&g_SaveMagic2, g_DataFilePath);
}

int __stdcall LoadGameFromFile(int save_slot)
{                                               // Open save file for reading
  DBG_ENTER("LoadGameFromFile");
int v1;
// esi
char saved_sound_flag;
// bl
int snapshot_idx;
// ebp
ScreenSnapshot *snapshot;
// edi
unsigned int total_data_size;
// ebx
char *alloc_ptr;
// eax
char *obj_data_ptr;
// esi
unsigned int sprite_data_size;
// ebx
void *sprite_alloc;
// eax
int saved_music_index;
// [esp+10h] [ebp-1Ch]
int saved_font_index;
// [esp+14h] [ebp-18h]
_DWORD magic_header[2];
// [esp+18h] [ebp-14h] BYREF
char saved_error_flag;
// [esp+20h] [ebp-Ch]
char saved_video_flag;
// [esp+21h] [ebp-Bh]
int read_buf;
// [esp+24h] [ebp-8h] BYREF
char *pixel_data_base;
// [esp+28h] [ebp-4h]

  if ( !(unsigned __int8)OpenSaveFile(save_slot, (int)&g_SaveMagic2, g_DataFilePath) )
    return 0;
  if ( ReadFromFile(8u, magic_header) && magic_header[0] == 1028871741 && magic_header[1] == -2147483626 )// Validate 8-byte magic: '=VS=' (0x3D53563D) + version 0x80000016
  {
saved_sound_flag = g_SoundEnabledFlag;
// Preserve current system flags before overwriting with save data
    saved_error_flag = g_ErrorFlag;
    saved_video_flag = video_enabled_flag;
ReadSaveHeader();
// Read save state header (0x34 bytes) into global state area
g_SoundEnabledFlag = saved_sound_flag;
// Restore preserved system flags (sound, error, video) after header load
    g_ErrorFlag = saved_error_flag;
    video_enabled_flag = saved_video_flag;
*(_WORD *)((char *)&g_GraphicsConfigPtr + 1) = (unsigned __int8)g_PreviousScreen;
// Restore screen/graphics config from save data
SetMusicVolume(0);
// Mute music during load
saved_music_index = g_CurrentMusicIndex;
// Save current music and font indices to restore after game re-init
    saved_font_index = g_CurrentFontIndex;
g_FirstRun = 1;
// Set first run + demo mode flags to re-initialize game engine
    g_DemoMode = 1;
    g_GameStateFlag = 0;
GameMainLoop(v1, g_SavedScriptPtr);
// Re-initialize game engine with saved script pointer
ReadCustomSaveData(g_CustomDataPtr1);
// Read XOR-encrypted custom variable data blocks from save file
    ReadCustomSaveData(g_CustomDataPtr2);
    ReadCustomSaveData(g_CustomDataPtr3);
snapshot_idx = 0;
// Load all screen snapshots from save file
    while ( 1 )
    {
      ReadFromFile(4u, &read_buf);              // Read snapshot index from file
      snapshot = &g_ScreenSnapshotArray[read_buf];
      ReadFromFile(4u, &read_buf);              // Read object count for this snapshot
      snapshot->objectCount = read_buf;
      total_data_size = 16 * read_buf + 2 * g_ScreenPixelCount;// Total size = pixel data (2 * pixel_count) + object records (16 bytes each)
      alloc_ptr = (char *)AllocFromMemoryPool(total_data_size);// Allocate memory for snapshot pixel + object data
      snapshot->decompressedData = alloc_ptr;
      pixel_data_base = alloc_ptr;
      ReadFromFile(total_data_size, alloc_ptr); // Read all pixel and object data from file
      obj_data_ptr = &pixel_data_base[2 * g_ScreenPixelCount];// Object data starts after pixel data in the buffer
      FixupObjectPointers(g_MemoryPool.poolBasePtr, read_buf, obj_data_ptr);// Fix up serialized object pointers by adding memory pool base
      for ( ; read_buf--; obj_data_ptr += 16 )
      {                                         // If object has sprite flag (bit 1) and custom data flag (bit 3), read extra sprite data
        if ( (*obj_data_ptr & 2) != 0 && (*obj_data_ptr & 8) != 0 )
        {
          sprite_data_size = HIBYTE(g_SpriteSlotArray[(unsigned __int8)obj_data_ptr[1]].baseData);
          sprite_alloc = (void *)AllocFromMemoryPool(sprite_data_size);
          *((_DWORD *)obj_data_ptr + 3) = sprite_alloc;
          ReadFromFile(sprite_data_size, sprite_alloc);
        }
      }
      if ( ++snapshot_idx >= (unsigned int)g_TotalSnapshotCount )
      {
        g_FirstRun = 0;                         // All snapshots loaded: clear init flags and resume game
        g_DemoMode = 0;
        ExecuteGameLoop(0, g_snapshotIndex);    // Execute game loop to restore to saved snapshot state
        CloseSaveFile();
        BYTE1(g_GameLoopDataPtr->flags_or_id) = HIBYTE(g_GraphicsConfigPtr);// Restore graphics and frame rate from save data
        *(_DWORD *)&g_GameLoopDataPtr->frame_rate = g_SavedFrameRate;
        if ( saved_music_index >= 0 )           // Restore music track if one was playing
          SwitchMusicTrack(saved_music_index);
        BYTE2(g_GraphicsConfigPtr) = 0;
        if ( saved_font_index >= 0 )            // Restore font/graphics mode if one was set
          InitGraphicsMode(0, saved_font_index);
        g_GameStateFlag = 1;                    // Mark game state as active
        LongjmpRestart();                       // Longjmp back to main game loop (never returns)
      }
    }
  }
TruncateFile();
// Invalid save file: truncate to 0 and close
  CloseSaveFile();
  return 0;
}

char __stdcall SaveSnapshotsToFile(int a1)
{
  unsigned int v1; // edi
  ScreenSnapshot *v2; // ebp
  int objectCount; // esi
  char *v4; // ebx
  int v5; // ecx
  int v6; // esi
  char *Buf; // [esp+10h] [ebp-8h]
  int v10; // [esp+14h] [ebp-4h]

  if ( !(unsigned __int8)OpenSaveFile(a1, (int)&g_SaveMagic2, g_DataFilePath) )
    return 0;
  g_GameStateFlag = 0;
  CaptureScreenSnapshot(g_CurrentSnapshotIndex);
  HIBYTE(g_GraphicsConfigPtr) = *(_BYTE *)(g_GameLoopDataPtr + 1);
  g_SavedFrameRate = *(_DWORD *)(g_GameLoopDataPtr + 4);
  dword_4162B0 = g_TotalSnapshotCount;
  g_SavedScriptPtr = dword_4162D4;
  g_snapshotIndex = g_CurrentSnapshotIndex;
  WriteSaveHeader();
  WriteCustomSaveData(g_CustomDataPtr3);
  v1 = 0;
  v10 = 0;
  do
  {
    v2 = &g_ScreenSnapshotArray[v10];
    if ( g_ScreenSnapshotArray[v10].compressedData )
    {
      objectCount = v2->objectCount;
      Buf = (char *)v2->decompressedData;
      v4 = &Buf[2 * g_ScreenPixelCount];
      WriteDword(v1);
      WriteDword(objectCount);
      FixupObjectPointers(-g_MemoryPool.poolBasePtr, objectCount, v4);
      WriteToFile(16 * objectCount + 2 * g_ScreenPixelCount, Buf);
      FixupObjectPointers(g_MemoryPool.poolBasePtr, objectCount, v4);
      v5 = objectCount;
      v6 = objectCount - 1;
      if ( v5 )
      {
        do
        {
          if ( (*v4 & 2) != 0 && (*v4 & 8) != 0 )
            WriteToFile(HIBYTE(g_SpriteSlotArray[(unsigned __int8)v4[1]].baseData), *((void **)v4 + 3));
          v4 += 16;
        }
        while ( v6-- );
      }
    }
    ++v10;
    ++v1;
  }
  while ( v1 < dword_4161E4 );
  CloseSaveFile();
  ExecuteGameLoop(1, g_CurrentSnapshotIndex);
  g_GameStateFlag = 1;
  return 1;
}

BOOL CheckGameState()
{
  if ( video_queue_active && g_DisplayMode[1] == 1 )
    return ExecuteVideoPlayback(0);
  else
    return 1;
}

char ResetGameState()
{
  char result; // al

  if ( byte_411710 )
    result = StopMidiMusic();
  g_CurrentMusicIndex = -1;
  byte_411710 = 0;
  return result;
}

int __stdcall sub_408870(int a1, int a2, int a3)
{
  int result; // eax

  result = a2;
  switch ( a2 )
  {
    case 0:
      return (char)CheckSaveFileExists(a3);
    case 1:
      return SaveSnapshotsToFile(a3);
    case 2:
      return (char)LoadGameFromFile(a3);
  }
  return result;
}

#pragma pack(pop)
