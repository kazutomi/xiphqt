## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = dryice

dryice_SOURCES = core.c
EXTRA_dryice_SOURCES = ../codec_modules/theora.c

dryice_DEPENDENCIES = @DRYICE_OPTIONAL@
dryice_LDADD = $(dryice_DEPENDENCIES) @XIPH_LIBS@
    
AM_CFLAGS = @XIPH_CFLAGS@
AM_CPPFLAGS = @XIPH_CPPFLAGS@ -I$(top_srcdir)/include


debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

