# Microsoft Developer Studio Project File - Name="colorconversions" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=colorconversions - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "colorconversions.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "colorconversions.mak" CFG="colorconversions - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "colorconversions - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "colorconversions - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "colorconversions - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\ObjectCode\ColorSpaces\Release"
# PROP Intermediate_Dir "..\..\..\ObjectCode\ColorSpaces\Release"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\include" /I "..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\win32\release\s_cconv.lib"

!ELSEIF  "$(CFG)" == "colorconversions - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\ObjectCode\ColorSpaces\Debug"
# PROP Intermediate_Dir "..\..\..\ObjectCode\ColorSpaces\Debug"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /GX /Z7 /Od /I "..\..\include" /I "..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\win32\debug\s_cconv.lib"

!ENDIF 

# Begin Target

# Name "colorconversions - Win32 Release"
# Name "colorconversions - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Win32\rgb24toyv12_mmx.asm

!IF  "$(CFG)" == "colorconversions - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Release
InputPath=.\Win32\rgb24toyv12_mmx.asm
InputName=rgb24toyv12_mmx

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "colorconversions - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Debug
InputPath=.\Win32\rgb24toyv12_mmx.asm
InputName=rgb24toyv12_mmx

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\rgb24toyv12_xmm.asm

!IF  "$(CFG)" == "colorconversions - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Release
InputPath=.\Win32\rgb24toyv12_xmm.asm
InputName=rgb24toyv12_xmm

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "colorconversions - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Debug
InputPath=.\Win32\rgb24toyv12_xmm.asm
InputName=rgb24toyv12_xmm

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\rgb32toyv12_mmx.asm

!IF  "$(CFG)" == "colorconversions - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Release
InputPath=.\Win32\rgb32toyv12_mmx.asm
InputName=rgb32toyv12_mmx

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "colorconversions - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Debug
InputPath=.\Win32\rgb32toyv12_mmx.asm
InputName=rgb32toyv12_mmx

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\rgb32toyv12_mmxlu.asm

!IF  "$(CFG)" == "colorconversions - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Release
InputPath=.\Win32\rgb32toyv12_mmxlu.asm
InputName=rgb32toyv12_mmxlu

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "colorconversions - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Debug
InputPath=.\Win32\rgb32toyv12_mmxlu.asm
InputName=rgb32toyv12_mmxlu

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\rgb32toyv12_xmm.asm

!IF  "$(CFG)" == "colorconversions - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Release
InputPath=.\Win32\rgb32toyv12_xmm.asm
InputName=rgb32toyv12_xmm

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "colorconversions - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\ColorSpaces\Debug
InputPath=.\Win32\rgb32toyv12_xmm.asm
InputName=rgb32toyv12_xmm

".\$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\uyvytoyv12_mmx.c
# End Source File
# Begin Source File

SOURCE=.\Win32\yuy2toyv12_mmx.c
# End Source File
# Begin Source File

SOURCE=.\Win32\yvyutoyv12_mmx.c
# End Source File
# End Group
# Begin Group "Generic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ColorConversions.c
# End Source File
# Begin Source File

SOURCE=.\lutbl.c
# End Source File
# Begin Source File

SOURCE=.\rgb24toyv12.c
# End Source File
# Begin Source File

SOURCE=.\rgb32toyv12.c
# End Source File
# Begin Source File

SOURCE=.\rgbtorgb.c
# End Source File
# Begin Source File

SOURCE=.\rgbtoyuv.c
# End Source File
# Begin Source File

SOURCE=.\uyvytoyv12.c
# End Source File
# Begin Source File

SOURCE=.\yuy2toyv12.c
# End Source File
# Begin Source File

SOURCE=.\yvyutoyv12.c
# End Source File
# End Group
# End Group
# Begin Group "External Dependecies"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ExternalDependencies\CPUIdLib.h
# End Source File
# End Group
# End Target
# End Project
