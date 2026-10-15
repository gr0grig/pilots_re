/*
 * Game Maker Player («Братья Пилоты: По следам полосатого слона», PILOTS.EXE).
 * Общий заголовок: типы данных формата игры, глобальное состояние движка, прототипы.
 *
 * Восстановлено по базе IDA. Размеры всех структур, которые читаются из файла игры
 * или лежат в пуле записей, проверяются _Static_assert. Сборка — только x86 (32 разряда):
 * записи файла хранят 32-разрядные указатели, а скрипты передают адреса как int32_t.
 */
#ifndef GM_H
#define GM_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>

_Static_assert(sizeof(void *) == 4, "Сборка только для x86: формат игры хранит 32-разрядные указатели");

#define NORETURN __declspec(noreturn)

/* ========================================================================== */
/* Типы                                                                       */
/* ========================================================================== */

typedef struct GM_WINDOW GM_WINDOW;
typedef struct GM_GAME_HEADER GM_GAME_HEADER;
typedef struct GM_ARENA GM_ARENA;
typedef struct GM_CLASS GM_CLASS;
typedef struct GM_SOUND GM_SOUND;
typedef struct GM_PANEL_ITEM GM_PANEL_ITEM;
typedef struct GM_PANEL GM_PANEL;
typedef struct GM_ROOM GM_ROOM;
typedef struct GM_CLASS_SET GM_CLASS_SET;
typedef union GM_FRAME_LISTS GM_FRAME_LISTS;
typedef struct GM_GRAPHIC GM_GRAPHIC;
typedef struct GM_IMAGE_DISK GM_IMAGE_DISK;
typedef struct GM_IMAGE GM_IMAGE;
typedef union GM_TILEPOS GM_TILEPOS;
typedef struct GM_SPRITE GM_SPRITE;
typedef struct GM_DIRTY_RECT GM_DIRTY_RECT;
typedef struct GM_SYSVARS GM_SYSVARS;
typedef struct GM_SETTINGS GM_SETTINGS;
typedef union GM_TILE GM_TILE;
typedef union GM_RULE_CELL GM_RULE_CELL;
typedef struct GM_FRAME GM_FRAME;
typedef struct GM_FONT GM_FONT;
typedef struct GM_OBJECT GM_OBJECT;
typedef struct GM_BACKGROUND GM_BACKGROUND;
typedef struct GM_WORLD_HEADER GM_WORLD_HEADER;
typedef struct GM_VLQ_RANGE GM_VLQ_RANGE;
typedef struct GM_ROW_SHIFT GM_ROW_SHIFT;
typedef struct GM_MOVIE_CHUNK GM_MOVIE_CHUNK;
typedef struct GM_SOUND_CHANNEL GM_SOUND_CHANNEL;
typedef union GM_POOL_ENTRY GM_POOL_ENTRY;
typedef union GM_RULE_HEADER GM_RULE_HEADER;
typedef union GM_RULE_GROUP GM_RULE_GROUP;
typedef union GM_RULE_ITEM GM_RULE_ITEM;
typedef struct GM_RULE GM_RULE;
typedef struct GM_MATCH_RANGE GM_MATCH_RANGE;
typedef struct GM_RECORD_SIZES GM_RECORD_SIZES;
typedef struct GM_FILE_TRAILER GM_FILE_TRAILER;
typedef struct GM_INDEX_HEADER GM_INDEX_HEADER;
typedef struct GM_INLINE_CALL GM_INLINE_CALL;
typedef struct GM_NATIVE GM_NATIVE;
typedef union GM_FORMAT_BUFFER GM_FORMAT_BUFFER;
typedef struct GM_BITMAPINFO_PAL GM_BITMAPINFO_PAL;
typedef struct GM_VAR_RANGE GM_VAR_RANGE;
typedef struct GM_VAR_DESC GM_VAR_DESC;
typedef struct GM_MUSIC_DATA GM_MUSIC_DATA;
typedef struct GM_SOUND_DATA GM_SOUND_DATA;

typedef uint8_t GM_EVENT;
enum
{
    GMEV_NONE = 0xE,
    GMEV_MOUSE_DOWN = 0x81,
    GMEV_ACTION1 = 0x82,
    GMEV_ACTION2 = 0x83,
    GMEV_HELP = 0x84,
    GMEV_NEXT_FOCUS = 0x8F,
    GMEV_NO_TARGET = 0x0,
    GMEV_CLEARED = 0xFF,
};

struct GM_WINDOW
{
    GM_GAME_HEADER *header;
    GM_ARENA *arena;
    GM_SETTINGS *settings;
    HDC hdc;
    HPALETTE hPalette;
    HGLOBAL hArenaMem;
    void *arenaMem;
    int nShowCmd;
    const char *errorMessage;
    uint32_t frameDelayMs;
    int unused28;
    int clientWidth;
    int clientHeight;
    int viewWidth;
    int viewHeight;
    int screenWidth;
    int screenHeight;
    int frameExtraWidth;
    int frameExtraHeight;
    int mouseX;
    int mouseY;
    int clickX;
    int clickY;
    GM_EVENT event;
    uint8_t actionKey;
    uint8_t lastChar;
    uint8_t pad5F;
    int dataFile;
    int saveFile;
    char helpPath[256];
};
_Static_assert(sizeof(GM_WINDOW) == 360, "GM_WINDOW");

struct GM_GAME_HEADER
{
    uint32_t version;
    int memoryKB;
    uint32_t fileBufferSize;
    int screenWidth;
    int screenHeight;
    int tileWidth;
    int tileHeight;
    int movieCount;
    uint8_t reserved20[3];
    uint8_t frameRate;
    bool resizable;
    bool systemCursor;
    uint8_t transition;
    uint8_t introMode;
    int introX;
    int introY;
    int introWidth;
    int introHeight;
    char movieDir[64];
    char title[32];
};
_Static_assert(sizeof(GM_GAME_HEADER) == 152, "GM_GAME_HEADER");

struct GM_ARENA
{
    int freeBytes;
    uint8_t *freePtr;
    int frameBufferSize;
    void *frameBuffer;
};
_Static_assert(sizeof(GM_ARENA) == 16, "GM_ARENA");

typedef uint8_t GM_CLASS_KIND;
enum
{
    GMCLASS_STATIC = 0x0,
    GMCLASS_ALL_INPUT = 0x1,
    GMCLASS_POINTABLE = 0x2,
    GMCLASS_FOCUSABLE = 0x3,
};

struct GM_CLASS
{
    GM_CLASS_KIND kind;
    uint8_t ruleOrientations;
    uint8_t defaultLayer;
    uint8_t varsSizeMinus1;
    uint8_t *createScript;
    GM_RULE **rules;
    uint8_t *destroyScript;
};
_Static_assert(sizeof(GM_CLASS) == 16, "GM_CLASS");

struct GM_SOUND
{
    uint32_t reserved;
    GM_SOUND_DATA *data;
};
_Static_assert(sizeof(GM_SOUND) == 8, "GM_SOUND");

struct GM_PANEL_ITEM
{
    uint16_t x;
    uint16_t y;
    uint16_t graphicIndex;
    uint8_t unused;
    bool isLast;
};
_Static_assert(sizeof(GM_PANEL_ITEM) == 8, "GM_PANEL_ITEM");

struct GM_PANEL
{
    GM_RULE *rule;
    GM_PANEL_ITEM *items;
};
_Static_assert(sizeof(GM_PANEL) == 8, "GM_PANEL");

struct GM_ROOM
{
    void *loadBuffer;
    uint32_t fileOffset;
    void *savedState;
    int savedObjectCount;
};
_Static_assert(sizeof(GM_ROOM) == 16, "GM_ROOM");

struct GM_CLASS_SET
{
    uint8_t *memberBits;
    uint8_t *randomChoices;
    uint8_t *orientations;
    uint32_t unused;
};
_Static_assert(sizeof(GM_CLASS_SET) == 16, "GM_CLASS_SET");

union GM_FRAME_LISTS
{
    GM_FRAME *list;
    GM_FRAME **byDirection;
};
_Static_assert(sizeof(GM_FRAME_LISTS) == 4, "GM_FRAME_LISTS");

struct GM_GRAPHIC
{
    bool isFont;
    uint8_t options;
    uint8_t dirMode;
    uint8_t frameCount;
    GM_FRAME_LISTS frames;
};
_Static_assert(sizeof(GM_GRAPHIC) == 8, "GM_GRAPHIC");

typedef enum GM_RECORD_TYPE
{
    GMREC_SETTINGS = 0xF,
    GMREC_GAME_HEADER = 0x10,
    GMREC_WORLD_HEADER = 0x11,
    GMREC_SYSVARS = 0x12,
    GMREC_ZEROED_BLOCK = 0x13,
    GMREC_MOVIE = 0x14,
    GMREC_ROOM = 0x18,
    GMREC_ROOM_BUILD = 0x19,
    GMREC_CLASS = 0x20,
    GMREC_CLASS_CREATE_SCRIPT = 0x21,
    GMREC_CLASS_DESTROY_SCRIPT = 0x22,
    GMREC_CLASS_RULES = 0x23,
    GMREC_RULE = 0x2A,
    GMREC_RULE_PRECONDITION = 0x2B,
    GMREC_RULE_SCRIPT = 0x2C,
    GMREC_CLASS_SET_MEMBERS = 0x38,
    GMREC_CLASS_SET_CHOICES = 0x39,
    GMREC_CLASS_SET_ORIENTATIONS = 0x3A,
    GMREC_GRAPHIC = 0x40,
    GMREC_GRAPHIC_FRAMES = 0x41,
    GMREC_GRAPHIC_DIR_FRAMES = 0x42,
    GMREC_FRAME_IMAGE = 0x43,
    GMREC_SOUND = 0x50,
    GMREC_SOUND_DATA = 0x51,
    GMREC_MUSIC = 0x52,
    GMREC_PANEL_RULE = 0x60,
    GMREC_PANEL_ITEMS = 0x61,
    GMREC_VARS_KEEP_ALWAYS = 0x7C,
    GMREC_VARS_KEEP_ON_RESTART = 0x7D,
    GMREC_VARS_SAVEGAME_ONLY = 0x7E,
} GM_RECORD_TYPE;

struct GM_IMAGE_DISK
{
    uint32_t magic;
    uint32_t fileOffset;
    uint32_t packedSize;
};
_Static_assert(sizeof(GM_IMAGE_DISK) == 12, "GM_IMAGE_DISK");

struct GM_IMAGE
{
    uint16_t width;
    uint16_t height;
    union
    {
        uint8_t pixels[12];
        GM_IMAGE_DISK disk;
    };
};
_Static_assert(sizeof(GM_IMAGE) == 16, "GM_IMAGE");

typedef uint8_t GM_SPRITE_FLAGS;
enum
{
    GMSPR_USED = 0x1,
    GMSPR_OBJECT = 0x2,
    GMSPR_PLAY_ONCE = 0x4,
    GMSPR_MIRROR_X = 0x8,
    GMSPR_MIRROR_Y = 0x10,
    GMSPR_TEXT_GLYPH = 0x20,
    GMSPR_SCREEN_ANCHORED = 0x40,
    GMSPR_VISIBLE = 0x80,
};

union GM_TILEPOS
{
    struct
    {
        uint8_t x;
        uint8_t y;
    };
    uint16_t packed;
};
_Static_assert(sizeof(GM_TILEPOS) == 2, "GM_TILEPOS");

struct GM_SPRITE
{
    GM_SPRITE_FLAGS flags;
    uint8_t frameIndex;
    uint8_t frameCount;
    uint8_t layer;
    GM_TILEPOS fromTile;
    uint16_t ownerIndex;
    int16_t x;
    int16_t y;
    GM_FRAME *frame;
};
_Static_assert(sizeof(GM_SPRITE) == 16, "GM_SPRITE");

struct GM_DIRTY_RECT
{
    int left;
    int top;
    int right;
    int bottom;
    GM_DIRTY_RECT *next;
};
_Static_assert(sizeof(GM_DIRTY_RECT) == 20, "GM_DIRTY_RECT");

struct GM_SYSVARS
{
    bool quitRequest;
    uint8_t frameRate;
    bool keyPressed;
    uint8_t lastKey;
    uint32_t frameCount;
};
_Static_assert(sizeof(GM_SYSVARS) == 8, "GM_SYSVARS");

typedef uint8_t GM_INPUT_DEVICES;
enum
{
    GMDEV_KEYBOARD = 0x1,
    GMDEV_MOUSE = 0x2,
};

struct GM_SETTINGS
{
    char saveExt[4];
    GM_INPUT_DEVICES inputFlags;
    uint8_t reserved5[3];
    int startEntry;
    int currentRoom;
    int roomsVisited;
    bool soundOn;
    bool musicOn;
    bool inputEnabled;
    bool moviesOn;
    uint8_t soundChannels;
    uint8_t soundVolume;
    uint8_t musicVolume;
    uint8_t frameRate;
    int frameCount;
    int currentMusic;
    int cursorGraphic;
    uint8_t dirKeys[8];
    uint8_t specialKeys[4];
};
_Static_assert(sizeof(GM_SETTINGS) == 52, "GM_SETTINGS");

typedef enum GM_OPCODE
{
    OP_RETURN = 0x0,
    OP_CMP_EQ = 0x1,
    OP_CMP_NE = 0x2,
    OP_CMP_LT_SIGNED = 0x3,
    OP_CMP_LE_SIGNED = 0x4,
    OP_CMP_GT_SIGNED = 0x5,
    OP_CMP_GE_SIGNED = 0x6,
    OP_CMP_LT_UNSIGNED = 0x7,
    OP_CMP_LE_UNSIGNED = 0x8,
    OP_CMP_GT_UNSIGNED = 0x9,
    OP_CMP_GE_UNSIGNED = 0xA,
    OP_JZ = 0xB,
    OP_JMP = 0xC,
    OP_ADJUST_SP = 0xD,
    OP_STORE_BYTE_GLOBAL = 0xE,
    OP_STORE_BYTE_OBJECT = 0xF,
    OP_STORE_DWORD_GLOBAL = 0x10,
    OP_STORE_DWORD_OBJECT = 0x11,
    OP_RET_CALL = 0x12,
    OP_RET_FREE = 0x13,
    OP_DUP = 0x14,
    OP_ADD = 0x15,
    OP_MUL = 0x16,
    OP_OR = 0x17,
    OP_XOR = 0x18,
    OP_AND = 0x19,
    OP_NEG = 0x1A,
    OP_SAR = 0x1B,
    OP_SHL = 0x1C,
    OP_PUSH_IMM32 = 0x1D,
    OP_INC = 0x1E,
    OP_DEC = 0x1F,
    OP_XCHG = 0x20,
    OP_PUSH_LO = 0x21,
    OP_POP_HI = 0x22,
    OP_LEA_GLOBAL = 0x23,
    OP_LEA_GLOBAL_ALT = 0x24,
    OP_LEA_OBJECT = 0x25,
    OP_LEA_LOCAL = 0x26,
    OP_STORE_BYTE_IND = 0x27,
    OP_STORE_DWORD_IND = 0x28,
    OP_MUL4 = 0x29,
    OP_ADD4 = 0x2A,
    OP_SUB4 = 0x2B,
    OP_XCHG_STACK = 0x2C,
    OP_SUB = 0x2D,
    OP_IDIV = 0x2E,
    OP_LOAD_SBYTE_GLOBAL = 0x2F,
    OP_LOAD_SBYTE_OBJECT = 0x30,
    OP_LOAD_DWORD_GLOBAL = 0x31,
    OP_LOAD_DWORD_OBJECT = 0x32,
    OP_LOAD_SBYTE_IND = 0x33,
    OP_LOAD_DWORD_IND = 0x34,
    OP_CALL_SCRIPT = 0x35,
    OP_CALL_NATIVE = 0x36,
    OP_CALL_INDIRECT = 0x37,
} GM_OPCODE;

typedef enum GM_FORMAT_KIND
{
    GMFMT_SBYTE = 0x0,
    GMFMT_STRING_PTR = 0x1,
    GMFMT_INLINE_STRING = 0x2,
    GMFMT_INT = 0x3,
    GMFMT_INT_PTR = 0x4,
    GMFMT_SCRIPT_CALL = 0x5,
    GMFMT_KIND_MASK = 0x7,
} GM_FORMAT_KIND;

union GM_TILE
{
    struct
    {
        uint8_t classIndex;
        uint8_t flags;
    };
    uint16_t raw;
};
_Static_assert(sizeof(GM_TILE) == 2, "GM_TILE");

union GM_RULE_CELL
{
    struct
    {
        uint8_t classIndex;
        uint8_t flags;
        int8_t dx;
        int8_t dy;
    };
    struct
    {
        GM_TILE tile;
        uint16_t delta;
    };
    uint32_t raw;
};
_Static_assert(sizeof(GM_RULE_CELL) == 4, "GM_RULE_CELL");

typedef uint8_t GM_DIRECTION;
enum
{
    GMDIR_NONE = 0x0,
    GMDIR_SOUTH = 0x10,
    GMDIR_WEST = 0x20,
    GMDIR_NORTH = 0x40,
    GMDIR_EAST = 0x80,
};

struct GM_FRAME
{
    GM_IMAGE *image;
    int16_t hotX;
    int16_t hotY;
};
_Static_assert(sizeof(GM_FRAME) == 8, "GM_FRAME");

struct GM_FONT
{
    bool isFont;
    uint8_t firstChar;
    uint8_t lastChar;
    uint8_t frameCount;
    GM_FRAME *glyphs;
};
_Static_assert(sizeof(GM_FONT) == 8, "GM_FONT");

typedef uint8_t GM_INPUT_BITS;
enum
{
    GMIN_DIR_MASK = 0x7,
    GMIN_SHIFT = 0x8,
    GMIN_HOVER = 0x10,
    GMIN_PRESSED = 0x20,
    GMIN_KEY = 0x40,
    GMIN_RELEASED = 0x80,
};

struct GM_OBJECT
{
    uint8_t flags;
    uint8_t classIndex;
    GM_TILE underTile;
    GM_INPUT_BITS input;
    uint8_t layer;
    GM_TILEPOS tile;
    uint16_t restSpriteIndex;
    uint16_t spriteIndex;
    uint8_t *vars;
};
_Static_assert(sizeof(GM_OBJECT) == 16, "GM_OBJECT");

struct GM_BACKGROUND
{
    uint32_t reserved0;
    uint32_t reserved4;
    int width;
    int height;
    int paletteOffset;
    uint32_t paletteEnd; /* Смещение конца палитры (paletteOffset + 768) — движком не читается. */
    uint8_t pixels[1];
};
_Static_assert(sizeof(GM_BACKGROUND) == 28, "GM_BACKGROUND");

struct GM_WORLD_HEADER
{
    int roomCountA; /* Один из двух размеров сетки комнат мира; код использует только произведение roomCountA·roomCountB (= g_RoomCount). Какое из полей — строки, а какое — столбцы, по коду и данным (во всех 20 мирах оба = 1) установить нельзя. */
    int roomCountB; /* Один из двух размеров сетки комнат мира; код использует только произведение roomCountA·roomCountB (= g_RoomCount). Какое из полей — строки, а какое — столбцы, по коду и данным (во всех 20 мирах оба = 1) установить нельзя. */
    int mapWidth;
    int mapHeight;
    int viewWidth;
    int viewHeight;
    int backgroundSize;
    int classCount;
    int classSetCount;
    int graphicCount;
    int soundCount;
    int musicCount;
    int panelCount;
};
_Static_assert(sizeof(GM_WORLD_HEADER) == 52, "GM_WORLD_HEADER");

struct GM_VLQ_RANGE
{
    int min;
    int max;
};
_Static_assert(sizeof(GM_VLQ_RANGE) == 8, "GM_VLQ_RANGE");

struct GM_ROW_SHIFT
{
    int maxWidth;
    int shift;
};
_Static_assert(sizeof(GM_ROW_SHIFT) == 8, "GM_ROW_SHIFT");

typedef uint8_t GM_MOVIE_CHUNK_TYPE;
enum
{
    GMMC_HEADER = 0x0,
    GMMC_FRAME = 0x1,
    GMMC_PALETTE = 0x2,
    GMMC_SOUND = 0x3,
    GMMC_MUSIC = 0x4,
};

struct GM_MOVIE_CHUNK
{
    GM_MOVIE_CHUNK_TYPE type;
    uint8_t subtype;
    uint8_t hasArg0;
    union
    {
        uint8_t hasArg1;
        uint8_t fillColor;
    };
    uint32_t arg0;
    uint32_t arg1;
};
_Static_assert(sizeof(GM_MOVIE_CHUNK) == 12, "GM_MOVIE_CHUNK");

struct GM_SOUND_CHANNEL
{
    uint8_t *start;
    uint8_t *cur;
    uint32_t remaining;
    uint8_t unreadFF;
    uint8_t unreadZero;
    int8_t mode;
    uint8_t pad;
};
_Static_assert(sizeof(GM_SOUND_CHANNEL) == 16, "GM_SOUND_CHANNEL");

union GM_POOL_ENTRY
{
    uint8_t flags;
    GM_SPRITE sprite;
    GM_OBJECT object;
    uint32_t words[4];
};
_Static_assert(sizeof(GM_POOL_ENTRY) == 16, "GM_POOL_ENTRY");

typedef uint8_t GM_LOAD_TAG;
enum
{
    GMTAG_END = 0x0,
    GMTAG_ARG1 = 0x1,
    GMTAG_ARG2 = 0x2,
    GMTAG_ARG3 = 0x3,
    GMTAG_BODY = 0x4,
    GMTAG_BLOCK = 0x5,
    GMTAG_APPLY = 0xFF,
};

typedef enum GM_GMW_COMMAND
{
    GMW_SET_SOUND_VOLUME_BASE = 0x0,
    GMW_SET_MUSIC_VOLUME_BASE = 0x100,
    GMW_OPEN = 0x200,
    GMW_CLOSE = 0x201,
    GMW_SUBMIT = 0x202,
    GMW_RESET = 0x203,
    GMW_GET_SOUND_VOLUME = 0x204,
    GMW_NEXT_BUFFER = 0x205,
    GMW_PAUSE = 0x206,
    GMW_RESUME = 0x207,
    GMW_PLAY_MUSIC = 0x208,
    GMW_STOP_MUSIC = 0x209,
    GMW_GET_MUSIC_VOLUME = 0x20A,
} GM_GMW_COMMAND;

typedef uint8_t GM_RULE_FLAGS;
enum
{
    GMRULE_PRECONDITION = 0x1,
    GMRULE_PATTERN = 0x2,
    GMRULE_MOVE = 0x4,
    GMRULE_SCRIPT = 0x8,
    GMRULE_PLACE = 0x10,
    GMRULE_LAST = 0x80,
};

union GM_RULE_HEADER
{
    struct
    {
        uint8_t ruleFlags;
        uint8_t moveFlags;
        int8_t moveDx;
        int8_t moveDy;
    };
    GM_RULE_CELL move;
    uint32_t raw;
};
_Static_assert(sizeof(GM_RULE_HEADER) == 4, "GM_RULE_HEADER");

union GM_RULE_GROUP
{
    struct
    {
        uint16_t count;
        uint8_t isLast;
        uint8_t type;
    };
    uint32_t raw;
};
_Static_assert(sizeof(GM_RULE_GROUP) == 4, "GM_RULE_GROUP");

union GM_RULE_ITEM
{
    GM_RULE_GROUP group;
    GM_RULE_CELL cell;
    uint8_t *script;
    uint32_t raw;
};
_Static_assert(sizeof(GM_RULE_ITEM) == 4, "GM_RULE_ITEM");

struct GM_RULE
{
    union
    {
        GM_RULE_HEADER header;
        GM_RULE_ITEM words[1];
    };
    GM_RULE_ITEM items[1];
};
_Static_assert(sizeof(GM_RULE) == 8, "GM_RULE");

struct GM_MATCH_RANGE
{
    int first;
    int end;
};
_Static_assert(sizeof(GM_MATCH_RANGE) == 8, "GM_MATCH_RANGE");

struct GM_RECORD_SIZES
{
    uint32_t storedSize;
    uint32_t unpackedSize;
};
_Static_assert(sizeof(GM_RECORD_SIZES) == 8, "GM_RECORD_SIZES");

struct GM_FILE_TRAILER
{
    uint32_t indexSize;
    uint8_t versionLow[3];
    uint8_t formatVersion;
    uint32_t magic;
};
_Static_assert(sizeof(GM_FILE_TRAILER) == 12, "GM_FILE_TRAILER");

struct GM_INDEX_HEADER
{
    uint32_t lastResourceId;
    uint32_t dataBase;
    uint8_t rest[176];
};
_Static_assert(sizeof(GM_INDEX_HEADER) == 184, "GM_INDEX_HEADER");

#pragma pack(push, 1)
struct GM_INLINE_CALL
{
    uint8_t opcode;
    int scriptOffset;
    uint8_t terminator;
};
#pragma pack(pop)
_Static_assert(sizeof(GM_INLINE_CALL) == 6, "GM_INLINE_CALL");

union GM_FORMAT_BUFFER
{
    char text[256];
    GM_INLINE_CALL call;
};
_Static_assert(sizeof(GM_FORMAT_BUFFER) == 256, "GM_FORMAT_BUFFER");

struct GM_BITMAPINFO_PAL
{
    BITMAPINFOHEADER bmiHeader;
    uint16_t paletteIndices[256];
};
_Static_assert(sizeof(GM_BITMAPINFO_PAL) == 552, "GM_BITMAPINFO_PAL");

struct GM_VAR_RANGE
{
    int offset;
    uint32_t size;
};
_Static_assert(sizeof(GM_VAR_RANGE) == 8, "GM_VAR_RANGE");

struct GM_VAR_DESC
{
    int count;
    GM_VAR_RANGE ranges[1];
};
_Static_assert(sizeof(GM_VAR_DESC) == 12, "GM_VAR_DESC");

typedef uint8_t GM_WALK_CELL;
enum
{
    GMWALK_OPEN = 0x0,
    GMWALK_GOAL = 0x2,
    GMWALK_BLOCKED = 0x3,
    GMWALK_MARK4 = 0x4,
    GMWALK_MARK5 = 0x5,
    GMWALK_MARK6 = 0x6,
};

struct GM_MUSIC_DATA
{
    uint32_t length;
    uint8_t events[1];
};
_Static_assert(sizeof(GM_MUSIC_DATA) == 8, "GM_MUSIC_DATA");

struct GM_SOUND_DATA
{
    uint32_t length;
    uint8_t pcm[1];
};
_Static_assert(sizeof(GM_SOUND_DATA) == 8, "GM_SOUND_DATA");

typedef uint8_t GM_CELL_FLAGS;
enum
{
    GMCELL_BY_CLASS_SET = 0x1,
    GMCELL_MUST_MATCH = 0x2,
    GMCELL_MODE_MASK = 0xC,
    GMCELL_DIR_MASK = 0xF0,
};

typedef enum GM_CELL_MODE
{
    GMCELL_GROUP_DONE = 0x0,
    GMCELL_VETO = 0x4,
    GMCELL_ACCEPT = 0x8,
    GMCELL_COLLECT = 0xC,
} GM_CELL_MODE;

typedef uint8_t GM_OBJECT_FLAGS;
enum
{
    GMOBJ_USED = 0x1,
    GMOBJ_INSTANCE = 0x2,
    GMOBJ_MOVED = 0x4,
    GMOBJ_VARS_ALLOCATED = 0x8,
    GMOBJ_DIR_MASK = 0xF0,
};

typedef uint8_t GM_UNDER_TILE_FLAGS;
enum
{
    GMUT_SKIP_TICK = 0x1,
    GMUT_HAS_TEXT = 0x2,
    GMUT_DIR_MASK = 0xF0,
};

typedef enum GM_RULE_GROUP_BITS
{
    GMGROUP_IS_LAST = 0x10000,
} GM_RULE_GROUP_BITS;

typedef uint8_t GM_MOVIE_STEP;
enum
{
    GMMS_ERROR = 0x0,
    GMMS_CONTINUE = 0x1,
    GMMS_SKIP_TO_HEADER = 0x2,
    GMMS_FINISHED = 0x3,
};

typedef enum GM_FORMAT_SOURCE
{
    GMFMT_FROM_GLOBALS = 0x0,
    GMFMT_FROM_OBJECT = 0x10,
    GMFMT_FROM_NATIVE = 0x20,
    GMFMT_SOURCE_MASK = 0x70,
} GM_FORMAT_SOURCE;

typedef enum GM_TEXT_CODE
{
    GMTEXT_FIELD = 0xF,
} GM_TEXT_CODE;

typedef enum GM_PLACE_GROUP_TYPE
{
    GMPG_ONE_OF_GROUPS = 0x0,
    GMPG_RANDOM_COUNT = 0x1,
    GMPG_ONE_CELL = 0x2,
    GMPG_EACH_HALF = 0x3,
    GMPG_BY_MATCH = 0x4,
} GM_PLACE_GROUP_TYPE;

typedef uint8_t GM_MOVIE_HEADER_SUBTYPE;
enum
{
    GMMH_END_MARK = 0x0,
    GMMH_REPEAT = 0x1,
    GMMH_PACKED_BUFFER = 0x2,
    GMMH_FRAME_BUFFERS = 0x3,
    GMMH_AUDIO_BUFFERS = 0x4,
    GMMH_OFFSET = 0x5,
    GMMH_SIZE = 0x6,
};

typedef enum GM_SPECIAL_INDEX
{
    GM_NO_INDEX = 0xFFFF,
} GM_SPECIAL_INDEX;

typedef uint8_t GM_SPECIAL_CLASS;
enum
{
    GMCLASS_EMPTY_TILE = 0xFE,
    GMCLASS_ANY = 0xFF,
};

typedef enum GM_DIR8
{
    GMDIR8_N = 0x0,
    GMDIR8_NE = 0x1,
    GMDIR8_E = 0x2,
    GMDIR8_SE = 0x3,
    GMDIR8_S = 0x4,
    GMDIR8_SW = 0x5,
    GMDIR8_W = 0x6,
    GMDIR8_NW = 0x7,
    GMDIR8_SHIFT_HELD = 0x8,
} GM_DIR8;

typedef uint8_t GM_GRAPHIC_OPTIONS;
enum
{
    GMGR_PLAY_ONCE = 0x1,
    GMGR_MIRROR_X_EAST = 0x2,
    GMGR_MIRROR_Y_NORTH = 0x4,
    GMGR_STANDING_FRAME = 0x8,
    GMGR_ON_DISK = 0x80,
};

typedef uint8_t GM_MOVIE_RECT_FLAGS;
enum
{
    GMMR_CODEC_MASK = 0x3,
    GMMR_X16 = 0x4,
    GMMR_Y16 = 0x8,
    GMMR_WIDTH16 = 0x10,
    GMMR_HEIGHT16 = 0x20,
    GMMR_FULL_FRAME = 0x40,
    GMMR_LAST = 0x80,
};

typedef enum GM_MIDI_STATUS
{
    MIDI_NOTE_OFF = 0x80,
    MIDI_NOTE_ON = 0x90,
    MIDI_PROGRAM_CHANGE = 0xC0,
    MIDI_CHANNEL_PRESSURE = 0xD0,
    MIDI_SYSEX = 0xF0,
    MIDI_SYSEX_END = 0xF7,
    MIDI_TIMING_CLOCK = 0xF8,
    MIDI_META = 0xFF,
    MIDI_COMMAND_MASK = 0xF0,
} GM_MIDI_STATUS;

typedef enum GM_FRAME_SLOT
{
    GMSLOT_STANDING = 0x0,
    GMSLOT_S = 0x1,
    GMSLOT_SW = 0x2,
    GMSLOT_W = 0x3,
    GMSLOT_NW = 0x4,
    GMSLOT_N = 0x5,
    GMSLOT_NE = 0x6,
    GMSLOT_E = 0x7,
    GMSLOT_SE = 0x8,
} GM_FRAME_SLOT;

typedef uint8_t GM_INTRO_MODE;
enum
{
    GMINTRO_NONE = 0x0,
    GMINTRO_MOVIE = 0x1,
    GMINTRO_AVI = 0x2,
};

typedef uint8_t GM_MOVIE_FRAME_SUBTYPE;
enum
{
    GMMF_NORMAL = 0x0,
    GMMF_FIRST = 0x1,
    GMMF_LOOP_END = 0x2,
};

typedef enum GM_MIDI_LOOP
{
    GM_MIDI_NO_LOOP = 0xFFFF,
} GM_MIDI_LOOP;

typedef uint8_t GM_SEEK_MODE;
enum
{
    GMSEEK_THROUGH_EMPTY = 0xFE,
    GMSEEK_DIRECTION_ONLY = 0xFF,
};

typedef uint8_t GM_NUMBER_TAG_BITS;
enum
{
    GMNUM_HIGH_BITS_MASK = 0x3,
    GMNUM_BYTES_MASK = 0xC,
    GMNUM_NEGATIVE = 0x10,
    GMNUM_RANGED = 0x20,
    GMNUM_RAW_BYTES_MASK = 0x1F,
};

typedef uint8_t GM_RECORD_TAG_BITS;
enum
{
    GMRT_SIZE_BYTES_MASK = 0x3,
    GMRT_PACKED_MASK = 0xC,
    GMRT_SHORT_LENGTH_MASK = 0x1F,
    GMRT_SHORT = 0x40,
};

typedef uint8_t GM_BLOCK_TAG_BITS;
enum
{
    GMBT_SIZE_BYTES_MASK = 0x3,
    GMBT_MARK = 0xEC,
};

typedef uint8_t GM_HEADER_TAG_BITS;
enum
{
    GMHT_TYPE_MASK = 0x7F,
    GMHT_NO_SLOT = 0x80,
};

/* Режим встроенных функций поиска пути (Native_SeekPointer/SeekClass/SeekClassSet). 1/2 — к ближайшему тайлу класса (прямо / путём); тройки 3–5, 6–8, 9–11 — цель под указателем, щелчок действием 1, действием 2; внутри тройки: прямое направление, вариант «пусто» (у SeekPointer — прямо, если цель пуста; у SeekClass — путь по пустым), путь (у SeekClass — только по тайлам класса). */
typedef enum GM_SEEK_COMMAND
{
    GMSEEKCMD_NEAREST_DIRECT = 0x1,
    GMSEEKCMD_NEAREST_PATH = 0x2,
    GMSEEKCMD_POINTER_DIRECT = 0x3,
    GMSEEKCMD_POINTER_EMPTY = 0x4,
    GMSEEKCMD_POINTER_PATH = 0x5,
    GMSEEKCMD_ACTION1_DIRECT = 0x6,
    GMSEEKCMD_ACTION1_EMPTY = 0x7,
    GMSEEKCMD_ACTION1_PATH = 0x8,
    GMSEEKCMD_ACTION2_DIRECT = 0x9,
    GMSEEKCMD_ACTION2_EMPTY = 0xA,
    GMSEEKCMD_ACTION2_PATH = 0xB,
} GM_SEEK_COMMAND;

/* Код Native_SetOption: чётный — выключить, нечётный — включить (ввод, звук, музыка, фильмы). */
typedef enum GM_OPTION_SET
{
    GMOPT_INPUT_OFF = 0x0,
    GMOPT_INPUT_ON = 0x1,
    GMOPT_SOUND_OFF = 0x2,
    GMOPT_SOUND_ON = 0x3,
    GMOPT_MUSIC_OFF = 0x4,
    GMOPT_MUSIC_ON = 0x5,
    GMOPT_MOVIES_OFF = 0x6,
    GMOPT_MOVIES_ON = 0x7,
} GM_OPTION_SET;

/* Код Native_GetOption: какой параметр прочитать. */
typedef enum GM_OPTION_QUERY
{
    GMOPTQ_INPUT = 0x0,
    GMOPTQ_SOUND = 0x1,
    GMOPTQ_MUSIC = 0x2,
    GMOPTQ_MOVIES = 0x3,
} GM_OPTION_QUERY;

/* Код Native_SaveGameOp: удалить, сохранить или загрузить слот. */
typedef enum GM_SAVE_OPERATION
{
    GMSAVEOP_DELETE = 0x0,
    GMSAVEOP_SAVE = 0x1,
    GMSAVEOP_LOAD = 0x2,
} GM_SAVE_OPERATION;

/* Код Native_PathAction: забыть найденный путь, очистить тайл цели, переместить объект на цель. */
typedef enum GM_PATH_ACTION
{
    GMPATH_FORGET = 0x0,
    GMPATH_CLEAR_GOAL = 0x1,
    GMPATH_MOVE_TO_GOAL = 0x2,
} GM_PATH_ACTION;

/* Слово тайла целиком: GMTILE_EMPTY_RAW = класс 0xFE (пустой тайл) и байт флагов 0xF0 (все разряды направления). */
typedef uint16_t GM_TILE_RAW;
enum
{
    GMTILE_EMPTY_RAW = 0xF0FE,
};

/* Версия формата файла игры и сохранений (поле version заголовка игры, второе двойное слово магии сохранения). */
typedef uint32_t GM_FILE_VERSION;
#define GM_FILE_VERSION_CURRENT 0x80000016u

/* Версия хвоста исполняемого файла с упакованной игрой. */
typedef enum GM_TRAILER_VERSION
{
    GM_TRAILER_VERSION_CURRENT = 0x4,
} GM_TRAILER_VERSION;

/* Байт-код RLE кадров фильма: разряд 7 сброшен — n+1 литералов; 10xxxxxx — пропуск n+1; 11xxxxxx — особые коды; младшие 6 разрядов — n. */
typedef uint8_t GM_RLE_CODE_BITS;
enum
{
    GMRLE_COUNT_MASK = 0x3F,
    GMRLE_SKIP_OR_SPECIAL = 0x40,
    GMRLE_NOT_LITERAL = 0x80,
};

/* Задержка собственного формата MIDI: разряд 7 первого байта — есть второй байт; младшие 7 разрядов — значение. */
typedef uint8_t GM_MIDI_DELTA_BITS;
enum
{
    MIDIDELTA_VALUE_MASK = 0x7F,
    MIDIDELTA_CONTINUED = 0x80,
};

/* Последний байт данных MIDI-сообщения собственного формата: младшие 7 разрядов — данные, разряд 7 — следующее событие без задержки. */
typedef uint8_t GM_MIDI_DATA_BITS;
enum
{
    MIDIDATA_VALUE_MASK = 0x7F,
    MIDIDATA_NO_DELAY = 0x80,
};

/* Заголовок блока кучи движка: длина в двойных словах и признак занятости в разряде 31. */
typedef uint32_t GM_HEAP_HEADER_BITS;
#define GMHEAP_SIZE_MASK 0x7FFFFFFFu
#define GMHEAP_USED 0x80000000u

/* 16-разрядный индекс записи пула в полях спрайта/объекта; GM_POOL_NONE — нет записи (нет владельца, нет спрайта). */
typedef uint16_t GM_POOL_INDEX;
enum
{
    GM_POOL_NONE = 0xFFFF,
};

/* Пределы насыщения 8-разрядного беззнакового PCM в микшере: при переполнении вверх отсчёт зажимается в 0xF0 (не 0xFF — особенность движка), вниз — в 0. */
typedef uint8_t GM_PCM8_LIMIT;
enum
{
    GMPCM_CLIP_LOW = 0x0,
    GMPCM_CLIP_HIGH = 0xF0,
};

/* ---- Указатели на функции ---- */

/* Преобразование клетки правила (поворот/отражение, выбор направления). */
typedef void (*GM_RULE_XFORM_FN)(GM_RULE_CELL *cell);
/* Постановка клетки правила на карту (абсолютные или относительные координаты). */
typedef int (*GM_PLACE_CELL_FN)(GM_RULE_CELL cell);
/* Добавочный блит кадра фильма: прибавляет к приёмнику значения из потока src
   (прямоугольник width x height, после строки приёмник сдвигается на rowStep назад);
   возвращает конец разобранного потока. */
typedef const uint8_t *(*GM_ADD_BLIT_FN)(uint8_t *dst, const uint8_t *src, int width, int height, int rowStep);
/* Обработчики звуковой подсистемы (WinMM, GMW.DLL или заглушки). */
typedef int (*GM_AUDIO_FN)(void);
typedef int (*GM_AUDIO_VOLUME_FN)(uint8_t volume);
typedef int (*GM_MUSIC_PLAY_FN)(const GM_MUSIC_DATA *music);
typedef void (*GM_AUDIO_VOID_FN)(void);
/* Универсальный переходник Win32s (UTRegister) в 16-разрядную GMW.DLL. */
typedef DWORD (WINAPI *GM_GMW_THUNK_FN)(void *buffer, DWORD command, void *translationList);
typedef BOOL (WINAPI *PFN_UTREGISTER)(HANDLE hModule, LPCSTR lpsz16BitDll, LPCSTR lpszInitName,
                                      LPCSTR lpszProcName, GM_GMW_THUNK_FN *ppfn32Thunk,
                                      FARPROC pfnUT32Callback, LPVOID lpBuff);
typedef VOID (WINAPI *PFN_UTUNREGISTER)(HANDLE hModule);

/* Привязка встроенной функции скриптов: число доводов на стеке машины и переходник,
   который снимает их (args[0] — вершина стека) и вызывает функцию с её настоящим прототипом. */
struct GM_NATIVE
{
    int argCount;
    int32_t (*invoke)(const int32_t *args);
};

/* ========================================================================== */
/* Глобальное состояние (определения — data.c)                                */
/* ========================================================================== */

extern GM_SPRITE g_CursorSprite;
extern uint8_t g_XformDirMap_RotateCW[16];
extern uint8_t g_XformDirMap_Rotate180[16];
extern uint8_t g_XformDirMap_RotateCCW[16];
extern uint8_t g_XformDirMap_MirrorLeftRight[16];
extern uint8_t g_XformDirMap_MirrorSwNeAxis[16];
extern uint8_t g_XformDirMap_MirrorUpDown[16];
extern uint8_t g_XformDirMap_MirrorNwSeAxis[16];
extern GM_RULE_XFORM_FN g_RuleXforms[8]; /* Восемь преобразований клетки правила (группа симметрий квадрата), по порядку: тождественное, поворот на 90° по часовой, на 180°, на 90° против часовой, отражение слева-направо, по оси ЮЗ–СВ, сверху-вниз, по оси СЗ–ЮВ. Индекс — ориентация, в которой проверяется правило. */
extern GM_RULE_XFORM_FN g_RotateCellByDirection[16];
extern uint8_t g_DirChoices_S_W[2];
extern uint8_t g_DirChoices_S_N[2];
extern uint8_t g_DirChoices_W_N[2];
extern uint8_t g_DirChoices_S_W_N[3];
extern uint8_t g_DirChoices_S_E[2];
extern uint8_t g_DirChoices_W_E[2];
extern uint8_t g_DirChoices_S_W_E[3];
extern uint8_t g_DirChoices_N_E[2];
extern uint8_t g_DirChoices_S_N_E[3];
extern uint8_t g_DirChoices_W_N_E[3];
extern uint8_t g_DirChoices_All[4];
extern GM_RULE_XFORM_FN g_ResolveDirectionMask[16]; /* Шестнадцать обработчиков по маске направлений клетки (флаги >> 4): одно направление — оставить как есть, несколько — выбрать одно случайно (g_DirChoices_*), 0 — ничего не делать. */
extern uint32_t g_RandomSeed;
extern char s_InsufficientMemory[21];
extern char s_InvalidGameData[24];
extern GM_RECORD_SIZES g_RecordSizesZero;
extern GM_ROW_SHIFT g_MapRowShifts[7];
extern uint8_t g_SaveXorKey[32];
extern uint32_t g_SaveMagic[2];
extern char g_SaveGameExt[4];
extern char s_MoviePlaybackError[22];
extern const GM_NATIVE g_NativeFunctions[57]; /* Таблица 57 встроенных функций скриптов (код 54 интерпретатора GmVm_Run, номер — операнд). */
extern uint8_t g_DirIndexMirror[8];
extern char s_PlayAviArgsFormat[15];
extern const GM_ADD_BLIT_FN g_AddBlitters[4];
extern GM_AUDIO_FN g_pfnAudioSubmit;
extern GM_MUSIC_PLAY_FN g_pfnMusicPlay;
extern GM_AUDIO_FN g_pfnAudioReset;
extern GM_AUDIO_FN g_pfnMusicStop;
extern GM_AUDIO_FN g_pfnGetSoundVolume;
extern GM_AUDIO_FN g_pfnGetMusicVolume;
extern GM_AUDIO_FN g_pfnAudioNextBuffer;
extern GM_AUDIO_FN g_pfnAudioPause;
extern GM_AUDIO_FN g_pfnAudioResume;
extern GM_AUDIO_VOLUME_FN g_pfnSetSoundVolume;
extern GM_AUDIO_VOLUME_FN g_pfnSetMusicVolume;
extern GM_AUDIO_VOID_FN g_pfnAudioShutdown;
extern char s_GmwDll[8];
extern char s_UTUnRegister[13];
extern char s_UTRegister[11];
extern char s_Kernel32Dll[13];
extern GM_BITMAPINFO_PAL g_FrameBitmapInfo;
extern char s_GameMakerPlayer[18];
extern char s_HelpExt[5];
extern char g_DriveRootPath[4];
extern char s_BmpExt[5];
extern char s_AviExt[5];
extern char s_Backslash[2];
extern char s_InvalidMovieBackdrop[33];
extern char s_DefaultIntroName[6];
extern char s_GameRequiresLargerScreen[29];
extern char s_WindowClassName[11];
extern char s_Gdi32Dll[10];
extern char s_CreateDIBSection[17];
extern COLORREF g_TransitionGridColors[4];
extern int g_DissolvePatternX[16];
extern int g_DissolvePatternY[16];
extern STARTUPINFOA g_StartupInfoTemplate;
extern PROCESS_INFORMATION g_ProcessInfoTemplate;
extern char s_SavePathFormat[8];
extern char s_ExeExt[5];
extern char s_TempSavePathFormat[6];
extern int g_ClipWidth;
extern int g_ClipHeight;
extern int g_ScrollX;
extern int g_ScrollY;
extern GM_DIRTY_RECT *g_DirtyRects;
extern GM_OBJECT *g_FocusObject;
extern jmp_buf g_RestartJmp;
extern bool g_CurObjectMoved;
extern uint8_t g_DissolvePending;
extern bool g_CursorInWindow;
extern GM_OBJECT *g_ResumeObject;
extern uint16_t g_PatternMatches[512];
extern uint32_t *g_HeapBase;
extern uint32_t *g_HeapTop;
extern uint32_t g_HeapTopFreeDwords;
extern void *g_LoadScratch;
extern uint8_t g_SmallTempBuffer[152];
extern uint32_t g_ReadBufFill;
extern uint32_t g_ReadBufLeft;
extern uint8_t *g_ReadBuf;
extern int g_ResourceDataBase;
extern int g_ResourceIndexSize;
extern uint32_t g_LastResourceId;
extern uint32_t g_RecordSize; /* Размер текущей записи (распакованный, если запись сжата). */
extern uint32_t g_RecordPackedSize; /* Упакованный размер текущей записи (0 — запись не сжата). */
extern int8_t g_LoadTag;
extern uint8_t *g_LoadPtr; /* Куда загрузчик кладёт следующую запись (вершина резерва кучи). */
extern void *g_RecordData;
extern uint32_t g_RoomFileOffset;
extern int g_StartRoom;
extern bool g_EngineInitPending;
extern uint8_t g_GraphicOnDisk; /* Признак текущей графики «изображения на диске»: вместо пикселей пишется заголовок {'Disk', смещение, упакованный размер}. */
extern uint32_t g_DiskImageCachedOffset;
extern uint32_t g_DiskImageMaxSize; /* Наибольший размер изображения «на диске» — под него заводится общий буфер g_DiskImageBuffer. */
extern uint8_t *g_DiskImageBuffer;
extern int g_SaveVarsOffset;
extern GM_MOVIE_CHUNK g_MovieChunk;
extern uint32_t g_MovieRepeatCount;
extern int g_MovieLoopOffset;
extern uint32_t g_MovieFrameDelay;
extern uint32_t g_MoviePackedBufSize;
extern uint8_t *g_MoviePackedBuf;
extern uint32_t g_MovieFrameBufSize;
extern uint8_t *g_MovieFrameBuf;
extern uint32_t g_MoviePaletteBufSize;
extern uint8_t *g_MoviePaletteBuf;
extern uint32_t g_MovieSoundBufSize;
extern GM_SOUND_DATA *g_MovieSoundBuf;
extern uint32_t g_MovieMusicBufSize;
extern GM_MUSIC_DATA *g_MovieMusicBuf;
extern bool g_MovieSoundPlaying;
extern bool g_MovieMusicPlaying;
extern bool g_MovieLoopEnded;
extern int g_MovieX;
extern int g_MovieY;
extern int g_MovieWidth;
extern int g_MovieHeight;
extern bool g_MovieShowFrames;
extern DWORD g_MovieStartTime;
extern uint32_t g_MovieFramesShown;
extern uint8_t g_MovieSkippedFrames;
extern bool g_MusicActive;
extern uint8_t *g_TextCursor;
extern int g_TextFontIndex;
extern int g_TextX;
extern int g_TextY;
extern GM_FONT *g_TextFont;
extern char g_TextInputBuffer[256];
extern GM_SPRITE *g_TextGlyphSprites[255];
extern int g_TextInputLength;
extern int g_TextInputMaxLength;
extern char *g_TextInputTarget;
extern int8_t g_TextInputMode;
extern bool g_TextInputActive;
extern bool g_TextInputDigitsOnly;
extern GM_OBJECT *g_TextOwnerObject;
extern GM_CLASS *g_TextOwnerClass;
extern int g_TextOwnerIndex;
extern int g_PathOriginX;
extern int g_PathOriginY;
extern int g_PathGoalX;
extern int g_PathGoalY;
extern int g_PathDir8;
extern int g_PathDir4;
extern uint8_t *g_FrameBits;
extern int g_FrameStride;
extern int g_FrameHeight;
extern int g_BlitRowWidth;
extern HWAVEOUT g_hWaveOut;
extern UINT g_WaveOutId;
extern WAVEHDR g_WaveHeaders[8];
extern HMIDIOUT g_hMidiOut;
extern UINT g_MidiOutId;
extern UINT g_MidiTimerId;
extern DWORD g_MidiNextEventTime;
extern DWORD g_MidiInCallback;
extern uint32_t g_MidiLoopDelay;
extern uint32_t g_MidiDelayCounter;
extern uint8_t *g_MidiTrackStart;
extern uint8_t *g_MidiCursor;
extern uint8_t g_MidiRunningStatus;
extern uint8_t g_MidiVolume;
extern UINT g_MidiTimerPeriod;
extern HGLOBAL g_GmwBuffer;
extern GM_GMW_THUNK_FN g_pfnGmwThunk;
extern PFN_UTREGISTER g_pfnUTRegister;
extern PFN_UTUNREGISTER g_pfnUTUnRegister;
extern HMODULE g_hKernel32ForUT;
extern GM_SOUND_CHANNEL g_SoundChannels[8];
extern uint8_t g_WaveBuffers[8][2048];
extern uint32_t g_SoundChannelCount;
extern uint32_t g_NextSoundChannel;
extern int g_ChannelsInLastMix;
extern char g_ExecutablePath[MAX_PATH]; /* В оригинале 64 байта без проверки длины; расширено до MAX_PATH. */
extern GM_WINDOW g_Window;
extern HGDIOBJ g_hDibSection;
extern void *g_DibBits;
extern HDC g_hDibDC;
extern int g_AviX;
extern int g_AviY;
extern int g_AviWidth;
extern int g_AviHeight;
extern char g_AviBasePath[MAX_PATH]; /* В оригинале 64 байта без проверки длины; расширено до MAX_PATH. */
extern MCIDEVICEID g_AviDeviceId;
extern HWND g_hAviWindow;
extern bool g_AviPlaying;
extern bool g_AviQuitRequested;
extern uint8_t g_AviDissolve;
extern bool g_AviActive;
extern bool g_AviPaletteReady;
extern bool g_IsWin3x;
extern bool g_AviWindowPlaced;
extern DWORD g_LastFrameTime;
extern GM_WORLD_HEADER g_World;
extern GM_CLASS *g_CurClass;
extern GM_OBJECT *g_CurObject;
extern int g_CurObjectIndex;
extern int g_CurTileX;
extern int g_CurTileY;
extern int g_RuleTileX;
extern int g_RuleTileY;
extern GM_RULE_XFORM_FN g_pfnRuleXform;
extern GM_PLACE_CELL_FN g_pfnPlaceCell;
extern void *g_UnusedObject;
extern int g_UnusedObjectIndex;
extern GM_POOL_ENTRY *g_PoolTop; /* Вершина пула записей по 16 байт (растёт вниз): экземпляры GM_OBJECT и спрайты GM_SPRITE; номер записи = g_PoolTop − запись. */
extern GM_POOL_ENTRY *g_PoolBottom; /* Нижняя граница занятой части пула. */
extern GM_CLASS *g_Classes; /* Классы мира (GM_CLASS). */
extern GM_CLASS_SET *g_ClassSets; /* Наборы классов (битовые множества GM_CLASS_SET). */
extern GM_TILE *g_TileMap; /* Карта тайлов комнаты (строки по 1 << g_TileMapShift клеток). */
extern int g_TileMapShift; /* Двоичный логарифм длины строки карты тайлов (строка y начинается с клетки y << g_TileMapShift). */
extern GM_SOUND *g_Sounds; /* Звуки мира (GM_SOUND). */
extern GM_GRAPHIC *g_Graphics; /* Графика мира: спрайты и шрифты (GM_GRAPHIC). */
extern int g_MapTileCount; /* Число клеток карты (ширина · высота). */
extern int g_RoomCount;
extern GM_ROOM *g_Rooms; /* Комнаты мира (GM_ROOM). */
extern GM_BACKGROUND *g_Background; /* Фон текущей комнаты (пиксели и палитра). */
extern GM_GAME_HEADER g_GameHeader;
extern GM_ARENA g_Arena;
extern GM_PANEL *g_Panels; /* Панели (GM_PANEL). */
extern const GM_MUSIC_DATA **g_MusicTracks; /* Мелодии мира (указатели на данные собственного формата MIDI). */
extern GM_SETTINGS g_Settings;
extern int g_StartEntry;
extern int g_CurrentRoom;
extern uint8_t *g_ActiveClassFilter;
extern GM_SYSVARS *g_SysVars; /* Системные переменные скриптов (GM_SYSVARS). */
extern GM_VAR_DESC *g_VarsKeepAlways; /* Описатель переменных, сохраняемых всегда (и при автосохранении). */
extern GM_VAR_DESC *g_VarsKeepOnRestart; /* Описатель переменных, сохраняемых при перезапуске игры. */
extern GM_VAR_DESC *g_VarsSaveGameOnly; /* Описатель переменных, сохраняемых только в сохранении игры. */
extern int g_RoomsLoaded;
extern int g_AutosaveSlot;
extern char g_GameFilePath[MAX_PATH]; /* В оригинале 64 байта без проверки длины; расширено до MAX_PATH. */
extern uint32_t *g_MovieOffsets;
extern char *g_WalkMap;
extern int g_MapMaxX;
extern int g_WalkMapRowUp;
extern int g_MapMaxY;
extern uint8_t *g_GlobalVars;
extern uint8_t *g_CurVars;
extern uint32_t g_CurRuleXform;
extern int g_PointerTileY;
extern int g_PointerTileX;
extern int g_ClickTileY;
extern int g_ClickTileX;
extern HINSTANCE g_hAppInstance;
extern HWND g_MainWindow;
extern GM_DIRECTION g_CurDirection;
extern bool g_LoadingSave;
extern bool g_BuildingRoom; /* Идёт построение начального состояния комнаты: звуки и музыка из скриптов в это время не запускаются. */
extern bool g_FirstRun;
extern uint8_t g_TargetSoundVolume;
extern uint8_t g_TargetMusicVolume;
extern bool g_SpriteSetThisStep;
extern uint8_t g_ClickTileClass;
extern GM_EVENT g_ClickEvent;
extern bool g_ContinueRules;
extern uint8_t g_InputChar;
extern uint8_t g_MovieDepth;
extern bool g_TextPending;
extern uint8_t g_PointerTileClass;
extern bool g_PathFound;
extern GM_EVENT g_PointerEvent;
extern uint8_t g_SystemSoundVolume;
extern uint8_t g_SystemMusicVolume;
extern bool g_GameRunning;

/* ========================================================================== */
/* Функции                                                                    */
/* ========================================================================== */

void GmXform_Identity(GM_RULE_CELL *cell);
void GmXform_RotateCW(GM_RULE_CELL *cell);
void GmXform_Rotate180(GM_RULE_CELL *cell);
void GmXform_RotateCCW(GM_RULE_CELL *cell);
void GmXform_MirrorLeftRight(GM_RULE_CELL *cell);
void GmXform_MirrorSwNeAxis(GM_RULE_CELL *cell);
void GmXform_MirrorUpDown(GM_RULE_CELL *cell);
void GmXform_MirrorNwSeAxis(GM_RULE_CELL *cell);
void GmXform_NoneForMixedDirection(GM_RULE_CELL *cell);
void GmResolveDir_None(GM_RULE_CELL *cell);
void GmResolveDir_Single(GM_RULE_CELL *cell);
void GmResolveDir_S_W(GM_RULE_CELL *cell);
void GmResolveDir_S_N(GM_RULE_CELL *cell);
void GmResolveDir_W_N(GM_RULE_CELL *cell);
void GmResolveDir_S_W_N(GM_RULE_CELL *cell);
void GmResolveDir_S_E(GM_RULE_CELL *cell);
void GmResolveDir_W_E(GM_RULE_CELL *cell);
void GmResolveDir_S_W_E(GM_RULE_CELL *cell);
void GmResolveDir_N_E(GM_RULE_CELL *cell);
void GmResolveDir_S_N_E(GM_RULE_CELL *cell);
void GmResolveDir_W_N_E(GM_RULE_CELL *cell);
void GmResolveDir_Any(GM_RULE_CELL *cell);
void GmSprite_Destroy(GM_SPRITE *sprite);
void GmObject_RemoveText(GM_OBJECT *obj);
void GmPool_DestroySpriteByIndex(int index);
void GmRunScriptWithContext(uint8_t *script, GM_CLASS *cls, GM_OBJECT *obj, int objectIndex, uint8_t *vars, uint8_t tileX, uint8_t tileY, uint8_t direction);
void GmFocus_Advance(GM_OBJECT *obj);
void GmCurObject_DestroyUnderTile(void);
void GmTile_Clear(int tileX, int tileY, bool alsoFreeSprites);
void GmCurObject_SetUnderTile(int classOrSet, bool fromClassSet);
void GmTile_Place(GM_RULE_CELL cell, int tileX, int tileY);
void GmCurObject_Move(GM_RULE_CELL step);
bool GmSysVars_Update(void);
void GmInput_Dispatch(int mouseX, int mouseY, int clickX, int clickY, GM_EVENT event, uint8_t actionKey);
int GmStepToward(uint8_t *current, uint8_t target);
void GmVolume_FadeStep(void);
bool GmSprite_Animate(GM_SPRITE *sprite, bool forceEnd, bool freeWhenDone);
bool GmRules_RunTick(void);
bool GmTick_TextMode(GM_EVENT event);
bool GmCursor_Update(int mouseX, int mouseY);
bool GmCursor_SetGraphic(int graphicIndex, bool redraw);
bool GmRender_Flush(void);
bool GmGame_Init(void);
void GmGame_ClearRunning(void);
void GmTick_InitState(void);
void GmRoom_ResetView(bool blitRects);
NORETURN void GmGame_Restart(void);
int GmPlaceCell_Absolute(GM_RULE_CELL cell);
int GmPlaceCell_Relative(GM_RULE_CELL cell);
int GmRule_Run(GM_RULE *rule, bool absolute);
bool GmGame_Initialize(char *gameFilePath);
bool GmGame_Tick(int clipWidth, int clipHeight, int mouseX, int mouseY, int clickX, int clickY, GM_EVENT event, uint8_t actionKey, uint8_t inputChar, bool cursorInWindow);
void GmDirty_Add(int left, int top, int right, int bottom);
bool GmRoom_RepaintAll(void);
void GmCursor_OnMouseMove(uint16_t mouseX, uint16_t mouseY);
void GmGame_Shutdown(void);
void *GmError_OutOfMemory(void);
void GmHeap_ClearState(void);
uint32_t *GmHeap_Init(void);
uint8_t *GmHeap_Reserve(uint32_t size);
void *GmHeap_Alloc(uint32_t size);
void GmHeap_Free(void *block);
void *GmLoad_AllocRoomScratch(void);
void GmLoad_FreeRoomScratch(void);
uint8_t *GmTempBuffer(uint32_t size);
void GmPool_SetBottom(GM_POOL_ENTRY *bottom);
void GmPool_Create(void);
GM_POOL_ENTRY *GmPool_Alloc(void);
void GmPool_Free(GM_POOL_ENTRY *entry);
void GmPool_Reset(void);
uint32_t GmRead_SetBuffer(void *buffer);
uint32_t GmRead_Tell(void);
bool GmRead_Seek(int offset, int origin);
bool GmRead_Bytes(void *dst, uint32_t size);
bool GmLoad_Fail(void);
bool GmRead_Tag(void);
bool GmLoad_OpenResource(uint32_t resourceId);
bool GmRead_Number(int *value);
bool GmRead_SkipBlock(uint32_t *blockStart);
bool GmRead_Record(void *dst);
bool GmLoad_OpenGameFile(char *path);
void GmRoom_SaveState(int roomIndex);
void GmLoad_CopyRecord(void *dst);
bool GmLoad_GameHeader(void);
void GmLoad_WorldHeader(void);
bool GmLoad_ApplyRecord(GM_RECORD_TYPE type, int slot, int index1, int index2, int dataOffset, void *data);
bool GmLoad_Run(int startEntry);
bool GmRoom_Enter(int roomIndex, bool keepCurrent);
uint8_t *GmImage_GetPixels(GM_IMAGE *image);
void GmLoad_CloseGameFile(void);
void GmSave_Xor(uint8_t *data, int size, int varOffset);
bool GmSave_ReadVars(GM_VAR_DESC *desc);
bool GmSave_WriteVars(GM_VAR_DESC *desc);
void GmSave_ClearVars(GM_VAR_DESC *desc);
bool GmSave_ReadSettings(void);
bool GmSave_WriteHeader(void);
bool GmSave_WriteInt(uint32_t value);
void GmSave_RebaseSpriteFrames(GM_POOL_ENTRY *records, int count, int delta);
bool GmAutosave_Load(int slot);
bool GmAutosave_Write(int slot);
bool GmSaveGame_Delete(int slot);
bool GmSaveGame_Load(int slot);
bool GmSaveGame_Save(int slot);
bool GmMovie_Begin(int fileOffset);
void GmHeap_FreeIfSet(void *block);
bool GmMovie_End(void);
bool GmMovie_Fail(void);
bool GmMovie_ReadChunkData(void *dst, uint32_t bufferSize);
GM_MOVIE_STEP GmMovie_HeaderChunk(void);
GM_MOVIE_STEP GmMovie_FrameChunk(void);
bool GmMovie_PaletteChunk(void);
bool GmMovie_SoundChunk(void);
bool GmMovie_MusicChunk(void);
bool GmMovie_Play(int fileOffset);
int GmGame_PlayStartMovie(void);
bool GmMusic_Start(const GM_MUSIC_DATA *music);
void GmMusic_Stop(void);
int Native_ContinueRules(void);
int Native_HasNoSprite(void);
int Native_RestGraphicIs(int graphicIndex);
int Native_IsHovered(void);
int Native_IsPressed(void);
int Native_MouseInputIs(int bits);
int Native_KeyInputIs(int bits);
int GmDirIndex_MatchesRotated(int dir, int baseDir);
int Native_IsDirectionKey(uint8_t dirAndShift);
int Native_LayerIs(int layer);
int GmRule_RunWithSavedContext(GM_RULE **rulePtr);
int Native_RunPanel(int panelIndex);
int Native_IsUnderTileEmpty(void);
int Native_UnderTileIs(int classIndex);
int Native_UnderTileInSet(int classSet);
int Native_InputCharIn(const uint8_t *chars);
NORETURN void Native_RestartGame(int startEntry);
NORETURN void Native_GotoRoom(int roomIndex);
void Native_UnreferencedNop(int unused);
int Native_PlayMusic(int track);
int Native_PlaySound(int soundIndex);
int Native_PlayMovie(int movie);
int GmSprite_Create(int graphicIndex, uint16_t x, uint16_t y, bool screenAnchored);
int Native_ShowGraphic(int graphicIndex);
int Native_ShowPanel(int panelIndex);
GM_SPRITE *GmText_PutGlyph(int character, int fontIndex, int *x, uint16_t y);
uint8_t *GmText_Begin(uint8_t *text);
void GmText_EraseChars(int count);
uint8_t GmText_Process(uint8_t key);
int GmText_StartInput(uint8_t *text, int fontIndex, int x, int y);
int Native_InputTextAtTile(int fontIndex, uint8_t *text);
int Native_InputTextAtPanel(int panelIndex, uint8_t *text);
int GmText_Print(uint8_t *text, int fontIndex, int x, uint16_t y);
int Native_PrintAtTile(int fontIndex, uint8_t *text);
int Native_PrintAtPanel(int panelIndex, uint8_t *text);
int Native_SetLayer(uint8_t layer);
int Native_RemoveText(void);
int Native_DestroyUnderTile(void);
int Native_SetUnderTile(int classIndex);
int Native_SetUnderTileFromSet(int classSet);
void Native_RemoveSprite(void);
int Native_SetCursor(int graphicIndex);
int Native_ResetCursor(void);
int Native_LayerFromRow(void);
int Native_GetLayer(uint8_t *layer);
int GmPath_Setup(GM_EVENT target);
int GmPath_DirectDirection(void);
int GmPath_Trace(uint8_t lastWave);
uint8_t GmPath_ExpandWave4From6(void);
uint8_t GmPath_ExpandWave5From4(void);
uint8_t GmPath_ExpandWave6From5(void);
int GmPath_Search(bool directionOnly);
int GmWalkMap_GoalsClassNoObstacles(uint8_t classIndex);
int GmWalkMap_GoalsClassEmptyFree(uint8_t classIndex);
int GmWalkMap_EmptyFree(void);
int GmWalkMap_ClassFree(uint8_t classIndex);
int GmPath_SeekClass(GM_EVENT target, uint8_t classIndex, uint8_t mode);
int GmWalkMap_GoalsInSetNoObstacles(const uint8_t *memberBits);
int GmWalkMap_GoalsInSetEmptyFree(const uint8_t *memberBits);
int GmWalkMap_SetFree(const uint8_t *memberBits);
int GmPath_SeekClassSet(GM_EVENT target, uint8_t classSet, uint8_t mode);
int Native_SeekPointer(GM_SEEK_COMMAND mode);
int Native_SeekClass(uint8_t classIndex, GM_SEEK_COMMAND mode);
int Native_SeekClassSet(uint8_t classSet, GM_SEEK_COMMAND mode);
int Native_PathDir4Is(int dir);
int Native_PathDir8Is(int dir);
int Native_PathDir4IsRotated(int baseDir);
int Native_PathDir8IsRotated(int baseDir);
int Native_PathAction(GM_PATH_ACTION action);
int Native_PlaceAtGoal(uint8_t classIndex);
int Native_PlaceAtGoalFromSet(uint8_t classSet);
int Native_ObjectFlagsAny(uint8_t mask);
char *Native_StrCopy(char *dst, char *src);
int Native_GetOption(GM_OPTION_QUERY which);
int Native_SetOption(GM_OPTION_SET which);
int Native_SaveGameOp(int slot, GM_SAVE_OPERATION action);
int Native_SetClassFilter(int classSet);
int Native_ClearClassFilter(void);
int Native_ShowHelp(const char *topic);
int Native_SetDirKey(const uint8_t *virtualKey, int index);
uint32_t Native_Random(int range);
void GmAudio_StopAndShutdown(void);
void GmAudio_Restart(void);
int Native_PlayAvi(const char *args);
int Native_RunProgram(char *commandLine);
void GmSprite_UpdatePosition(GM_SPRITE *sprite);
void GmSprite_Invalidate(GM_SPRITE *sprite);
void GmSprite_InvalidateByIndex(int index);
void GmSprite_SetDirectionalFrames(GM_GRAPHIC *graphic, GM_SPRITE *sprite, bool screenAnchored);
GM_GRAPHIC *GmSprite_FindGraphic(GM_SPRITE *sprite);
void GmObject_RefreshSprite(GM_OBJECT *obj);
bool GmObject_HitTest(GM_OBJECT *obj, int x, int y);
void *GmMemZero(void *buffer, uint32_t size);
int GmVm_Run(const uint8_t *code);
void GmVm_ResetStack(void);
bool GmFrameModule_Startup(void);
void GmFrameModule_Shutdown(void);
bool GmFrame_Init(GM_ARENA *arena, GM_GAME_HEADER *header);
void GmFrame_CopyRect(uint8_t *src, int srcOriginX, int srcOriginY, int srcStride, int srcHeight, int left, int top, int right, int bottom);
void GmFrame_DrawSprite(uint8_t *pixels, int skipX, int skipY, int width, int height, int left, int top, int right, int bottom);
void GmFrame_DrawSpriteMirrorY(uint8_t *pixels, int skipX, int skipY, int width, int height, int left, int top, int right, int bottom);
void GmFrame_DrawSpriteMirrorX(uint8_t *pixels, int skipX, int skipY, int width, int height, int left, int top, int right, int bottom);
void GmFrame_DrawSpriteMirrorXY(uint8_t *pixels, int skipX, int skipY, int width, int height, int left, int top, int right, int bottom);
void GmFrame_Fill(uint8_t color);
const uint8_t *GmAddBlit_Raw(uint8_t *dst, const uint8_t *src, int width, int height, int rowStep);
const uint8_t *GmAddBlit_RleShortSkip(uint8_t *dst, const uint8_t *src, int width, int height, int rowStep);
const uint8_t *GmAddBlit_RleShortSpecial(uint8_t *dst, const uint8_t *src, int width, int height, int rowStep);
const uint8_t *GmAddBlit_RleShortCopy(uint8_t *dst, const uint8_t *src, int width, int height, int rowStep);
int GmMovie_BlitDelta(uint8_t *src, int codec, int x, int y, int width, int height);
void GmLzss_Unpack(const uint8_t *src, uint8_t *dst, int packedSize);
void GmFrame_Present(int left, int top, int right, int bottom, bool dissolve);
void GmBlitModule_Shutdown(void);
int GmWave_Close(void);
bool GmWave_Open(uint8_t *buffers);
int GmWave_FindFreeBuffer(void);
int GmWave_Submit(void);
int GmWave_Reset(void);
int GmWave_Pause(void);
int GmWave_Resume(void);
int GmWave_SetVolume(uint8_t volume);
int GmMidi_Halt(void);
int GmMidi_ReadDelta(void);
void GmMidi_Pump(void);
void CALLBACK GmMidi_TimerProc(UINT timerId, UINT msg, DWORD_PTR user, DWORD_PTR reserved1, DWORD_PTR reserved2);
int GmMidi_Close(void);
bool GmMidi_Open(void);
int GmMidi_Play(const GM_MUSIC_DATA *music);
int GmMidi_Stop(void);
int GmMidi_SetVolume(uint8_t volume);
void GmAudioStub_Shutdown(void);
int GmAudioStub_ReturnTrue(void);
int GmAudioStub_SetVolume(uint8_t volume);
int GmAudioStub_PlayMusic(const GM_MUSIC_DATA *music);
void GmGmw_Close(void);
int GmGmw_Submit(void);
int GmGmw_Reset(void);
int GmGmw_GetSoundVolume(void);
int GmGmw_NextBuffer(void);
int GmGmw_Pause(void);
int GmGmw_Resume(void);
int GmGmw_SetSoundVolume(uint8_t volume);
int GmGmw_PlayMusic(const GM_MUSIC_DATA *music);
int GmGmw_StopMusic(void);
int GmGmw_GetMusicVolume(void);
int GmGmw_SetMusicVolume(uint8_t volume);
int GmAudio_OpenGmw(uint8_t *buffers);
void GmAudio_CloseWinMM(void);
int GmAudio_OpenWinMM(uint8_t *buffers);
void GmMix_AddSaturated(uint32_t *dst, const uint32_t *src, uint32_t size);
bool GmAudio_Init(uint8_t channelCount, int unused, uint8_t *soundVolume, uint8_t *musicVolume);
int GmAudio_PlayMusic(const GM_MUSIC_DATA *music, int unusedMinusOne, int unusedZero);
bool GmAudio_PlaySound(const GM_SOUND_DATA *sound, uint8_t unreadFF, uint8_t unreadZero, int8_t mode);
int GmAudio_SetMusicVolume(uint8_t volume);
int GmAudio_SetSoundVolume(uint8_t volume);
int GmAudio_StopMusic(void);
int GmAudio_StopSounds(void);
int GmAudio_Mix(void);
int GmAudio_Pause(void);
int GmAudio_Resume(void);
void GmAudio_Shutdown(void);
int Host_MessageBox(LPCSTR caption, LPCSTR text);
int Host_ErrorBox(LPCSTR text);
void Host_ShowPendingError(void);
char *Host_SetHelpPath(const char *exePath);
void Host_CloseWindow(void);
void Host_Shutdown(void);
HCURSOR Host_GetCursor(void);
bool Host_MciCheck(MCIERROR error);
bool Host_MciSend(UINT command, DWORD flags, DWORD_PTR params);
bool Host_MciSendNoParams(UINT command, DWORD_PTR flags);
bool Host_AviPlay(void);
LRESULT CALLBACK Host_AviWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool Host_TakeDriveOfType(char *rootOut, uint8_t *driveTypes, uint8_t driveType);
bool Host_NextSearchDrive(char *rootOut, uint8_t *driveTypes);
bool Host_FindAviOnDrives(char *aviPath, char *bmpPath, const char *name);
bool Host_FindAviNextToExe(char *aviPath, char *bmpPath, const char *name);
int Host_LoadAviBackdrop(const char *bmpPath);
bool Host_AviOpen(void);
int Host_DrainMessages(void);
bool Host_PlayAvi(const char *name, int x, int y, int width, int height);
LRESULT CALLBACK Host_MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool Host_ShowWindowOrIntro(void);
HDC Host_CreateWindow(GM_GAME_HEADER *header);
bool Host_InitVideo(GM_ARENA *arena, GM_GAME_HEADER *header, GM_SETTINGS *settings);
bool Host_SetFrameRate(uint8_t frameRate);
bool Host_SetPalette(const uint8_t *rgb, int count, char gridColor, bool keepSystemColors);
int Host_BlitRect(void *bits, int x, int y, int width, int height, int frameHeight);
bool Host_Present(void *bits, int x, int y, int width, int height, int frameStride, int frameHeight, bool dissolve);
int Host_Flush(void);
GM_EVENT Host_PumpMessages(DWORD *now);
int Host_ShowHelp(const char *topic);
bool Host_RunProgram(LPSTR commandLine);
bool Host_SetError(const char *message);
bool Host_OpenDataFile(const char *path);
int Host_DataTell(void);
bool Host_DataSeek(int offset, int origin);
bool Host_DataRead(void *dst, uint32_t size);
int Host_DataReadSome(void *dst, uint32_t size);
int Host_CloseDataFile(void);
int Host_BuildSavePath(LPSTR out, const char *exePath, const char *ext, int slot);
int Host_BuildTempSavePath(LPSTR out, const char *path);
bool Host_OpenSave(const char *exePath, const char *ext, int slot);
bool Host_DeleteSave(const char *exePath, const char *ext, int slot);
bool Host_SaveRead(void *dst, uint32_t size);
bool Host_SaveWrite(const void *src, uint32_t size);
int Host_SaveTell(void);
bool Host_SaveSeek(int offset);
int Host_SaveTruncate(void);
int Host_CloseSave(void);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);
int GmWave_GetVolume(void);
int GmMidi_GetVolume(void);

#endif /* GM_H */
