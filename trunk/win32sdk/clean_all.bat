@echo off
rem
rem $Id: clean_all.bat,v 1.3 2001/10/20 21:12:34 cwolf Exp $
rem Call the make clean targets in each respective modules' subdirectory
rem
if ."%USENMAKE%"==."" (
  msdev all.dsw /make "all - ALL" /clean /out clean.out
) else (
  call build_all.bat CLEAN 2> nul
)
