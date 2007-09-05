# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too

PREFIX    = /usr/local
NAME      = sushivision
MAJOR     = 0
MINOR     = 0
SUBMINOR  = 0

CC        = gcc 
LD        = gcc
INSTALL   = install
STRIP     = strip
LDCONFIG  = /sbin/ldconfig

VERSION   = $(MAJOR).$(MINOR).$(SUBMINOR)
TARGET    = lib$(NAME).so.$(VERSION)
BINDIR    = $(PREFIX)/bin
INCDIR    = $(PREFIX)/include
LIBDIR    = $(PREFIX)/lib
ETCDIR    = /etc/$(TARGET)
MANDIR    = $(PREFIX)/man
SOCFLAGS  = -fPIC
SOLDFLAGS = -shared -nostdlib -Wl,-soname="lib$(NAME).so.$(MAJOR)"

SRC       = main.c scale.c plot.c slider.c slice.c spinner.c panel.c panel-1d.c panel-2d.c \
	panel-xy.c mapping.c dimension.c function.c objective.c undo.c gtksucks.c xml.c \
	tokens.c example_fractal.c example_discrete.c example_chirp.c example_spirograph.c
INC       = sushivision.h sushimacro.h
MAN	  =
EXAMPLES  = sushivision_fractal #sushivision_discrete sushivision_chirp
EX_OBJ    = example_fractal.o #example_discrete.o example_chirp.o example_spirograph.o
OBJ       = main.o scale.o plot.o slider.o slice.o spinner.c panel.o panel-1d.o panel-2d.o \
	panel-xy.o mapping.o dimension.o function.o objective.o undo.o gtksucks.o xml.o \
	tokens.c
LIBS      = -lpthread -ldl
CAIROVER  =  >= 1.4.1
GTKVER    =  >= 2.10.0
PKGARG	  = "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) cairo-ft $(CAIROVER) gthread-2.0 libxml-2.0"
GCF       = -std=gnu99 `pkg-config --cflags $(PKGARG)`
LDF       = -pthread -L/lib -ldl -rdynamic `pkg-config --libs $(PKGARG)`

all:    
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) target CFLAGS='-O2 -g $(SOCFLAGS) $(GCF) $(ADD_DEF)'
	$(MAKE) examples CFLAGS='-O2 -g $(GCF) $(ADD_DEF)'

debug:
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) target CFLAGS='-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(SOCFLAGS) $(GCF) $(ADD_DEF)'
	$(MAKE) examples CFLAGS='-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) $(ADD_DEF)'

profile:
	pkg-config --cflags $(PKGARG) 1>/dev/null
	$(MAKE) examples CFLAGS='-pg -g -O2 $(GCF) $(ADD_DEF)' 

clean:
	rm -f *.o *.d *.d.* *.pc gmon.out $(TARGET) sushivision_* sushi-gtkrc.h

distclean: clean
	rm -f core core.* *~ 

%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

main.o: sushi-gtkrc.h

ifeq ($(MAKECMDGOALS),target)
include $(SRC:.c=.d)
endif

ifeq ($(MAKECMDGOALS),static-target)
include $(SRC:.c=.d)
endif

sushivision.pc: Makefile
	rm -f sushivision.pc
	touch sushivision.pc
	echo "prefix = $(PREFIX)" >> sushivision.pc
	echo "version = $(VERSION)" >> sushivision.pc
	echo "gtkver = $(GTKVER)" >> sushivision.pc
	echo "cairover = $(CAIROVER)" >> sushivision.pc
	cat sushivision.pc.in >> sushivision.pc	

sushi-gtkrc.h: sushi-gtkrc.in
	rm -f sushi-gtkrc.h
	touch sushi-gtkrc.h
	echo "static char *_SUSHI_GTKRC_STRING=" >> sushi-gtkrc.h
	sed -e 's/\(\"\)/\\\"/g' -e 's/^\(.*\)$$/"\1\\n"/' sushi-gtkrc.in >> sushi-gtkrc.h
	echo "\"\";" >> sushi-gtkrc.h

target:  $(OBJ) examples sushivision.pc
	$(LD) $(OBJ) $(CFLAGS) $(SOLDFLAGS) -o $(TARGET) $(LIBS) $(LDF)

examples:  $(OBJ) $(EX_OBJ)
	$(LD) $(OBJ) example_fractal.o $(CFLAGS) -o sushivision_fractal $(LIBS) $(LDF)
	#$(LD) $(OBJ) example_discrete.o $(CFLAGS) -o sushivision_discrete $(LIBS) $(LDF)
	#$(LD) $(OBJ) example_chirp.o $(CFLAGS) -o sushivision_chirp $(LIBS) $(LDF)
	#$(LD) $(OBJ) example_spirograph.o $(CFLAGS) -o sushivision_spirograph $(LIBS) $(LDF)

$(TARGET): all

install: $(TARGET)
	$(INSTALL) -d -m 0755 $(INCDIR)
	$(INSTALL) -m 0644 $(INC) $(INCDIR)
#	$(INSTALL) -d -m 0755 $(MANDIR)
#	$(INSTALL) -m 0644 $(MAN) $(MANDIR)
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 $(EXAMPLES) $(BINDIR)
	$(INSTALL) -d -m 0755 $(LIBDIR)
	$(INSTALL) -d -m 0755 $(LIBDIR)/pkgconfig
	$(INSTALL) -m 0755 $(TARGET) $(LIBDIR)
	$(INSTALL) -m 0755 sushivision.pc $(LIBDIR)/pkgconfig
	$(LDCONFIG)
	ln -fs $(LIBDIR)/$(TARGET) $(LIBDIR)/lib$(NAME).so
