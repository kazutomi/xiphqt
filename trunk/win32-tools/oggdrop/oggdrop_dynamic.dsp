# Microsoft Developer Studio Project File - Name="oggdrop_dynamic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=oggdrop_dynamic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "oggdrop_dynamic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "oggdrop_dynamic.mak" CFG="oggdrop_dynamic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "oggdrop_dynamic - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "oggdrop_dynamic - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "oggdrop_dynamic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "oggdrop_dynamic___Win32_Release"
# PROP BASE Intermediate_Dir "oggdrop_dynamic___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release/dynamic"
# PROP Intermediate_Dir "Release/dynamic"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\ogg\include" /I "..\..\vorbis\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ogg.lib vorbis.lib vorbisenc.lib /nologo /subsystem:windows /machine:I386 /out:"Release/dynamic/oggdrop.exe" /libpath:"..\..\ogg\win32\Dynamic_Release" /libpath:"..\..\vorbis\win32\Vorbis_Dynamic_Release" /libpath:"..\..\vorbis\win32\VorbisEnc_Dynamic_Release"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "oggdrop_dynamic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "oggdrop_dynamic___Win32_Debug"
# PROP BASE Intermediate_Dir "oggdrop_dynamic___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug/dynamic"
# PROP Intermediate_Dir "Debug/dynamic"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\ogg\include" /I "..\..\vorbis\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ogg_d.lib vorbis_d.lib vorbisenc_d.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/dynamic/oggdrop.exe" /pdbtype:sept /libpath:"..\..\ogg\win32\Dynamic_Debug" /libpath:"..\..\vorbis\win32\Vorbis_Dynamic_Debug" /libpath:"..\..\vorbis\win32\VorbisEnc_Dynamic_Debug"
# SUBTRACT LINK32 /verbose /pdb:none

!ENDIF 

# Begin Target

# Name "oggdrop_dynamic - Win32 Release"
# Name "oggdrop_dynamic - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\audio.c
# End Source File
# Begin Source File

SOURCE=.\encode.c
# End Source File
# Begin Source File

SOURCE=.\encthread.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\oe_win32.c
# End Source File
# Begin Source File

SOURCE=.\Script.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\audio.h
# End Source File
# Begin Source File

SOURCE=.\encode.h
# End Source File
# Begin Source File

SOURCE=.\encthread.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\resource\fish.ico
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish01.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish02.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish03.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish04.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish05.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish06.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish07.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish08.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish09.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish10.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish11.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\twirlfish12.bmp
# End Source File
# End Group
# End Target
# End Project
