@echo off
rem $Id: build_all.bat,v 1.4 2001/10/18 03:18:56 cwolf Exp $
rem
rem Invoke as "build_all.bat CLEAN" to clean all targets
rem
if ."%SDKHOME%"==."" goto notset

if not exist %SDKHOME%\lib\ogg.lib goto nolib

rem If one of the makefiles doesn't exist,
rem assume they all need to be generated
rem
if not exist %SDKHOME%\build\examples.mak (
  echo Must run "mkmak.bat" first...
  goto done
)

nmake /nologo /f encoder.mak CFG="encoder - Win32 Debug" %1
nmake /nologo /f encoder.mak CFG="encoder - Win32 Release" %1
nmake /nologo /f encoder_static.mak CFG="encoder_static - Win32 Debug" %1
nmake /nologo /f encoder_static.mak CFG="encoder_static - Win32 Release" %1
nmake /nologo /f decoder.mak CFG="decoder - Win32 Debug" %1
nmake /nologo /f decoder.mak CFG="decoder - Win32 Release" %1
nmake /nologo /f decoder_static.mak CFG="decoder_static - Win32 Debug" %1
nmake /nologo /f decoder_static.mak CFG="decoder_static - Win32 Release" %1
nmake /nologo /f chaining.mak CFG="chaining - Win32 Debug" %1
nmake /nologo /f chaining.mak CFG="chaining - Win32 Release" %1
nmake /nologo /f chaining_static.mak CFG="chaining_static - Win32 Debug" %1
nmake /nologo /f chaining_static.mak CFG="chaining_static - Win32 Release" %1
nmake /nologo /f seeking.mak CFG="seeking - Win32 Debug" %1
nmake /nologo /f seeking.mak CFG="seeking - Win32 Release" %1
nmake /nologo /f seeking_static.mak CFG="seeking_static - Win32 Debug" %1
nmake /nologo /f seeking_static.mak CFG="seeking_static - Win32 Release" %1
nmake /nologo /f vorbisfile.mak CFG="vorbisfile - Win32 Debug" %1
nmake /nologo /f vorbisfile.mak CFG="vorbisfile - Win32 Release" %1
nmake /nologo /f vorbisfile_static.mak CFG="vorbisfile_static - Win32 Debug" %1
nmake /nologo /f vorbisfile_static.mak CFG="vorbisfile_static - Win32 Release" %1

goto done

:nolib
echo ***** SDK needs to be built first, run win32sdk\maksdk.bat
goto done

:notset
echo ***** Error: must set SDKHOME
goto done

:done
