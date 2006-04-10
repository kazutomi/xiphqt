## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

noinst_HEADERS = ogginternal.h mutex.h

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include

lib_LTLIBRARIES = libogg2.la

libogg2_la_SOURCES = sync.c stream.c bitwise.c bytewise.c buffer.c mutex.h  ogginternal.h
libogg2_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

