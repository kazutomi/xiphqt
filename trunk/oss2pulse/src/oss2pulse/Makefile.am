bin_PROGRAMS = oss2pulse
oss2pulse_SOURCES = oss2pulse.c oss2pulse.h mixer.c dsp.c sndstat.c common.c
oss2pulse_CFLAGS = $(PULSE_CFLAGS)
oss2pulse_LDFLAGS = $(PULSE_LDFLAGS) $(PULSE_LIBADD)
oss2pulse_LIBS = -lfusd -lpulse $(PULSE_LIBADD)

install-exec-hook:
	chown root $(bindir)/oss2pulse
	chmod u+s $(bindir)/oss2pulse