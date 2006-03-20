bin_PROGRAMS = oggplay

oggplay_SOURCES = main.c \
                  gui.c gui.h \
                  decoder.c decoder.h \
                  audio.c audio.h \
                  stream.c stream.h \
                  ringbuffer.c ringbuffer.h

oggplay_CFLAGS = $(GTK_CFLAGS) $(OSSO_CFLAGS)

oggplay_LDADD = $(GTK_LIBS) $(OSSO_LIBS) \
                /usr/local/lib/libvorbisidec.la \
                -lSDL
