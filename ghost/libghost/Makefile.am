# Disable automatic dependency tracking if using other tools than gcc and gmake
#AUTOMAKE_OPTIONS = no-dependencies


#EXTRA_DIST=

#INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir) @OGG_CFLAGS@

lib_LTLIBRARIES = libghost.la

# Sources for compilation in the library
libghost_la_SOURCES = adpcm.c fftwrap.c ghost.c lifting.c lpc.c misc.c pitch.c \
	sinusoids.c smallft.c vorbis_psy.c

#noinst_HEADERS =

libghost_la_LDFLAGS = -version-info @GHOST_LT_CURRENT@:@GHOST_LT_REVISION@:@GHOST_LT_AGE@

noinst_HEADERS = adpcm.h arch.h fftwrap.h ghost.h lifting.h lpc.h misc.h \
	pitch.h smallft.h vorbis_psy.h

noinst_PROGRAMS = testghost
testghost_SOURCES = testghost.c
testghost_LDADD = $(top_builddir)/libghost/libghost.la
