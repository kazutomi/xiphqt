#ifndef __TARKIN_H
#define __TARKIN_H

#include <stdio.h>
#include "wavelet.h"


#define BUG(x...)                                                            \
   do {                                                                      \
      printf("BUG in %s (%s: line %i): ", __FUNCTION__, __FILE__, __LINE__); \
      printf(#x);                                                            \
      printf("\n");                                                          \
      exit (-1);                                                             \
   } while (0);



typedef enum {
   TARKIN_GRAYSCALE,
   TARKIN_RGB24,       /*  tight packed RGB        */
   TARKIN_RGB32,       /*  32bit, no alphachannel  */
   TARKIN_RGBA,        /*  dito w/ alphachannel    */
} TarkinColorFormat;

typedef enum {
   TARKIN_OK = 0,
   TARKIN_IO_ERROR,
   TARKIN_SIGNATURE_NOT_FOUND,
   TARKIN_INVALID_LAYER,
   TARKIN_INVALID_COLOR_FORMAT
} TarkinError;


typedef struct {
   uint32_t width;
   uint32_t height;
   uint32_t a_moments;
   uint32_t s_moments;
   uint32_t frames_per_buf;
   uint32_t bitstream_len;              /*  for all color components, bytes */
   TarkinColorFormat format;
} TarkinVideoLayerDesc;


typedef struct {
   TarkinVideoLayerDesc desc;
   uint32_t n_comp;                     /*  number of color components */
   Wavelet3DBuf **waveletbuf;
   uint32_t current_frame_in_buf;

   void (*color_fwd_xform) (uint8_t *rgba, Wavelet3DBuf *yuva [], uint32_t count);
   void (*color_inv_xform) (Wavelet3DBuf *yuva [], uint8_t *rgba, uint32_t count);
} TarkinVideoLayer;


typedef struct {
   int fd;
   uint32_t n_layers;
   TarkinVideoLayer *layer;
   uint32_t current_frame;
   uint32_t current_frame_in_buf;
   uint32_t frames_per_buf;
   uint8_t *bitstream;
} TarkinStream;


extern
TarkinStream* tarkin_stream_new (int fd);

extern
void tarkin_stream_destroy (TarkinStream *s);


/**
 *   Copy layer description of layer into desc
 */
extern
TarkinError tarkin_stream_get_layer_desc (TarkinStream *s,
                                          uint32_t layer_id,
                                          TarkinVideoLayerDesc *desc);

/**
 *   Return value: number of layers, 0 on i/o error
 */
extern
uint32_t tarkin_stream_read_header (TarkinStream *s);


/**
 *   Read all layers of the next frame to buf[0..n_layers]
 *   returns the number of this frame on success, -1 on error
 */
extern
uint32_t tarkin_stream_read_frame (TarkinStream *s, uint8_t **buf);


/**
 *   Setup file configuration by writing layer descriptions
 *   Has to be done once after creating the stream
 */
extern
TarkinError tarkin_stream_write_layer_descs (TarkinStream *s,
                                             uint32_t n_layers,
                                             TarkinVideoLayerDesc desc []);

/**
 *   Write a single frame. This means that content is copied into
 *   a wavelet buffer. As soon this gets filled, the stream will be
 *   flushed implicitly.
 *   Each pointer in buf points to an layer imaged described by the
 *   tarkin_stream_write_layer_descs() call
 */
extern
uint32_t tarkin_stream_write_frame (TarkinStream *s, uint8_t **buf);


extern
void tarkin_stream_flush (TarkinStream *s);


#endif

