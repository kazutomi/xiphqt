@echo off
rem $Id: clean_dist.bat,v 1.1 2001/09/15 08:09:23 cwolf Exp $
rem
rd /s /q Debug\ 2> nul
rd /s /q Release\ 2> nul
rd /s /q sdk\include 2> nul
rd /s /q sdk\lib 2> nul
rd /s /q sdk\bin 2> nul
rd /s /q sdk\doc 2> nul
rd /s /q sdk\examples 2> nul
del *.mak 2> nul
del *.dep 2> nul
del *.plg 2> nul
del *.ncb 2> nul
cd sdk\build
call clean_dist.bat
