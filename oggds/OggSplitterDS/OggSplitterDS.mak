# Microsoft Developer Studio Generated NMAKE File, Based on OggSplitterDS.dsp
!IF "$(CFG)" == ""
CFG=OggSplitterDS - Win32 Release
!MESSAGE No configuration specified. Defaulting to OggSplitterDS - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "OggSplitterDS - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OggSplitterDS.mak" CFG="OggSplitterDS - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OggSplitterDS - Win32 Release" (based on "Win32 (x86) Static Library")
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

ALL : "$(OUTDIR)\OggSplitterDS.lib"


CLEAN :
	-@erase "$(INTDIR)\OggSplitPropPage.obj"
	-@erase "$(INTDIR)\OggSplitStream.obj"
	-@erase "$(INTDIR)\OggSplitterDS.obj"
	-@erase "$(INTDIR)\OggSplitterInput.obj"
	-@erase "$(INTDIR)\OggSplitterOutput.obj"
	-@erase "$(INTDIR)\SplitMediaSeeking.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\OggSplitterDS.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\OggSplitterDS.bsc" 
BSC32_SBRS= \
	
LIB32=xilink6.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\OggSplitterDS.lib" 
LIB32_OBJS= \
	"$(INTDIR)\OggSplitPropPage.obj" \
	"$(INTDIR)\OggSplitStream.obj" \
	"$(INTDIR)\OggSplitterDS.obj" \
	"$(INTDIR)\OggSplitterInput.obj" \
	"$(INTDIR)\OggSplitterOutput.obj" \
	"$(INTDIR)\SplitMediaSeeking.obj"

"$(OUTDIR)\OggSplitterDS.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

CPP_PROJ=/nologo /MD /W3 /Gi /GX /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\OggSplitterDS.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /O3 /Qsox- /c 

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
!IF EXISTS("OggSplitterDS.dep")
!INCLUDE "OggSplitterDS.dep"
!ELSE 
!MESSAGE Warning: cannot find "OggSplitterDS.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "OggSplitterDS - Win32 Release"
SOURCE=.\OggSplitPropPage.cpp

"$(INTDIR)\OggSplitPropPage.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggSplitStream.cpp

"$(INTDIR)\OggSplitStream.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggSplitterDS.cpp

"$(INTDIR)\OggSplitterDS.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggSplitterInput.cpp

"$(INTDIR)\OggSplitterInput.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggSplitterOutput.cpp

"$(INTDIR)\OggSplitterOutput.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\SplitMediaSeeking.cpp

"$(INTDIR)\SplitMediaSeeking.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

