# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too

PREFIX    = ~/.gimp-2.4
MAJOR     = 0
MINOR     = 0
SUBMINOR  = 0

CC        = gcc 
LD        = gcc
INSTALL   = install
STRIP     = strip
LDCONFIG  = /sbin/ldconfig

TARGETS   = denoise scanclean
VERSION   = $(MAJOR).$(MINOR).$(SUBMINOR)
BINDIR    = $(PREFIX)/plug-ins

MAN	  =
PKGARG	  = "gimp-2.0 gimpui-2.0 gtk+-2.0 >= 2.10 "
GCF       = -std=gnu99 `pkg-config --cflags $(PKGARG)` -DVERSION="\"$(VERSION)\""
LDF       = -L/lib `pkg-config --libs $(PKGARG)`

all:    
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) montypak CFLAGS='-O2 -g $(GCF) '

debug:
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) montypak CFLAGS='-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) '

profile:
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) montypak CFLAGS='-g -pg -O2 $(GCF) '

clean:
	rm -f *.o *.d *.d.* *.pc gmon.out $(TARGETS)

distclean: clean
	rm -f core core.* *~ 

%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

ifeq ($(MAKECMDGOALS),target)
include $(SRC:.c=.d)
endif

montypak: $(TARGETS)

denoise.o: wavelet.c

denoise:  denoise.o
	$(LD) $< $(CFLAGS) -o $@ $(LIBS) $(LDF)

scanclean.o: blur.c

scanclean: scanclean.o 
	$(LD) $< $(CFLAGS) -o $@ $(LIBS) $(LDF)

install: all
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 $(TARGETS) $(BINDIR)
