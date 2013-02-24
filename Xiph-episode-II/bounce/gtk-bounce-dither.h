#ifndef _BOUNCE_DITHER_H_
#define _BOUNCE_DITHER_H_

#include <math.h>
#include <stdlib.h>

typedef struct shapestate {
  float b_buf[8];
  float a_buf[8];
  int mute;
} shapestate;

extern void subquant_dither_to_X(shapestate *ss, float *data, int n,
                              float quant_bits);

#endif
