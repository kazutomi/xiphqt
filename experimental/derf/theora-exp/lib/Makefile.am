lib_LTLIBRARIES = libtheorabase.la libtheoraenc.la libtheoradec.la

INCLUDES = -I $(top_srcdir)/include
AM_CFLAGS = $(OGG_CFLAGS)

noinst_HEADERS = \
	dct.h \
	decint.h \
	dequant.h \
	encint.h \
	enquant.h \
	fdct.h \
	huffdec.h \
	huffenc.h \
	huffman.h \
	idct.h \
	internal.h \
	ocintrin.h \
	psych.h \
	quant.h 

libtheorabase_la_SOURCES = \
	fragment.c \
	idct.c \
	info.c \
	internal.c \
	quant.c \
	state.c

libtheorabase_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@ @OGG_LIBS@

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

libtheoraenc_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@ @OGG_LIBS@

libtheoradec_la_SOURCES = \
	decinfo.c \
	decode.c \
	dequant.c \
	huffdec.c

libtheoradec_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@ @OGG_LIBS@

