# NOTE: This Makefile requires GNU make
# Location to put the targets.
TARGETBINDIR = .
TARGETLIBDIR = .
# Name of the targets
LIBTHEORABASE_TARGET = libtheorabase.a
LIBTHEORAENC_TARGET = libtheoraenc.a
LIBTHEORADEC_TARGET = libtheoradec.a
DUMP_VIDEO_TARGET = dump_video
PLAYER_EXAMPLE_TARGET = player_example
ENCODER_EXAMPLE_TARGET = encoder_example
# The compiler to use
CC = gcc
# The command to use to generate dependency information
MAKEDEPEND = gcc -MM
#MAKEDEPEND = makedepend -f- -Y --
# The location of include files.
# Modify these to point to your Ogg and Vorbis include directories if they are
#  not installed in a standard location.
CINCLUDE = -I/usr/include/SDL -D_REENTRANT
# Extra compilation flags.
# You may get speed increases by including flags such as -O2 or -O3 or
#  -ffast-math, or additional flags, depending on your system and compiler.
# The -g flag will generally include debugging information.
CFLAGS = -g
# Libraries to link with, and the location of library files.
LIBS = -logg -lvorbis -lvorbisenc

# ANYTHING BELOW THIS LINE PROBABLY DOES NOT NEED EDITING
CINCLUDE := -I../include ${CINCLUDE}
LIBSRCDIR = ../lib
BINSRCDIR = ../examples
WORKDIR = objs

# C source file lists
LIBTHEORABASE_CSOURCES = \
fragment.c \
idct.c \
info.c \
internal.c \
quant.c \
state.c \

LIBTHEORABASE_CHEADERS = \
dct.h \
idct.h \
huffman.h \
internal.h \
ocintrin.h \
quant.h \
../include/theora/theora.h \

LIBTHEORADEC_CSOURCES = \
decinfo.c \
decode.c \
dequant.c \
huffdec.c \
mcdec.c \

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
impmap.c \
mcenc.c \
psych.c \

LIBTHEORAENC_CHEADERS =   \
${LIBTHEORABASE_CHEADERS} \
encint.h \
enquant.h \
fdct.h \
huffenc.h \
psych.h \

LIBTHEORAENC_CSOURCES = \
bitrate.c \
encinfo.c \
encode.c \
enquant.c \
fdct.c \
huffenc.c \
impmap.c \
mcenc.c \
psych.c \

DUMP_VIDEO_CSOURCES = dump_video.c
ENCODER_EXAMPLE_CSOURCES = encoder_example.c
PLAYER_EXAMPLE_CSOURCES = player_example.c

# Create object file list.
LIBTHEORABASE_OBJS:= ${LIBTHEORABASE_CSOURCES:%.c=${WORKDIR}/%.o}
LIBTHEORADEC_OBJS:= ${LIBTHEORADEC_CSOURCES:%.c=${WORKDIR}/%.o}
LIBTHEORAENC_OBJS:= ${LIBTHEORAENC_CSOURCES:%.c=${WORKDIR}/%.o}
DUMP_VIDEO_OBJS:= ${DUMP_VIDEO_CSOURCES:%.c=${WORKDIR}/%.o}
ENCODER_EXAMPLE_OBJS:= ${ENCODER_EXAMPLE_CSOURCES:%.c=${WORKDIR}/%.o}
PLAYER_EXAMPLE_OBJS:= ${PLAYER_EXAMPLE_CSOURCES:%.c=${WORKDIR}/%.o}
ALL_OBJS:= ${LIBTHEORABASE_OBJS} ${LIBTHEORADEC_OBJS} ${LIBTHEORAENC_OBJS} \
 ${DUMP_VIDEO_OBJS} ${ENCODER_EXAMPLE_OBJS} ${PLAYER_EXAMPLE_OBJS}
# Create the dependency file list
ALL_DEPS:= ${ALL_OBJS:%.o=%.d}
# Prepend source path to file names.
LIBTHEORABASE_CSOURCES:= ${LIBTHEORABASE_CSOURCES:%=${LIBSRCDIR}/%}
LIBTHEORABASE_CHEADERS:= ${LIBTHEORABASE_CHEADERS:%=${LIBSRCDIR}/%}
LIBTHEORADEC_CSOURCES:= ${LIBTHEORADEC_CSOURCES:%=${LIBSRCDIR}/%}
LIBTHEORADEC_CHEADERS:= ${LIBTHEORADEC_CHEADERS:%=${LIBSRCDIR}/%}
LIBTHEORAENC_CSOURCES:= ${LIBTHEORAENC_CSOURCES:%=${LIBSRCDIR}/%}
LIBTHEORAENC_CHEADERS:= ${LIBTHEORAENC_CHEADERS:%=${LIBSRCDIR}/%}
DUMP_VIDEO_CSOURCES:= ${DUMP_VIDEO_CSOURCES:%=${BINSRCDIR}/%}
ENCODER_EXAMPLE_CSOURCES:= ${ENCODER_EXAMPLE_CSOURCES:%=${BINSRCDIR}/%}
PLAYER_EXAMPLE_CSOURCES:= ${PLAYER_EXAMPLE_CSOURCES:%=${BINSRCDIR}/%}
ALL_CSOURCES:= ${LIBTHEORABASE_CSOURCES} ${LIBTHEORADEC_CSOURCES} \
 ${LIBTHEORAENC_CSOURCES} ${DUMP_VIDEO_CSOURCES} ${ENCODER_EXAMPLE_CSOURCES} \
 ${PLAYER_EXAMPLE_CSOURCES}
# Prepand target path to file names.
LIBTHEORABASE_TARGET:= ${TARGETLIBDIR}/${LIBTHEORABASE_TARGET}
LIBTHEORADEC_TARGET:= ${TARGETLIBDIR}/${LIBTHEORADEC_TARGET}
LIBTHEORAENC_TARGET:= ${TARGETLIBDIR}/${LIBTHEORAENC_TARGET}
DUMP_VIDEO_TARGET:= ${TARGETBINDIR}/${DUMP_VIDEO_TARGET}
ENCODER_EXAMPLE_TARGET:= ${TARGETBINDIR}/${ENCODER_EXAMPLE_TARGET}
PLAYER_EXAMPLE_TARGET:= ${TARGETBINDIR}/${PLAYER_EXAMPLE_TARGET}
ALL_TARGETS:= ${LIBTHEORABASE_TARGET} ${LIBTHEORADEC_TARGET} \
 ${LIBTHEORAENC_TARGET} ${DUMP_VIDEO_TARGET} ${ENCODER_EXAMPLE_TARGET} \
 ${PLAYER_EXAMPLE_TARGET}

# Targets:
# Everything (default)
all: ${ALL_TARGETS}

# libtheorabase
${LIBTHEORABASE_TARGET}: ${LIBTHEORABASE_OBJS}
	mkdir -p ${TARGETLIBDIR}
	ar cqs $@ ${LIBTHEORABASE_OBJS}
# libtheoradec
${LIBTHEORADEC_TARGET}: ${LIBTHEORADEC_OBJS}
	mkdir -p ${TARGETLIBDIR}
	ar cqs $@ ${LIBTHEORADEC_OBJS}
# libtheoraenc
${LIBTHEORAENC_TARGET}: ${LIBTHEORAENC_OBJS}
	mkdir -p ${TARGETLIBDIR}
	ar cqs $@ ${LIBTHEORAENC_OBJS}

# dump_video
${DUMP_VIDEO_TARGET}: ${DUMP_VIDEO_OBJS} ${LIBTHEORABASE_TARGET} \
                       ${LIBTHEORADEC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${DUMP_VIDEO_OBJS} ${LIBS} \
         ${LIBTHEORADEC_TARGET} ${LIBTHEORABASE_TARGET}

# encoder_example
${ENCODER_EXAMPLE_TARGET}: ${ENCODER_EXAMPLE_OBJS} ${LIBTHEORABASE_TARGET} \
                            ${LIBTHEORAENC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${ENCODER_EXAMPLE_OBJS} ${LIBS} \
         ${LIBTHEORAENC_TARGET} ${LIBTHEORABASE_TARGET}

# player_example
${PLAYER_EXAMPLE_TARGET}: ${PLAYER_EXAMPLE_OBJS} ${LIBTHEORABASE_TARGET} \
                           ${LIBTHEORADEC_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${PLAYER_EXAMPLE_OBJS} ${LIBS} -lSDL -lpthread \
         ${LIBTHEORADEC_TARGET} ${LIBTHEORABASE_TARGET}

# Remove all targets.
clean:
	-rm ${ALL_OBJS} ${ALL_DEPS} ${ALL_TARGETS}
	-rmdir ${WORKDIR}

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
include ${ALL_DEPS}
