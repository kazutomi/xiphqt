
#ifndef __YUV_H
#define __YUV_H

#include <stdint.h>

extern void rgb24_to_yuv (uint8_t *rgb, int16_t *y, int16_t *u, int16_t *v, uint32_t count, uint32_t rgbstride);
extern void yuv_to_rgb24 (int16_t *y, int16_t *u, int16_t *v, uint8_t *rgb, uint32_t count, uint32_t rgbstride);


#endif

