## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6 foreign dist-zip
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = m4/ogg.m4 m4/xiph_compiler.m4 m4/theora.m4 m4/vorbis.m4 m4/shout.m4

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

static:
	$(MAKE) all LDFLAGS="${LDFLAGS} -all-static"
