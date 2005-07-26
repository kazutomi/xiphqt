## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign 1.6 dist-zip

SUBDIRS = src include doc win32 debian

m4datadir = $(datadir)/aclocal
m4data_DATA = ogg2.m4

EXTRA_DIST = README AUTHORS CHANGES COPYING libogg2.spec ogg2.m4 \
	macos macosx

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ogg2.pc

dist-hook:
	rm -rf `find $(distdir)/macos -name CVS`
	rm -rf `find $(distdir)/macosx -name CVS`

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
