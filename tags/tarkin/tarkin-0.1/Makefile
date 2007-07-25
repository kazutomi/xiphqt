# Generated automatically from Makefile.in by configure.
# $Id: Makefile,v 1.1 2001/02/13 01:06:24 giles Exp $
#
# My test system is Linux 2.1.30, gcc 2.7.2, libc 5.4.27

CC=gcc
CCFLAGS=-lm -g -funsigned-char
TARGETS=tarkin nikrat tarkinplay
PROGOBJS=bitpack.o bitwise.o dwt.o
SDLFLAGS=`sdl-config --cflags`
SDLLIBS=`sdl-config --libs`

%.o: $.c $(CC) $(CCFLAGS) -c -o $@ $<

all:	$(TARGETS)

clean:
	rm -f *.o

tarkin:	tarkin.o $(PROGOBJS)
	$(CC) $(CCFLAGS) -o $@ tarkin.o $(PROGOBJS)

nikrat:	nikrat.o $(PROGOBJS)
	$(CC) $(CCFLAGS) -o $@ nikrat.o $(PROGOBJS)

tarkinplay: tarkinplay.o $(PROGOBJS)
	$(CC) $(CCFLAGS) $(SDLFAGS) -o $@ tarkinplay.o $(PROGOBJS) $(SDLLIBS)

dwt:	dwt.o
	$(CC) $(CCFLAGS) -o $@ dwt.o
