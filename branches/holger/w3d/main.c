
#include <stdio.h>
#include <string.h>
#include "wavelet.h"
#include "coder.h"
#include "yuv.h"


#define  N_FRAMES  8


int read_ppm_info (char *prefix, int *w, int *h)
{
   int i;
   char fname [256];
   FILE *file;

   sprintf (fname , "%s0.ppm", prefix);
   file = fopen (fname, "r");

   if (!file) {
      fprintf(stderr, "Error: opening first frame '%s'\n", fname);
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


int read_ppm (char *prefix, int frame, uint8 *buf, int w, int h)
{
   int i;
   long _w, _h;
   char fname [256];
   FILE *file;

   sprintf (fname , "%s%i.ppm", prefix, frame);
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


void save_ppm (char *prefix, uint8 *buf, int w, int h, int first_frame, int frames)
{
   int i;

   for (i=0; i<frames; i++)
   {
      char fname [256];
      FILE *outfile;
      uint8 *img = buf + w * h * 3 * i;

      sprintf (fname , "%s%i.ppm", prefix, i + first_frame);
      outfile = fopen (fname, "w");
      fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
      fwrite (img, 3, w*h, outfile);
      fclose (outfile);
   }
}


void save_ppm16 (char *prefix, int16 *buf, int w, int h, int first_frame, int frames)
{
   int i, j;

   for (i=0; i<frames; i++)
   {
      char fname [256];
      FILE *outfile;
      int16 *img = buf + w * h * i;

      sprintf (fname , "%s%i.ppm", prefix, i + first_frame);
      outfile = fopen (fname, "w");
      fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
      for (j=0; j<w*h; j++) {
         uint8 c [3] = { img[j], img [j], img[j] };
         fwrite (c, 1, 3, outfile);
      }
      fclose (outfile);
   }
}




int main (int argc, char **argv)
{
   char *ppm_prefix = "";
   uint8 *rgb, *rgb2;
   char *bitstream [3];
   int i, ycount, ucount, vcount;
   int ylimit, ulimit, vlimit;
   int width = -1, height = -1, frames = 0, frame = 0;

   if (argc == 5)
      ppm_prefix = argv[4];
   else if (argc != 4) {
      printf ("\n"
        " usage: %s <ylimit> <ulimit> <vlimit> <ppm prefix>\n"
        "\n"
        "   ylimit, ulimit, vlimit: cut Y/U/V bitstream after limit bytes\n"
        "   input ppm prefix:       optional, empty by default\n"
        "\n", argv[0]);
      exit (-1);
   }

   ylimit = strtol (argv[1], 0, 0);
   ulimit = strtol (argv[2], 0, 0);
   vlimit = strtol (argv[3], 0, 0);

   if (read_ppm_info (ppm_prefix, &width, &height) < 0)
      exit (-1);

   rgb  = malloc (width * height * 3 * N_FRAMES);
   rgb2 = malloc (width * height * 3 * N_FRAMES);
   bitstream[0] = malloc (width * height * N_FRAMES);
   bitstream[1] = malloc (width * height * N_FRAMES);
   bitstream[2] = malloc (width * height * N_FRAMES);

   if (!rgb || !rgb2 || !bitstream[0] || !bitstream[1] || !bitstream[2]) {
      printf ("memory allocation failed.\n");
      return (-1);
   }


   do {
      Wavelet3DBuf *y, *u, *v, *y2, *u2, *v2;

      for (frames=0; frames<N_FRAMES; frames++) {
         printf ("read frame %i", frame + frames);
         if (read_ppm (ppm_prefix, frame + frames,
                       rgb + width * height * 3 * frames, width, height) < 0)
         {
            printf (" failed.\n");
            break;
         }
         printf ("\n");
      }

      y = wavelet_3d_buf_new (width, height, frames);
      u = wavelet_3d_buf_new (width, height, frames);
      v = wavelet_3d_buf_new (width, height, frames);
      y2 = wavelet_3d_buf_new (width, height, frames);
      u2 = wavelet_3d_buf_new (width, height, frames);
      v2 = wavelet_3d_buf_new (width, height, frames);

      if (!y || !u || !v || !y2 || !u2 || !v2) {
         printf ("memory allocation failed.\n");
         return (-1);
      }

      save_ppm ("orig", rgb, width, height, frame, frames);

      rgb2yuv (rgb, y->data, u->data, v->data, width * height * frames, 3);

      save_ppm16 ("y", y->data, width, height, frame, frames);
      save_ppm16 ("u", u->data, width, height, frame, frames);
      save_ppm16 ("v", v->data, width, height, frame, frames);

      wavelet_3d_buf_fwd_xform (y);
      wavelet_3d_buf_fwd_xform (u);
      wavelet_3d_buf_fwd_xform (v);

      save_ppm16 ("y.coeff", y->data, width, height, frame, frames);
      save_ppm16 ("u.coeff", u->data, width, height, frame, frames);
      save_ppm16 ("v.coeff", v->data, width, height, frame, frames);

      ycount = encode_coeff3d (y, bitstream [0], width * height * frames);
      ucount = encode_coeff3d (u, bitstream [1], width * height * frames);
      vcount = encode_coeff3d (v, bitstream [2], width * height * frames);

      if (ycount < ylimit) ylimit = ycount;
      if (ucount < ulimit) ulimit = ucount;
      if (vcount < vlimit) vlimit = vcount;

      for (i=1; i<y2->scales; i++)  y2->minmax[i] = y->minmax[i];
      for (i=1; i<u2->scales; i++)  u2->minmax[i] = u->minmax[i];
      for (i=1; i<v2->scales; i++)  v2->minmax[i] = v->minmax[i];

      decode_coeff3d (y2, bitstream [0], ylimit);
      decode_coeff3d (u2, bitstream [1], ulimit);
      decode_coeff3d (v2, bitstream [2], vlimit);

      for (i=0; i<width*height*frames; i++) {
         rgb [3*i]   = (y->data[i] == y2->data [i]) ? 0 : ~0;
         rgb [3*i+1] = (u->data[i] == u2->data [i]) ? 0 : ~0;
         rgb [3*i+2] = (v->data[i] == v2->data [i]) ? 0 : ~0;
      }

      save_ppm ("coeffdiff", rgb, width, height, frame, frames);

      save_ppm16 ("y.rcoeff", y2->data, width, height, frame, frames);
      save_ppm16 ("u.rcoeff", u2->data, width, height, frame, frames);
      save_ppm16 ("v.rcoeff", v2->data, width, height, frame, frames);

      wavelet_3d_buf_inv_xform (y2);
      wavelet_3d_buf_inv_xform (u2);
      wavelet_3d_buf_inv_xform (v2);

      save_ppm16 ("yr", y2->data, width, height, frame, frames);
      save_ppm16 ("ur", u2->data, width, height, frame, frames);
      save_ppm16 ("vr", v2->data, width, height, frame, frames);

      yuv2rgb (y2->data, u2->data, v2->data, rgb2, width * height * frames, 3);

      save_ppm ("out", rgb2, width, height, frame, frames);

      wavelet_3d_buf_destroy (y);
      wavelet_3d_buf_destroy (u);
      wavelet_3d_buf_destroy (v);
      wavelet_3d_buf_destroy (y2);
      wavelet_3d_buf_destroy (u2);
      wavelet_3d_buf_destroy (v2);

      frame += frames;

   } while (frames == N_FRAMES);

   free (rgb);
   free (rgb2);
   free (bitstream[0]);
   free (bitstream[1]);
   free (bitstream[2]);
   return 0;
}

