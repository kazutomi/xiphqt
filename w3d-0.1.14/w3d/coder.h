
#ifndef __CODER_H
#define __CODER_H

#include <stdlib.h>
#include "global_defs.h"
#include "wavelet.h"


typedef struct {
   int     bit_count;          /*  number of valid bits in byte    */
   uint8   byte;               /*  buffer to save bits             */
   int     byte_count;         /*  number of bytes written         */
   uint8  *bitstream;
   size_t  limit;              /*  don't write more bytes to bitstream ... */
} BitCoderState;


typedef struct {
   int mps;                    /*  more probable symbol            */
   int count;                  /*  have seen count+1 mps's         */
   BitCoderState bitcoder;
} RLECoderState;



typedef struct {
   Wavelet3DBuf *waveletbuf;
   RLECoderState  rlecoder;
} Coder;



extern size_t encode_coeff3d (Wavelet3DBuf *waveletbuf, uint8 *bitstream, size_t limit);
extern void   decode_coeff3d (Wavelet3DBuf *waveletbuf, uint8 *bitstream, size_t count);

#endif

