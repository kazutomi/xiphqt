/**
 *   Not yet working.
 *
 *   Everything here should get oggetized. But for now I don't
 *   want to struggle with packets, so I simply write everything 
 *   binary.
 */

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "tarkin.h"




#if __BYTE_ORDER == __LITTLE_ENDIAN

#define LE32_TO_CPU(x)
#define CPU_TO_LE32(x)

#elif __BYTE_ORDER == __BIG_ENDIAN

#define LE32_TO_CPU(x) x = (((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) \
                          | ((x & 0x00ff0000) >> 8)  | ((x & 0xff000000) << 24))
#define CPU_TO_LE32(x) x = (((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) \
                          | ((x & 0x00ff0000) >> 8)  | ((x & 0xff000000) << 24))

#else
#error  unknown endianess !!
#endif


static
int read_tarkin_header (int fd, TarkinStream *s)
{
   char signature [6];

   if (read (fd, signature, 6) < 6)
      return TARKIN_IO_ERROR;

   if (!strncmp(signature, "tarkin", 6) == 0)
      return TARKIN_SIGNATURE_NOT_FOUND;

   if (read (fd, &s->n_layers, 4) < 4)
      return TARKIN_IO_ERROR;

   LE32_TO_CPU(s->n_layers);

   return 0;
}


static
int read_layer_descs (int fd, TarkinStream *s)
{
   int i;

   s->layer = (TarkinVideoLayer*) calloc (1, s->n_layers * sizeof(TarkinVideoLayer));

   for (i=0; i<s->n_layers; i++) {
      if (read (fd, &s->layer[i], sizeof(TarkinVideoLayer)) < sizeof(TarkinVideoLayer)) {
         tarkin_stream_destroy (s);
         return TARKIN_IO_ERROR;
      }
      LE32_TO_CPU(s->layer[i].width);
      LE32_TO_CPU(s->layer[i].height);
      LE32_TO_CPU(s->layer[i].frames_per_buf);
      LE32_TO_CPU(s->layer[i].bitrate);
      LE32_TO_CPU(s->layer[i].format);
   }

   return 0;
}


TarkinStream* tarkin_stream_new (int fd)
{
   TarkinStream *s = (TarkinStream*) calloc (1, sizeof(TarkinStream));

   if (!s)
      return NULL;

   s->fd = fd;

   return s;
}


void tarkin_stream_destroy (TarkinStream *s)
{
   int i;

   if (!s)
      return;

   for (i=0; i<s->n_layers; i++)
      if (s->layer[i].waveletbuf)
         wavelet_3d_buf_destroy (s->layer[i].waveletbuf);

   free(s->layer);
   free(s);
}


uint32_t tarkin_read_frame (TarkinStream *s, uint8_t *buf)
{
   if (s->n_layers == 0) {
      if (read_tarkin_header(s->fd, s) != 0) {
         tarkin_stream_destroy (s);
         return 0;
      }

      if (read_layer_descs(s->fd, s) != 0) {
         tarkin_stream_destroy (s);
         return 0;
      }
   }
return 0;
}


uint32_t tarkin_write_set_bitrate (TarkinStream *s, uint32_t bitrate);
uint32_t tarkin_write_frame (TarkinStream *s, uint8_t *buf);


