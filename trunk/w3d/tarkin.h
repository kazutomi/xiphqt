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
   TARKIN_SIGNATURE_NOT_FOUND = -2,
   TARKIN_IO_ERROR = -1,
   TARKIN_OK = 0
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


extern TarkinStream* tarkin_stream_new (int fd);
extern void tarkin_stream_destroy (TarkinStream *s);

extern int tarkin_stream_get_layer_desc (TarkinStream *s,
                                         uint32_t layer_id,
                                         TarkinVideoLayerDesc *desc);
extern uint32_t tarkin_stream_read_header (TarkinStream *s);
extern uint32_t tarkin_stream_read_frame (TarkinStream *s, uint8_t **buf);

extern int tarkin_stream_write_layer_descs (TarkinStream *s,
                                            uint32_t n_layers,
                                            TarkinVideoLayerDesc desc []);
extern uint32_t tarkin_stream_write_frame (TarkinStream *s, uint8_t **buf);
extern void tarkin_stream_flush (TarkinStream *s);

#endif

