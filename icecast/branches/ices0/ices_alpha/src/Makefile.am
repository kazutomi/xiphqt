## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = playlist_builtin interpreter xml

bin_PROGRAMS = ices

noinst_HEADERS = definitions.h setup.h log.h stream.h util.h playlist.h cue.h dj.h mp3.h id3.h signals.h

ices_SOURCES = log.c setup.c ices.c stream.c util.c playlist.c dj.c mp3.c cue.c id3.c signals.c

ices_LDADD = playlist_builtin/libplaylist.a interpreter/libinterpreter.a xml/libicesxml.a $(top_srcdir)/resolver/libresolver.a $(top_srcdir)/thread/libicethread.a $(top_srcdir)/thread/avl/libavl.a -lices 

INCLUDES = -I$(top_srcdir)/resolver -I$(top_srcdir)/thread

if FSSTD
bindir=$(ICES_SBINDIR)
else
bindir=$(ICES_BINDIR)
endif
