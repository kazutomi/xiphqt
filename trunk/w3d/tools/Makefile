CC = gcc
RM = rm -rf

CFLAGS = -g -O0 -Wall -I.. -DTYPE=uint16_t
LFLAGS = -g

TARGETS = ppmdiff

OBJS = $(TARGETS:=.o) ../ppm.o
SRCS = $(OBJS:.o=.c)


all: ppmdiff

ppmdiff: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

.c.o: .depend
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGETS) gmon.out core .depend .depend.bak


.depend: $(SRCS)
	$(RM) .depend
	touch .depend
	sh ../tools/makedepend.sh -f.depend -- $(CFLAGS) -- $(SRCS)

-include .depend
