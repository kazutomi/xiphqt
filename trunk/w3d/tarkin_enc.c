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
   char *fmt = "%i.ppm";
   char fname[256];
   uint32_t frame = 0;
   uint8_t *rgb;
   int ylimit, ulimit, vlimit;
   int a_moments, s_moments;
   int fd;
   TarkinStream *tarkin_stream;
   TarkinVideoLayerDesc layer [] = { { 0, 0, 1, 5000, TARKIN_RGB24 } };

   if (argc == 1) {
      ylimit = 1000;
      ulimit = 150;
      vlimit = 150;
      a_moments = 2;
      s_moments = 2;
   } else if (argc == 7) {
      fmt = argv[1];
      ylimit = strtol (argv[2], 0, 0);
      ulimit = strtol (argv[3], 0, 0);
      vlimit = strtol (argv[4], 0, 0);
      a_moments = strtol (argv[5], 0, 0);
      s_moments = strtol (argv[6], 0, 0);
   } else {
      printf ("\n"
        " usage: %s <input filename format string> <ylimit> <ulimit> <vlimit> <a_m> <s_m>\n"
        "\n"
        "   input ppm filename format:  optional, \"%%i.ppm\" by default\n"
        "   ylimit, ulimit, vlimit:     cut Y/U/V bitstream after limit bytes/frame\n"
        "   a_m, s_m:                   number of vanishing moments of the\n"
        "                               analysis/synthesis filter, (2,2) by default\n"
        "\n", argv[0]);
      exit (-1);
   }

   snprintf (fname, 256, fmt, 0);
   if (read_ppm_info (fname, &layer[0].width, &layer[0].height) < 0)
      exit (-1);

   rgb  = malloc (layer[0].width * layer[0].height * 3);

   if ((fd = open ("stream.tarkin", O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
      printf ("error opening '%s' for writing !\n", "stream.tarkin");
      exit (-1);
   }

   tarkin_stream = tarkin_stream_new (fd);
   tarkin_stream_write_layer_descs (tarkin_stream, 1, layer);

   do {
      snprintf (fname, 256, fmt, frame);
      printf ("read '");
      printf (fname, frame);
      printf ("'");

      if (read_ppm (fname, rgb, layer[0].width, layer[0].height) < 0)
      {
         printf (" failed.\n");
         break;
      }
      printf ("\n");

      tarkin_stream_write_frame (tarkin_stream, &rgb);
      frame++;
   } while (0);

   free (rgb);
   tarkin_stream_destroy (tarkin_stream);
   close (fd);

   return 0;
}

