@echo off
rem $Id: mkmak.bat,v 1.1 2001/09/14 03:16:14 cwolf Exp $
rem
rem This can't be called from the build script because
rem it runs asychronously.
rem
if ."%SDKHOME%"==."" goto notset

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
if not exist %SDKHOME%\build\examples.mak (
   msdev -ex ExportExampleMakefiles
)
goto done

:notset
echo **** Error: must set SDKHOME
goto done

:done
