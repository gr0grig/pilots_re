#include "pilots_fwd.h"

/* Forward declarations for function pointer initializers */
char __cdecl MultimediaStub_NoArgs();
char __stdcall MultimediaStub_OneArg(int param);
char __stdcall MultimediaStub_StartMidi(int param);
_DWORD nullsub_6();

/* Debug log globals (only used in _DEBUG builds) */
#ifdef _DEBUG
HANDLE g_hDebugLogFile = INVALID_HANDLE_VALUE;
CRITICAL_SECTION g_DebugLogCS;
#endif

/* Null-op stub functions */
void __stdcall nullsub_3(int a1) { (void)a1; }
int nullsub_4(void) { return 0; }
int nullsub_5(void) { return 0; }
_DWORD nullsub_6(void) { return 0; }
void __stdcall TransformDir_Identity(SpriteDesc *desc) { (void)desc; }

/* auxMainLoop: wrapper that calls GameMainLoop then restarts via longjmp.
   Original Watcom binary: __userpurge with ESI register arg (opcodeType) + 1 stack arg (startResourceId).
   ESI (opcodeType) is immediately overwritten by the first opcode in GameMainLoop, so it is unused.
   Our __stdcall version takes 1 stack arg (startResourceId) matching the original @4 convention.
   The VM pushes only startResourceId to the stack before calling this native function. */
__declspec(noreturn) int __stdcall auxMainLoop(int startResourceId)
{
    DBG_LOG("auxMainLoop: startResourceId=%d", startResourceId);
    GameMainLoop(0, startResourceId);
    LongjmpRestart();
}

/* ---- GameGlobals struct instance (auto-generated initializer) ---- */
#include "game_globals_init.h"

/* ---- Non-packed global definitions ---- */
char g_SaveMagic1[8] = { 0x3D, 0x56, 0x53, 0x3D, 0x16, 0x00, 0x00, 0x80 };
char g_SaveMagic2[4] = { 0x53, 0x41, 0x56, 0x00 };
BlitFuncPtrInternal g_BlitFuncTable[4] = {
  (BlitFuncPtrInternal)BlitAdditive_Raw,
  (BlitFuncPtrInternal)RLEBlitAdditive_TypeA,
  (BlitFuncPtrInternal)RLEBlitAdditive_TypeB,
  (BlitFuncPtrInternal)RLEBlitAdditive_TypeC
};
int g_DissolvePatternX[16] = { 0, 16, 48, 16, 0, 32, 16, 48, 32, 0, 32, 16, 48, 32, 0, 48 };
int g_DissolvePatternY[16] = { 0, 32, 16, 48, 32, 48, 16, 0, 32, 48, 16, 0, 32, 0, 16, 48 };
STARTUPINFOA g_StartupInfoTemplate = {0};
PROCESS_INFORMATION g_ProcessInfoTemplate = {0};
char g_SmallTempBuffer[152] = {0};
