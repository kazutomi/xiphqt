@echo off
echo ---+++--- Making Win32 SDK ---+++---
rem
rem $Id: makesdk.bat,v 1.13 2001/10/20 21:12:34 cwolf Exp $
rem

if ."%SRCROOT%"==."" goto notset

if ."%MSDEVDIR%"==."" goto msdevnotset


if not exist execwait.exe (
  cl /nologo execwait.c
)

rd /s /q sdk\include 2> nul
rd /s /q sdk\lib 2> nul
rd /s /q sdk\bin 2> nul
rd /s /q sdk\doc 2> nul
rd /s /q sdk\examples 2> nul
md sdk\include\ogg
md sdk\include\vorbis
md sdk\lib
md sdk\bin
md sdk\doc\ogg\ogg
md sdk\doc\vorbis\vorbisenc
md sdk\doc\vorbis\vorbisfile
md sdk\examples\vorbis

attrib +r sdk\build\test.ogg
attrib +r sdk\build\test.wav

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


rem --- build all
echo Building libraries...
call build_all.bat
if errorlevel 1 goto ERROR


rem --- copy include files into sdk

echo Copying include files...

xcopy %SRCROOT%\ogg\include\ogg\*.h %SRCROOT%\win32sdk\sdk\include\ogg > nul
xcopy %SRCROOT%\vorbis\include\vorbis\*.h %SRCROOT%\win32sdk\sdk\include\vorbis > nul

echo ... copied.

rem --- copy docs into sdk

echo Copying docs...

xcopy %SRCROOT%\ogg\doc\*.html %SRCROOT%\win32sdk\sdk\doc\ogg > nul
xcopy %SRCROOT%\ogg\doc\*.png %SRCROOT%\win32sdk\sdk\doc\ogg > nul
xcopy %SRCROOT%\ogg\doc\ogg\*.html %SRCROOT%\win32sdk\sdk\doc\ogg\ogg > nul
xcopy %SRCROOT%\ogg\doc\ogg\*.css %SRCROOT%\win32sdk\sdk\doc\ogg\ogg > nul
xcopy %SRCROOT%\vorbis\doc\*.html %SRCROOT%\win32sdk\sdk\doc\vorbis > nul
xcopy %SRCROOT%\vorbis\doc\*.txt %SRCROOT%\win32sdk\sdk\doc\vorbis > nul
xcopy %SRCROOT%\vorbis\doc\*.png %SRCROOT%\win32sdk\sdk\doc\vorbis > nul
xcopy %SRCROOT%\vorbis\doc\vorbisenc\*.html %SRCROOT%\win32sdk\sdk\doc\vorbis\vorbisenc > nul
xcopy %SRCROOT%\vorbis\doc\vorbisenc\*.css %SRCROOT%\win32sdk\sdk\doc\vorbis\vorbisenc > nul
xcopy %SRCROOT%\vorbis\doc\vorbisfile\*.html %SRCROOT%\win32sdk\sdk\doc\vorbis\vorbisfile > nul
xcopy %SRCROOT%\vorbis\doc\vorbisfile\*.css %SRCROOT%\win32sdk\sdk\doc\vorbis\vorbisfile > nul

copy execwait.exe %SRCROOT%\win32sdk\sdk\build

echo ... copied.

rem --- copy examples into sdk

echo Copying examples...

xcopy /y %SRCROOT%\vorbis\examples\*.c %SRCROOT%\win32sdk\sdk\examples\vorbis > nul

echo ... copied.


xcopy %SRCROOT%\ogg\win32\Static_Release\ogg_static.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Static_Debug\ogg_static_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Release\ogg.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Release\ogg.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Debug\ogg_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Debug\ogg_d.pdb %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Debug\ogg_d.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Static_Release\vorbis_static.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Static_Debug\vorbis_static_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Release\vorbis.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Release\vorbis.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Debug\vorbis_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Debug\vorbis_d.pdb %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Debug\vorbis_d.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Static_Release\vorbisfile_static.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Static_Debug\vorbisfile_static_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Release\vorbisfile.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Release\vorbisfile.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Debug\vorbisfile_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Debug\vorbisfile_d.pdb %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Debug\vorbisfile_d.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Static_Release\vorbisenc_static.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Static_Debug\vorbisenc_static_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Release\vorbisenc.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Release\vorbisenc.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Debug\vorbisenc_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Debug\vorbisenc_d.pdb %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Debug\vorbisenc_d.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR

goto DONE

:ERROR

echo Some error(s) occurred. See output above...
goto EXIT

:notset
echo ***** Error: must set SRCROOT
goto exit

:DONE
cd %SRCROOT%\win32sdk
echo All done.
goto exit

:msdevnotset
echo ***** Error: must set MSDEVDIR
goto exit

:EXIT
