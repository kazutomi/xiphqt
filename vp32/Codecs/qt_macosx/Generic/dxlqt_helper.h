//============================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////
//
// dxlqt_helper.h
//
// Purpose: A list of helper functions to the quick time codec code
//
/////////////////////////////////////////////////////////////////////////


// draw informative dots on to the screen
void DrawInformativeDots( unsigned char* startingPositionOfBuffer,enum BITDEPTH displayType);

// draw checkerboard pattern on screen 
void DrawCheckers(char *addr,long w,long h);

// draw Random Color  on screen 
void DrawRandom(char *addr,long w,long h,long p);

// shift up by one
void Shift(char *addr,long w,long h);

// Convert the condition flags to a string (Helper Function)
void condflags2string(long flags,char * str);

// Convert the caller flags to a string (Helper Function)
void callflags2string(long flags,char * str);

// Get system milliseconds
unsigned long Milliseconds(void) ;

// convert bitdepth 2 string
void bitdepth2string(enum BITDEPTH bitdepth,char *str);

extern "C"
void ConvertRGBtoYUV(
	unsigned char *r_src,unsigned char *g_src,unsigned char *b_src, 
	int width, int height, int rgb_step, int rgb_pitch,
	unsigned char *y_src, unsigned char *u_src, unsigned char *v_src,  
	int uv_width_shift, int uv_height_shift,
	int y_step, int y_pitch,int uv_step,int uv_pitch
	);
