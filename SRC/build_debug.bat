@echo off
setlocal

set VCTOOLS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717
set WINSDK=C:\Program Files (x86)\Windows Kits\10
set SDKVER=10.0.26100.0

set PATH=%VCTOOLS%\bin\Hostx86\x86;%WINSDK%\bin\%SDKVER%\x86;%PATH%
set INCLUDE=%VCTOOLS%\include;%WINSDK%\Include\%SDKVER%\ucrt;%WINSDK%\Include\%SDKVER%\um;%WINSDK%\Include\%SDKVER%\shared
set LIB=%VCTOOLS%\lib\x86;%WINSDK%\Lib\%SDKVER%\ucrt\x86;%WINSDK%\Lib\%SDKVER%\um\x86

pushd "%~dp0"

echo === DEBUG BUILD ===

del /Q *.obj 2>nul

cl.exe /c /TC /W3 /WX- /Od /Zi /D_DEBUG /DWIN32 /MTd ^
  /wd4996 /wd4113 /wd4047 /wd4024 /wd4133 /wd4028 /wd4029 /wd4090 ^
  /wd4244 /wd4267 /wd4554 /wd4838 /wd4309 /wd4005 /wd4701 ^
  /FdPILOT_RV.pdb ^
  globals.c animation.c render.c gameloop.c ^
  script.c resource.c saveload.c audio.c ^
  video.c text.c tilemap.c blitting.c ^
  graphics.c main.c fileio.c memory.c ^
  input.c native_funcs.c
if errorlevel 1 goto :fail

link.exe /OUT:PILOT_RV.EXE /SUBSYSTEM:WINDOWS /MACHINE:X86 /NOLOGO ^
  /DEBUG /DYNAMICBASE:NO /FIXED ^
  /PDB:PILOT_RV.pdb /MAP:PILOT_RV.map ^
  kernel32.lib user32.lib gdi32.lib winmm.lib vfw32.lib ^
  comdlg32.lib advapi32.lib shell32.lib ole32.lib oldnames.lib ^
  libcmtd.lib libvcruntimed.lib libucrtd.lib ^
  *.obj
if errorlevel 1 goto :fail

echo.
echo SUCCESS: PILOT_RV.EXE (DEBUG)
goto :end

:fail
echo.
echo BUILD FAILED
popd
exit /b 1

:end
popd
endlocal
