# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too

PREFIX    = /usr/local
NAME      = pngxpdf
MAJOR     = 0
MINOR     = 0
SUBMINOR  = 0

CC        = gcc 
LD        = gcc
INSTALL   = install
STRIP     = strip
LDCONFIG  = /sbin/ldconfig

VERSION   = $(MAJOR).$(MINOR).$(SUBMINOR)
TARGET    = $(NAME)
BINDIR    = $(PREFIX)/bin
INCDIR    = $(PREFIX)/include
LIBDIR    = $(PREFIX)/lib
MANDIR    = $(PREFIX)/man

SRC       = main.c
MAN	  =
OBJ       = main.o
CAIROVER  =  >= 1.4.1
PKGARG	  = "cairo $(CAIROVER) cairo-ft $(CAIROVER) "
GCF       = -std=gnu99 `pkg-config --cflags $(PKGARG)` -DVERSION="\"$(VERSION)\""
LDF       = -L/lib `pkg-config --libs $(PKGARG)`

all:    
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) target CFLAGS='-O2 -g $(GCF) '

debug:
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) target CFLAGS='-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) '

profile:
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) target CFLAGS='-g -pg -O2 $(GCF) '

clean:
	rm -f *.o *.d *.d.* *.pc gmon.out $(TARGET)

distclean: clean
	rm -f core core.* *~ 

%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

ifeq ($(MAKECMDGOALS),target)
include $(SRC:.c=.d)
endif

target:  $(OBJ) 
	$(LD) $(OBJ) $(CFLAGS) -o $(TARGET) $(LIBS) $(LDF)

$(TARGET): all

install: $(TARGET)
#	$(INSTALL) -d -m 0755 $(MANDIR)
#	$(INSTALL) -m 0644 $(MAN) $(MANDIR)
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 $(TARGET) $(BINDIR)
