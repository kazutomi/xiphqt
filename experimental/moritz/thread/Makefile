PROG =		thread
LIB =		libthread.a

SRC =		main.c
OBJ =		main.o
LIBSRC =	thread.c
LIBOBJ =	thread.o

THREADS =	-pthread
CC ?=		gcc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-DHAVE_CONFIG_H=1 -DTHREAD_DEBUG=1 -DXALLOC_DEBUG=1 -DXALLOC_WITH_XASPRINTF=1 -fstrict-aliasing -Wall -W -ansi -pedantic -Wwrite-strings -Wpointer-arith -Wmissing-prototypes -Wstrict-prototypes $(THREADS)
DEBUG ?=	-g -ggdb
INCLUDEFLAGS =	-I../compat -I../xalloc -pthread
LDFLAGS =	-L. -L../xalloc -lxalloc -lthread $(THREADS)


all: depend $(LIB) $(PROG)

depend: $(SRC)
	$(CC) -M $(CFLAGS) $(INCLUDEFLAGS) $(SRC) > .depend

.c.o: $(SRC)
	$(CC) $(CFLAGS) $(DEBUG) $(INCLUDEFLAGS) -c $<

lib: $(LIB)

$(LIB): $(LIBOBJ)
	ar cru $(LIB) $(LIBOBJ)
	ranlib $(LIB)

$(PROG): $(LIB) $(OBJ)
	$(CC) $(DEBUG) $(OBJ) $(LDFLAGS) -o $(PROG)

clean:
	-@rm *.o *~ *.core core .depend $(PROG) $(LIB)
