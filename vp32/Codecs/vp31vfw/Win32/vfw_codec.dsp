# Microsoft Developer Studio Project File - Name="vfw_codec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vfw_codec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vfw_codec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vfw_codec.mak" CFG="vfw_codec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vfw_codec - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vfw_codec - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vfw_codec - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VFW_CODEC_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "c:\ducksoft\vp3\codec" /I "c:\ducksoft\vp3\codec\common_code" /I "c:\ducksoft\vp3\codec\pbdll" /I "c:\ducksoft\vp3\codec\compdll" /I "c:\ducksoft\vp3\codec\dct" /I "c:\ducksoft\vp3\codec\dswv" /I "c:\ducksoft\vp3\codec\dxref" /I "c:\ducksoft\vp3\codec\vgrab" /I "c:\ducksoft\vp3\codec\preprocessor" /I "c:\ducksoft\vp3\codec\comms" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VFW_CODEC_EXPORTS" /D "VP30_COMPRESS" /D "DXV_DECOMPRESS" /D "VP30" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib s_dxv.lib s_vp3glue.lib cclib.lib winmm.lib /nologo /dll /pdb:none /machine:I386 /out:"Release/vp3vfw.dll" /OPT:REF /OPT:ICF,2
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to Win Sys
PostBuild_Cmds=copy c:\ducksoft\vp3\vfw_codec\release\vp3vfw.dll c:\windows\system
# End Special Build Tool

!ELSEIF  "$(CFG)" == "vfw_codec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VFW_CODEC_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "c:\ducksoft\vp3\codec" /I "c:\ducksoft\vp3\codec\common_code" /I "c:\ducksoft\vp3\codec\pbdll" /I "c:\ducksoft\vp3\codec\compdll" /I "c:\ducksoft\vp3\codec\dct" /I "c:\ducksoft\vp3\codec\dswv" /I "c:\ducksoft\vp3\codec\dxref" /I "c:\ducksoft\vp3\codec\vgrab" /I "c:\ducksoft\vp3\codec\preprocessor" /I "c:\ducksoft\vp3\codec\comms" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VFW_CODEC_EXPORTS" /D "VP30_COMPRESS" /D "DXV_DECOMPRESS" /D "VP30" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib s_dxv.lib s_vp3glue.lib cclib.lib winmm.lib /nologo /dll /debug /machine:I386 /out:"Debug/vp3vfw.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copying to Win Sys
PostBuild_Cmds=copy c:\ducksoft\vp3\vfw_codec\debug\vp3vfw.dll c:\windows\system
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "vfw_codec - Win32 Release"
# Name "vfw_codec - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\drvproci.cpp
# End Source File
# Begin Source File

SOURCE=.\regentry.cpp
# End Source File
# Begin Source File

SOURCE=.\testmain.c
# End Source File
# Begin Source File

SOURCE=.\TM20.cpp
# End Source File
# Begin Source File

SOURCE=.\TM20.def
# End Source File
# Begin Source File

SOURCE=.\vfw_config_dlg.c
# End Source File
# Begin Source File

SOURCE=.\vp3vfw.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\compddk.h
# End Source File
# Begin Source File

SOURCE=.\confdll.h
# End Source File
# Begin Source File

SOURCE=.\duck_dxl.h
# End Source File
# Begin Source File

SOURCE=.\Icomp.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\TM20.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\codec\qt_test\dxlqt_codec.r
# End Source File
# End Group
# End Target
# End Project
