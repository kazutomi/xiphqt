if TH_ENCODER
ENCODER_LA=libtheoraenc.la
else
ENCODER_LA=
endif

lib_LTLIBRARIES = libtheoradec.la libtheoracompat.la $(ENCODER_LA)

INCLUDES = -I $(top_srcdir)/include -I $(top_srcdir)/include/theora/compat
AM_CFLAGS = $(OGG_CFLAGS) $(OPTIMIZATION_CFLAGS)

EXTRA_DIST = \
	x86/cpu.c \
	x86/mmxidct.c \
	x86/mmxfrag.c \
	x86/mmxstate.c \
	x86/x86state.c

if OC_X86ASM
X86ASM_FILES = \
	x86/cpu.c \
	x86/mmxidct.c \
	x86/mmxfrag.c \
	x86/mmxstate.c \
	x86/x86state.c
endif

noinst_HEADERS = \
	apiwrapper.h \
	dct.h \
	decint.h \
	dequant.h \
	encint.h \
	enquant.h \
	encvbr.h \
	fdct.h \
	huffdec.h \
	huffenc.h \
	huffman.h \
	idct.h \
	internal.h \
	ocintrin.h \
	psych.h \
	quant.h \
	x86/cpu.h \
	x86/x86int.h

libtheoracompat_la_SOURCES = \
	apiwrapper.c \
	decapiwrapper.c
#	encapiwrapper.c

libtheoradec_la_SOURCES = \
	decinfo.c \
	decode.c \
	dequant.c \
	fragment.c \
	huffdec.c \
	idct.c \
	info.c \
	internal.c \
	quant.c \
	state.c  \
	$(X86ASM_FILES)


libtheoradec_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@ @OGG_LIBS@

libtheoraenc_la_SOURCES = \
	bitrate.c \
	encinfo.c \
	encode.c \
	encmsc.c \
	encvbr.c \
	enquant.c \
	fdct.c \
	huffenc.c \
	impmap.c \
	mcenc.c \
	psych.c

libtheoraenc_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@ @OGG_LIBS@

debug:
	$(MAKE) all CFLAGS="@DEBUG@" LDFLAGS="-lefence"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

