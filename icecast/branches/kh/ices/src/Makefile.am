## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = log 

bin_PROGRAMS = ices
AM_CFLAGS = @XIPH_CFLAGS@
AM_CPPFLAGS = @XIPH_CPPFLAGS@
CFLAGS= -g -O20

EXTRA_DIST = thread net timing
EXTRA_ices_SOURCES = im_oss.c im_sun.c im_alsa.c

if HAVE_OSS_AUDIO
oss = im_oss.c
endif

if HAVE_SUN_AUDIO
sun = im_sun.c
endif

if HAVE_ALSA_AUDIO
alsa = im_alsa.c
endif

if HAVE_JACK
jack = im_jack.c
endif

input_hdrs = im_pcm.h im_sun.h im_oss.h im_alsa.h im_jack.h im_playlist.h playlist_basic.h
output_hdrs = om_shout.h om_file.h
dist_noinst_HEADERS = cfgparse.h inputmodule.h signals.h runner.h reencode.h encode.h logging.h stream.h metadata.h audio.h resample.h $(input_hdrs) $(output_hdrs)

input_srcs = im_pcm.c im_playlist.c playlist_basic.c playlist_script.c $(oss) $(sun) $(alsa) $(jack)
output_srcs = om_shout.c om_file.c
ices_SOURCES = input.c cfgparse.c runner.c ices.c signals.c reencode.c encode.c stream.c metadata.c audio.c resample.c $(input_srcs) $(output_srcs)

ices_LDADD = log/libicelog.la @ALSA_LIBS@ @JACK_LIBS@ @XIPH_LIBS@

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

