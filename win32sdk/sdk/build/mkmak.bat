@echo off
rem $Id: mkmak.bat,v 1.2 2001/09/15 08:09:25 cwolf Exp $
rem
rem This can't be called from the build script because
rem it runs asychronously.
rem
if ."%SDKHOME%"==."" goto notset

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
if not exist %SDKHOME%\build\examples.mak (
   echo Generating makefiles, please wait...
   msdev -ex ExportExampleMakefiles
   sleep 5
   echo Done.
)
goto done

:notset
echo **** Error: must set SDKHOME
goto done

:done
