# Microsoft Developer Studio Project File - Name="vp3p" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vp3p - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vp3p.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vp3p.mak" CFG="vp3p - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vp3p - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vp3p - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vp3p - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "vp3p___Win32_Release"
# PROP BASE Intermediate_Dir "vp3p___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\Lib\Win32\Release"
# PROP Intermediate_Dir "..\..\..\..\ObjectCode\vp3p\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3E_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\Include" /I "Include" /I "..\..\Include" /I "..\..\..\Include\VP31" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3E_EXPORTS" /D "PREDICT_2D" /D "VFW_COMP" /D "COMPDLL" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\Lib\Win32\Release\s_vp31p.lib"

!ELSEIF  "$(CFG)" == "vp3p - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vp3p___Win32_Debug"
# PROP BASE Intermediate_Dir "vp3p___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\Lib\Win32\Debug"
# PROP Intermediate_Dir "..\..\..\..\ObjectCode\vp3p\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3E_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /GB /MTd /W3 /Gm /GX /Zi /Od /I "..\..\..\Include" /I "Include" /I "..\..\Include" /I "..\..\..\Include\VP31" /D "VP3E_EXPORTS" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "PREDICT_2D" /D "VFW_COMP" /D "COMPDLL" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\Lib\Win32\Debug\s_vp31p.lib"

!ENDIF 

# Begin Target

# Name "vp3p - Win32 Release"
# Name "vp3p - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\PP\Generic\BlockMap.c
# End Source File
# Begin Source File

SOURCE=.\PP\Generic\Cscanyuv.c
# End Source File
# Begin Source File

SOURCE=.\PP\Generic\PreprocGlobals.c
# End Source File
# Begin Source File

SOURCE=.\pp\Generic\PreprocIf.c
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\pp\Win32\PreprocOptFunctions.c
# End Source File
# Begin Source File

SOURCE=.\pp\Win32\XmmRowSAD.asm

!IF  "$(CFG)" == "vp3p - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3p\Release
InputPath=.\pp\Win32\XmmRowSAD.asm
InputName=XmmRowSAD

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vp3p - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3p\debug
InputPath=.\pp\Win32\XmmRowSAD.asm
InputName=XmmRowSAD

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
