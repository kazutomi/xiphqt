INCLUDES = -I $(top_srcdir)/include

AM_CFLAGS = $(SDL_CFLAGS) $(OGG_CFLAGS)

LDADD = ../lib/libtheoradec.la ../lib/libtheoraenc.la ../lib/libtheorabase.la -lm $(OGG_LIBS)

bin_PROGRAMS = dump_video encoder_example $(BUILDABLE_EXAMPLES)

EXTRA_PROGRAMS = encoder_example player_example

dump_video_SOURCES = dump_video.c
dump_video_LDFLAGS = $(OGG_LIBS)

encoder_example_SOURCES = encoder_example.c
encoder_example_LDADD = $(LDADD) $(VORBIS_LIBS) $(VORBISENC_LIBS)

player_example_SOURCES = player_example.c
player_example_LDADD = $(LDADD) $(SDL_LIBS) $(VORBIS_LIBS)
