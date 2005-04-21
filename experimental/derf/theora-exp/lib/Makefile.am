lib_LTLIBRARIES = libtheorabase.la libtheoraenc.la libtheoradec.la

INCLUDES = -I $(top_srcdir)/include
AM_CFLAGS = $(OGG_CFLAGS) @OPTIMIZATION_CFLAGS@

if OC_X86ASM
X86ASMSOURCES = \
	x86/mmxstate.c \
	x86/x86state.c \
	x86/mmxfrag.c \
	x86/cpu.c 
X86ASMHEADERS = 
else
X86ASMSOURCES = 
X86ASMHEADERS = \
	x86/mmxstate.c \
	x86/x86state.c \
	x86/mmxfrag.c \
	x86/cpu.c 
endif
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
	quant.h \
	x86/x86int.h \
	x86/cpu.h \
	$(X86ASMHEADERS)


libtheorabase_la_SOURCES = \
	fragment.c \
	idct.c \
	info.c \
	internal.c \
	quant.c \
	state.c \
	$(X86ASMSOURCES)


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
