# GNU Makefile for xiph rtp example code

CC = gcc -g -O2 -Wall -DCHECK -DDEBUG
PROGS = vorbisrtp theorartp vorbisrtp-client theorartp-client

vorbisrtp_SRCS = vorbisrtp.c
vorbisrtp_PKGS = ogg vorbis

theorartp_SRCS = vorbisrtp.c
theorartp_PKGS = ogg theora

vorbisrtp-client_SRCS = vorbisrtp-client.c
vorbisrtp-client_PKGS = ogg vorbis

theorartp-client_SRCS = theorartp-client.c
theorartp-client_PKGS = ogg

all : $(PROGS)

clean:
	-$(RM) *.o
	-$(RM) $(PROGS)

vorbisrtp : vorbisrtp.c xiph_rtp.c
	$(CC) -o $@ `pkg-config --cflags $($@_PKGS)` \
		$^ `pkg-config --libs $($@_PKGS)`

theorartp : theorartp.c xiph_rtp.c
	$(CC) -o $@ `pkg-config --cflags $($@_PKGS)` \
		$^ `pkg-config --libs $($@_PKGS)`

vorbisrtp-client : vorbisrtp-client.c
	$(CC) -o $@  `pkg-config --cflags $($@_PKGS)` \
		$< `pkg-config --libs $($@_PKGS)`

theorartp-client :theorartp-client.c
	$(CC) -o $@ `pkg-config --cflags $($@_PKGS)` \
		$< `pkg-config --libs $($@_PKGS)`
