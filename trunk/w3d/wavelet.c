#include <stdlib.h>
#include "wavelet.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MAX3(a,b,c) (MAX(a,MAX(b,c)))


Wavelet3DBuf* wavelet_3d_buf_new (uint32_t width, uint32_t height, uint32_t frames)
{
   Wavelet3DBuf* buf = (Wavelet3DBuf*) malloc (sizeof (Wavelet3DBuf));
   uint32_t _w = width;
   uint32_t _h = height;
   uint32_t _f = frames;
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

   buf->w = (uint32_t*) malloc (buf->scales * sizeof (uint32_t));
   buf->h = (uint32_t*) malloc (buf->scales * sizeof (uint32_t));
   buf->f = (uint32_t*) malloc (buf->scales * sizeof (uint32_t));
   buf->offset = (uint32_t (*) [8]) malloc (8 * buf->scales * sizeof (uint32_t));

   buf->scratchbuf = (TYPE*) malloc (MAX3(width, height, frames) * sizeof (TYPE));

   if (!buf->w || !buf->h || !buf->f || !buf->offset || !buf->scratchbuf) {
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
      buf->offset[level][0] = 0;
      buf->offset[level][1] = buf->w [level];
      buf->offset[level][2] = buf->h [level] * width;
      buf->offset[level][3] = buf->f [level] * width * height;
      buf->offset[level][4] = buf->offset [level][2] + buf->w [level];
      buf->offset[level][5] = buf->offset [level][3] + buf->w [level];
      buf->offset[level][6] = buf->offset [level][3] + buf->offset [level][2];
      buf->offset[level][7] = buf->offset [level][6] + buf->w [level];
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
      if (buf->offset)
         free (buf->offset);
      if (buf->scratchbuf)
         free (buf->scratchbuf);
      free (buf);
   }
}




static inline
void __fwd_xform__ (Wavelet3DBuf *buf, TYPE *data, int stride, int n)
{
   TYPE *d = buf->scratchbuf;
   TYPE *x = data;
   TYPE *s = data;
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

   for (i=0; i<n/2; i++)
      x [(n-k+i)*stride] = d [i];
}


static inline
void __inv_xform__ (Wavelet3DBuf *buf, TYPE *data, int stride, int n)
{
   int i, k=n/2;
   TYPE *s = buf->scratchbuf;
   TYPE *d = buf->scratchbuf + n - k;
   TYPE *x = data;

   for (i=0; i<k+1; i++)
      s [i] = x [i*stride];

   for (i=0; i<n/2; i++)
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
}





void wavelet_3d_buf_fwd_xform (Wavelet3DBuf* buf)
{
   int level;

   for (level=buf->scales-1; level>0; level--) {
      uint32_t w = buf->w[level];
      uint32_t h = buf->h[level];
      uint32_t f = buf->f[level];

      if (w > 1) {
         int row, frame;
         for (frame=0; frame<f; frame++) {
            for (row=0; row<h; row++) {
               TYPE *data = buf->data + (frame * buf->height + row) * buf->width;
               __fwd_xform__ (buf, data, 1, w);
            }
         }
      }

      if (h > 1) {
         int col, frame;
         for (frame=0; frame<f; frame++) {
            for (col=0; col<w; col++) {
               TYPE *data = buf->data + frame * buf->width * buf->height + col;
               __fwd_xform__ (buf, data, buf->width, h);
            }
         }
      }

      if (f > 1) {
         int i, j;
         for (j=0; j<h; j++) {
            for (i=0; i<w; i++) {
               TYPE *data = buf->data + j*buf->width + i;
               __fwd_xform__ (buf, data, buf->width * buf->height, f);
            }
         }
      }
   }
}




void wavelet_3d_buf_inv_xform (Wavelet3DBuf* buf)
{
   int level;

   for (level=1; level<buf->scales; level++) {
      uint32_t w = buf->w[level];
      uint32_t h = buf->h[level];
      uint32_t f = buf->f[level];

      if (f > 1) {
         int i, j;
         for (j=0; j<h; j++) {
            for (i=0; i<w; i++) {
               TYPE *data = buf->data + j*buf->width + i;
               __inv_xform__ (buf, data, buf->width * buf->height, f);
            }
         }
      }

      if (h > 1) {
         int col, frame;
         for (frame=0; frame<f; frame++) {
            for (col=0; col<w; col++) {
               TYPE *data = buf->data + frame * buf->width * buf->height + col;
               __inv_xform__ (buf, data, buf->width, h);
            }
         }
      }

     if (w > 1) {
         int row, frame;
         for (frame=0; frame<f; frame++) {
            for (row=0; row<h; row++) {
               TYPE *data = buf->data + (frame * buf->height + row) * buf->width;
               __inv_xform__ (buf, data, 1, w);
            }
         }
      }
   }
}


