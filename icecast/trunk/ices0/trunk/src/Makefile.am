## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = playlist

bin_PROGRAMS = ices

noinst_HEADERS = icestypes.h definitions.h setup.h log.h stream.h util.h \
	cue.h metadata.h in_vorbis.h mp3.h id3.h signals.h reencode.h \
	ices_config.h

ices_SOURCES = ices.c log.c setup.c stream.c util.c mp3.c cue.c metadata.c \
	id3.c signals.c

if USE_XML
ices_SOURCES += ices_config.c
endif

if USE_LAME
ices_SOURCES += reencode.c
endif

if USE_VORBIS
ices_SOURCES += in_vorbis.c
endif

ices_LDADD = playlist/libplaylist.a

AM_CPPFLAGS = -DICES_ETCDIR=\"$(sysconfdir)\" -DICES_MODULEDIR=\"$(moddir)\"
