@echo off
rem $Id: mkmak.bat,v 1.6 2001/10/18 17:21:59 cwolf Exp $
rem
rem
if ."%SRCROOT%"==."" goto notset

rem Create and install MSVC macros, if needed
call mfmacro.bat
if errorlevel 1 goto error

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
if not exist %SRCROOT%\vorbis\win32\vorbis_dynamic.mak (
  echo Generating makefiles, please wait...
  execwait msdev -ex ExportMakefile
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
