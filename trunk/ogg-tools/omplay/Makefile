CFLAGS=-O3 -g
LDFLAGS=
LIBS=-logg -lvorbis -lasound

all: omplay

omplay: omplay.o midi.o
	$(CC) $(CFLAGS) -o $@ omplay.o midi.o $(LDFLAGS) $(LIBS)

omplay.o: omplay.c

midi.o: midi.c midi.h

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f omplay midi.o omplay.o
