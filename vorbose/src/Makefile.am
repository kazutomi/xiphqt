## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = COPYING autogen.sh

bin_PROGRAMS = vorbose

vorbose_SOURCES = codebook.c codec.h info.c map.c res.c \
		  vorbose.c

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
