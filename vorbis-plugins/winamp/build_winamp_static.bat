@echo off
echo ---+++--- Building Winamp Plugin (Static) ---+++---

if .%SRCROOT%==. set SRCROOT=c:\src

set OLDPATH=%PATH%
set OLDINCLUDE=%INCLUDE%
set OLDLIB=%LIB%

call "c:\program files\microsoft visual studio\vc98\bin\vcvars32.bat"
echo Setting include/lib paths for Ogg and Vorbis
set INCLUDE=%INCLUDE%;%SRCROOT%\ogg\include;%SRCROOT%\vorbis\include
set LIB=%LIB%;%SRCROOT%\ogg\win32\Static_Release;%SRCROOT%\vorbis\win32\Static_Release
echo Compiling...
msdev winamp_plugin.dsp /useenv /make "winamp_plugin - Win32 Release" /rebuild

set PATH=%OLDPATH%
set INCLUDE=%OLDINCLUDE%
set LIB=%OLDLIB%
