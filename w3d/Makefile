CC = gcc
RM = rm -rf

CFLAGS = -pg -O2 -Wall -I../ogg/ogg/include

# use 16 bit signed integers as wavelet coefficients
CFLAGS+= -DTYPE=int16_t

# we'll actually use TYPE_BITS bits of type (e.g. 9 magnitude + 1 sign)
CFLAGS+= -DTYPE_BITS=10

# use the rle entropy coder
CFLAGS+= -DRLECODER

# simple malloc debugging
#CFLAGS+= -DDBG_MEMLEAKS

# dump a lot debug images
#CFLAGS+= -DDBG_XFORM

# dump ogg packet infos
#CFLAGS+= -DDBG_OGG

# disable assertions
#CFLAGS+= -DNDEBUG

LFLAGS = -pg -lefence -logg -L../ogg/ogg/src/.libs

OBJS = mem.o pnm.o wavelet.o wavelet_xform.o wavelet_coeff.o \
	yuv.o tarkin.o info.o

TEST_TARGETS = _test_bitcoder _test_rle

SRCS = $(OBJS:.o=.c)
TEST_OBJS = $(TEST_TARGETS:=.o)
TEST_SRCS = $(TEST_OBJS:.o=.c)


all: tarkin_enc tarkin_dec

tarkin_enc: $(OBJS) tarkin_enc.o
	$(CC) $(LFLAGS) $(OBJS) tarkin_enc.o -o $@

tarkin_dec: $(OBJS) tarkin_dec.o
	$(CC) $(LFLAGS) $(OBJS) tarkin_dec.o -o $@

.c.o: .depend
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJS) $(TARGET) gmon.out core .depend .depend.bak rle.histogram
	$(RM) $(TEST_TARGETS) $(TEST_OBJS)
	$(RM) tarkin_enc tarkin_dec tarkin_enc.o tarkin_dec.o
	$(RM) stream.ogg
	$(RM) *.ppm *.pgm

test: .depend $(TEST_TARGETS)
	./_test_bitcoder
	./_test_rle

.depend: $(SRCS) $(TEST_SRCS)
	$(RM) .depend
	touch .depend
	sh tools/makedepend.sh -f.depend -- $(CFLAGS) -- $(SRCS) $(TEST_SRCS) tarkin_enc.c tarkin_dec.c

-include .depend

