# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too

TARGET  = sushivision
CC      = gcc 
LD      = gcc
INSTALL = install
STRIP   = strip
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
ETCDIR  = /etc/$(TARGET)
MANDIR  = $(PREFIX)/man

SRC  = main.c scale.c plot.c slider.c slice.c panel.c panel-2d.c mapping.c dimension.c objective.c example_submain.c
OBJ  = main.o scale.o plot.o slider.o slice.o panel.o panel-2d.o mapping.o dimension.o objective.o example_submain.o
INC  = sushivision.h
LIBS = -lpthread
CAIROVER =  >= 1.0.0
GTKVER   =  >= 2.8.0
GCF  = `pkg-config --static --cflags "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2 gthread-2.0"`
LDF  = -pthread -L/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lpango-1.0 -lgobject-2.0 -lgmodule-2.0 -ldl -lm -lfontconfig -lpng12 -lXrender -lX11 -lpthread -lfreetype -lz -lgthread-2.0 -lglib-2.0 -lcairo

all:    
	pkg-config --cflags "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2" 1>/dev/null
	$(MAKE) target CFLAGS='-O2 -ffast-math $(GCF) $(ADD_DEF)'
	$(STRIP) $(TARGET)

debug:
	pkg-config --cflags "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2" 1>/dev/null
	$(MAKE) target CFLAGS='-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) $(ADD_DEF)'

profile:
	pkg-config --cflags "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2" 1>/dev/null
	$(MAKE) target CFLAGS='-pg -g -O2 -ffast-math $(GCF) $(ADD_DEF)' LIBS='-lgprof-helper'

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
	$(LD) $(OBJ) $(CFLAGS) -o $(TARGET) $(LIBS) $(LDF)

install: target
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 $(TARGET) $(BINDIR)
