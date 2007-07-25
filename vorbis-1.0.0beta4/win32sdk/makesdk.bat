@echo off
echo ---+++--- Making Win32 SDK ---+++---

if .%SRCROOT%==. set SRCROOT=c:\src

rem --- prepare directory

rd /s /q sdk > nul
md sdk
md sdk\include
md sdk\include\ogg
md sdk\include\vorbis
md sdk\release
md sdk\debug
md sdk\doc
md sdk\examples

rem --- is ogg here?

echo Searching for ogg...

if exist %SRCROOT%\ogg\include\ogg\ogg.h goto OGGFOUND

echo ... ogg not found.

goto ERROR

:OGGFOUND

echo ... ogg found.

rem --- is vorbis here?

echo Searching for vorbis...

if exist %SRCROOT%\vorbis\include\vorbis\codec.h goto VORBISFOUND

echo ... vorbis not found.

goto ERROR

:VORBISFOUND

echo ... vorbis found.

rem --- copy include files into sdk

echo Copying include files...

xcopy %SRCROOT%\ogg\include\ogg\*.h %SRCROOT%\win32sdk\sdk\include\ogg > nul
xcopy %SRCROOT%\vorbis\include\vorbis\*.h %SRCROOT%\win32sdk\sdk\include\vorbis > nul

echo ... copied.

rem --- build and copy ogg

echo Building ogg...
cd %SRCROOT%\ogg\win32
call build_ogg_static.bat > nul
echo ... static release built ...
xcopy %SRCROOT%\ogg\win32\Static_Release\ogg_static.lib %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
echo ... static release copied ...
call build_ogg_static_debug.bat > nul
echo ... static debug built ...
xcopy %SRCROOT%\ogg\win32\Static_Debug\ogg_static.lib %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
echo ... static debug copied ...
call build_ogg_dynamic.bat > nul
echo ... dynamic release built ...
xcopy %SRCROOT%\ogg\win32\Dynamic_Release\ogg.lib %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Release\ogg.dll %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
echo ... dynamic release copied ...
call build_ogg_dynamic_debug.bat > nul
echo ... dynamic debug built ...
xcopy %SRCROOT%\ogg\win32\Dynamic_Debug\ogg.lib %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Debug\ogg.dll %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
echo ... dynamic debug copied ...
echo ... ogg building done.

rem --- build and copy vorbis

echo Building vorbis...
cd %SRCROOT%\vorbis\win32
call build_vorbis_static.bat > nul
echo ... static release built ...
xcopy %SRCROOT%\vorbis\win32\Static_Release\vorbis_static.lib %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
echo ... static release copied ...
call build_vorbis_static_debug.bat > nul
echo ... static debug built ...
xcopy %SRCROOT%\vorbis\win32\Static_Debug\vorbis_static.lib %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
echo ... static debug copied ...
call build_vorbis_dynamic.bat > nul
echo ... dynamic release built ...
xcopy %SRCROOT%\vorbis\win32\Dynamic_Release\vorbis.lib %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Dynamic_Release\vorbis.dll %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
echo ... dynamic release copied ...
call build_vorbis_dynamic_debug.bat > nul
echo ... dynamic debug built ...
xcopy %SRCROOT%\vorbis\win32\Dynamic_Debug\vorbis.lib %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Dynamic_Debug\vorbis.dll %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
echo ... dynamic debug copied ...
echo ... vorbis building done.

rem --- build and copy vorbisfile

echo Building vorbisfile...
cd %SRCROOT%\vorbis\win32
call build_vorbisfile_static.bat > nul
echo ... static release built ...
xcopy %SRCROOT%\vorbis\win32\Static_Release\vorbisfile_static.lib %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
echo ... static release copied ...
call build_vorbisfile_static_debug.bat > nul
echo ... static debug built ...
xcopy %SRCROOT%\vorbis\win32\Static_Debug\vorbisfile_static.lib %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
echo ... static debug copied ...
call build_vorbisfile_dynamic.bat > nul
echo ... dynamic release built ...
xcopy %SRCROOT%\vorbis\win32\Dynamic_Release\vorbisfile.lib %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Dynamic_Release\vorbisfile.dll %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
echo ... dynamic release copied ...
call build_vorbisfile_dynamic_debug.bat > nul
echo ... dynamic debug built ...
xcopy %SRCROOT%\vorbis\win32\Dynamic_Debug\vorbisfile.lib %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Dynamic_Debug\vorbisfile.dll %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
echo ... dynamic debug copied ...
echo ... vorbisfile building done.

rem --- build and copy vorbisenc

echo Building vorbisenc...
cd %SRCROOT%\vorbis\win32
call build_vorbisenc_static.bat > nul
echo ... static release built ...
xcopy %SRCROOT%\vorbis\win32\Static_Release\vorbisenc_static.lib %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
echo ... static release copied ...
call build_vorbisenc_static_debug.bat > nul
echo ... static debug built ...
xcopy %SRCROOT%\vorbis\win32\Static_Debug\vorbisenc_static.lib %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
echo ... static debug copied ...
call build_vorbisenc_dynamic.bat > nul
echo ... dynamic release built ...
xcopy %SRCROOT%\vorbis\win32\Dynamic_Release\vorbisenc.lib %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Dynamic_Release\vorbisenc.dll %SRCROOT%\win32sdk\sdk\release > nul
if errorlevel 1 goto ERROR
echo ... dynamic release copied ...
call build_vorbisenc_dynamic_debug.bat > nul
echo ... dynamic debug built ...
xcopy %SRCROOT%\vorbis\win32\Dynamic_Debug\vorbisenc.lib %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Dynamic_Debug\vorbisenc.dll %SRCROOT%\win32sdk\sdk\debug > nul
if errorlevel 1 goto ERROR
echo ... dynamic debug copied ...
echo ... vorbisenc building done.

rem -- finished

goto DONE
:ERROR

cd %SRCROOT%\win32sdk
rd /s /q sdk > nul
echo.
echo Some error(s) occurred. Fix it.
goto EXIT

:DONE
cd %SRCROOT%\win32sdk
echo All done.

:EXIT
