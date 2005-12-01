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

PROGS     = http_strm.cgi

APPSOURCES= http_strm.c 
SRCS= $(APPSOURCES)

APPSOBJ= http_strm.o 

OBJS= $(APPSOBJ)

CFLAGS	+= -Wall

all:	$(PROGS)

http_strm.cgi: $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
	strip-cris http_strm.cgi

clean:
	rm -f $(PROGS) *.o *.bak *~ core 

