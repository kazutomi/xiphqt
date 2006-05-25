## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
  theoraenc.h

if TH_ENCODER
TH_ENCODER_FILES= \
  theoraenc.h
else
TH_ENCODER_FILES=
endif

theoraincludedir = $(includedir)/theora

theorainclude_HEADERS = codec.h theoradec.h $(TH_ENCODER_FILES)

