
#ifndef __YUV_H
#define __YUF_H

#include <stdint.h>

extern void rgb2yuv (uint8_t *rgb, int16_t *y, int16_t *u, int16_t *v, uint32_t count, uint32_t rgbstride);
extern void yuv2rgb (int16_t *y, int16_t *u, int16_t *v, uint8_t *rgb, uint32_t count, uint32_t rgbstride);


#endif

