/*Forward DCT transforms.*/
#include "encint.h"
#if !defined(_fdct_H)
# define _fdct_H (1)

void oc_fdct8x8(ogg_int16_t _y[64],const ogg_int16_t _x[64]);
void oc_border_enc_info_init(oc_border_enc_info *_border_info,
 const oc_border_info *_border);
void oc_fdct8x8_border(const oc_border_info *_border,
 const oc_border_enc_info *_border_info,ogg_int16_t _y[64],
 ogg_int16_t _x[64]);

#endif
