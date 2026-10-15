/*
 * Загрузка игры: чтение файла данных (игра дописана в хвост исполняемого файла),
 * теговый поток ресурсов, распаковка LZSS и применение прочитанных записей.
 *
 * Поток ресурса — последовательность байтов-тегов (GM_LOAD_TAG):
 *   1/2/3 — числовые доводы следующей записи (index1, index2, dataOffset);
 *   4     — тело записи; 5 — пропускаемый блок (фильм); 0xFF — применить запись
 *           к уже прочитанным данным; 0 — конец ресурса;
 *   иначе — заголовок записи: тип = тег & 0x7F, номер слота — числом, если тег < 0x80.
 */
#include "gm.h"

#include <stdio.h>
#include <string.h>

/* Магия хвоста файла игры и сохранений: байты «=VS=». */
#define GM_FILE_MAGIC 0x3D53563Du

/* Пометка изображения, оставленного на диске: 'Disk'. */
#define GM_DISK_IMAGE_MAGIC 0x4469736Bu

/* Длина записи индекса ресурсов: смещение (4 байта) и контрольный номер (1 байт). */
#define GM_INDEX_ENTRY_SIZE 5

/* Длина хвоста файла игры (GM_FILE_TRAILER). */
#define GM_TRAILER_SIZE 12

/* Последний допустимый номер ресурса и наибольшая база данных ресурсов. */
#define GM_MAX_RESOURCE_ID 0xFE
#define GM_MAX_RESOURCE_DATA_BASE 80000

/*
 * Диапазоны чисел переменной длины (GmRead_Number) для 1–3 дополнительных байтов:
 * [0..2] — отрицательные (значение = max − число), [3..5] — положительные
 * (значение = число + min). Запись [6] — нули: в оригинале за таблицей лежит
 * g_RecordSizesZero, и 4-байтовая положительная форма берёт базу 0 оттуда.
 */
static const GM_VLQ_RANGE s_VlqRanges[7] =
{
    { -0x400,      -0x1 },
    { -0x40400,    -0x401 },
    { -0x4040400,  -0x40401 },
    { 0x80,        0x47F },
    { 0x480,       0x4047F },
    { 0x40480,     0x404047F },
    { 0,           0 },
};

/* Выделить рабочий буфер комнаты: max(4 · клеток + 20, размер фона) — в него читаются записи комнат. */
void *GmLoad_AllocRoomScratch(void)
{
    uint32_t size = 4 * (uint32_t)g_MapTileCount + 20;

    if (size <= (uint32_t)g_World.backgroundSize)
        size = (uint32_t)g_World.backgroundSize;
    g_LoadScratch = GmHeap_Alloc(size);
    return g_LoadScratch;
}

/* Освободить рабочий буфер комнаты после загрузки эпизода. */
void GmLoad_FreeRoomScratch(void)
{
    GmHeap_Free(g_LoadScratch);
}

/* Задать буфер чтения файла данных (NULL — без буфера). Возвращает его размер. */
uint32_t GmRead_SetBuffer(void *buffer)
{
    g_ReadBufFill = 0;
    g_ReadBufLeft = 0;
    g_ReadBuf = buffer;
    if (buffer == NULL)
        return 0;
    g_ReadBufFill = g_GameHeader.fileBufferSize;
    return g_GameHeader.fileBufferSize;
}

/* Логическая позиция чтения: позиция файла минус непрочитанный остаток буфера. */
uint32_t GmRead_Tell(void)
{
    return (uint32_t)Host_DataTell() - g_ReadBufLeft;
}

/* Перейти в файле данных; для SEEK_CUR учитывается непрочитанный остаток буфера. Буфер сбрасывается. */
bool GmRead_Seek(int offset, int origin)
{
    if (origin == SEEK_CUR)
        offset -= (int)g_ReadBufLeft;
    g_ReadBufLeft = 0;
    return Host_DataSeek(offset, origin);
}

/* Прочитать size байт через буфер; запросы не меньше буфера читаются из файла напрямую. */
bool GmRead_Bytes(void *dst, uint32_t size)
{
    uint8_t *out = dst;

    do
    {
        uint32_t chunk;

        if (g_ReadBufLeft == 0)
        {
            if (size >= g_ReadBufFill || g_ReadBuf == NULL)
                return Host_DataRead(out, size);
            g_ReadBufLeft = (uint32_t)Host_DataReadSome(g_ReadBuf, g_GameHeader.fileBufferSize);
            g_ReadBufFill = g_ReadBufLeft;
        }

        chunk = size < g_ReadBufLeft ? size : g_ReadBufLeft;
        memcpy(out, &g_ReadBuf[g_ReadBufFill - g_ReadBufLeft], chunk);
        out += chunk;
        g_ReadBufLeft -= chunk;
        size -= chunk;
    } while (size != 0);

    return true;
}

/* Ошибка «Invalid game data file.»: закрыть файл данных и запомнить сообщение. Возвращает false. */
bool GmLoad_Fail(void)
{
    Host_CloseDataFile();
    return Host_SetError(s_InvalidGameData);
}

/* Прочитать следующий байт-тег потока в g_LoadTag. */
bool GmRead_Tag(void)
{
    return GmRead_Bytes(&g_LoadTag, 1);
}

/*
 * Открыть ресурс по номеру: прочитать его запись в индексе в конце файла, сверить
 * контрольный номер, перейти к началу ресурса и прочитать первый тег.
 * Ресурс 1 — общий, ресурс n + 2 — эпизод n.
 */
bool GmLoad_OpenResource(uint32_t resourceId)
{
    struct
    {
        int32_t offset;
        uint8_t id;
    } entry;

    if (resourceId <= g_LastResourceId
        && GmRead_Seek(-(g_ResourceIndexSize + GM_INDEX_ENTRY_SIZE * (int)(g_LastResourceId - resourceId)
                         + GM_INDEX_ENTRY_SIZE), SEEK_END)
        && GmRead_Bytes(&entry, GM_INDEX_ENTRY_SIZE)
        && entry.id == (uint8_t)resourceId
        && GmRead_Seek(g_ResourceDataBase + entry.offset, SEEK_SET)
        && GmRead_Tag())
    {
        return true;
    }
    return GmLoad_Fail();
}

/*
 * Прочитать число переменной длины и следующий тег. Тег < 0x80 — само число.
 * Иначе при разряде GMNUM_RANGED: n = ((тег >> 2) & 3) + 1 байтов числа плюс два старших
 * разряда (тег & 3) над ними, затем перенос в диапазон s_VlqRanges (вниз от max при
 * GMNUM_NEGATIVE, вверх от min иначе); без GMNUM_RANGED — тег & 0x1F сырых байтов.
 */
bool GmRead_Number(int *value)
{
    uint32_t byteCount;

    if (!GmRead_Tag())
        return GmLoad_Fail();

    if (g_LoadTag >= 0)
    {
        *value = (uint8_t)g_LoadTag;
        return GmRead_Tag();
    }

    if (g_LoadTag & GMNUM_RANGED)
        byteCount = (((uint8_t)g_LoadTag >> 2) & 3) + 1;
    else
        byteCount = g_LoadTag & GMNUM_RAW_BYTES_MASK;   /* Особенность оригинала: до 31 байта в int */

    *value = 0;
    if (!GmRead_Bytes(value, byteCount))
        return GmLoad_Fail();

    if (g_LoadTag & GMNUM_RANGED)
    {
        uint32_t highBits = g_LoadTag & GMNUM_HIGH_BITS_MASK;

        /* Особенность оригинала: сдвиг выполнялся командой shl, у которой счётчик берётся
           по модулю 32, — при n = 4 старшие разряды прибавляются без сдвига. */
        *value += (int)(highBits << ((8 * byteCount) & 31));
        if (g_LoadTag & GMNUM_NEGATIVE)
            *value = s_VlqRanges[byteCount - 1].max - *value;
        else
            *value += s_VlqRanges[byteCount + 2].min;
    }
    return GmRead_Tag();
}

/*
 * Пропустить блок (тег вида 0xEC | n): прочитать его длину (n + 1 байт), запомнить начало
 * блока в blockStart, перейти за его конец и прочитать следующий тег. Так хранятся фильмы.
 */
bool GmRead_SkipBlock(uint32_t *blockStart)
{
    uint32_t blockSize = 0;

    if (!GmRead_Tag() || ((uint8_t)g_LoadTag & GMBT_MARK) != GMBT_MARK)
        return GmLoad_Fail();
    if (!GmRead_Bytes(&blockSize, (g_LoadTag & GMBT_SIZE_BYTES_MASK) + 1))
        return GmLoad_Fail();

    *blockStart = GmRead_Tell();
    GmRead_Seek((int)blockSize, SEEK_CUR);
    return GmRead_Tag();
}

/*
 * Прочитать запись в dst. Тег записи ≥ 0x80; при GMRT_SHORT длина (тег & 0x1F) — в самом
 * теге; иначе далее идут размеры по (тег & 3) + 1 байт: хранимый и, если тег & 0xC, —
 * распакованный, тогда данные упакованы LZSS. Запоминает g_RecordSize и
 * g_RecordPackedSize (0 — не упакована), читает следующий тег.
 */
bool GmRead_Record(void *dst)
{
    GM_RECORD_SIZES sizes;
    uint32_t sizeBytes;

    g_RecordPackedSize = 0;
    g_RecordSize = 0;
    if (!GmRead_Tag() || g_LoadTag >= 0)
        return GmLoad_Fail();

    if (g_LoadTag & GMRT_SHORT)
    {
        g_RecordSize = g_LoadTag & GMRT_SHORT_LENGTH_MASK;
        if (g_RecordSize != 0 && !GmRead_Bytes(dst, g_RecordSize))
            return GmLoad_Fail();
        return GmRead_Tag();
    }

    sizes = g_RecordSizesZero;
    sizeBytes = (g_LoadTag & GMRT_SIZE_BYTES_MASK) + 1;
    if (!GmRead_Bytes(&sizes.storedSize, sizeBytes))
        return GmLoad_Fail();

    if (g_LoadTag & GMRT_PACKED_MASK)
    {
        uint8_t *packed = GmTempBuffer(sizes.storedSize);

        if (!GmRead_Bytes(&sizes.unpackedSize, sizeBytes) || !GmRead_Bytes(packed, sizes.storedSize))
            return GmLoad_Fail();
        g_RecordPackedSize = sizes.storedSize;
        GmLzss_Unpack(packed, dst, (int)sizes.storedSize);
        g_RecordSize = sizes.unpackedSize;
    }
    else
    {
        g_RecordSize = sizes.storedSize;
        if (!GmRead_Bytes(dst, sizes.storedSize))
            return GmLoad_Fail();
    }
    return GmRead_Tag();
}

/*
 * Открыть файл данных path: проверить хвост {размер индекса, версия, магия} и прочитать
 * заголовок индекса перед ним (последний номер ресурса и база данных ресурсов).
 */
bool GmLoad_OpenGameFile(char *path)
{
    GM_FILE_TRAILER trailer;
    GM_INDEX_HEADER index;

    g_LoadingSave = false;
    GmRead_SetBuffer(NULL);

    if (Host_OpenDataFile(path)
        && GmRead_Seek(-GM_TRAILER_SIZE, SEEK_END)
        && Host_DataRead(GmMemZero(&trailer, sizeof(trailer)), sizeof(trailer))
        && trailer.formatVersion == GM_TRAILER_VERSION_CURRENT
        && trailer.magic == GM_FILE_MAGIC)
    {
        g_ResourceIndexSize = (int)trailer.indexSize + GM_TRAILER_SIZE;
        /* Особенность оригинала: длина заголовка индекса из файла не сверяется с sizeof(index). */
        if (GmRead_Seek(-g_ResourceIndexSize, SEEK_END)
            && Host_DataRead(GmMemZero(&index, sizeof(index)), trailer.indexSize))
        {
            g_ResourceDataBase = (int)index.dataBase;
            if (index.dataBase <= GM_MAX_RESOURCE_DATA_BASE)
            {
                g_LastResourceId = index.lastResourceId;
                if (index.lastResourceId <= GM_MAX_RESOURCE_ID)
                    return true;
            }
        }
    }
    return GmLoad_Fail();
}

/* Скопировать текущую запись (g_LoadPtr, g_RecordSize байт) в dst — начала классов, графики и звуков. */
void GmLoad_CopyRecord(void *dst)
{
    memcpy(dst, g_LoadPtr, g_RecordSize);
}

/*
 * Запись заголовка игры → g_GameHeader: проверка версии, размеры арены и кадрового
 * буфера, окно/вывод, куча, таблица смещений фильмов и буфер чтения файла.
 */
bool GmLoad_GameHeader(void)
{
    uint8_t *movieTable;

    GmRead_Record(&g_GameHeader);
    if (g_GameHeader.version != GM_FILE_VERSION_CURRENT)
        return GmLoad_Fail();

    /* Обрыв строки на последнем байте записи (запись может быть короче структуры). */
    ((uint8_t *)&g_GameHeader)[g_RecordSize - 1] = 0;

    g_Arena.freeBytes = g_GameHeader.memoryKB << 10;
    g_Arena.frameBufferSize = g_GameHeader.screenWidth * g_GameHeader.screenHeight;
    if (!Host_InitVideo(&g_Arena, &g_GameHeader, &g_Settings) || !GmFrame_Init(&g_Arena, &g_GameHeader))
        return false;

    GmHeap_ClearState();
    g_MovieOffsets = GmHeap_Init();
    movieTable = GmHeap_Reserve(4 * (uint32_t)g_GameHeader.movieCount);
    g_LoadPtr = movieTable;
    g_RecordData = movieTable;
    if (g_GameHeader.fileBufferSize != 0)
    {
        GmRead_SetBuffer(movieTable);
        g_LoadPtr = GmHeap_Reserve(g_GameHeader.fileBufferSize);
        g_RecordData = g_LoadPtr;
    }
    return true;
}

/*
 * Запись заголовка мира выбранного эпизода → g_World: число комнат, сдвиг строки карты
 * и подряд в куче фон, карта тайлов, комнаты, карта проходимости, классы, наборы классов,
 * графика, звуки, музыка и панели (всё после фона обнуляется); за ними начинаются
 * глобальные переменные скриптов.
 */
void GmLoad_WorldHeader(void)
{
    int shiftIndex;
    uint8_t *clearStart;

    GmRead_Record(&g_World);
    g_RoomCount = g_World.roomCountA * g_World.roomCountB;
    g_MapTileCount = g_World.mapWidth * g_World.mapHeight;

    g_Background = (GM_BACKGROUND *)g_LoadPtr;
    g_LoadPtr = GmHeap_Reserve((uint32_t)g_World.backgroundSize);

    /* Строка карты — наименьшая степень двойки не меньше ширины. */
    g_TileMap = (GM_TILE *)g_LoadPtr;
    for (shiftIndex = 0; g_MapRowShifts[shiftIndex].maxWidth < g_World.mapWidth; shiftIndex++)
        ;
    g_TileMapShift = g_MapRowShifts[shiftIndex].shift;
    g_LoadPtr = GmHeap_Reserve((uint32_t)g_World.mapHeight << (g_TileMapShift + 1));

    clearStart = g_LoadPtr;
    g_Rooms = (GM_ROOM *)g_LoadPtr;
    g_LoadPtr = GmHeap_Reserve(sizeof(GM_ROOM) * (uint32_t)g_RoomCount);

    g_WalkMap = (char *)g_LoadPtr;
    g_MapMaxX = g_World.mapWidth - 1;
    g_WalkMapRowUp = -g_World.mapWidth;
    g_MapMaxY = g_World.mapHeight - 1;
    g_LoadPtr = GmHeap_Reserve((uint32_t)g_MapTileCount);

    g_Classes = (GM_CLASS *)g_LoadPtr;
    g_LoadPtr = GmHeap_Reserve(sizeof(GM_CLASS) * (uint32_t)g_World.classCount);
    g_ClassSets = (GM_CLASS_SET *)g_LoadPtr;
    g_LoadPtr = GmHeap_Reserve(sizeof(GM_CLASS_SET) * (uint32_t)g_World.classSetCount);
    g_Graphics = (GM_GRAPHIC *)g_LoadPtr;
    g_LoadPtr = GmHeap_Reserve(sizeof(GM_GRAPHIC) * (uint32_t)g_World.graphicCount);
    g_Sounds = (GM_SOUND *)g_LoadPtr;
    g_LoadPtr = GmHeap_Reserve(sizeof(GM_SOUND) * (uint32_t)g_World.soundCount);
    g_MusicTracks = (const GM_MUSIC_DATA **)g_LoadPtr;
    g_LoadPtr = GmHeap_Reserve(sizeof(GM_MUSIC_DATA *) * (uint32_t)g_World.musicCount);
    g_Panels = (GM_PANEL *)g_LoadPtr;
    g_LoadPtr = GmHeap_Reserve(sizeof(GM_PANEL) * (uint32_t)g_World.panelCount);

    GmMemZero(clearStart, (uint32_t)(g_LoadPtr - clearStart));
    g_RecordData = g_LoadPtr;
    g_GlobalVars = g_LoadPtr;
}

/* Найти слово правила, куда кладётся его скрипт: за заголовком, предусловием и группами образца. */
static GM_RULE_ITEM *GmRule_ScriptSlot(GM_RULE *rule)
{
    int word = (rule->header.ruleFlags & GMRULE_PRECONDITION) ? 2 : 1;

    if (rule->header.ruleFlags & GMRULE_PATTERN)
    {
        GM_RULE_GROUP group;

        do
        {
            group = rule->words[word].group;
            word += group.count + 1;
        } while ((group.raw & GMGROUP_IS_LAST) == 0);
    }
    return &rule->words[word];
}

/* Список кадров графики для направления index1 (у шрифтов и графики без направлений — общий). */
static GM_FRAME **GmGraphic_FrameListSlot(GM_GRAPHIC *graphic, int direction)
{
    if (graphic->isFont || graphic->dirMode <= 1)
        return &graphic->frames.list;
    return &graphic->frames.byDirection[direction];
}

/*
 * Применить прочитанную запись типа type к слоту slot (index1/index2 — подындексы,
 * dataOffset — сдвиг внутри записи, data — данные записи): системные переменные,
 * комнаты и постройка их начального состояния, классы, их скрипты и правила, наборы
 * классов, графика и кадры, звуки, музыка, панели, описатели сохраняемых переменных.
 */
bool GmLoad_ApplyRecord(GM_RECORD_TYPE type, int slot, int index1, int index2, int dataOffset, void *data)
{
    uint8_t *bytes = data;

    switch (type)
    {
    case GMREC_SYSVARS:
        g_SysVars = data;
        g_SysVars->frameRate = g_GameHeader.frameRate;
        Host_SetFrameRate(g_GameHeader.frameRate);
        break;

    case GMREC_WORLD_HEADER:
    case GMREC_ZEROED_BLOCK:
        /* Память под обнулённый блок отводится, адрес нигде не запоминается. */
        break;

    case GMREC_ROOM:
    {
        GM_ROOM *room = &g_Rooms[slot];

        room->loadBuffer = g_Background;
        room->fileOffset = g_RoomFileOffset;
        room->savedState = NULL;
        room->savedObjectCount = 0;
        /* Разряд 31 первого двойного слова фона помечает стартовую комнату. */
        if (g_StartRoom == -1 && (((const GM_BACKGROUND *)data)->reserved0 & 0x80000000u) != 0)
            g_StartRoom = slot;
        break;
    }

    case GMREC_ROOM_BUILD:
        if (!g_LoadingSave)
        {
            int tileCount = g_World.mapHeight << g_TileMapShift;
            int i;

            for (i = 0; i < tileCount; i++)
                g_TileMap[i].raw = GMTILE_EMPTY_RAW;
            g_BuildingRoom = true;
            GmRule_Run(data, true);
            g_BuildingRoom = false;
            GmRoom_SaveState(slot);
        }
        break;

    case GMREC_CLASS:
        GmLoad_CopyRecord(&g_Classes[slot]);
        break;
    case GMREC_CLASS_CREATE_SCRIPT:
        g_Classes[slot].createScript = bytes + dataOffset;
        break;
    case GMREC_CLASS_DESTROY_SCRIPT:
        g_Classes[slot].destroyScript = bytes + dataOffset;
        break;
    case GMREC_CLASS_RULES:
        g_Classes[slot].rules = data;
        break;
    case GMREC_RULE:
        g_Classes[slot].rules[index1] = data;
        break;
    case GMREC_RULE_PRECONDITION:
        g_Classes[slot].rules[index1]->items[0].script = bytes + dataOffset;
        break;
    case GMREC_RULE_SCRIPT:
        GmRule_ScriptSlot(g_Classes[slot].rules[index1])->script = bytes + dataOffset;
        break;

    case GMREC_CLASS_SET_MEMBERS:
        g_ClassSets[slot].memberBits = data;
        break;
    case GMREC_CLASS_SET_CHOICES:
        g_ClassSets[slot].randomChoices = data;
        break;
    case GMREC_CLASS_SET_ORIENTATIONS:
        g_ClassSets[slot].orientations = data;
        break;

    case GMREC_GRAPHIC:
        GmLoad_CopyRecord(&g_Graphics[slot]);
        g_GraphicOnDisk = ((const GM_GRAPHIC *)g_LoadPtr)->options & GMGR_ON_DISK;
        break;
    case GMREC_GRAPHIC_FRAMES:
        g_Graphics[slot].frames.list = data;
        break;
    case GMREC_GRAPHIC_DIR_FRAMES:
        *GmGraphic_FrameListSlot(&g_Graphics[slot], index1) = data;
        break;
    case GMREC_FRAME_IMAGE:
        (*GmGraphic_FrameListSlot(&g_Graphics[slot], index1))[index2].image = data;
        break;

    case GMREC_SOUND:
        GmLoad_CopyRecord(&g_Sounds[slot]);
        break;
    case GMREC_SOUND_DATA:
        g_Sounds[slot].data = data;
        break;
    case GMREC_MUSIC:
        g_MusicTracks[slot] = data;
        break;

    case GMREC_PANEL_RULE:
        g_Panels[slot].rule = data;
        break;
    case GMREC_PANEL_ITEMS:
        g_Panels[slot].items = data;
        break;

    case GMREC_VARS_KEEP_ALWAYS:
        g_VarsKeepAlways = data;
        break;
    case GMREC_VARS_KEEP_ON_RESTART:
        g_VarsKeepOnRestart = data;
        break;
    case GMREC_VARS_SAVEGAME_ONLY:
        g_VarsSaveGameOnly = data;
        break;

    default:
        break;
    }
    return true;
}

/*
 * Изображение графики «на диске» не остаётся в памяти: вместо пикселей в запись
 * кладётся заглушка {'Disk', смещение данных в файле, упакованный размер}, а наибольший
 * размер таких изображений запоминается под общий буфер g_DiskImageBuffer.
 */
static void GmLoad_LeaveImageOnDisk(GM_IMAGE *image)
{
    /* Хранимые данные — пиксели без 4-байтового заголовка; упакованные — вместе с ним. */
    uint32_t bytesInFile = g_RecordSize - 4;
    uint32_t bufferNeeded = g_RecordSize - 4;

    if (g_RecordPackedSize != 0)
    {
        bytesInFile = g_RecordPackedSize;
        bufferNeeded = g_RecordSize;
    }
    if (bufferNeeded > g_DiskImageMaxSize)
        g_DiskImageMaxSize = bufferNeeded;

    image->disk.magic = GM_DISK_IMAGE_MAGIC;
    /* Позиция чтения уже за следующим тегом — отсюда лишний байт. */
    image->disk.fileOffset = GmRead_Tell() - bytesInFile - 1;
    image->disk.packedSize = g_RecordPackedSize;
    g_RecordSize = sizeof(GM_IMAGE);
}

/*
 * Тело записи (тег GMTAG_BODY): прочитать запись типа type в g_LoadPtr и применить её.
 * Записи постоянных данных остаются в куче (вершина сдвигается за запись); заголовки,
 * начала классов/графики/звуков и записи комнат читаются во временное место.
 */
static bool GmLoad_RecordBody(GM_RECORD_TYPE type, int slot, int index1, int index2, int dataOffset,
                              int startEntry)
{
    switch (type)
    {
    case GMREC_SETTINGS:
    {
        GM_SETTINGS discarded;

        /* Настройки по умолчанию нужны только при первом запуске. */
        GmRead_Record(g_FirstRun && !g_LoadingSave ? &g_Settings : &discarded);
        return true;
    }

    case GMREC_GAME_HEADER:
        return GmLoad_GameHeader();

    case GMREC_WORLD_HEADER:
        if (slot == startEntry)
        {
            GmLoad_WorldHeader();
            return true;
        }
        break;   /* заголовки других эпизодов читаются вхолостую */

    case GMREC_ROOM:
        if (g_EngineInitPending)
        {
            /* Первая комната: все постоянные данные эпизода уже в куче. */
            g_EngineInitPending = false;
            if (g_DiskImageMaxSize != 0)
            {
                g_DiskImageBuffer = g_LoadPtr;
                GmHeap_Reserve(g_DiskImageMaxSize);
            }
            GmTick_InitState();
            GmPool_Create();
            g_LoadPtr = GmLoad_AllocRoomScratch();
        }
        g_RoomFileOffset = GmRead_Tell();
        g_RoomsLoaded++;
        break;

    default:
        break;
    }

    g_RecordData = g_LoadPtr;
    GmRead_Record(g_LoadPtr);
    if (!GmLoad_ApplyRecord(type, slot, index1, index2, dataOffset, g_LoadPtr))
        return false;

    switch (type)
    {
    case GMREC_FRAME_IMAGE:
        if (g_GraphicOnDisk)
            GmLoad_LeaveImageOnDisk((GM_IMAGE *)g_LoadPtr);
        break;

    case GMREC_WORLD_HEADER:
    case GMREC_CLASS:
    case GMREC_GRAPHIC:
    case GMREC_SOUND:
    case GMREC_ROOM:
    case GMREC_ROOM_BUILD:
        return true;

    default:
        break;
    }
    g_LoadPtr = GmHeap_Reserve(g_RecordSize);
    return true;
}

/*
 * Загрузить эпизод startEntry: сначала общий ресурс 1 (настройки по умолчанию, заголовок
 * игры, заголовки миров, системные переменные, описатели), затем ресурс эпизода
 * startEntry + 2. После загрузки — автосохранение, курсор 0 и вход в стартовую комнату
 * (если не идёт загрузка сохранения).
 */
bool GmLoad_Run(int startEntry)
{
    uint32_t resourceId = 1;
    /* Особенность оригинала: доводы и тип записи не инициализировались до первого
       заголовка; поток всегда начинается с заголовка, так что нули ничего не меняют. */
    GM_RECORD_TYPE recordType = 0;
    int slot = 0;
    int index1 = 0;
    int index2 = 0;
    int dataOffset = 0;
    uint32_t blockStart;

    GmRead_SetBuffer(NULL);
    if ((!g_FirstRun && !GmAutosave_Write(g_AutosaveSlot)) || !GmLoad_OpenResource(resourceId))
        return false;

    g_EngineInitPending = true;
    g_CurrentRoom = -1;
    g_StartRoom = -1;
    g_RoomsLoaded = 0;
    g_StartEntry = startEntry;
    g_VarsSaveGameOnly = NULL;
    g_VarsKeepOnRestart = NULL;
    g_VarsKeepAlways = NULL;
    g_DiskImageMaxSize = 0;
    g_DiskImageCachedOffset = 0;
    g_DiskImageBuffer = NULL;
    GmMusic_Stop();
    GmAudio_StopSounds();

    for (;;)
    {
        /* Ошибки чтения доводов и блоков здесь не проверяются (как в оригинале). */
        while (g_LoadTag != GMTAG_END)
        {
            switch ((uint8_t)g_LoadTag)
            {
            case GMTAG_ARG1:
                GmRead_Number(&index1);
                break;
            case GMTAG_ARG2:
                GmRead_Number(&index2);
                break;
            case GMTAG_ARG3:
                GmRead_Number(&dataOffset);
                break;

            case GMTAG_BLOCK:
                GmRead_SkipBlock(&blockStart);
                if (recordType == GMREC_MOVIE)
                    g_MovieOffsets[slot] = blockStart;
                break;

            case GMTAG_APPLY:
                if (!GmLoad_ApplyRecord(recordType, slot, index1, index2, dataOffset, g_RecordData)
                    || !GmRead_Tag())
                {
                    return false;
                }
                break;

            case GMTAG_BODY:
                if (!GmLoad_RecordBody(recordType, slot, index1, index2, dataOffset, startEntry))
                    return false;
                break;

            default:
                /* Заголовок записи. */
                slot = 0;
                index1 = 0;
                index2 = 0;
                dataOffset = 0;
                recordType = (GM_RECORD_TYPE)(g_LoadTag & GMHT_TYPE_MASK);
                if (g_LoadTag & GMHT_NO_SLOT)
                    GmRead_Tag();
                else
                    GmRead_Number(&slot);
                break;
            }
        }

        if (resourceId != 1)
            break;
        resourceId = (uint32_t)startEntry + 2;
        if (!GmLoad_OpenResource(resourceId))
            return false;
    }

    GmLoad_FreeRoomScratch();
    if (g_LoadingSave)
        return true;
    if (!GmCursor_SetGraphic(0, false) || !GmAutosave_Load(g_AutosaveSlot))
        return false;
    return GmRoom_Enter(g_StartRoom != -1 ? g_StartRoom : 0, false);
}

/*
 * Пиксели изображения. У обычного изображения они лежат сразу за заголовком; изображение
 * «на диске» читается из файла (с кэшем по смещению) в общий буфер g_DiskImageBuffer и при
 * ненулевом упакованном размере распаковывается — тогда пиксели идут за 4-байтовым
 * заголовком.
 */
uint8_t *GmImage_GetPixels(GM_IMAGE *image)
{
    uint32_t packedSize;
    uint32_t fileOffset;

    if (image->disk.magic != GM_DISK_IMAGE_MAGIC)
        return image->pixels;

    packedSize = image->disk.packedSize;
    fileOffset = image->disk.fileOffset;
    if (fileOffset != g_DiskImageCachedOffset)
    {
        g_DiskImageCachedOffset = fileOffset;
        GmRead_Seek((int)fileOffset, SEEK_SET);
        if (packedSize != 0)
        {
            uint8_t *packed = GmTempBuffer(packedSize);

            Host_DataRead(packed, packedSize);
            GmLzss_Unpack(packed, g_DiskImageBuffer, (int)packedSize);
        }
        else
        {
            Host_DataRead(g_DiskImageBuffer, (uint32_t)image->width * image->height);
        }
    }
    return g_DiskImageBuffer + (packedSize != 0 ? 4 : 0);
}

/* Закрыть файл данных (при завершении игры). */
void GmLoad_CloseGameFile(void)
{
    Host_CloseDataFile();
}

/*
 * Распаковка LZSS. Байт флагов на 8 элементов (младший разряд первым): 1 — литерал,
 * 0 — ссылка из двух байтов: смещение назад = младший байт | (старшая тетрада второго
 * байта << 8), длина = младшая тетрада второго байта + 3. packedSize — длина
 * упакованного потока.
 */
void GmLzss_Unpack(const uint8_t *src, uint8_t *dst, int packedSize)
{
    for (;;)
    {
        uint8_t flags = *src++;
        int itemsLeft;

        if (--packedSize < 0)
            return;

        for (itemsLeft = 8; itemsLeft != 0; itemsLeft--)
        {
            bool isLiteral = (flags & 1) != 0;

            flags >>= 1;
            if (isLiteral)
            {
                /* Особенность оригинала: у литерала конец потока не проверяется —
                   группа из 8 элементов дочитывается до конца. */
                packedSize--;
                *dst++ = *src++;
            }
            else
            {
                uint32_t backOffset;
                int count;

                packedSize -= 2;
                if (packedSize < 0)
                    return;
                backOffset = src[0] | ((uint32_t)(src[1] >> 4) << 8);
                count = (src[1] & 0x0F) + 3;
                src += 2;
                /* Побайтно вперёд: источник может перекрываться с приёмником. */
                while (count-- != 0)
                {
                    *dst = *(dst - backOffset);
                    dst++;
                }
            }
        }
    }
}
