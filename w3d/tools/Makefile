CC = gcc
RM = rm -rf

CFLAGS = -g -O0 -Wall -I.. -DTYPE=uint16_t
LFLAGS = -g

TARGETS = pnmdiff yuv2ppm deinterlace

OBJS = $(TARGETS:=.o) ../pnm.o
SRCS = $(OBJS:.o=.c)


all: $(OBJS) $(TARGETS)

pnmdiff: pnmdiff.o ../pnm.o
	$(CC) $(LFLAGS) pnmdiff.o ../pnm.o -o $@

yuv2ppm: yuv2ppm.o ../pnm.o
	$(CC) $(LFLAGS) yuv2ppm.o ../pnm.o -o $@

deinterlace: deinterlace.o ../pnm.o
	$(CC) $(LFLAGS) deinterlace.o ../pnm.o -o $@

.c.o: .depend
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGETS) gmon.out core .depend .depend.bak


.depend: $(SRCS)
	$(RM) .depend
	touch .depend
	sh ../tools/makedepend.sh -f.depend -- $(CFLAGS) -- $(SRCS)

-include .depend
