@echo off
rem $Id: mkmak.bat,v 1.7 2001/10/20 21:12:34 cwolf Exp $
rem
rem
if ."%SRCROOT%"==."" goto notset

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
if not exist %SRCROOT%\vorbis\win32\vorbis_dynamic.mak (
  echo Generating makefiles, please wait...
  execwait %SystemRoot%\system32\cscript.exe //nologo exportmf.js
  echo Done.
)
goto done

:notset
echo **** Error: must set SRCROOT
goto done

:done
