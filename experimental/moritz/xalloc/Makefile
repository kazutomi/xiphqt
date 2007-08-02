PROG =		xalloc-test
LIB =		libxalloc.a

SRC =		main.c
OBJ =		main.o
LIBSRC =	xalloc.c
LIBOBJ =	xalloc.o

COMPAT_INCLUDES = -I../compat
THREADS =	-pthread
CC ?=		gcc
CFLAGS ?=	-O2 -pipe
CFLAGS +=	-DHAVE_CONFIG_H=1 -fstrict-aliasing -Wall -W -ansi -pedantic -Wwrite-strings -Wpointer-arith -Wmissing-prototypes -Wstrict-prototypes $(THREADS)
#CFLAGS +=	-DXALLOC_DEBUG=1
CFLAGS +=	-DXALLOC_WITH_XASPRINTF=1
DEBUG ?=	-g -ggdb
INCLUDEFLAGS =	$(COMPAT_INCLUDES)
LDFLAGS =	-L. -lxalloc $(THREADS)


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
