## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6 foreign dist-bzip2
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include src examples debian

EXTRA_DIST = README COPYING NEWS m4

DISTCLEANFILES = shout-config.in

docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = COPYING README examples/example.c

m4datadir = $(datadir)/aclocal
m4data_DATA = m4/shout.m4

if HAVE_PKGCONFIG
  pkgconfigdir = $(libdir)/pkgconfig
  pkgconfig_DATA = shout.pc
else
  bin_SCRIPTS = shout-config
endif

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

dist-hook:
	rm -rf `find $(distdir)/m4 -name .svn`
