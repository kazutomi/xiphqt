CC = gcc
RM = rm -rf

CFLAGS = -pg -O0 -Wall -DTYPE=int16_t -DRLECODER -DDBG_XFORM -DDBG_MEMLEAKS
LFLAGS = -pg -lefence

OBJS = mem.o pnm.o wavelet.o wavelet_xform.o yuv.o tarkin.o tarkin-io.o

TEST_TARGETS = _test_bitcoder _test_rle _test_huffman

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
	$(RM) $(OBJS) $(TARGET) gmon.out core .depend .depend.bak rle.histogram *.ppm
	$(RM) $(TEST_TARGETS) $(TEST_OBJS)
	$(RM) tarkin_enc tarkin_dec tarkin_enc.o tarkin_dec.o


test: .depend $(TEST_TARGETS)
	./_test_bitcoder
	./_test_huffman
	./_test_rle


.depend: $(SRCS) $(TEST_SRCS)
	$(RM) .depend
	touch .depend
	sh tools/makedepend.sh -f.depend -- $(CFLAGS) -- $(SRCS) $(TEST_SRCS) tarkin_enc.c tarkin_dec.c

-include .depend
