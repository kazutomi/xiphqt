## Process this file with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = example example2

example_SOURCES = example.c
example2_SOURCES = example2.c
LDADD = ../src/libshout.la $(XIPH_LIBS)
AM_CFLAGS = @XIPH_CFLAGS@
AM_CPPFLAGS = @XIPH_CFLAGS@

INCLUDES = -I$(top_builddir)/include
