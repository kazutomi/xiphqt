
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
#include "ppm.h"
#include "bitcoder.h"


static
void usage (const char *program_name)
{
   printf ("\n"
     " usage: %s <file1.ppm> <file2.ppm>\n"
     "\n", program_name);
   exit (-1);
}



int main (int argc, char **argv)
{
   uint8_t *rgb1, *rgb2, *diff;
   uint32_t width, height, width1, height1;
   uint32_t i;

   if (argc != 3)
      usage (argv[0]);

   if (read_ppm_info (argv[1], &width1, &height1) < 0)
      exit (-1);

   if (read_ppm_info (argv[2], &width, &height) < 0)
      exit (-1);

   if (!(width1 == width && height1 == height)) {
      printf ("image sizes differ !!\n");
      exit (-1);
   }

   rgb1  = malloc (width * height * 3);
   rgb2  = malloc (width * height * 3);
   diff  = malloc (width * height * 3);

   if (read_ppm (argv[1], rgb1, width, height) < 0)
   {
      printf ("error opening '%s' !\n", argv[1]);
      exit(-1);
   }

   if (read_ppm (argv[2], rgb2, width, height) < 0)
   {
      printf ("error opening '%s' !\n", argv[2]);
      exit(-1);
   }

   for (i=0; i<width*height*3; i++) {
      diff[i] = (rgb1[i] == rgb2[i]) ? 0 : 255;
      if (diff[i]) {
         printf("%i: %i <-> %i\n", i, rgb1[i], rgb2[i]);
         bit_print(rgb1[i]);
         bit_print(rgb2[i]);
      }
   }

   write_ppm ("diff.ppm", diff, width, height);

   free (rgb1);
   free (rgb2);
   free (diff);

   return 0;
}

