## process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = dist-zip foreign

SUBDIRS = testfiles tools

bin_PROGRAMS = oggmerge

oggmerge_SOURCES = oggmerge.c vorbis.c midi.c\
			oggmerge.h vorbis.h midi.h mng.h

oggmerge_CFLAGS = @OGG_CFLAGS@ @VORBIS_CFLAGS@
oggmerge_LDADD = @OGG_LIBS@ @VORBIS_LIBS@
