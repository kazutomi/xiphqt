
/**
 *   Ugly Hack.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mem.h"
#include "pnm.h"
#include "bitcoder.h"


static
void usage (const char *program_name)
{
   printf ("\n"
     " usage: %s <file1.ppm> <file2.ppm>\n"
     "\n", program_name);
   exit (-1);
}


#define ABS(x) ((x) < 0 ? (-x) : (x))


int main (int argc, char **argv)
{
   int n_chan1, n_chan;
   uint8_t *rgb1, *rgb2, *diff;
   uint32_t width, height, width1, height1;
   uint32_t i;

   if (argc != 3)
      usage (argv[0]);

   if ((n_chan1 = read_pnm_header (argv[1], &width1, &height1)) < 0) {
      printf ("error opening '%s' !!\n", argv[1]);
      exit (-1);
   }

   if ((n_chan = read_pnm_header (argv[2], &width, &height)) < 0) {
      printf ("error opening '%s' !!\n", argv[2]);
      exit (-1);
   }

   if (!(width1 == width && height1 == height && n_chan1 == n_chan)) {
      printf ("image sizes differ !!\n");
      exit (-1);
   }

   rgb1  = MALLOC (width * height * n_chan);
   rgb2  = MALLOC (width * height * n_chan);
   diff  = MALLOC (width * height * n_chan);

   if (read_pnm (argv[1], rgb1) < 0)
   {
      printf ("error opening '%s' !\n", argv[1]);
      exit(-1);
   }

   if (read_pnm (argv[2], rgb2) < 0)
   {
      printf ("error opening '%s' !\n", argv[2]);
      exit(-1);
   }

   for (i=0; i<width*height*n_chan; i++) {
      diff[i] = (rgb1[i] == rgb2[i]) ? 0 : 255;
//      diff[i] = ((int) rgb1[i] - rgb2[i])*64 + 128;
      if (diff[i] != 0/*128*/) {
         printf("%i/%i: %u <-> %u  -- diff == %i\n",
                i, width*height*n_chan-1, rgb1[i], rgb2[i], diff[i] - 128);
         bit_print(rgb1[i]);
         bit_print(rgb2[i]);
      }
   }

   write_pnm (n_chan == 1 ? "diff.pgm" : "diff.ppm", diff, width, height);

   FREE (rgb1);
   FREE (rgb2);
   FREE (diff);

   return 0;
}


