# UMAKE generated Makefile
# build choices: default,release
# platform: linux-2.0-libc6-i386
# Wed Feb  2 15:36:22 2000
.SUFFIXES: .cpp .so
.c.so:
	$(CC) $(CCFLAGS) -fPIC -DPIC -o  $@ -c $<
.cpp.so:
	$(CXX) $(CXXFLAGS) -fPIC -DPIC -o  $@ -c $<

RM = rm -f
RM_DIR = rm -rf
MAKE_DEP = makedep.py
MAKE_DEP_FLAGS =   $(INCLUDES) $(DEFINES)
MAKE_LIB = ar cr 
MAKE_LIB_FLAGS =  
RANLIB = ranlib
CP = cp
MAKE = make

# Compilers
CC = gcc
CCFLAGS = -Wall -pipe -O2  $(INCLUDES) $(DEFINES)
CXX = g++
CXXFLAGS = -Wall -pipe -O2  $(INCLUDES) $(DEFINES)

# Linker
LD = g++
LDFLAGS = -shared -L/usr/X11R6/lib -L/usr/X11R6/lib 

DEPLIBS = 

DYNAMIC_LIBS = -logg -lvorbis

SRCS = \
	../fvorbis.cpp

OBJS = $(COMPILED_OBJS) $(SOURCE_OBJS)

COMPILED_OBJS = \
	rel/obj/fvorbis.so

SOURCE_OBJS = 

INCLUDES = \
	-I/usr/X11R6/include \
	-I/usr/X11R6/include \
	-I/home/jack/src/rmasdk_6_0/include \
	-I.. \
	-I./pub

DEFINES = \
	-D_LINUX \
	-D_REENTRANT \
	-D_UNIX \
	-D_RED_HAT_5_X_

SYSLIBS = 

LOCAL_LIBS = 

rel/fvorbis.so : rel/obj $(OBJS) $(DEPLIBS)
	if test -d rel; then echo; else mkdir rel; fi
	$(LD) $(LDFLAGS) -o rel/fvorbis.so $(OBJS) $(DYNAMIC_LIBS) $(DEPLIBS) $(LOCAL_LIBS) $(SYSLIBS) -lgcc

clean::
	$(RM) rel/fvorbis.so $(COMPILED_OBJS)

## OBJECT DEPENDANCIES
rel/obj:
	if test -d rel; then echo; else mkdir rel; fi
	if test -d rel/obj; then echo; else mkdir rel/obj; fi
rel/obj/fvorbis.so : ../fvorbis.cpp
	$(CXX) $(CXXFLAGS) -fPIC -DPIC -o rel/obj/fvorbis.so -c ../fvorbis.cpp
copy:
	if test -d ../release; then echo; else mkdir ../release; fi
	cp rel/fvorbis.so ../release/fvorbis.so
depend:
	$(MAKE_DEP) $(MAKE_DEP_FLAGS) rel/obj/ $(SRCS)

