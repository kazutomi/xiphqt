CC = gcc
RM = rm -rf

CFLAGS = -g -O0 -Wall
LFLAGS = 

SRCS = coder.c main.c wavelet.c yuv.c
OBJS = $(SRCS:.c=.o)
TARGET = main


all: .depend $(TARGET)

$(TARGET): .depend $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

.c.o: .depend
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJS) $(TARGET) core .depend .depend.bak rle.histogram *.ppm


.depend: $(SRCS)
	touch .depend
	makedepend -f.depend -- $(CFLAGS) -- $(SRCS)

-include .depend
