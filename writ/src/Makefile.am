## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include

lib_LTLIBRARIES = libwrit.la

libwrit_la_SOURCES = format.c 
libwrit_la_LDFLAGS = -no-undefined -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
