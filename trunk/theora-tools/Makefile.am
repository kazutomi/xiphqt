## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip dist-bzip2

SUBDIRS = theoraenc theoracomment png2theora vp32theora

EXTRA_DIST = COPYING autogen.sh

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
