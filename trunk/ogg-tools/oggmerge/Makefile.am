## process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = dist-zip foreign

SUBDIRS = testfiles tools

bin_PROGRAMS = oggmerge

oggmerge_SOURCES = oggmerge.c vorbis.c midi.c mng.c\
			oggmerge.h vorbis.h midi.h mng.h
EXTRA_oggmerge_SOURCES = getopt.c getopt1.c getopt.h

oggmerge_CFLAGS = $(OGG_CFLAGS) $(VORBIS_CFLAGS)
oggmerge_LDADD = $(GETOPT_OBJS) $(OGG_LIBS) $(VORBIS_LIBS)
oggmerge_DEPENDENCIES = $(GETOPT_OBJS)
