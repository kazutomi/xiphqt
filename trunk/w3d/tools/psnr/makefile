RM              =	rm
#CFLAGS          =  -O2 
CFLAGS          =  -g -Wall 
LDLIBS	= -lm

COMMON_SRCS = error.c pnmio.c image_ops.c 
SRCS =  psnr.c  psnr_seq.c  $(COMMON_SRCS)
COMMON_OBJS =  $(COMMON_SRCS:%.c=%.o)
OBJS =  $(SRCS:%.c=%.o) $(COMMON_OBJS)
PROGS = psnr psnr_seq

ALL_DEP = $(SRCS:%.c=%.d)

all: bin

bin: $(PROGS)

$(PROGS) : %: %.o $(COMMON_OBJS)
	$(LINK.o) $^ $(LDLIBS) -o $@

clean:
	$(RM) -f $(OBJS) 
	$(RM) -f $(PROGS)

veryclean: clean
	$(RM) -f core TAGS
	$(RM) -f $(ALL_DEP)

realclean: veryclean
	$(RM) -f *.out *.pgm *~

#automate dependancies
DEPENDS.c = $(CC) -MM $(CFLAGS) $(CPPFLAGS)

%.d: %.c
	$(DEPENDS.c) $< \
		| sed "s#\(.*\.o\)[ :]*#\1 $@ : #" > $@; \
		[ -s $@ ] || rm -f $@

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
include $(ALL_DEP)
endif
