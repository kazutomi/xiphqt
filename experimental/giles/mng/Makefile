# GNU makefile for mng stuff

PROGS = mngplay mng

CFLAGS = -g -O2 -Wall

mngplay_SRCS = mngplay.c
mngplay_OBJS = $(mngplay_SRCS:.c=.o)
mngplay_CFLAGS = `sdl-config --cflags`
mngplay_LIBS = `sdl-config --libs` -lmng -lz -ljpeg -llcms -logg

mng_SRCS = mng.c
mng_OBJS = $(mng_SRCS:.c=.o)
mng_LIBS = -logg

all : $(PROGS)

clean :
	$(RM) $(mngplay_OBJS)
	$(RM) $(PROGS)

.PHONY: all clean

mngplay : CFLAGS += $(mngplay_CFLAGS)
mngplay : LDFLAGS += $(mngplay_LIBS)
mngplay : $(mngplay_OBJS)

mng : CFLAGS += $(mng_CFLAGS)
mng : LDFLAGS += $(mng_LIBS)
mng : $(mng_OBJS)
