
#include <stdint.h>
#include <stdio.h>
#include "../pnm.h"
#include "../mem.h"


static
void usage (char *progname)
{
   printf ("\n"
           "usage: %s <in.ppm> <out.ppm>\n"
           "\n"
           "        where <in.ppm> is the input filename template\n"
           "        and <out.ppm> is the output filename template"
           " (e.g. image%%03d.ppm)\n"
           "\n", progname);
   exit (-1);
}


/**
 *  w in bytes ( == width * channels)
 */
static
void write_even_scanlines (uint8_t *in, uint8_t *out, int w, int h)
{
   uint8_t *prev, *next, *this;
   int i, j;

   for (i=0; i<h; i+=2)
      memcpy (out + i * w * sizeof(uint8_t), in + i * w * sizeof(uint8_t),
              w * sizeof(uint8_t));

   for (j=1; j<h-1; j+=2) {
      this = out + j*w;
      next = this + w;
      prev = this - w;

      for (i=0; i<w; i++)
         this[i] = (prev[i] + next[i]) / 2;
   }

   this = out + (h-1) * w;
   prev = this - w;
   for (i=0; i<w; i++)
      this[i] = prev[i];
}


static
void write_odd_scanlines (uint8_t *in, uint8_t *out, int w, int h)
{
   uint8_t *prev, *next, *this;
   int i, j;

   for (i=1; i<h; i+=2)
      memcpy (out + i * w * sizeof(uint8_t), in + i * w * sizeof(uint8_t),
              w * sizeof(uint8_t));

   this = out;
   next = this + w;
   for (i=0; i<w; i++)
      this[i] = next[i];

   for (j=2; j<h; j+=2) {
      this = out + j*w;
      next = this + w;
      prev = this - w;

      for (i=0; i<w; i++)
         this[i] = (prev[i] + next[i]) / 2;
   }
}


int main (int argc, char **argv)
{
   int odd_lines_first = 1;
   int channels;
   int w, h;
   uint8_t *in, *out;
   int frame = 0;
   char fname[256];

   if (argc != 3)
      usage (argv[0]);

   snprintf (fname, 256, argv[1], 0);
   channels = read_pnm_header (fname, &w, &h);

   if (h == 576)
      odd_lines_first = 0;
   else if (h == 486) {
   } else {
      printf ("\n"
              "Could not determine whether odd scanlines come temporally "
              "earlier or later.\n"
              "Input files should have height 576 (625 scanlines) " "or 486 (525 scanlines) !\n"
              "\n"
              "Assume odd scanlines first.\n"
              "\n");
   }

   in = (uint8_t*) MALLOC (channels * w * h * sizeof(uint8_t));
   out = (uint8_t*) MALLOC (channels * w * h * sizeof(uint8_t));

   do {
      snprintf (fname, 256, argv[1], frame);
      printf ("read '");
      printf (fname, frame);
      printf ("'");
      if (read_pnm (fname, in) < 0)
      {
         printf (" failed.\n");
         break;
      }
      printf ("\n");

      if (odd_lines_first) {
         write_odd_scanlines (in, out, w * channels, h);
         snprintf (fname, 256, argv[2], 2*frame);
         write_pnm (fname, out, w, h);
         write_even_scanlines (in, out, w * channels, h);
         snprintf (fname, 256, argv[2], 2*frame + 1);
         write_pnm (fname, out, w, h);
      } else {
         write_even_scanlines (in, out, w * channels, h);
         snprintf (fname, 256, argv[2], 2*frame);
         write_pnm (fname, out, w, h);
         write_odd_scanlines (in, out, w * channels, h);
         snprintf (fname, 256, argv[2], 2*frame + 1);
         write_pnm (fname, out, w, h);
      }
      frame++;
   } while (1);

   FREE(in);
   FREE(out);

   return 0;
}

