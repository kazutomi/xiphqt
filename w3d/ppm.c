/**
 *   This code has some serious problems with DOS-style CR/LF linebreaks.
 *   Simon already contributed better code, but there has been no attempt to
 *   use them for now.
 *   If you want do do this, please send me a patch.
 *
 *     - Holger
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>



int read_ppm_info (char *fname, int *w, int *h)
{
   int i;
   FILE *file;

   file = fopen (fname, "r");

   if (!file) {
      fprintf(stderr, "Error opening '%s'\n", fname);
      return -1;
   }

   for (i=0; i<3; i++) {
      char ln [255];
      fgets(ln, 255, file);
      if(*ln == '#')
         i--;
      else {
         if (i == 0 && strncmp("P6", ln, 2)) {
            fprintf(stderr, "Error: Need PPM file for input\n");
            fclose (file);
            exit(-1);
         }
         if (i == 1) {
            ln[20] = 0;
            sscanf(ln, "%i %i", w, h);
         }
      }
   }

   fclose (file);
   return 0;
}


int read_ppm (char *fname, uint8_t *buf, int w, int h)
{
   int i;
   long _w, _h;
   FILE *file;

   file = fopen (fname, "r");

   if (!file)
      return -1;

   for (i=0; i<3; i++) {
      char ln [256];

      fgets(ln, 255, file);
      if(*ln == '#')
         i--;
      else {
         if (i == 0 && strncmp("P6", ln, 2)) {
            fprintf(stderr, "Error: Need PPM file for input\n");
            fclose (file);
            exit(-1);
         }
         if (i == 1) {
            ln[20] = 0;
            sscanf(ln, "%ld %ld", &_w, &_h);
         }
      }
   }

   if (w != _w || h != _h) {
      fprintf (stderr, "%s: image size inconsistent (w: %i <-> %ld, h: %i <-> %ld) !\n", __FUNCTION__, w, _w, h , _h);
      fclose (file);
      exit (-1);
   }

   fread (buf, 3, w*h, file);
   fclose (file);
   return 0;
}


void write_ppm (char *fname, uint8_t *rgb, int w, int h)
{
   FILE *outfile;

   outfile = fopen (fname, "w");

   if (!outfile) {
      printf ("error opening '%s' for writing !!!\n", fname);
      exit (-1);
   }

   fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
   fwrite (rgb, 3, w*h, outfile);
   fclose (outfile);
}


void write_ppm16 (char *fname, int16_t *rgb, int w, int h)
{
   int i;
   FILE *outfile;

   outfile = fopen (fname, "w");

   if (!outfile) {
      printf ("error opening '%s' for writing !!!\n", fname);
      exit (-1);
   }

   fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
   for (i=0; i<w*h; i++) {
      uint8_t c [3] = { rgb[i], rgb[i], rgb[i] };
      fwrite (c, 1, 3, outfile);
   }
   fclose (outfile);
}


