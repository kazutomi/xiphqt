#
# Makefile for the Elphel streamer.
# This probably requires GNU make.
#
AXIS_USABLE_LIBS = UCLIBC GLIBC
include $(AXIS_TOP_DIR)/tools/build/Rules.axis

THEORACOMMON=../common
THEORACOMMON_OBJS=$(THEORACOMMON)/oggtheora.o $(THEORACOMMON)/utils.o

DEFS   = -DHAVE_CONFIG_H
CFLAGS = -O2 -W  -Wwrite-strings -Wbad-function-cast -Wmissing-prototypes -Wcast-qual -Wmissing-declarations  $(DEFS) -I$(THEORACOMMON)
LIBS   = -lm 

LD	= ld-cris

TARGET = TheoraShout
OBJS   = TheoraShout.o
SRCS   = $(OBJS:%.o=%.c)

all: $(TARGET)

$(TARGET): $(OBJS) $(THEORACOMMON_OBJS)
	 $(CC) $(CFLAGS) -o $@ $(OBJS) $(THEORACOMMON_OBJS) $(LIBS)
	 strip-cris $@

.c.o:
	$(CC) $(CFLAGS) $(INC) -c $<

$(THEORACOMMON_OBJS): 
	cd $(THEORACOMMON) && $(MAKE)

clean:
	-rm -f *.o $(OBJS) $(TARGET)
