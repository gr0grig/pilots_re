/*
 * Правила классов: преобразования клеток правила (повороты и отражения),
 * случайный выбор направления, выполнение одного правила (предусловие, образец,
 * ход, скрипт, размещение) и главный проход правил за такт.
 *
 * Таблицы обработчиков g_RuleXforms, g_RotateCellByDirection и
 * g_ResolveDirectionMask определены в data.c.
 */
#include "gm.h"

/* Младшая тетрада флагов клетки — признаки сличения (набор классов, «должна
   совпасть», режим); старшая — маска направлений. */
#define CELL_MATCH_BITS (GMCELL_BY_CLASS_SET | GMCELL_MUST_MATCH | GMCELL_MODE_MASK)

/* Число наборов совпадений образца, которые может запомнить одно правило. */
#define MAX_MATCH_RANGES 3

/* ========================================================================== */
/* Генератор случайных чисел ANSI C                                           */
/* ========================================================================== */

static void Random_Advance(void)
{
    g_RandomSeed = g_RandomSeed * 1103515245u + 12345u;
}

/* Случайное число в [0, n): старшее слово состояния, умноженное на n,
   затем генератор продвигается. */
static uint32_t Random_Below(uint32_t n)
{
    uint32_t value = (n * (g_RandomSeed >> 16)) >> 16;

    Random_Advance();
    return value;
}

/* ========================================================================== */
/* Преобразования клетки правила (экранные координаты: Y растёт вниз)         */
/* ========================================================================== */

/* Новые флаги клетки: направление — через таблицу преобразования, признаки сличения — как были. */
static uint8_t Xform_MapFlags(const uint8_t *dirMap, uint8_t flags)
{
    return (uint8_t)(dirMap[flags >> 4] | (flags & CELL_MATCH_BITS));
}

/* Тождественное преобразование (g_RuleXforms[0]; также поворот для направления «юг»). */
void GmXform_Identity(GM_RULE_CELL *cell)
{
    (void)cell;
}

/* Поворот на 90° по часовой: (dx,dy)→(−dy,dx), Ю→З→С→В→Ю. */
void GmXform_RotateCW(GM_RULE_CELL *cell)
{
    int8_t newDx = (int8_t)-cell->dy;

    cell->dy = cell->dx;
    cell->dx = newDx;
    cell->flags = Xform_MapFlags(g_XformDirMap_RotateCW, cell->flags);
}

/* Поворот на 180°: (dx,dy)→(−dx,−dy), Ю↔С, З↔В. */
void GmXform_Rotate180(GM_RULE_CELL *cell)
{
    cell->dy = (int8_t)-cell->dy;
    cell->dx = (int8_t)-cell->dx;
    cell->flags = Xform_MapFlags(g_XformDirMap_Rotate180, cell->flags);
}

/* Поворот на 90° против часовой: (dx,dy)→(dy,−dx), Ю→В→С→З→Ю. */
void GmXform_RotateCCW(GM_RULE_CELL *cell)
{
    int8_t newDy = (int8_t)-cell->dx;

    cell->dx = cell->dy;
    cell->dy = newDy;
    cell->flags = Xform_MapFlags(g_XformDirMap_RotateCCW, cell->flags);
}

/* Зеркало слева-направо: dx → −dx, З↔В. */
void GmXform_MirrorLeftRight(GM_RULE_CELL *cell)
{
    cell->dx = (int8_t)-cell->dx;
    cell->flags = Xform_MapFlags(g_XformDirMap_MirrorLeftRight, cell->flags);
}

/* Зеркало относительно диагонали ЮЗ–СВ: (dx,dy)→(−dy,−dx), Ю↔З, С↔В. */
void GmXform_MirrorSwNeAxis(GM_RULE_CELL *cell)
{
    int8_t newDy = (int8_t)-cell->dx;

    cell->dx = (int8_t)-cell->dy;
    cell->dy = newDy;
    cell->flags = Xform_MapFlags(g_XformDirMap_MirrorSwNeAxis, cell->flags);
}

/* Зеркало сверху-вниз: dy → −dy, Ю↔С. */
void GmXform_MirrorUpDown(GM_RULE_CELL *cell)
{
    cell->dy = (int8_t)-cell->dy;
    cell->flags = Xform_MapFlags(g_XformDirMap_MirrorUpDown, cell->flags);
}

/* Зеркало относительно диагонали СЗ–ЮВ: обмен dx и dy, Ю↔В, З↔С. */
void GmXform_MirrorNwSeAxis(GM_RULE_CELL *cell)
{
    int8_t dx = cell->dx;

    cell->dx = cell->dy;
    cell->dy = dx;
    cell->flags = Xform_MapFlags(g_XformDirMap_MirrorNwSeAxis, cell->flags);
}

/* Поворот по направлению для масок, которые не задают одно направление
   (0 и сочетания): клетка не меняется. */
void GmXform_NoneForMixedDirection(GM_RULE_CELL *cell)
{
    (void)cell;
}

/* ========================================================================== */
/* Выбор одного направления из маски (g_ResolveDirectionMask)                 */
/* ========================================================================== */

/* Записать во флаги клетки случайно выбранное из choices направление;
   признаки сличения при этом обнуляются. */
static void ResolveDir_Pick(GM_RULE_CELL *cell, const uint8_t *choices, uint32_t count)
{
    cell->flags = choices[Random_Below(count)];
}

/* В маске нет ни одного направления — клетка остаётся как есть. */
void GmResolveDir_None(GM_RULE_CELL *cell)
{
    (void)cell;
}

/* Маска уже задаёт одно направление: сбрасываются только признаки сличения. */
void GmResolveDir_Single(GM_RULE_CELL *cell)
{
    cell->flags &= GMCELL_DIR_MASK;
}

void GmResolveDir_S_W(GM_RULE_CELL *cell)   { ResolveDir_Pick(cell, g_DirChoices_S_W, 2); }
void GmResolveDir_S_N(GM_RULE_CELL *cell)   { ResolveDir_Pick(cell, g_DirChoices_S_N, 2); }
void GmResolveDir_W_N(GM_RULE_CELL *cell)   { ResolveDir_Pick(cell, g_DirChoices_W_N, 2); }
void GmResolveDir_S_W_N(GM_RULE_CELL *cell) { ResolveDir_Pick(cell, g_DirChoices_S_W_N, 3); }
void GmResolveDir_S_E(GM_RULE_CELL *cell)   { ResolveDir_Pick(cell, g_DirChoices_S_E, 2); }
void GmResolveDir_W_E(GM_RULE_CELL *cell)   { ResolveDir_Pick(cell, g_DirChoices_W_E, 2); }
void GmResolveDir_S_W_E(GM_RULE_CELL *cell) { ResolveDir_Pick(cell, g_DirChoices_S_W_E, 3); }
void GmResolveDir_N_E(GM_RULE_CELL *cell)   { ResolveDir_Pick(cell, g_DirChoices_N_E, 2); }
void GmResolveDir_S_N_E(GM_RULE_CELL *cell) { ResolveDir_Pick(cell, g_DirChoices_S_N_E, 3); }
void GmResolveDir_W_N_E(GM_RULE_CELL *cell) { ResolveDir_Pick(cell, g_DirChoices_W_N_E, 3); }
void GmResolveDir_Any(GM_RULE_CELL *cell)   { ResolveDir_Pick(cell, g_DirChoices_All, 4); }

/* ========================================================================== */
/* Плавное изменение громкости                                                */
/* ========================================================================== */

/* Сдвинуть *current к target не более чем на 255/частота кадров за вызов.
   Возвращает расстояние до цели перед шагом (0 — уже на месте). */
int GmStepToward(uint8_t *current, uint8_t target)
{
    int delta = target - *current;

    if (delta != 0)
    {
        int step = 255 / g_GameHeader.frameRate;

        if (delta < 0)
        {
            step = -step;
            if (step < delta)
                step = delta;
        }
        else if (step > delta)
        {
            step = delta;
        }
        *current = (uint8_t)(*current + step);
    }
    return delta;
}

/* ========================================================================== */
/* Главный проход правил                                                      */
/* ========================================================================== */

/* Итог правил объекта: объект уступает такт (Native_ContinueRules на последнем правиле). */
#define RULES_YIELD (-1)

/* Показать спрайт покоя объекта вместо доигранного спрайта хода. */
static void Object_ShowRestSprite(GM_OBJECT *obj)
{
    uint16_t restIndex = obj->restSpriteIndex;

    obj->spriteIndex = restIndex;
    if (restIndex != GM_POOL_NONE)
    {
        GM_SPRITE *sprite = &g_PoolTop[-restIndex].sprite;

        sprite->flags |= GMSPR_VISIBLE;
        sprite->fromTile = obj->tile;
        GmSprite_UpdatePosition(sprite);
        GmSprite_Invalidate(sprite);
    }
}

/* Выполнить правила текущего объекта начиная с *rule. Каждое правило пробуется во всех
   ориентациях, разрешённых классом (повёрнутых по направлению объекта: Ю=0, З=1, С=2,
   В=3), до первого сработавшего. Возвращает результат последнего GmRule_Run
   (0 — ни одно не сработало, 1 — сработало, > 1 — объект заменил сам себя)
   или RULES_YIELD. */
static int Object_RunRules(GM_RULE **rule)
{
    for (;;)
    {
        uint32_t turn;
        uint32_t xform;
        int result = 0;

        g_CurDirection = g_CurObject->flags & GMOBJ_DIR_MASK;
        switch (g_CurDirection)
        {
        case GMDIR_EAST:  turn = 3; break;
        case GMDIR_NORTH: turn = 2; break;
        case GMDIR_WEST:  turn = 1; break;
        default:          turn = 0; break;
        }

        for (xform = 0; xform < 8; ++xform)
        {
            if ((g_CurClass->ruleOrientations & (1u << xform)) == 0)
                continue;
            g_CurRuleXform = (turn + (xform & 3)) & 3;
            if (xform > 3)
                g_CurRuleXform += 4;
            g_pfnRuleXform = g_RuleXforms[g_CurRuleXform];
            g_ContinueRules = false;
            result = GmRule_Run(*rule, false);
            if (result != 0)
                break;
        }

        /* Сработавшее правило заканчивает обработку объекта, если не было Native_ContinueRules. */
        if (result > 1 || (result == 1 && !g_ContinueRules))
            return result;

        if (((*rule)->header.ruleFlags & GMRULE_LAST) == 0)
        {
            ++rule;
            continue;
        }
        return result == 0 ? 0 : RULES_YIELD;
    }
}

/* Главный проход правил за такт. Обходит экземпляры от g_ResumeObject (или от вершины
   пула), пропуская объекты классов вне g_ActiveClassFilter и помеченные «пропустить
   такт»; доигрывает анимацию хода (по её окончании показывается спрайт покоя) и
   выполняет правила класса. Объект, заменивший сам себя, передаёт фокус дальше.
   Объект, вызвавший Native_ContinueRules на последнем правиле, становится
   g_ResumeObject: проход прерывается, а следующий такт начнётся с него и закончится
   на нём же. Свободные спрайты без владельца доигрываются, пока нет фильтра классов. */
bool GmRules_RunTick(void)
{
    GM_POOL_ENTRY *entry = g_ResumeObject != NULL ? (GM_POOL_ENTRY *)g_ResumeObject : g_PoolTop;

    for (; entry > g_PoolBottom; --entry)
    {
        GM_OBJECT *obj = &entry->object;
        int outcome;

        if ((obj->flags & (GMOBJ_USED | GMOBJ_INSTANCE)) != (GMOBJ_USED | GMOBJ_INSTANCE))
        {
            if (g_ActiveClassFilter == NULL
                && (entry->sprite.flags & (GMSPR_USED | GMSPR_OBJECT | GMSPR_VISIBLE)) == (GMSPR_USED | GMSPR_VISIBLE)
                && entry->sprite.ownerIndex == GM_POOL_NONE)
            {
                GmSprite_Animate(&entry->sprite, false, true);
            }
            continue;
        }

        if (g_ActiveClassFilter != NULL
            && (g_ActiveClassFilter[obj->classIndex >> 3] & (1 << (obj->classIndex & 7))) == 0)
        {
            continue;
        }

        if (obj->underTile.flags & GMUT_SKIP_TICK)
        {
            obj->underTile.flags &= (uint8_t)~GMUT_SKIP_TICK;
            continue;
        }

        if (obj->flags & GMOBJ_MOVED)
        {
            /* Ход ещё доигрывается: правила объекта в этом такте не выполняются. */
            if (!GmSprite_Animate(&g_PoolTop[-obj->spriteIndex].sprite, true,
                                  obj->spriteIndex != obj->restSpriteIndex))
            {
                if (obj == g_ResumeObject)
                    break;
                continue;
            }
            Object_ShowRestSprite(obj);
            obj->flags &= (uint8_t)~GMOBJ_MOVED;
        }
        else if (obj->spriteIndex != GM_POOL_NONE
                 && GmSprite_Animate(&g_PoolTop[-obj->spriteIndex].sprite, false, true))
        {
            Object_ShowRestSprite(obj);
        }

        g_CurObject = obj;
        g_CurObjectIndex = (int)(g_PoolTop - entry);
        g_CurClass = &g_Classes[obj->classIndex];
        g_CurVars = (obj->flags & GMOBJ_VARS_ALLOCATED) ? obj->vars : (uint8_t *)&obj->vars;
        g_PathFound = false;
        if (g_CurClass->rules == NULL)
            continue;

        outcome = Object_RunRules(g_CurClass->rules);
        if (outcome == RULES_YIELD)
        {
            g_ResumeObject = obj;
            break;
        }
        if (g_ResumeObject != NULL)
        {
            /* Такт, продолживший отложенный объект, на нём и заканчивается. */
            g_ResumeObject = NULL;
            break;
        }
        if (outcome > 1)
            GmFocus_Advance(obj);
    }

    g_CurObject = NULL;
    g_CurObjectIndex = GM_NO_INDEX;
    return true;
}

/* ========================================================================== */
/* Выполнение одного правила                                                  */
/* ========================================================================== */

/* g_pfnPlaceCell в абсолютном режиме (комнатные правила при постройке и панели):
   клетка преобразуется и ставится на тайл (dx,dy) как абсолютные координаты.
   Всегда 0 (не «самозамена»). */
int GmPlaceCell_Absolute(GM_RULE_CELL cell)
{
    g_pfnRuleXform(&cell);
    GmTile_Place(cell, cell.dx, cell.dy);
    return 0;
}

/* g_pfnPlaceCell в обычном режиме: клетка преобразуется по ориентации правила и ставится
   на тайл объекта + (dx,dy) с переносом по краям карты. Возвращает 1, если смещение
   нулевое — объект заменил сам себя. */
int GmPlaceCell_Relative(GM_RULE_CELL cell)
{
    g_pfnRuleXform(&cell);
    GmTile_Place(cell,
                 (g_World.mapWidth + g_CurTileX + cell.dx) % g_World.mapWidth,
                 (g_World.mapHeight + g_CurTileY + cell.dy) % g_World.mapHeight);
    return cell.delta == 0;
}

/* Преобразовать клетку образца по ориентации правила и сличить её с тайлом карты:
   по классу или по набору классов, затем по пересечению направлений. Для набора
   классов с ориентациями направление клетки берётся из ориентации члена набора,
   повёрнутой по направлению клетки. Истина — тайл подходит. */
static bool Pattern_MatchCell(GM_RULE_CELL *cell, bool absolute)
{
    GM_TILE tile;

    g_pfnRuleXform(cell);
    if (absolute)
    {
        tile = g_TileMap[(uint8_t)cell->dx + ((uint8_t)cell->dy << g_TileMapShift)];
    }
    else
    {
        int x = (g_World.mapWidth + g_CurTileX + cell->dx) % g_World.mapWidth;
        int y = (g_World.mapHeight + g_CurTileY + cell->dy) % g_World.mapHeight;

        tile = g_TileMap[(y << g_TileMapShift) + x];
    }

    if (cell->flags & GMCELL_BY_CLASS_SET)
    {
        const GM_CLASS_SET *set;

        if (tile.classIndex == GMCLASS_EMPTY_TILE)
            return false;
        set = &g_ClassSets[cell->classIndex];
        if ((set->memberBits[tile.classIndex >> 3] & (1 << (tile.classIndex & 7))) == 0)
            return false;
        if (set->orientations != NULL)
        {
            uint8_t cellDirMask = cell->flags >> 4;

            cell->flags = (uint8_t)(set->orientations[tile.classIndex] | (cell->flags & CELL_MATCH_BITS));
            g_RotateCellByDirection[cellDirMask](cell);
        }
    }
    else if (cell->classIndex != tile.classIndex)
    {
        return false;
    }
    return (cell->flags & GMCELL_DIR_MASK & tile.flags) != 0;
}

/* Сличить группы образца, начиная с *itemPtr. Клетка «срабатывает», если результат
   сличения равен её признаку «должна совпасть»; тогда её режим: GMCELL_GROUP_DONE —
   к следующей группе, GMCELL_VETO — правило не сработало, GMCELL_ACCEPT — образец
   принят, GMCELL_COLLECT — запомнить смещение клетки в g_PatternMatches (для групп
   GMPG_BY_MATCH — в очередной набор ranges) и сличать дальше. Группа GMPG_BY_MATCH,
   не собравшая ни одного совпадения, проваливает правило. По успеху *itemPtr
   указывает за образец. */
static bool Pattern_Match(const GM_RULE_ITEM **itemPtr, bool absolute, GM_MATCH_RANGE *ranges)
{
    const GM_RULE_ITEM *item = *itemPtr;
    int rangeGroup = -1;

    for (;;)
    {
        GM_RULE_GROUP group = item->group;
        const GM_RULE_ITEM *groupEnd;
        GM_MATCH_RANGE *range = NULL;

        ++item;
        groupEnd = item + group.count;

        /* Наборы совпадений идущих подряд групп GMPG_BY_MATCH лежат в g_PatternMatches
           один за другим; любая иная группа начинает нумерацию заново. */
        if (group.type == GMPG_BY_MATCH)
        {
            ++rangeGroup;
            range = &ranges[rangeGroup];
            range->first = rangeGroup == 0 ? 0 : ranges[rangeGroup - 1].end;
            range->end = range->first;
        }
        else
        {
            rangeGroup = -1;
        }

        for (;;)
        {
            GM_RULE_CELL cell = item->cell;
            GM_CELL_FLAGS matched;

            ++item;
            matched = Pattern_MatchCell(&cell, absolute) ? GMCELL_MUST_MATCH : 0;

            if (matched == (cell.flags & GMCELL_MUST_MATCH))
            {
                GM_CELL_MODE mode = (GM_CELL_MODE)(cell.flags & GMCELL_MODE_MASK);

                if (mode == GMCELL_GROUP_DONE)
                {
                    /* Особенность оригинала: признак последней группы здесь не проверяется. */
                    item = groupEnd;
                    break;
                }
                if (mode == GMCELL_VETO)
                    return false;
                if (mode == GMCELL_ACCEPT)
                {
                    /* Пропустить оставшиеся группы образца. */
                    while ((group.raw & GMGROUP_IS_LAST) == 0)
                    {
                        group = groupEnd->group;
                        groupEnd += group.count + 1;
                    }
                    *itemPtr = groupEnd;
                    return true;
                }
                /* GMCELL_COLLECT. Вне группы GMPG_BY_MATCH оригинал пишет по указателю
                   на сохранённые регистры (мусорный номер); здесь такая запись пропускается. */
                if (range != NULL)
                    g_PatternMatches[range->end++] = cell.delta;
            }
            else if (item == groupEnd && range != NULL && range->first == range->end)
            {
                return false;
            }

            if (item == groupEnd)
            {
                if (group.raw & GMGROUP_IS_LAST)
                {
                    *itemPtr = item;
                    return true;
                }
                break;
            }
        }
    }
}

/* Выполнить группы размещения правила начиная с item. Тип группы:
   GMPG_ONE_OF_GROUPS — одна из 1–3 идущих подряд групп (число — в младших двух разрядах
   moveFlags заголовка) целиком, после чего размещение заканчивается;
   GMPG_RANDOM_COUNT — все клетки, кроме одной случайной; GMPG_ONE_CELL — одна случайная
   клетка; GMPG_EACH_HALF — каждая клетка с вероятностью 1/2; иначе — клетка, чьё смещение
   совпало со случайным совпадением из очередного набора образца.
   Возвращает число «самозамен». */
static int Rule_Place(const GM_RULE_ITEM *item, uint8_t alternativeCount, const GM_MATCH_RANGE *ranges)
{
    int selfPlacements = 0;
    int choiceGroup = -1;
    GM_RULE_GROUP group = item->group;
    const GM_RULE_ITEM *cmd = item + 1;
    const GM_RULE_ITEM *groupEnd = cmd + group.count;

    for (;;)
    {
        switch (group.type)
        {
        case GMPG_ONE_OF_GROUPS:
        {
            uint32_t choice = Random_Below(alternativeCount);

            if (choice == 2)
                groupEnd += groupEnd->group.count + 1;
            if (choice == 1 || choice == 2)
            {
                cmd = groupEnd + 1;
                groupEnd += groupEnd->group.count + 1;
            }
            do
            {
                selfPlacements += g_pfnPlaceCell(cmd->cell);
                ++cmd;
            } while (cmd != groupEnd);
            return selfPlacements;
        }

        case GMPG_RANDOM_COUNT:
        {
            /* Несмотря на имя типа, пропускается ровно одна клетка со случайным номером. */
            uint32_t skipIndex = Random_Below(group.count);
            uint32_t index = 0;

            do
            {
                if (index != skipIndex)
                    selfPlacements += g_pfnPlaceCell(cmd->cell);
                ++cmd;
                ++index;
            } while (cmd != groupEnd);
            break;
        }

        case GMPG_ONE_CELL:
            cmd += Random_Below(group.count);
            selfPlacements += g_pfnPlaceCell(cmd->cell);
            break;

        case GMPG_EACH_HALF:
            do
            {
                if (Random_Below(2) != 0)
                    selfPlacements += g_pfnPlaceCell(cmd->cell);
                ++cmd;
            } while (cmd != groupEnd);
            break;

        default:
        {
            const GM_MATCH_RANGE *range = &ranges[++choiceGroup];
            /* Особенность оригинала: начало набора прибавляется до деления на 65536,
               поэтому для наборов с ненулевым началом номер берётся почти всегда из
               начала g_PatternMatches, а не из своего набора. */
            uint32_t matchIndex = ((uint32_t)(range->end - range->first) * (g_RandomSeed >> 16)
                                   + (uint32_t)range->first) >> 16;
            uint16_t chosenDelta = g_PatternMatches[matchIndex];

            Random_Advance();
            for (;;)
            {
                if (cmd->cell.delta == chosenDelta)
                {
                    selfPlacements += g_pfnPlaceCell(cmd->cell);
                    break;
                }
                if (++cmd == groupEnd)
                    break;
            }
            break;
        }
        }

        if (group.raw & GMGROUP_IS_LAST)
            return selfPlacements;
        group = groupEnd->group;
        cmd = groupEnd + 1;
        groupEnd = cmd + group.count;
    }
}

/* Выполнить одно правило класса (или комнатное/панельное при absolute). Правило — поток
   двойных слов: заголовок (ruleFlags и клетка хода), [предусловие: скрипт, 0 — правило
   не сработало], [группы образца], [скрипт], [группы размещения].
   Возвращает 0, если правило не сработало, иначе 1 + число «самозамен» при размещении. */
int GmRule_Run(GM_RULE *rule, bool absolute)
{
    /* В оригинале наборы совпадений не инициализируются; обнуление делает размещение
       по совпадениям без образца детерминированным. */
    GM_MATCH_RANGE ranges[MAX_MATCH_RANGES] = { 0 };
    GM_RULE_HEADER header;
    const GM_RULE_ITEM *item;

    if (absolute)
    {
        g_CurObject = NULL;
        g_CurObjectIndex = GM_NO_INDEX;
        g_CurClass = NULL;
        g_UnusedObject = NULL;
        g_UnusedObjectIndex = GM_NO_INDEX;
        g_CurTileY = 0;
        g_CurTileX = 0;
        g_RuleTileY = 0;
        g_RuleTileX = 0;
        g_CurDirection = GMDIR_SOUTH;
        g_pfnRuleXform = GmXform_Identity;
        g_pfnPlaceCell = GmPlaceCell_Absolute;
        g_CurVars = NULL;
    }
    else
    {
        g_CurTileX = g_CurObject->tile.x;
        g_RuleTileX = g_CurTileX;
        g_CurTileY = g_CurObject->tile.y;
        g_RuleTileY = g_CurTileY;
        g_pfnPlaceCell = GmPlaceCell_Relative;
    }

    header = rule->header;
    item = rule->items;

    if (header.ruleFlags & GMRULE_PRECONDITION)
    {
        if (!GmVm_Run(item->script))
            return 0;
        ++item;
    }

    if ((header.ruleFlags & GMRULE_PATTERN) && !Pattern_Match(&item, absolute, ranges))
        return 0;

    g_CurObjectMoved = false;
    if (header.ruleFlags & GMRULE_MOVE)
    {
        uint8_t matchBits = header.moveFlags & CELL_MATCH_BITS;

        g_pfnRuleXform(&header.move);
        g_ResolveDirectionMask[header.moveFlags >> 4](&header.move);
        /* Особенность оригинала: генератор продвигается ещё раз сверх выбора направления. */
        Random_Advance();
        header.moveFlags = (uint8_t)(matchBits | (header.moveFlags & GMCELL_DIR_MASK));
        GmCurObject_Move(header.move);
    }

    g_SpriteSetThisStep = false;
    if (header.ruleFlags & GMRULE_SCRIPT)
    {
        /* Если объекта нет, оригинал слой не запоминает; сравнение ниже тогда не выполняется. */
        uint8_t layerBefore = g_CurObject != NULL ? g_CurObject->layer : 0;

        GmVm_Run(item->script);
        ++item;
        if (!g_SpriteSetThisStep
            && !g_CurObjectMoved
            && g_CurObject != NULL
            && g_CurObject->layer != layerBefore
            && g_CurObject->spriteIndex != GM_POOL_NONE)
        {
            GmSprite_InvalidateByIndex(g_CurObject->spriteIndex);
        }
    }

    if (!g_SpriteSetThisStep && g_CurObjectMoved)
        GmObject_RefreshSprite(g_CurObject);

    if (header.ruleFlags & GMRULE_PLACE)
        return Rule_Place(item, header.moveFlags & 3, ranges) + 1;
    return 1;
}

/* Сравнить номер направления dir (0=С, 1=СВ … 7=СЗ, по часовой) с baseDir с учётом
   текущего преобразования правила: для зеркальных преобразований (номер > 3) baseDir
   отражается по g_DirIndexMirror, затем поворачивается на 2·(номер & 3). */
int GmDirIndex_MatchesRotated(int dir, int baseDir)
{
    int dirIndex = baseDir;
    uint32_t rotation = g_CurRuleXform;

    if (rotation > 3)
    {
        rotation -= 4;
        dirIndex = g_DirIndexMirror[baseDir];
    }
    return ((dirIndex + 2 * (int)rotation) & 7) == dir;
}

/* Выполнить правило в абсолютном режиме, сохранив и восстановив весь контекст скрипта
   (объект, класс, тайлы, направление, преобразование, размещение, переменные, признак
   показа спрайта) — для панелей, которые вызываются изнутри правил. */
int GmRule_RunWithSavedContext(GM_RULE **rulePtr)
{
    GM_OBJECT *savedObject = g_CurObject;
    int savedObjectIndex = g_CurObjectIndex;
    GM_CLASS *savedClass = g_CurClass;
    void *savedUnusedObject = g_UnusedObject;
    int savedUnusedObjectIndex = g_UnusedObjectIndex;
    int savedRuleTileX = g_RuleTileX;
    int savedRuleTileY = g_RuleTileY;
    int savedTileX = g_CurTileX;
    int savedTileY = g_CurTileY;
    GM_DIRECTION savedDirection = g_CurDirection;
    GM_RULE_XFORM_FN savedRuleXform = g_pfnRuleXform;
    GM_PLACE_CELL_FN savedPlaceCell = g_pfnPlaceCell;
    uint8_t *savedVars = g_CurVars;
    bool savedSpriteSetThisStep = g_SpriteSetThisStep;
    int result;

    result = GmRule_Run(*rulePtr, true);

    g_SpriteSetThisStep = savedSpriteSetThisStep;
    g_CurVars = savedVars;
    g_pfnPlaceCell = savedPlaceCell;
    g_pfnRuleXform = savedRuleXform;
    g_CurDirection = savedDirection;
    g_CurTileY = savedTileY;
    g_CurTileX = savedTileX;
    g_RuleTileY = savedRuleTileY;
    g_RuleTileX = savedRuleTileX;
    g_UnusedObjectIndex = savedUnusedObjectIndex;
    g_UnusedObject = savedUnusedObject;
    g_CurClass = savedClass;
    g_CurObjectIndex = savedObjectIndex;
    g_CurObject = savedObject;
    return result;
}
