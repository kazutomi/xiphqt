/*
 *   The real io-stuff is in tarkin-io.c
 *   (this one has to be rewritten to write ogg streams ...)
 */

#include "mem.h"
#include "tarkin.h"
#include "tarkin-io.h"
#include "yuv.h"


#define N_FRAMES 1



TarkinStream* tarkin_stream_new (int fd)
{
   TarkinStream *s = (TarkinStream*) CALLOC (1, sizeof(TarkinStream));

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

   for (i=0; i<s->n_layers; i++) {
      if (s->layer[i].waveletbuf) {
         for (j=0; j<s->layer[i].n_comp; j++) {
            wavelet_3d_buf_destroy (s->layer[i].waveletbuf[j]);
         }
         FREE(s->layer[i].waveletbuf);
      }
   }

   if (s->layer)
      FREE(s->layer);

   if (s->bitstream);
      FREE(s->bitstream);

   FREE(s);
}



TarkinError tarkin_stream_write_layer_descs (TarkinStream *s,
                                             uint32_t n_layers,
                                             TarkinVideoLayerDesc desc [])
{
   TarkinError err;
   uint32_t max_bitstream_len = 0;
   uint32_t i, j;

   s->n_layers = n_layers;
   s->layer = (TarkinVideoLayer*) CALLOC (n_layers, sizeof(TarkinVideoLayer));

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
         return -TARKIN_INVALID_COLOR_FORMAT;
      };

      layer->waveletbuf = (Wavelet3DBuf**) CALLOC (layer->n_comp,
                                                   sizeof(Wavelet3DBuf*));
      for (j=0; j<layer->n_comp; j++)
         layer->waveletbuf[j] = wavelet_3d_buf_new (desc[i].width,
                                                    desc[i].height,
                                                    desc[i].frames_per_buf);

      max_bitstream_len += layer->desc.bitstream_len
          + 2 * 10 * sizeof(uint32_t) * layer->n_comp;    // truncation tables 
   }

   if ((err = write_tarkin_header(s->fd, s)) != TARKIN_OK)
      return err;

   if ((err = write_layer_descs(s->fd, s)) != TARKIN_OK)
      return err;

   s->bitstream = (uint8_t*) MALLOC (max_bitstream_len);

   return TARKIN_OK;
}


void tarkin_stream_flush (TarkinStream *s)
{
   uint32_t i, j;

   s->current_frame_in_buf=0;

   for (i=0; i<s->n_layers; i++) {
      TarkinVideoLayer *layer = &s->layer[i];

      for (j=0; j<layer->n_comp; j++) {
         uint32_t comp_bitstream_len;
         uint32_t bitstream_len;

        /**
         *  implicit 6:1:1 subsampling
         */
         if (j == 0)
            comp_bitstream_len = 6*layer->desc.bitstream_len/(layer->n_comp+5);
         else
            comp_bitstream_len = layer->desc.bitstream_len/(layer->n_comp+5);

         wavelet_3d_buf_dump ("color-%d-%03d.pgm",
                              s->current_frame, j,
                              layer->waveletbuf[j], j == 0 ? 0 : 128);

         wavelet_3d_buf_fwd_xform (layer->waveletbuf[j],
                                   layer->desc.a_moments,
                                   layer->desc.s_moments);

         wavelet_3d_buf_dump ("coeff-%d-%03d.pgm",
                              s->current_frame, j,
                              layer->waveletbuf[j], 128);

         bitstream_len = wavelet_3d_buf_encode_coeff (layer->waveletbuf[j],
                                                      s->bitstream,
                                                      comp_bitstream_len);

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

   s->layer = (TarkinVideoLayer*) CALLOC (1, s->n_layers * sizeof(TarkinVideoLayer));

   if (read_layer_descs(s->fd, s) != TARKIN_OK)
      return 0;

   for (i=0; i<s->n_layers; i++) {
      TarkinVideoLayer *layer = &s->layer[i];

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

      layer->waveletbuf = (Wavelet3DBuf**) CALLOC (layer->n_comp,
                                                   sizeof(Wavelet3DBuf*));
      for (j=0; j<layer->n_comp; j++) {
         layer->waveletbuf[j] = wavelet_3d_buf_new (layer->desc.width,
                                                    layer->desc.height,
                                                    layer->desc.frames_per_buf);
         if (!layer->waveletbuf[j])
            return 0;
      }

      max_bitstream_len += layer->desc.bitstream_len
          + 2 * 10 * sizeof(uint32_t) * layer->n_comp;
   }

   s->bitstream = (uint8_t*) MALLOC (max_bitstream_len);

   return s->n_layers;
}



TarkinError tarkin_stream_get_layer_desc (TarkinStream *s,
                                          uint32_t layer_id,
                                          TarkinVideoLayerDesc *desc)
{
   if (layer_id > s->n_layers-1)
      return -TARKIN_INVALID_LAYER;

   memcpy (desc, &(s->layer[layer_id].desc), sizeof(TarkinVideoLayerDesc));

   return TARKIN_OK;
}


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

            wavelet_3d_buf_dump ("rcoeff-%d-%03d.pgm",
                                 s->current_frame, j, layer->waveletbuf[j],
                                 128);

            wavelet_3d_buf_inv_xform (layer->waveletbuf[j], 
                                      layer->desc.a_moments,
                                      layer->desc.s_moments);

            wavelet_3d_buf_dump ("rcolor-%d-%03d.pgm",
                                 s->current_frame, j,
                                 layer->waveletbuf[j], j == 0 ? 0 : 128);
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


