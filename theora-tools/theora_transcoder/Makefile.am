## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = theora_transcoder avi2vp3

LDADD = -ltheora -logg -lvorbis -lm

theora_transcoder_SOURCES = theora_transcoder.c
theora_transcoder_LDADD = $(LDADD) -lvorbisenc

avi2vp3_SOURCES = avi2vp3.c avilib.c avilib.h
