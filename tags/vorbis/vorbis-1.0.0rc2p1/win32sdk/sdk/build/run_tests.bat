@echo off
rem $Id: run_tests.bat,v 1.3 2001/09/16 23:52:57 cwolf Exp $
rem
rem  Not using setlocal, so it will work on windows 9x
set OLDPATH=%PATH%
set PATH=..\bin
rem
if not exist ..\bin\ goto nosdk
if not exist ..\bin\encoder_static_d.exe goto notbuilt

echo **** encoder_static_d ****
encoder_static_d < test.wav > out.ogg

echo **** encoder_static ****
encoder_static < test.wav > out.ogg

echo **** encoder_d ****
encoder_d < test.wav > out.ogg

echo **** encoder ****
encoder < test.wav > out.ogg

echo **** decoder_static_d ****
decoder_static_d < test.ogg > out.pcm

echo **** decoder_static ****
decoder_static < test.ogg > out.pcm

echo **** decoder_d ****
decoder_d < test.ogg > out.pcm

echo **** decoder ****
decoder < test.ogg > out.pcm

echo **** vorbisfile_static_d ****
vorbisfile_static_d < test.ogg > out.pcm

echo **** vorbisfile_static ****
vorbisfile_static < test.ogg > out.pcm

echo **** vorbisfile_d ****
vorbisfile_d < test.ogg > out.pcm

echo **** vorbisfile ****
vorbisfile < test.ogg > out.pcm

echo **** chaining_static_d ****
chaining_static_d < test.ogg

echo **** chaining_static ****
chaining_static < test.ogg

echo **** chaining_d ****
chaining_d < test.ogg

echo **** chaining ****
chaining < test.ogg

echo **** seeking_static_d ****
seeking_static_d < test.ogg

echo **** seeking_static ****
seeking_static < test.ogg

echo **** seeking_d ****
seeking_d < test.ogg

echo **** seeking ****
seeking < test.ogg
goto done

:nosdk
echo Error: must build sdk, run win32sdk\makesdk.bat
goto done

:notbuilt
echo Error: must build example programs, run build_all.bat
goto done

:done
rem not using endlocal, so it will work on windows 9x
set PATH=%OLDPATH%
