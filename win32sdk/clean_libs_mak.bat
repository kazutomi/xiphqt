@echo off
rem $Id: clean_libs_mak.bat,v 1.1 2001/09/15 08:09:23 cwolf Exp $
rem
rem Deletes auto generated makefiles in ogg/win32 and vorbis/win32
rem
if ."%SRCROOT%"=="". goto notset
del %SRCROOT%\ogg\win32\*.mak 2> nul
del %SRCROOT%\ogg\win32\*.dep 2> nul
del %SRCROOT%\vorbis\win32\*.mak 2> nul
del %SRCROOT%\vorbis\win32\*.dep 2> nul
goto done

:notset
echo **** SRCROOT must be set
goto done

:done
