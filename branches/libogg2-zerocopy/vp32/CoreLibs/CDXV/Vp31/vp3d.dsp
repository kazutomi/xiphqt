# Microsoft Developer Studio Project File - Name="vp3d" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vp3d - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vp3d.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vp3d.mak" CFG="vp3d - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vp3d - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vp3d - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vp3d - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Output_Dir "SRelease"
# PROP BASE Intermediate_Dir "SRelease"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Output_Dir "..\..\Lib\Win32\Release"
# PROP Intermediate_Dir "..\..\..\..\ObjectCode\vp3d\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE CPP /nologo /G5 /MD /W3 /GX /O2 /Ob2 /I "Include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3D_EXPORTS" /D "PREDICT_2D" /D "PBDLL" /D "VFW_PB" /D "USE_DRAWDIB" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MT /W3 /GX /O2 /Ob2 /I "Include ..\..\Include" /I "..\..\Include" /I "..\..\..\Include" /I "Include" /I "..\..\..\Include\VP31" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3D_EXPORTS" /D "PREDICT_2D" /D "PBDLL" /D "VFW_PB" /D "USE_DRAWDIB" /D "POSTPROCESS" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\Lib\Win32\Release\s_vp31d.lib"

!ELSEIF  "$(CFG)" == "vp3d - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Output_Dir "..\..\Lib\Win32\Debug"
# PROP Intermediate_Dir "..\..\..\..\ObjectCode\vp3d\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE CPP /nologo /G5 /MDd /W3 /Gm /GX /Zi /Od /I "Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3D_EXPORTS" /D "PREDICT_2D" /D "PBDLL" /D "VFW_PB" /D "USE_DRAWDIB" /YX /FD /c
# ADD CPP /nologo /GB /MTd /W3 /Gm /GX /Zi /Od /I "Include ..\..\Include" /I "..\..\..\Include" /I "Include" /I "..\..\Include" /I "..\..\..\Include\VP31" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VP3D_EXPORTS" /D "PREDICT_2D" /D "PBDLL" /D "VFW_PB" /D "USE_DRAWDIB" /D "POSTPROCESS" /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\Lib\Win32\Debug\s_vp31d.lib"

!ENDIF 

# Begin Target

# Name "vp3d - Win32 Release"
# Name "vp3d - Win32 Debug"
# Begin Group "Common Code"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Common\Generic\BlockMapping.c
# End Source File
# Begin Source File

SOURCE=.\Common\Generic\dct_globals.c
# End Source File
# Begin Source File

SOURCE=.\Common\Generic\fdct.c
# End Source File
# Begin Source File

SOURCE=.\Common\Generic\FrameIni.c
# End Source File
# Begin Source File

SOURCE=.\Common\Generic\Huffman.c
# End Source File
# Begin Source File

SOURCE=.\Common\Generic\Quantize.c
# End Source File
# Begin Source File

SOURCE=.\Common\Generic\Reconstruct.c
# End Source File
# Begin Source File

SOURCE=.\dx\Generic\vp31dxv.c
# End Source File
# Begin Source File

SOURCE=.\Common\Generic\YUVtofromRGB.c
# End Source File
# End Group
# Begin Group "Decompress"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DX\Generic\DCT_decode.c
# End Source File
# Begin Source File

SOURCE=.\DX\Generic\DDecode.c
# End Source File
# Begin Source File

SOURCE=.\DX\Generic\DFrameR.c
# End Source File
# Begin Source File

SOURCE=.\DX\Generic\DSystemDependant.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\DX\Generic\Frarray.c
# End Source File
# Begin Source File

SOURCE=.\dx\Generic\GetInfo.c
# End Source File
# Begin Source File

SOURCE=.\DX\Generic\IDctPart.c
# End Source File
# Begin Source File

SOURCE=.\DX\Generic\pb_globals.c
# End Source File
# Begin Source File

SOURCE=.\dx\Generic\postproc.c
# End Source File
# Begin Source File

SOURCE=.\dx\Generic\unpack.c
# End Source File
# Begin Source File

SOURCE=.\DX\Generic\vfwPback.c
# End Source File
# Begin Source File

SOURCE=.\DX\Generic\vfwpbdll_if.c
# End Source File
# End Group
# Begin Group "Win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dx\Win32\DeblockOpt.c
# End Source File
# Begin Source File

SOURCE=.\dx\Win32\DeRingOpt.c
# End Source File
# Begin Source File

SOURCE=.\DX\Win32\DOptSystemDependant.c
# End Source File
# Begin Source File

SOURCE=.\Common\Win32\fdct_m.asm

!IF  "$(CFG)" == "vp3d - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3d\Release
InputPath=.\Common\Win32\fdct_m.asm
InputName=fdct_m

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vp3d - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\ObjectCode\vp3d\Debug
InputPath=.\Common\Win32\fdct_m.asm
InputName=fdct_m

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dx\Win32\loopf_asm.c
# End Source File
# Begin Source File

SOURCE=.\DX\Win32\mmxIdct.c
# End Source File
# Begin Source File

SOURCE=.\Common\Win32\OptFunctions.c
# End Source File
# Begin Source File

SOURCE=.\Common\Win32\OptYUV2RGB.c
# End Source File
# Begin Source File

SOURCE=.\dx\Win32\quantindexmmx.c
# End Source File
# Begin Source File

SOURCE=.\dx\Win32\Unpackvideo.c
# End Source File
# Begin Source File

SOURCE=.\dx\Win32\Wmtidct.c
# End Source File
# Begin Source File

SOURCE=.\Common\Win32\WmtOptFunctions.c
# End Source File
# End Group
# End Target
# End Project
