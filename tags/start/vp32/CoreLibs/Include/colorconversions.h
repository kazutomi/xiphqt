//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//--------------------------------------------------------------------------


#ifndef COLORCONVERSIONS_H
#define COLORCONVERSIONS_H

void CC_RGB32toYV12_C( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
                       unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer );

void CC_RGB24toYV12_C( unsigned char *RGBBuffer, int ImageWidth, int ImageHeight,
                       unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer );

void CC_YVYUtoYV12_C( unsigned char *YVYUBuffer, int ImageWidth, int ImageHeight,
                      unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer );

void ConvertRGBtoYUV(
	unsigned char *r_src,unsigned char *g_src,unsigned char *b_src, 
	int width, int height, int rgb_step, int rgb_pitch,
	unsigned char *y_src, unsigned char *u_src, unsigned char *v_src,  
	int uv_width_scale, int uv_height_scale,
	int y_step, int y_pitch,int uv_step,int uv_pitch
	);


#endif /* COLORCONVERSIONS_H */
