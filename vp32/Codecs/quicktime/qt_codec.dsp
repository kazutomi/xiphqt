# Microsoft Developer Studio Project File - Name="qt_codec" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=qt_codec - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "qt_codec.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "qt_codec.mak" CFG="qt_codec - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "qt_codec - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "qt_codec - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "qt_codec - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "QT_CODEC_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "\qtwsdk4\cincludes" /I "\qtwsdk4\rincludes" /I "\qtwsdk4\componentincludes" /I "..\..\Include" /I "..\..\..\Include" /I "Include" /I "..\..\..\Include\VP30" /I "..\..\Include\VP31" /I ".\win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "QT_CODEC_EXPORTS" /D "COMP_BUILD" /D VP3_COMPRESS=1 /D COMP_BUILD=1 /D "DXV_DECOMPRESS" /D "COMP_ENABLED" /FD /c
# SUBTRACT CPP /WX /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib qtmlclient.lib s_dxv.lib s_sal.lib s_vp31d.lib s_vp31e.lib s_vp31p.lib s_vpxblit.lib s_cconv.lib s_cpuid.lib winmm.lib /nologo /dll /incremental:yes /pdb:"Release/on2_vp3p.pdb" /machine:I386 /out:"Release/On2_VP3.dll" /libpath:"..\..\Lib\Win32\Release"
# SUBTRACT LINK32 /pdb:none
# Begin Custom Build - Performing Rezwack on $(InputPath)
InputPath=.\Release\On2_VP3.dll
SOURCE="$(InputPath)"

".\release\On2_VP3.qtx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	del .\release\On2_VP3.qtx 
	rezwack -d $(InputPath) -r .\release\On2_VP3.qtr -o .\release\On2_VP3.qtx 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=set read only
PostBuild_Cmds=attrib -r .\release\On2_VP3.qtx
# End Special Build Tool

!ELSEIF  "$(CFG)" == "qt_codec - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "qt_codec___Win32_Debug"
# PROP BASE Intermediate_Dir "qt_codec___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "QT_CODEC_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\Include\VP31" /I "..\..\include" /I ".\win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "QT_CODEC_EXPORTS" /D "COMP_BUILD" /D VP3_COMPRESS=1 /D COMP_BUILD=1 /D "DXV_DECOMPRESS" /D "COMP_ENABLED" /Fr /FD /GZ /c
# SUBTRACT CPP /WX /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib qtmlclient.lib s_dxv.lib s_sal.lib s_vp31d.lib s_vp31e.lib s_vp31p.lib s_vpxblit.lib s_cconv.lib s_cpuid.lib winmm.lib /nologo /dll /debug /machine:I386 /out:"Debug/On2_VP3.dll" /pdbtype:sept /libpath:"..\..\Lib\Win32\Debug\\"
# SUBTRACT LINK32 /pdb:none /incremental:no
# Begin Custom Build - Performing Rezwack on $(InputPath)
InputPath=.\Debug\On2_VP3.dll
SOURCE="$(InputPath)"

".\debug\On2_VP3.qtx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	del .\debug\On2_VP3.qtx 
	rezwack -d $(InputPath) -r .\debug\On2_VP3.qtr -o .\debug\On2_VP3.qtx 
	
# End Custom Build
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Clean and Copy To System
PostBuild_Cmds=attrib -r .\debug\On2_VP3.qtx	copy .\debug\On2_VP3.qtx c:\winnt\system32\QuickTime\On2_VP3d.qtx
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "qt_codec - Win32 Release"
# Name "qt_codec - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Win32\dxlqt.def
# End Source File
# Begin Source File

SOURCE=.\Win32\dxlqt.rc
# End Source File
# Begin Source File

SOURCE=.\Generic\dxlqt_cx.cpp
# End Source File
# Begin Source File

SOURCE=.\Generic\dxlqt_default.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32\dxlqt_dllmain.c
# End Source File
# Begin Source File

SOURCE=.\Generic\dxlqt_dx.cpp
# End Source File
# Begin Source File

SOURCE=.\Generic\dxlqt_helper.cpp
# End Source File
# Begin Source File

SOURCE=.\Generic\dxlqt_x.cpp
# End Source File
# Begin Source File

SOURCE=.\win32\regentry.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32\vfw_config_dlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Generic\dxlqt_codec.r

!IF  "$(CFG)" == "qt_codec - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Performing Rez on $(InputPath)
TargetPath=.\Release\On2_VP3.dll
InputPath=.\Generic\dxlqt_codec.r

".\release\On2_VP3.qtr" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Rez $(InputPath)  -i c:\vp3\qt6sdk\qtdevwin\RIncludes -o $(TargetPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "qt_codec - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build - Performing Rez on $(InputPath)
TargetPath=.\Debug\On2_VP3.dll
InputPath=.\Generic\dxlqt_codec.r

".\debug\On2_VP3.qtr" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	Rez $(InputPath) -i c:\vp3\qt6sdk\qtdevwin\RIncludes -o $(TargetPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Win32\on2splash.bmp
# End Source File
# End Group
# End Target
# End Project
