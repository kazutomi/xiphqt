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

SRC = spectrum.c waveform.c spec_process.c wave_process.c spec_panel.c wave_panel.c spec_plot.c wave_plot.c io.c
SPECTRUM_OBJ = spectrum.o spec_process.o spec_panel.o spec_plot.o io.o
WAVEFORM_OBJ = waveform.o wave_process.o wave_panel.o wave_plot.o io.o
OBJ = $(SPECTRUM_OBJ) $(WAVEFORM_OBJ)

GCF = `pkg-config --cflags gtk+-3.0` -DETCDIR=$(ETCDIR) -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE #-DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE

CFLAGS := ${CFLAGS} $(GCF) $(ADD_DEF)

all:	
	$(MAKE) target CFLAGS="${CFLAGS} -O3 -ffast-math -fomit-frame-pointer"

debug:
	$(MAKE) target CFLAGS="${CFLAGS} -g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES"

profile:
	$(MAKE) target CFLAGS="${CFLAGS} -pg -g -O3 -ffast-math"

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

spectrum:  $(SPECTRUM_OBJ) spectrum-wisdomrc
	./touch-version
	$(LD) $(SPECTRUM_OBJ) -o spectrum $(LIBS) $(CFLAGS) `pkg-config --libs gtk+-3.0` -lpthread -lfftw3f -lm 

waveform:  $(WAVEFORM_OBJ) 
	./touch-version
	$(LD) $(WAVEFORM_OBJ) -o waveform $(LIBS) $(CFLAGS) `pkg-config --libs gtk+-3.0` -lpthread -lm 

target:  spectrum # waveform

install: target
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 spectrum $(BINDIR)
	$(INSTALL) -m 0755 waveform $(BINDIR)
	$(INSTALL) -d -m 0755 $(ETCDIR)
	$(INSTALL) -m 0644 spectrum-gtkrc $(ETCDIR)
	$(INSTALL) -m 0644 waveform-gtkrc $(ETCDIR)
	$(INSTALL) -m 0644 spectrum-wisdomrc $(ETCDIR)
#	$(INSTALL) -d -m 0755 $(MANDIR)
#	$(INSTALL) -d -m 0755 $(MANDIR)/man1
#	$(INSTALL) -m 0644 spectrum.1 $(MANDIR)/man1

