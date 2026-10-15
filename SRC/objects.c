/*
 * Объекты и спрайты: пул записей, фокус клавиатуры, постановка и очистка тайлов,
 * ход объектов, раздача ввода, такт в режиме текста, курсор-спрайт.
 *
 * Пул записей по 16 байт растёт вниз от g_PoolTop к g_PoolBottom; запись с номером i
 * лежит в g_PoolTop[-i]. В записи — экземпляр (GM_OBJECT) или спрайт (GM_SPRITE),
 * их различает разряд GMOBJ_INSTANCE.
 */
#include "gm.h"

#include <time.h>

/* ========================================================================== */
/* Вспомогательные                                                            */
/* ========================================================================== */

/* Шаг линейного конгруэнтного генератора движка. */
static void Random_Next(void)
{
    g_RandomSeed = g_RandomSeed * 1103515245u + 12345u;
}

/* Случайный член набора классов: randomChoices[0] — число вариантов, за ним сами классы.
   Вариант выбирается по старшему слову текущего зерна, затем генератор продвигается. */
static uint8_t ClassSet_RandomMember(const GM_CLASS_SET *set)
{
    uint8_t member = set->randomChoices[((set->randomChoices[0] * (g_RandomSeed >> 16)) >> 16) + 1];
    Random_Next();
    return member;
}

/* Номер записи пула (пул растёт вниз от g_PoolTop). */
static int Pool_IndexOf(const GM_POOL_ENTRY *entry)
{
    return (int)(g_PoolTop - entry);
}

/* Запись пула, в которой лежит спрайт (спрайт — первый член объединения записи). */
static GM_POOL_ENTRY *Pool_EntryOfSprite(GM_SPRITE *sprite)
{
    return (GM_POOL_ENTRY *)sprite;
}

/* Запись пула, в которой лежит экземпляр. */
static GM_POOL_ENTRY *Pool_EntryOfObject(GM_OBJECT *obj)
{
    return (GM_POOL_ENTRY *)obj;
}

/* ========================================================================== */
/* Пул записей                                                                */
/* ========================================================================== */

/* Опустить нижнюю границу пула: вершинный свободный блок кучи укорачивается так,
   чтобы кончаться у новой границы (пул растёт вниз навстречу куче). */
void GmPool_SetBottom(GM_POOL_ENTRY *bottom)
{
    uint32_t freeDwords;

    g_PoolBottom = bottom;
    freeDwords = (uint32_t)((uint32_t *)bottom - g_HeapTop) - 4;
    *g_HeapTop = freeDwords;
    g_HeapTopFreeDwords = freeDwords;
}

/* Завести пул у верхнего края вершинного свободного блока кучи (последние 16 байт). */
void GmPool_Create(void)
{
    g_PoolTop = (GM_POOL_ENTRY *)&g_HeapTop[g_HeapTopFreeDwords - 4];
    GmPool_SetBottom(g_PoolTop);
}

/* Взять свободную запись пула: первую свободную от вершины. Если дошли до нижней
   границы, граница опускается на запись. Запись помечается занятой. */
GM_POOL_ENTRY *GmPool_Alloc(void)
{
    GM_POOL_ENTRY *entry = g_PoolTop;

    while ((entry->flags & GMOBJ_USED) && entry != g_PoolBottom)
        --entry;
    entry->flags = GMOBJ_USED;
    if (entry == g_PoolBottom)
        GmPool_SetBottom(entry - 1);
    return entry;
}

/* Освободить запись пула; если под ней была граница, граница поднимается
   до ближайшей занятой записи. */
void GmPool_Free(GM_POOL_ENTRY *entry)
{
    GM_POOL_ENTRY *lowest = entry;

    entry->flags = 0;
    if (entry - 1 == g_PoolBottom)
    {
        while (lowest != g_PoolTop)
        {
            ++lowest;
            if (lowest->flags & GMOBJ_USED)
            {
                --lowest;
                break;
            }
        }
        GmPool_SetBottom(lowest);
    }
}

/* Опустошить пул (после сохранения состояния комнаты). */
void GmPool_Reset(void)
{
    g_PoolTop->flags = 0;
    GmPool_SetBottom(g_PoolTop);
}

/* ========================================================================== */
/* Спрайты                                                                    */
/* ========================================================================== */

/* Добавить прямоугольник текущего кадра спрайта в список грязных. Экранные спрайты
   (GMSPR_SCREEN_ANCHORED) стоят в явных координатах и рисуются от горячей точки. */
void GmSprite_Invalidate(GM_SPRITE *sprite)
{
    const GM_FRAME *frame = sprite->frame;
    int x = sprite->x;
    int y = sprite->y;

    if (sprite->flags & GMSPR_SCREEN_ANCHORED)
    {
        x -= frame->hotX;
        y -= frame->hotY;
    }
    GmDirty_Add(x, y, x + frame->image->width, y + frame->image->height);
}

/* Добавить в грязные прямоугольник спрайта по номеру записи пула. */
void GmSprite_InvalidateByIndex(int index)
{
    GmSprite_Invalidate(&g_PoolTop[-index].sprite);
}

/* Удалить спрайт: видимый сначала отмечается грязным, затем запись пула освобождается. */
void GmSprite_Destroy(GM_SPRITE *sprite)
{
    if (sprite->flags & GMSPR_VISIBLE)
        GmSprite_Invalidate(sprite);
    GmPool_Free(Pool_EntryOfSprite(sprite));
}

/* Удалить спрайт по номеру записи пула (GM_NO_INDEX — спрайта нет). */
void GmPool_DestroySpriteByIndex(int index)
{
    if (index != GM_NO_INDEX)
        GmSprite_Destroy(&g_PoolTop[-index].sprite);
}

/* Пересчитать экранную позицию привязанного к тайлу спрайта: тайл начала хода × размер
   тайла; если владелец двигается, позиция сдвигается к его тайлу пропорционально
   (кадр + 1) / число кадров — так рисуется ход между тайлами. Затем вычитается горячая
   точка кадра (у зеркальных — отражённая). Координаты спрайта 16-разрядные. */
void GmSprite_UpdatePosition(GM_SPRITE *sprite)
{
    const GM_FRAME *frame = sprite->frame;
    const GM_IMAGE *image = frame->image;
    int x = sprite->fromTile.x;
    int y = sprite->fromTile.y;
    int pixelX = g_GameHeader.tileWidth * x;
    int pixelY = g_GameHeader.tileHeight * y;
    int16_t hotX;
    int16_t hotY;

    if (sprite->ownerIndex != GM_POOL_NONE)
    {
        const GM_OBJECT *owner = &g_PoolTop[-sprite->ownerIndex].object;

        if (owner->flags & GMOBJ_MOVED)
        {
            int step = sprite->frameIndex + 1;

            pixelX += step * g_GameHeader.tileWidth * (owner->tile.x - x) / sprite->frameCount;
            pixelY += step * g_GameHeader.tileHeight * (owner->tile.y - y) / sprite->frameCount;
        }
    }

    if (sprite->flags & GMSPR_MIRROR_X)
        hotX = (int16_t)(image->width - (uint16_t)g_GameHeader.tileWidth - frame->hotX);
    else
        hotX = frame->hotX;
    sprite->x = (int16_t)(pixelX - hotX);

    if (sprite->flags & GMSPR_MIRROR_Y)
        hotY = (int16_t)(image->height - (uint16_t)g_GameHeader.tileHeight - frame->hotY);
    else
        hotY = frame->hotY;
    sprite->y = (int16_t)(pixelY - hotY);
}

/* Слот списка кадров по направлению хода текущего объекта (g_CurDirection) и сдвигу
   с тайла правила на текущий тайл; недостающие списки берутся отражением (ставит
   разряды зеркал спрайта). Стоячий кадр — при GMGR_STANDING_FRAME, если объект не сдвинулся
   (у юга и севера проверяется только строка). */
static GM_FRAME_SLOT Sprite_ChooseDirSlot(const GM_GRAPHIC *graphic, GM_SPRITE *sprite)
{
    bool standingFrame = (graphic->options & GMGR_STANDING_FRAME) != 0;
    bool mirrorXEast = (graphic->options & GMGR_MIRROR_X_EAST) != 0;
    bool mirrorYNorth = (graphic->options & GMGR_MIRROR_Y_NORTH) != 0;

    switch (g_CurDirection)
    {
    case GMDIR_SOUTH:
        if (g_CurTileY == g_RuleTileY && standingFrame)
            return GMSLOT_STANDING;
        return GMSLOT_S;

    case GMDIR_WEST:
        if (g_CurTileY > g_RuleTileY)
            return GMSLOT_SW;
        if (g_CurTileY < g_RuleTileY)
        {
            if (mirrorYNorth)
            {
                sprite->flags |= GMSPR_MIRROR_Y;
                return GMSLOT_SW;
            }
            return GMSLOT_NW;
        }
        if (standingFrame && g_CurTileX == g_RuleTileX)
            return GMSLOT_STANDING;
        return GMSLOT_W;

    case GMDIR_NORTH:
        if (g_CurTileY == g_RuleTileY && standingFrame)
            return GMSLOT_STANDING;
        if (mirrorYNorth)
        {
            sprite->flags |= GMSPR_MIRROR_Y;
            return GMSLOT_S;
        }
        return GMSLOT_N;

    default: /* восток (и любое иное значение) */
        if (g_CurTileY == g_RuleTileY)
        {
            if (standingFrame && g_CurTileX == g_RuleTileX)
                return GMSLOT_STANDING;
            if (mirrorXEast)
            {
                sprite->flags |= GMSPR_MIRROR_X;
                return GMSLOT_W;
            }
            return GMSLOT_E;
        }
        if (g_CurTileY > g_RuleTileY)
        {
            if (mirrorXEast)
            {
                sprite->flags |= GMSPR_MIRROR_X;
                return GMSLOT_SW;
            }
            return GMSLOT_SE;
        }
        if (!mirrorYNorth)
        {
            if (mirrorXEast)
            {
                sprite->flags |= GMSPR_MIRROR_X;
                return GMSLOT_NW;
            }
            return GMSLOT_NE;
        }
        /* Северо-восток отражением юго-востока (или юго-запада по обеим осям). */
        sprite->flags |= GMSPR_MIRROR_Y;
        if (mirrorXEast)
        {
            sprite->flags |= GMSPR_MIRROR_X;
            return GMSLOT_SW;
        }
        return GMSLOT_SE;
    }
}

/* Выбрать кадры спрайта по направлению хода текущего объекта. dirMode 1 — один список
   (зеркалится для востока/севера по options), иначе девять списков по направлениям
   (GM_FRAME_SLOT). Экранный спрайт получает GMSPR_SCREEN_ANCHORED, привязанный — тайл
   начала хода (тайл правила) и позицию. Прямоугольник спрайта отмечается грязным. */
void GmSprite_SetDirectionalFrames(GM_GRAPHIC *graphic, GM_SPRITE *sprite, bool screenAnchored)
{
    sprite->flags &= ~(GMSPR_MIRROR_X | GMSPR_MIRROR_Y);
    sprite->frameIndex = 0;

    if (graphic->dirMode == 1)
    {
        sprite->frame = graphic->frames.list;
        if (g_CurDirection == GMDIR_EAST)
        {
            if (graphic->options & GMGR_MIRROR_X_EAST)
                sprite->flags |= GMSPR_MIRROR_X;
        }
        else if (g_CurDirection == GMDIR_NORTH && (graphic->options & GMGR_MIRROR_Y_NORTH))
        {
            sprite->flags |= GMSPR_MIRROR_Y;
        }
    }
    else
    {
        GM_FRAME_SLOT slot = Sprite_ChooseDirSlot(graphic, sprite);

        sprite->frame = graphic->frames.byDirection[slot];
    }

    if (screenAnchored)
    {
        sprite->flags |= GMSPR_SCREEN_ANCHORED;
    }
    else
    {
        sprite->fromTile.x = (uint8_t)g_RuleTileX;
        sprite->fromTile.y = (uint8_t)g_RuleTileY;
        GmSprite_UpdatePosition(sprite);
    }
    GmSprite_Invalidate(sprite);
}

/* Найти графику спрайта по адресу его кадра: графики лежат в куче по возрастанию,
   идём от последней назад, пока её кадры начинаются выше кадра спрайта.
   Нижняя граница не проверяется. */
GM_GRAPHIC *GmSprite_FindGraphic(GM_SPRITE *sprite)
{
    const GM_FRAME *frame = sprite->frame;
    GM_GRAPHIC *graphic = &g_Graphics[g_World.graphicCount - 1];

    while (frame < graphic->frames.list)
        --graphic;
    return graphic;
}

/* Создать спрайт графики graphicIndex в точке (x, y). Экранный (screenAnchored) — без
   владельца, со слоем текущего объекта. Иначе — спрайт текущего объекта: прежний
   показываемый спрайт скрывается и удаляется (если это не спрайт покоя), неодноразовый
   новый спрайт заменяет и спрайт покоя; при смене тайла объект помечается «двигался».
   Без текущего объекта (статический класс) — свободный спрайт тайла со слоем класса. */
int GmSprite_Create(int graphicIndex, uint16_t x, uint16_t y, bool screenAnchored)
{
    GM_GRAPHIC *graphic = &g_Graphics[graphicIndex];
    GM_POOL_ENTRY *entry = GmPool_Alloc();
    GM_SPRITE *sprite = &entry->sprite;

    sprite->flags |= GMSPR_VISIBLE;
    if (graphic->options & GMGR_PLAY_ONCE)
        sprite->flags |= GMSPR_PLAY_ONCE;
    sprite->frameCount = graphic->frameCount;
    sprite->ownerIndex = screenAnchored ? GM_POOL_NONE : (uint16_t)g_CurObjectIndex;
    sprite->x = (int16_t)x;
    sprite->y = (int16_t)y;

    if (screenAnchored)
    {
        sprite->fromTile.y = 0xFF;
        sprite->fromTile.x = 0xFF;
        sprite->layer = g_CurObject->layer;
    }
    else if (g_CurObject != NULL)
    {
        uint16_t newIndex = (uint16_t)Pool_IndexOf(entry);
        uint16_t restIndex = g_CurObject->restSpriteIndex;
        uint16_t shownIndex = g_CurObject->spriteIndex;

        if (shownIndex != GM_POOL_NONE)
        {
            GM_POOL_ENTRY *oldEntry = &g_PoolTop[-shownIndex];

            GmSprite_Invalidate(&oldEntry->sprite);
            oldEntry->sprite.flags &= ~GMSPR_VISIBLE;
            if (shownIndex != restIndex)
                GmPool_Free(oldEntry);
        }
        g_CurObject->spriteIndex = newIndex;
        if (!(sprite->flags & GMSPR_PLAY_ONCE))
        {
            if (restIndex != GM_POOL_NONE)
                GmPool_Free(&g_PoolTop[-restIndex]);
            g_CurObject->restSpriteIndex = newIndex;
        }
        sprite->layer = g_CurObject->layer;
        if (g_CurTileX != g_RuleTileX || g_CurTileY != g_RuleTileY)
            g_CurObject->flags |= GMOBJ_MOVED;
    }
    else
    {
        sprite->layer = g_CurClass->defaultLayer;
    }

    GmSprite_SetDirectionalFrames(graphic, sprite, screenAnchored);
    return 1;
}

/* Перерисовать спрайт после смены кадра: привязанный к тайлу сначала переставляется. */
static void Sprite_ShowNewFrame(GM_SPRITE *sprite)
{
    if (!(sprite->flags & GMSPR_SCREEN_ANCHORED))
        GmSprite_UpdatePosition(sprite);
    GmSprite_Invalidate(sprite);
}

/* Перевести спрайт на следующий кадр. Возвращает true, когда одноразовая (GMSPR_PLAY_ONCE)
   или принудительно завершаемая (forceEnd) анимация дошла до конца; тогда при
   freeWhenDone запись спрайта освобождается. Иначе анимация зацикливается — false. */
bool GmSprite_Animate(GM_SPRITE *sprite, bool forceEnd, bool freeWhenDone)
{
    unsigned frameCount = sprite->frameCount;
    bool finishes = forceEnd || (sprite->flags & GMSPR_PLAY_ONCE);

    if (frameCount > 1)
    {
        GmSprite_Invalidate(sprite);
        if (++sprite->frameIndex != frameCount)
        {
            ++sprite->frame;
            Sprite_ShowNewFrame(sprite);
            return false;
        }
        sprite->frameIndex = 0;
        sprite->frame -= frameCount - 1;
        if (!finishes)
        {
            Sprite_ShowNewFrame(sprite);
            return false;
        }
    }
    else
    {
        if (!finishes)
            return false;
        GmSprite_Invalidate(sprite);
    }

    if (freeWhenDone)
        GmPool_Free(Pool_EntryOfSprite(sprite));
    return true;
}

/* ========================================================================== */
/* Экземпляры                                                                 */
/* ========================================================================== */

/* Убрать текст объекта: если поднят GMUT_HAS_TEXT, снять его и удалить все глифы
   (занятые видимые экранные спрайты-глифы), владелец которых — этот объект. */
void GmObject_RemoveText(GM_OBJECT *obj)
{
    enum
    {
        GLYPH_MASK = GMSPR_USED | GMSPR_OBJECT | GMSPR_TEXT_GLYPH | GMSPR_SCREEN_ANCHORED | GMSPR_VISIBLE,
        GLYPH_BITS = GMSPR_USED | GMSPR_TEXT_GLYPH | GMSPR_SCREEN_ANCHORED | GMSPR_VISIBLE,
    };
    GM_POOL_ENTRY *entry;
    int ownerIndex;

    if (!(obj->underTile.flags & GMUT_HAS_TEXT))
        return;

    obj->underTile.flags &= ~GMUT_HAS_TEXT;
    ownerIndex = Pool_IndexOf(Pool_EntryOfObject(obj));
    for (entry = g_PoolTop; entry > g_PoolBottom; --entry)
    {
        if ((entry->sprite.flags & GLYPH_MASK) == GLYPH_BITS && entry->sprite.ownerIndex == ownerIndex)
            GmSprite_Destroy(&entry->sprite);
    }
}

/* Обновить спрайт объекта после хода: перерисовать, при смене тайла пометить
   «двигался» и заново выбрать кадры той же графики по направлению. */
void GmObject_RefreshSprite(GM_OBJECT *obj)
{
    GM_SPRITE *sprite;
    GM_GRAPHIC *graphic;

    if (obj->spriteIndex == GM_POOL_NONE)
        return;

    sprite = &g_PoolTop[-obj->spriteIndex].sprite;
    graphic = GmSprite_FindGraphic(sprite);
    GmSprite_Invalidate(sprite);
    if (g_RuleTileX != g_CurTileX || g_RuleTileY != g_CurTileY)
        obj->flags |= GMOBJ_MOVED;
    GmSprite_SetDirectionalFrames(graphic, sprite, false);
}

/* Попадает ли точка (x, y) в пикселях карты в текущий кадр показываемого спрайта
   объекта (границы включительно). Нет спрайта — нет попадания. */
bool GmObject_HitTest(GM_OBJECT *obj, int x, int y)
{
    const GM_SPRITE *sprite;
    const GM_IMAGE *image;

    if (obj->spriteIndex == GM_POOL_NONE)
        return false;

    sprite = &g_PoolTop[-obj->spriteIndex].sprite;
    if (sprite->x > x || sprite->y > y)
        return false;
    image = sprite->frame->image;
    return image->width + sprite->x >= x && image->height + sprite->y >= y;
}

/* Перевести фокус клавиатуры на следующий фокусируемый (GMCLASS_FOCUSABLE) экземпляр.
   Действует, только если obj — текущий фокус (или obj == NULL и фокуса нет): берётся
   первый фокусируемый экземпляр после obj в порядке пула, иначе первый с начала;
   если таких нет — фокус снимается. */
void GmFocus_Advance(GM_OBJECT *obj)
{
    GM_POOL_ENTRY *entry;
    GM_OBJECT *nextFocus = NULL;

    if (obj != g_FocusObject)
        return;

    for (entry = g_PoolTop; entry > g_PoolBottom; --entry)
    {
        bool pastObj = obj != NULL && &entry->object < obj;

        if ((entry->object.flags & (GMOBJ_USED | GMOBJ_INSTANCE)) != (GMOBJ_USED | GMOBJ_INSTANCE)
            || g_Classes[entry->object.classIndex].kind != GMCLASS_FOCUSABLE)
            continue;
        if (pastObj)
        {
            nextFocus = &entry->object;
            break;
        }
        if (nextFocus == NULL)
            nextFocus = &entry->object;
    }
    g_FocusObject = nextFocus;
}

/* ========================================================================== */
/* Тайлы                                                                      */
/* ========================================================================== */

/* Уничтожить статический тайл под текущим объектом (не сам объект): удалить свободный
   спрайт этого тайла (без владельца, не экранный, начат на тайле объекта), выполнить
   скрипт уничтожения класса и пометить underTile пустым (GMTILE_EMPTY_RAW). */
void GmCurObject_DestroyUnderTile(void)
{
    int underClass = g_CurObject->underTile.classIndex;
    GM_CLASS *cls;
    GM_POOL_ENTRY *entry;

    if (underClass == GMCLASS_EMPTY_TILE)
        return;

    cls = &g_Classes[underClass];
    /* Особенность оригинала: вершина пула проверяется без сравнения с нижней границей,
       а разряд GMSPR_OBJECT не проверяется (подходит и запись экземпляра, чьи поля
       по смещениям спрайта случайно совпали). */
    entry = g_PoolTop;
    for (;;)
    {
        if ((entry->sprite.flags & (GMSPR_USED | GMSPR_SCREEN_ANCHORED | GMSPR_VISIBLE)) == (GMSPR_USED | GMSPR_VISIBLE)
            && entry->sprite.ownerIndex == GM_POOL_NONE
            && entry->sprite.fromTile.packed == g_CurObject->tile.packed)
        {
            GmSprite_Destroy(&entry->sprite);
            break;
        }
        if (--entry <= g_PoolBottom)
            break;
    }

    GmRunScriptWithContext(cls->destroyScript, cls, NULL, GM_NO_INDEX, NULL,
                           g_CurObject->tile.x, g_CurObject->tile.y, g_CurObject->underTile.flags);
    g_CurObject->underTile.raw = GMTILE_EMPTY_RAW;
}

/* Очистить тайл карты (tileX, tileY). Экземпляр на тайле разбирается: удаляются его
   спрайты (показываемый и покоя), блок переменных, текст, запись пула, фокус передаётся
   дальше; на карту возвращается тайл, лежавший под объектом (только разряды
   направления). С alsoFreeSprites удаляются и свободные спрайты, начатые на тайле.
   В конце выполняется скрипт уничтожения класса, стоявшего на тайле. */
void GmTile_Clear(int tileX, int tileY, bool alsoFreeSprites)
{
    GM_TILE *tile = &g_TileMap[tileX + (tileY << g_TileMapShift)];
    GM_CLASS *cls;
    GM_POOL_ENTRY *entry;
    GM_OBJECT *removedObj = NULL;
    GM_TILEPOS tilePos;
    int found = 0;

    if (tile->classIndex == GMCLASS_EMPTY_TILE)
        return;

    cls = &g_Classes[tile->classIndex];
    tilePos.x = (uint8_t)tileX;
    tilePos.y = (uint8_t)tileY;

    /* Особенность оригинала: обход прекращается на второй находке (спрайт или экземпляр),
       а без alsoFreeSprites — на первом экземпляре; вершина пула проверяется без сравнения
       с нижней границей. */
    entry = g_PoolTop;
    for (;;)
    {
        if (entry->flags & GMOBJ_USED)
        {
            if (!(entry->flags & GMOBJ_INSTANCE))
            {
                if (alsoFreeSprites
                    && entry->sprite.fromTile.packed == tilePos.packed
                    && entry->sprite.ownerIndex == GM_POOL_NONE
                    && !(entry->sprite.flags & GMSPR_SCREEN_ANCHORED))
                {
                    GmSprite_Destroy(&entry->sprite);
                    if (found++)
                        break;
                }
            }
            else if (entry->object.tile.packed == tilePos.packed)
            {
                GM_OBJECT *obj = &entry->object;

                GmPool_DestroySpriteByIndex(obj->spriteIndex);
                if (obj->spriteIndex != obj->restSpriteIndex)
                    GmPool_DestroySpriteByIndex(obj->restSpriteIndex);
                if (obj->flags & GMOBJ_VARS_ALLOCATED)
                    GmHeap_Free(obj->vars);
                removedObj = obj;
                GmObject_RemoveText(obj);
                GmPool_Free(entry);
                GmFocus_Advance(obj);
                if (!alsoFreeSprites || found != 0)
                    break;
                ++found;
            }
        }
        if (--entry <= g_PoolBottom)
            break;
    }

    if (removedObj != NULL)
    {
        GM_TILE under = removedObj->underTile;

        under.flags &= GMUT_DIR_MASK;
        *tile = under;
    }
    GmRunScriptWithContext(cls->destroyScript, cls, NULL, GM_NO_INDEX, NULL,
                           (uint8_t)tileX, (uint8_t)tileY, tile->flags);
}

/* Поставить под текущий объект новый статический тайл: уничтожить прежний, записать
   класс classOrSet (или случайный член набора классов, если fromClassSet) с направлением
   юг и выполнить скрипт создания класса на тайле объекта без экземпляра. */
void GmCurObject_SetUnderTile(int classOrSet, bool fromClassSet)
{
    GmCurObject_DestroyUnderTile();
    if (fromClassSet)
        classOrSet = ClassSet_RandomMember(&g_ClassSets[classOrSet]);

    g_CurObject->underTile.classIndex = (uint8_t)classOrSet;
    g_CurObject->underTile.flags = GMDIR_SOUTH;
    GmRunScriptWithContext(g_Classes[classOrSet].createScript, &g_Classes[classOrSet], NULL, GM_NO_INDEX, NULL,
                           g_CurObject->tile.x, g_CurObject->tile.y, GMDIR_SOUTH);
}

/* Поставить класс на тайл по клетке правила. GMCELL_BY_CLASS_SET: classIndex клетки —
   номер набора, берётся случайный член (направление — ориентация члена, повёрнутая
   по направлению клетки, если у набора есть ориентации). Класс GMCLASS_EMPTY_TILE просто
   очищает тайл. Иначе маска направлений разрешается в одно направление, тайл очищается
   и для классов вида ≠ GMCLASS_STATIC заводится экземпляр; затем выполняется скрипт
   создания класса. */
void GmTile_Place(GM_RULE_CELL cell, int tileX, int tileY)
{
    GM_TILE *tile = &g_TileMap[tileX + (tileY << g_TileMapShift)];
    uint8_t classIndex = cell.classIndex;
    GM_CLASS *cls;
    GM_OBJECT *obj;
    int objectIndex;
    uint8_t *vars;

    if (cell.flags & GMCELL_BY_CLASS_SET)
    {
        const GM_CLASS_SET *set = &g_ClassSets[cell.classIndex];

        classIndex = ClassSet_RandomMember(set);
        if (set->orientations != NULL)
        {
            int cellDir = cell.flags >> 4;

            cell.flags = set->orientations[classIndex];
            g_RotateCellByDirection[cellDir](&cell);
        }
    }
    else if (cell.classIndex == GMCLASS_EMPTY_TILE)
    {
        GmTile_Clear(tileX, tileY, true);
        *tile = cell.tile;
        return;
    }

    g_ResolveDirectionMask[cell.flags >> 4](&cell);
    Random_Next();
    cell.classIndex = classIndex;
    cls = &g_Classes[classIndex];

    if (cls->kind != GMCLASS_STATIC)
    {
        GM_POOL_ENTRY *entry;
        uint32_t varsSize;

        GmTile_Clear(tileX, tileY, false);
        entry = GmPool_Alloc();
        obj = &entry->object;
        obj->flags = (cell.flags & GMCELL_DIR_MASK) | GMOBJ_USED | GMOBJ_INSTANCE;
        obj->classIndex = classIndex;
        obj->input = 0;
        obj->layer = cls->defaultLayer;
        obj->tile.x = (uint8_t)tileX;
        obj->tile.y = (uint8_t)tileY;
        obj->spriteIndex = GM_POOL_NONE;
        obj->restSpriteIndex = GM_POOL_NONE;
        obj->underTile = *tile;
        /* Экземпляр, созданный впереди идущего обхода пула, в этом такте не ходит. */
        if (g_CurObject != NULL && obj < g_CurObject)
            obj->underTile.flags |= GMUT_SKIP_TICK;

        /* Переменные до 4 байт хранятся прямо в поле vars. */
        varsSize = cls->varsSizeMinus1 + 1u;
        if (varsSize <= sizeof(obj->vars))
        {
            obj->vars = NULL;
            vars = (uint8_t *)&obj->vars;
        }
        else
        {
            vars = GmHeap_Alloc(varsSize);
            obj->vars = vars;
            GmMemZero(vars, varsSize);
            obj->flags |= GMOBJ_VARS_ALLOCATED;
        }

        objectIndex = Pool_IndexOf(entry);
        if (cls->kind == GMCLASS_FOCUSABLE && g_FocusObject == NULL)
            g_FocusObject = obj;
    }
    else
    {
        GmTile_Clear(tileX, tileY, true);
        obj = NULL;
        objectIndex = GM_NO_INDEX;
        vars = NULL;
    }

    *tile = cell.tile;
    GmRunScriptWithContext(cls->createScript, cls, obj, objectIndex, vars, (uint8_t)tileX, (uint8_t)tileY, cell.flags);
}

/* Ход/поворот текущего объекта по клетке step. Ненулевой сдвиг (dx, dy): объект уходит
   с тайла правила (там восстанавливается тайл, лежавший под ним), целевой тайл (с переносом
   через края карты) очищается и запоминается в underTile, на карту пишется класс
   и направление объекта. Смена направления разворачивает объект и его тайл.
   При любой перемене поднимается g_CurObjectMoved. */
void GmCurObject_Move(GM_RULE_CELL step)
{
    if (step.delta != 0)
    {
        uint8_t lowFlags;
        GM_TILE under;
        GM_TILE *newTile;

        g_CurTileX = (g_World.mapWidth + g_CurTileX + step.dx) % g_World.mapWidth;
        g_CurTileY = (g_World.mapHeight + g_CurTileY + step.dy) % g_World.mapHeight;

        lowFlags = g_CurObject->underTile.flags & (uint8_t)~GMUT_DIR_MASK;
        under = g_CurObject->underTile;
        under.flags &= GMUT_DIR_MASK;
        g_TileMap[g_RuleTileX + (g_RuleTileY << g_TileMapShift)] = under;

        GmTile_Clear(g_CurTileX, g_CurTileY, false);
        g_CurObject->tile.x = (uint8_t)g_CurTileX;
        g_CurObject->tile.y = (uint8_t)g_CurTileY;
        newTile = &g_TileMap[g_CurTileX + (g_CurTileY << g_TileMapShift)];
        g_CurObject->underTile = *newTile;
        g_CurObject->underTile.flags |= lowFlags;
        newTile->classIndex = g_CurObject->classIndex;
        newTile->flags = g_CurObject->flags & GMOBJ_DIR_MASK;
        g_CurObjectMoved = true;
    }

    if ((step.flags & GMCELL_DIR_MASK) != g_CurDirection)
    {
        g_CurDirection = step.flags & GMCELL_DIR_MASK;
        g_CurObject->flags &= ~GMOBJ_DIR_MASK;
        g_CurObject->flags |= g_CurDirection;
        g_TileMap[g_CurTileX + (g_CurTileY << g_TileMapShift)].flags = g_CurDirection;
        g_CurObjectMoved = true;
    }
}

/* ========================================================================== */
/* Такт: системные переменные, ввод, громкость, режим текста                  */
/* ========================================================================== */

/* Ежетактовое обновление системных переменных: публикует нажатый в такте знак,
   возвращает false (останов), если игра выставила quitRequest, считает кадры, зажимает
   частоту кадров в 1..50 и перепрограммирует таймер кадров при её изменении. */
bool GmSysVars_Update(void)
{
    GM_SYSVARS *sysVars = g_SysVars;

    if (g_InputChar == GMEV_NONE)
    {
        sysVars->keyPressed = false;
        sysVars->lastKey = 0;
    }
    else
    {
        sysVars->lastKey = g_InputChar;
        sysVars->keyPressed = true;
    }

    if (sysVars->quitRequest)
        return false;

    ++sysVars->frameCount;
    if (sysVars->frameRate == 0)
        sysVars->frameRate = 1;
    if (sysVars->frameRate > 50)
        sysVars->frameRate = 50;
    if (sysVars->frameRate != g_GameHeader.frameRate)
    {
        g_GameHeader.frameRate = sysVars->frameRate;
        Host_SetFrameRate(sysVars->frameRate);
    }
    return true;
}

/* Раздать ввод такта экземплярам. Разряды ввода (GM_INPUT_BITS): GMIN_KEY | номер клавиши
   направления, HOVER|RELEASED / PRESSED|RELEASED для действий 1/2, HOVER — наведение,
   PRESSED — нажатие кнопки мыши. Вид GMCLASS_ALL_INPUT получает всё, GMCLASS_POINTABLE
   и объект в фокусе — только клавиши; верхний (меньший слой) объект под указателем
   получает разряды мыши, щелчок действием по фокусируемому объекту передаёт ему фокус.
   Запоминает тайл под указателем и, для событий действия, тайл щелчка. */
void GmInput_Dispatch(int mouseX, int mouseY, int clickX, int clickY, GM_EVENT event, uint8_t actionKey)
{
    enum { KEY_BITS = GMIN_DIR_MASK | GMIN_SHIFT | GMIN_KEY };
    GM_OBJECT *hitObject = NULL;
    GM_CLASS_KIND hitKind = GMCLASS_STATIC;
    uint8_t hitLayer = 0xFF;
    GM_INPUT_BITS inputBits = 0;
    GM_POOL_ENTRY *entry;
    int mapX;
    int mapY;

    switch (event)
    {
    case GMEV_NEXT_FOCUS:
        GmFocus_Advance(g_FocusObject);
        break;
    case GMEV_ACTION1:
        inputBits = GMIN_HOVER | GMIN_RELEASED;
        break;
    case GMEV_ACTION2:
        inputBits = GMIN_PRESSED | GMIN_RELEASED;
        break;
    case GMEV_NONE:
        clickX = mouseX;
        clickY = mouseY;
        break;
    default:
        break;
    }
    if (actionKey != GMEV_NONE)
        inputBits |= actionKey | GMIN_KEY;

    mapX = g_ScrollX + clickX;
    mapY = g_ScrollY + clickY;

    for (entry = g_PoolTop; entry > g_PoolBottom; --entry)
    {
        GM_OBJECT *obj = &entry->object;
        const GM_CLASS *cls;

        if ((obj->flags & (GMOBJ_USED | GMOBJ_INSTANCE)) != (GMOBJ_USED | GMOBJ_INSTANCE))
            continue;

        cls = &g_Classes[obj->classIndex];
        switch (cls->kind)
        {
        case GMCLASS_ALL_INPUT:
            obj->input = inputBits;
            break;
        case GMCLASS_POINTABLE:
            obj->input = inputBits & KEY_BITS;
            break;
        case GMCLASS_FOCUSABLE:
            obj->input = (obj == g_FocusObject) ? (inputBits & KEY_BITS) : 0;
            break;
        default:
            break;
        }

        if ((hitObject == NULL || obj->layer <= hitLayer) && GmObject_HitTest(obj, mapX, mapY))
        {
            hitObject = obj;
            hitKind = cls->kind;
            hitLayer = obj->layer;
        }
    }

    if (hitObject == NULL)
    {
        g_PointerTileX = mapX / g_GameHeader.tileWidth;
        g_PointerTileY = mapY / g_GameHeader.tileHeight;
        g_PointerTileClass = g_TileMap[g_PointerTileX + (g_PointerTileY << g_TileMapShift)].classIndex;
    }
    else
    {
        /* Особенность оригинала: при попадании в объект сюда кладётся вид его класса,
           а не номер класса. */
        g_PointerTileClass = hitKind;
        if (hitKind == GMCLASS_POINTABLE)
        {
            if (event == GMEV_NONE)
                inputBits |= GMIN_HOVER;
            else if (event == GMEV_MOUSE_DOWN)
                inputBits |= GMIN_PRESSED;
            hitObject->input = inputBits;
        }
        else if (hitKind == GMCLASS_FOCUSABLE
                 && (inputBits == (GMIN_HOVER | GMIN_RELEASED) || inputBits == (GMIN_PRESSED | GMIN_RELEASED)))
        {
            g_FocusObject = hitObject;
            hitObject->input = inputBits;
        }
        g_PointerTileX = hitObject->tile.x;
        g_PointerTileY = hitObject->tile.y;
    }

    g_PointerEvent = event;
    if (event == GMEV_ACTION1 || event == GMEV_ACTION2)
    {
        g_ClickEvent = event;
        g_ClickTileClass = g_PointerTileClass;
        g_ClickTileX = g_PointerTileX;
        g_ClickTileY = g_PointerTileY;
    }
    else
    {
        if (event == GMEV_MOUSE_DOWN)
            g_ClickEvent = GMEV_NONE;
        g_PointerEvent = GMEV_NONE;
    }
}

/* Шаг плавного изменения громкости звука и музыки к целевой; при изменении
   громкость устройства перепрограммируется. */
void GmVolume_FadeStep(void)
{
    if (GmStepToward(&g_Settings.soundVolume, g_TargetSoundVolume))
        GmAudio_SetSoundVolume(g_Settings.soundVolume);
    if (GmStepToward(&g_Settings.musicVolume, g_TargetMusicVolume))
        GmAudio_SetMusicVolume(g_Settings.musicVolume);
}

/* Такт в режиме ввода текста: правила не выполняются, анимируются только видимые
   неодноразовые спрайты (на последнем кадре спрайт с владельцем переносится к тайлу
   владельца). Пробел, Enter или любой знак без события передаются в GmText_Process. */
bool GmTick_TextMode(GM_EVENT event)
{
    enum
    {
        ANIM_MASK = GMSPR_USED | GMSPR_OBJECT | GMSPR_PLAY_ONCE | GMSPR_VISIBLE,
        ANIM_BITS = GMSPR_USED | GMSPR_VISIBLE,
    };
    GM_POOL_ENTRY *entry;

    for (entry = g_PoolTop; entry > g_PoolBottom; --entry)
    {
        GM_SPRITE *sprite = &entry->sprite;

        if ((sprite->flags & ANIM_MASK) != ANIM_BITS)
            continue;
        if (sprite->frameIndex + 1 == sprite->frameCount && sprite->ownerIndex != GM_POOL_NONE)
        {
            const GM_OBJECT *owner = &g_PoolTop[-sprite->ownerIndex].object;

            sprite->fromTile.x = owner->tile.x;
            sprite->fromTile.y = owner->tile.y;
        }
        GmSprite_Animate(sprite, false, false);
    }

    if (g_InputChar == ' ' || g_InputChar == '\r' || (g_InputChar != GMEV_NONE && event == GMEV_NONE))
        return GmText_Process(g_InputChar) != 0;
    return true;
}

/* Подготовка состояния такта при первой записи комнаты: засев генератора временем,
   нет фокуса, пустой список грязных прямоугольников. */
void GmTick_InitState(void)
{
    g_RandomSeed = (uint32_t)time(NULL) * 1103515245u + 12345u;
    g_FocusObject = NULL;
    g_DirtyRects = NULL;
}

/* ========================================================================== */
/* Курсор                                                                     */
/* ========================================================================== */

/* Перенести курсор-спрайт к указателю мыши и отметить его новое место грязным. */
static void Cursor_MoveTo(int mouseX, int mouseY)
{
    g_CursorSprite.x = (int16_t)mouseX;
    g_CursorSprite.y = (int16_t)mouseY;
    if (g_CursorInWindow)
        GmSprite_Invalidate(&g_CursorSprite);
}

/* Анимировать курсор-спрайт и перенести его к указателю мыши (если курсор рисует движок).
   Одноразовая анимация по окончании возвращает курсор к графике 0. Ставит g_GameRunning,
   кроме случая, когда курсор неподвижен и не меняется. Всегда возвращает true. */
bool GmCursor_Update(int mouseX, int mouseY)
{
    if (!g_GameHeader.systemCursor)
    {
        unsigned frameCount = g_CursorSprite.frameCount;

        if (frameCount > 1)
        {
            GmSprite_Invalidate(&g_CursorSprite);
            if (++g_CursorSprite.frameIndex != frameCount)
            {
                ++g_CursorSprite.frame;
            }
            else if (!(g_CursorSprite.flags & GMSPR_PLAY_ONCE))
            {
                g_CursorSprite.frameIndex = 0;
                g_CursorSprite.frame -= frameCount - 1;
            }
            else
            {
                GmCursor_SetGraphic(0, true);
            }
        }
        else if (g_CursorSprite.flags & GMSPR_PLAY_ONCE)
        {
            GmCursor_SetGraphic(0, true);
        }
        else if (g_CursorSprite.x != mouseX || g_CursorSprite.y != mouseY || !g_CursorInWindow)
        {
            GmSprite_Invalidate(&g_CursorSprite);
        }
        else
        {
            return true;
        }
        Cursor_MoveTo(mouseX, mouseY);
    }
    g_GameRunning = true;
    return true;
}

/* Сделать графику graphicIndex курсором: экранный видимый спрайт, первый кадр первого
   списка (или единственного при dirMode 1). Номер запоминается в g_Settings.cursorGraphic.
   redraw — отметить грязным место прежнего курсора. */
bool GmCursor_SetGraphic(int graphicIndex, bool redraw)
{
    const GM_GRAPHIC *graphic;

    if (g_GameHeader.systemCursor)
        return true;

    if (redraw && g_CursorSprite.frame != NULL)
        GmSprite_Invalidate(&g_CursorSprite);
    g_Settings.cursorGraphic = graphicIndex;
    graphic = &g_Graphics[graphicIndex];
    g_CursorSprite.flags = GMSPR_USED | GMSPR_SCREEN_ANCHORED | GMSPR_VISIBLE;
    g_CursorSprite.frameCount = graphic->frameCount;
    g_CursorSprite.frameIndex = 0;
    g_CursorSprite.y = 0;
    g_CursorSprite.x = 0;
    if (graphic->dirMode == 1)
        g_CursorSprite.frame = graphic->frames.list;
    else
        g_CursorSprite.frame = graphic->frames.byDirection[0];
    return true;
}

/* Движение мыши (WM_MOUSEMOVE): если курсор рисует движок и игра идёт (нет фильма,
   нет фильтра классов), перенести курсор-спрайт и сразу перерисовать. */
void GmCursor_OnMouseMove(uint16_t mouseX, uint16_t mouseY)
{
    if (g_GameHeader.systemCursor || g_ActiveClassFilter != NULL || g_MovieDepth != 0
        || g_CursorSprite.frame == NULL || !g_GameRunning)
        return;

    g_CursorInWindow = true;
    GmSprite_Invalidate(&g_CursorSprite);
    g_CursorSprite.x = (int16_t)mouseX;
    g_CursorSprite.y = (int16_t)mouseY;
    GmSprite_Invalidate(&g_CursorSprite);
    GmRender_Flush();
}
