## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = $(SPLAYER)

# possible contents of BUILDABLE_PLAYERS:
EXTRA_PROGRAMS = theora_splayer

CFLAGS = $(SDL_CFLAGS)
LDADD = -ltheora -logg -lvorbis -lm

theora_splayer_SOURCES = theora_splayer.c
theora_splayer_LDADD = $(LDADD) $(SDL_LIBS) -lportaudio -lpthread
