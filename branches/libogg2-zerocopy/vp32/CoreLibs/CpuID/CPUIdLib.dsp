# Microsoft Developer Studio Project File - Name="CPUIdLib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=CPUIdLib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CPUIdLib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CPUIdLib.mak" CFG="CPUIdLib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CPUIdLib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "CPUIdLib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CPUIdLib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\ObjectCode\cpuID\release"
# PROP Intermediate_Dir "..\..\..\ObjectCode\cpuID\release"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\include" /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\win32\Release\s_cpuid.lib"

!ELSEIF  "$(CFG)" == "CPUIdLib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\ObjectCode\cpuID\debug"
# PROP Intermediate_Dir "..\..\..\ObjectCode\cpuID\debug"
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
# ADD LIB32 /nologo /out:"..\..\lib\win32\debug\s_cpuid.lib"

!ENDIF 

# Begin Target

# Name "CPUIdLib - Win32 Release"
# Name "CPUIdLib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Win32\cid.c
# End Source File
# Begin Source File

SOURCE=.\Win32\cpuid.asm

!IF  "$(CFG)" == "CPUIdLib - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\cpuID\release
InputPath=.\Win32\cpuid.asm
InputName=cpuid

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "CPUIdLib - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\cpuID\debug
InputPath=.\Win32\cpuid.asm
InputName=cpuid

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\InitXMMReg.asm

!IF  "$(CFG)" == "CPUIdLib - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\cpuID\release
InputPath=.\Win32\InitXMMReg.asm
InputName=InitXMMReg

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "CPUIdLib - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\cpuID\debug
InputPath=.\Win32\InitXMMReg.asm
InputName=InitXMMReg

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\TrashXMMreg.asm

!IF  "$(CFG)" == "CPUIdLib - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\cpuID\release
InputPath=.\Win32\TrashXMMreg.asm
InputName=TrashXMMreg

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "CPUIdLib - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\cpuID\debug
InputPath=.\Win32\TrashXMMreg.asm
InputName=TrashXMMreg

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\VerifyXMMReg.asm

!IF  "$(CFG)" == "CPUIdLib - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\cpuID\release
InputPath=.\Win32\VerifyXMMReg.asm
InputName=VerifyXMMReg

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "CPUIdLib - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\ObjectCode\cpuID\debug
InputPath=.\Win32\VerifyXMMReg.asm
InputName=VerifyXMMReg

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\Wmt_CpuID.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
