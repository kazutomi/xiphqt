CC=gcc
CFLAGS=-g -I/usr/include/python2.3
LDFLAGS=-logg -lvorbis -lvorbisfile

all: _vorbisfile.so

%.o: %.c
	$(CC) $(CFLAGS) -c $*.c

_vorbisfile.so: vorbisfile.o
	ld -shared vorbisfile.o -o _vorbisfile.so $(LDFLAGS)

clean:
	-rm -f *.o *.so
