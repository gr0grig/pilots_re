/*
 * Поиск пути волной по карте проходимости g_WalkMap (по клетке на тайл, строки по
 * g_World.mapWidth). Метки клеток — GM_WALK_CELL: 0 свободна, 2 цель, 3 непроходима,
 * 4/5/6 — фронты волны, идущей от объекта (6 → 4 → 5 → 6 …).
 * Результат — направления первого шага объекта g_PathDir4/g_PathDir8 (GM_DIR8, −1 — нет).
 */
#include "gm.h"

#include <stdlib.h>

/* Результат шага волны помимо метки нового фронта. */
enum
{
    WAVE_EXHAUSTED = 0,    /* волна не расширилась — пути нет */
    WAVE_REACHED_GOAL = 1, /* волна дошла до клетки-цели */
};

/* Подготовить поиск пути: цель — тайл щелчка (для событий действия) или тайл под
   указателем, начало — тайл текущего объекта; направления сбрасываются в −1, путь не найден.
   Возвращает −1. */
int GmPath_Setup(GM_EVENT target)
{
    if (target == GMEV_ACTION1 || target == GMEV_ACTION2)
    {
        g_PathGoalX = g_ClickTileX;
        g_PathGoalY = g_ClickTileY;
    }
    else
    {
        g_PathGoalX = g_PointerTileX;
        g_PathGoalY = g_PointerTileY;
    }
    g_PathOriginX = g_CurObject->tile.x;
    g_PathOriginY = g_CurObject->tile.y;
    g_PathDir4 = -1;
    g_PathDir8 = -1;
    g_PathFound = false;
    return -1;
}

/* Направление без карты, по разности цели и объекта. Если разности соизмеримы (каждая
   меньше учетверённой другой) — диагональ в g_PathDir8 и большая ось в g_PathDir4.
   Иначе направление по главной оси; в g_PathDir8 оно пишется, только если по этой оси
   не меньше двух шагов. Возвращает 0, если цель совпадает с объектом, иначе 1. */
int GmPath_DirectDirection(void)
{
    int dx = abs(g_PathOriginX - g_PathGoalX);
    int dy = abs(g_PathOriginY - g_PathGoalY);
    bool goalEast = g_PathOriginX < g_PathGoalX;
    bool goalSouth = g_PathOriginY < g_PathGoalY;

    if (dx == 0 && dy == 0)
        return 0;

    if (dx != 0 && dy / dx < 4)
    {
        if (dy != 0 && dx / dy < 4)
        {
            if (goalEast)
                g_PathDir8 = goalSouth ? GMDIR8_SE : GMDIR8_NE;
            else
                g_PathDir8 = goalSouth ? GMDIR8_SW : GMDIR8_NW;

            if (dx > dy)
                g_PathDir4 = goalEast ? GMDIR8_E : GMDIR8_W;
            else
                g_PathDir4 = goalSouth ? GMDIR8_S : GMDIR8_N;
        }
        else
        {
            if (dx >= 2)
                g_PathDir8 = goalEast ? GMDIR8_E : GMDIR8_W;
            g_PathDir4 = goalEast ? GMDIR8_E : GMDIR8_W;
        }
    }
    else
    {
        if (dy >= 2)
            g_PathDir8 = goalSouth ? GMDIR8_S : GMDIR8_N;
        g_PathDir4 = goalSouth ? GMDIR8_S : GMDIR8_N;
    }

    g_PathFound = true;
    return 1;
}

/* Метка предыдущего фронта волны (ближе к объекту): 4 ← 6, 5 ← 4, 6 ← 5. */
static uint8_t Wave_PrevMark(uint8_t mark)
{
    switch (mark)
    {
    case GMWALK_MARK4:
        return GMWALK_MARK6;
    case GMWALK_MARK5:
        return GMWALK_MARK4;
    case GMWALK_MARK6:
        return GMWALK_MARK5;
    default:
        return mark;
    }
}

/* Проследить путь от цели к объекту по убывающим фронтам волны (lastWave — метка фронта,
   дошедшего до цели) и вычислить направления первого шага объекта: g_PathDir4 — по клетке,
   соседней с объектом, g_PathDir8 — по клетке через одну (диагональ), если путь длиннее
   одного шага. На каждом шаге предпочитается ось с большим расстоянием до объекта.
   Возвращает 1, если путь прослежен. */
int GmPath_Trace(uint8_t lastWave)
{
    int curX = g_PathGoalX;
    int curY = g_PathGoalY;
    int prevX = curX;
    int prevY = curY;
    int prevPrevX = -1;
    int prevPrevY = -1;
    const char *cell = &g_WalkMap[g_World.mapWidth * g_PathGoalY + g_PathGoalX];

    for (;;)
    {
        int distX = abs(g_PathOriginX - curX);
        int distY = abs(g_PathOriginY - curY);
        int stepX = 0;
        int stepY = 0;

        if (distX > distY)
        {
            if (curX != 0 && cell[-1] == lastWave)
                stepX = -1;
            else if (curX < g_MapMaxX && cell[1] == lastWave)
                stepX = 1;
            else if (curY != 0 && cell[g_WalkMapRowUp] == lastWave)
                stepY = -1;
            else if (curY < g_MapMaxY && cell[g_World.mapWidth] == lastWave)
                stepY = 1;
        }
        else
        {
            if (curY != 0 && cell[g_WalkMapRowUp] == lastWave)
                stepY = -1;
            else if (curY < g_MapMaxY && cell[g_World.mapWidth] == lastWave)
                stepY = 1;
            else if (curX != 0 && cell[-1] == lastWave)
                stepX = -1;
            else if (curX < g_MapMaxX && cell[1] == lastWave)
                stepX = 1;
        }

        /* Особенность оригинала: если волна оборвалась (у клетки нет соседа с ожидаемой
           меткой), возвращается расстояние по вертикали до объекта — ненулевое значение
           вызывающие примут за успех, хотя g_PathFound не поднят. При корректной волне
           это недостижимо. */
        if (stepX == 0 && stepY == 0)
            return distY;

        curX += stepX;
        curY += stepY;
        cell += stepX + stepY * g_World.mapWidth;
        if (curX == g_PathOriginX && curY == g_PathOriginY)
            break;

        prevPrevX = prevX;
        prevPrevY = prevY;
        prevX = curX;
        prevY = curY;
        lastWave = Wave_PrevMark(lastWave);
    }

    /* (curX, curY) — клетка объекта, prev — первый шаг пути, prevPrev — второй. */
    if (curX < prevX)
        g_PathDir4 = GMDIR8_E;
    else if (curX > prevX)
        g_PathDir4 = GMDIR8_W;
    else if (curY < prevY)
        g_PathDir4 = GMDIR8_S;
    else
        g_PathDir4 = GMDIR8_N;

    if (prevPrevX != -1)
    {
        if (curY < prevPrevY)
        {
            if (curX < prevPrevX)
                g_PathDir8 = GMDIR8_SE;
            else if (curX > prevPrevX)
                g_PathDir8 = GMDIR8_SW;
            else
                g_PathDir8 = GMDIR8_S;
        }
        else if (curY > prevPrevY)
        {
            if (curX < prevPrevX)
                g_PathDir8 = GMDIR8_NE;
            else if (curX > prevPrevX)
                g_PathDir8 = GMDIR8_NW;
            else
                g_PathDir8 = GMDIR8_N;
        }
        else
        {
            g_PathDir8 = (curX < prevPrevX) ? GMDIR8_E : GMDIR8_W;
        }
    }

    g_PathFound = true;
    return 1;
}

/* Есть ли у клетки (x, y) карты проходимости сосед (по четырём сторонам) с меткой mark. */
static bool Walk_HasNeighbour(const char *cell, int x, int y, char mark)
{
    return (x != 0 && cell[-1] == mark)
        || (x < g_MapMaxX && cell[1] == mark)
        || (y != 0 && cell[g_WalkMapRowUp] == mark)
        || (y < g_MapMaxY && cell[g_World.mapWidth] == mark);
}

/* Шаг волны: свободные клетки рядом с меткой from получают метку to. Если рядом с from
   оказалась клетка-цель, её координаты становятся целью пути (g_PathGoalX/Y) и
   возвращается WAVE_REACHED_GOAL; WAVE_EXHAUSTED — волна иссякла; иначе — метка to. */
static uint8_t Wave_Expand(char from, char to)
{
    char *cell = g_WalkMap;
    uint8_t result = WAVE_EXHAUSTED;
    int x;
    int y;

    for (y = 0; y < g_World.mapHeight; y++)
    {
        for (x = 0; x < g_World.mapWidth; x++, cell++)
        {
            if (*cell == GMWALK_OPEN)
            {
                if (Walk_HasNeighbour(cell, x, y, from))
                {
                    *cell = to;
                    result = (uint8_t)to;
                }
            }
            else if (*cell == GMWALK_GOAL && Walk_HasNeighbour(cell, x, y, from))
            {
                g_PathGoalX = x;
                g_PathGoalY = y;
                return WAVE_REACHED_GOAL;
            }
        }
    }
    return result;
}

/* Шаг волны 6 → 4 (результат: 0 — иссякла, 1 — цель достигнута, 4 — продолжить). */
uint8_t GmPath_ExpandWave4From6(void)
{
    return Wave_Expand(GMWALK_MARK6, GMWALK_MARK4);
}

/* Шаг волны 4 → 5 (результат: 0 — иссякла, 1 — цель достигнута, 5 — продолжить). */
uint8_t GmPath_ExpandWave5From4(void)
{
    return Wave_Expand(GMWALK_MARK4, GMWALK_MARK5);
}

/* Шаг волны 5 → 6 (результат: 0 — иссякла, 1 — цель достигнута, 6 — продолжить). */
uint8_t GmPath_ExpandWave6From5(void)
{
    return Wave_Expand(GMWALK_MARK5, GMWALK_MARK6);
}

/* Завершить найденный волной путь: прямое направление (directionOnly) или прослеживание
   от цели по фронту lastWave. */
static int Path_Finish(bool directionOnly, uint8_t lastWave)
{
    if (directionOnly)
        return GmPath_DirectDirection();
    return GmPath_Trace(lastWave);
}

/* Волновой поиск: клетка объекта получает метку 6, шаги 6→4→5→6 повторяются до
   достижения цели или исчерпания волны (тогда 0). С directionOnly вместо прослеживания
   берётся прямое направление к найденной цели. */
int GmPath_Search(bool directionOnly)
{
    g_WalkMap[g_World.mapWidth * g_PathOriginY + g_PathOriginX] = GMWALK_MARK6;
    for (;;)
    {
        uint8_t step;

        step = GmPath_ExpandWave4From6();
        if (step == WAVE_EXHAUSTED)
            return 0;
        if (step == WAVE_REACHED_GOAL)
            return Path_Finish(directionOnly, GMWALK_MARK6);

        step = GmPath_ExpandWave5From4();
        if (step == WAVE_EXHAUSTED)
            return 0;
        if (step == WAVE_REACHED_GOAL)
            return Path_Finish(directionOnly, GMWALK_MARK4);

        step = GmPath_ExpandWave6From5();
        if (step == WAVE_EXHAUSTED)
            return 0;
        if (step == WAVE_REACHED_GOAL)
            return Path_Finish(directionOnly, GMWALK_MARK5);
    }
}

/* ========================================================================== */
/* Построение карты проходимости                                              */
/* ========================================================================== */

/* Входит ли класс в набор (битовое множество членов набора). */
static bool ClassSet_Contains(const uint8_t *memberBits, uint8_t classIndex)
{
    return (memberBits[classIndex >> 3] & (1 << (classIndex & 7))) != 0;
}

/* Поставить цель пути (g_PathGoalX/Y) на карте проходимости. */
static void Walk_MarkGoal(void)
{
    g_WalkMap[g_World.mapWidth * g_PathGoalY + g_PathGoalX] = GMWALK_GOAL;
}

/* Карта: тайлы класса classIndex — цели, прочие свободны; ищется только направление
   к ближайшей цели. */
int GmWalkMap_GoalsClassNoObstacles(uint8_t classIndex)
{
    char *walk = g_WalkMap;
    int x;
    int y;

    for (y = 0; y < g_World.mapHeight; y++)
    {
        const GM_TILE *tile = &g_TileMap[y << g_TileMapShift];

        for (x = 0; x < g_World.mapWidth; x++, tile++)
            *walk++ = tile->classIndex == classIndex ? GMWALK_GOAL : GMWALK_OPEN;
    }
    return GmPath_Search(true);
}

/* Карта: пустые тайлы свободны, тайлы класса classIndex — цели, прочие непроходимы;
   полный поиск пути к ближайшей цели. */
int GmWalkMap_GoalsClassEmptyFree(uint8_t classIndex)
{
    char *walk = g_WalkMap;
    int x;
    int y;

    for (y = 0; y < g_World.mapHeight; y++)
    {
        const GM_TILE *tile = &g_TileMap[y << g_TileMapShift];

        for (x = 0; x < g_World.mapWidth; x++, tile++)
        {
            if (tile->classIndex == GMCLASS_EMPTY_TILE)
                *walk++ = GMWALK_OPEN;
            else
                *walk++ = tile->classIndex == classIndex ? GMWALK_GOAL : GMWALK_BLOCKED;
        }
    }
    return GmPath_Search(false);
}

/* Карта: пустые тайлы свободны, прочие непроходимы, цель — тайл цели пути. */
int GmWalkMap_EmptyFree(void)
{
    char *walk = g_WalkMap;
    int x;
    int y;

    for (y = 0; y < g_World.mapHeight; y++)
    {
        const GM_TILE *tile = &g_TileMap[y << g_TileMapShift];

        for (x = 0; x < g_World.mapWidth; x++, tile++)
            *walk++ = tile->classIndex == GMCLASS_EMPTY_TILE ? GMWALK_OPEN : GMWALK_BLOCKED;
    }
    Walk_MarkGoal();
    return GmPath_Search(false);
}

/* Карта: тайлы класса classIndex свободны, прочие непроходимы, цель — тайл цели пути. */
int GmWalkMap_ClassFree(uint8_t classIndex)
{
    char *walk = g_WalkMap;
    int x;
    int y;

    for (y = 0; y < g_World.mapHeight; y++)
    {
        const GM_TILE *tile = &g_TileMap[y << g_TileMapShift];

        for (x = 0; x < g_World.mapWidth; x++, tile++)
            *walk++ = tile->classIndex == classIndex ? GMWALK_OPEN : GMWALK_BLOCKED;
    }
    Walk_MarkGoal();
    return GmPath_Search(false);
}

/* Карта: тайлы классов набора — цели, прочие свободны; только направление. */
int GmWalkMap_GoalsInSetNoObstacles(const uint8_t *memberBits)
{
    char *walk = g_WalkMap;
    int x;
    int y;

    for (y = 0; y < g_World.mapHeight; y++)
    {
        const GM_TILE *tile = &g_TileMap[y << g_TileMapShift];

        for (x = 0; x < g_World.mapWidth; x++, tile++)
            *walk++ = ClassSet_Contains(memberBits, tile->classIndex) ? GMWALK_GOAL : GMWALK_OPEN;
    }
    return GmPath_Search(true);
}

/* Карта: пустые тайлы свободны, классы набора — цели, прочие непроходимы; полный поиск. */
int GmWalkMap_GoalsInSetEmptyFree(const uint8_t *memberBits)
{
    char *walk = g_WalkMap;
    int x;
    int y;

    for (y = 0; y < g_World.mapHeight; y++)
    {
        const GM_TILE *tile = &g_TileMap[y << g_TileMapShift];

        for (x = 0; x < g_World.mapWidth; x++, tile++)
        {
            if (tile->classIndex == GMCLASS_EMPTY_TILE)
                *walk++ = GMWALK_OPEN;
            else
                *walk++ = ClassSet_Contains(memberBits, tile->classIndex) ? GMWALK_GOAL : GMWALK_BLOCKED;
        }
    }
    return GmPath_Search(false);
}

/* Карта: классы набора свободны, прочие непроходимы, цель — тайл цели пути. */
int GmWalkMap_SetFree(const uint8_t *memberBits)
{
    char *walk = g_WalkMap;
    int x;
    int y;

    for (y = 0; y < g_World.mapHeight; y++)
    {
        const GM_TILE *tile = &g_TileMap[y << g_TileMapShift];

        for (x = 0; x < g_World.mapWidth; x++, tile++)
            *walk++ = ClassSet_Contains(memberBits, tile->classIndex) ? GMWALK_OPEN : GMWALK_BLOCKED;
    }
    Walk_MarkGoal();
    return GmPath_Search(false);
}

/* ========================================================================== */
/* Поиск по классу и набору классов                                           */
/* ========================================================================== */

/* Поиск пути по классу classIndex (GMCLASS_ANY — любой). Для событий действия нужен
   совпадающий запомненный щелчок по тайлу класса, для указателя (GMEV_NONE) — класс тайла
   под указателем; затем mode GMSEEK_DIRECTION_ONLY — прямое направление,
   GMSEEK_THROUGH_EMPTY — путь по пустым тайлам, иначе путь по тайлам класса.
   Прочие target (GMEV_NO_TARGET) — к ближайшему тайлу класса: GMSEEK_DIRECTION_ONLY —
   только направление, иначе в обход. Возвращает ненулевое значение, если путь найден. */
int GmPath_SeekClass(GM_EVENT target, uint8_t classIndex, uint8_t mode)
{
    GmPath_Setup(target);
    if (target == GMEV_ACTION1 || target == GMEV_ACTION2)
    {
        if (target != g_ClickEvent || (classIndex != GMCLASS_ANY && classIndex != g_ClickTileClass))
            return 0;
    }
    else if (target != GMEV_NONE)
    {
        if (mode == GMSEEK_DIRECTION_ONLY)
            return GmWalkMap_GoalsClassNoObstacles(classIndex);
        return GmWalkMap_GoalsClassEmptyFree(classIndex);
    }
    else if (classIndex != GMCLASS_ANY && classIndex != g_PointerTileClass)
    {
        return 0;
    }

    if (mode == GMSEEK_DIRECTION_ONLY)
        return GmPath_DirectDirection();
    if (mode == GMSEEK_THROUGH_EMPTY)
        return GmWalkMap_EmptyFree();
    return GmWalkMap_ClassFree(classIndex);
}

/* Поиск пути по набору классов classSet (по битам членства); логика как у GmPath_SeekClass. */
int GmPath_SeekClassSet(GM_EVENT target, uint8_t classSet, uint8_t mode)
{
    const uint8_t *memberBits;

    GmPath_Setup(target);
    memberBits = g_ClassSets[classSet].memberBits;
    if (target == GMEV_ACTION1 || target == GMEV_ACTION2)
    {
        if (target != g_ClickEvent || !ClassSet_Contains(memberBits, g_ClickTileClass))
            return 0;
    }
    else if (target != GMEV_NONE)
    {
        if (mode == GMSEEK_DIRECTION_ONLY)
            return GmWalkMap_GoalsInSetNoObstacles(memberBits);
        return GmWalkMap_GoalsInSetEmptyFree(memberBits);
    }
    else if (!ClassSet_Contains(memberBits, g_PointerTileClass))
    {
        return 0;
    }

    if (mode == GMSEEK_DIRECTION_ONLY)
        return GmPath_DirectDirection();
    if (mode == GMSEEK_THROUGH_EMPTY)
        return GmWalkMap_EmptyFree();
    return GmWalkMap_SetFree(memberBits);
}
