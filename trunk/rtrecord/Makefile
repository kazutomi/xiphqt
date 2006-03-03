# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too

TARGET  = rtrecord
ALTTARGET1 = record
CC      = gcc 
LD      = gcc
INSTALL = install
STRIP   = strip
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
ETCDIR  = /etc/$(TARGET)
MANDIR  = $(PREFIX)/man

SRC  = main.c tty.c
OBJ  = main.o tty.o

GCF  = $(CFLAGS)
LDF  = $(LDFLAGS)
LIBS = -lncurses -lpthread -lm -lasound

all:    
	$(MAKE) target CFLAGS='-O2 -ffast-math $(GCF) $(ADD_DEF)'
	$(STRIP) $(TARGET)

debug:
	$(MAKE) target CFLAGS='-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) $(ADD_DEF)'

profile:
	$(MAKE) target CFLAGS='-pg -g -O2 -ffast-math $(GCF) $(ADD_DEF)' LIBS='-lgprof-helper $(LIBS)' 

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
	chmod u+s $(BINDIR)/$(TARGET)
	-ln -fs $(BINDIR)/$(TARGET) $(BINDIR)/$(ALTTARGET1)
