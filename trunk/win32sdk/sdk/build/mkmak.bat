@echo off
rem $Id: mkmak.bat,v 1.4 2001/10/18 17:22:00 cwolf Exp $
rem
rem
if ."%SDKHOME%"==."" goto notset

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
if not exist %SDKHOME%\build\examples.mak (
   echo Generating makefiles, please wait...
   execwait msdev -ex ExportExampleMakefiles
   echo Done.
)
goto done

:notset
echo **** Error: must set SDKHOME
goto done

:done
