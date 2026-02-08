/*
 * HexRays decompiler compatibility definitions for MSVC.
 * Provides macros and types that the IDA Hex-Rays decompiler uses
 * in its pseudocode output.
 */
#ifndef __DEFS_H__
#define __DEFS_H__

#pragma once

#include <string.h>  /* memcpy, memset */
#include <stdlib.h>

/* ---- IDA integer type aliases ---- */
typedef unsigned char      _BYTE;
typedef unsigned short     _WORD;
typedef unsigned int       _DWORD;
typedef unsigned __int64   _QWORD;
typedef int                _UNKNOWN;

/* ---- Byte/word extraction macros ---- */

/* Low/high byte of a value */
#define LOBYTE(x)   (*((_BYTE*)&(x)))
#define HIBYTE(x)   (*(((_BYTE*)&(x)) + sizeof(x) - 1))

/* Low/high word of a value */
#define LOWORD(x)   (*((_WORD*)&(x)))
#define HIWORD(x)   (*(((_WORD*)&(x)) + (sizeof(x)/sizeof(_WORD)) - 1))

/* Low/high dword of a 64-bit value */
#define LODWORD(x)  (*((_DWORD*)&(x)))
#define HIDWORD(x)  (*(((_DWORD*)&(x)) + 1))

/* Signed high dword (for signed shift/compare on upper 32 bits) */
#define SHIDWORD(x) (*((int*)&(x) + 1))

/* Byte at index n */
#define BYTEn(x, n) (*(((_BYTE*)&(x)) + (n)))
#define BYTE1(x)    BYTEn(x, 1)
#define BYTE2(x)    BYTEn(x, 2)
#define BYTE3(x)    BYTEn(x, 3)
#define BYTE4(x)    BYTEn(x, 4)

/* Signed byte at index n */
#define SBYTEn(x, n) (*((signed char*)&(x) + (n)))
#define SBYTE1(x)    SBYTEn(x, 1)
#define SBYTE2(x)    SBYTEn(x, 2)
#define SHIBYTE(x)   (*((signed char*)&(x) + sizeof(x) - 1))

/* Word at index n */
#define WORDn(x, n) (*(((_WORD*)&(x)) + (n)))
#define WORD1(x)    WORDn(x, 1)

/* ---- 64-bit pair construction ---- */
#define __PAIR64__(hi, lo) \
    ( ((unsigned __int64)(unsigned int)(hi) << 32) | (unsigned int)(lo) )

/* ---- Rotate left (32-bit) ---- */
static __inline unsigned int __ROL4__(unsigned int value, int count)
{
    return _rotl(value, count);
}

/* ---- Overflow detection for addition (signed byte) ---- */
static __inline int __OFADD__(signed char a, signed char b)
{
    int r = (int)a + (int)b;
    return (r > 127 || r < -128) ? 1 : 0;
}

/* ---- qmemcpy: IDA's memcpy alias ---- */
#define qmemcpy(dst, src, n) memcpy(dst, src, n)

/* ---- memset32: fill memory with 32-bit pattern ---- */
static __inline void memset32(void *dst, unsigned int val, unsigned int count)
{
    unsigned int *p = (unsigned int *)dst;
    while (count--)
        *p++ = val;
}

/* ---- setjmp compatibility ---- */
/* MSVC uses _setjmp3 internally for setjmp */
#include <setjmp.h>
#ifndef _setjmp3
#define _setjmp3(buf, arg) setjmp(buf)
#endif

#endif /* __DEFS_H__ */
