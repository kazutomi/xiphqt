#ifndef __TARKIN_H
#define __TARKIN_H

#include "wavelet.h"

typedef enum {
   TARKIN_GRAYSCALE,
   TARKIN_RGB24,       /*  tight packed RGB        */
   TARKIN_RGB32,       /*  32bit, no alphachannel  */
   TARKIN_RGBA,        /*  dito w/ alphachannel    */
   TARKIN_ARGB,
   TARKIN_BGRA
} TarkinColorFormat;

typedef enum {
   TARKIN_SIGNATURE_NOT_FOUND = -2,
   TARKIN_IO_ERROR = -1,
   TARKIN_OK = 0
} TarkinError;


typedef struct {
   uint32_t width;
   uint32_t height;
   uint32_t frames_per_buf;
   uint32_t bitrate;
   TarkinColorFormat format;
} TarkinVideoLayerDesc;


typedef struct {
   TarkinVideoLayerDesc desc;
   Wavelet3DBuf *waveletbuf;
   uint32_t frames_in_readbuf;
} TarkinVideoLayer;


typedef struct {
   int fd;
   uint32_t n_layers;
   TarkinVideoLayer *layer;
} TarkinStream;


TarkinStream* tarkin_stream_new (int fd);
void tarkin_stream_destroy (TarkinStream *s);

uint32_t tarkin_read_frame (TarkinStream *s, uint8_t *buf);

uint32_t tarkin_write_set_bitrate (TarkinStream *s, uint32_t bitrate);
uint32_t tarkin_write_frame (TarkinStream *s, uint8_t *buf);
uint32_t tarkin_write_frame (TarkinStream *s, uint8_t *buf);

#endif

