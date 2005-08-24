## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = theoraenc

theoraenc_SOURCES = theoraenc.c
EXTRA_theoraenc_SOURCES = getopt.c getopt1.c getopt.h
theoraenc_LDADD  = $(GETOPT_OBJS)  @VORBISENC_LIBS@
theoraenc_CFLAGS = $(CFLGAS) @VORBISENC_CFLAGS@
theoraenc_DEPENDENCIES = $(GETOPT_OBJS)
