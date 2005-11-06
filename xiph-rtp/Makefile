# GNU Makefile for xiph rtp example code

CC = gcc -g -O2 -Wall
PROGS = vorbisrtp vorbisrtp-client theorartp-client

OGG_CFLAGS = $(shell pkg-config --cflags ogg)
OGG_LIBS = $(shell pkg-config --libs ogg)
VORBIS_CFLAGS = $(shell pkg-config --cflags vorbis)
VORBIS_LIBS = $(shell pkg-config --libs vorbis)
THEORA_CFLAGS = $(shell pkg-config --cflags theora)
THEORA_LIBS = $(shell pkg-config --libs theora)

all : $(PROGS)

clean:
	-$(RM) *.o
	-$(RM) $(PROGS)

vorbisrtp : vorbisrtp.c
	$(CC) -fpack-struct -o $@ $(OGG_CFLAGS) $(VORBIS_CFLAGS) \
		$< $(VORBIS_LIBS) $(OGG_LIBS)

vorbisrtp-client : vorbisrtp-client.c
	$(CC) -fpack-struct -o $@ $(OGG_CFLAGS) $(VORBIS_CFLAGS) \
		$< $(VORBIS_LIBS) $(OGG_LIBS)

theorartp-client :theorartp-client.c
	$(CC) -fpack-struct -o $@ $(OGG_CFLAGS) $< $(OGG_LIBS)
