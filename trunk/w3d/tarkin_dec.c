#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "tarkin.h"
#include "ppm.h"



int main (int argc, char **argv)
{
   char *fname = "stream.tarkin";
   char ofname [11];
   uint32_t frame = 0;
   uint8_t *rgb;
   int fd;
   TarkinStream *tarkin_stream;
   uint32_t n_layers;
   TarkinVideoLayerDesc layer;

   if (argc == 2) {
      fname = argv[1];
   } else if (argc != 1) {
      printf ("\n usage: %s <tarkin_stream>\n\n", argv[0]);
      exit (-1);
   }

   if ((fd = open (fname, O_RDONLY)) < 0) {
      printf ("error opening '%s'\n", fname);
      exit (-1);
   }

   tarkin_stream = tarkin_stream_new (fd);
   n_layers = tarkin_stream_read_header (tarkin_stream);

   if (n_layers == 0) {
      printf ("empty tarkin stream !!!\n");
      exit (-1);
   }

   tarkin_stream_get_layer_desc (tarkin_stream, 0, &layer);

   rgb  = malloc (layer.width * layer.height * 3);

   while (tarkin_stream_read_frame (tarkin_stream, &rgb) != 0xffffffff) {
      snprintf(ofname, 11, "out%03d.ppm", frame);
      printf ("write '%s'\n", ofname);
      write_ppm (ofname, rgb, layer.width, layer.height);

      frame ++;
   };

   free (rgb);
   tarkin_stream_destroy (tarkin_stream);
   close (fd);

   return 0;
}

