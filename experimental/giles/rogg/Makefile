# simple Makefile for rogg and related utilities

OPTS = -g -O2 -Wall

rogg_UTILS = rogg_pagedump rogg_eosfix rogg_aspect

all : librogg.a $(rogg_UTILS)

librogg.a : rogg.o
	$(AR) cr $@ $^
	ranlib $@

rogg_eosfix : rogg_eosfix.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

rogg_pagedump : rogg_pagedump.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

rogg_aspect : rogg_aspect.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

clean :
	-rm -f $(rogg_UTILS)
	-rm -f librogg.a
	-rm -f *.o

.c.o :
	$(CC) $(OPTS) $(CFLAGS) -I. -c $<
