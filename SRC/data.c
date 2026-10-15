/*
 * Глобальное состояние движка и постоянные таблицы.
 * Объявления и описания — в gm.h; таблица встроенных функций — в natives.c.
 */
#include "gm.h"

/* Спрайт курсора игры: занят, видим, экранный (GMSPR_USED | GMSPR_SCREEN_ANCHORED | GMSPR_VISIBLE), без владельца. */
GM_SPRITE g_CursorSprite = { .flags = GMSPR_USED | GMSPR_SCREEN_ANCHORED | GMSPR_VISIBLE, .ownerIndex = GM_POOL_NONE };
uint8_t g_XformDirMap_RotateCW[16] = { 0x00, 0x20, 0x40, 0x60, 0x80, 0xA0, 0xC0, 0xE0, 0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0 };
uint8_t g_XformDirMap_Rotate180[16] = { 0x00, 0x40, 0x80, 0xC0, 0x10, 0x50, 0x90, 0xD0, 0x20, 0x60, 0xA0, 0xE0, 0x30, 0x70, 0xB0, 0xF0 };
uint8_t g_XformDirMap_RotateCCW[16] = { 0x00, 0x80, 0x10, 0x90, 0x20, 0xA0, 0x30, 0xB0, 0x40, 0xC0, 0x50, 0xD0, 0x60, 0xE0, 0x70, 0xF0 };
uint8_t g_XformDirMap_MirrorLeftRight[16] = { 0x00, 0x10, 0x80, 0x90, 0x40, 0x50, 0xC0, 0xD0, 0x20, 0x30, 0xA0, 0xB0, 0x60, 0x70, 0xE0, 0xF0 };
uint8_t g_XformDirMap_MirrorSwNeAxis[16] = { 0x00, 0x20, 0x10, 0x30, 0x80, 0xA0, 0x90, 0xB0, 0x40, 0x60, 0x50, 0x70, 0xC0, 0xE0, 0xD0, 0xF0 };
uint8_t g_XformDirMap_MirrorUpDown[16] = { 0x00, 0x40, 0x20, 0x60, 0x10, 0x50, 0x30, 0x70, 0x80, 0xC0, 0xA0, 0xE0, 0x90, 0xD0, 0xB0, 0xF0 };
uint8_t g_XformDirMap_MirrorNwSeAxis[16] = { 0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0 };
GM_RULE_XFORM_FN g_RuleXforms[8] =
{
    GmXform_Identity, GmXform_RotateCW, GmXform_Rotate180, GmXform_RotateCCW,
    GmXform_MirrorLeftRight, GmXform_MirrorSwNeAxis, GmXform_MirrorUpDown, GmXform_MirrorNwSeAxis,
};
/* Поворот клетки правила по направлению объекта (индекс — разряды направления >> 4):
   ровно одно направление — соответствующий поворот, иначе преобразования нет. */
GM_RULE_XFORM_FN g_RotateCellByDirection[16] =
{
    GmXform_NoneForMixedDirection, GmXform_Identity, GmXform_RotateCW, GmXform_NoneForMixedDirection,
    GmXform_Rotate180, GmXform_NoneForMixedDirection, GmXform_NoneForMixedDirection, GmXform_NoneForMixedDirection,
    GmXform_RotateCCW, GmXform_NoneForMixedDirection, GmXform_NoneForMixedDirection, GmXform_NoneForMixedDirection,
    GmXform_NoneForMixedDirection, GmXform_NoneForMixedDirection, GmXform_NoneForMixedDirection, GmXform_NoneForMixedDirection,
};
uint8_t g_DirChoices_S_W[2] = { 0x10, 0x20 };
uint8_t g_DirChoices_S_N[2] = { 0x10, 0x40 };
uint8_t g_DirChoices_W_N[2] = { 0x20, 0x40 };
uint8_t g_DirChoices_S_W_N[3] = { 0x10, 0x20, 0x40 };
uint8_t g_DirChoices_S_E[2] = { 0x10, 0x80 };
uint8_t g_DirChoices_W_E[2] = { 0x20, 0x80 };
uint8_t g_DirChoices_S_W_E[3] = { 0x10, 0x20, 0x80 };
uint8_t g_DirChoices_N_E[2] = { 0x40, 0x80 };
uint8_t g_DirChoices_S_N_E[3] = { 0x10, 0x40, 0x80 };
uint8_t g_DirChoices_W_N_E[3] = { 0x20, 0x40, 0x80 };
uint8_t g_DirChoices_All[4] = { 0x10, 0x20, 0x40, 0x80 };
GM_RULE_XFORM_FN g_ResolveDirectionMask[16] =
{
    GmResolveDir_None, GmResolveDir_Single, GmResolveDir_Single, GmResolveDir_S_W,
    GmResolveDir_Single, GmResolveDir_S_N, GmResolveDir_W_N, GmResolveDir_S_W_N,
    GmResolveDir_Single, GmResolveDir_S_E, GmResolveDir_W_E, GmResolveDir_S_W_E,
    GmResolveDir_N_E, GmResolveDir_S_N_E, GmResolveDir_W_N_E, GmResolveDir_Any,
};
/* Состояние линейного конгруэнтного генератора (seed·1103515245 + 12345). */
uint32_t g_RandomSeed = 1;
char s_InsufficientMemory[21] = "Insufficient memory.";
char s_InvalidGameData[24] = "Invalid game data file.";
GM_RECORD_SIZES g_RecordSizesZero;
/* Длина строки карты тайлов — степень двойки не меньше ширины мира: { наибольшая ширина, сдвиг }. */
GM_ROW_SHIFT g_MapRowShifts[7] = { { 4, 2 }, { 8, 3 }, { 16, 4 }, { 32, 5 }, { 64, 6 }, { 128, 7 }, { 256, 8 } };
uint8_t g_SaveXorKey[32] = { 0xA7, 0x15, 0xF0, 0x56, 0xF3, 0xFA, 0x84, 0x2C, 0xFD, 0x81, 0x38, 0xAC, 0x73, 0xD2, 0x22, 0x47, 0xA0, 0x12, 0xB8, 0x19, 0x20, 0x6A, 0x26, 0x7C, 0x32, 0x57, 0xDD, 0xB2, 0x38, 0xA7, 0x95, 0x7A };
/* Магия файла сохранения: '=SV=' и версия формата. */
uint32_t g_SaveMagic[2] = { 0x3D53563Du /* '=VS=' в памяти: "=VS=" */, GM_FILE_VERSION_CURRENT };
char g_SaveGameExt[4] = "SAV";
char s_MoviePlaybackError[22] = "Movie playback error.";
uint8_t g_DirIndexMirror[8] = { 0x00, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
char s_PlayAviArgsFormat[15] = "%s %d %d %d %d";
/* Добавочные блиттеры кадров фильма по номеру кодека. */
const GM_ADD_BLIT_FN g_AddBlitters[4] = { GmAddBlit_Raw, GmAddBlit_RleShortSkip, GmAddBlit_RleShortSpecial, GmAddBlit_RleShortCopy };
/* Обработчики звука. До выбора драйвера (GmAudio_Init) — заглушки. */
GM_AUDIO_FN g_pfnAudioSubmit = GmAudioStub_ReturnTrue;
GM_MUSIC_PLAY_FN g_pfnMusicPlay = GmAudioStub_PlayMusic;
GM_AUDIO_FN g_pfnAudioReset = GmAudioStub_ReturnTrue;
GM_AUDIO_FN g_pfnMusicStop = GmAudioStub_ReturnTrue;
GM_AUDIO_FN g_pfnGetSoundVolume = GmAudioStub_ReturnTrue;
GM_AUDIO_FN g_pfnGetMusicVolume = GmAudioStub_ReturnTrue;
GM_AUDIO_FN g_pfnAudioNextBuffer = GmAudioStub_ReturnTrue;
GM_AUDIO_FN g_pfnAudioPause = GmAudioStub_ReturnTrue;
GM_AUDIO_FN g_pfnAudioResume = GmAudioStub_ReturnTrue;
GM_AUDIO_VOLUME_FN g_pfnSetSoundVolume = GmAudioStub_SetVolume;
GM_AUDIO_VOLUME_FN g_pfnSetMusicVolume = GmAudioStub_SetVolume;
GM_AUDIO_VOID_FN g_pfnAudioShutdown = GmAudioStub_Shutdown;
char s_GmwDll[8] = "GMW.DLL";
char s_UTUnRegister[13] = "UTUnRegister";
char s_UTRegister[11] = "UTRegister";
char s_Kernel32Dll[13] = "KERNEL32.DLL";
/* Заголовок DIB кадрового буфера: 8 разрядов, 256 цветов, таблица цветов — индексы
   логической палитры (DIB_PAL_COLORS) 0..255. Ширину и высоту ставит Host_CreateFrameBuffer. */
GM_BITMAPINFO_PAL g_FrameBitmapInfo =
{
    .bmiHeader =
    {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biWidth = -1,
        .biHeight = -1,
        .biPlanes = 1,
        .biBitCount = 8,
        .biCompression = BI_RGB,
        .biClrUsed = 256,
        .biClrImportant = 256,
    },
    .paletteIndices =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
        48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
        64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
        80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
        96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
        144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
    },
};
char s_GameMakerPlayer[18] = "Game Maker Player";
char s_HelpExt[5] = ".HLP";
char g_DriveRootPath[4] = "?:\\";
char s_BmpExt[5] = ".BMP";
char s_AviExt[5] = ".AVI";
char s_Backslash[2] = "\\";
char s_InvalidMovieBackdrop[33] = "Invalid movie background bitmap.";
char s_DefaultIntroName[6] = "intro";
char s_GameRequiresLargerScreen[29] = "Game REQUIRES larger screen.";
char s_WindowClassName[11] = "GMW_PLAYER";
char s_Gdi32Dll[10] = "GDI32.DLL";
char s_CreateDIBSection[17] = "CreateDIBSection";
COLORREF g_TransitionGridColors[4] = { RGB(0, 0, 0), RGB(128, 128, 128), RGB(192, 192, 192), RGB(255, 255, 255) };
int g_DissolvePatternX[16] = { 0, 16, 48, 16, 0, 32, 16, 48, 32, 0, 32, 16, 48, 32, 0, 48 };
int g_DissolvePatternY[16] = { 0, 32, 16, 48, 32, 48, 16, 0, 32, 48, 16, 0, 32, 0, 16, 48 };
STARTUPINFOA g_StartupInfoTemplate;
PROCESS_INFORMATION g_ProcessInfoTemplate;
char s_SavePathFormat[8] = "%s%d.%s";
char s_ExeExt[5] = ".EXE";
char s_TempSavePathFormat[6] = "%s\\%s";
char _tzname_std[12];
char _tzname_dst[12];
int g_ClipWidth;
int g_ClipHeight;
int g_ScrollX;
int g_ScrollY;
GM_DIRTY_RECT * g_DirtyRects;
GM_OBJECT * g_FocusObject;
jmp_buf g_RestartJmp;
bool g_CurObjectMoved;
uint8_t g_DissolvePending;
bool g_CursorInWindow;
GM_OBJECT * g_ResumeObject;
uint16_t g_PatternMatches[512];
uint32_t * g_HeapBase;
uint32_t * g_HeapTop;
uint32_t g_HeapTopFreeDwords;
void * g_LoadScratch;
uint8_t g_SmallTempBuffer[152];
uint32_t g_ReadBufFill;
uint32_t g_ReadBufLeft;
uint8_t * g_ReadBuf;
int g_ResourceDataBase;
int g_ResourceIndexSize;
uint32_t g_LastResourceId;
uint32_t g_RecordSize; /* Размер текущей записи (распакованный, если запись сжата). */
uint32_t g_RecordPackedSize; /* Упакованный размер текущей записи (0 — запись не сжата). */
int8_t g_LoadTag;
uint8_t * g_LoadPtr; /* Куда загрузчик кладёт следующую запись (вершина резерва кучи). */
void * g_RecordData;
uint32_t g_RoomFileOffset;
int g_StartRoom;
bool g_EngineInitPending;
uint8_t g_GraphicOnDisk; /* Признак текущей графики «изображения на диске»: вместо пикселей пишется заголовок {'Disk', смещение, упакованный размер}. */
uint32_t g_DiskImageCachedOffset;
uint32_t g_DiskImageMaxSize; /* Наибольший размер изображения «на диске» — под него заводится общий буфер g_DiskImageBuffer. */
uint8_t * g_DiskImageBuffer;
int g_SaveVarsOffset;
GM_MOVIE_CHUNK g_MovieChunk;
uint32_t g_MovieRepeatCount;
int g_MovieLoopOffset;
uint32_t g_MovieFrameDelay;
uint32_t g_MoviePackedBufSize;
uint8_t * g_MoviePackedBuf;
uint32_t g_MovieFrameBufSize;
uint8_t * g_MovieFrameBuf;
uint32_t g_MoviePaletteBufSize;
uint8_t * g_MoviePaletteBuf;
uint32_t g_MovieSoundBufSize;
GM_SOUND_DATA * g_MovieSoundBuf;
uint32_t g_MovieMusicBufSize;
GM_MUSIC_DATA * g_MovieMusicBuf;
bool g_MovieSoundPlaying;
bool g_MovieMusicPlaying;
bool g_MovieLoopEnded;
int g_MovieX;
int g_MovieY;
int g_MovieWidth;
int g_MovieHeight;
bool g_MovieShowFrames;
DWORD g_MovieStartTime;
uint32_t g_MovieFramesShown;
uint8_t g_MovieSkippedFrames;
bool g_MusicActive;
uint8_t * g_TextCursor;
int g_TextFontIndex;
int g_TextX;
int g_TextY;
GM_FONT * g_TextFont;
char g_TextInputBuffer[256];
GM_SPRITE * g_TextGlyphSprites[255];
int g_TextInputLength;
int g_TextInputMaxLength;
char * g_TextInputTarget;
int8_t g_TextInputMode;
bool g_TextInputActive;
bool g_TextInputDigitsOnly;
GM_OBJECT * g_TextOwnerObject;
GM_CLASS * g_TextOwnerClass;
int g_TextOwnerIndex;
int g_PathOriginX;
int g_PathOriginY;
int g_PathGoalX;
int g_PathGoalY;
int g_PathDir8;
int g_PathDir4;
uint8_t * g_FrameBits;
int g_FrameStride;
int g_FrameHeight;
int g_BlitRowWidth;
HWAVEOUT g_hWaveOut;
UINT g_WaveOutId;
WAVEHDR g_WaveHeaders[8];
HMIDIOUT g_hMidiOut;
UINT g_MidiOutId;
UINT g_MidiTimerId;
DWORD g_MidiNextEventTime;
DWORD g_MidiInCallback;
uint32_t g_MidiLoopDelay;
uint32_t g_MidiDelayCounter;
uint8_t * g_MidiTrackStart;
uint8_t * g_MidiCursor;
uint8_t g_MidiRunningStatus;
uint8_t g_MidiVolume;
UINT g_MidiTimerPeriod;
HGLOBAL g_GmwBuffer;
GM_GMW_THUNK_FN g_pfnGmwThunk;
PFN_UTREGISTER g_pfnUTRegister;
PFN_UTUNREGISTER g_pfnUTUnRegister;
HMODULE g_hKernel32ForUT;
GM_SOUND_CHANNEL g_SoundChannels[8];
uint8_t g_WaveBuffers[8][2048];
uint32_t g_SoundChannelCount;
uint32_t g_NextSoundChannel;
int g_ChannelsInLastMix;
char g_ExecutablePath[MAX_PATH];
GM_WINDOW g_Window;
HGDIOBJ g_hDibSection;
void * g_DibBits;
HDC g_hDibDC;
int g_AviX;
int g_AviY;
int g_AviWidth;
int g_AviHeight;
char g_AviBasePath[MAX_PATH];
MCIDEVICEID g_AviDeviceId;
HWND g_hAviWindow;
bool g_AviPlaying;
bool g_AviQuitRequested;
uint8_t g_AviDissolve;
bool g_AviActive;
bool g_AviPaletteReady;
bool g_IsWin3x;
bool g_AviWindowPlaced;
DWORD g_LastFrameTime;
GM_WORLD_HEADER g_World;
GM_CLASS * g_CurClass;
GM_OBJECT * g_CurObject;
int g_CurObjectIndex;
int g_CurTileX;
int g_CurTileY;
int g_RuleTileX;
int g_RuleTileY;
GM_RULE_XFORM_FN g_pfnRuleXform;
GM_PLACE_CELL_FN g_pfnPlaceCell;
void * g_UnusedObject;
int g_UnusedObjectIndex;
GM_POOL_ENTRY * g_PoolTop; /* Вершина пула записей по 16 байт (растёт вниз): экземпляры GM_OBJECT и спрайты GM_SPRITE; номер записи = g_PoolTop − запись. */
GM_POOL_ENTRY * g_PoolBottom; /* Нижняя граница занятой части пула. */
GM_CLASS * g_Classes; /* Классы мира (GM_CLASS). */
GM_CLASS_SET * g_ClassSets; /* Наборы классов (битовые множества GM_CLASS_SET). */
GM_TILE * g_TileMap; /* Карта тайлов комнаты (строки по 1 << g_TileMapShift клеток). */
int g_TileMapShift; /* Двоичный логарифм длины строки карты тайлов (строка y начинается с клетки y << g_TileMapShift). */
GM_SOUND * g_Sounds; /* Звуки мира (GM_SOUND). */
GM_GRAPHIC * g_Graphics; /* Графика мира: спрайты и шрифты (GM_GRAPHIC). */
int g_MapTileCount; /* Число клеток карты (ширина · высота). */
int g_RoomCount;
GM_ROOM * g_Rooms; /* Комнаты мира (GM_ROOM). */
GM_BACKGROUND * g_Background; /* Фон текущей комнаты (пиксели и палитра). */
GM_GAME_HEADER g_GameHeader;
GM_ARENA g_Arena;
GM_PANEL * g_Panels; /* Панели (GM_PANEL). */
const GM_MUSIC_DATA ** g_MusicTracks; /* Мелодии мира (указатели на данные собственного формата MIDI). */
GM_SETTINGS g_Settings;
int g_StartEntry;
int g_CurrentRoom;
uint8_t * g_ActiveClassFilter;
GM_SYSVARS * g_SysVars; /* Системные переменные скриптов (GM_SYSVARS). */
GM_VAR_DESC * g_VarsKeepAlways; /* Описатель переменных, сохраняемых всегда (и при автосохранении). */
GM_VAR_DESC * g_VarsKeepOnRestart; /* Описатель переменных, сохраняемых при перезапуске игры. */
GM_VAR_DESC * g_VarsSaveGameOnly; /* Описатель переменных, сохраняемых только в сохранении игры. */
int g_RoomsLoaded;
int g_AutosaveSlot;
char g_GameFilePath[MAX_PATH];
uint32_t * g_MovieOffsets;
char * g_WalkMap;
int g_MapMaxX;
int g_WalkMapRowUp;
int g_MapMaxY;
uint8_t * g_GlobalVars;
uint8_t * g_CurVars;
uint32_t g_CurRuleXform;
int g_PointerTileY;
int g_PointerTileX;
int g_ClickTileY;
int g_ClickTileX;
HINSTANCE g_hAppInstance;
HWND g_MainWindow;
GM_DIRECTION g_CurDirection;
bool g_LoadingSave;
bool g_BuildingRoom; /* Идёт построение начального состояния комнаты: звуки и музыка из скриптов в это время не запускаются. */
bool g_FirstRun;
uint8_t g_TargetSoundVolume;
uint8_t g_TargetMusicVolume;
bool g_SpriteSetThisStep;
uint8_t g_ClickTileClass;
GM_EVENT g_ClickEvent;
bool g_ContinueRules;
uint8_t g_InputChar;
uint8_t g_MovieDepth;
bool g_TextPending;
uint8_t g_PointerTileClass;
bool g_PathFound;
GM_EVENT g_PointerEvent;
uint8_t g_SystemSoundVolume;
uint8_t g_SystemMusicVolume;
bool g_GameRunning;
