# Microsoft Developer Studio Generated NMAKE File, Based on OggMuxDS.dsp
!IF "$(CFG)" == ""
CFG=OggMuxDS - Win32 Release
!MESSAGE No configuration specified. Defaulting to OggMuxDS - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "OggMuxDS - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OggMuxDS.mak" CFG="OggMuxDS - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OggMuxDS - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=xicl6.exe
RSC=rc.exe
OUTDIR=.\..\Lib
INTDIR=.\..\BuildTemp
# Begin Custom Macros
OutDir=.\..\Lib
# End Custom Macros

ALL : "$(OUTDIR)\OggMuxDS.lib"


CLEAN :
	-@erase "$(INTDIR)\MuxMediaSeeking.obj"
	-@erase "$(INTDIR)\OggMuxDS.obj"
	-@erase "$(INTDIR)\OggMuxInput.obj"
	-@erase "$(INTDIR)\OggMuxOutput.obj"
	-@erase "$(INTDIR)\OggMuxPropPage.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\OggMuxDS.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\OggMuxDS.bsc" 
BSC32_SBRS= \
	
LIB32=xilink6.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\OggMuxDS.lib" 
LIB32_OBJS= \
	"$(INTDIR)\MuxMediaSeeking.obj" \
	"$(INTDIR)\OggMuxDS.obj" \
	"$(INTDIR)\OggMuxInput.obj" \
	"$(INTDIR)\OggMuxOutput.obj" \
	"$(INTDIR)\OggMuxPropPage.obj"

"$(OUTDIR)\OggMuxDS.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

CPP_PROJ=/nologo /MD /W3 /Gi /GX /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\OggMuxDS.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /O3 /Qsox- /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("OggMuxDS.dep")
!INCLUDE "OggMuxDS.dep"
!ELSE 
!MESSAGE Warning: cannot find "OggMuxDS.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "OggMuxDS - Win32 Release"
SOURCE=.\MuxMediaSeeking.cpp

"$(INTDIR)\MuxMediaSeeking.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggMuxDS.cpp

"$(INTDIR)\OggMuxDS.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggMuxInput.cpp

"$(INTDIR)\OggMuxInput.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggMuxOutput.cpp

"$(INTDIR)\OggMuxOutput.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggMuxPropPage.cpp

"$(INTDIR)\OggMuxPropPage.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

