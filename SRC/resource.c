#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- resource module ---- */

void *__stdcall InitFileReadBuffer(void *buffer)
{
  void *bufferSize; // eax

  g_BufferSize = 0;
  g_FileBufferSize = 0;
  bufferSize = buffer;
  g_BufferPtr = buffer;
  if ( buffer )
  {
    g_BufferSize = g_MaxReadBufferSize;
    return (void *)g_MaxReadBufferSize;
  }
  return bufferSize;
}

size_t GetCurrentFileOffset()
{
  return FileTell() - g_FileBufferSize;
}

BOOL __stdcall SeekFileWithBuffer(int Origin, int Offset)
{
  if ( Origin == 1 )
    Offset -= g_FileBufferSize;
  g_FileBufferSize = 0;
  return SeekFile(Origin, Offset);
}

int __stdcall BufferedRead(unsigned int maxBytes, char *DstBuf)
{
  size_t v5; // ebx

  do
  {
    if ( !g_FileBufferSize )
    {
      if ( maxBytes >= g_BufferSize || !g_BufferPtr )
        return ReadFileData(maxBytes, DstBuf);
      g_FileBufferSize = ReadFileRaw(g_MaxReadBufferSize, g_BufferPtr);
      g_BufferSize = g_FileBufferSize;
    }
    v5 = g_FileBufferSize;
    if ( maxBytes < g_FileBufferSize )
      v5 = maxBytes;
    memcpy(DstBuf, (char *)g_BufferPtr + g_BufferSize - g_FileBufferSize, v5);
    DstBuf += v5;
    g_FileBufferSize -= v5;
    maxBytes -= v5;
  }
  while ( maxBytes );
  return 1;
}

int ShowFileError()
{
  DBG_ENTER("ShowFileError");
  DBG_LOG("ShowFileError: caller=0x%p opcode=0x%X(byte=0x%02X)", _ReturnAddress(), g_CurrentOpcode, (unsigned char)g_CurrentOpcode);
  CloseFile();
  SetErrorMessage(aInvalidGameDat); return 0;
}

int ReadByte()
{
  g_CurrentOpcode = 0;
  return BufferedRead(1u, (char *)&g_CurrentOpcode);
}

char __stdcall LoadResourceById(unsigned int resourceId)
{
  /* Must be contiguous: BufferedRead(5, &resourceOffset) reads into both */
#pragma pack(push, 1)
  struct { int resourceOffset; char idByte; } _res_buf;
#pragma pack(pop)
#define resourceOffset _res_buf.resourceOffset
#define idByte _res_buf.idByte

  if ( resourceId <= g_MaxResourceIndex
    && SeekFileWithBuffer(2, -(g_IndexTableOffset + 5 * (g_MaxResourceIndex - resourceId) + 5))
    && BufferedRead(5u, (char *)&resourceOffset)
    && idByte == (_BYTE)resourceId
    && SeekFileWithBuffer(0, g_DataOffset + resourceOffset)
    && ReadByte() )
  {
    return 1;
  }
  else
  {
    return ShowFileError();
  }
#undef openFileStruct
#undef headerVersion
#undef headerMagic
#undef resourceCount
#undef dataOffset
}

char __stdcall DecodeVLQ(char *resultBuffer)
{
  unsigned int byteCount; // esi
  int _vlq_entry_opcode;

  _vlq_entry_opcode = (unsigned char)g_CurrentOpcode;
  if ( !ReadByte() )
    return ShowFileError();
  if ( (signed char)g_CurrentOpcode >= 0 )
  {
    *(_DWORD *)resultBuffer = (unsigned __int8)g_CurrentOpcode;
  }
  else
  {
    if ( (g_CurrentOpcode & 0x20) != 0 )
      byteCount = (((int)(unsigned __int8)g_CurrentOpcode >> 2) & 3) + 1;
    else
      byteCount = g_CurrentOpcode & 0x1F;
    *(_DWORD *)resultBuffer = 0;
    if ( !BufferedRead(byteCount, resultBuffer) )
      return ShowFileError();
    if ( (g_CurrentOpcode & 0x20) != 0 )
    {
      *(_DWORD *)resultBuffer += (g_CurrentOpcode & 3) * (1 << (8 * byteCount));
      if ( (g_CurrentOpcode & 0x10) != 0 )
        *(_DWORD *)resultBuffer = *(_DWORD *)&aInvalidGameDat[8 * byteCount + 20] - *(_DWORD *)resultBuffer;
      else
        *(_DWORD *)resultBuffer += dword_41020C[2 * byteCount];
    }
  }
  { int _vlq_result = *(_DWORD *)resultBuffer;
    if (_vlq_result < 0 || _vlq_result > 300) {
      DBG_LOG("DecodeVLQ: SUSPICIOUS result=%d (0x%X) entry_opcode=0x%02X first_byte=0x%02X", _vlq_result, (unsigned)_vlq_result, _vlq_entry_opcode, (unsigned char)g_CurrentOpcode);
    }
  }
  return ReadByte();
}

char __stdcall ReadJumpOffset(int initialValue, size_t *outPosition)
{
  int fileOffset; // [esp+0h] [ebp-4h] BYREF

  fileOffset = initialValue;
  if ( !ReadByte() )
    return ShowFileError();
  if ( (g_CurrentOpcode & 0xEC) != 0xEC )
    return ShowFileError();
  fileOffset = (g_CurrentOpcode & 3) + 1;
  if ( !BufferedRead(fileOffset, (char *)&fileOffset) )
    return ShowFileError();
  *outPosition = GetCurrentFileOffset();
  SeekFileWithBuffer(1, fileOffset);
  return ReadByte();
}

char __stdcall ReadCompressedData(char *destBuffer)
{
  char *compressedBuf; // esi
  unsigned int decompressedSize; // [esp+8h] [ebp-Ch] BYREF
  int compressedSize; // [esp+Ch] [ebp-8h] BYREF
  unsigned int sizeFieldBytes; // [esp+10h] [ebp-4h]

  g_DecompressedDataSize = 0;
  g_TemplateDataSize = 0;
  if ( !ReadByte() || (signed char)g_CurrentOpcode >= 0 )
    return ShowFileError();
  if ( (g_CurrentOpcode & 0x40) != 0 )
  {
    g_TemplateDataSize = g_CurrentOpcode & 0x1F;
    if ( (g_CurrentOpcode & 0x1F) == 0 || BufferedRead(g_CurrentOpcode & 0x1F, destBuffer) )
      return ReadByte();
  }
  else
  {
    decompressedSize = dword_41022C;
    compressedSize = dword_410230;
    sizeFieldBytes = (g_CurrentOpcode & 3) + 1;
    if ( BufferedRead(sizeFieldBytes, (char *)&decompressedSize) )
    {
      if ( (g_CurrentOpcode & 0xC) != 0 )
      {
        compressedBuf = (char *)AllocTempBuffer(decompressedSize);
        if ( !BufferedRead(sizeFieldBytes, (char *)&compressedSize) || !BufferedRead(decompressedSize, compressedBuf) )
          return ShowFileError();
        g_DecompressedDataSize = decompressedSize;
        LZSSDecompress(decompressedSize, destBuffer, compressedBuf);
        g_TemplateDataSize = compressedSize;
      }
      else
      {
        g_TemplateDataSize = decompressedSize;
        if ( !BufferedRead(decompressedSize, destBuffer) )
          return ShowFileError();
      }
      return ReadByte();
    }
  }
  return ShowFileError();
}

char __stdcall LoadDataFile(char *FileName)
{
  DBG_ENTER("LoadDataFile");
  void *headerBuffer; // eax
  void *indexBuffer; // eax
  /* Stack layout must match original binary: ClearBuffer writes 0xB8 bytes
     from &resourceCount upward. Original stack frame was 0xC4 bytes.
     Variables overlaid at original stack offsets. */
  char _stackBuf[0xC4];
#define openFileStruct  (*(unsigned int *)(_stackBuf + 0x00))
#define headerVersion   (*(UINT *)(_stackBuf + 0x04))
#define headerMagic     (*(int *)(_stackBuf + 0x08))
#define resourceCount   (*(unsigned int *)(_stackBuf + 0x0C))
#define dataOffset      (*(unsigned int *)(_stackBuf + 0x10))

  g_DemoMode = 0;
  InitFileReadBuffer(0);
  if ( (unsigned __int8)Game_OpenFile(FileName, (LPOFSTRUCT)openFileStruct, headerVersion)
    && SeekFileWithBuffer(2, -12)
    && (headerBuffer = ClearBuffer(0xCu, &openFileStruct), ReadFileData(0xCu, headerBuffer))
    && HIBYTE(headerVersion) == 4
    && headerMagic == 1028871741
    && (g_IndexTableOffset = openFileStruct + 12, SeekFileWithBuffer(2, -(openFileStruct + 12)))
    && (indexBuffer = ClearBuffer(0xB8u, &resourceCount), ReadFileData(openFileStruct, indexBuffer))
    && (g_DataOffset = dataOffset, dataOffset <= 0x13880)
    && (g_MaxResourceIndex = resourceCount, resourceCount <= 0xFE) )
  {
    return 1;
  }
  else
  {
    return ShowFileError();
  }
}

int __stdcall CaptureScreenSnapshot(int snapIdx)
{
  ScreenSnapshot *snapshot; // edi
  int objectType; // eax
  int destRowOffset; // ebx
  int decompressedData; // ecx
  RenderObject *parentObjAddr; // ebp
  RenderObject *childScanAddr; // ebp
  int i; // ebx
  RenderObject *currentObjPtr; // [esp+10h] [ebp-14h]
  RenderObject *objScanAddr; // [esp+10h] [ebp-14h]
  RenderObject *objDataPtr; // [esp+14h] [ebp-10h]
  RenderObject *objDataPtr2; // [esp+14h] [ebp-10h]
  unsigned __int16 savedIndex; // [esp+18h] [ebp-Ch]
  int objectFlags; // [esp+20h] [ebp-4h]
  int snapshotIndexa; // [esp+28h] [ebp+4h]
  int snapshotIndexb; // [esp+28h] [ebp+4h]

  snapshot = &g_ScreenSnapshotArray[snapIdx];// Get snapshot structure for given index
  snapshot->objectCount = 0;                    // Initialize: set objectCount to 0 before counting
  currentObjPtr = g_RenderObjectsStart;         // PHASE 1: COUNT OBJECTS - Start scanning from top of render objects array (grows downward in memory)
  if ( g_RenderObjectsStart > g_RenderObjectsEnd )
  {
    while ( 1 )
    {
      objectType = currentObjPtr->flags & 3;    // Get object type from flags bits 0-1: 0=free, 1=simple, 3=parent
      if ( objectType != 1 )
        break;                                  // Type 1 (simple object): check if should be saved
      if ( (currentObjPtr->flags & 4) == 0 )
        goto LABEL_6;                           // Check bit 2 (skip flag): if clear, object should be saved
LABEL_7:
      if ( --currentObjPtr <= g_RenderObjectsEnd )
        goto LABEL_8;
    }
    if ( objectType != 3 )
      goto LABEL_7;                             // Type 3 (parent object): always count it
LABEL_6:
    ++snapshot->objectCount;                    // Increment count of objects to save
    goto LABEL_7;
  }
LABEL_8:
  snapshot->decompressedData = AllocFromMemoryPool(16 * snapshot->objectCount + 2 * g_ScreenPixelCount);// PHASE 2: ALLOCATE BUFFER - Size = screen pixels (2 bytes each) + render objects (16 bytes each)
  snapshotIndexa = 0;                           // PHASE 3: COPY SCREEN DATA - Initialize screen copy loop
  destRowOffset = 0;
  do
  {
    memcpy(
      (char *)snapshot->decompressedData + g_ScreenWidth * destRowOffset,
      (const void *)(g_TileMapBuffer + 2 * (snapshotIndexa << g_TileMapShift)),
      2 * g_ScreenWidth);                       // Copy each row from tile map buffer to snapshot (skips every other row, copies 2 bytes per pixel)
    destRowOffset += 2;
    ++snapshotIndexa;
  }
  while ( snapshotIndexa < g_ScreenHeight );
  decompressedData = (int)snapshot->decompressedData;
  objDataPtr = (RenderObject *)(decompressedData + 2 * g_ScreenPixelCount);// objDataPtr (uint32_t*) - Pointer to write render objects as DWORDs. Each object = 4 DWORDs (16 bytes). Points to position after screen data in snapshot buffer.
  snapshotIndexb = 0;
  for ( objScanAddr = g_RenderObjectsStart; objScanAddr > g_RenderObjectsEnd; --objScanAddr )// Iterate through all render objects from start to end
  {
    LOBYTE(decompressedData) = objScanAddr->flags;
    objectFlags = decompressedData;
    decompressedData = objScanAddr->flags & 3;
    if ( decompressedData == 3 )                // TYPE 3 HANDLER: Parent object with potential children
    {
      objScanAddr->posY = -1;                   // Mark end of chain: set nextLink to 0xFFFF
      savedIndex = snapshotIndexb;
      if ( objScanAddr->posX == 0xFFFF )        // Check if root object: prevLink == 0xFFFF means no parent above this
      {
        *objDataPtr++ = *objScanAddr;
        objScanAddr->flags = 0;
      }
      else
      {
        parentObjAddr = &g_RenderObjectsStart[-objScanAddr->posX];// Has parent reference: calculate parent address from index stored in prevLink
        objScanAddr->posX = snapshotIndexb + 1; // Update link indices: convert from array indices to snapshot indices
        objScanAddr->posY = snapshotIndexb + 1;
        *objDataPtr = *objScanAddr;             // Copy object data: 4 DWORDs = 16 bytes
        objDataPtr2 = objDataPtr + 1;           // objDataPtr2 (uint32_t*) - Secondary pointer for writing parent object data when handling hierarchies.
        objScanAddr->flags = 0;                 // Mark as free: set flags to 0 (object now saved to snapshot)
        ++snapshotIndexb;
        parentObjAddr->parentIndex = savedIndex;// Update parent's child index to point to this saved position
        parentObjAddr->flags |= 0x80u;          // Set bit 7 on parent: flag that child has been saved
        *objDataPtr2 = *parentObjAddr;
        objDataPtr = objDataPtr2 + 1;
        parentObjAddr->flags = 0;
      }
      ++snapshotIndexb;
      childScanAddr = g_RenderObjectsStart;     // FIND CHILDREN: Scan for all child objects belonging to this parent
      decompressedData = 16;                    // Calculate parent's array index for matching children
      for ( i = g_RenderObjectsStart - objScanAddr; childScanAddr > g_RenderObjectsEnd; --childScanAddr )
      {                                         // Check if child: Type 1 (simple) with parentIndex matching current parent
        if ( (childScanAddr->flags & 7) == 1 && childScanAddr->parentIndex == i )
        {
          childScanAddr->parentIndex = savedIndex;// Update child's parent index to snapshot position
          *objDataPtr++ = *childScanAddr;
          childScanAddr->flags = 0;
          ++snapshotIndexb;
        }
      }
    }
    else if ( decompressedData == 1 && objScanAddr->parentIndex == 0xFFFF )// TYPE 1 HANDLER: Simple object with no parent (parentIndex == 0xFFFF)
    {
      decompressedData = objectFlags;
      if ( (objectFlags & 4) == 0 )             // Check skip flag (bit 2): only save if flag is clear
      {
        *objDataPtr++ = *objScanAddr;
        objScanAddr->flags = 0;
        ++snapshotIndexb;
      }
    }
  }
  return ResetRenderObjects();                  // PHASE 5: RESET - Clear render objects array and update bounds
}

void *__stdcall InitSlotFromTemplate(void *a1)
{
  return memcpy(a1, g_TemplateDataBuffer, g_TemplateDataSize);
}

char ProcessSpriteData()
{
  DBG_ENTER("ProcessSpriteData");
  void *v1; // eax

  ReadCompressedData((char *)&dword_4161F0);
  DBG_LOG("ProcessSpriteData: magic=0x%08X (expect 0x80000016)", (unsigned int)dword_4161F0);
  if ( dword_4161F0 != -2147483626 )
    return ShowFileError();
  *((_BYTE *)&g_SnapshotBasePtr + g_TemplateDataSize + 3) = 0;
  g_MemoryPool.poolSizeBytes = dword_4161F4 << 10;
  g_MemoryPool.frameDataSize = dword_416200 * dword_4161FC;
  DBG_LOG("ProcessSpriteData: pool.poolSizeBytes=%d pool.frameDataSize=%d",
    g_MemoryPool.poolSizeBytes, g_MemoryPool.frameDataSize);
  { int vsRet = InitializeVideoSystem(&unk_4162A0, (struct VideoConfigEx *)&dword_4161F0, &g_MemoryPool);
    DBG_LOG("ProcessSpriteData: InitializeVideoSystem returned %d", vsRet);
    if (!vsRet) return 0; }
  { int bbRet = InitBackbuffer((int)&dword_4161F0, &g_MemoryPool);
    DBG_LOG("ProcessSpriteData: InitBackbuffer returned %d", bbRet);
    if (!bbRet) return 0; }
  /* diagnostic removed: PRE-RESET snapshot logging */
  ClearRenderAndMemoryPoolState();
  ResetMemoryPool();
  v1 = (void *)AllocMemoryChunk(4 * video_queue_active);
  g_TemplateDataBuffer = v1;
  opcodeDataPtr = (int)v1;
  if ( g_MaxReadBufferSize )
  {
    InitFileReadBuffer(v1);
    g_TemplateDataBuffer = (void *)AllocMemoryChunk(g_MaxReadBufferSize);
    opcodeDataPtr = (int)g_TemplateDataBuffer;
  }
  /* Preallocate slot arrays using cached config from previous
   * InitializeFrameBuffers call. This prevents stale pointers when
   * InitializeFrameBuffers is not called (startResourceId mismatch). */
  ReallocFrameBuffers();
  return 1;
}

void *InitializeFrameBuffers()
{
  DBG_ENTER("InitializeFrameBuffers");
  int v0; // ebx
  int *v1; // ecx
  _BYTE *v2; // esi
  void *result; // eax

  ReadCompressedData((char *)&dword_416160);
  dword_4161E4 = dword_416164 * dword_416160;
  g_ScreenPixelCount = g_ScreenHeight * g_ScreenWidth;
  g_SnapshotBasePtr = (int)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(dword_416178);
  g_TileMapBuffer = (int)g_TemplateDataBuffer;
  v0 = 0;
  if ( dword_410234 < g_ScreenWidth )
  {
    v1 = &dword_410234;
    do
    {
      v1 += 2;
      ++v0;
    }
    while ( *v1 < g_ScreenWidth );
  }
  g_TileMapShift = dword_410238[2 * v0];
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(g_ScreenHeight << (g_TileMapShift + 1));
  v2 = g_TemplateDataBuffer;
  g_ScreenSnapshotArray = (ScreenSnapshot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(16 * dword_4161E4);
  g_TileMapPtr = (int)g_TemplateDataBuffer;
  g_MapMaxX = g_ScreenWidth - 1;
  g_NegScreenWidth = -g_ScreenWidth;
  g_MapMaxY = g_ScreenHeight - 1;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(g_ScreenPixelCount);
  g_SpriteSlotArray = (SpriteSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(16 * dword_41617C);
  g_AnimationSlotArray = (AnimationSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(16 * dword_416180);
  g_FontSlotArray = (FontSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(8 * dword_416184);
  g_SoundSlotArray = (SoundSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(8 * dword_416188);
  g_MusicTrackArray = (MusicTrackSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(4 * dword_41618C);
  g_OverlaySlotArray = (OverlaySlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(8 * dword_416190);
  result = ClearBuffer((_BYTE *)g_TemplateDataBuffer - v2, v2);
  opcodeDataPtr = (int)g_TemplateDataBuffer;
  g_GlobalVarsPtr = (int)g_TemplateDataBuffer;
  return result;
}

void ReallocFrameBuffers(void)
{
  int v0;
  int *v1;
  _BYTE *slotStart;
  /* Only run if a previous InitializeFrameBuffers set the config */
  if ( !dword_416178 ) return;
  DBG_MSG("ReallocFrameBuffers: preallocating arrays from cached config");
  dword_4161E4 = dword_416164 * dword_416160;
  g_ScreenPixelCount = g_ScreenHeight * g_ScreenWidth;
  g_SnapshotBasePtr = (int)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(dword_416178);
  g_TileMapBuffer = (int)g_TemplateDataBuffer;
  v0 = 0;
  if ( dword_410234 < g_ScreenWidth )
  {
    v1 = &dword_410234;
    do
    {
      v1 += 2;
      ++v0;
    }
    while ( *v1 < g_ScreenWidth );
  }
  g_TileMapShift = dword_410238[2 * v0];
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(g_ScreenHeight << (g_TileMapShift + 1));
  /* Skip snapshot and tilemap areas — do NOT overwrite g_ScreenSnapshotArray.
     In the original binary, these areas keep valid data from the previous
     GameMainLoop call because ResetMemoryPool only moves the cursor,
     not zeroing memory, and InitializeFrameBuffers is not called again.
     We just advance the pool cursor past them. */
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(16 * dword_4161E4);
  g_TileMapPtr = (int)g_TemplateDataBuffer;
  g_MapMaxX = g_ScreenWidth - 1;
  g_NegScreenWidth = -g_ScreenWidth;
  g_MapMaxY = g_ScreenHeight - 1;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(g_ScreenPixelCount);
  /* Slot arrays start here — these MUST be zeroed to prevent stale pointers */
  slotStart = (_BYTE *)g_TemplateDataBuffer;
  g_SpriteSlotArray = (SpriteSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(16 * dword_41617C);
  g_AnimationSlotArray = (AnimationSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(16 * dword_416180);
  g_FontSlotArray = (FontSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(8 * dword_416184);
  g_SoundSlotArray = (SoundSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(8 * dword_416188);
  g_MusicTrackArray = (MusicTrackSlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(4 * dword_41618C);
  g_OverlaySlotArray = (OverlaySlot *)g_TemplateDataBuffer;
  g_TemplateDataBuffer = (void *)AllocMemoryChunk(8 * dword_416190);
  /* Only clear the slot arrays, NOT the snapshot/tilemap data above */
  ClearBuffer((_BYTE *)g_TemplateDataBuffer - slotStart, slotStart);
  opcodeDataPtr = (int)g_TemplateDataBuffer;
  g_GlobalVarsPtr = (int)g_TemplateDataBuffer;
}

unsigned __int16 *__stdcall LoadSpriteData(unsigned __int16 *a1)
{
  DBG_ENTER("LoadSpriteData");
  unsigned int v1; // esi
  int v2; // ecx
  void *v3; // ebx
  int v4; // eax

  if ( *((_DWORD *)a1 + 1) != 1147761515 )
    return a1 + 2;
  v1 = *((_DWORD *)a1 + 3);
  v2 = *((_DWORD *)a1 + 2);
  if ( v2 != g_CachedSpriteOffset )
  {
    g_CachedSpriteOffset = *((_DWORD *)a1 + 2);
    SeekFileWithBuffer(0, v2);
    if ( v1 )
    {
      v3 = (void *)AllocTempBuffer(v1);
      ReadFileData(v1, v3);
      LZSSDecompress(v1, g_SpriteDataBuffer, v3);
    }
    else
    {
      ReadFileData(a1[1] * *a1, g_SpriteDataBuffer);
    }
  }
  v4 = -(v1 == 0);
  LOBYTE(v4) = v4 & 0xFC;
  return (unsigned __int16 *)((char *)g_SpriteDataBuffer + v4 + 4);
}

int CloseFile_Wrapper()
{
  return CloseFile();
}

char __stdcall LZSSDecompress(int a1, _BYTE *a2, char *a3)
{
  char v6; // cf
  unsigned __int8 v7; // dl
  char result; // al
  char v9; // dh
  int v10; // eax
  int v11; // ebx
  int v12; // ebx
  char v13; // ah

LABEL_9:
  while ( 1 )
  {
    v7 = *a3++;
    v9 = 8;
    if ( --a1 < 0 )
      return result;
    while ( 1 )
    {
      v6 = v7 & 1;
      v7 >>= 1;
      if ( v6 )
        break;
LABEL_5:
      a1 -= 2;
      if ( a1 < 0 )
        return result;
      v10 = *(unsigned __int16 *)a3;
      a3 += 2;
      v11 = v10;
      BYTE1(v11) = BYTE1(v10) >> 4;
      v12 = -v11;
      v13 = (BYTE1(v10) & 0xF) + 2;
      do
      {
        result = a2[v12];
        *a2++ = result;
        --v13;
      }
      while ( v13 >= 0 );
      if ( !--v9 )
        goto LABEL_9;
    }
    while ( 1 )
    {
      --a1;
      result = *a3++;
      *a2++ = result;
      if ( !--v9 )
        break;
      v6 = v7 & 1;
      v7 >>= 1;
      if ( !v6 )
        goto LABEL_5;
    }
  }
}

#pragma pack(pop)
