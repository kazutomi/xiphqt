# UMAKE generated Makefile
# build choices: default,release
# platform: win32-i386
# Wed Feb 02 04:24:03 2000
.mc.rc:
	$(MC) $<
.cpp.obj:
	$(CC) $(CCFLAGS)  /MD /Fo$@ /c  $<
.c.obj:
	$(CC) $(CCFLAGS)  /MD /Fo$@ /c  $<
.rc.res:
	$(RC) $(RCFLAGS) /fo $@ $<

RM = rm -rf
RM_DIR = rm -rf
MAKE_DEP = makedpnd /DMAKE_DEPEND
MAKE_DEP_FLAGS =   $(INCLUDES) $(DEFINES)
MAKE_LIB = link
MAKE_LIB_FLAGS = /lib /nologo 
CP = copy
MAKE = nmake

# Compilers
MC = mc
MCFLAGS =   $(INCLUDES) $(DEFINES)
CC = cl
CCFLAGS = /nologo /Zp1 /W3 /GX- /D "_WINDOWS" /D "THREADS_SUPPORTED" /O2 /Oy-  $(INCLUDES) $(DEFINES)
CC = cl
CCFLAGS = /nologo /Zp1 /W3 /GX- /D "_WINDOWS" /D "THREADS_SUPPORTED" /O2 /Oy-  $(INCLUDES) $(DEFINES)
RC = rc
RCFLAGS = /l 0x409  $(INCLUDES) $(DEFINES)

# Linker
LD = link
LDFLAGS =  /nologo /subsystem:windows /dll /incremental:no /machine:I386 /OPT:REF  /LIBPATH:\src\win32sdk\sdk\release ogg_static.lib vorbis_static.lib

DEPLIBS = 

DYNAMIC_LIBS = 

SRCS = \
        ..\rvorbis.cpp \
        ..\fivemque.cpp

OBJS = $(COMPILED_OBJS) $(SOURCE_OBJS)

COMPILED_OBJS = \
        Rel32\obj\rvorbis.obj \
        Rel32\obj\fivemque.obj

SOURCE_OBJS = 

INCLUDES = \
	/I.\win \
	/I.\pub \
	/I.\pub\win \
	/I. \
        /I\rmasdk\include \
        /I\src\win32sdk\sdk\include \
	/I.. \
	/I.\pub

DEFINES = \
	/D_WINDOWS \
	/DSTRICT \
	/DWIN32 \
	/D_WIN32 \
	/D_M_IX86

SYSLIBS = 

LOCAL_LIBS = 

Rel32\rvorbis.dll : Rel32\obj $(OBJS) $(DEPLIBS)
	if NOT exist "Rel32" mkdir "Rel32"
        $(LD)  @<<rvorbis.lnk
$(LDFLAGS) /out:Rel32\rvorbis.dll /def:rvorbis.def /implib:Rel32\rvorbis.lib /PDB:NONE $(OBJS) $(DEPLIBS) $(LOCAL_LIBS) $(SYSLIBS) $(DYNAMIC_LIBS)
<<KEEP


clean::
        $(RM) Rel32\rvorbis.dll $(COMPILED_OBJS)

## OBJECT DEPENDANCIES
Rel32\obj:
	if NOT exist "Rel32" mkdir "Rel32"
	if NOT exist "Rel32\obj" mkdir "Rel32\obj"
Rel32\obj\rvorbis.obj : ..\rvorbis.cpp
        $(CC) $(CCFLAGS)  /MD /FoRel32\obj\rvorbis.obj /c  ..\rvorbis.cpp
Rel32\obj\fivemque.obj : ..\fivemque.cpp
        $(CC) $(CCFLAGS)  /MD /FoRel32\obj\fivemque.obj /c  ..\fivemque.cpp
copy:
	if NOT exist "..\release" mkdir "..\release"
        copy Rel32\rvorbis.dll ..\release\rvorbis.dll
depend:
	$(MAKE_DEP) $(MAKE_DEP_FLAGS) -t Rel32\obj\ $(SRCS)

