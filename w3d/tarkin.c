/*
 *   The real io-stuff is in tarkin-io.c
 *   (this one has to be rewritten to write ogg streams ...)
 */

#include <stdlib.h>
#include <string.h>

#include "tarkin.h"
#include "tarkin-io.h"
#include "yuv.h"


#define N_FRAMES 1



TarkinStream* tarkin_stream_new (int fd)
{
   TarkinStream *s = (TarkinStream*) calloc (1, sizeof(TarkinStream));

   if (!s)
      return NULL;

   s->fd = fd;
   s->frames_per_buf = N_FRAMES;

   return s;
}


void tarkin_stream_destroy (TarkinStream *s)
{
   uint32_t i, j;

   if (!s)
      return;

   for (i=0; i<s->n_layers; i++)
      if (s->layer[i].waveletbuf)
         for (j=0; j<s->layer[i].n_comp; j++)
            wavelet_3d_buf_destroy (s->layer[i].waveletbuf[j]);

   if (s->layer)
      free(s->layer);

   if (s->bitstream);
      free(s->bitstream);

   free(s);
}



int tarkin_stream_write_layer_descs (TarkinStream *s,
                                     uint32_t n_layers,
                                     TarkinVideoLayerDesc desc [])
{
   TarkinError err;
   uint32_t max_bitstream_len = 0;
   uint32_t i, j;

   s->n_layers = n_layers;
   s->layer = (TarkinVideoLayer*) calloc (n_layers, sizeof(TarkinVideoLayer));

   for (i=0; i<n_layers; i++) {
      TarkinVideoLayer *layer = &s->layer[i];

      memcpy (&layer->desc, &desc[i], sizeof(TarkinVideoLayerDesc));

      switch (layer->desc.format) {
      case TARKIN_GRAYSCALE:
         layer->n_comp = 1;
         layer->color_fwd_xform = grayscale_to_y;
         layer->color_inv_xform = y_to_grayscale;
         break;
      case TARKIN_RGB24:
         layer->n_comp = 3;
         layer->color_fwd_xform = rgb24_to_yuv;
         layer->color_inv_xform = yuv_to_rgb24;
         break;
      case TARKIN_RGB32:
         layer->n_comp = 3;
         layer->color_fwd_xform = rgb32_to_yuv;
         layer->color_inv_xform = yuv_to_rgb32;
         break;
      case TARKIN_RGBA:
         layer->n_comp = 4;
         layer->color_fwd_xform = rgba_to_yuv;
         layer->color_inv_xform = yuv_to_rgba;
         break;
      default:
         BUG("unknown color format");
         break;
      };

      layer->waveletbuf = (Wavelet3DBuf**) calloc (layer->n_comp,
                                                   sizeof(Wavelet3DBuf*));
      for (j=0; j<layer->n_comp; j++)
         layer->waveletbuf[j] = wavelet_3d_buf_new (desc[i].width,
                                                    desc[i].height,
                                                    desc[i].frames_per_buf);

      layer->bitstream_len = layer->desc.bitrate / (8 * layer->n_comp);
      max_bitstream_len += layer->bitstream_len * layer->n_comp
          + 5000
          + 2 * 9 * sizeof(uint32_t) * layer->n_comp;    // truncation tables 
   }

   if ((err = write_tarkin_header(s->fd, s)) != TARKIN_OK)
      return err;

   err = write_layer_descs(s->fd, s);

   s->bitstream = (uint8_t*) malloc (max_bitstream_len);

   return err;
}


void tarkin_stream_flush (TarkinStream *s)
{
   uint32_t i, j;

   s->current_frame_in_buf=0;

   for (i=0; i<s->n_layers; i++) {
      TarkinVideoLayer *layer = &s->layer[i];

      for (j=0; j<layer->n_comp; j++) {
         uint32_t bitstream_len;

         wavelet_3d_buf_fwd_xform (layer->waveletbuf[j], 2, 2);
         wavelet_3d_buf_dump ("coeff-%d-%03d.ppm",
                              s->current_frame, j, layer->waveletbuf[j]);

         bitstream_len = wavelet_3d_buf_encode_coeff (layer->waveletbuf[j],
                                                      s->bitstream,
                                                      layer->bitstream_len);
         write_tarkin_bitstream (s->fd, s->bitstream, bitstream_len);
      }
   }
}


uint32_t tarkin_stream_write_frame (TarkinStream *s, uint8_t **rgba)
{
   uint32_t i;

   for (i=0; i<s->n_layers; i++) {
      TarkinVideoLayer *layer = &s->layer[i];

      layer->color_fwd_xform (rgba[i], layer->waveletbuf,
                              s->current_frame_in_buf);
   }

   s->current_frame_in_buf++;

   if (s->current_frame_in_buf == s->frames_per_buf)
      tarkin_stream_flush (s);

   return (++s->current_frame);
}




/**
 *   return value: number of layers, 0 on i/o error
 */
uint32_t tarkin_stream_read_header (TarkinStream *s)
{
   uint32_t max_bitstream_len = 0;
   uint32_t i, j;

   if (read_tarkin_header(s->fd, s) != TARKIN_OK)
      return 0;

   if (read_layer_descs(s->fd, s) != TARKIN_OK)
      return 0;

   for (i=0; i<s->n_layers; i++) {
      TarkinVideoLayer *layer = &s->layer[i];

      if (layer->desc.format != TARKIN_RGB24)
         exit (-1);

      switch (layer->desc.format) {
      case TARKIN_GRAYSCALE:
         layer->n_comp = 1;
         layer->color_fwd_xform = grayscale_to_y;
         layer->color_inv_xform = y_to_grayscale;
         break;
      case TARKIN_RGB24:
         layer->n_comp = 3;
         layer->color_fwd_xform = rgb24_to_yuv;
         layer->color_inv_xform = yuv_to_rgb24;
         break;
      case TARKIN_RGB32:
         layer->n_comp = 3;
         layer->color_fwd_xform = rgb32_to_yuv;
         layer->color_inv_xform = yuv_to_rgb32;
         break;
      case TARKIN_RGBA:
         layer->n_comp = 4;
         layer->color_fwd_xform = rgba_to_yuv;
         layer->color_inv_xform = yuv_to_rgba;
         break;
      default:
         BUG("unknown color format");
      };

      layer->waveletbuf = (Wavelet3DBuf**) calloc (layer->n_comp,
                                                   sizeof(Wavelet3DBuf*));
      for (j=0; j<layer->n_comp; j++) {
         layer->waveletbuf[j] = wavelet_3d_buf_new (layer->desc.width,
                                                    layer->desc.height,
                                                    layer->desc.frames_per_buf);
         if (!layer->waveletbuf[j])
            return 0;
      }

      layer->bitstream_len = layer->desc.bitrate / (8 * layer->n_comp);
      max_bitstream_len += layer->bitstream_len * layer->n_comp
          + 5000
          + 2 * 9 * sizeof(uint32_t) * layer->n_comp;
   }

   s->bitstream = (uint8_t*) malloc (max_bitstream_len);

   return s->n_layers;
}



int tarkin_stream_get_layer_desc (TarkinStream *s,
                                  uint32_t layer_id,
                                  TarkinVideoLayerDesc *desc)
{
   if (layer_id > s->n_layers-1)
      return -1;

   memcpy (desc, &(s->layer[layer_id].desc), sizeof(TarkinVideoLayerDesc));

   return 0;
}


/**
 *   read all layers of the next frame to buf[0..n_layers]
 *   returns the number of this frame on success, -1 on error
 */
uint32_t tarkin_stream_read_frame (TarkinStream *s, uint8_t **rgba)
{
   uint32_t i, j;

   if (s->current_frame_in_buf == 0) {
      for (i=0; i<s->n_layers; i++) {
         TarkinVideoLayer *layer = &s->layer[i];

         for (j=0; j<layer->n_comp; j++) {
            uint32_t bitstream_len;

            bitstream_len = read_tarkin_bitstream (s->fd, s->bitstream);

            if (bitstream_len == 0)
               return 0xffffffff;

            wavelet_3d_buf_decode_coeff (layer->waveletbuf[j], s->bitstream,
                                         bitstream_len);
            wavelet_3d_buf_dump ("rcoeff-%d-%03d.ppm",
                                 s->current_frame, j, layer->waveletbuf[j]);
            wavelet_3d_buf_inv_xform (layer->waveletbuf[j], 2, 2);
         }
      }
   }

   for (i=0; i<s->n_layers; i++) {
      TarkinVideoLayer *layer = &s->layer[i];

      layer->color_inv_xform (layer->waveletbuf, rgba[i],
                              s->current_frame_in_buf);
   }

   s->current_frame_in_buf++;

   if (s->current_frame_in_buf == s->frames_per_buf)
      s->current_frame_in_buf=0;

   return (++s->current_frame);
}


