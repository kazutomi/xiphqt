#
# Makefile for the Elphel streamer.
# This probably requires GNU make.
#
AXIS_USABLE_LIBS = UCLIBC GLIBC
include $(AXIS_TOP_DIR)/tools/build/Rules.axis



DEFS   = -DHAVE_CONFIG_H
CFLAGS = -O2 -W  -Wwrite-strings -Wbad-function-cast -Wmissing-prototypes -Wcast-qual -Wmissing-declarations  $(DEFS) 
LIBS   = -lm 

LD	= ld-cris

OBJS=oggtheora.o utils.o cgi.o
SRCS=$(OBJS:%.o=%.c)

all: $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $(INC) -c $<

clean:
	-rm -f *.o *~
