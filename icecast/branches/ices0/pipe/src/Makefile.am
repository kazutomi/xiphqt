## Process this with automake to create Makefile.in

SUBDIRS = playlist

bin_PROGRAMS = ices

noinst_HEADERS = icestypes.h definitions.h setup.h log.h stream.h util.h \
	cue.h metadata.h in_vorbis.h mp3.h in_mp4.h in_flac.h id3.h pipe.h \
        signals.h reencode.h ices_config.h

ices_SOURCES = crossfade.c cue.c encode_pipe.c ices.c id3.c log.c metadata.c \
	mp3.c pipe.c setup.c signals.c stream.c util.c

EXTRA_ices_SOURCES = ices_config.c reencode.c in_vorbis.c in_mp4.c in_flac.c

ices_LDADD = $(ICES_OBJECTS) playlist/libplaylist.a
ices_DEPENDENCIES = $(ices_LDADD)

AM_CPPFLAGS = -DICES_ETCDIR=\"$(sysconfdir)\" -DICES_MODULEDIR=\"$(moddir)\"
