@echo off
rem $Id: build_all.bat,v 1.5 2001/09/15 07:00:31 cwolf Exp $
rem
rem Invoke as "build_all.bat CLEAN" to clean all targets
rem
if ."%SRCROOT%"==."" goto notset

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
rem Makefile generation is asychronous, so it cannot be 
rem called inline with this script.
if not exist %SRCROOT%\vorbis\win32\vorbis_dynamic.mak (
  echo Error: must invoke "mkmak.bat" first
  exit
)

if not exist %SRCROOT%\ogg\include\ogg\ogg.h goto noogg
if not exist %SRCROOT%\vorbis\include\vorbis\codec.h goto novorbis

cd "..\ogg\win32"
nmake /nologo /F .\ogg_dynamic.mak CFG="ogg_dynamic - Win32 Debug" %1
nmake /nologo /F .\ogg_dynamic.mak CFG="ogg_dynamic - Win32 Release" %1
nmake /nologo /F .\ogg_static.mak CFG="ogg_static - Win32 Debug" %1
nmake /nologo /F .\ogg_static.mak CFG="ogg_static - Win32 Release" %1
cd "..\..\win32sdk"

cd "..\vorbis\win32"
nmake /nologo /F .\vorbis_dynamic.mak CFG="vorbis_dynamic - Win32 Debug" %1
nmake /nologo /F .\vorbis_dynamic.mak CFG="vorbis_dynamic - Win32 Release" %1
nmake /nologo /F .\vorbis_static.mak CFG="vorbis_static - Win32 Debug" %1
nmake /nologo /F .\vorbis_static.mak CFG="vorbis_static - Win32 Release" %1
nmake /nologo /F .\vorbisenc_dynamic.mak CFG="vorbisenc_dynamic - Win32 Debug" %1
nmake /nologo /F .\vorbisenc_dynamic.mak CFG="vorbisenc_dynamic - Win32 Release" %1
nmake /nologo /F .\vorbisenc_static.mak CFG="vorbisenc_static - Win32 Debug" %1
nmake /nologo /F .\vorbisenc_static.mak CFG="vorbisenc_static - Win32 Release" %1
nmake /nologo /F .\vorbisfile_dynamic.mak CFG="vorbisfile_dynamic - Win32 Debug" %1
nmake /nologo /F .\vorbisfile_dynamic.mak CFG="vorbisfile_dynamic - Win32 Release" %1
nmake /nologo /F .\vorbisfile_static.mak CFG="vorbisfile_static - Win32 Debug" %1
nmake /nologo /F .\vorbisfile_static.mak CFG="vorbisfile_static - Win32 Release" %1
cd "..\..\win32sdk"
goto normal

:noogg
echo ***** Need module OGG -- not present
exit

:novorbis
echo ***** Need module VORBIS -- not present
exit

:notset
echo ***** Error: must set SRCROOT
exit

:normal
