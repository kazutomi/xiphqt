## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = avl net timing httpp thread

lib_LTLIBRARIES = libshout.la
libshout_la_LDFLAGS = -version-info 3:0:0
AM_CFLAGS = @XIPH_CFLAGS@
AM_CPPFLAGS = @XIPH_CPPFLAGS@

noinst_HEADERS = shout_private.h util.h
libshout_la_SOURCES = shout.c util.c ogg.c mp3.c

libshout_la_LIBADD = net/libicenet.la timing/libicetiming.la avl/libiceavl.la\
		httpp/libicehttpp.la thread/libicethread.la @XIPH_LIBS@

INCLUDES = -I$(top_builddir)/include

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

