CC = gcc
RM = rm -rf

CFLAGS = -g -O0 -Wall -DTYPE=int16_t -DRLECODER
LFLAGS = -g

OBJS = wavelet.o wavelet_xform.o yuv.o tarkin.o main.o
SRCS = $(OBJS:.o=.c)
TEST_TARGETS = _test_bitcoder _test_rle _test_huffman
TEST_OBJS = $(TEST_TARGETS:=.o)
TEST_SRCS = $(TEST_OBJS:.o=.c)
TARGET = main


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

.c.o: .depend
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJS) $(TARGET) gmon.out core .depend .depend.bak rle.histogram *.ppm
	$(RM) $(TEST_TARGETS) $(TEST_OBJS)


test: .depend $(TEST_TARGETS)
	./_test_bitcoder
	./_test_huffman
	./_test_rle


.depend: $(SRCS) $(TEST_SRCS)
	$(RM) .depend
	touch .depend
	sh makedepend -f.depend -- $(CFLAGS) -- $(SRCS) $(TEST_SRCS)

-include .depend
