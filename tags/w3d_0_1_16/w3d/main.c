
#include <stdio.h>
#include "videodev.h"
#include "wavelet.h"
#include "coder.h"
#include "yuv.h"


#define  N_FRAMES  1


void save_ppm (char *prefix, uint8 *buf, int w, int h)
{
   int i;

   for (i=0; i<N_FRAMES; i++)
   {
      char fname [256];
      FILE *outfile;
      uint8 *img = buf + w * h * 3 * i;

      sprintf (fname , "%s%i.ppm", prefix, i);
      outfile = fopen (fname, "w");
      fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
      fwrite (img, 3, w*h, outfile);
   }
}


void save_ppm16 (char *prefix, int16 *buf, int w, int h)
{
   int i, j;

   for (i=0; i<N_FRAMES; i++)
   {
      char fname [256];
      FILE *outfile;
      int16 *img = buf + w * h * i;

      sprintf (fname , "%s%i.ppm", prefix, i);
      outfile = fopen (fname, "w");
      fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
      for (j=0; j<w*h; j++) {
         uint8 c [3] = { img[j], img [j], img[j] };
         fwrite (c, 1, 3, outfile);
      }
   }
}




int main (int argc, char **argv)
{
   VideoDev *vdev;
   char *vdev_name = "/dev/video0";
   uint8 *rgb, *rgb2;
   char *bitstream [3];
   int i, ycount, ucount, vcount;
   int ylimit, ulimit, vlimit;
   Wavelet3DBuf *y, *u, *v, *y2, *u2, *v2;

   if (argc == 5)
      vdev_name = argv[4];
   else if (argc != 4) {
      printf ("\n"
        " usage: %s <ylimit> <ulimit> <vlimit> <videodevice>\n"
        "\n"
        "   ylimit, ulimit, vlimit: cut Y/U/V bitstream after limit bytes\n"
        "   videodevice:            optional, /dev/video0 by default\n"
        "\n", argv[0]);
      exit (-1);
   }

   ylimit = strtol (argv[1], 0, 0);
   ulimit = strtol (argv[2], 0, 0);
   vlimit = strtol (argv[3], 0, 0);

   vdev = video_device_new (vdev_name);

   if (!vdev) {
      printf ("failed opening videodevice.\n");
      return (-1);
   }

   rgb  = malloc (vdev->win.width * vdev->win.height * 3 * N_FRAMES);
   rgb2 = malloc (vdev->win.width * vdev->win.height * 3 * N_FRAMES);
   bitstream[0] = malloc (vdev->win.width * vdev->win.height * N_FRAMES);
   bitstream[1] = malloc (vdev->win.width * vdev->win.height * N_FRAMES);
   bitstream[2] = malloc (vdev->win.width * vdev->win.height * N_FRAMES);

   y = wavelet_3d_buf_new (vdev->win.width, vdev->win.height, N_FRAMES);
   u = wavelet_3d_buf_new (vdev->win.width, vdev->win.height, N_FRAMES);
   v = wavelet_3d_buf_new (vdev->win.width, vdev->win.height, N_FRAMES);
   y2 = wavelet_3d_buf_new (vdev->win.width, vdev->win.height, N_FRAMES);
   u2 = wavelet_3d_buf_new (vdev->win.width, vdev->win.height, N_FRAMES);
   v2 = wavelet_3d_buf_new (vdev->win.width, vdev->win.height, N_FRAMES);

   if (!rgb || !rgb2 || !y || !u || !v || !y2 || !u2 || !v2 ||
       !bitstream[0] || !bitstream[1] || !bitstream[2])
   {
      printf ("memory allocation failed.\n");
      return (-1);
   }

   video_device_try_palette (vdev, VIDEO_PALETTE_RGB24);

   for (i=0; i<N_FRAMES; i++)
      video_device_grab_frame (vdev, rgb + vdev->win.width * vdev->win.height * 3 * i);

   save_ppm ("orig", rgb, vdev->win.width, vdev->win.height);

   rgb2yuv (rgb, y->data, u->data, v->data,
            vdev->win.width * vdev->win.height * N_FRAMES, 3);

   save_ppm16 ("y", y->data, vdev->win.width, vdev->win.height);
   save_ppm16 ("u", u->data, vdev->win.width, vdev->win.height);
   save_ppm16 ("v", v->data, vdev->win.width, vdev->win.height);

   wavelet_3d_buf_fwd_xform (y);
   wavelet_3d_buf_fwd_xform (u);
   wavelet_3d_buf_fwd_xform (v);

   save_ppm16 ("y.coeff", y->data, vdev->win.width, vdev->win.height);
   save_ppm16 ("u.coeff", u->data, vdev->win.width, vdev->win.height);
   save_ppm16 ("v.coeff", v->data, vdev->win.width, vdev->win.height);

   ycount = encode_coeff3d (y, bitstream [0], vdev->win.width * vdev->win.height * N_FRAMES);
   ucount = encode_coeff3d (u, bitstream [1], vdev->win.width * vdev->win.height * N_FRAMES);
   vcount = encode_coeff3d (v, bitstream [2], vdev->win.width * vdev->win.height * N_FRAMES);

   if (ycount < ylimit) ylimit = ycount;
   if (ucount < ulimit) ulimit = ucount;
   if (vcount < vlimit) vlimit = vcount;

   for (i=1; i<y2->scales; i++)  y2->minmax[i] = y->minmax[i];
   for (i=1; i<u2->scales; i++)  u2->minmax[i] = u->minmax[i];
   for (i=1; i<v2->scales; i++)  v2->minmax[i] = v->minmax[i];

   decode_coeff3d (y2, bitstream [0], ylimit);
   decode_coeff3d (u2, bitstream [1], ulimit);
   decode_coeff3d (v2, bitstream [2], vlimit);

   for (i=0; i<vdev->win.width*vdev->win.height*N_FRAMES; i++) {
      rgb [3*i]   = (y->data[i] == y2->data [i]) ? 0 : ~0;
      rgb [3*i+1] = (u->data[i] == u2->data [i]) ? 0 : ~0;
      rgb [3*i+2] = (v->data[i] == v2->data [i]) ? 0 : ~0;
   }

   save_ppm ("coeffdiff", rgb, vdev->win.width, vdev->win.height);

   save_ppm16 ("y.rcoeff", y2->data, vdev->win.width, vdev->win.height);
   save_ppm16 ("u.rcoeff", u2->data, vdev->win.width, vdev->win.height);
   save_ppm16 ("v.rcoeff", v2->data, vdev->win.width, vdev->win.height);

   wavelet_3d_buf_inv_xform (y2);
   wavelet_3d_buf_inv_xform (u2);
   wavelet_3d_buf_inv_xform (v2);

   save_ppm16 ("yr", y2->data, vdev->win.width, vdev->win.height);
   save_ppm16 ("ur", u2->data, vdev->win.width, vdev->win.height);
   save_ppm16 ("vr", v2->data, vdev->win.width, vdev->win.height);

   yuv2rgb (y2->data, u2->data, v2->data, rgb2,
            vdev->win.width * vdev->win.height * N_FRAMES, 3);

   save_ppm ("out", rgb2, vdev->win.width, vdev->win.height);

   video_device_destroy (vdev);

   free (rgb);
   free (rgb2);
   wavelet_3d_buf_destroy (y);
   wavelet_3d_buf_destroy (u);
   wavelet_3d_buf_destroy (v);
   wavelet_3d_buf_destroy (y2);
   wavelet_3d_buf_destroy (u2);
   wavelet_3d_buf_destroy (v2);
   free (bitstream[0]);
   free (bitstream[1]);
   free (bitstream[2]);
   return 0;
}

