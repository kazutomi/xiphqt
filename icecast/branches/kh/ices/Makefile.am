## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6 foreign dist-bzip2
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = conf debian doc src

EXTRA_DIST = README AUTHORS COPYING TODO NEWS m4 contrib

# SCCS Definitions (for BitKeeper)
GET = true

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

dist-hook:
	rm -rf `find $(distdir)/contrib -name .svn`
	rm -rf `find $(distdir)/m4 -name .svn`
	rm -rf `find $(distdir)/src/net -name .svn`
	rm -rf `find $(distdir)/src/thread -name .svn`
	rm -rf `find $(distdir)/src/timing -name .svn`
