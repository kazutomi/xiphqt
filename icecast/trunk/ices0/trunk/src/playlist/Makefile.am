## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -DICES_MODULEDIR=\"$(moddir)\" -I$(top_srcdir)/src

noinst_LIBRARIES = libplaylist.a
noinst_HEADERS = playlist.h pm_builtin.h rand.h

libplaylist_a_SOURCES = playlist.c pm_builtin.c rand.c

if USE_PYTHON
libplaylist_a_SOURCES += pm_python.c
endif

if USE_PERL
libplaylist_a_SOURCES += pm_perl.c
endif

EXTRA_DIST = pm_perl.c pm_python.c
