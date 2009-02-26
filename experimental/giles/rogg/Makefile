# simple Makefile for rogg and related utilities

PACKAGE = rogg
VERSION = 0.2

prefix = /usr/local

OPTS = -g -O2 -Wall

rogg_UTILS = rogg_pagedump rogg_eosfix rogg_crcfix \
	rogg_aspect rogg_stats rogg_serial

all : librogg.a $(rogg_UTILS)

EXTRA_DIST = Makefile README

librogg.a : rogg.o
	$(AR) cr $@ $^
	ranlib $@

rogg_eosfix : rogg_eosfix.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

rogg_crcfix : rogg_crcfix.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

rogg_pagedump : rogg_pagedump.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

rogg_aspect : rogg_aspect.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

rogg_stats : rogg_stats.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

rogg_serial : rogg_serial.o librogg.a
	$(CC) $(CFLAGS) -o $@ $^

check : all

clean :
	-rm -f $(rogg_UTILS)
	-rm -f librogg.a
	-rm -f *.o

.PHONY : all check clean install uninstall dist

.c.o :
	$(CC) $(OPTS) $(CFLAGS) -I. -c $<

install : all
	cp librogg.a $(prefix)/lib/
	cp rogg.h $(prefix)/include/
	cp $(rogg_UTILS) $(prefix)/bin/

uninstall : 
	-rm -f $(prefix)/lib/librogg.a
	-rm -f $(prefix)/include/rogg.h
	-for util in $(rogg_UTILS); do rm -f $(prefix)/bin/$$util; done

distdir = $(PACKAGE)-$(VERSION)
dist : *.c *.h Makefile
	if test -d $(distdir); then rm -rf $(distdir); fi
	-rm -rf $(PACKAGE)-$(VERSION)
	mkdir $(distdir)
	cp *.c *.h $(distdir)/
	cp $(EXTRA_DIST) $(distdir)/
	tar czf $(distdir).tar.gz $(distdir)
	rm -rf $(distdir)
