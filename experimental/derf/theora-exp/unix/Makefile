# NOTE: This Makefile requires GNU make
# Location to put the targets.
TARGETBINDIR = .
TARGETLIBDIR = .
# Name of the targets
LIBTHEORA_TARGET = libtheora.a
DUMP_VIDEO_TARGET = dump_video
ENCODER_EXAMPLE_TARGET = encoder_example
# The compiler to use
CC = gcc
# The command to use to generate dependency information
MAKEDEPEND = gcc -MM
#MAKEDEPEND = makedepend -f- -Y --
# The location of include files.
# Modify these to point to your Ogg and Vorbis include directories if they are
#  not installed in a standard location.
CINCLUDE =
# Extra compilation flags.
# You may get speed increases by including flags such as -O2 or -O3 or
#  -ffast-math, or additional flags, depending on your system and compiler.
# The -g flag will generally include debugging information.
CFLAGS =
# Libraries to link with, and the location of library files.
LIBS = -logg -lvorbis -lvorbisenc

# ANYTHING BELOW THIS LINE PROBABLY DOES NOT NEED EDITING
CINCLUDE+= -I../include
LIBSRCDIR = ../lib
BINSRCDIR = ../examples
WORKDIR = objs

# C source file lists
LIBTHEORA_CSOURCES = \
fragment.c           \
idct.c               \
info.c               \
internal.c           \
quant.c              \
state.c              \
decinfo.c            \
decode.c             \
dequant.c            \
huffdec.c            \
mcdec.c              \
bitrate.c            \
encinfo.c            \
encode.c             \
enquant.c            \
fdct.c               \
huffenc.c            \
impmap.c             \
mcenc.c              \
psych.c

LIBTHEORA_CHEADERS = \
dct.h                \
idct.h               \
huffman.h            \
internal.h           \
ocintrin.h           \
quant.h              \
decint.h             \
dequant.h            \
huffdec.h            \
encint.h             \
enquant.h            \
fdct.h               \
huffenc.h            \
psych.h

DUMP_VIDEO_CSOURCES = dump_video.c
ENCODER_EXAMPLE_CSOURCES = encoder_example.c

# Create object file list.
LIBTHEORA_OBJS:= ${LIBTHEORA_CSOURCES:%.c=${WORKDIR}/%.o}
DUMP_VIDEO_OBJS:= ${DUMP_VIDEO_CSOURCES:%.c=${WORKDIR}/%.o}
ENCODER_EXAMPLE_OBJS:= ${ENCODER_EXAMPLE_CSOURCES:%.c=${WORKDIR}/%.o}
# Prepend source path to file names.
LIBTHEORA_CSOURCES:= ${LIBTHEORA_CSOURCES:%=${LIBSRCDIR}/%}
LIBTHEORA_CHEADERS:= ${LIBTHEORA_CHEADERS:%=${LIBSRCDIR}/%}
DUMP_VIDEO_CSOURCES:= ${DUMP_VIDEO_CSOURCES:%=${BINSRCDIR}/%}
ENCODER_EXAMPLE_CSOURCES:= ${ENCODER_EXAMPLE_CSOURCES:%=${BINSRCDIR}/%}
# Prepand target path to file names.
LIBTHEORA_TARGET:= ${TARGETLIBDIR}/${LIBTHEORA_TARGET}
DUMP_VIDEO_TARGET:= ${TARGETBINDIR}/${DUMP_VIDEO_TARGET}
ENCODER_EXAMPLE_TARGET:= ${TARGETBINDIR}/${ENCODER_EXAMPLE_TARGET}

# Targets:
# Everything (default)
all: ${LIBTHEORA_TARGET} ${DUMP_VIDEO_TARGET} ${ENCODER_EXAMPLE_TARGET}

# libtheora
${LIBTHEORA_TARGET}: ${LIBTHEORA_OBJS}
	mkdir -p ${TARGETLIBDIR}
	ar cqs $@ ${LIBTHEORA_OBJS}

# dump_video
${DUMP_VIDEO_TARGET}: ${DUMP_VIDEO_OBJS} ${LIBTHEORA_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${DUMP_VIDEO_OBJS} ${LIBS} ${LIBTHEORA_TARGET}

# encoder_example
${ENCODER_EXAMPLE_TARGET}: ${ENCODER_EXAMPLE_OBJS} ${LIBTHEORA_TARGET}
	mkdir -p ${TARGETBINDIR}
	${CC} ${CFLAGS} -o $@ ${ENCODER_EXAMPLE_OBJS} ${LIBS} \
         ${LIBTHEORA_TARGET}

# Remove all targets.
clean:
	-rm ${LIBTHEORA_OBJS} ${LIBTHEORA_TARGET}          \
         ${DUMP_VIDEO_OBJS} ${DUMP_VIDEO_TARGET}           \
         ${ENCODER_EXAMPLE_OBJS} ${ENCODER_EXAMPLE_TARGET}
	-rmdir ${WORKDIR}

# Make the dependency list.
# This should be automatically re-made if the source files or header files
#  change.
# Only tracks dependencies on headers in this project, not libraries.
depend: ${LIBTHEORA_CSOURCES} ${LIBTHEORA_CHEADERS} \
         ${DUMP_VIDEO_CSOURCES}                     \
         ${ENCODER_EXAMPLE_CSOURCES}
	${MAKEDEPEND} ${CINCLUDE} ${CFLAGS} ${LIBTHEORA_CSOURCES} > $@

# Specify which targets are phony for GNU make
.PHONY : all clean

# Rules
${WORKDIR}/%.o : ${LIBSRCDIR}/%.c
	mkdir -p ${dir $@}
	${CC} ${CINCLUDE} ${CFLAGS} -c -o $@ $<
${WORKDIR}/%.o : ${BINSRCDIR}/%.c
	mkdir -p ${dir $@}
	${CC} ${CINCLUDE} ${CFLAGS} -c -o $@ $<

# Include header file dependencies
include depend
