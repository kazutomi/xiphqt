# Microsoft Developer Studio Project File - Name="vp3e" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vp3e - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vp3e.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vp3e.mak" CFG="vp3e - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vp3e - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vp3e - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vp3e - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Output_Dir "..\..\Lib\Win32\Release"
# PROP Intermediate_Dir "..\..\..\..\ObjectCode\vp3e\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3E_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\Include" /I "Include" /I "..\..\Include" /I "..\..\..\Include\VP31" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3E_EXPORTS" /D "PREDICT_2D" /D "VFW_COMP" /D "COMPDLL" /D "POSTPROCESS" /D "CPUISLITTLEENDIAN" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\Lib\Win32\Release\s_vp31e.lib"

!ELSEIF  "$(CFG)" == "vp3e - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Output_Dir "..\..\Lib\Win32\Debug"
# PROP Intermediate_Dir "..\..\..\..\ObjectCode\vp3e\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3E_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /GB /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\Include" /I "Include" /I "..\..\Include" /I "..\..\..\Include\VP31" /D "VP3E_EXPORTS" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PREDICT_2D" /D "VFW_COMP" /D "COMPDLL" /D "POSTPROCESS" /D "CPUISLITTLEENDIAN" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\Lib\Win32\Debug\s_vp31e.lib"

!ENDIF 

# Begin Target

# Name "vp3e - Win32 Release"
# Name "vp3e - Win32 Debug"
# Begin Group "Compress"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CX\Generic\CBitman.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\CEncode.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\CFrameW.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\CFrarray.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\Comp_Globals.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\CSystemDependant.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\DCT_encode.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\mcomp.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\misc_common.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\vfw_comp_main.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\vfwcomp.c
# End Source File
# Begin Source File

SOURCE=.\CX\Generic\vfwcomp_if.c
# End Source File
# End Group
# Begin Group "Common Code"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Common\Generic\Quantize.c
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cx\Win32\COptFunctions.c
# End Source File
# Begin Source File

SOURCE=.\CX\Win32\COptSystemDependant.c
# End Source File
# Begin Source File

SOURCE=.\cx\Win32\CWmtFunctions.c
# End Source File
# Begin Source File

SOURCE=.\cx\Win32\MmxEncodeMath.asm

!IF  "$(CFG)" == "vp3e - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3e\Release
InputPath=.\cx\Win32\MmxEncodeMath.asm
InputName=MmxEncodeMath

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vp3e - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3e\debug
InputPath=.\cx\Win32\MmxEncodeMath.asm
InputName=MmxEncodeMath

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cx\Win32\XmmGetError.asm

!IF  "$(CFG)" == "vp3e - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3e\Release
InputPath=.\cx\Win32\XmmGetError.asm
InputName=XmmGetError

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vp3e - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3e\debug
InputPath=.\cx\Win32\XmmGetError.asm
InputName=XmmGetError

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\CX\Win32\XmmGetSAD8.asm

!IF  "$(CFG)" == "vp3e - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3e\Release
InputPath=.\CX\Win32\XmmGetSAD8.asm
InputName=XmmGetSAD8

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vp3e - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3e\debug
InputPath=.\CX\Win32\XmmGetSAD8.asm
InputName=XmmGetSAD8

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cx\Win32\XmmSAD.asm

!IF  "$(CFG)" == "vp3e - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3e\Release
InputPath=.\cx\Win32\XmmSAD.asm
InputName=XmmSAD

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vp3e - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3e\debug
InputPath=.\cx\Win32\XmmSAD.asm
InputName=XmmSAD

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
