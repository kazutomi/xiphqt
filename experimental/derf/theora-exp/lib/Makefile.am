lib_LTLIBRARIES = libtheorabase.la libtheoraenc.la libtheoradec.la

INCLUDES = -I $(top_srcdir)/include
AM_CFLAGS = $(OGG_CFLAGS)

libtheorabase_la_SOURCES = \
	fragment.c \
	idct.c \
	info.c \
	internal.c \
	quant.c \
	state.c

libtheorabase_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@

libtheoraenc_la_SOURCES = \
	bitrate.c \
	encinfo.c \
	encode.c \
	enquant.c \
	fdct.c \
	huffenc.c \
	impmap.c \
	mcenc.c \
	psych.c

libtheoraenc_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@

libtheoradec_la_SOURCES = \
	decinfo.c \
	decode.c \
	dequant.c \
	huffdec.c

libtheoradec_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@
