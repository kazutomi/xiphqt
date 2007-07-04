# Disable automatic dependency tracking if using other tools than gcc and gmake
#AUTOMAKE_OPTIONS = no-dependencies


#EXTRA_DIST=

#INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir)

lib_LTLIBRARIES = libentcode.la

# Sources for compilation in the library
libentcode_la_SOURCES = bitree.c entcode.c probmod.c bitrdec.c entdec.c \
	mfrngdec.c probdec.c bitrenc.c entenc.c mfrngenc.c probenc.c

#noinst_HEADERS =

libentcode_la_LDFLAGS = -version-info @GHOST_LT_CURRENT@:@GHOST_LT_REVISION@:@GHOST_LT_AGE@

noinst_HEADERS = bitree.h ecintrin.h entcode.h probmod.h bitrdec.h \
	mfrngcod.h probdec.h bitrenc.h mfrngcod.h probenc.h

noinst_PROGRAMS = ectest
ectest_SOURCES = ectest.c
ectest_LDADD = $(top_builddir)/libentcode/libentcode.la
