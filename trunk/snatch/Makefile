BINDIR  = /usr/local/bin
LIBDIR  = /usr/local/lib

CC	= gcc
CFLAGS	= -O2 -W
SOFLAGS = -shared
INSTALL = install

all:		libsnatch.so snatch2yuv snatch2yuv2 snatch2wav

libsnatch.so: 	libsnatch.c x11.c oss.c esd.c realppm.h snatchppm.h waitppm.h
	$(CC) $(CFLAGS) $(SOFLAGS) libsnatch.c -o libsnatch.so

snatch2yuv: 	snatchconvert.c snatch2yuv.c
	$(CC) $(CFLAGS) snatchconvert.c snatch2yuv.c -o snatch2yuv -lm

snatch2yuv2: 	snatch2yuv
	ln -s snatch2yuv snatch2yuv2

snatch2wav: 	snatchconvert.c snatch2wav.c
	$(CC) $(CFLAGS) snatchconvert.c snatch2wav.c -o snatch2wav -lm

install: 	all
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -d -m 0755 $(LIBDIR)
	$(INSTALL) -m 0755 ./snatch2yuv $(BINDIR)
	$(INSTALL) -m 0755 ./snatch2yuv2 $(BINDIR)
	$(INSTALL) -m 0755 ./snatch2wav $(BINDIR)
	$(INSTALL) -m 0755 ./Snatch $(BINDIR)
	$(INSTALL) -m 0644 ./libsnatch.so $(LIBDIR)

clean:
	rm -f libsnatch.so snatch2yuv snatch2yuv2 snatch2wav