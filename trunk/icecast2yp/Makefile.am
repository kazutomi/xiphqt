## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6 foreign dist-zip
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src sql

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
static:
	$(MAKE) all LDFLAGS="${LDFLAGS} -all-static"

