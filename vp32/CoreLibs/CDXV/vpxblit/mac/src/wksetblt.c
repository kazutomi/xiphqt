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


#include "dxl_main.h"

#include "vfw_pb_interface.h"


#include <gestalt.h>



/* ------------------------------------------- */
void BuildYUY2toRGB(void);

/* ------------------------------------------- */
extern void bcs00(unsigned char *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcs10(unsigned char *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

extern void bct00(unsigned char *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bct10(unsigned char *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

extern void bcy00(unsigned char *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

/* ------------------------------------------- */
/* Altivec blitters */
void bct00av(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src);
void bcs00av(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src);
void bcy00av(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src);

void bct10av(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src);
void bcs10av(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src);

void BuildDitherForAltivec(void);

/* ------------------------------------------- */
void vp3SetBlit(void)
{ 
    int temp;
	blitFunc dummy = (blitFunc)-1L;


/* this should be eventually be moved into dxv */
	OSErr err;
	long processorAttributes = 0;
	long hasAltiVec = 0;
 
 	err = Gestalt(gestaltPowerPCProcessorFeatures, &processorAttributes);
	if (err == noErr)
    	hasAltiVec = (1 << gestaltPowerPCHasVectorInstructions) & processorAttributes;
/* this should be eventually be moved into dxv */


	if(hasAltiVec)
	{
		BuildDitherForAltivec();
	
	    /* 555 */
	    temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB16_555);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs00av, dummy, dummy); 

	    temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB16_555);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs10av, dummy, dummy); 


		/* rgb 32 */
	    temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB32);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct00av, dummy, dummy); 

	    temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB32);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct10av, dummy, dummy); 


	/*
	yuy2 currently not implemented yet
	*/
	    temp = DXL_OverrideBlitter(DXBLIT_SAME,DXYUY2);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00av, dummy, dummy); 

	    temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXYUY2);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00av, dummy, dummy); 
	}
	else
	{
	    /* 555 */
	    temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB16_555);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs00, dummy, dummy); 

	    temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB16_555);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs10, dummy, dummy); 


		/* rgb 32 */
	    temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB32);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct00, dummy, dummy); 

	    temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB32);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct10, dummy, dummy); 


	/*
	yuy2 currently not implemented yet
	*/
	    temp = DXL_OverrideBlitter(DXBLIT_SAME,DXYUY2);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00, dummy, dummy); 

	    temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXYUY2);
	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00, dummy, dummy); 
	}
	
	
    /* build the conversion tables */
    BuildYUY2toRGB();
}
