#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- tilemap module ---- */

int __stdcall SetupPathfindPositions(unsigned __int8 search_type)
{
  if ( search_type == 130 || search_type == 131 )
  {
    g_PathStartX = g_SelectedTileX;
    g_PathStartY = g_SelectedTileY;
  }
  else
  {
    g_PathStartX = g_CurrentTileX;
    g_PathStartY = g_CurrentTileY;
  }
  g_PathTargetX = *(unsigned __int8 *)(g_GameFlags1 + 6);
  g_PathTargetY = *(unsigned __int8 *)(g_GameFlags1 + 7);
  g_PathDirection1 = -1;
  g_PathDirection2 = -1;
  g_RenderComplete = 0;
  return -1;
}

int __cdecl ComputeDirectDirection()
{
  int v0; // esi
  int v1; // ebx
  int v3; // eax
  int v4; // eax
  int v5; // eax
  int v6; // eax
  int v7; // edx
  int v8; // ecx

  v0 = g_PathTargetX - g_PathStartX;
  if ( g_PathTargetX - g_PathStartX <= 0 )
    v0 = g_PathStartX - g_PathTargetX;
  v1 = g_PathTargetY - g_PathStartY;
  if ( g_PathTargetY - g_PathStartY <= 0 )
    v1 = g_PathStartY - g_PathTargetY;
  if ( !v0 && !v1 )
    return 0;
  if ( v0 && v1 / v0 < 4 )
  {
    if ( v1 && v0 / v1 < 4 )
    {
      if ( g_PathTargetX >= g_PathStartX )
      {
        v7 = 5;
        if ( g_PathTargetY >= g_PathStartY )
          LOBYTE(v7) = 7;
      }
      else
      {
        v7 = 3;
        if ( g_PathTargetY >= g_PathStartY )
          LOBYTE(v7) = 1;
      }
      g_PathDirection2 = v7;
      if ( v0 <= v1 )
      {
        v8 = 4;
        if ( g_PathTargetY >= g_PathStartY )
          v8 = 0;
      }
      else
      {
        v8 = 2;
        if ( g_PathTargetX >= g_PathStartX )
          LOBYTE(v8) = 6;
      }
      g_PathDirection1 = v8;
    }
    else
    {
      v5 = 2;
      if ( v0 >= 2 )
      {
        if ( g_PathTargetX >= g_PathStartX )
          LOBYTE(v5) = 6;
        g_PathDirection2 = v5;
      }
      v6 = 2;
      if ( g_PathTargetX >= g_PathStartX )
        LOBYTE(v6) = 6;
      g_PathDirection1 = v6;
    }
  }
  else
  {
    if ( v1 >= 2 )
    {
      v3 = 4;
      if ( g_PathTargetY >= g_PathStartY )
        v3 = 0;
      g_PathDirection2 = v3;
    }
    v4 = 4;
    if ( g_PathTargetY >= g_PathStartY )
      v4 = 0;
    g_PathDirection1 = v4;
  }
  g_RenderComplete = 1;
  return 1;
}

int __stdcall TracePathAlongWave(unsigned __int8 wave_value)
{
  int curX; // esi
  int curY; // edi
  int prevX; // ebp
  int bufIdx; // ebx
  int distX; // edx
  int distY; // eax
  int dir1; // edx
  int dir2; // eax
  int prevY; // [esp+10h] [ebp-Ch]
  int prev2X; // [esp+14h] [ebp-8h]
  int prev2Y; // [esp+18h] [ebp-4h]

  curX = g_PathStartX;                          // Trace path from start position to target along wave-filled tiles.
                                                // Uses flood-fill wave values (4,5,6) to follow the shortest path.
                                                // Sets g_PathDirection1 (primary) and g_PathDirection2 (secondary) on success.
                                                // Directions: 0=S, 1=SW, 2=W, 3=NW, 4=N, 5=NE, 6=E, 7=SE
  curY = g_PathStartY;
  prevX = g_PathStartX;
  prevY = g_PathStartY;
  prev2X = -1;
  prev2Y = -1;
  bufIdx = g_PathStartX + g_TileMapPtr + g_ScreenWidth * g_PathStartY;// bufIdx = linear index into tile map: base + x + y * stride
  while ( 1 )
  {
    distX = g_PathTargetX - curX;               // Compute Manhattan distances to target; prefer horizontal or vertical movement based on which axis has greater distance
    if ( g_PathTargetX - curX <= 0 )
      distX = curX - g_PathTargetX;
    distY = g_PathTargetY - curY;
    if ( g_PathTargetY - curY <= 0 )
      distY = curY - g_PathTargetY;
    if ( distX <= distY )                       // distX <= distY: prefer vertical movement (try up, down, left, right)
    {
      if ( !curY || *(char *)(bufIdx + g_NegScreenWidth) != wave_value )
      {
        if ( curY >= g_MapMaxY || *(char *)(bufIdx + g_ScreenWidth) != wave_value )
        {
          if ( !curX || *(char *)(bufIdx - 1) != wave_value )
          {
            if ( curX >= g_MapMaxX || *(char *)(bufIdx + 1) != wave_value )
              return distY;
LABEL_53:
            ++curX;
            ++bufIdx;
            goto LABEL_16;
          }
LABEL_50:
          --curX;
          --bufIdx;
          goto LABEL_16;
        }
        goto LABEL_47;
      }
    }
    else
    {
      if ( curX && *(char *)(bufIdx - 1) == wave_value )
        goto LABEL_50;                          // distX > distY: prefer horizontal movement (try left, right, up, down)
      if ( curX < g_MapMaxX && *(char *)(bufIdx + 1) == wave_value )
        goto LABEL_53;
      if ( !curY || *(char *)(bufIdx + g_NegScreenWidth) != wave_value )
      {
        if ( curY >= g_MapMaxY || *(char *)(bufIdx + g_ScreenWidth) != wave_value )
          return distY;
LABEL_47:
        ++curY;
        bufIdx += g_ScreenWidth;
        goto LABEL_16;
      }
    }
    --curY;
    bufIdx -= g_ScreenWidth;
LABEL_16:
    if ( curX == g_PathTargetX && curY == g_PathTargetY )
      break;                                    // Check if we've reached the target position
    prev2X = prevX;                             // Save previous positions for direction computation at the end
    prev2Y = prevY;
    prevX = curX;
    prevY = curY;
    switch ( wave_value )
    {                                           // Rotate wave value for next step: 4->6, 5->4, 6->5 (reverse of flood-fill order)
      case 4u:
        wave_value = 6;
        break;
      case 5u:
        wave_value = 4;
        break;
      case 6u:
        wave_value = 5;
        break;
    }
  }
  dir1 = 2;                                     // Reached target: compute primary direction from prevX/prevY to curX/curY
  if ( curX >= prevX )
  {
    LOBYTE(dir1) = 6;
    if ( curX <= prevX )
    {
      LOBYTE(dir1) = 4;
      if ( curY >= prevY )
        dir1 = 0;
    }
  }
  g_PathDirection1 = dir1;
  if ( prev2X != -1 )                           // If we have a second-to-last position, compute secondary direction from prev2X/prev2Y to curX/curY
  {
    if ( curY >= prev2Y )
    {
      if ( curY <= prev2Y )
      {
        dir2 = 2;
        if ( curX >= prev2X )
          LOBYTE(dir2) = 6;
      }
      else
      {
        dir2 = 1;
        if ( curX >= prev2X )
        {
          LOBYTE(dir2) = 7;
          if ( curX <= prev2X )
            dir2 = 0;
        }
      }
    }
    else
    {
      dir2 = 3;
      if ( curX >= prev2X )
      {
        LOBYTE(dir2) = 5;
        if ( curX <= prev2X )
          --dir2;
      }
    }
    g_PathDirection2 = dir2;
  }
  g_RenderComplete = 1;                         // Path found successfully, signal render update
  return 1;
}

char __cdecl ExpandWave_Pass1()
{
  int v0; // ecx
  int v1; // ebx
  _BYTE *i; // edx
  char v4; // [esp+4h] [ebp-4h]

  v0 = 0;
  v1 = 0;
  v4 = 0;
  for ( i = (_BYTE *)g_TileMapPtr; !*i; ++i )
  {
    if ( v0 && *(i - 1) == 6
      || v0 < g_MapMaxX && i[1] == 6
      || v1 && i[g_NegScreenWidth] == 6
      || v1 < g_MapMaxY && i[g_ScreenWidth] == 6 )
    {
      v4 = 4;
      *i = 4;
    }
LABEL_12:
    if ( ++v0 >= g_ScreenWidth )
    {
      if ( ++v1 >= g_ScreenHeight )
        return v4;
      v0 = 0;
    }
  }
  if ( *i != 2
    || (!v0 || *(i - 1) != 6)
    && (v0 >= g_MapMaxX || i[1] != 6)
    && (!v1 || i[g_NegScreenWidth] != 6)
    && (v1 >= g_MapMaxY || i[g_ScreenWidth] != 6) )
  {
    goto LABEL_12;
  }
  g_PathStartX = v0;
  g_PathStartY = v1;
  return 1;
}

char __cdecl ExpandWave_Pass2()
{
  int v0; // ecx
  int v1; // ebx
  _BYTE *i; // edx
  char v4; // [esp+4h] [ebp-4h]

  v0 = 0;
  v1 = 0;
  v4 = 0;
  for ( i = (_BYTE *)g_TileMapPtr; !*i; ++i )
  {
    if ( v0 && *(i - 1) == 4
      || v0 < g_MapMaxX && i[1] == 4
      || v1 && i[g_NegScreenWidth] == 4
      || v1 < g_MapMaxY && i[g_ScreenWidth] == 4 )
    {
      v4 = 5;
      *i = 5;
    }
LABEL_12:
    if ( ++v0 >= g_ScreenWidth )
    {
      if ( ++v1 >= g_ScreenHeight )
        return v4;
      v0 = 0;
    }
  }
  if ( *i != 2
    || (!v0 || *(i - 1) != 4)
    && (v0 >= g_MapMaxX || i[1] != 4)
    && (!v1 || i[g_NegScreenWidth] != 4)
    && (v1 >= g_MapMaxY || i[g_ScreenWidth] != 4) )
  {
    goto LABEL_12;
  }
  g_PathStartX = v0;
  g_PathStartY = v1;
  return 1;
}

char __cdecl ExpandWave_Pass3()
{
  int v0; // ecx
  int v1; // ebx
  _BYTE *i; // edx
  char v4; // [esp+4h] [ebp-4h]

  v0 = 0;
  v1 = 0;
  v4 = 0;
  for ( i = (_BYTE *)g_TileMapPtr; !*i; ++i )
  {
    if ( v0 && *(i - 1) == 5
      || v0 < g_MapMaxX && i[1] == 5
      || v1 && i[g_NegScreenWidth] == 5
      || v1 < g_MapMaxY && i[g_ScreenWidth] == 5 )
    {
      v4 = 6;
      *i = 6;
    }
LABEL_12:
    if ( ++v0 >= g_ScreenWidth )
    {
      if ( ++v1 >= g_ScreenHeight )
        return v4;
      v0 = 0;
    }
  }
  if ( *i != 2
    || (!v0 || *(i - 1) != 5)
    && (v0 >= g_MapMaxX || i[1] != 5)
    && (!v1 || i[g_NegScreenWidth] != 5)
    && (v1 >= g_MapMaxY || i[g_ScreenWidth] != 5) )
  {
    goto LABEL_12;
  }
  g_PathStartX = v0;
  g_PathStartY = v1;
  return 1;
}

int __stdcall FindPathToTarget(char use_direct_direction)
{
  char v1; // al
  char v2; // al
  char v3; // al

  *(_BYTE *)(g_TileMapPtr + g_ScreenWidth * g_PathTargetY + g_PathTargetX) = 6;
  do
  {
    v1 = ExpandWave_Pass1();
    if ( !v1 )
      return 0;
    if ( v1 == 1 )
    {
      if ( !use_direct_direction )
        return TracePathAlongWave(6u);
      return ComputeDirectDirection();
    }
    v2 = ExpandWave_Pass2();
    if ( !v2 )
      return 0;
    if ( v2 == 1 )
    {
      if ( !use_direct_direction )
        return TracePathAlongWave(4u);
      return ComputeDirectDirection();
    }
    v3 = ExpandWave_Pass3();
    if ( !v3 )
      return 0;
  }
  while ( v3 != 1 );
  if ( use_direct_direction )
    return ComputeDirectDirection();
  return TracePathAlongWave(5u);
}

int __stdcall BuildWalkMapDirectByType(unsigned __int8 a1)
{
  char *v1; // esi
  int v2; // edi
  _BYTE *v3; // edx
  int v4; // ebx
  char v5; // cl

  v1 = (char *)g_TileMapPtr;
  v2 = 0;
  do
  {
    v3 = (_BYTE *)(g_TileMapBuffer + 2 * (v2 << g_TileMapShift));
    v4 = 0;
    do
    {
      v5 = 2;
      if ( *v3 != a1 )
        v5 = 0;
      *v1 = v5;
      v3 += 2;
      ++v1;
      ++v4;
    }
    while ( v4 < g_ScreenWidth );
    ++v2;
  }
  while ( v2 < g_ScreenHeight );
  return FindPathToTarget(1);
}

int __stdcall BuildWalkMapObstaclesByType(unsigned __int8 a1)
{
  char *v1; // esi
  int v2; // edi
  _BYTE *v3; // edx
  int v4; // ebx
  char v5; // al

  v1 = (char *)g_TileMapPtr;
  v2 = 0;
  do
  {
    v3 = (_BYTE *)(g_TileMapBuffer + 2 * (v2 << g_TileMapShift));
    v4 = 0;
    do
    {
      v5 = 0;
      if ( *v3 != 0xFE )
      {
        v5 = 2;
        if ( *v3 != a1 )
          v5 = 3;
      }
      *v1 = v5;
      v3 += 2;
      ++v1;
      ++v4;
    }
    while ( v4 < g_ScreenWidth );
    ++v2;
  }
  while ( v2 < g_ScreenHeight );
  return FindPathToTarget(0);
}

int BuildWalkMapAllPassable()
{
  char *v0; // esi
  int v1; // edi
  _BYTE *v2; // edx
  int v3; // ebx
  char v4; // cl

  v0 = (char *)g_TileMapPtr;
  v1 = 0;
  do
  {
    v2 = (_BYTE *)(g_TileMapBuffer + 2 * (v1 << g_TileMapShift));
    v3 = 0;
    do
    {
      v4 = 0;
      if ( *v2 != 0xFE )
        v4 = 3;
      *v0 = v4;
      v2 += 2;
      ++v0;
      ++v3;
    }
    while ( v3 < g_ScreenWidth );
    ++v1;
  }
  while ( v1 < g_ScreenHeight );
  *(_BYTE *)(g_TileMapPtr + g_ScreenWidth * g_PathStartY + g_PathStartX) = 2;
  return FindPathToTarget(0);
}

int __stdcall BuildWalkMapByTileType(unsigned __int8 a1)
{
  char *v1; // esi
  int v2; // edi
  _BYTE *v3; // edx
  int v4; // ebx
  char v5; // cl

  v1 = (char *)g_TileMapPtr;
  v2 = 0;
  do
  {
    v3 = (_BYTE *)(g_TileMapBuffer + 2 * (v2 << g_TileMapShift));
    v4 = 0;
    do
    {
      v5 = 0;
      if ( *v3 != a1 )
        v5 = 3;
      *v1 = v5;
      v3 += 2;
      ++v1;
      ++v4;
    }
    while ( v4 < g_ScreenWidth );
    ++v2;
  }
  while ( v2 < g_ScreenHeight );
  *(_BYTE *)(g_TileMapPtr + g_ScreenWidth * g_PathStartY + g_PathStartX) = 2;
  return FindPathToTarget(0);
}

int __stdcall PathfindByTileType(unsigned __int8 pathfind_mode, char tile_type, unsigned __int8 search_type)
{
  SetupPathfindPositions(search_type);
  if ( search_type == 130 || search_type == 131 )
  {
    if ( search_type != (unsigned __int8)g_PreviousInput || tile_type != -1 && tile_type != (char)*((_BYTE*)&g_PreviousScreen + 3) )
      return 0;
  }
  else
  {
    if ( search_type != 14 )
    {
      if ( pathfind_mode == 0xFF )
        return BuildWalkMapDirectByType(tile_type);
      else
        return BuildWalkMapObstaclesByType(tile_type);
    }
    if ( tile_type != -1 && tile_type != (char)*((_BYTE*)&video_recursion_counter + 3) )
      return 0;
  }
  if ( pathfind_mode == 255 )
    return ComputeDirectDirection();
  if ( pathfind_mode == 254 )
    return BuildWalkMapAllPassable();
  return BuildWalkMapByTileType(tile_type);
}

int __stdcall BuildWalkMapDirect(unsigned __int8 *bitmask)
{
  _BYTE *v1; // edi
  int v2; // ebx
  _BYTE *v4; // [esp+10h] [ebp-8h]
  int v5; // [esp+14h] [ebp-4h]

  v4 = (_BYTE *)g_TileMapPtr;
  v5 = 0;
  do
  {
    v1 = (_BYTE *)(g_TileMapBuffer + 2 * (v5 << g_TileMapShift));
    v2 = 0;
    do
    {
      *v4 = (bitmask[(int)(unsigned __int8)*v1 >> 3] & (1 << (*v1 & 7))) == 0 ? 0 : 2;
      v1 += 2;
      ++v4;
      ++v2;
    }
    while ( v2 < g_ScreenWidth );
    ++v5;
  }
  while ( v5 < g_ScreenHeight );
  return FindPathToTarget(1);
}

int __stdcall BuildWalkMapWithObstacles(unsigned __int8 *bitmask)
{
  unsigned __int8 *v1; // edi
  int v2; // ebx
  int v3; // esi
  char v4; // cl
  char *v6; // [esp+10h] [ebp-8h]
  int v7; // [esp+14h] [ebp-4h]

  v6 = (char *)g_TileMapPtr;
  v7 = 0;
  do
  {
    v1 = (unsigned __int8 *)(g_TileMapBuffer + 2 * (v7 << g_TileMapShift));
    v2 = 0;
    do
    {
      v3 = *v1;
      v4 = 0;
      if ( v3 != 254 )
        v4 = (bitmask[v3 >> 3] & (1 << (v3 & 7))) == 0 ? 3 : 2;
      *v6 = v4;
      v1 += 2;
      ++v6;
      ++v2;
    }
    while ( v2 < g_ScreenWidth );
    ++v7;
  }
  while ( v7 < g_ScreenHeight );
  return FindPathToTarget(0);
}

int __stdcall BuildWalkMapInverted(unsigned __int8 *bitmask)
{
  _BYTE *v1; // edi
  int v2; // ebx
  _BYTE *v4; // [esp+10h] [ebp-8h]
  int v5; // [esp+14h] [ebp-4h]

  v4 = (_BYTE *)g_TileMapPtr;
  v5 = 0;
  do
  {
    v1 = (_BYTE *)(g_TileMapBuffer + 2 * (v5 << g_TileMapShift));
    v2 = 0;
    do
    {
      *v4 = (bitmask[(int)(unsigned __int8)*v1 >> 3] & (1 << (*v1 & 7))) == 0 ? 3 : 0;
      v1 += 2;
      ++v4;
      ++v2;
    }
    while ( v2 < g_ScreenWidth );
    ++v5;
  }
  while ( v5 < g_ScreenHeight );
  *(_BYTE *)(g_TileMapPtr + g_ScreenWidth * g_PathStartY + g_PathStartX) = 2;
  return FindPathToTarget(0);
}

int __stdcall PathfindWithBitmask(unsigned __int8 pathfind_mode, unsigned __int8 anim_index, char search_type)
{
  unsigned __int8 *bitmaskData; // esi

  SetupPathfindPositions(search_type);          // Initialize start/target positions based on search_type
  bitmaskData = (unsigned __int8 *)g_AnimationSlotArray[anim_index].frameBitmask;// Get the walkability bitmask from the animation slot
  if ( (unsigned __int8)search_type == 130 || (unsigned __int8)search_type == 131 )// search_type 0x82/0x83 (selected tile): verify search_type matches previous input
                                                // AND target tile is walkable in bitmask before proceeding
  {
    if ( search_type != g_PreviousInput
      || ((1 << (HIBYTE(g_PreviousScreen) & 7)) & bitmaskData[(int)HIBYTE(g_PreviousScreen) >> 3]) == 0 )
    {
      return 0;                                 // Bitmask test: check if tile (HIBYTE(g_PreviousScreen)) is set in bitmaskData.
                                                // Bit index = tile & 7, byte index = tile >> 3
    }
  }
  else
  {                                             // search_type != 14 (normal pathfinding): build walk map and run pathfinder
    if ( search_type != 14 )
    {                                           // pathfind_mode 0xFF: build walk map with direct passability check (0=blocked, 2=walkable)
      if ( pathfind_mode == 0xFF )
        return BuildWalkMapDirect((int)bitmaskData);
      else
        return BuildWalkMapWithObstacles((int)g_AnimationSlotArray[anim_index].frameBitmask);// Other pathfind_mode: build walk map with obstacle marking (0=special, 2=walkable, 3=obstacle)
    }
    if ( ((1 << (*((_BYTE*)&video_recursion_counter + 3) & 7)) & bitmaskData[(int)*((_BYTE*)&video_recursion_counter + 3) >> 3]) == 0 )// search_type 14: check if current tile (*(&video_recursion_counter+3)) is walkable in bitmask
      return 0;
  }
  if ( pathfind_mode == 255 )                   // After validation: dispatch based on pathfind_mode
    return ComputeDirectDirection();            // pathfind_mode 0xFF: compute direction directly from dx/dy without wave fill
  if ( pathfind_mode == 254 )
    return BuildWalkMapAllPassable();           // pathfind_mode 0xFE: build all-passable map (only tile 0xFE is blocked)
  return BuildWalkMapInverted((int)g_AnimationSlotArray[anim_index].frameBitmask);// Specific anim index: build inverted walk map from bitmask (3=passable, 0=blocked, start=2)
}

#pragma pack(pop)
