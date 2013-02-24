CC=gcc
LD=gcc
INSTALL=install
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/man

SRC = gtk-bounce-panel.c gtk-bounce-audio.c gtk-bounce-widget.c twopole.c \
      gtk-bounce-dither.c gtk-bounce-filter.c gtk-bounce-wavheader.c
OBJ = gtk-bounce-panel.o gtk-bounce-audio.o gtk-bounce-widget.o twopole.o \
      gtk-bounce-dither.o gtk-bounce-filter.o gtk-bounce-wavheader.o

GCF = `pkg-config --cflags gtk+-2.0` -DETCDIR=$(ETCDIR)

CFLAGS := ${CFLAGS} $(GCF) $(ADD_DEF)
LIBS   := -lfftw3 -lfftw3f -lasound -lm -lpthread

all:
	$(MAKE) target CFLAGS="${CFLAGS} -g -O2 -ffast-math -fomit-frame-pointer"

debug:
	$(MAKE) target CFLAGS="${CFLAGS} -g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES"

profile:
	$(MAKE) target CFLAGS="${CFLAGS} -pg -g -O2 -ffast-math"

clean:
	rm -f $(OBJ) *.d *.d.* gmon.out gtk-bounce

%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

ifeq ($(MAKECMDGOALS),target)
include $(SRC:.c=.d)
endif

gtk-bounce:  $(OBJ)
	$(LD) $(OBJ) -o gtk-bounce $(LIBS) $(CFLAGS) `pkg-config --libs gtk+-2.0`

target:  gtk-bounce

