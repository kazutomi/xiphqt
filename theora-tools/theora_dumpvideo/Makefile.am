## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = theora_dumpvideo

CFLAGS = $(SDL_CFLAGS)
LDADD = -ltheora -logg -lvorbis -lm

theora_dumpvideo_SOURCES = theora_dumpvideo.c
theora_dumpvideo_LDADD = $(LDADD)
