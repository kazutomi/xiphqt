@echo off
rem $Id: mkmak.bat,v 1.2 2001/09/15 08:09:24 cwolf Exp $
rem
rem This can't be called from the build script because 
rem it runs asychronously.
rem
if ."%SRCROOT"==."" goto notset

rem Create and install MSVC macros, if needed
call mfmacro.bat
if errorlevel 1 goto error

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
if not exist %SRCROOT%\vorbis\win32\vorbis_dynamic.mak (
  echo Generating makefiles, please wait...
  msdev -ex ExportMakefile
  sleep 10
  echo Done.
)
goto done

:error
echo **** Error: couldn't create or install macro
goto done

:notset
echo **** Error: must set SRCROOT
goto done

:done
