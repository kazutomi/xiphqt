@echo off
rem $Id: clean_dist.bat,v 1.1 2001/09/15 07:25:36 cwolf Exp $
rem
rd /s /q Debug\ 2> nul
rd /s /q Release\ 2> nul
del *.mak 2> nul
del *.dep 2> nul
del *.plg 2> nul
del *.ncb 2> nul
