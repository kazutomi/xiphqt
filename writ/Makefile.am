## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = src include

m4datadir = $(datadir)/aclocal
m4data_DATA = writ.m4

EXTRA_DIST = HACKING todo.txt \
	autogen.sh libwrit.spec writ.m4

dist-hook:
	rm -rf `find $(distdir)/macos -name CVS`
	rm -rf `find $(distdir)/macosx -name CVS`

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
