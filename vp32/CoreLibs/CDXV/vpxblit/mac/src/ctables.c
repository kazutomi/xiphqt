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


#include <stdio.h>
#include <math.h>

void BuildClampTables(void);



/* altivec blitter constants */

vector signed short vp3_vConst1; 
vector signed short vp3_ditherPats_av[4];

/* lookups to avoid multiplies -- 24/32 bit conversions */
short WK_YforY[256];	
short WK_UforBG[256*2];
short WK_VforRG[256*2];

/* used by assembly 16 bit blitters */ 
unsigned long WK_ditherPats[4]; 

/*

  Video Demystified  by Keith Jack (Harris is Publisher).

	r = 1.164(Y-16) + 1.596(Cr - 128)
	g = 1.164(Y - 16) - 0.813(Cr - 128) - 0.392(Cb - 128)
	b = 1.164(Y - 16) + 2.017 (Cb - 128)

  Note that here:	Cr = Y - R
			and		Cb = Y - B

		V is used as Cr, below and u is used as Cb.
		Part of the mismatch between this and older versions of the
		YUV->RGB transcode comes from the fact that the range of Y is 16 to 235
		and U and V (Cb, Cr) is 16 to 240, so some stretching has to
		be done to get to the space where the range of R,G and B is 0-255
*/

/*
    Also see Frequently Asked Question About Color by Charles A. Poynton

    ftp://ftp.inforamp.net/pub/users/poynton/doc/colour/ColorFAQ.pdf

*/

void BuildDitherForAltivec(void)
{
	/* dither patterns for altivec blitters */
	vp3_ditherPats_av[0] = (vector signed short) (0, 6, 1, 7, 0, 6, 1, 7);
	vp3_ditherPats_av[1] = (vector signed short) (4, 2, 5, 3, 4, 2, 5, 3);
	vp3_ditherPats_av[2] = (vector signed short) (1, 7, 0, 6, 1, 7, 0, 6);
	vp3_ditherPats_av[3] = (vector signed short) (5, 3, 4, 2, 5, 3, 4, 2);
	
	vp3_vConst1 =
		(vector signed short) (9535, 13074, 6660, 3211, 16523, 16, 128, 00);
	
	
}

void BuildYUY2toRGB(void)
{
    int i,j;

    BuildClampTables();

	/* dither patterns for ppc asm blitters */
	WK_ditherPats[0] = 0x00060107; 
	WK_ditherPats[1] = 0x04020503;
	WK_ditherPats[2] = 0x01070006;
    WK_ditherPats[3] = 0x05030402;


	for (i = 0; i < 256; i++) 
        WK_YforY[i] =(short)( ( ((i-16.0)*1.164 ) ) + 0.0 );   

	for (i = 0, j = 0; i < 512; i+=2, j++) 
	{
		WK_UforBG[i] = (short)(((j-128.0)*2.017)+0.0);
		WK_UforBG[i+1] = (short)(((j-128.0)*0.392)+0.0);

		WK_VforRG[i] = (short)(((j-128.0)*1.596)+0.0);
		WK_VforRG[i+1] = (short)(((j-128.0)*0.813)+0.0);
    }
 
}


/* marky marks stuff */
typedef unsigned char RGBColorComponent;

RGBColorComponent WK_ClampTable[1024];
RGBColorComponent *WK_ClampOrigin;

#define MAX_DITHER_VALUE  7
#define MIN_OUTRANGE -276 - 4  /* rounding the first terms to be safe */
#define MAX_OUTRANGE 534 + 1 + MAX_DITHER_VALUE 


RGBColorComponent WK_ClampTable555[1024];
RGBColorComponent *WK_ClampOrigin555;

void BuildClampTables(void)
{   
    int t;
   
    /* chosen so that indexes will not outrange out storage */    
	WK_ClampOrigin = (RGBColorComponent *) &(WK_ClampTable[256+128]);  
	WK_ClampOrigin555 = (RGBColorComponent *) &(WK_ClampTable555[256+128]); 

	for (t = MIN_OUTRANGE; t <= MAX_OUTRANGE; t++) {
   		if (t < 0) {
        	WK_ClampOrigin[t] = 0;
        	WK_ClampOrigin555[t] = 0;

        }
    	else if (t > 255) 
        {
        	WK_ClampOrigin[t] = 0xff;
        	WK_ClampOrigin555[t] = 0x1f;
        } 
        else 
        {
        	WK_ClampOrigin[t] = t;
        	WK_ClampOrigin555[t] = (t>>3)&0x1f;
        }
	}
} /* WK_BuildClampTables */
