@echo off
rem
rem $Id: clean_all.bat,v 1.3 2001/10/20 21:12:37 cwolf Exp $
rem Call the make clean targets for each example program target
rem
if ."%USENMAKE%"==."" (
  msdev examples.dsw /make "examples - ALL" /clean /out clean.out
) else (
  call build_all.bat CLEAN 2> nul
)
