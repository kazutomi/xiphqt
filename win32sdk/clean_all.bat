@echo off
rem
rem $Id: clean_all.bat,v 1.2 2001/09/15 07:03:06 cwolf Exp $
rem Call the make clean targets in each respective modules' subdirectory
rem
call build_all.bat CLEAN 2> nul
