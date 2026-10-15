/*
 * Игровой такт и общее управление игрой: инициализация, перезапуск такта,
 * список грязных прямоугольников и вывод кадра, сохранение и вход в комнату.
 */
#include "gm.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* Наибольшее число спрайтов в одном кадре (стековый массив GmRender_Flush). */
#define GM_MAX_DRAWN_SPRITES 1024

/* Засеять генератор случайных чисел текущим временем. */
static void GmRandom_SeedFromTime(void)
{
    g_RandomSeed = (uint32_t)time(NULL) * 1103515245u + 12345u;
}

/* Пересекается ли (или касается) прямоугольник rect с прямоугольником left..bottom. */
static bool GmDirty_Overlaps(const GM_DIRTY_RECT *rect, int left, int top, int right, int bottom)
{
    return rect->left <= right && rect->top <= bottom && rect->right >= left && rect->bottom >= top;
}

/* Новый узел списка грязных прямоугольников (из кучи). */
static GM_DIRTY_RECT *GmDirty_NewRect(int left, int top, int right, int bottom)
{
    GM_DIRTY_RECT *rect = GmHeap_Alloc(sizeof(GM_DIRTY_RECT));

    /* Особенность оригинала: результат GmHeap_Alloc не проверяется. */
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
    rect->next = NULL;
    return rect;
}

/*
 * Перерисовать все грязные прямоугольники. Видимые спрайты пула (не объекты)
 * сортируются по слою — больший слой рисуется первым, так что меньший оказывается
 * сверху; последним рисуется собственный курсор. Для каждого прямоугольника
 * копируется фон комнаты, рисуются обрезанные спрайты и прямоугольник выводится
 * в окно (с растворением, если оно запрошено). Список прямоугольников освобождается.
 */
bool GmRender_Flush(void)
{
    /* Особенность оригинала: переполнение массива при большем числе спрайтов не проверяется. */
    GM_SPRITE *sprites[GM_MAX_DRAWN_SPRITES];
    GM_DIRTY_RECT *rect;
    GM_DIRTY_RECT *nextRect;
    GM_POOL_ENTRY *entry;
    int spriteCount;
    int i;
    int j;

    rect = g_DirtyRects;
    if (rect == NULL)
        return true;

    spriteCount = 0;
    for (entry = g_PoolTop; entry > g_PoolBottom; entry--)
    {
        if ((entry->flags & (GMSPR_USED | GMSPR_OBJECT | GMSPR_VISIBLE)) == (GMSPR_USED | GMSPR_VISIBLE))
            sprites[spriteCount++] = &entry->sprite;
    }

    for (i = 0; i < spriteCount; i++)
    {
        for (j = i + 1; j < spriteCount; j++)
        {
            if (sprites[i]->layer < sprites[j]->layer)
            {
                GM_SPRITE *swap = sprites[i];
                sprites[i] = sprites[j];
                sprites[j] = swap;
            }
        }
    }

    if (!g_GameHeader.systemCursor && g_CursorInWindow)
        sprites[spriteCount++] = &g_CursorSprite;

    do
    {
        int clipLeft;
        int clipTop;
        int clipRight;
        int clipBottom;

        nextRect = rect->next;

        clipLeft = rect->left - g_ScrollX;
        if (clipLeft < 0)
            clipLeft = 0;
        clipRight = rect->right - g_ScrollX;
        if (clipRight > g_ClipWidth)
            clipRight = g_ClipWidth;

        if (clipRight > clipLeft)
        {
            clipTop = rect->top - g_ScrollY;
            if (clipTop < 0)
                clipTop = 0;
            clipBottom = rect->bottom - g_ScrollY;
            if (clipBottom > g_ClipHeight)
                clipBottom = g_ClipHeight;

            if (clipBottom > clipTop)
            {
                GmFrame_CopyRect(g_Background->pixels, -clipLeft, -clipTop,
                                 g_Background->width, g_Background->height,
                                 clipLeft, clipTop, clipRight, clipBottom);

                for (i = 0; i < spriteCount; i++)
                {
                    const GM_SPRITE *sprite = sprites[i];
                    const GM_FRAME *frame = sprite->frame;
                    GM_IMAGE *image = frame->image;
                    GM_SPRITE_FLAGS flags = sprite->flags;
                    int drawX = sprite->x - g_ScrollX;
                    int drawY = sprite->y - g_ScrollY;
                    int width;
                    int height;
                    int spriteRight;
                    int spriteBottom;
                    int offsetX;
                    int offsetY;
                    uint8_t *pixels;
                    int skipX;
                    int skipY;
                    int drawLeft;
                    int drawTop;
                    int drawRight;
                    int drawBottom;

                    if (flags & GMSPR_SCREEN_ANCHORED)
                    {
                        drawX -= frame->hotX;
                        drawY -= frame->hotY;
                    }
                    if (drawX >= clipRight || drawY >= clipBottom)
                        continue;

                    width = image->width;
                    spriteRight = drawX + width;
                    if (spriteRight <= clipLeft)
                        continue;
                    height = image->height;
                    spriteBottom = drawY + height;
                    if (spriteBottom <= clipTop)
                        continue;

                    offsetY = drawY - clipTop;
                    pixels = GmImage_GetPixels(image);
                    offsetX = drawX - clipLeft;

                    /* skipX/skipY — отрицательный сдвиг, если спрайт выходит за левый/верхний край. */
                    skipX = offsetX < 0 ? offsetX : 0;
                    skipY = offsetY < 0 ? offsetY : 0;
                    drawLeft = offsetX < 0 ? clipLeft : drawX;
                    drawTop = offsetY < 0 ? clipTop : drawY;
                    drawRight = clipRight < spriteRight ? clipRight : spriteRight;
                    drawBottom = clipBottom < spriteBottom ? clipBottom : spriteBottom;

                    if (flags & GMSPR_MIRROR_X)
                    {
                        if (flags & GMSPR_MIRROR_Y)
                            GmFrame_DrawSpriteMirrorXY(pixels, skipX, skipY, width, height,
                                                       drawLeft, drawTop, drawRight, drawBottom);
                        else
                            GmFrame_DrawSpriteMirrorX(pixels, skipX, skipY, width, height,
                                                      drawLeft, drawTop, drawRight, drawBottom);
                    }
                    else if (flags & GMSPR_MIRROR_Y)
                    {
                        GmFrame_DrawSpriteMirrorY(pixels, skipX, skipY, width, height,
                                                  drawLeft, drawTop, drawRight, drawBottom);
                    }
                    else
                    {
                        GmFrame_DrawSprite(pixels, skipX, skipY, width, height,
                                           drawLeft, drawTop, drawRight, drawBottom);
                    }
                }

                GmFrame_Present(clipLeft, clipTop, clipRight, clipBottom, g_DissolvePending != 0);
            }
        }

        GmHeap_Free(rect);
        rect = nextRect;
    } while (rect != NULL);

    Host_Flush();
    g_DirtyRects = NULL;
    g_DissolvePending = 0;
    return true;
}

/*
 * Начальная настройка движка перед первой загрузкой: засев генератора, сброс контекста
 * скрипта, описателей сохраняемых переменных и громкостей; затем загрузка эпизода 0.
 */
bool GmGame_Init(void)
{
    GmRandom_SeedFromTime();
    g_UnusedObjectIndex = GM_NO_INDEX;
    g_CurObjectIndex = GM_NO_INDEX;
    g_CurObject = NULL;
    g_CurClass = NULL;
    g_UnusedObject = NULL;
    g_VarsSaveGameOnly = NULL;
    g_VarsKeepOnRestart = NULL;
    g_VarsKeepAlways = NULL;
    g_MovieDepth = 0;
    g_TextPending = false;
    g_GameRunning = false;
    g_FirstRun = true;
    g_SystemMusicVolume = 0;
    g_SystemSoundVolume = 0;
    g_TargetMusicVolume = 0;
    g_TargetSoundVolume = 0;
    g_CursorInWindow = false;
    return GmLoad_Run(0);
}

/* Сбросить признак идущей игры (обновление курсора по движению мыши запрещается). */
void GmGame_ClearRunning(void)
{
    g_GameRunning = false;
}

/*
 * Сброс вида при входе в комнату: освободить грязные прямоугольники (при blitRects —
 * вывести их в окно), забыть запомненный щелчок и фильтр классов, пересеять генератор
 * и выбрать первый фокусируемый объект.
 */
void GmRoom_ResetView(bool blitRects)
{
    GM_DIRTY_RECT *rect = g_DirtyRects;

    while (rect != NULL)
    {
        GM_DIRTY_RECT *next = rect->next;

        if (blitRects)
            GmFrame_Present(rect->left, rect->top, rect->right, rect->bottom, false);
        GmHeap_Free(rect);
        rect = next;
    }
    Host_Flush();

    g_DirtyRects = NULL;
    g_ClickTileClass = GMCLASS_ANY;
    g_ClickEvent = GMEV_NONE;
    g_ActiveClassFilter = NULL;
    GmRandom_SeedFromTime();
    g_FocusObject = NULL;
    GmFocus_Advance(NULL);
}

/*
 * Перезапуск такта: возврат в точку, поставленную GmGame_Tick. Так переход в комнату,
 * перезапуск игры и загрузка сохранения прерывают текущий проход правил.
 */
NORETURN void GmGame_Restart(void)
{
    longjmp(g_RestartJmp, 1);
}

/*
 * Инициализация игры: файл данных — сам исполняемый файл gameFilePath (игра дописана
 * в его хвост); начальная загрузка, окно или AVI-заставка, частота кадров, вывод,
 * звук (системные громкости становятся целевыми) и стартовый фильм.
 */
bool GmGame_Initialize(char *gameFilePath)
{
    if (!GmLoad_OpenGameFile(strcpy(g_GameFilePath, gameFilePath))
        || !GmGame_Init()
        || !Host_ShowWindowOrIntro()
        || !Host_SetFrameRate(g_SysVars->frameRate)
        || !GmFrameModule_Startup()
        || !GmAudio_Init(g_Settings.soundChannels, 0, &g_SystemSoundVolume, &g_SystemMusicVolume))
    {
        return false;
    }

    g_TargetSoundVolume = g_SystemSoundVolume;
    g_TargetMusicVolume = g_SystemMusicVolume;

    /* Проверяется только младший байт результата. */
    return (uint8_t)GmGame_PlayStartMovie() != 0;
}

/*
 * Один игровой такт (зовётся из цикла WinMain с частотой кадров). После перезапуска
 * такта (GmGame_Restart) или при выключенном вводе ввод этого такта отбрасывается.
 * Раздаёт ввод, двигает громкости, обновляет системные переменные, выполняет проход
 * правил (или текстовый такт) и повторяет его, пока задан фильтр классов; затем
 * обновляет курсор и перерисовывает экран. false — игра запросила выход.
 */
bool GmGame_Tick(int clipWidth, int clipHeight, int mouseX, int mouseY, int clickX, int clickY,
                 GM_EVENT event, uint8_t actionKey, uint8_t inputChar, bool cursorInWindow)
{
    /* После longjmp event/actionKey/inputChar всё равно перезаписываются,
       поэтому их значения на момент перезапуска неважны. */
    if (setjmp(g_RestartJmp) != 0)
    {
        /* В оригинале longjmp восстанавливал ESP и тем самым бросал всё,
           что скрипты оставили на стеке; у машины скриптов стек свой. */
        GmVm_ResetStack();
        event = GMEV_NONE;
        actionKey = GMEV_NONE;
        inputChar = GMEV_NONE;
    }
    else if (!g_Settings.inputEnabled)
    {
        event = GMEV_NONE;
        actionKey = GMEV_NONE;
        inputChar = GMEV_NONE;
    }

    g_ClipWidth = clipWidth;
    g_ClipHeight = clipHeight;
    g_InputChar = inputChar;
    g_CursorInWindow = cursorInWindow;
    GmInput_Dispatch(mouseX, mouseY, clickX, clickY, event, actionKey);
    GmVolume_FadeStep();

    do
    {
        if (!GmSysVars_Update())
            return false;
        if (g_TextPending)
        {
            if (!GmTick_TextMode(event))
                return false;
        }
        else if (!GmRules_RunTick())
        {
            return false;
        }

        /* Повторные проходы при заданном фильтре классов идут без символа ввода. */
        if (g_ActiveClassFilter != NULL)
            g_InputChar = GMEV_NONE;
    } while (g_ActiveClassFilter != NULL);

    return GmCursor_Update(mouseX, mouseY) && GmRender_Flush();
}

/*
 * Добавить прямоугольник в список грязных: если он пересекается с имеющимся —
 * расширить тот, иначе добавить узел из кучи. После расширения пересекающиеся узлы
 * сливаются, пока такие есть.
 */
void GmDirty_Add(int left, int top, int right, int bottom)
{
    GM_DIRTY_RECT *rect = g_DirtyRects;
    int grown;

    if (rect == NULL)
    {
        g_DirtyRects = GmDirty_NewRect(left, top, right, bottom);
        return;
    }

    while (!GmDirty_Overlaps(rect, left, top, right, bottom))
    {
        if (rect->next == NULL)
        {
            rect->next = GmDirty_NewRect(left, top, right, bottom);
            return;
        }
        rect = rect->next;
    }

    grown = 0;
    if (rect->left > left)
    {
        rect->left = left;
        grown++;
    }
    if (rect->right < right)
    {
        rect->right = right;
        grown++;
    }
    if (rect->top > top)
    {
        rect->top = top;
        grown++;
    }
    if (rect->bottom < bottom)
    {
        rect->bottom = bottom;
        grown++;
    }
    if (!grown)
        return;

    /* Сливать пересекающиеся пары: узел cur поглощается первым пересекающимся
       с ним узлом дальше по списку, и поиск начинается заново. */
    for (;;)
    {
        GM_DIRTY_RECT *prev = NULL;
        GM_DIRTY_RECT *cur;
        GM_DIRTY_RECT *other = NULL;

        for (cur = g_DirtyRects; cur != NULL; prev = cur, cur = cur->next)
        {
            for (other = cur->next; other != NULL; other = other->next)
            {
                if (GmDirty_Overlaps(other, cur->left, cur->top, cur->right, cur->bottom))
                    break;
            }
            if (other != NULL)
                break;
        }
        if (cur == NULL)
            return;

        if (cur->left < other->left)
            other->left = cur->left;
        if (cur->right > other->right)
            other->right = cur->right;
        if (cur->top < other->top)
            other->top = cur->top;
        if (cur->bottom > other->bottom)
            other->bottom = cur->bottom;

        if (prev != NULL)
            prev->next = cur->next;
        else
            g_DirtyRects = cur->next;
        GmHeap_Free(cur);
    }
}

/*
 * Полная перерисовка комнаты: стиль перехода из заголовка игры (цвет сетки и растворение),
 * палитра фона и весь вид — в грязные прямоугольники.
 */
bool GmRoom_RepaintAll(void)
{
    const uint8_t *palette = &g_Background->pixels[g_Background->paletteOffset];

    g_DissolvePending = g_GameHeader.transition;
    Host_SetPalette(palette, 256, (char)g_GameHeader.transition, true);
    GmDirty_Add(0, 0, g_World.viewWidth, g_World.viewHeight);
    return true;
}

/*
 * Завершение игры: остановить звуки и музыку, вернуть параметры во включённое состояние
 * и системные громкости, записать автосохранение, выключить звук и вывод, закрыть файл данных.
 */
void GmGame_Shutdown(void)
{
    GmAudio_StopSounds();
    GmAudio_StopMusic();
    g_Settings.moviesOn = true;
    g_Settings.musicOn = true;
    g_Settings.soundOn = true;
    g_Settings.inputEnabled = true;
    g_FirstRun = true;
    g_Settings.soundVolume = g_SystemSoundVolume;
    GmAudio_SetSoundVolume(g_SystemSoundVolume);
    g_Settings.musicVolume = g_SystemMusicVolume;
    GmAudio_SetMusicVolume(g_SystemMusicVolume);
    GmAutosave_Write(g_AutosaveSlot);
    GmAudio_Shutdown();
    GmFrameModule_Shutdown();
    GmBlitModule_Shutdown();
    GmGame_ClearRunning();
    GmLoad_CloseGameFile();
}

/* Ошибка «Insufficient memory.»: запомнить сообщение и вернуть NULL. */
void *GmError_OutOfMemory(void)
{
    Host_SetError(s_InsufficientMemory);
    return NULL;
}

/* Подлежит ли запись пула сохранению вместе с комнатой (экземпляр или не одноразовый спрайт). */
static bool GmRoom_IsSavedEntry(const GM_POOL_ENTRY *entry)
{
    uint8_t kind = entry->flags & (GMOBJ_USED | GMOBJ_INSTANCE);

    if (kind == (GMOBJ_USED | GMOBJ_INSTANCE))
        return true;
    return kind == GMSPR_USED && (entry->flags & GMSPR_PLAY_ONCE) == 0;
}

/*
 * Сохранить состояние текущей комнаты в g_Rooms[roomIndex].savedState: строки карты
 * тайлов и записи пула — экземпляры, их спрайт покоя и принадлежащие им спрайты (глифы)
 * с переписанным номером владельца, а также свободные не одноразовые спрайты.
 * Номер владельца — порядковый номер записи в сохранённом состоянии. После сохранения
 * пул опустошается.
 */
void GmRoom_SaveState(int roomIndex)
{
    GM_ROOM *room = &g_Rooms[roomIndex];
    GM_POOL_ENTRY *entry;
    GM_POOL_ENTRY *out;
    int savedCount;
    int row;

    room->savedObjectCount = 0;
    for (entry = g_PoolTop; entry > g_PoolBottom; entry--)
    {
        if (GmRoom_IsSavedEntry(entry))
            room->savedObjectCount++;
    }

    room->savedState = GmHeap_Alloc(sizeof(GM_POOL_ENTRY) * room->savedObjectCount
                                    + sizeof(GM_TILE) * g_MapTileCount);
    for (row = 0; row < g_World.mapHeight; row++)
    {
        memcpy((GM_TILE *)room->savedState + row * g_World.mapWidth,
               &g_TileMap[row << g_TileMapShift],
               sizeof(GM_TILE) * g_World.mapWidth);
    }

    out = (GM_POOL_ENTRY *)((GM_TILE *)room->savedState + g_MapTileCount);
    savedCount = 0;
    for (entry = g_PoolTop; entry > g_PoolBottom; entry--)
    {
        uint8_t flags = entry->flags;
        uint8_t kind = flags & (GMOBJ_USED | GMOBJ_INSTANCE);

        if (kind == (GMOBJ_USED | GMOBJ_INSTANCE))
        {
            GM_OBJECT *obj = &entry->object;
            uint16_t savedObjIndex = (uint16_t)savedCount;
            int objIndex;
            GM_POOL_ENTRY *child;

            /* Анимация хода теряется: текущим спрайтом объекта становится спрайт покоя. */
            obj->spriteIndex = GM_POOL_NONE;
            if (obj->restSpriteIndex == GM_POOL_NONE)
            {
                *out++ = *entry;
                entry->flags = 0;
            }
            else
            {
                GM_POOL_ENTRY *restSprite = g_PoolTop - obj->restSpriteIndex;

                obj->restSpriteIndex = (uint16_t)(savedCount + 1);
                obj->spriteIndex = (uint16_t)(savedCount + 1);
                *out++ = *entry;
                entry->flags = 0;
                savedCount++;

                restSprite->sprite.ownerIndex = savedObjIndex;
                restSprite->flags |= GMSPR_VISIBLE;
                *out++ = *restSprite;
                restSprite->flags = 0;
            }
            savedCount++;

            objIndex = (int)(g_PoolTop - entry);
            for (child = g_PoolTop; child > g_PoolBottom; child--)
            {
                if ((child->flags & (GMSPR_USED | GMSPR_OBJECT | GMSPR_PLAY_ONCE)) == GMSPR_USED
                    && child->sprite.ownerIndex == objIndex)
                {
                    child->sprite.ownerIndex = savedObjIndex;
                    *out++ = *child;
                    child->flags = 0;
                    savedCount++;
                }
            }
        }
        else if (kind == GMSPR_USED && entry->sprite.ownerIndex == GM_POOL_NONE
                 && (flags & GMSPR_PLAY_ONCE) == 0)
        {
            *out++ = *entry;
            entry->flags = 0;
            savedCount++;
        }
    }

    GmPool_Reset();
}

/*
 * Войти в комнату roomIndex: сохранить текущую (если не keepCurrent), прочитать фон
 * комнаты, восстановить карту тайлов и записи пула из сохранённого состояния и
 * освободить его, сбросить вид и (если не keepCurrent) перерисовать.
 * false — не удалось прочитать фон или перерисовать.
 */
bool GmRoom_Enter(int roomIndex, bool keepCurrent)
{
    GM_ROOM *room;
    const GM_POOL_ENTRY *saved;
    int count;
    int row;

    if (g_CurrentRoom != -1 && !keepCurrent)
        GmRoom_SaveState(g_CurrentRoom);

    g_CurrentRoom = roomIndex;
    room = &g_Rooms[roomIndex];
    if (!GmRead_Seek((int)room->fileOffset, SEEK_SET) || !GmRead_Record(room->loadBuffer))
        return false;

    for (row = 0; row < g_World.mapHeight; row++)
    {
        memcpy(&g_TileMap[row << g_TileMapShift],
               (const GM_TILE *)room->savedState + row * g_World.mapWidth,
               sizeof(GM_TILE) * g_World.mapWidth);
    }

    saved = (const GM_POOL_ENTRY *)((const GM_TILE *)room->savedState + g_MapTileCount);
    for (count = room->savedObjectCount; count != 0; count--)
        *GmPool_Alloc() = *saved++;

    room->savedObjectCount = 0;
    GmHeap_Free(room->savedState);
    GmRoom_ResetView(false);
    if (!keepCurrent)
        return GmRoom_RepaintAll();
    return true;
}

/* Стартовый фильм: если фильмы есть и заставка — фильм, проиграть фильм 0. */
int GmGame_PlayStartMovie(void)
{
    if (g_GameHeader.movieCount != 0 && g_GameHeader.introMode == GMINTRO_MOVIE)
        return Native_PlayMovie(0);
    return 1;
}
