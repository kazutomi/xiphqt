# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too

TARGETS	= y4oi
CC      = gcc 
LD      = gcc
export INSTALL = install
STRIP   = strip
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
ETCDIR  = /etc/$(TARGET)
MANDIR  = $(PREFIX)/man

SRC  = main.c y4o.c filter.c output.c
OBJ  = main.o y4o.o filter.o output.o

GCF  = `pkg-config --static --cflags "ogg vorbis theora"`
LDF  = `pkg-config --static --libs "ogg vorbis theora"`

all: 
	pkg-config --cflags "ogg vorbis theora" 1>/dev/null
	$(MAKE) target CFLAGS='-O2 -ffast-math $(GCF) $(ADD_DEF) $(CFLAGS)'
	$(STRIP) $(TARGETS)

debug:
	pkg-config --cflags "ogg vorbis theora" 1>/dev/null
	$(MAKE) target CFLAGS='-g -Wall -W -Werror -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) $(ADD_DEF) $(CFLAGS)'

clean:
	rm -f $(OBJ) *.d *.d.* gmon.out $(TARGET)

distclean: clean
	rm -f *~

%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

ifeq ($(MAKECMDGOALS),target)
include $(SRC:.c=.d)
endif

ifeq ($(MAKECMDGOALS),static-target)
include $(SRC:.c=.d)
endif

target:  $(OBJ) 
	$(LD) $(OBJ) $(CFLAGS) -o y4oi $(LDF)

install: target
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 $(TARGETS) $(BINDIR)

