## Process this file with automake to produce Makefile.in. -*-Makefile-*-

# To disable automatic dependency tracking if using other tools than
# gcc and gmake, add the option 'no-dependencies'
AUTOMAKE_OPTIONS = 1.6

EXTRA_DIST = COPYING autogen.sh

bin_PROGRAMS = chirp

chirp_SOURCES = chirp.c chirp.h chirpgraph.c chirpgraph.h window.c window.h chirptest.c 

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
