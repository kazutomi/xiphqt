# Microsoft Developer Studio Project File - Name="vp31vfw" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vp31vfw - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vp31vfw.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vp31vfw.mak" CFG="vp31vfw - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vp31vfw - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vp31vfw - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vp31vfw - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP31VFW_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GX /O2 /I "..\..\Include" /I "..\..\..\Include" /I "Include" /I "..\..\..\Include\VP31" /I "..\..\Include\VP31" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP31VFW_EXPORTS" /D "VP30_COMPRESS" /D "DXV_DECOMPRESS" /FD /opt:ref /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /pdb:none /map /machine:I386 /nodefaultlib:"bsrx86w.lib" /nodefaultlib:"wksx86w.lib" /libpath:"..\..\Lib\Win32\Release\\" /opt:ref

!ELSEIF  "$(CFG)" == "vp31vfw - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP31VFW_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GX /Zi /Od /I "..\..\Include" /I "..\..\..\Include" /I "Include" /I "..\..\..\Include\VP31" /I "..\..\Include\VP31" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP31VFW_EXPORTS" /D "VP30_COMPRESS" /D "DXV_DECOMPRESS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /incremental:no /map /debug /machine:I386 /nodefaultlib:"bsrx86w.lib" /nodefaultlib:"wksx86w.lib" /pdbtype:sept /libpath:"..\..\Lib\Win32\Debug\\"
# Begin Custom Build
TargetPath=.\Debug\vp31vfw.dll
InputPath=.\Debug\vp31vfw.dll
SOURCE="$(InputPath)"

"c:\winnt\system32\vp31vfw.dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy $(TargetPath) c:\winnt\system32

# End Custom Build

!ENDIF 

# Begin Target

# Name "vp31vfw - Win32 Release"
# Name "vp31vfw - Win32 Debug"
# Begin Source File

SOURCE=.\Win32\drvproci.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32\LOGOSM.bmp
# End Source File
# Begin Source File

SOURCE=.\Win32\regentry.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32\regentry.h
# End Source File
# Begin Source File

SOURCE=.\Win32\resource.h
# End Source File
# Begin Source File

SOURCE=.\Win32\vfw_config_dlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32\vp3vfw.rc
# ADD BASE RSC /l 0x409 /i "Win32"
# ADD RSC /l 0x409 /i "Win32" /i "..\..\include"
# End Source File
# Begin Source File

SOURCE=.\Win32\vpvfw.cpp
# End Source File
# Begin Source File

SOURCE=.\Win32\vpvfw.def
# End Source File
# Begin Source File

SOURCE=.\Win32\vpvfw.h
# End Source File
# End Target
# End Project
