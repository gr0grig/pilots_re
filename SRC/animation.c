#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- animation module ---- */

void __stdcall TransformDir_Rotate90CCW(SpriteDesc *desc)
{
  int8_t v1; // dl

  v1 = -desc->dy;
  desc->dy = desc->dx;
  desc->dx = v1;
  desc->dirFlags = g_DirFlagRemap_Rotate90CCW[(int)desc->dirFlags >> 4] | desc->dirFlags & 0xF;
}

void __stdcall TransformDir_Rotate180(SpriteDesc *desc)
{
  desc->dy = -desc->dy;
  desc->dx = -desc->dx;
  desc->dirFlags = g_DirFlagRemap_Rotate180[(int)desc->dirFlags >> 4] | desc->dirFlags & 0xF;
}

void __stdcall TransformDir_Rotate90CW(SpriteDesc *desc)
{
  int8_t v1; // dl

  v1 = -desc->dx;
  desc->dx = desc->dy;
  desc->dy = v1;
  desc->dirFlags = g_DirFlagRemap_Rotate90CW[(int)desc->dirFlags >> 4] | desc->dirFlags & 0xF;
}

void __stdcall TransformDir_FlipHorizontal(SpriteDesc *desc)
{
  desc->dx = -desc->dx;
  desc->dirFlags = g_DirFlagRemap_FlipHorizontal[(int)desc->dirFlags >> 4] | desc->dirFlags & 0xF;
}

void __stdcall TransformDir_ReflectAntiDiag(SpriteDesc *desc)
{
  int8_t v1; // dl

  v1 = -desc->dx;                               // temp = -desc->dx
  desc->dx = -desc->dy;                         // desc->dx = -desc->dy
  desc->dy = v1;                                // desc->dy = temp (original -dx)
  desc->dirFlags = g_DirFlagRemap_ReflectAntiDiag[(int)desc->dirFlags >> 4] | desc->dirFlags & 0xF;// Store remapped direction flags back
}

void __stdcall TransformDir_FlipVertical(SpriteDesc *desc)
{
  desc->dy = -desc->dy;
  desc->dirFlags = g_DirFlagRemap_FlipVertical[(int)desc->dirFlags >> 4] | desc->dirFlags & 0xF;
}

void __stdcall TransformDir_ReflectDiag(SpriteDesc *desc)
{
  int8_t dx; // dl

  dx = desc->dx;
  desc->dx = desc->dy;
  desc->dy = dx;
  desc->dirFlags = g_DirFlagRemap_ReflectDiag[(int)desc->dirFlags >> 4] | desc->dirFlags & 0xF;
}

int __stdcall RandomSpriteAttrib_Clear(int a1)
{
  *(_BYTE *)(a1 + 1) &= 0xF0u;
  return a1;
}

int __stdcall RandomSpriteAttrib_2A(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_2A[(2 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_2B(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_2B[(2 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_2C(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_2C[(2 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_3A(int renderObj)
{
  int result; // eax

  *(_BYTE *)(renderObj + 1) = g_SpriteVariantTable_3A[(3 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_2D(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_2D[(2 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_2E(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_2E[(2 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_3B(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_3B[(3 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_2F(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_2F[(2 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_3C(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_3C[(3 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_3D(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_3D[(3 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall RandomSpriteAttrib_4A(int a1)
{
  int result; // eax

  *(_BYTE *)(a1 + 1) = g_SpriteVariantTable_4A[(4 * (unsigned int)HIWORD(g_RandomSeed)) >> 16];
  result = 1103515245 * g_RandomSeed;
  g_RandomSeed *= 1103515245;
  g_RandomSeed += 12345;
  return result;
}

int __stdcall SelectRandomAnimationFrame(char a1, int a2)
{
  ReexecuteCurrentSpriteScript();
  if ( a1 )
  {
    a2 = *((unsigned __int8 *)g_AnimationSlotArray[a2].frameSelectionData
         + ((*(unsigned __int8 *)g_AnimationSlotArray[a2].frameSelectionData * (unsigned int)HIWORD(g_RandomSeed)) >> 16)
         + 1);
    g_RandomSeed *= 1103515245;
    g_RandomSeed += 12345;
  }
  *(_BYTE *)(g_GameFlags1 + 2) = a2;
  *(_BYTE *)(g_GameFlags1 + 3) = 16;
  return ExecuteScriptWithContext(
           16,
           *(_DWORD *)(g_GameFlags1 + 7),
           *(_DWORD *)(g_GameFlags1 + 6),
           0,
           0xFFFF,
           0,
           (int)&g_SpriteSlotArray[a2],
           (BYTE *)g_SpriteSlotArray[a2].dataPtr);
}

char __stdcall UpdateSpriteAnimation(int shouldFree, int forceUpdate, RenderObject *object)
{
  unsigned int v3; // esi

  v3 = LOBYTE(object->renderInfo);
  if ( v3 <= 1 )
  {
    if ( forceUpdate || (object->flags & 4) != 0 )
    {
      AddObjectToDirtyRegions(object);
      goto LABEL_14;
    }
    return 0;
  }
  AddObjectToDirtyRegions(object);
  if ( ++object->slotIndex != v3 )
  {
    *(_DWORD *)&object->spriteDataPtr += 8;
LABEL_7:
    if ( (object->flags & 0x40) == 0 )
      CalculateObjectPosition(object);
    AddObjectToDirtyRegions(object);
    return 0;
  }
  object->slotIndex = 0;
  *(_DWORD *)&object->spriteDataPtr -= 8 * v3 - 8;
  if ( !forceUpdate && (object->flags & 4) == 0 )
    goto LABEL_7;
LABEL_14:
  if ( shouldFree )
    FreeRenderObject(object);
  return 1;
}

BOOL __stdcall CheckRotatedDirectionMatch(int base_direction, int expected_direction)
{
  char baseDir; // dl
  char rotIndex; // cl

  baseDir = base_direction;
  rotIndex = g_SpriteDirectionIndex;
  if ( (unsigned int)g_SpriteDirectionIndex > 3 )// Index > 3: mirror the base direction and reduce rotation index by 4
  {
    rotIndex = g_SpriteDirectionIndex - 4;
    baseDir = g_MirrorDirectionTable[base_direction];
  }
  return ((baseDir + 2 * rotIndex) & 7) == expected_direction;// Compute rotated direction and compare: (base + 2*index) & 7 == expected
}

int __stdcall SetCurrentAnimationSlot(int a1)
{
  g_SpecialMode = (int)g_AnimationSlotArray[a1].frameBitmask;
  return 1;
}

#pragma pack(pop)
