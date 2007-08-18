AUTOMAKE_OPTIONS = 1.9 foreign

bin_PROGRAMS =	iceadmin

iceadmin_SOURCES = iceadmin.c $(top_srcdir)/lib/xalloc.c
iceadmin_LDADD = $(top_builddir)/lib/libiceadmin.la @XML2_LIBS@

INCLUDES =	@COMPAT_INCLUDES@ -I$(top_srcdir)/include -I$(top_srcdir)/lib
AM_CFLAGS =	@XML2_CFLAGS@
# AM_CPPFLAGS =	@XIPH_CPPFLAGS@ -DUSE_XALLOC=1
AM_CPPFLAGS =	@XIPH_CPPFLAGS@

CLEANFILES =	core *.core *~ .*~
