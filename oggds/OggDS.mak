# Microsoft Developer Studio Generated NMAKE File, Based on OggDS.dsp
!IF "$(CFG)" == ""
CFG=OggDS - Win32 Release
!MESSAGE No configuration specified. Defaulting to OggDS - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "OggDS - Win32 Release"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OggDS.mak" CFG="OggDS - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OggDS - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe
OUTDIR=.\Release
INTDIR=.\BuildTemp
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\OggDS.dll"

!ELSE 

ALL : "OggStream - Win32 Release" "VorbEncDS - Win32 Release" "VorbDecDS - Win32 Release" "OggSplitterDS - Win32 Release" "OggMuxDS - Win32 Release" "BaseClasses - Win32 Release" "$(OUTDIR)\OggDS.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"BaseClasses - Win32 ReleaseCLEAN" "OggMuxDS - Win32 ReleaseCLEAN" "OggSplitterDS - Win32 ReleaseCLEAN" "VorbDecDS - Win32 ReleaseCLEAN" "VorbEncDS - Win32 ReleaseCLEAN" "OggStream - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\AboutPage.obj"
	-@erase "$(INTDIR)\common.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\OggDS.res"
	-@erase "$(INTDIR)\OggPageQueue.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\OggDS.dll"
	-@erase "$(OUTDIR)\OggDS.exp"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\OggDS.bsc" 
BSC32_SBRS= \
	
LINK32=xilink6.exe
LINK32_FLAGS=winmm.lib quartz.lib vfw32.lib version.lib largeint.lib comctl32.lib olepro32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib odbc32.lib odbccp32.lib oleaut32.lib OggSDK/ogg.lib OggSDK/vorbis.lib OggSDK/vorbisenc.lib libmmt.lib libguide.lib libirc.lib msvcrt.lib /nologo /entry:"DllEntryPoint@12" /dll /incremental:no /pdb:"$(OUTDIR)\OggDS.pdb" /machine:I386 /nodefaultlib /def:".\OggDS.def" /out:"$(OUTDIR)\OggDS.dll" /implib:"$(OUTDIR)\OggDS.lib" 
DEF_FILE= \
	".\OggDS.def"
LINK32_OBJS= \
	"$(INTDIR)\AboutPage.obj" \
	"$(INTDIR)\common.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\OggPageQueue.obj" \
	"$(INTDIR)\OggDS.res" \
	"..\..\..\Programme\Microsoft SDK\Samples\Multimedia\directshow\baseclasses\Release\STRMBASE.lib" \
	".\Lib\OggMuxDS.lib" \
	".\Lib\OggSplitterDS.lib" \
	".\Lib\VorbDecDS.lib" \
	".\Lib\VorbEncDS.lib" \
	".\OggStream\Release\OggStream.lib"

"$(OUTDIR)\OggDS.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "OggStream - Win32 Release" "VorbEncDS - Win32 Release" "VorbDecDS - Win32 Release" "OggSplitterDS - Win32 Release" "OggMuxDS - Win32 Release" "BaseClasses - Win32 Release" "$(OUTDIR)\OggDS.dll"
   makensis ogg.nsi
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

CPP_PROJ=/nologo /MD /W3 /GX /D DBG=0 /D WINVER=0x400 /D _X86_=1 /D "_DLL" /D "_MT" /D "_WIN32" /D "WIN32" /D "STRICT" /D "INC_OLE2" /D try=__try /D except=__except /D leave=__leave /D finally=__finally /Fp"$(INTDIR)\OggDS.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /O3 /Qsox- /c 

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

MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x407 /fo"$(INTDIR)\OggDS.res" /d "NDEBUG" 

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("OggDS.dep")
!INCLUDE "OggDS.dep"
!ELSE 
!MESSAGE Warning: cannot find "OggDS.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "OggDS - Win32 Release"
SOURCE=.\AboutPage.cpp
CPP_SWITCHES=/nologo /MT /W3 /GX /D DBG=0 /D WINVER=0x400 /D _X86_=1 /D "_DLL" /D "_MT" /D "_WIN32" /D "WIN32" /D "STRICT" /D "INC_OLE2" /D try=__try /D except=__except /D leave=__leave /D finally=__finally /Fp"$(INTDIR)\OggDS.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /O3 /Qsox- /c 

"$(INTDIR)\AboutPage.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


SOURCE=.\common.cpp
CPP_SWITCHES=/nologo /MT /W3 /GX /D DBG=0 /D WINVER=0x400 /D _X86_=1 /D "_DLL" /D "_MT" /D "_WIN32" /D "WIN32" /D "STRICT" /D "INC_OLE2" /D try=__try /D except=__except /D leave=__leave /D finally=__finally /Fp"$(INTDIR)\OggDS.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /O3 /Qsox- /c 

"$(INTDIR)\common.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


SOURCE=.\main.cpp

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\OggPageQueue.cpp
CPP_SWITCHES=/nologo /MT /W3 /GX /D DBG=0 /D WINVER=0x400 /D _X86_=1 /D "_DLL" /D "_MT" /D "_WIN32" /D "WIN32" /D "STRICT" /D "INC_OLE2" /D try=__try /D except=__except /D leave=__leave /D finally=__finally /Fp"$(INTDIR)\OggDS.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /O3 /Qsox- /c 

"$(INTDIR)\OggPageQueue.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


SOURCE=.\OggDS.rc

"$(INTDIR)\OggDS.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


!IF  "$(CFG)" == "OggDS - Win32 Release"

"BaseClasses - Win32 Release" : 
   cd "\Programme\Microsoft SDK\Samples\Multimedia\directshow\baseclasses"
   $(MAKE) /$(MAKEFLAGS) /F ".\baseclasses.mak" CFG="BaseClasses - Win32 Release" 
   cd "..\..\..\..\..\..\Projects\c\ogg"

"BaseClasses - Win32 ReleaseCLEAN" : 
   cd "\Programme\Microsoft SDK\Samples\Multimedia\directshow\baseclasses"
   $(MAKE) /$(MAKEFLAGS) /F ".\baseclasses.mak" CFG="BaseClasses - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\..\..\..\..\Projects\c\ogg"

!ENDIF 

!IF  "$(CFG)" == "OggDS - Win32 Release"

"OggMuxDS - Win32 Release" : 
   cd ".\OggMuxDS"
   $(MAKE) /$(MAKEFLAGS) /F .\OggMuxDS.mak CFG="OggMuxDS - Win32 Release" 
   cd ".."

"OggMuxDS - Win32 ReleaseCLEAN" : 
   cd ".\OggMuxDS"
   $(MAKE) /$(MAKEFLAGS) /F .\OggMuxDS.mak CFG="OggMuxDS - Win32 Release" RECURSE=1 CLEAN 
   cd ".."

!ENDIF 

!IF  "$(CFG)" == "OggDS - Win32 Release"

"OggSplitterDS - Win32 Release" : 
   cd ".\OggSplitterDS"
   $(MAKE) /$(MAKEFLAGS) /F .\OggSplitterDS.mak CFG="OggSplitterDS - Win32 Release" 
   cd ".."

"OggSplitterDS - Win32 ReleaseCLEAN" : 
   cd ".\OggSplitterDS"
   $(MAKE) /$(MAKEFLAGS) /F .\OggSplitterDS.mak CFG="OggSplitterDS - Win32 Release" RECURSE=1 CLEAN 
   cd ".."

!ENDIF 

!IF  "$(CFG)" == "OggDS - Win32 Release"

"VorbDecDS - Win32 Release" : 
   cd ".\VorbDecDS"
   $(MAKE) /$(MAKEFLAGS) /F .\VorbDecDS.mak CFG="VorbDecDS - Win32 Release" 
   cd ".."

"VorbDecDS - Win32 ReleaseCLEAN" : 
   cd ".\VorbDecDS"
   $(MAKE) /$(MAKEFLAGS) /F .\VorbDecDS.mak CFG="VorbDecDS - Win32 Release" RECURSE=1 CLEAN 
   cd ".."

!ENDIF 

!IF  "$(CFG)" == "OggDS - Win32 Release"

"VorbEncDS - Win32 Release" : 
   cd ".\VorbEncDS"
   $(MAKE) /$(MAKEFLAGS) /F .\VorbEncDS.mak CFG="VorbEncDS - Win32 Release" 
   cd ".."

"VorbEncDS - Win32 ReleaseCLEAN" : 
   cd ".\VorbEncDS"
   $(MAKE) /$(MAKEFLAGS) /F .\VorbEncDS.mak CFG="VorbEncDS - Win32 Release" RECURSE=1 CLEAN 
   cd ".."

!ENDIF 

!IF  "$(CFG)" == "OggDS - Win32 Release"

"OggStream - Win32 Release" : 
   cd ".\OggStream"
   $(MAKE) /$(MAKEFLAGS) /F .\OggStream.mak CFG="OggStream - Win32 Release" 
   cd ".."

"OggStream - Win32 ReleaseCLEAN" : 
   cd ".\OggStream"
   $(MAKE) /$(MAKEFLAGS) /F .\OggStream.mak CFG="OggStream - Win32 Release" RECURSE=1 CLEAN 
   cd ".."

!ENDIF 


!ENDIF 

