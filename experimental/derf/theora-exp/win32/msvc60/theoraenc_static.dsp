# Microsoft Developer Studio Project File - Name="theoraenc_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=theoraenc_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "theoraenc_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "theoraenc_static.mak" CFG="theoraenc_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "theoraenc_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "theoraenc_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "theoraenc_static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_theoraenc_static"
# PROP Intermediate_Dir "Release_theoraenc_static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /Ob2 /I "../../../../../trunk/ogg/include" /I "../../include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /U "OC_DUMP_IMAGES" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "theoraenc_static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_theoraenc_static"
# PROP Intermediate_Dir "Debug_theoraenc_static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /I "../../../../../trunk/ogg/include" /I "../../include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /U "OC_DUMP_IMAGES" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug_theoraenc_static\theoraenc_static_d.lib"

!ENDIF 

# Begin Target

# Name "theoraenc_static - Win32 Release"
# Name "theoraenc_static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\lib\bitrate.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\encinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\encmsc.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\encode.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\encvbr.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\enquant.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\fdct.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\huffenc.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\impmap.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\mcenc.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\psych.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\theora\codec.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\dct.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\encint.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\encvbr.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\enquant.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\fdct.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\huffenc.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\huffman.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\internal.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\ocintrin.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\psych.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\quant.h
# End Source File
# Begin Source File

SOURCE=..\..\include\theora\theoraenc.h
# End Source File
# End Group
# End Target
# End Project
