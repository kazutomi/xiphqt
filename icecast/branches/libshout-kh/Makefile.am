## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6 foreign dist-bzip2
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = include src examples

EXTRA_DIST = README COPYING NEWS m4

if HAVE_PKGCONFIG
  pkgconfigdir = $(libdir)/pkgconfig
  pkgconfig_DATA = shout.pc
else
  bin_SCRIPTS = shout-config
endif
DISTCLEANFILES = shout-config.in

## SCCS stuff (for BitKeeper)
GET = true

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
