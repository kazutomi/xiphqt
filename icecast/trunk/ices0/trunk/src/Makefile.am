## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = playlist_builtin interpreter xml

bin_PROGRAMS = ices

noinst_HEADERS = definitions.h setup.h log.h stream.h util.h playlist.h cue.h dj.h mp3.h id3.h signals.h reencode.h

ices_SOURCES = log.c setup.c ices.c stream.c util.c playlist.c dj.c mp3.c cue.c id3.c signals.c reencode.c

ices_LDADD = playlist_builtin/libplaylist.a interpreter/libinterpreter.a xml/libicesxml.a $(top_srcdir)/resolver/libresolver.a $(top_srcdir)/thread/libicethread.a $(top_srcdir)/thread/avl/libavl.a $(top_srcdir)/libshout/.libs/libshout.a

INCLUDES = -I$(top_srcdir)/resolver -I$(top_srcdir)/thread -I$(top_srcdir)/libshout

if FSSTD
bindir=$(ICES_SBINDIR)
else
bindir=$(ICES_BINDIR)
endif
