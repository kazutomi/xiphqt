
#ifndef __YUV_H
#define __YUF_H

#include "global_defs.h"

extern void rgb2yuv (uint8 *rgb, int16 *y, int16 *u, int16 *v, uint32 count, uint32 stride);
extern void yuv2rgb (int16 *y, int16 *u, int16 *v, uint8 *rgb, uint32 count, uint32 stride);


#endif

