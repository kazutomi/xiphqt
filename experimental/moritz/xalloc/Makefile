PROG =		xalloc-test

SRC =		main.c xalloc.c
OBJ =		main.o xalloc.o

CC ?=		gcc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-DHAVE_CONFIG_H=1 -fstrict-aliasing -Wall -W -ansi -pedantic -Wwrite-strings -Wpointer-arith
DEBUG ?=	-g -ggdb
INCLUDEFLAGS =	-I../compat
LDFLAGS =	


all: depend $(PROG)

depend: $(SRC)
	$(CC) -M $(CFLAGS) $(INCLUDEFLAGS) $(SRC) > .depend

.c.o: $(SRC)
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDEFLAGS) -c $<

$(PROG): $(OBJ)
	$(CC) $(DEBUG) $(OBJ) $(LDFLAGS) -o $(PROG)

clean:
	-@rm *.o *~ *.core core .depend $(PROG)
