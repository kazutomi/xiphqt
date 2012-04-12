# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too


# Use the below line to build for PowerPC
# The PPC build *must* use -maltivec, even if the target is a non-altivec machine

#ADD_DEF= -DUGLY_IEEE754_FLOAT32_HACK=1 -maltivec -mcpu=7400

# use the below for x86 and most other platforms where 'float' is 32 bit IEEE754

#ADD_DEF= -DUGLY_IEEE754_FLOAT32_HACK=1

# use the below for anything without IEE754 floats (eg, VAX)

# ADD_DEF=


CC=gcc 
LD=gcc
INSTALL=install
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
ETCDIR=/etc/spectrum
MANDIR=$(PREFIX)/man

SRC = main.c process.c panel.c plot.c io.c
OBJ = main.o process.o panel.o plot.o io.o
GCF = -DETCDIR=\\\"$(ETCDIR)\\\" `pkg-config --cflags gtk+-2.0`

all:	
	$(MAKE) target CFLAGS="-O3 -ffast-math -fomit-frame-pointer $(GCF) $(ADD_DEF)"

debug:
	$(MAKE) target CFLAGS="-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) $(ADD_DEF)"

profile:
	$(MAKE) target CFLAGS="-pg -g -O3 -ffast-math $(GCF) $(ADD_DEF)" LIBS="-lgprof-helper "

clean:
	rm -f $(OBJ) *.d *.d.* gmon.out spectrum

distclean: clean
	rm -f spectrum-wisdomrc

%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

spectrum-wisdomrc:
	fftwf-wisdom -v -o spectrum-wisdomrc \
	rif8192 rib8192

ifeq ($(MAKECMDGOALS),target)
include $(SRC:.c=.d)
endif

target:  $(OBJ) spectrum-wisdomrc
	./touch-version
	$(LD) $(OBJ) $(CFLAGS) -o spectrum $(LIBS) `pkg-config --libs gtk+-2.0` -lpthread -lfftw3f -lm 

install: target
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 spectrum $(BINDIR)
	$(INSTALL) -d -m 0755 $(ETCDIR)
	$(INSTALL) -m 0644 spectrum-gtkrc $(ETCDIR)
	$(INSTALL) -m 0644 spectrum-wisdomrc $(ETCDIR)
#	$(INSTALL) -d -m 0755 $(MANDIR)
#	$(INSTALL) -d -m 0755 $(MANDIR)/man1
#	$(INSTALL) -m 0644 spectrum.1 $(MANDIR)/man1

