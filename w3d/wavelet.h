#ifndef __WAVELET_H
#define __WAVELET_H

#include <stdint.h>


typedef struct {
   TYPE *data;
   uint32_t width;
   uint32_t height;
   uint32_t frames;
   uint32_t scales;
   uint32_t *w;
   uint32_t *h;
   uint32_t *f;
   uint32_t (*offset)[8];
   TYPE *scratchbuf;
} Wavelet3DBuf;


extern Wavelet3DBuf* wavelet_3d_buf_new (uint32_t width, uint32_t height,
                                         uint32_t frames);

extern void wavelet_3d_buf_destroy (Wavelet3DBuf* buf);

extern void wavelet_3d_buf_fwd_xform (Wavelet3DBuf* buf);
extern void wavelet_3d_buf_inv_xform (Wavelet3DBuf* buf);

#endif
