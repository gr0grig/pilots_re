#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#pragma once

#include <windows.h>
#include <stdio.h>
#include <intrin.h>
#include <crtdbg.h>
#include <stdlib.h>

#ifdef _DEBUG

/*
 * Debug logging via OutputDebugString.
 * View output with:
 *   - Visual Studio Output window (Debug -> Windows -> Output)
 *   - DebugView (Sysinternals) - https://learn.microsoft.com/en-us/sysinternals/downloads/debugview
 *   - WinDbg
 *
 * Also writes to debug_log.txt in the EXE directory.
 */

extern HANDLE g_hDebugLogFile;
extern CRITICAL_SECTION g_DebugLogCS;

/*
 * Custom invalid parameter handler for MSVC debug CRT.
 * The original Watcom runtime simply returned -1/errno on invalid handles
 * (e.g. read(-1, ...) or close(-1)). MSVC debug CRT triggers an assertion
 * dialog instead. This handler suppresses the popup and lets the CRT function
 * return an error code, matching Watcom behavior.
 */
static void __cdecl DbgInvalidParameterHandler(
    const wchar_t *expression,
    const wchar_t *function,
    const wchar_t *file,
    unsigned int line,
    uintptr_t pReserved)
{
    /* Silently ignore — the CRT function will return an error value */
    (void)expression; (void)function; (void)file; (void)line; (void)pReserved;
}

static void DbgLogInit(void)
{
    char logPath[MAX_PATH];

    /* Suppress MSVC debug CRT assertion popups for invalid I/O handles.
     * Original Watcom CRT returned -1 on invalid handles; MSVC debug CRT
     * fires assertions. This makes _read/_write/_close/_lseek behave like
     * Watcom: return -1 and set errno. */
    _set_invalid_parameter_handler(DbgInvalidParameterHandler);
    _CrtSetReportMode(_CRT_ASSERT, 0);  /* Disable assert popups */

    GetModuleFileNameA(NULL, logPath, MAX_PATH);
    /* Replace .exe with _debug.log */
    {
        char *dot = strrchr(logPath, '.');
        if (dot) *dot = 0;
    }
    strcat(logPath, "_debug.log");

    g_hDebugLogFile = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    InitializeCriticalSection(&g_DebugLogCS);

    OutputDebugStringA("[PILOTS] Debug logging started\n");
}

static void DbgLogClose(void)
{
    if (g_hDebugLogFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hDebugLogFile);
        g_hDebugLogFile = INVALID_HANDLE_VALUE;
    }
    DeleteCriticalSection(&g_DebugLogCS);
}

static void DbgPrintf(const char *fmt, ...)
{
    char buf[2048];
    va_list ap;
    DWORD written;

    va_start(ap, fmt);
    wvsprintfA(buf, fmt, ap);
    va_end(ap);

    /* OutputDebugString - visible in debugger/DebugView */
    OutputDebugStringA(buf);

    /* Also write to log file */
    EnterCriticalSection(&g_DebugLogCS);
    if (g_hDebugLogFile != INVALID_HANDLE_VALUE) {
        WriteFile(g_hDebugLogFile, buf, lstrlenA(buf), &written, NULL);
        FlushFileBuffers(g_hDebugLogFile);
    }
    LeaveCriticalSection(&g_DebugLogCS);
}

/* Log function entry */
#define DBG_ENTER(func) DbgPrintf("[PILOTS] ENTER: %s\n", func)
#define DBG_EXIT(func)  DbgPrintf("[PILOTS] EXIT:  %s\n", func)

/* Log a message */
#define DBG_LOG(msg, ...) DbgPrintf("[PILOTS] " msg "\n", __VA_ARGS__)
#define DBG_MSG(msg) DbgPrintf("[PILOTS] " msg "\n")

/* Log an error */
#define DBG_ERR(msg, ...) DbgPrintf("[PILOTS] ERROR: " msg "\n", __VA_ARGS__)

/* ===== SEH Exception Handler ===== */

static const char* ExceptionCodeToString(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:       return "ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:  return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:             return "BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:  return "DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND:   return "FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:     return "FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:     return "FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:  return "FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:           return "FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:        return "FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:          return "FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:    return "ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:          return "IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:     return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:           return "INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:    return "INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION:       return "PRIVILEGED_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP:            return "SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:         return "STACK_OVERFLOW";
    default:                               return "UNKNOWN";
    }
}

static LONG WINAPI DebugExceptionFilter(EXCEPTION_POINTERS *pExcept)
{
    EXCEPTION_RECORD *rec = pExcept->ExceptionRecord;
    CONTEXT *ctx = pExcept->ContextRecord;
    DWORD *sp;
    int i;

    DbgPrintf("[PILOTS] *** EXCEPTION: 0x%08X (%s) at address 0x%08X ***\n",
        rec->ExceptionCode,
        ExceptionCodeToString(rec->ExceptionCode),
        (unsigned int)rec->ExceptionAddress);

    DbgPrintf("[PILOTS] Registers: EAX=0x%08X EBX=0x%08X ECX=0x%08X EDX=0x%08X\n",
        ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx);
    DbgPrintf("[PILOTS]            ESI=0x%08X EDI=0x%08X EBP=0x%08X ESP=0x%08X\n",
        ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp);
    DbgPrintf("[PILOTS]            EIP=0x%08X EFLAGS=0x%08X\n",
        ctx->Eip, ctx->EFlags);

    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2) {
        DbgPrintf("[PILOTS] Access violation: %s address 0x%08X\n",
            rec->ExceptionInformation[0] ? "WRITE to" : "READ from",
            (unsigned int)rec->ExceptionInformation[1]);
    }

    /* Dump raw stack contents around ESP */
    sp = (DWORD *)ctx->Esp;
    DbgPrintf("[PILOTS] Raw stack (ESP=0x%08X):\n", ctx->Esp);
    for (i = 0; i < 32; i += 4) {
        DbgPrintf("[PILOTS]   +%02X: %08X %08X %08X %08X\n",
            i*4, sp[i], sp[i+1], sp[i+2], sp[i+3]);
    }

    /* Walk stack frames via EBP chain */
    {
        DWORD *ebp = (DWORD *)ctx->Ebp;
        int frame = 0;
        DbgPrintf("[PILOTS] EBP chain:\n");
        while (ebp && frame < 20) {
            if ((unsigned int)ebp < 0x10000 || (unsigned int)ebp > 0x7FFFFFFF)
                break;
            DbgPrintf("[PILOTS]   F%d: EBP=0x%08X Ret=0x%08X\n", frame, (unsigned int)ebp, ebp[1]);
            ebp = (DWORD *)ebp[0];
            frame++;
        }
    }

    /* Flush the log before we crash */
    DbgLogClose();

    return EXCEPTION_CONTINUE_SEARCH;
}

static void DbgInstallExceptionHandler(void)
{
    SetUnhandledExceptionFilter(DebugExceptionFilter);
    DBG_MSG("Unhandled exception filter installed");
}

#else /* Release build - all macros become no-ops */

#define DbgLogInit()                ((void)0)
#define DbgLogClose()               ((void)0)
#define DbgPrintf(...)              ((void)0)
#define DbgInstallExceptionHandler() ((void)0)
#define DBG_ENTER(func)             ((void)0)
#define DBG_EXIT(func)              ((void)0)
#define DBG_LOG(msg, ...)           ((void)0)
#define DBG_MSG(msg)                ((void)0)
#define DBG_ERR(msg, ...)           ((void)0)

#endif /* _DEBUG */

#endif /* DEBUG_LOG_H */
