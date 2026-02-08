#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- blitting module ---- */

char __stdcall InitBackbuffer(int videoConfig, VideoDataBuffer *dataBuffer)
{
  void *allocPtr; // eax

  g_BackbufferStride = *(_DWORD *)(videoConfig + 12);// Store backbuffer stride (width) and height from video config
  g_BackbufferHeight = *(_DWORD *)(videoConfig + 16);
  g_BackbufferPixels = dataBuffer->buffer;      // Try to use existing buffer pointer from data buffer
  if ( !g_BackbufferPixels )                    // No buffer allocated yet: bump-allocate from data buffer's free region
  {
    allocPtr = (void *)dataBuffer->field_4;     // Take current free pointer as buffer start
    dataBuffer->buffer = allocPtr;
    g_BackbufferPixels = allocPtr;
    dataBuffer->field_4 = (int)allocPtr + dataBuffer->size;// Advance free pointer by buffer size
    dataBuffer->field_0 -= dataBuffer->size;    // Decrease available memory by buffer size
  }
  return 1;
}

void __stdcall CopyRectToBackbuffer(
        int bottom,
        int right,
        int top,
        int left,
        int srcStride,
        int srcWidth,
        int offsetY,
        int offsetX,
        void *srcPtr)
{
  char *v9; // edi
  char *v10; // esi
  int v11; // edx
  int v12; // ebx
  int v13; // ebp
  unsigned int v14; // eax
  unsigned __int8 v15; // cl
  char *v16; // edi
  char *v17; // esi
  int v18; // cc
  char *v19; // edi
  char *v20; // esi
  char *v21; // edi
  char *v22; // esi

  v9 = (char *)g_BackbufferPixels + left + g_BackbufferStride * (g_BackbufferHeight - 1 - top);
  v10 = (char *)srcPtr + srcWidth * -offsetY - offsetX;
  v11 = right - left + g_BackbufferStride;
  v12 = srcWidth - (right - left);
  v13 = bottom - top;
  v14 = (unsigned int)(right - left) >> 2;
  v15 = (right - left) & 3;
  if ( v15 )
  {
    if ( v15 >> 1 )
    {
      if ( ((_BYTE)right - (_BYTE)left) & 1 )
      {
        do
        {
          qmemcpy(v9, v10, 4 * v14);
          v17 = &v10[4 * v14];
          v16 = &v9[4 * v14];
          *(_WORD *)v16 = *(_WORD *)v17;
          v17 += 2;
          v16 += 2;
          *v16 = *v17;
          v9 = &v16[-v11 + 1];
          v10 = &v17[v12 + 1];
          v18 = v13-- <= 1;
        }
        while ( !v18 );
      }
      else
      {
        do
        {
          qmemcpy(v9, v10, 4 * v14);
          v20 = &v10[4 * v14];
          v19 = &v9[4 * v14];
          *(_WORD *)v19 = *(_WORD *)v20;
          v9 = &v19[-v11 + 2];
          v10 = &v20[v12 + 2];
          v18 = v13-- <= 1;
        }
        while ( !v18 );
      }
    }
    else
    {
      do
      {
        qmemcpy(v9, v10, 4 * v14);
        v22 = &v10[4 * v14];
        v21 = &v9[4 * v14];
        *v21 = *v22;
        v9 = &v21[-v11 + 1];
        v10 = &v22[v12 + 1];
        v18 = v13-- <= 1;
      }
      while ( !v18 );
    }
  }
  else
  {
    do
    {
      qmemcpy(v9, v10, 4 * v14);
      v9 = &v9[4 * v14 - v11];
      v10 += 4 * v14 + v12;
      v18 = v13-- <= 1;
    }
    while ( !v18 );
  }
}

char __stdcall DrawSpriteTransparent(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
  char *v9; // edi
  char *v10; // esi
  int v11; // edx
  int v12; // ebp
  int v13; // ecx
  char result; // al
  char *v15; // esi

  v9 = (char *)g_BackbufferPixels + a4 + g_BackbufferStride * (g_BackbufferHeight - 1 - a3);
  v10 = (char *)(a9 + a6 * -a7 - a8);
  v11 = a2 - a4 + g_BackbufferStride;
  v12 = a1 - a3;
  g_CurrentScanlineWidth = a2 - a4;
  do
  {
    v13 = g_CurrentScanlineWidth;
    result = *v10;
    v15 = v10 + 1;
    if ( !result )
      goto LABEL_19;
LABEL_3:
    *v9++ = result;
    if ( !--v13 )
      goto LABEL_34;
    result = *v15++;
    if ( result )
    {
      *v9++ = result;
      if ( !--v13 )
        goto LABEL_34;
      result = *v15++;
      if ( result )
      {
        *v9++ = result;
        if ( !--v13 )
          goto LABEL_34;
        result = *v15++;
        if ( result )
        {
          *v9++ = result;
          if ( !--v13 )
            goto LABEL_34;
          result = *v15++;
          if ( result )
          {
            *v9++ = result;
            if ( !--v13 )
              goto LABEL_34;
            result = *v15++;
            if ( result )
            {
              *v9++ = result;
              if ( !--v13 )
                goto LABEL_34;
              result = *v15++;
              if ( result )
              {
                *v9++ = result;
                if ( !--v13 )
                  goto LABEL_34;
                result = *v15++;
                if ( result )
                {
                  *v9++ = result;
                  if ( !--v13 )
                    goto LABEL_34;
                  goto LABEL_18;
                }
              }
            }
          }
        }
      }
    }
LABEL_19:
    while ( 1 )
    {
      ++v9;
      if ( !--v13 )
        break;
      result = *v15++;
      if ( result )
        goto LABEL_3;
      ++v9;
      if ( !--v13 )
        break;
      result = *v15++;
      if ( result )
        goto LABEL_3;
      ++v9;
      if ( !--v13 )
        break;
      result = *v15++;
      if ( result )
        goto LABEL_3;
      ++v9;
      if ( !--v13 )
        break;
      result = *v15++;
      if ( result )
        goto LABEL_3;
      ++v9;
      if ( !--v13 )
        break;
      result = *v15++;
      if ( result )
        goto LABEL_3;
      ++v9;
      if ( !--v13 )
        break;
      result = *v15++;
      if ( result )
        goto LABEL_3;
      ++v9;
      if ( !--v13 )
        break;
      result = *v15++;
      if ( result )
        goto LABEL_3;
      ++v9;
      if ( !--v13 )
        break;
LABEL_18:
      result = *v15++;
      if ( result )
        goto LABEL_3;
    }
LABEL_34:
    v9 -= v11;
    v10 = &v15[a6 - (a2 - a4)];
  }
  while ( v12-- > 1 );
  return result;
}

char __stdcall DrawSpriteFlippedV(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
  char *v9; // edi
  char *v10; // esi
  int v11; // edx
  int v12; // ebp
  int v13; // ecx
  char result; // al

  v9 = (char *)g_BackbufferPixels + a4 + g_BackbufferStride * (g_BackbufferHeight - 1 - a3);
  v10 = (char *)(a9 + a6 * (a7 + a5 - 1) - a8);
  v11 = a2 - a4 + g_BackbufferStride;
  v12 = a1 - a3;
  g_CurrentScanlineWidth = a2 - a4;
  do
  {
    v13 = g_CurrentScanlineWidth;
    while ( 1 )
    {
      result = *v10++;
      if ( !result )
        break;
LABEL_4:
      *v9++ = result;
      if ( !--v13 )
        goto LABEL_8;
    }
    while ( 1 )
    {
      ++v9;
      if ( !--v13 )
        break;
      result = *v10++;
      if ( result )
        goto LABEL_4;
    }
LABEL_8:
    v9 -= v11;
    v10 += -(a2 - a4) - a6;
  }
  while ( v12-- > 1 );
  return result;
}

char __stdcall DrawSpriteFlippedH(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
  char *v9; // edi
  char *v10; // esi
  int v11; // edx
  int v12; // ebp
  int v13; // ecx
  char result; // al

  v9 = (char *)g_BackbufferPixels + a4 + g_BackbufferStride * (g_BackbufferHeight - 1 - a3);
  v10 = (char *)(a9 + a8 + a6 + a6 * -a7 - 1);
  v11 = a2 - a4 + g_BackbufferStride;
  v12 = a1 - a3;
  g_CurrentScanlineWidth = a2 - a4;
  do
  {
    v13 = g_CurrentScanlineWidth;
    while ( 1 )
    {
      result = *v10--;
      if ( !result )
        break;
LABEL_4:
      *v9++ = result;
      if ( !--v13 )
        goto LABEL_8;
    }
    while ( 1 )
    {
      ++v9;
      if ( !--v13 )
        break;
      result = *v10--;
      if ( result )
        goto LABEL_4;
    }
LABEL_8:
    v9 -= v11;
    v10 += a2 - a4 + a6;
  }
  while ( v12-- > 1 );
  return result;
}

char __stdcall DrawSpriteFlippedHV(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
  char *v9; // edi
  char *v10; // esi
  int v11; // edx
  int v12; // ebp
  int v13; // ecx
  char result; // al

  v9 = (char *)g_BackbufferPixels + a4 + g_BackbufferStride * (g_BackbufferHeight - 1 - a3);
  v10 = (char *)(a9 + a8 + a6 + a6 * (a7 + a5 - 1) - 1);
  v11 = a2 - a4 + g_BackbufferStride;
  v12 = a1 - a3;
  g_CurrentScanlineWidth = a2 - a4;
  do
  {
    v13 = g_CurrentScanlineWidth;
    while ( 1 )
    {
      result = *v10--;
      if ( !result )
        break;
LABEL_4:
      *v9++ = result;
      if ( !--v13 )
        goto LABEL_8;
    }
    while ( 1 )
    {
      ++v9;
      if ( !--v13 )
        break;
      result = *v10--;
      if ( result )
        goto LABEL_4;
    }
LABEL_8:
    v9 -= v11;
    v10 -= a6 - (a2 - a4);
  }
  while ( v12-- > 1 );
  return result;
}

void __stdcall ClearBackbuffer(unsigned __int8 colorIndex)
{
  int fillPattern; // eax

  LOBYTE(fillPattern) = colorIndex;             // AL = colorIndex
  BYTE1(fillPattern) = colorIndex;              // AH = AL → AX = colorIndex | (colorIndex << 8)
  memset32(
    g_BackbufferPixels,
    (fillPattern << 16) | fillPattern,
    (unsigned int)(g_BackbufferHeight * g_BackbufferStride) >> 2);// rep stosd: fill backbuffer DWORDs with replicated color byte
}

unsigned char *__cdecl BlitAdditive_Raw(
        int width, int height, int strideAdjust,
        unsigned char *dst, unsigned char *src)
{
  int colsRemaining; // ecx
  char srcPixel; // al

  colsRemaining = width;                        // ECX = row pixel counter (initialized from width each row)
  do
  {
    do
    {
      srcPixel = *src++;                        // Inner loop: read source byte, add to destination byte (additive blending for palette-indexed pixels)
      *dst++ += srcPixel;
      --colsRemaining;
    }
    while ( colsRemaining );
    colsRemaining = width;                      // Reset column counter for next row
    dst -= strideAdjust;                        // Adjust destination pointer for next scanline: dst -= strideAdjust (accounts for stride vs width difference and scan direction)
    --height;                                   // Decrement row counter
  }
  while ( height );
  return src;                                   // xchg eax, esi: return updated source pointer in EAX
}

char *__cdecl RLEBlitAdditive_TypeA(
        int a1, int a2, int a3, int a4, _BYTE *a5, char *a6)
{
  int v7; // ecx
  char v8; // al

  do
  {
    v7 = a2;
    while ( 1 )
    {
      while ( 1 )
      {
        while ( 1 )
        {
          LOBYTE(a1) = *a6++;
          if ( (a1 & 0x80u) != 0 )
            break;
          a5 += ++a1;
          v7 -= a1;
          if ( !v7 )
            goto LABEL_18;
        }
        if ( (a1 & 0x40) != 0 )
          break;
        LOBYTE(a1) = a1 & 0x3F;
        v7 -= ++a1;
        BYTE1(a1) = a1;
        do
        {
          LOBYTE(a1) = *a6++;
          *a5++ += a1;
          --BYTE1(a1);
        }
        while ( BYTE1(a1) );
        if ( !v7 )
          goto LABEL_18;
      }
      LOBYTE(a1) = a1 & 0x3F;
      if ( !(_BYTE)a1 )
      {
        a5 += v7;
        goto LABEL_18;
      }
      if ( (_BYTE)a1 == 1 )
        break;
      v7 -= ++a1;
      BYTE1(a1) = a1;
      LOBYTE(a1) = *a6++;
      do
      {
        *a5++ += a1;
        --BYTE1(a1);
      }
      while ( BYTE1(a1) );
      if ( !v7 )
        goto LABEL_18;
    }
    do
    {
      v8 = *a6++;
      *a5++ += v8;
      --v7;
    }
    while ( v7 );
LABEL_18:
    a5 -= a4;
    --a3;
  }
  while ( a3 );
  return a6;
}

char *__cdecl RLEBlitAdditive_TypeB(
        int a1, int a2, int a3, int a4, _BYTE *a5, char *a6)
{
  int v7; // ecx
  char v8; // al

  do
  {
    v7 = a2;
    while ( 1 )
    {
      while ( 1 )
      {
        LOBYTE(a1) = *a6++;
        if ( (a1 & 0x80u) == 0 )
          break;
        if ( (a1 & 0x40) != 0 )
        {
          LOBYTE(a1) = a1 & 0x3F;
          v7 -= ++a1;
          BYTE1(a1) = a1;
          do
          {
            LOBYTE(a1) = *a6++;
            *a5++ += a1;
            --BYTE1(a1);
          }
          while ( BYTE1(a1) );
          if ( !v7 )
            goto LABEL_18;
        }
        else
        {
          LOBYTE(a1) = a1 & 0x3F;
          a5 += ++a1;
          v7 -= a1;
          if ( !v7 )
            goto LABEL_18;
        }
      }
      if ( !(_BYTE)a1 )
      {
        a5 += v7;
        goto LABEL_18;
      }
      if ( (_BYTE)a1 == 1 )
        break;
      v7 -= ++a1;
      BYTE1(a1) = a1;
      LOBYTE(a1) = *a6++;
      do
      {
        *a5++ += a1;
        --BYTE1(a1);
      }
      while ( BYTE1(a1) );
      if ( !v7 )
        goto LABEL_18;
    }
    do
    {
      v8 = *a6++;
      *a5++ += v8;
      --v7;
    }
    while ( v7 );
LABEL_18:
    a5 -= a4;
    --a3;
  }
  while ( a3 );
  return a6;
}

char *__cdecl RLEBlitAdditive_TypeC(
        int a1, int a2, int a3, int a4, _BYTE *a5, char *a6)
{
  int v7; // ecx
  char v8; // al

  do
  {
    v7 = a2;
    while ( 1 )
    {
      while ( 1 )
      {
        while ( 1 )
        {
          LOBYTE(a1) = *a6++;
          if ( (a1 & 0x80u) != 0 )
            break;
          v7 -= ++a1;
          BYTE1(a1) = a1;
          do
          {
            LOBYTE(a1) = *a6++;
            *a5++ += a1;
            --BYTE1(a1);
          }
          while ( BYTE1(a1) );
          if ( !v7 )
            goto LABEL_18;
        }
        if ( (a1 & 0x40) != 0 )
          break;
        LOBYTE(a1) = a1 & 0x3F;
        a5 += ++a1;
        v7 -= a1;
        if ( !v7 )
          goto LABEL_18;
      }
      LOBYTE(a1) = a1 & 0x3F;
      if ( !(_BYTE)a1 )
      {
        a5 += v7;
        goto LABEL_18;
      }
      if ( (_BYTE)a1 == 1 )
        break;
      v7 -= ++a1;
      BYTE1(a1) = a1;
      LOBYTE(a1) = *a6++;
      do
      {
        *a5++ += a1;
        --BYTE1(a1);
      }
      while ( BYTE1(a1) );
      if ( !v7 )
        goto LABEL_18;
    }
    do
    {
      v8 = *a6++;
      *a5++ += v8;
      --v7;
    }
    while ( v7 );
LABEL_18:
    a5 -= a4;
    --a3;
  }
  while ( a3 );
  return a6;
}

unsigned __int8 *__stdcall RLEBlitAdditive_Dispatch(int a1, int a2, int a3, int a4, int a5, unsigned __int8 *a6)
{
  return (unsigned __int8 *)(((BlitFuncPtrInternal)g_BlitFuncTable[a5])(
                               a2,
                               a1,
                               a2 + g_BackbufferStride,
                               (unsigned __int8 *)g_BackbufferPixels
                             + a4
                             + g_BackbufferStride * (g_BackbufferHeight - 1 - a3),
                               a6)
                           - a6);
}

int __stdcall BlitDirtyRegion(char a1, int a2, int a3, int y, int x)
{
  return BlitScreenWithDissolve(a1, g_BackbufferHeight, g_BackbufferStride, a2 - y, a3 - x, y, x, g_BackbufferPixels);
}

#pragma pack(pop)
