/*Forward DCT transforms.*/
#include "encint.h"
#if !defined(_fdct_H)
# define _fdct_H (1)

void oc_fdct8x8(ogg_int16_t _y[64],const ogg_int16_t _x[64]);
void oc_fdct8x8_border(const oc_border_info *_border,ogg_int16_t _y[64],
 ogg_int16_t _x[64]);
void oc_frag_intra_fdct(const oc_fragment *_frag,ogg_int16_t _dct_vals[64],
 int _ystride,int _framei);
void oc_fdct_pipe_init(oc_enc_pipe_stage *_stage,oc_enc_ctx *_enc);

#endif
