#include <stdlib.h>
#include "wavelet.h"



Wavelet3DBuf* wavelet_3d_buf_new (uint32 width, uint32 height, uint32 frames)
{
   Wavelet3DBuf* buf = (Wavelet3DBuf*) malloc (sizeof (Wavelet3DBuf));
   uint32 _w = width;
   uint32 _h = height;
   uint32 _f = frames;
   int level;

   if (!buf)
      return NULL;

   buf->data = (TYPE*) malloc (width * height * frames * sizeof (TYPE));

   if (!buf->data) {
      wavelet_3d_buf_destroy (buf);
      return NULL;
   }

   buf->width = width;
   buf->height = height;
   buf->frames = frames;
   buf->scales = 1;

   while (_w > 1 || _h > 1 || _f > 1) {
      buf->scales++;
      _w = (_w+1)/2;
      _h = (_h+1)/2;
      _f = (_f+1)/2;
   }

   buf->w = (uint32*) malloc (buf->scales * sizeof (uint32));
   buf->h = (uint32*) malloc (buf->scales * sizeof (uint32));
   buf->f = (uint32*) malloc (buf->scales * sizeof (uint32));
   buf->minmax = (TYPE*) malloc (buf->scales * sizeof (TYPE));

   if (!buf->w || !buf->h || !buf->f || !buf->minmax) {
      wavelet_3d_buf_destroy (buf);
      return NULL;
   }

   buf->w [buf->scales-1] = width;
   buf->h [buf->scales-1] = height;
   buf->f [buf->scales-1] = frames;

   for (level=buf->scales-2; level>=0; level--) {
      buf->w [level] = (buf->w [level+1] + 1) / 2;
      buf->h [level] = (buf->h [level+1] + 1) / 2;
      buf->f [level] = (buf->f [level+1] + 1) / 2;
   }

   return buf;
}




void wavelet_3d_buf_destroy (Wavelet3DBuf* buf)
{
   if (buf) {
      if (buf->data)
         free (buf->data);
      if (buf->w)
         free (buf->w);
      if (buf->h)
         free (buf->h);
      if (buf->f)
         free (buf->f);
      free (buf);
   }
}




/*
 *  returns value is MAX(largest coefficient, ~(smallest coefficient))
 *  (will be used to skip empty bitplanes)
 */
static inline
TYPE __fwd_xform__ (TYPE *data, int stride, int n)
{
   TYPE *_d = (TYPE*) malloc (sizeof(TYPE) * n/2);
   TYPE *x = data;
   TYPE *s = data;
   TYPE *d = _d;
   TYPE min = ~0;
   TYPE max = 0;
   int i, k=n/2;

   for (i=0; i<((n&1) ? k : (k-1)); i++)   /*  highpass coefficients */
      d [i] = x [(2*i+1)*stride] - ((x [2*i*stride] + x[(2*i+2)*stride]) >> 1);

   if (!(n & 1))                                   /*  n is even       */
      d [k-1] = x[(n-1)*stride] - x[(n-2)*stride];

   s [0] = x[0] + (d[0] >> 1);                /*  lowpass coefficients */

   for (i=1; i<k; i++)
      s [i*stride] = x [2*i*stride] + ((d [i-1] + d[i]) >> 2);

   if (n & 1 || n <=2)                          /*  n is odd   */
      s [k*stride] = x[(n-1)*stride] + (d[k-1] >> 1);

   for (i=0; i<n/2; i++) {
      x [(n-k+i)*stride] = d [i];

      if (d [i] > max)
         max = d [i];

      if (d [i] < min)
         min = d [i];
   }

   free (_d);

   return (max | ~min);   
}


static inline
void __inv_xform__ (TYPE *data, int stride, int n)
{
   int i, k=n/2;
   TYPE *_s = (TYPE*) malloc (sizeof(TYPE) * k+1);
   TYPE *_d = (TYPE*) malloc (sizeof(TYPE) * k);
   TYPE *x = data;
   TYPE *d = _d;
   TYPE *s = _s;


   for (i=0; i<k+1; i++)
      s [i] = x [i*stride];

   for (i=0; i<n-k; i++)
      d [i] = x [(n-k+i)*stride];

   x [0] = s[0] - (d[0] >> 1);

   for (i=1; i<k; i++)
      x [2*i*stride] = s [i] - ((d [i-1] + d[i]) >> 2);

   if (n & 1 || n <= 2)                         /*  n is odd   */
      x [(n-1)*stride] = s[k] - (d[k-1] >> 1);

   for (i=0; i<((n&1) ? k : (k-1)); i++)
      x [(2*i+1)*stride] = d [i] + ((x [2*i*stride] + x[(2*i+2)*stride]) >> 1);

   if (!(n & 1))                                /*  n is even   */
      x [(n-1)*stride] = d[k-1] + x[(n-2)*stride];

   free (_s);
   free (_d);
}





void wavelet_3d_buf_fwd_xform (Wavelet3DBuf* buf)
{
   int level;

   for (level=buf->scales-1; level>0; level--) {
      uint32 w = buf->w[level];
      uint32 h = buf->h[level];
      uint32 f = buf->f[level];
      buf->minmax[level] = 0;

      if (w > 1) {
         int row, frame;
         for (frame=0; frame<f; frame++) {
            for (row=0; row<h; row++) {
               TYPE *data = buf->data + (frame * buf->height + row) * buf->width;
               buf->minmax [level] |= __fwd_xform__ (data, 1, w);
            }
         }
      }

      if (h > 1) {
         int col, frame;
         for (frame=0; frame<f; frame++) {
            for (col=0; col<w; col++) {
               TYPE *data = buf->data + frame * buf->width * buf->height + col;
               buf->minmax [level] |= __fwd_xform__ (data, buf->width, h);
            }
         }
      }

      if (f > 1) {
         int i, j;
         for (j=0; j<h; j++) {
            for (i=0; i<w; i++) {
               TYPE *data = buf->data + j*buf->width + i;
               buf->minmax [level] |= __fwd_xform__ (data, buf->width * buf->height, f);
            }
         }
      }
   }

printf ("s == %i, minmax == %i\n", buf->data[0], buf->minmax [2]);

   if (buf->data[0] >= 0)          /*  put DC coefficient bitmask in level 2 */
      buf->minmax [2] |= buf->data[0];
   else
      buf->minmax [2] |= ~(buf->data[0]);
printf ("s == %i, minmax == %i\n", buf->data[0], buf->minmax [2]);
printf ("\n");
}




void wavelet_3d_buf_inv_xform (Wavelet3DBuf* buf)
{
   int level;

   for (level=1; level<buf->scales; level++) {
      uint32 w = buf->w[level];
      uint32 h = buf->h[level];
      uint32 f = buf->f[level];

      if (f > 1) {
         int i, j;
         for (j=0; j<h; j++) {
            for (i=0; i<w; i++) {
               TYPE *data = buf->data + j*buf->width + i;
               __inv_xform__ (data, buf->width * buf->height, f);
            }
         }
      }

      if (h > 1) {
         int col, frame;
         for (frame=0; frame<f; frame++) {
            for (col=0; col<w; col++) {
               TYPE *data = buf->data + frame * buf->width * buf->height + col;
               __inv_xform__ (data, buf->width, h);
            }
         }
      }

     if (w > 1) {
         int row, frame;
         for (frame=0; frame<f; frame++) {
            for (row=0; row<h; row++) {
               TYPE *data = buf->data + (frame * buf->height + row) * buf->width;
               __inv_xform__ (data, 1, w);
            }
         }
      }
   }
}

