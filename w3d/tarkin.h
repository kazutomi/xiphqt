#ifndef __TARKIN_H
#define __TARKIN_H


typedef struct {
   int fd;
   ogg_stream_state os;
   ogg_page op;
   WaveletBuf3D *waveletbuf [2];
   uint32 bitrate;
} TarkinStream;


TarkinStream* tarkin_read_open (int fd);
TarkinStream* tarkin_read_open_by_url (char *fname);
int tarkin_read_frame (TarkinStream *s, uint8 *buf);

TarkinStream* tarkin_write_open (int fd);
TarkinStream* tarkin_write_open_by_url (char *fname);
int tarkin_write_set_bitrate (TarkinStream *s, uint32 bitrate);
int tarkin_write_frame (TarkinStream *s, uint8 *buf);

void tarkin_close (TarkinStream *s);

#endif
