/**
 *   Not yet working.
 *
 *   Everything here should get oggetized. But for now I don't
 *   want to struggle with packets, so I simply write everything 
 *   binary.
 */

#include <unistd.h>
#include <ctype.h>
#include "mem.h"
#include "tarkin-io.h"



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



int write_tarkin_header (int fd, TarkinStream *s)
{
   char signature [6] = "tarkin";
   uint32_t n_layers = s->n_layers;

   if (write (fd, signature, 6) < 6)
      return TARKIN_IO_ERROR;

   CPU_TO_LE32(n_layers);

   if (write (fd, &n_layers, 4) < 4)
      return TARKIN_IO_ERROR;

   return TARKIN_OK;
}


int write_layer_descs (int fd, TarkinStream *s)
{
   int i;
   TarkinVideoLayer layer;

   for (i=0; i<s->n_layers; i++) {
      memcpy(&layer, &s->layer[i], sizeof(TarkinVideoLayer));

      CPU_TO_LE32(layer.width);
      CPU_TO_LE32(layer.height);
      CPU_TO_LE32(layer.frames_per_buf);
      CPU_TO_LE32(layer.bitrate);
      CPU_TO_LE32(layer.format);

      if (write (fd, &layer, sizeof(TarkinVideoLayer)) < sizeof(TarkinVideoLayer)) {
         return TARKIN_IO_ERROR;
      }
   }

   return TARKIN_OK;
}


int write_tarkin_bitstream (int fd, uint8_t *bitstream, uint32_t len)
{
   uint32_t bytes = len;

   CPU_TO_LE32(bytes);

   return (write (fd, &bytes, 4) + write (fd, bitstream, bytes));
}




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

   return TARKIN_OK;
}


int read_layer_descs (int fd, TarkinStream *s)
{
   int i;

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

   return TARKIN_OK;
}


int read_tarkin_bitstream (int fd, uint8_t *bitstream)
{
   uint32_t bytes = 0;
   uint32_t len;

   if (read (fd, &len, 4) < 4 || len == 0)
      return 0;

   LE32_TO_CPU(len);

   while (bytes < len)
      bytes += read (fd, bitstream + bytes, len - bytes);

   return len;
}



