## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = $(SPLAYER)

# possible contents of BUILDABLE_PLAYERS:
EXTRA_PROGRAMS = theora_splayer

theora_splayer_SOURCES = theora_splayer.c
theora_splayer_LDADD = $(SDL_LIBS) @PORTAUDIO_LIBS@
theora_splayer_CFLAGS = $(CFLAGS) $(SDL_CFLAGS)
