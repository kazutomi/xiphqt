## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = theoraenc

CFLAGS = $(SDL_CFLAGS)
LDADD = -ltheora -logg -lvorbis -lm

theoraenc_SOURCES = theoraenc.c
EXTRA_theoraenc_SOURCES = getopt.c getopt1.c getopt.h
theoraenc_LDADD = $(GETOPT_OBJS) $(LDADD) -lvorbisenc
theoraenc_DEPENDENCIES = $(GETOPT_OBJS)
