## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign 1.11

EXTRA_DIST = COPYING autogen.sh


lib_LTLIBRARIES = libsquishyio.la
libsquishyio_la_SOURCES = squishyio.c squishyio.h
libsquishyio_la_LDFLAGS = -no-undefined -version-info @SqI_LIB_CURRENT@:@SqI_LIB_REVISION@:@SqI_LIB_AGE@
libsquishyioincludedir = $(includedir)/squishyio
libsquishyioinclude_HEADERS = squishyio.h

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
