#ifndef __TARKIN_H
#define __TARKIN_H

typedef enum { TARKIN_RGB, TARKIN_RGBA, TARKIN_GRAYSCALE, TARKIN_ALPHA } VideoChannelType;


typedef struct {
   char *name;
   VideoChannelType type;
   uint32 n_component;
   WaveletBuf3D **waveletbuf [2];
   uint32 *frames_in_readbuf;
   uint32 bitrate;
} TarkinVideoChannel;


typedef struct {
   int fd;
   ogg_stream_state os;
   uint32 n_videochannels;
   TarkinVideoChannel *channel;
} TarkinStream;


TarkinStream* tarkin_stream_new (int fd);
void tarkin_stream_destroy (TarkinStream *s);

int tarkin_read_frame (TarkinStream *s, uint8 *buf);

int tarkin_write_set_bitrate (TarkinStream *s, uint32 bitrate);
int tarkin_write_frame (TarkinStream *s, uint8 *buf);

#endif
