#include "yuv.h"

void rgb2yuv (uint8 *rgb, int16 *y, int16 *u, int16 *v, uint32 count, uint32 stride)
{
/*
   for (; count; count--, rgb+=stride, y++, u++, v++) {
      *y = ((int16)  77 * rgb [0] + 150 * rgb [1] +  29 * rgb [2]) / 256;
      *u = ((int16) -44 * rgb [0] -  87 * rgb [1] + 131 * rgb [2]) / 256;
      *v = ((int16) 131 * rgb [0] - 110 * rgb [1] -  21 * rgb [2]) / 256;
   }
*/
   for (; count; count--, rgb+=stride, y++, u++, v++) {
      *u = rgb [0] - rgb [1];
      *v = rgb [1] - rgb [2];
      *y = rgb [1] + ((*u + *v) >> 2);
   }
}


#define CLAMP(x) x
#undef CLAMP
static inline uint8 CLAMP(int16 x) { return  x > 255 ? 255 : x < 0 ? 0 : x; }


void yuv2rgb (int16 *y, int16 *u, int16 *v, uint8 *rgb, uint32 count, uint32 stride)
{
/*
   for (; count; count--, rgb+=stride, y++, u++, v++) {
      rgb [0] = CLAMP(*y + 1.371 * (*v));
      rgb [1] = CLAMP(*y - 0.698 * (*v) - 0.336 * (*u));
      rgb [2] = CLAMP(*y + 1.732 * (*u));
   }
*/
   for (; count; count--, rgb+=stride, y++, u++, v++) {
      rgb [1] = CLAMP(*y - ((*u + *v) >> 2));
      rgb [2] = CLAMP(rgb [1] - *v);
      rgb [0] = CLAMP(*u + rgb [1]);
   }
}

