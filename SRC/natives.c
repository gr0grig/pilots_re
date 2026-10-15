/*
 * Встроенные функции скриптов (Native_*) и таблица g_NativeFunctions, по которой
 * их вызывает машина скриптов (код OP_CALL_NATIVE, номер — операнд команды).
 *
 * Скрипт кладёт доводы на стек машины слева направо, поэтому последний довод
 * лежит на вершине (args[0]). Переходники Invoke_* снимают доводы и вызывают
 * функцию с её настоящим прототипом.
 */
#include "gm.h"

#include <stdio.h>
#include <string.h>

/* ========================================================================== */
/* Проверки состояния текущего объекта                                        */
/* ========================================================================== */

/* №0. Разрешить продолжение правил: после сработавшего правила проход перейдёт
   к следующему правилу класса (на последнем правиле — уступит такт этому объекту). */
int Native_ContinueRules(void)
{
    g_ContinueRules = true;
    return 1;
}

/* №1. Истина, если у текущего объекта нет показываемого спрайта. */
int Native_HasNoSprite(void)
{
    return g_CurObject->spriteIndex == GM_POOL_NONE;
}

/* №2. Истина, если спрайт покоя текущего объекта показывает графику graphicIndex
   (графика ищется по адресу кадра). */
int Native_RestGraphicIs(int graphicIndex)
{
    uint16_t restSpriteIndex = g_CurObject->restSpriteIndex;

    return restSpriteIndex != GM_POOL_NONE
        && GmSprite_FindGraphic(&g_PoolTop[-restSpriteIndex].sprite) - g_Graphics == graphicIndex;
}

/* №3. Истина, если указатель наведён на объект и кнопка не отпущена. */
int Native_IsHovered(void)
{
    return (g_CurObject->input & (GMIN_HOVER | GMIN_RELEASED)) == GMIN_HOVER;
}

/* №4. Истина, если по объекту нажата кнопка мыши и она не отпущена. */
int Native_IsPressed(void)
{
    return (g_CurObject->input & (GMIN_PRESSED | GMIN_RELEASED)) == GMIN_PRESSED;
}

/* №5. Сравнить разряды мыши объекта (наведение, нажатие, отпускание) с bits. */
int Native_MouseInputIs(int bits)
{
    return (g_CurObject->input & (GMIN_HOVER | GMIN_PRESSED | GMIN_RELEASED)) == bits;
}

/* №6. Сравнить разряды клавиатуры объекта (признак клавиши, Shift, номер направления) с bits. */
int Native_KeyInputIs(int bits)
{
    return (g_CurObject->input & (GMIN_DIR_MASK | GMIN_SHIFT | GMIN_KEY)) == bits;
}

/* №7. Истина, если нажата клавиша направления, совпадающая с dirAndShift
   (разряды 0–2 — направление 0=С … 7=СЗ по часовой, разряд 3 — Shift)
   с учётом текущего преобразования правила. */
int Native_IsDirectionKey(uint8_t dirAndShift)
{
    GM_INPUT_BITS input = g_CurObject->input;

    if ((input & GMIN_KEY) && (input & GMIN_SHIFT) == (dirAndShift & GMIN_SHIFT))
        return GmDirIndex_MatchesRotated(input & GMIN_DIR_MASK, dirAndShift & GMIN_DIR_MASK);
    return 0;
}

/* №8. Истина, если слой текущего объекта равен layer. */
int Native_LayerIs(int layer)
{
    return g_CurObject->layer == layer;
}

/* №9. Выполнить правило панели в абсолютном режиме (без пересоздания её элементов). */
int Native_RunPanel(int panelIndex)
{
    return GmRule_RunWithSavedContext(&g_Panels[panelIndex].rule);
}

/* №10. Истина, если под текущим объектом пустой тайл. */
int Native_IsUnderTileEmpty(void)
{
    return g_CurObject->underTile.classIndex == GMCLASS_EMPTY_TILE;
}

/* №11. Истина, если под текущим объектом тайл класса classIndex. */
int Native_UnderTileIs(int classIndex)
{
    return g_CurObject->underTile.classIndex == classIndex;
}

/* №12. Не ноль, если класс тайла под текущим объектом входит в набор classSet
   (возвращается сам разряд членства, а не 0/1). */
int Native_UnderTileInSet(int classSet)
{
    uint8_t classIndex = g_CurObject->underTile.classIndex;

    return g_ClassSets[classSet].memberBits[classIndex >> 3] & (1 << (classIndex & 7));
}

/* №13. Истина, если знак, введённый в этом такте, входит в строку chars. */
int Native_InputCharIn(const uint8_t *chars)
{
    for (; *chars != '\0'; ++chars)
    {
        if (*chars == g_InputChar)
            return 1;
    }
    return 0;
}

/* ========================================================================== */
/* Смена эпизода и комнаты                                                    */
/* ========================================================================== */

/* №14. Перезапустить игру с эпизода startEntry: загрузить его и перезапустить такт. */
NORETURN void Native_RestartGame(int startEntry)
{
    GmLoad_Run(startEntry);
    GmGame_Restart();
}

/* №15. Перейти в комнату roomIndex и перезапустить такт. */
NORETURN void Native_GotoRoom(int roomIndex)
{
    GmRoom_Enter(roomIndex, false);
    GmGame_Restart();
}

/* Пустая функция без единой ссылки: в таблицу встроенных функций не входит.
   Вероятно, исключённая встроенная функция; что она делала, не установить. */
void Native_UnreferencedNop(int unused)
{
    (void)unused;
}

/* ========================================================================== */
/* Звук, музыка, фильмы                                                       */
/* ========================================================================== */

/* №16. Играть дорожку track (если музыка включена и не идёт постройка комнаты). */
int Native_PlayMusic(int track)
{
    if (g_BuildingRoom)
        return 1;
    GmMusic_Stop();
    if (!g_Settings.musicOn)
        return 1;
    g_Settings.currentMusic = track;
    return GmMusic_Start(g_MusicTracks[track]);
}

/* №17. Сыграть звук soundIndex (если звук включён и не идёт постройка комнаты);
   режим 1 — начать вывод, если устройство простаивает. */
int Native_PlaySound(int soundIndex)
{
    if (!g_BuildingRoom && g_Settings.soundOn)
        GmAudio_PlaySound(g_Sounds[soundIndex].data, 0xFF, 0, 1);
    return 1;
}

/* №18. Проиграть встроенный фильм movie (если фильмы включены);
   g_MovieDepth блокирует курсор на время фильма. */
int Native_PlayMovie(int movie)
{
    int played;

    if (!g_Settings.moviesOn)
        return 1;
    ++g_MovieDepth;
    played = GmMovie_Play(g_MovieOffsets[movie]);
    --g_MovieDepth;
    return played;
}

/* ========================================================================== */
/* Спрайты, панели, текст                                                     */
/* ========================================================================== */

/* №19. Показать на тайле текущего объекта графику graphicIndex
   (с выбором кадров по направлению хода). */
int Native_ShowGraphic(int graphicIndex)
{
    g_SpriteSetThisStep = true;
    return GmSprite_Create(graphicIndex,
                           (uint16_t)(g_GameHeader.tileWidth * g_CurTileX),
                           (uint16_t)(g_GameHeader.tileHeight * g_CurTileY),
                           false);
}

/* №20. Показать панель: создать экранные спрайты её элементов (до isLast)
   и выполнить её правило. */
int Native_ShowPanel(int panelIndex)
{
    GM_PANEL *panel = &g_Panels[panelIndex];
    const GM_PANEL_ITEM *item = panel->items;

    if (item != NULL)
    {
        for (;;)
        {
            GmSprite_Create(item->graphicIndex, item->x, item->y, true);
            if (item->isLast)
                break;
            ++item;
        }
    }
    return GmRule_RunWithSavedContext(&panel->rule);
}

/* №21. Запросить ввод текста на тайле объекта: text — поток знаков с полями
   ввода GMTEXT_FIELD, fontIndex — шрифт. */
int Native_InputTextAtTile(int fontIndex, uint8_t *text)
{
    return GmText_StartInput(text, fontIndex,
                             g_GameHeader.tileWidth * g_CurTileX,
                             g_GameHeader.tileHeight * g_CurTileY);
}

/* №22. Запросить ввод текста в позиции и шрифтом первого элемента панели. */
int Native_InputTextAtPanel(int panelIndex, uint8_t *text)
{
    const GM_PANEL_ITEM *item = g_Panels[panelIndex].items;

    return GmText_StartInput(text, item->graphicIndex, item->x, item->y);
}

/* №23. Напечатать текст (с подстановками GMTEXT_FIELD) на тайле объекта. */
int Native_PrintAtTile(int fontIndex, uint8_t *text)
{
    return GmText_Print(text, fontIndex,
                        g_GameHeader.tileWidth * g_CurTileX,
                        (uint16_t)(g_GameHeader.tileHeight * g_CurTileY));
}

/* №24. Напечатать текст в позиции и шрифтом первого элемента панели. */
int Native_PrintAtPanel(int panelIndex, uint8_t *text)
{
    const GM_PANEL_ITEM *item = g_Panels[panelIndex].items;

    return GmText_Print(text, item->graphicIndex, item->x, item->y);
}

/* №25. Задать слой текущего объекта и его спрайтов. */
int Native_SetLayer(uint8_t layer)
{
    GM_OBJECT *obj = g_CurObject;

    if (obj->layer != layer)
    {
        obj->layer = layer;
        if (obj->restSpriteIndex != GM_POOL_NONE)
            g_PoolTop[-obj->restSpriteIndex].sprite.layer = layer;
        if (obj->spriteIndex != GM_POOL_NONE)
        {
            GM_SPRITE *sprite = &g_PoolTop[-obj->spriteIndex].sprite;

            sprite->layer = layer;
            GmSprite_Invalidate(sprite);
        }
    }
    return 1;
}

/* №26. Убрать текст текущего объекта. */
int Native_RemoveText(void)
{
    GmObject_RemoveText(g_CurObject);
    return 1;
}

/* №27. Уничтожить статический тайл под текущим объектом. */
int Native_DestroyUnderTile(void)
{
    GmCurObject_DestroyUnderTile();
    return 1;
}

/* №28. Поставить под объект тайл класса classIndex. */
int Native_SetUnderTile(int classIndex)
{
    GmCurObject_SetUnderTile(classIndex, false);
    return 1;
}

/* №29. Поставить под объект случайный класс из набора classSet. */
int Native_SetUnderTileFromSet(int classSet)
{
    GmCurObject_SetUnderTile(classSet, true);
    return 1;
}

/* №30. Убрать показываемый спрайт объекта. */
void Native_RemoveSprite(void)
{
    uint16_t spriteIndex = g_CurObject->spriteIndex;

    if (spriteIndex != GM_POOL_NONE)
    {
        /* Особенность оригинала: обнуляются оба индекса, но удаляется только текущий
           спрайт — если шла анимация хода, спрайт покоя остаётся в пуле без ссылки
           до смены комнаты. */
        g_CurObject->restSpriteIndex = GM_POOL_NONE;
        g_CurObject->spriteIndex = GM_POOL_NONE;
        GmSprite_Destroy(&g_PoolTop[-spriteIndex].sprite);
    }
}

/* №31. Сделать курсором графику graphicIndex. */
int Native_SetCursor(int graphicIndex)
{
    return GmCursor_SetGraphic(graphicIndex, true);
}

/* №32. Вернуть курсор к графике 0. */
int Native_ResetCursor(void)
{
    return GmCursor_SetGraphic(0, true);
}

/* №33. Слой объекта по строке: mapHeight − tileY (нижние строки рисуются поверх).
   Счёт ведётся в байте. */
int Native_LayerFromRow(void)
{
    g_CurObject->layer = (uint8_t)(g_World.mapHeight - g_CurObject->tile.y);
    return 1;
}

/* №34. Выдать слой текущего объекта в *layer. */
int Native_GetLayer(uint8_t *layer)
{
    *layer = g_CurObject->layer;
    return 1;
}

/* ========================================================================== */
/* Поиск пути                                                                 */
/* ========================================================================== */

/* Номера встроенных функций поиска пути. При неизвестном коде режима оригинал
   возвращает содержимое EAX на входе, а машина скриптов вызывает встроенную
   функцию, держа в EAX её номер, — его и возвращаем. */
enum
{
    NATIVE_SEEK_POINTER = 35,
    NATIVE_SEEK_CLASS = 36,
    NATIVE_SEEK_CLASS_SET = 37,
};

/* №35. Поиск пути к тайлу указателя или щелчка, без класса. mode: 3–5 — тайл под
   указателем, 6–8 — щелчок действием 1, 9–11 — действием 2; внутри тройки: прямое
   направление; прямое, если тайл пуст; путь по пустым тайлам.
   Результат — направления g_PathDir4/g_PathDir8. */
int Native_SeekPointer(GM_SEEK_COMMAND mode)
{
    switch (mode)
    {
    case GMSEEKCMD_POINTER_DIRECT:
        return GmPath_SeekClass(GMEV_NONE, GMCLASS_ANY, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_POINTER_EMPTY:
        return GmPath_SeekClass(GMEV_NONE, GMCLASS_EMPTY_TILE, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_POINTER_PATH:
        return GmPath_SeekClass(GMEV_NONE, GMCLASS_EMPTY_TILE, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_ACTION1_DIRECT:
        return GmPath_SeekClass(GMEV_ACTION1, GMCLASS_ANY, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_ACTION1_EMPTY:
        return GmPath_SeekClass(GMEV_ACTION1, GMCLASS_EMPTY_TILE, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_ACTION1_PATH:
        return GmPath_SeekClass(GMEV_ACTION1, GMCLASS_EMPTY_TILE, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_ACTION2_DIRECT:
        return GmPath_SeekClass(GMEV_ACTION2, GMCLASS_ANY, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_ACTION2_EMPTY:
        return GmPath_SeekClass(GMEV_ACTION2, GMCLASS_EMPTY_TILE, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_ACTION2_PATH:
        return GmPath_SeekClass(GMEV_ACTION2, GMCLASS_EMPTY_TILE, GMSEEK_THROUGH_EMPTY);
    default:
        return NATIVE_SEEK_POINTER;
    }
}

/* №36. Поиск пути с классом classIndex: mode 1/2 — к ближайшему тайлу класса
   (прямое направление / путь в обход занятых); 3–5, 6–8, 9–11 — как
   Native_SeekPointer, но цель должна быть класса classIndex, а третий вариант
   тройки — путь только по тайлам этого класса. */
int Native_SeekClass(uint8_t classIndex, GM_SEEK_COMMAND mode)
{
    switch (mode)
    {
    case GMSEEKCMD_NEAREST_DIRECT:
        return GmPath_SeekClass(GMEV_NO_TARGET, classIndex, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_NEAREST_PATH:
        return GmPath_SeekClass(GMEV_NO_TARGET, classIndex, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_POINTER_DIRECT:
        return GmPath_SeekClass(GMEV_NONE, classIndex, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_POINTER_EMPTY:
        return GmPath_SeekClass(GMEV_NONE, classIndex, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_POINTER_PATH:
        return GmPath_SeekClass(GMEV_NONE, classIndex, classIndex);
    case GMSEEKCMD_ACTION1_DIRECT:
        return GmPath_SeekClass(GMEV_ACTION1, classIndex, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_ACTION1_EMPTY:
        return GmPath_SeekClass(GMEV_ACTION1, classIndex, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_ACTION1_PATH:
        return GmPath_SeekClass(GMEV_ACTION1, classIndex, classIndex);
    case GMSEEKCMD_ACTION2_DIRECT:
        return GmPath_SeekClass(GMEV_ACTION2, classIndex, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_ACTION2_EMPTY:
        return GmPath_SeekClass(GMEV_ACTION2, classIndex, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_ACTION2_PATH:
        return GmPath_SeekClass(GMEV_ACTION2, classIndex, classIndex);
    default:
        return NATIVE_SEEK_CLASS;
    }
}

/* №37. То же, что Native_SeekClass, но вместо класса — набор классов. */
int Native_SeekClassSet(uint8_t classSet, GM_SEEK_COMMAND mode)
{
    switch (mode)
    {
    case GMSEEKCMD_NEAREST_DIRECT:
        return GmPath_SeekClassSet(GMEV_NO_TARGET, classSet, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_NEAREST_PATH:
        return GmPath_SeekClassSet(GMEV_NO_TARGET, classSet, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_POINTER_DIRECT:
        return GmPath_SeekClassSet(GMEV_NONE, classSet, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_POINTER_EMPTY:
        return GmPath_SeekClassSet(GMEV_NONE, classSet, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_POINTER_PATH:
        return GmPath_SeekClassSet(GMEV_NONE, classSet, classSet);
    case GMSEEKCMD_ACTION1_DIRECT:
        return GmPath_SeekClassSet(GMEV_ACTION1, classSet, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_ACTION1_EMPTY:
        return GmPath_SeekClassSet(GMEV_ACTION1, classSet, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_ACTION1_PATH:
        return GmPath_SeekClassSet(GMEV_ACTION1, classSet, classSet);
    case GMSEEKCMD_ACTION2_DIRECT:
        return GmPath_SeekClassSet(GMEV_ACTION2, classSet, GMSEEK_DIRECTION_ONLY);
    case GMSEEKCMD_ACTION2_EMPTY:
        return GmPath_SeekClassSet(GMEV_ACTION2, classSet, GMSEEK_THROUGH_EMPTY);
    case GMSEEKCMD_ACTION2_PATH:
        return GmPath_SeekClassSet(GMEV_ACTION2, classSet, classSet);
    default:
        return NATIVE_SEEK_CLASS_SET;
    }
}

/* №38. Истина, если путь найден и основное (4-направленное) направление равно dir. */
int Native_PathDir4Is(int dir)
{
    return g_PathFound && dir == g_PathDir4;
}

/* №39. Истина, если путь найден и 8-направленное направление равно dir. */
int Native_PathDir8Is(int dir)
{
    return g_PathFound && dir == g_PathDir8;
}

/* №40. Как Native_PathDir4Is, но baseDir поворачивается текущим преобразованием правила. */
int Native_PathDir4IsRotated(int baseDir)
{
    return g_PathFound && GmDirIndex_MatchesRotated(g_PathDir4, baseDir);
}

/* №41. Как Native_PathDir8Is, но baseDir поворачивается текущим преобразованием правила. */
int Native_PathDir8IsRotated(int baseDir)
{
    return g_PathFound && GmDirIndex_MatchesRotated(g_PathDir8, baseDir);
}

/* №42. Действие с найденным путём: забыть путь и щелчок, очистить тайл цели
   (пустой тайл с текущим направлением) или переместить объект на цель. */
int Native_PathAction(GM_PATH_ACTION action)
{
    /* Оригинал не заполняет смещение клетки размещения и класс клетки хода (там мусор
       из регистров и стека); GmTile_Place и GmCurObject_Move эти байты не читают. */
    GM_RULE_CELL cell = { .raw = 0 };

    if (!g_PathFound)
        return 1;

    switch (action)
    {
    case GMPATH_FORGET:
        g_PathFound = false;
        g_ClickEvent = GMEV_CLEARED;
        g_PointerEvent = GMEV_CLEARED;
        g_ClickTileClass = GMCLASS_ANY;
        break;
    case GMPATH_CLEAR_GOAL:
        cell.classIndex = GMCLASS_EMPTY_TILE;
        cell.flags = g_CurDirection;
        GmTile_Place(cell, g_PathGoalX, g_PathGoalY);
        break;
    case GMPATH_MOVE_TO_GOAL:
        /* Смещение считается в байте. */
        cell.flags = g_CurDirection;
        cell.dx = (int8_t)(g_PathGoalX - g_PathOriginX);
        cell.dy = (int8_t)(g_PathGoalY - g_PathOriginY);
        GmCurObject_Move(cell);
        break;
    }
    return 1;
}

/* №43. Поставить класс classIndex на тайл цели пути (с текущим направлением). */
int Native_PlaceAtGoal(uint8_t classIndex)
{
    if (g_PathFound)
    {
        GM_RULE_CELL cell = { .raw = 0 };

        cell.classIndex = classIndex;
        cell.flags = g_CurDirection;
        GmTile_Place(cell, g_PathGoalX, g_PathGoalY);
    }
    return 1;
}

/* №44. Поставить на тайл цели пути случайный класс набора classSet. */
int Native_PlaceAtGoalFromSet(uint8_t classSet)
{
    if (g_PathFound)
    {
        GM_RULE_CELL cell = { .raw = 0 };

        cell.classIndex = classSet;
        cell.flags = g_CurDirection | GMCELL_BY_CLASS_SET;
        GmTile_Place(cell, g_PathGoalX, g_PathGoalY);
    }
    return 1;
}

/* №45. Истина, если mask пересекается с флагами объекта
   (направление в старшей тетраде, GMOBJ_MOVED — двигался в такте). */
int Native_ObjectFlagsAny(uint8_t mask)
{
    return (mask & g_CurObject->flags) != 0;
}

/* ========================================================================== */
/* Прочее: строки, параметры, сохранения, справка, внешние программы          */
/* ========================================================================== */

/* №46. strcpy(dst, src) для скриптов. */
char *Native_StrCopy(char *dst, char *src)
{
    return strcpy(dst, src);
}

/* №47. Прочитать параметр: ввод включён, звук включён, целевая громкость музыки
   не ноль, фильмы включены. */
int Native_GetOption(GM_OPTION_QUERY which)
{
    switch (which)
    {
    case GMOPTQ_INPUT:
        return g_Settings.inputEnabled != 0;
    case GMOPTQ_SOUND:
        return g_Settings.soundOn != 0;
    case GMOPTQ_MUSIC:
        return g_TargetMusicVolume != 0;
    case GMOPTQ_MOVIES:
        return g_Settings.moviesOn != 0;
    default:
        /* Оригинал проверяет неопределённый регистр CL; при вызове из машины скриптов
           там код команды вызова (не ноль), поэтому результат — 1. */
        return 1;
    }
}

/* №48. Изменить параметр: ввод, звук (целевая громкость 0 или системная),
   музыка (целевая громкость 0 или системная), фильмы — выключить или включить. */
int Native_SetOption(GM_OPTION_SET which)
{
    switch (which)
    {
    case GMOPT_INPUT_OFF:
    case GMOPT_INPUT_ON:
        g_Settings.inputEnabled = which == GMOPT_INPUT_ON;
        break;
    case GMOPT_SOUND_OFF:
        g_Settings.soundOn = false;
        g_TargetSoundVolume = 0;
        break;
    case GMOPT_SOUND_ON:
        g_Settings.soundOn = true;
        g_TargetSoundVolume = g_SystemSoundVolume;
        break;
    case GMOPT_MUSIC_OFF:
        g_TargetMusicVolume = 0;
        break;
    case GMOPT_MUSIC_ON:
        g_TargetMusicVolume = g_SystemMusicVolume;
        break;
    case GMOPT_MOVIES_OFF:
    case GMOPT_MOVIES_ON:
        g_Settings.moviesOn = which == GMOPT_MOVIES_ON;
        break;
    }
    return 1;
}

/* №49. Операция с сохранением в слоте slot: удалить, сохранить или загрузить
   (загрузка при успехе не возвращается). При неизвестном коде возвращается сам код. */
int Native_SaveGameOp(int slot, GM_SAVE_OPERATION action)
{
    switch (action)
    {
    case GMSAVEOP_DELETE:
        return GmSaveGame_Delete(slot);
    case GMSAVEOP_SAVE:
        return GmSaveGame_Save(slot);
    case GMSAVEOP_LOAD:
        return GmSaveGame_Load(slot);
    default:
        return action;
    }
}

/* №50. Обрабатывать в проходе правил только объекты классов из набора classSet
   (проход повторяется до снятия фильтра). */
int Native_SetClassFilter(int classSet)
{
    g_ActiveClassFilter = g_ClassSets[classSet].memberBits;
    return 1;
}

/* №51. Снять фильтр классов. */
int Native_ClearClassFilter(void)
{
    g_ActiveClassFilter = NULL;
    return 1;
}

/* №52. Показать справку: тему topic или содержание, если тема — «?». */
int Native_ShowHelp(const char *topic)
{
    return Host_ShowHelp(topic);
}

/* №53. Переназначить клавишу направления index (0..7: С, СВ, В, ЮВ, Ю, ЮЗ, З, СЗ)
   на виртуальный код *virtualKey. Возвращает этот код. */
int Native_SetDirKey(const uint8_t *virtualKey, int index)
{
    g_Settings.dirKeys[index] = *virtualKey;
    return *virtualKey;
}

/* №54. Случайное число в [0, range) по генератору ANSI C. */
uint32_t Native_Random(int range)
{
    uint32_t value = ((uint32_t)range * (g_RandomSeed >> 16)) >> 16;

    g_RandomSeed = g_RandomSeed * 1103515245u + 12345u;
    return value;
}

/* №55. Проиграть AVI: строка «имя x y ширина высота». Звук и музыка на это время
   останавливаются, затем перезапускаются. */
int Native_PlayAvi(const char *args)
{
    char aviName[64];
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool played;

    GmAudio_StopAndShutdown();
    /* Особенность оригинала: имя читается через «%s» без ограничения длины. */
    sscanf(args, s_PlayAviArgsFormat, aviName, &x, &y, &width, &height);
    played = Host_PlayAvi(aviName, x, y, width, height);
    GmAudio_Restart();
    return played;
}

/* №56. Запустить внешнюю программу и дождаться её завершения
   (звук на это время выключается). */
int Native_RunProgram(char *commandLine)
{
    bool ok;

    GmAudio_StopAndShutdown();
    ok = Host_RunProgram(commandLine);
    GmAudio_Restart();
    return ok;
}

/* ========================================================================== */
/* Переходники для машины скриптов и таблица встроенных функций               */
/* ========================================================================== */
/* args[0] — вершина стека, т. е. последний довод; у функции с n доводами
   естественный довод i лежит в args[n - 1 - i]. */

#define ARG_PTR(type, value) ((type)(intptr_t)(value))

static int32_t Invoke_ContinueRules(const int32_t *args)       { (void)args; return Native_ContinueRules(); }
static int32_t Invoke_HasNoSprite(const int32_t *args)         { (void)args; return Native_HasNoSprite(); }
static int32_t Invoke_RestGraphicIs(const int32_t *args)       { return Native_RestGraphicIs(args[0]); }
static int32_t Invoke_IsHovered(const int32_t *args)           { (void)args; return Native_IsHovered(); }
static int32_t Invoke_IsPressed(const int32_t *args)           { (void)args; return Native_IsPressed(); }
static int32_t Invoke_MouseInputIs(const int32_t *args)        { return Native_MouseInputIs(args[0]); }
static int32_t Invoke_KeyInputIs(const int32_t *args)          { return Native_KeyInputIs(args[0]); }
static int32_t Invoke_IsDirectionKey(const int32_t *args)      { return Native_IsDirectionKey((uint8_t)args[0]); }
static int32_t Invoke_LayerIs(const int32_t *args)             { return Native_LayerIs(args[0]); }
static int32_t Invoke_RunPanel(const int32_t *args)            { return Native_RunPanel(args[0]); }
static int32_t Invoke_IsUnderTileEmpty(const int32_t *args)    { (void)args; return Native_IsUnderTileEmpty(); }
static int32_t Invoke_UnderTileIs(const int32_t *args)         { return Native_UnderTileIs(args[0]); }
static int32_t Invoke_UnderTileInSet(const int32_t *args)      { return Native_UnderTileInSet(args[0]); }
static int32_t Invoke_InputCharIn(const int32_t *args)         { return Native_InputCharIn(ARG_PTR(const uint8_t *, args[0])); }
static int32_t Invoke_RestartGame(const int32_t *args)         { Native_RestartGame(args[0]); }
static int32_t Invoke_GotoRoom(const int32_t *args)            { Native_GotoRoom(args[0]); }
static int32_t Invoke_PlayMusic(const int32_t *args)           { return Native_PlayMusic(args[0]); }
static int32_t Invoke_PlaySound(const int32_t *args)           { return Native_PlaySound(args[0]); }
static int32_t Invoke_PlayMovie(const int32_t *args)           { return Native_PlayMovie(args[0]); }
static int32_t Invoke_ShowGraphic(const int32_t *args)         { return Native_ShowGraphic(args[0]); }
static int32_t Invoke_ShowPanel(const int32_t *args)           { return Native_ShowPanel(args[0]); }
static int32_t Invoke_InputTextAtTile(const int32_t *args)     { return Native_InputTextAtTile(args[1], ARG_PTR(uint8_t *, args[0])); }
static int32_t Invoke_InputTextAtPanel(const int32_t *args)    { return Native_InputTextAtPanel(args[1], ARG_PTR(uint8_t *, args[0])); }
static int32_t Invoke_PrintAtTile(const int32_t *args)         { return Native_PrintAtTile(args[1], ARG_PTR(uint8_t *, args[0])); }
static int32_t Invoke_PrintAtPanel(const int32_t *args)        { return Native_PrintAtPanel(args[1], ARG_PTR(uint8_t *, args[0])); }
static int32_t Invoke_SetLayer(const int32_t *args)            { return Native_SetLayer((uint8_t)args[0]); }
static int32_t Invoke_RemoveText(const int32_t *args)          { (void)args; return Native_RemoveText(); }
static int32_t Invoke_DestroyUnderTile(const int32_t *args)    { (void)args; return Native_DestroyUnderTile(); }
static int32_t Invoke_SetUnderTile(const int32_t *args)        { return Native_SetUnderTile(args[0]); }
static int32_t Invoke_SetUnderTileFromSet(const int32_t *args) { return Native_SetUnderTileFromSet(args[0]); }
static int32_t Invoke_SetCursor(const int32_t *args)           { return Native_SetCursor(args[0]); }
static int32_t Invoke_ResetCursor(const int32_t *args)         { (void)args; return Native_ResetCursor(); }
static int32_t Invoke_LayerFromRow(const int32_t *args)        { (void)args; return Native_LayerFromRow(); }
static int32_t Invoke_GetLayer(const int32_t *args)            { return Native_GetLayer(ARG_PTR(uint8_t *, args[0])); }
static int32_t Invoke_SeekPointer(const int32_t *args)         { return Native_SeekPointer((GM_SEEK_COMMAND)args[0]); }
static int32_t Invoke_SeekClass(const int32_t *args)           { return Native_SeekClass((uint8_t)args[1], (GM_SEEK_COMMAND)args[0]); }
static int32_t Invoke_SeekClassSet(const int32_t *args)        { return Native_SeekClassSet((uint8_t)args[1], (GM_SEEK_COMMAND)args[0]); }
static int32_t Invoke_PathDir4Is(const int32_t *args)          { return Native_PathDir4Is(args[0]); }
static int32_t Invoke_PathDir8Is(const int32_t *args)          { return Native_PathDir8Is(args[0]); }
static int32_t Invoke_PathDir4IsRotated(const int32_t *args)   { return Native_PathDir4IsRotated(args[0]); }
static int32_t Invoke_PathDir8IsRotated(const int32_t *args)   { return Native_PathDir8IsRotated(args[0]); }
static int32_t Invoke_PathAction(const int32_t *args)          { return Native_PathAction((GM_PATH_ACTION)args[0]); }
static int32_t Invoke_PlaceAtGoal(const int32_t *args)         { return Native_PlaceAtGoal((uint8_t)args[0]); }
static int32_t Invoke_PlaceAtGoalFromSet(const int32_t *args)  { return Native_PlaceAtGoalFromSet((uint8_t)args[0]); }
static int32_t Invoke_ObjectFlagsAny(const int32_t *args)      { return Native_ObjectFlagsAny((uint8_t)args[0]); }
static int32_t Invoke_GetOption(const int32_t *args)           { return Native_GetOption((GM_OPTION_QUERY)args[0]); }
static int32_t Invoke_SetOption(const int32_t *args)           { return Native_SetOption((GM_OPTION_SET)args[0]); }
static int32_t Invoke_SaveGameOp(const int32_t *args)          { return Native_SaveGameOp(args[1], (GM_SAVE_OPERATION)args[0]); }
static int32_t Invoke_SetClassFilter(const int32_t *args)      { return Native_SetClassFilter(args[0]); }
static int32_t Invoke_ClearClassFilter(const int32_t *args)    { (void)args; return Native_ClearClassFilter(); }
static int32_t Invoke_ShowHelp(const int32_t *args)            { return Native_ShowHelp(ARG_PTR(const char *, args[0])); }
static int32_t Invoke_SetDirKey(const int32_t *args)           { return Native_SetDirKey(ARG_PTR(const uint8_t *, args[1]), args[0]); }
static int32_t Invoke_Random(const int32_t *args)              { return (int32_t)Native_Random(args[0]); }
static int32_t Invoke_PlayAvi(const int32_t *args)             { return Native_PlayAvi(ARG_PTR(const char *, args[0])); }
static int32_t Invoke_RunProgram(const int32_t *args)          { return Native_RunProgram(ARG_PTR(char *, args[0])); }

static int32_t Invoke_StrCopy(const int32_t *args)
{
    return (int32_t)(intptr_t)Native_StrCopy(ARG_PTR(char *, args[1]), ARG_PTR(char *, args[0]));
}

/* Native_RemoveSprite ничего не возвращает: в оригинале в аккумулятор машины попадает
   случайное содержимое EAX (адрес объекта или остаток от освобождения записи пула).
   Скрипты его не используют; переходник отдаёт 1, как прочие функции-действия. */
static int32_t Invoke_RemoveSprite(const int32_t *args)
{
    (void)args;
    Native_RemoveSprite();
    return 1;
}

const GM_NATIVE g_NativeFunctions[57] =
{
    /*  0 */ { 0, Invoke_ContinueRules },
    /*  1 */ { 0, Invoke_HasNoSprite },
    /*  2 */ { 1, Invoke_RestGraphicIs },
    /*  3 */ { 0, Invoke_IsHovered },
    /*  4 */ { 0, Invoke_IsPressed },
    /*  5 */ { 1, Invoke_MouseInputIs },
    /*  6 */ { 1, Invoke_KeyInputIs },
    /*  7 */ { 1, Invoke_IsDirectionKey },
    /*  8 */ { 1, Invoke_LayerIs },
    /*  9 */ { 1, Invoke_RunPanel },
    /* 10 */ { 0, Invoke_IsUnderTileEmpty },
    /* 11 */ { 1, Invoke_UnderTileIs },
    /* 12 */ { 1, Invoke_UnderTileInSet },
    /* 13 */ { 1, Invoke_InputCharIn },
    /* 14 */ { 1, Invoke_RestartGame },
    /* 15 */ { 1, Invoke_GotoRoom },
    /* 16 */ { 1, Invoke_PlayMusic },
    /* 17 */ { 1, Invoke_PlaySound },
    /* 18 */ { 1, Invoke_PlayMovie },
    /* 19 */ { 1, Invoke_ShowGraphic },
    /* 20 */ { 1, Invoke_ShowPanel },
    /* 21 */ { 2, Invoke_InputTextAtTile },
    /* 22 */ { 2, Invoke_InputTextAtPanel },
    /* 23 */ { 2, Invoke_PrintAtTile },
    /* 24 */ { 2, Invoke_PrintAtPanel },
    /* 25 */ { 1, Invoke_SetLayer },
    /* 26 */ { 0, Invoke_RemoveText },
    /* 27 */ { 0, Invoke_DestroyUnderTile },
    /* 28 */ { 1, Invoke_SetUnderTile },
    /* 29 */ { 1, Invoke_SetUnderTileFromSet },
    /* 30 */ { 0, Invoke_RemoveSprite },
    /* 31 */ { 1, Invoke_SetCursor },
    /* 32 */ { 0, Invoke_ResetCursor },
    /* 33 */ { 0, Invoke_LayerFromRow },
    /* 34 */ { 1, Invoke_GetLayer },
    /* 35 */ { 1, Invoke_SeekPointer },
    /* 36 */ { 2, Invoke_SeekClass },
    /* 37 */ { 2, Invoke_SeekClassSet },
    /* 38 */ { 1, Invoke_PathDir4Is },
    /* 39 */ { 1, Invoke_PathDir8Is },
    /* 40 */ { 1, Invoke_PathDir4IsRotated },
    /* 41 */ { 1, Invoke_PathDir8IsRotated },
    /* 42 */ { 1, Invoke_PathAction },
    /* 43 */ { 1, Invoke_PlaceAtGoal },
    /* 44 */ { 1, Invoke_PlaceAtGoalFromSet },
    /* 45 */ { 1, Invoke_ObjectFlagsAny },
    /* 46 */ { 2, Invoke_StrCopy },
    /* 47 */ { 1, Invoke_GetOption },
    /* 48 */ { 1, Invoke_SetOption },
    /* 49 */ { 2, Invoke_SaveGameOp },
    /* 50 */ { 1, Invoke_SetClassFilter },
    /* 51 */ { 0, Invoke_ClearClassFilter },
    /* 52 */ { 1, Invoke_ShowHelp },
    /* 53 */ { 2, Invoke_SetDirKey },
    /* 54 */ { 1, Invoke_Random },
    /* 55 */ { 1, Invoke_PlayAvi },
    /* 56 */ { 1, Invoke_RunProgram },
};
