## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include 

bin_PROGRAMS = $(THEORA123)

# possible contents of BUILDABLE_PLAYERS:
EXTRA_PROGRAMS = theora123

theora123_SOURCES = theora123.c
theora123_CFLAGS  = $(SDL_CFLAGS)
theora123_LDADD = $(LDADD) $(SDL_LIBS)
