AXIS_USABLE_LIBS = UCLIBC GLIBC
include $(AXIS_TOP_DIR)/tools/build/Rules.axis

INCDIR    = $(prefix)/include
LIBDIR    = $(prefix)/lib

INSTDIR   = $(prefix)/bin
INSTMODE  = 0755

INSTOWNER = root
INSTGROUP = root

CFLAGS   += -Wall -g -I$(INCDIR) -I$(AXIS_KERNEL_DIR)/include
LDFLAGS  += -L$(LIBDIR) 

ifndef UCLIBC
# -lm - to use math library
LDLIBS = -lcrypt -lm
endif

# miscellaneous OS-dependent stuff
# linker
LN= $(CC)
# file deletion command
RM= rm -f
# file rename command
MV= mv
# End of configurable options.

PROGS     = cstrm.cgi

APPSOURCES= cstrm.c 
SRCS= $(APPSOURCES)

APPSOBJ= cstrm.o 

OBJS= $(APPSOBJ)

CFLAGS	+= -Wall

all:	$(PROGS)

cstrm.cgi: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	strip-cris cstrm.cgi

clean:
	rm -f $(PROGS) *.o *.bak *~ core 

