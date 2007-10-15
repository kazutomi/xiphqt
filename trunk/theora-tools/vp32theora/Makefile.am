## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = vp32theora avi2vp3

vp32theora_SOURCES = vp32theora.c
vp32theora_LDADD = -ltheora -lvorbisenc -lvorbis -logg -lm

avi2vp3_SOURCES = avi2vp3.c avilib.c avilib.h
