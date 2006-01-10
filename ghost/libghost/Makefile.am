# Disable automatic dependency tracking if using other tools than gcc and gmake
#AUTOMAKE_OPTIONS = no-dependencies


#EXTRA_DIST=

#INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir) @OGG_CFLAGS@

lib_LTLIBRARIES = libghost.la

# Sources for compilation in the library
libghost_la_SOURCES = ghost.c sinusoids.c pitch.c lifting.c \
	smallft.c fftwrap.c misc.c

#noinst_HEADERS =

libghost_la_LDFLAGS = -version-info @GHOST_LT_CURRENT@:@GHOST_LT_REVISION@:@GHOST_LT_AGE@

noinst_HEADERS = ghost.h pitch.h lifting.h smallft.h fftwrap.h arch.h misc.h
noinst_PROGRAMS = testghost
testghost_SOURCES = testghost.c
testghost_LDADD = $(top_builddir)/libghost/libghost.la
