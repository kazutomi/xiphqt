@echo off
rem $Id: clean_dist.bat,v 1.5 2001/10/20 21:12:37 cwolf Exp $
rem
rd /s /q Debug\ 2> nul
rd /s /q Release\ 2> nul
del *.mak 2> nul
del *.dep 2> nul
del *.plg 2> nul
del *.ncb 2> nul
del out.ogg 2> nul
del out.pcm 2> nul
del execwait.exe 2> nul
del clean.out 2> nul
del build.out 2> nul
