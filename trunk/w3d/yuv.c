#include "yuv.h"


void rgb2yuv (uint8_t *rgb, int16_t *y, int16_t *u, int16_t *v, uint32_t count, uint32_t rgbstride)
{
   int i;

#if defined(TARKIN_YUV_EXACT)
   for (i=0; i<count; i++, rgb+=rgbstride) {
      y [i] = ((int16)  77 * rgb [0] + 150 * rgb [1] +  29 * rgb [2]) / 256;
      u [i] = ((int16) -44 * rgb [0] -  87 * rgb [1] + 131 * rgb [2]) / 256;
      v [i] = ((int16) 131 * rgb [0] - 110 * rgb [1] -  21 * rgb [2]) / 256;
   }
#else
   for (i=0; i<count; i++, rgb+=rgbstride) {
      u [i] = rgb [0] - rgb [1];
      v [i] = rgb [2] - rgb [1];
      y [i] = rgb [1] + ((u [i] + v [i]) >> 2);
   }
#endif
}


static inline 
uint8_t CLAMP(int16_t x)
{
   return  ((x > 255) ? 255 : (x < 0) ? 0 : x);
}


void yuv2rgb (int16_t *y, int16_t *u, int16_t *v, uint8_t *rgb, uint32_t count, uint32_t rgbstride)
{
   int i;

#if defined(TARKIN_YUV_EXACT)
   for (i=0; i<count; i++, rgb+=rgbstride) {
      rgb [0] = CLAMP(y [i] + 1.371 * v [i]);
      rgb [1] = CLAMP(y [i] - 0.698 * v [i] - 0.336 * u [i]);
      rgb [2] = CLAMP(y [i] + 1.732 * u [i]);
   }
#else
   for (i=0; i<count; i++, rgb+=rgbstride) {
      rgb [1] = CLAMP(y [i] - ((u [i] + v [i]) >> 2));
      rgb [2] = CLAMP(v [i] + rgb [1]);
      rgb [0] = CLAMP(u [i] + rgb [1]);
   }
#endif
}

