CC = gcc
RM = rm -rf

CFLAGS = -g -O0 -Wall -DTYPE=int16_t
LFLAGS = 

OBJS = coder.o main.o wavelet.o yuv.o
SRCS = $(OBJS:.o=.c)
TARGET = main


all: .depend $(TARGET)

$(TARGET): .depend $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

coder.c: coder.c.in rle.h
	cpp -P $< | indent -i3 -kr -bad > $@

.c.o: .depend
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJS) $(TARGET) core .depend .depend.bak rle.histogram *.ppm coder.c


.depend: $(SRCS)
	touch .depend
	makedepend -f.depend -- $(CFLAGS) -- $(SRCS) coder.c.in

-include .depend
