# Microsoft Developer Studio Project File - Name="tarkin_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=tarkin_lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tarkin_lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tarkin_lib.mak" CFG="tarkin_lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tarkin_lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "tarkin_lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tarkin_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G5 /W3 /O2 /I "..\..\ogg\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "RLECODER" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "tarkin_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G5 /W3 /ZI /Od /I "..\..\ogg\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "DBG_XFORM" /D "RLECODER" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "tarkin_lib - Win32 Release"
# Name "tarkin_lib - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\ogg\src\bitwise.c
# End Source File
# Begin Source File

SOURCE=..\..\ogg\src\framing.c
# End Source File
# Begin Source File

SOURCE=..\info.c
# End Source File
# Begin Source File

SOURCE=..\mem.c
# End Source File
# Begin Source File

SOURCE=..\pnm.c
# End Source File
# Begin Source File

SOURCE=..\tarkin.c
# End Source File
# Begin Source File

SOURCE=..\wavelet.c
# End Source File
# Begin Source File

SOURCE=..\wavelet_coeff.c
# End Source File
# Begin Source File

SOURCE=..\wavelet_xform.c
# End Source File
# Begin Source File

SOURCE=..\yuv.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\bitcoder.h
# End Source File
# Begin Source File

SOURCE=..\golomb.h
# End Source File
# Begin Source File

SOURCE=..\mem.h
# End Source File
# Begin Source File

SOURCE=..\..\ogg\include\ogg\ogg.h
# End Source File
# Begin Source File

SOURCE=..\..\ogg\include\ogg\os_types.h
# End Source File
# Begin Source File

SOURCE=..\pnm.h
# End Source File
# Begin Source File

SOURCE=..\rle.h
# End Source File
# Begin Source File

SOURCE=..\tarkin.h
# End Source File
# Begin Source File

SOURCE=..\w3dtypes.h
# End Source File
# Begin Source File

SOURCE=..\wavelet.h
# End Source File
# Begin Source File

SOURCE=..\yuv.h
# End Source File
# End Group
# End Target
# End Project
