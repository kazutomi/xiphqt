
#ifndef __CODER_H
#define __CODER_H

#include <stdlib.h>
#include <stdint.h>
#include "wavelet.h"



extern size_t encode_coeff3d (Wavelet3DBuf *waveletbuf, uint8_t *bitstream, size_t limit);
extern void   decode_coeff3d (Wavelet3DBuf *waveletbuf, uint8_t *bitstream, size_t count);

extern void predict_childs3d (Wavelet3DBuf *waveletbuf);
extern void update_childs3d (Wavelet3DBuf *waveletbuf);


#endif

