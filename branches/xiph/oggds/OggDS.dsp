# Microsoft Developer Studio Project File - Name="OggDS" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=OggDS - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "OggDS.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "OggDS.mak" CFG="OggDS - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "OggDS - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "BuildTemp"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OggDS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /D DBG=0 /D WINVER=0x400 /D _X86_=1 /D "_DLL" /D "_MT" /D "_WIN32" /D "WIN32" /D "STRICT" /D "INC_OLE2" /D try=__try /D except=__except /D leave=__leave /D finally=__finally /YX /FD /O3 /Qsox- /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 winmm.lib quartz.lib vfw32.lib version.lib largeint.lib comctl32.lib olepro32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib uuid.lib odbc32.lib odbccp32.lib oleaut32.lib OggSDK/ogg.lib OggSDK/vorbis.lib OggSDK/vorbisenc.lib libmmt.lib libguide.lib libirc.lib msvcrt.lib /nologo /entry:"DllEntryPoint@12" /dll /machine:I386 /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=makensis ogg.nsi
# End Special Build Tool
# Begin Target

# Name "OggDS - Win32 Release"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AboutPage.cpp
# ADD CPP /MT
# End Source File
# Begin Source File

SOURCE=.\common.cpp
# ADD CPP /MT
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\OggDS.def
# End Source File
# Begin Source File

SOURCE=.\OggPageQueue.cpp
# ADD CPP /MT
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AboutPage.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\OggDS.h
# End Source File
# Begin Source File

SOURCE=.\OggPageQueue.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\VersionNo.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ogg.bmp
# End Source File
# Begin Source File

SOURCE=.\OggDS.rc
# End Source File
# Begin Source File

SOURCE=.\OggDS.rc2
# End Source File
# Begin Source File

SOURCE=.\xfish.bmp
# End Source File
# Begin Source File

SOURCE=.\xfish1.ico
# End Source File
# Begin Source File

SOURCE=.\xfish2.ico
# End Source File
# End Group
# Begin Group "Documentation"

# PROP Default_Filter "*.txt"
# Begin Source File

SOURCE=.\Install\FAQs.txt
# End Source File
# Begin Source File

SOURCE=.\Install\History.txt
# End Source File
# Begin Source File

SOURCE=.\Install\license.txt
# End Source File
# Begin Source File

SOURCE=.\Install\welcome.htm
# End Source File
# End Group
# End Target
# End Project
