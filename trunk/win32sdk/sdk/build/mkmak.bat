@echo off
rem $Id: mkmak.bat,v 1.5 2001/10/20 21:12:37 cwolf Exp $
rem
rem
if ."%SDKHOME%"==."" goto notset

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
if not exist %SDKHOME%\build\examples.mak (
   echo Generating makefiles, please wait...
   execwait %SystemRoot%\system32\cscript.exe //nologo exportmf.js
   echo Done.
)
goto done

:notset
echo **** Error: must set SDKHOME
goto done

:done
