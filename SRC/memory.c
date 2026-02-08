#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- memory module ---- */

void ResetMemoryPool()
{
  g_MemoryPoolAvailable = (unsigned int)g_MemoryPool.poolSizeBytes >> 2;// g_MemoryPoolAvailable = number of DWORDs that can be allocated
  g_MemoryPoolStart = g_MemoryPool.poolBasePtr; // g_MemoryPoolStart = base address of pool buffer
  g_MemoryPoolCurrent = g_MemoryPool.poolBasePtr;// g_MemoryPoolCurrent = current allocation pointer, reset to start
}

int __stdcall AllocMemoryChunk(unsigned int a1)
{
  unsigned int v1; // ecx
  unsigned int v2; // ecx

  v1 = a1;
  if ( (a1 & 3) != 0 )
    v1 = a1 + 4;
  v2 = v1 >> 2;
  if ( v2 < g_MemoryPoolAvailable )
  {
    g_MemoryPoolAvailable -= v2;
    g_MemoryPoolCurrent += 4 * v2;
    g_MemoryPoolStart = g_MemoryPoolCurrent;
    return g_MemoryPoolCurrent;
  }
  else
  {
    Error_InsufficientMemory();
    return g_MemoryPool.poolBasePtr;
  }
}

unsigned int *__stdcall AllocFromMemoryPool(unsigned int a1)
{
  unsigned int *v1; // ebx
  unsigned int v2; // edx
  unsigned int v3; // edx
  unsigned int v4; // ecx

  v1 = (unsigned int *)g_MemoryPoolStart;
  v2 = a1 >> 2;
  if ( (a1 & 3) != 0 )
    ++v2;
  v3 = v2 + 1;
  v4 = *(_DWORD *)g_MemoryPoolStart;
  if ( *(int *)g_MemoryPoolStart >= 0 && v4 > v3 )
  {
LABEL_9:
    if ( v4 != v3 )
    {
      v1[v3] = v4 - v3;
      if ( v1 == (unsigned int *)g_MemoryPoolCurrent )
      {
        g_MemoryPoolCurrent = (int)&v1[v3];
        g_MemoryPoolAvailable = v4 - v3;
      }
    }
    *v1 = v3 | 0x80000000;
    return v1 + 1;
  }
  else
  {
    while ( (unsigned int)v1 < g_MemoryPoolCurrent )
    {
      v1 += v4;
      v4 = *v1;
      if ( (*v1 & 0x80000000) == 0 && v4 > v3 )
        goto LABEL_9;
    }
    return (unsigned int *)Error_InsufficientMemory();
  }
}

int *__stdcall FreeMemoryBlock(int a1)
{
  int *v1; // edx
  int *v2; // ebx
  int v3; // esi
  int *v4; // ebp
  int v5; // edi
  int *result; // eax

  v1 = (int *)g_MemoryPoolStart;
  v2 = 0;
  if ( a1 - 4 != g_MemoryPoolStart )
  {
    do
    {
      v2 = v1;
      v1 += *v1;
    }
    while ( v1 != (int *)(a1 - 4) );
  }
  v3 = *v1 & 0x7FFFFFFF;
  v4 = 0;
  v5 = v1[*v1];
  if ( v5 >= 0 )
  {
    v4 = &v1[*v1];
    v3 += v5;
  }
  if ( v2 && *v2 >= 0 )
  {
    v3 += *v2;
    v1 = v2;
  }
  result = v4;
  if ( v4 == (int *)g_MemoryPoolCurrent )
  {
    g_MemoryPoolCurrent = (int)v1;
    g_MemoryPoolAvailable = v3;
  }
  *v1 = v3;
  return result;
}

unsigned int *AllocOpcodeBuffer()
{
  unsigned int v0; // ebx
  unsigned int *result; // eax

  v0 = 4 * g_ScreenPixelCount + 20;
  if ( v0 <= dword_416178 )
    v0 = dword_416178;
  result = AllocFromMemoryPool(v0);
  dword_4115C8 = (int)result;
  return result;
}

int *CleanupGameLoop()
{
  /* Guard: only free if opcode buffer ptr is within the current memory pool */
  if ( dword_4115C8 && (unsigned int)dword_4115C8 >= (unsigned int)g_MemoryPoolStart
    && (unsigned int)dword_4115C8 <= (unsigned int)g_MemoryPoolCurrent )
    return FreeMemoryBlock(dword_4115C8);
  return 0;
}

char *__stdcall AllocTempBuffer(unsigned int size)
{                                               // Small allocation: use fixed 0x98-byte static buffer
char *v1;
// edx

  if ( size <= 0x98 )
    return (char *)&g_SmallTempBuffer;
  if ( g_RenderObjectsEnd )                     // Allocate from top of render objects memory region
    return (char *)g_RenderObjectsEnd - size;
  if ( g_MemoryPoolCurrent )                    // Fallback: allocate from top of general memory pool (base + available*4 - size)
    return (char *)(g_MemoryPoolCurrent + 4 * g_MemoryPoolAvailable - size);
  return v1;
}

int SetupMemoryContext()
{
  g_RenderObjectsStart = (RenderObject *)(g_MemoryPoolCurrent + 4 * g_MemoryPoolAvailable - 16);
  return UpdateRenderObjectBounds((RenderObject *)(g_MemoryPoolCurrent + 4 * g_MemoryPoolAvailable - 16));
}

RenderObject *AllocRenderObject()
{
  RenderObject *i; // ebx

  for ( i = g_RenderObjectsStart; (i->flags & 1) != 0 && i != g_RenderObjectsEnd; --i )
    ;
  i->flags = 1;
  if ( i == g_RenderObjectsEnd )
    UpdateRenderObjectBounds((int)&i[-1]);
  return i;
}

int __stdcall FreeRenderObject(RenderObject *a1)
{
  RenderObject *v1; // ebx
  int result; // eax

  v1 = a1;
  result = 0;
  a1->flags = 0;
  if ( &a1[-1] == g_RenderObjectsEnd )
  {
    while ( v1 != g_RenderObjectsStart )
    {
      ++v1;
      if ( (v1->flags & 1) != 0 )
        return UpdateRenderObjectBounds((int)--v1);
    }
    return UpdateRenderObjectBounds((int)v1);
  }
  return result;
}

int ResetRenderObjects()
{
  g_RenderObjectsStart->flags = 0;
  return UpdateRenderObjectBounds(g_RenderObjectsStart);
}

void __stdcall SafeFreeMemoryBlock(void *ptr)
{
  if ( ptr )
    FreeMemoryBlock((int)ptr);
}

#pragma pack(pop)
