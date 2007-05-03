# NOTE: This Makefile requires GNU make
# Location to put the targets.
TARGETBINDIR = .
TARGETLIBDIR = .
# Name of the targets
LIBTHEORAENC_TARGET = libtheoraenc.a
LIBTHEORADEC_TARGET = libtheoradec.a
DUMP_VIDEO_TARGET = dump_video
PLAYER_EXAMPLE_TARGET = player_example
ENCODER_EXAMPLE_TARGET = encoder_example
REHUFF_EXAMPLE_TARGET = rehuff
# The compiler to use
CC = gcc
# The command to use to generate dependency information
MAKEDEPEND = gcc -MM
#MAKEDEPEND = makedepend -f- -Y --
# The location of include files.
# Modify these to point to your Ogg and Vorbis include directories if they are
#  not installed in a standard location.
CINCLUDE = `sdl-config --cflags` -D_REENTRANT
# Extra compilation flags.
# You may get speed increases by including flags such as -O2 or -O3 or
#  -ffast-math, or additional flags, depending on your system and compiler.
# The correct -march=<architecture> flag will also generate much better code
#  on newer architectures.
CFLAGS = -O3 -Wall -Wno-parentheses -fomit-frame-pointer -fforce-addr -finline-functions -DOC_X86ASM
# The -g flag will generally include debugging information.
#CFLAGS = -g -Wall -Wno-parentheses
# Libraries to link with, and the location of library files.
# Add -lpng -lz if you want to use -DOC_DUMP_IMAGES.
LIBS = -logg -lvorbis -lvorbisenc `sdl-config --libs`

# ANYTHING BELOW THIS LINE PROBABLY DOES NOT NEED EDITING
CINCLUDE := -I../include ${CINCLUDE}
LIBSRCDIR = ../lib
BINSRCDIR = ../examples
WORKDIR = objs

# C source file lists

LIBTHEORABASE_CHEADERS = \
dct.h \
idct.h \
huffman.h \
internal.h \
ocintrin.h \
quant.h \
../include/theora/theora.h \

LIBTHEORADEC_CSOURCES = \
apiwrapper.c \
decinfo.c \
decode.c \
dequant.c \
fragment.c \
huffdec.c \
idct.c \
info.c \
internal.c \
quant.c \
state.c \
$(if $(findstring -DOC_X86ASM,${CFLAGS}), \
x86/mmxstate.c \
x86/x86state.c \
x86/mmxidct.c \
x86/mmxfrag.c \
x86/cpu.c \
)

LIBTHEORADEC_CHEADERS =   \
${LIBTHEORABASE_CHEADERS} \
decint.h \
dequant.h \
huffdec.h \

LIBTHEORAENC_CSOURCES = \
bitrate.c \
encinfo.c \
encode.c \
enquant.c \
fdct.c \
huffenc.c \
mcenc.c \
encmsc.c \
encvbr.c \
impmap.c \
psych.c \

LIBTHEORAENC_CHEADERS =   \
${LIBTHEORABASE_CHEADERS} \
encint.h \
enquant.h \
fdct.h \
huffenc.h \
encvbr.h \
psych.h \

DUMP_VIDEO_CSOURCES = dump_video.c
ENCODER_EXAMPLE_CSOURCES = encoder_example.c
PLAYER_EXAMPLE_CSOURCES = player_example.c
#rehuff links staticaly directly to these objects, since it uses symbols which
# we may not want to publicly export from the libraries.
#This is pretty ugly; feel free to try to clean it up.
REHUFF_EXAMPLE_CSOURCES = rehuff.c
REHUFF_CSOURCES = \
decinfo.c \
decode.c \
dequant.c \
encinfo.c \
enquant.c \
fragment.c \
huffdec.c \
huffenc.c \
idct.c \
info.c \
internal.c \
quant.c \
recode.c \
state.c \
$(if $(findstring -DOC_X86ASM,${CFLAGS}), \
x86/mmxstate.c \
x86/x86state.c \
x86/mmxidct.c \
x86/mmxfrag.c \
x86/cpu.c \
)

# Create object file list.
LIBTHEORADEC_OBJS:= ${LIBTHEORADEC_CSOURCES:%.c=${WORKDIR}/%.o}
LIBTHEORAENC_OBJS:= ${LIBTHEORAENC_CSOURCES:%.c=${WORKDIR}/%.o}
DUMP_VIDEO_OBJS:= ${DUMP_VIDEO_CSOURCES:%.c=${WORKDIR}/%.o}
ENCODER_EXAMPLE_OBJS:= ${ENCODER_EXAMPLE_CSOURCES:%.c=${WORKDIR}/%.o}
PLAYER_EXAMPLE_OBJS:= ${PLAYER_EXAMPLE_CSOURCES:%.c=${WORKDIR}/%.o}
REHUFF_EXAMPLE_OBJS:= ${REHUFF_EXAMPLE_CSOURCES:%.c=${WORKDIR}/%.o} \
 ${REHUFF_CSOURCES:%.c=${WORKDIR}/%.o}
ALL_OBJS:= ${LIBTHEORADEC_OBJS} ${LIBTHEORAENC_OBJS} \
 ${DUMP_VIDEO_OBJS} ${ENCODER_EXAMPLE_OBJS} ${PLAYER_EXAMPLE_OBJS} \
 ${REHUFF_EXAMPLE_OBJS}
# Create the dependency file list
ALL_DEPS:= ${ALL_OBJS:%.o=%.d}
# Prepend source path to file names.
LIBTHEORADEC_CSOURCES:= ${LIBTHEORADEC_CSOURCES:%=${LIBSRCDIR}/%}
LIBTHEORADEC_CHEADERS:= ${LIBTHEORADEC_CHEADERS:%=${LIBSRCDIR}/%}
LIBTHEORAENC_CSOURCES:= ${LIBTHEORAENC_CSOURCES:%=${LIBSRCDIR}/%}
LIBTHEORAENC_CHEADERS:= ${LIBTHEORAENC_CHEADERS:%=${LIBSRCDIR}/%}
DUMP_VIDEO_CSOURCES:= ${DUMP_VIDEO_CSOURCES:%=${BINSRCDIR}/%}
ENCODER_EXAMPLE_CSOURCES:= ${ENCODER_EXAMPLE_CSOURCES:%=${BINSRCDIR}/%}
PLAYER_EXAMPLE_CSOURCES:= ${PLAYER_EXAMPLE_CSOURCES:%=${BINSRCDIR}/%}
REHUFF_EXAMPLE_CSOURCES:= ${REHUFF_EXAMPLE_CSOURCES:%=${BINSRCDIR}/%} \
 ${REHUFF_CSOURCES:%=${LIBSRCDIR}/%}
ALL_CSOURCES:= ${LIBTHEORADEC_CSOURCES} ${LIBTHEORAENC_CSOURCES} \
 ${DUMP_VIDEO_CSOURCES} ${ENCODER_EXAMPLE_CSOURCES} \
 ${PLAYER_EXAMPLE_CSOURCES} ${REHUFF_EXAMPLE_CSOURCES}
# Prepand target path to file names.
LIBTHEORADEC_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC_TARGET}
LIBTHEORAENC_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC_TARGET}
DUMP_VIDEO_TARGET:= ${TARGETBINDIR}/${DUMP_VIDEO_TARGET}
ENCODER_EXAMPLE_TARGET:= ${TARGETBINDIR}/${ENCODER_EXAMPLE_TARGET}
PLAYER_EXAMPLE_TARGET:= ${TARGETBINDIR}/${PLAYER_EXAMPLE_TARGET}
REHUFF_EXAMPLE_TARGET:= ${TARGETBINDIR}/${REHUFF_EXAMPLE_TARGET}
ALL_TARGETS:= ${LIBTHEORADEC_TARGET} ${LIBTHEORAENC_TARGET} \
 ${DUMP_VIDEO_TARGET} ${ENCODER_EXAMPLE_TARGET} ${PLAYER_EXAMPLE_TARGET} \
 ${REHUFF_EXAMPLE_TARGET}

# Targets:
# Everything (default)
all: ${ALL_TARGETS}

# libtheoradec
${LIBTHEORADEC_TARGET}: ${LIBTHEORADEC_OBJS}
	mkdir -p ${TARGETLIBDIR}
	ar cqs $@ ${LIBTHEORADEC_OBJS}
# libtheoraenc
${LIBTHEORAENC_TARGET}: ${LIBTHEORAENC_OBJS}
	mkdir -p ${TARGETLIBDIR}
	ar cqs $@ ${LIBTHEORAENC_OBJS}

# dump_video
${DUMP_VIDEO_TARGET}: ${DUMP_VIDEO_OBJS} ${LIBTHEORADEC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${DUMP_VIDEO_OBJS} ${LIBS} \
         ${LIBTHEORADEC_TARGET}

# encoder_example
${ENCODER_EXAMPLE_TARGET}: ${ENCODER_EXAMPLE_OBJS} ${LIBTHEORADEC_TARGET} \
                            ${LIBTHEORAENC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${ENCODER_EXAMPLE_OBJS} ${LIBS} \
         ${LIBTHEORAENC_TARGET} ${LIBTHEORADEC_TARGET}

# player_example
${PLAYER_EXAMPLE_TARGET}: ${PLAYER_EXAMPLE_OBJS} ${LIBTHEORADEC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${PLAYER_EXAMPLE_OBJS} ${LIBS} \
         ${LIBTHEORADEC_TARGET}

# rehuff
${REHUFF_EXAMPLE_TARGET}: ${REHUFF_EXAMPLE_OBJS}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${REHUFF_EXAMPLE_OBJS} ${LIBS}

# Remove all targets.
clean:
	-rm ${ALL_OBJS} ${ALL_DEPS} ${ALL_TARGETS}
	-rmdir ${WORKDIR}

# Make everything depend on changes in the Makefile
${ALL_OBJS} ${ALL_DEPS} ${ALL_TARGETS} : Makefile

# Specify which targets are phony for GNU make
.PHONY : all clean

# Rules
${WORKDIR}/%.d : ${LIBSRCDIR}/%.c
	mkdir -p ${dir $@}
	${MAKEDEPEND} ${CINCLUDE} ${CFLAGS} $< -MT ${@:%.d=%.o} > $@
${WORKDIR}/%.d : ${BINSRCDIR}/%.c
	mkdir -p ${dir $@}
	${MAKEDEPEND} ${CINCLUDE} ${CFLAGS} $< -MT ${@:%.d=%.o} > $@
${WORKDIR}/%.o : ${LIBSRCDIR}/%.c
	mkdir -p ${dir $@}
	${CC} ${CINCLUDE} ${CFLAGS} -c -o $@ $<
${WORKDIR}/%.o : ${BINSRCDIR}/%.c
	mkdir -p ${dir $@}
	${CC} ${CINCLUDE} ${CFLAGS} -c -o $@ $<

# Include header file dependencies
-include ${ALL_DEPS}
