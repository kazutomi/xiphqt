## process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS=foreign

bin_PROGRAMS=oggsplit

oggsplit_SOURCES=oggsplit.c stream.c stream.h output.c output.h \
	xmalloc.c system.h getopt.c getopt1.c getopt.h
oggsplit_CFLAGS=$(OGG_CFLAGS)
oggsplit_LDADD=$(OGG_LIBS)

man_MANS=oggsplit.1
