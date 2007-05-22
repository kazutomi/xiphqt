## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = $(PNG2THEORA)

# possible contents of BUILDABLE_PLAYERS:
EXTRA_PROGRAMS = png2theora

png2theora_SOURCES = png2theora.c
png2theora_CFLAGS = $(OGG_CFLAGS) $(PNG_CFLAGS)
png2theora_LDADD = $(GETOPT_OBJS) $(LDADD) $(PNG_LIBS)
png2theora_DEPENDENCIES = $(GETOPT_OBJS)
