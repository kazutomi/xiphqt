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
#include "cpuidlib.h"
/* ------------------------------------------- */
void BuildYUY2toRGB(void);

/* ------------------------------------------- */

extern void bcs00_555_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcs00_565_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcf00_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bct00_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

extern void bcy00_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcc00_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

extern void bcs10_555_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcs10_565_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcf10_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bct10_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 


/* ------------------------------------------- */
/* C VERSIONS */
extern void bcy00_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

extern void bct00_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bct10_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

extern void bcf00_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcf10_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

extern void bcs00_555_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcs00_565_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

extern void bcs10_555_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
extern void bcs10_565_c(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 



/* ------------------------------------------- */
void vp3SetBlit(void)
{ 
    int temp;
	blitFunc dummy = (blitFunc)-1L;
	enum PROCTYPE cpuID =  findCPUId();
	
    /* build the conversion tables */
    BuildYUY2toRGB();

    switch(cpuID)
    {
		case WMT:
		case XMM:
        case PII:
        case AMDK6:
        case AMDK63D:
        case C6X86MX:
	        /* yv12 */
            temp = DXL_OverrideBlitter(DXBLIT_SAME,DXYV12);
	        DXL_RegisterBlitter(temp, YV12, (blitFunc)bcc00_MMX, dummy, dummy); 

	        /* yuy2 */
            temp = DXL_OverrideBlitter(DXBLIT_SAME,DXYUY2);
	        DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXYUY2);
	        DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00_MMX, dummy, dummy); 

            /* 555 */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB16_555);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs00_555_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB16_555);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs10_555_MMX, dummy, dummy); 

            /* 565 */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB16_565);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs00_565_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB16_565);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs10_565_MMX, dummy, dummy); 

            /* 24 bit */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB24);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcf00_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB24);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcf10_MMX, dummy, dummy); 

            /* 32 bit */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB32);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct00_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB32);
		    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct10_MMX, dummy, dummy); 
            break;

        case PMMX:
	        /* yuy2 */
            temp = DXL_OverrideBlitter(DXBLIT_SAME,DXYUY2);
	        DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXYUY2);
	        DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00_MMX, dummy, dummy); 

            /* 555 */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB16_555);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs00_555_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB16_555);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs10_555_MMX, dummy, dummy); 

            /* 565 */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB16_565);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs00_565_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB16_565);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs10_565_MMX, dummy, dummy); 

            /* 24 bit */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB24);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcf00_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB24);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcf10_MMX, dummy, dummy); 

            /* 32 bit */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB32);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct00_MMX, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB32);
		    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct10_MMX, dummy, dummy); 
            break;

/* for now this is all c */
        default: 
	        /* yuy2 */
            temp = DXL_OverrideBlitter(DXBLIT_SAME,DXYUY2);
	        DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00_c, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXYUY2);
	        DXL_RegisterBlitter(temp, YV12, (blitFunc)bcy00_c, dummy, dummy); 

            /* 555 */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB16_555);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs00_555_c, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB16_555);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs10_555_c, dummy, dummy); 

            /* 565 */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB16_565);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs00_565_c, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB16_565);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcs10_565_c, dummy, dummy); 

            /* 24 bit */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB24);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcf00_c, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB24);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bcf10_c, dummy, dummy); 

            /* 32 bit */
	        temp = DXL_OverrideBlitter(DXBLIT_SAME,DXRGB32);
    	    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct00_c, dummy, dummy); 

	        temp = DXL_OverrideBlitter(DXBLIT_STRETCH,DXRGB32);
		    DXL_RegisterBlitter(temp, YV12, (blitFunc)bct10_c, dummy, dummy); 

            break;
    }
}

