AUTOMAKE_OPTIONS = 1.9 foreign

lib_LTLIBRARIES = libiceadmin.la

libiceadmin_la_SOURCES = error.c libiceadmin.c iceadmin_internal.c xalloc.c
libiceadmin_la_LDFLAGS = @LTLIBOBJS@ -version-info @LIBICEADMIN_SO_MAJOR@:@LIBICEADMIN_SO_MINOR@

INCLUDES =	@COMPAT_INCLUDES@ -I$(top_srcdir)/include

AM_CFLAGS =	@XML2_CFLAGS@
AM_CPPFLAGS =	@XIPH_CPPFLAGS@ @XALLOC_CPPFLAGS@

EXTRA_DIST =	error.h iceadmin_internal.h xalloc.h

CLEANFILES =	core *.core *~ .*~
