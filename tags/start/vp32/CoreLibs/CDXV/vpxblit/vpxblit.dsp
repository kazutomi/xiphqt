# Microsoft Developer Studio Project File - Name="vpxblit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=vpxblit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vpxblit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vpxblit.mak" CFG="vpxblit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vpxblit - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "vpxblit - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\Lib\Win32\Release"
# PROP Intermediate_Dir "..\..\..\..\..\ObjectCode\vpxblit\Release"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\include" /I "..\..\..\include\vp31" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\Lib\Win32\Release\s_vpxblit.lib"

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\Lib\Win32\Debug"
# PROP Intermediate_Dir "..\..\..\..\..\ObjectCode\vpxblit\Debug"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\..\..\include\vp31" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\Lib\Win32\Debug\s_vpxblit.lib"

!ENDIF 

# Begin Target

# Name "vpxblit - Win32 Release"
# Name "vpxblit - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "wx86"

# PROP Default_Filter "*.asm"
# Begin Source File

SOURCE=.\wx86\bcc00.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bcc00.asm
InputName=bcc00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bcc00.asm
InputName=bcc00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\bcc10.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bcc10.asm
InputName=bcc10

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bcc10.asm
InputName=bcc10

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\bcf00.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bcf00.asm
InputName=bcf00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bcf00.asm
InputName=bcf00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\bcf10.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bcf10.asm
InputName=bcf10

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bcf10.asm
InputName=bcf10

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\bcs00.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bcs00.asm
InputName=bcs00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bcs00.asm
InputName=bcs00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\bcs10.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bcs10.asm
InputName=bcs10

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bcs10.asm
InputName=bcs10

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\bct00.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bct00.asm
InputName=bct00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bct00.asm
InputName=bct00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\bct10.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bct10.asm
InputName=bct10

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bct10.asm
InputName=bct10

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\bcy00.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\bcy00.asm
InputName=bcy00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\bcy00.asm
InputName=bcy00

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wx86\const.asm

!IF  "$(CFG)" == "vpxblit - Win32 Release"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Release
InputPath=.\wx86\const.asm
InputName=const

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "vpxblit - Win32 Debug"

# Begin Custom Build
IntDir=.\..\..\..\..\..\ObjectCode\vpxblit\Debug
InputPath=.\wx86\const.asm
InputName=const

".\"$(IntDir)"\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	ml /Zi /Zm /Cx /c /coff /Fl$(IntDir)\$(InputName).lst /Fo $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\ctables.c
# ADD CPP /I ".\generic" /I "..\..\include" /I "..\..\..\include"
# End Source File
# Begin Source File

SOURCE=.\win32\wksetblt.c
# ADD CPP /I ".\generic" /I "..\..\include" /I "..\..\..\include"
# End Source File
# End Group
# Begin Group "Generic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\generic\bcf00_c.c
# ADD CPP /I "..\..\include" /I "..\..\..\include"
# End Source File
# Begin Source File

SOURCE=.\generic\bcf10_c.c
# ADD CPP /I "..\..\include" /I "..\..\..\include"
# End Source File
# Begin Source File

SOURCE=.\generic\bcs00_c.c
# ADD CPP /I "..\..\include" /I "..\..\..\include"
# End Source File
# Begin Source File

SOURCE=.\generic\bcs10_c.c
# ADD CPP /I "..\..\include" /I "..\..\..\include"
# End Source File
# Begin Source File

SOURCE=.\generic\bct00_c.c
# ADD CPP /I "..\..\include" /I "..\..\..\include"
# End Source File
# Begin Source File

SOURCE=.\generic\bct10_c.c
# ADD CPP /I "..\..\include" /I "..\..\..\include"
# End Source File
# Begin Source File

SOURCE=.\generic\bcy00_c.c
# ADD CPP /I "..\..\include" /I "..\..\..\include"
# End Source File
# Begin Source File

SOURCE=.\generic\ctables.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
