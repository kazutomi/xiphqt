@echo off
rem
rem $Id: clean_all.bat,v 1.2 2001/10/20 17:58:24 cwolf Exp $
rem Call the make clean targets for each example program target
rem
call build_all.bat CLEAN 2> nul
