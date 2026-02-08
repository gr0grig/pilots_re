#include "pilots_fwd.h"

#pragma pack(push, 2)

/* ---- fileio module ---- */

HFILE __stdcall Game_OpenFile(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle)
{
  DBG_ENTER("Game_OpenFile");
  { char fullPath[MAX_PATH];
    DBG_LOG("Game_OpenFile: lpFileName='%s'", lpFileName);
    GetFullPathNameA(lpFileName, MAX_PATH, fullPath, NULL);
    /* Redirect: replace our EXE name with the original PILOTS.EXE for resource data */
    { char *_sl = strrchr(fullPath, '\\');
      if (_sl && _strnicmp(_sl+1, "PILOT", 5) == 0 && _stricmp(_sl+1, "PILOTS.EXE") != 0)
        strcpy(_sl + 1, "PILOTS.EXE");
    }
    DBG_LOG("Game_OpenFile: fullPath='%s'", fullPath);
    DBG_LOG("Game_OpenFile: calling _open()");
    g_VideoConfig.dataFileHandle = _open(fullPath, _O_RDONLY | _O_BINARY);
    DBG_LOG("Game_OpenFile: fd=%d", g_VideoConfig.dataFileHandle);
  }
  return (-g_VideoConfig.dataFileHandle - (unsigned int)(g_VideoConfig.dataFileHandle != 0)) >> 31;
}

int FileTell()
{
  return _lseek(g_VideoConfig.dataFileHandle, 0, 1);
}

BOOL __stdcall SeekFile(int Origin, int Offset)
{
  return _lseek(g_VideoConfig.dataFileHandle, Offset, Origin) != -1;
}

BOOL __stdcall ReadFileData(unsigned int maxBytes, void *DstBuf)
{
  return _read(g_VideoConfig.dataFileHandle, DstBuf, maxBytes) == maxBytes;
}

int __stdcall ReadFileRaw(unsigned int maxBytes, void *DstBuf)
{
  return _read(g_VideoConfig.dataFileHandle, DstBuf, maxBytes);
}

int CloseFile()
{
  int result; // eax

  result = g_VideoConfig.dataFileHandle;
  if ( g_VideoConfig.dataFileHandle > 0 )
  {
    result = _close(g_VideoConfig.dataFileHandle);
    g_VideoConfig.dataFileHandle = -1;
  }
  return result;
}

int __stdcall BuildSaveFilePath(int a1, const char *a2, char *Source, LPSTR a4)
{
  char *v4; // eax
  char *v5; // eax
  char Destination[128]; // [esp+0h] [ebp-80h] BYREF

  v4 = strcpy(Destination, Source);
  v5 = strupr(v4);
  { char *_ext = strstr(v5, SubStr);
    if (_ext) *_ext = 0;
    else { char *_dot = strrchr(v5, '.'); if (_dot) *_dot = 0; } }
  return wsprintfA(a4, "%s%d.%s", Destination, a1, a2);
}

int __stdcall BuildAlternateSavePath(char *Str, LPSTR a2)
{
  char *v2; // eax
  CHAR tmpBuffer[128]; // [esp+0h] [ebp-80h] BYREF

  GetTempPathA(0x80u, tmpBuffer);
  v2 = strrchr(Str, 92);
  return wsprintfA(a2, "%s\\%s", tmpBuffer, v2);
}

unsigned int __stdcall OpenSaveFile(int save_slot, int ext_string, char *base_path)
{
  char FileName[128]; // [esp+0h] [ebp-100h] BYREF
  CHAR v5[128]; // [esp+80h] [ebp-80h] BYREF

  BuildSaveFilePath(save_slot, (const char *)ext_string, base_path, FileName);
  DBG_LOG("OpenSaveFile: slot=%d ext='%s' base='%s' file='%s'", save_slot, (const char *)ext_string, base_path, FileName);
  g_VideoConfig.saveFileHandle = _open(FileName, 33026, 384);
  DBG_LOG("OpenSaveFile: _open returned %d", g_VideoConfig.saveFileHandle);
  if ( g_VideoConfig.saveFileHandle <= 0 )
  {
    BuildAlternateSavePath(FileName, v5);
    g_VideoConfig.saveFileHandle = _open(v5, 33026, 384);
  }
  return (-g_VideoConfig.saveFileHandle - (unsigned int)(g_VideoConfig.saveFileHandle != 0)) >> 31;
}

int __stdcall DeleteSaveFile(int save_slot, int ext_string, char *base_path)
{
  int result; // al
  char FileName[128]; // [esp+0h] [ebp-100h] BYREF
  CHAR v5[128]; // [esp+80h] [ebp-80h] BYREF

  BuildSaveFilePath(save_slot, ext_string, base_path, FileName);
  result = 1;
  if ( remove(FileName) )
  {
    BuildAlternateSavePath(FileName, v5);
    if ( remove(v5) )
      return 0;
  }
  return result;
}

BOOL __stdcall ReadFromFile(unsigned int size, void *dst_buf)
{
  return _read(g_VideoConfig.saveFileHandle, dst_buf, size) == size;
}

BOOL __stdcall WriteToFile(unsigned int size, void *buf)
{
  return _write(g_VideoConfig.saveFileHandle, buf, size) == size;
}

int TellSaveFile()
{
  return _lseek(g_VideoConfig.saveFileHandle, 0, 1);
}

BOOL __stdcall SeekSaveFile(int seekOffset)
{
  return _lseek(g_VideoConfig.saveFileHandle, seekOffset, 0) != -1;// Check for lseek error (-1)
}

int TruncateFile()
{
  return _chsize(g_VideoConfig.saveFileHandle, 0);
}

int CloseSaveFile()
{
  int result; // eax

  result = g_VideoConfig.saveFileHandle;
  if ( g_VideoConfig.saveFileHandle > 0 )
  {
    result = _close(g_VideoConfig.saveFileHandle);
    g_VideoConfig.saveFileHandle = -1;
  }
  return result;
}

#pragma pack(pop)
