# Microsoft Developer Studio Generated NMAKE File, Based on VorbDecDS.dsp
!IF "$(CFG)" == ""
CFG=VorbDecDS - Win32 Release
!MESSAGE No configuration specified. Defaulting to VorbDecDS - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "VorbDecDS - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VorbDecDS.mak" CFG="VorbDecDS - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VorbDecDS - Win32 Release" (based on "Win32 (x86) Static Library")
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

ALL : "$(OUTDIR)\VorbDecDS.lib"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\VorbDecDS.obj"
	-@erase "$(INTDIR)\VorbDecPropPage.obj"
	-@erase "$(OUTDIR)\VorbDecDS.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\VorbDecDS.bsc" 
BSC32_SBRS= \
	
LIB32=xilink6.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\VorbDecDS.lib" 
LIB32_OBJS= \
	"$(INTDIR)\VorbDecDS.obj" \
	"$(INTDIR)\VorbDecPropPage.obj"

"$(OUTDIR)\VorbDecDS.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

CPP_PROJ=/nologo /MD /W3 /Gi /GX /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\VorbDecDS.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /O3 /Qsox- /c 

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
!IF EXISTS("VorbDecDS.dep")
!INCLUDE "VorbDecDS.dep"
!ELSE 
!MESSAGE Warning: cannot find "VorbDecDS.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "VorbDecDS - Win32 Release"
SOURCE=.\VorbDecDS.cpp

"$(INTDIR)\VorbDecDS.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\VorbDecPropPage.cpp

"$(INTDIR)\VorbDecPropPage.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

