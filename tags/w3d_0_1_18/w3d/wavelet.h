#ifndef __WAVELET_H
#define __WAVELET_H

#include "global_defs.h"


typedef struct {
   TYPE *data;
   uint32 width;
   uint32 height;
   uint32 frames;
   uint32 scales;
   uint32 *w;
   uint32 *h;
   uint32 *f;
   TYPE *minmax;
} Wavelet3DBuf;


extern Wavelet3DBuf* wavelet_3d_buf_new (uint32 width, uint32 height,
                                         uint32 frames);

extern void wavelet_3d_buf_destroy (Wavelet3DBuf* buf);

extern void wavelet_3d_buf_fwd_xform (Wavelet3DBuf* buf);
extern void wavelet_3d_buf_inv_xform (Wavelet3DBuf* buf);


#endif
