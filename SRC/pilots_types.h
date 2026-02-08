/*
 * PILOTS.EXE type definitions.
 * Extracted from IDA Pro local type library.
 * Fixed for MSVC compilation.
 */
#ifndef PILOTS_TYPES_H
#define PILOTS_TYPES_H

#pragma once

#include <windows.h>
#include <mmsystem.h>
#include "defs.h"

/* ---- Forward declarations ---- */
struct VideoConfigEx;
struct VideoDataBuffer;
struct InputConfig;

/* ---- Basic types ---- */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef signed char        int8_t;

/* ---- Structures ---- */

#pragma pack(push, 2)

struct SpriteDesc
{
    uint8_t spriteIndex;
    uint8_t dirFlags;
    int8_t dx;
    int8_t dy;
};

struct RenderObject
{
    unsigned char flags;
    unsigned char slotIndex;
    unsigned short renderInfo;
    unsigned short animLink;
    unsigned short parentIndex;
    unsigned short posX;
    unsigned short posY;
    void *spriteDataPtr;
};

struct DirtyRegion
{
    int x;
    int y;
    int width;
    int height;
    struct DirtyRegion *next;
};

struct SpriteSlot
{
    void *baseData;
    void *dataPtr;
    void **subObjectArray;
    void *scriptPtr;
};

struct AnimationSlot
{
    void *frameBitmask;
    void *frameSelectionData;
    void *frameModificationData;
    uint32_t reserved;
};

struct FontSlot
{
    uint8_t flags;
    uint8_t baseCharOffset;
    uint8_t type;
    uint8_t attributes;
    void *fontData;
};

struct SoundSlot
{
    uint32_t flags;
    void *soundDataPtr;
};

struct OverlayCoords
{
    uint16_t x;
    uint16_t y;
    uint16_t index;
    uint8_t reserved[2];
    uint8_t isLast;
};

struct OverlaySlot
{
    int *spriteDataPtr;
    struct OverlayCoords *coords;
};

struct ScreenSnapshot
{
    void *compressedData;
    void *sceneDataPtr;
    void *decompressedData;
    int objectCount;
};

struct MusicTrackSlot
{
    void *trackData;
    void *reserved;
};

struct DeferredLoadInfo
{
    uint32_t firstDword;
    uint32_t magicDisk;
    uint32_t fileOffset;
    uint32_t decompressedSize;
};

struct GameLoopData
{
    int flags_or_id;
    unsigned char frame_rate;
    unsigned char update_flag;
    unsigned char video_recursion_counter;
    int frame_counter;
};

struct MemoryPoolDescriptor
{
    int poolSizeBytes;
    int poolBasePtr;
    int frameDataSize;
    void *allocBuffer;
};

#pragma pack(pop)

/* ---- InputConfig ---- */
#pragma pack(push, 2)
struct InputConfig
{
    char padding[4];
    unsigned char flags;
    char padding2[35];
    unsigned char key0;
    unsigned char key1;
    unsigned char key2;
    unsigned char key3;
    unsigned char key4;
    unsigned char key5;
    unsigned char key6;
    unsigned char key7;
    unsigned char specialKey0;
    unsigned char specialKey1;
    unsigned char specialKey2;
    unsigned char specialKey3;
};
#pragma pack(pop)

/* ---- VideoConfigEx ---- */
#pragma pack(push, 2)
struct VideoConfigEx
{
    int field_0;
    int field_4;
    int field_8;
    int width;
    int height;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
    int field_24;
    int videoWindowHeight;
    int videoWindowWidth;
    int videoWindowY;
    int videoWindowX;
};
#pragma pack(pop)

/* ---- VideoDataBuffer ---- */
#pragma pack(push, 2)
struct VideoDataBuffer
{
    int field_0;
    int field_4;
    int size;
    void *buffer;
};
#pragma pack(pop)

/* ---- VideoConfig (main config struct) ---- */
#pragma pack(push, 2)
struct VideoConfig
{
    struct VideoConfigEx *pConfigEx;
    struct VideoDataBuffer *pDataBuffer;
    struct InputConfig *pInputConfig;
    HDC hDC;
    int hPalette;
    HGLOBAL hGlobalMem;
    void *pLockedMem;
    int nShowCmd;
    const char *pErrorMessage;
    int frameDelayMs;
    int field_28;
    int clientWidth;
    int clientHeight;
    int initialClientWidth;
    int initialClientHeight;
    int screenResX;
    int screenResY;
    int borderWidth;
    int titleBarHeight;
    int cursorX;
    int cursorY;
    int clickX;
    int clickY;
    char eventCode;
    char keyIndex;
    char charCode;
    char _pad5F;
    int dataFileHandle;
    int saveFileHandle;
    char helpFilePath[256];
};
#pragma pack(pop)

/* ---- Enums ---- */

enum GameOpcode
{
    OPCODE_SET_FRAMERATE = 0x12,
    OPCODE_UNKNOWN_19 = 0x13,
    OPCODE_INIT_SCREEN_SNAPSHOT = 0x18,
    OPCODE_CAPTURE_SNAPSHOT = 0x19,
    OPCODE_INIT_SPRITE_SLOT = 0x20,
    OPCODE_SET_SPRITE_DATA_PTR = 0x21,
    OPCODE_SET_SPRITE_SCRIPT_PTR = 0x22,
    OPCODE_SET_SPRITE_SUBOBJECT_ARRAY = 0x23,
    OPCODE_SET_SUBOBJECT_PTR = 0x2A,
    OPCODE_SET_SUBOBJECT_DATA_PTR = 0x2B,
    OPCODE_SET_SUBOBJECT_SCRIPT_PTR = 0x2C,
    OPCODE_SET_ANIMATION_FRAME_BITMASK = 0x38,
    OPCODE_SET_ANIMATION_FRAME_SELECTION = 0x39,
    OPCODE_SET_ANIMATION_FRAME_MODIFICATION = 0x3A,
    OPCODE_INIT_FONT_SLOT = 0x40,
    OPCODE_SET_FONT_DATA = 0x41,
    OPCODE_SET_FONT_DATA_OR_ARRAY = 0x42,
    OPCODE_SET_FONT_GLYPH_DATA = 0x43,
    OPCODE_INIT_SOUND_SLOT = 0x50,
    OPCODE_SET_SOUND_DATA = 0x51,
    OPCODE_SET_MUSIC_TRACK_DATA = 0x52,
    OPCODE_SET_OVERLAY_SPRITE_DATA = 0x60,
    OPCODE_SET_OVERLAY_COORDS = 0x61,
    OPCODE_SET_GLOBAL_PTR_1 = 0x7C,
    OPCODE_SET_GLOBAL_PTR_2 = 0x7D,
    OPCODE_SET_GLOBAL_PTR_3 = 0x7E,
    OPCODE_DECODE_ARG1 = 0x1,
    OPCODE_DECODE_ARG2 = 0x2,
    OPCODE_DECODE_ARG3 = 0x3,
    OPCODE_SPECIAL_PROCESSING = 0x4,
    OPCODE_READ_JUMP_OFFSET = 0x5,
    OPCODE_READ_TEXT_DATA = 0xF,
    OPCODE_PROCESS_SPRITE_ANIMATION = 0x10,
    OPCODE_UPDATE_FRAME = 0x11,
    OPCODE_CONDITIONAL_JUMP = 0x14,
    OPCODE_EXECUTE_COMMAND = 0xFF
};

enum ScriptOpcode
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
    OP_SKIP4 = 0xD,
    OP_STORE_BYTE_GLOBAL = 0xE,
    OP_STORE_BYTE_OBJECT = 0xF,
    OP_STORE_DWORD_GLOBAL = 0x10,
    OP_STORE_DWORD_OBJECT = 0x11,
    OP_RET_CALL = 0x12,
    OP_RET_CALL_SKIP4 = 0x13,
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
    OP_CALL_INDIRECT = 0x37
};

enum FmtDataType
{
    FMT_SBYTE = 0x0,
    FMT_STRPTR = 0x1,
    FMT_INLINE_STR = 0x2,
    FMT_DWORD_INT = 0x3,
    FMT_DEREF_DWORD = 0x4,
    FMT_SCRIPT_EVAL = 0x5
};

/* ---- C typedef aliases for struct names ---- */
/* These allow using struct names without the 'struct' prefix in C mode */
typedef struct SpriteDesc SpriteDesc;
typedef struct RenderObject RenderObject;
typedef struct DirtyRegion DirtyRegion;
typedef struct SpriteSlot SpriteSlot;
typedef struct AnimationSlot AnimationSlot;
typedef struct FontSlot FontSlot;
typedef struct SoundSlot SoundSlot;
typedef struct OverlayCoords OverlayCoords;
typedef struct OverlaySlot OverlaySlot;
typedef struct ScreenSnapshot ScreenSnapshot;
typedef struct MusicTrackSlot MusicTrackSlot;
typedef struct DeferredLoadInfo DeferredLoadInfo;
typedef struct GameLoopData GameLoopData;
typedef struct MemoryPoolDescriptor MemoryPoolDescriptor;
typedef struct InputConfig InputConfig;
typedef struct VideoConfigEx VideoConfigEx;
typedef struct VideoDataBuffer VideoDataBuffer;
typedef struct VideoConfig VideoConfig;

/* ---- Function pointer types ---- */
typedef void (__stdcall *MoveDirectionHandler)(int param);
typedef void (__stdcall *SpriteTransformHandler)(SpriteDesc *desc);

/* ---- Blit function type (register-based calling convention) ---- */
/* The original uses __userpurge with register params.
   We redefine these as __cdecl with explicit parameters. */
typedef unsigned char * (*BlitFunc_t)(int unused_eax, int width, int height,
                                      int strideAdjust, unsigned char *dst,
                                      unsigned char *src);

#endif /* PILOTS_TYPES_H */
