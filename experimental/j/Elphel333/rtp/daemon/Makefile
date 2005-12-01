#
# Makefile for the Elphel streamer.
# This probably requires GNU make.
#
AXIS_USABLE_LIBS = UCLIBC GLIBC
include $(AXIS_TOP_DIR)/tools/build/Rules.axis

THEORACOMMON=../../common
THEORACOMMON_OBJS=$(THEORACOMMON)/oggtheora.o $(THEORACOMMON)/utils.o

# Location of includes and library
CSRC   = rtp

# Library name
LNAME  = uclmmbase

DEFS   = -DHAVE_CONFIG_H
CFLAGS = -O2 -W  -Wwrite-strings -Wbad-function-cast -Wmissing-prototypes -Wcast-qual -Wmissing-declarations  $(DEFS) -I$(CSRC) -I$(THEORACOMMON)
LIBS   = -lm -L$(CSRC)/.libs -l$(LNAME) 

LD	= ld-cris

TARGET = TheoraStrm
OBJS   = TheoraStrm.o
SRCS   = $(OBJS:%.o=%.c)

all: $(TARGET)

$(TARGET): $(OBJS) $(CSRC)/lib$(LNAME).a  $(THEORACOMMON_OBJS)
	 $(CC) $(CFLAGS) -o $@ $(OBJS) $(THEORACOMMON_OBJS) $(LIBS)
	 strip-cris $@

.c.o:
	$(CC) $(CFLAGS) $(INC) -c $<

$(THEORACOMMON_OBJS): 
	cd $(THEORACOMMON) && $(MAKE)

$(CSRC)/lib$(LNAME).a:
	cd $(CSRC) && $(MAKE)

clean:
	-rm -f *.o $(OBJS) $(TARGET)

distclean: clean
	-rm -f Makefile
