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
#include "dkpltfrm.h"

#include "vfw_pb_interface.h"

/* params for blitters */
/*
typedef struct {
    int     YWidth;
    int     YHeight;
    int     YStride;

    int     UVWidth;
    int     UVHeight;
    int     UVStride;

    char *  YBuffer;
    char *  UBuffer;
    char *  VBuffer;

    unsigned char     *uvStart;
    int     uvDstArea;
    int     uvUsedArea;
} YUV_BUFFER_CONFIG;
*/

void bcy00_c(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
{
    unsigned char *ptrScrn = (unsigned char *)_ptrScreen;
    
    unsigned char *YBuffer = (unsigned char *)src->YBuffer;
    unsigned char *UBuffer = (unsigned char *)src->UBuffer;
    unsigned char *VBuffer = (unsigned char *)src->VBuffer;

    unsigned long yTemp;
    int i, j;

    for(i = 0; i < src->YHeight; i += 1)
    {
        int x;
   
        for(j = 0, x = 0; j < src->YWidth; j += 4, x += 2)
        {
        
            /* get 2 y's */
        	yTemp = (unsigned long) (YBuffer[j+0] | ((YBuffer[j+1]) << 16));

            /* get 1 uv's */
            yTemp |= (unsigned long)(UBuffer[x+0] << 8);
            yTemp |= (unsigned long)(VBuffer[x+0] << 24);
        	
            ((unsigned long *) ptrScrn)[x] = yTemp;

            /* get 2 y's */
        	yTemp = (unsigned long) (YBuffer[j+2] | ((YBuffer[j+3]) << 16));

            /* get 1 uv's */
            yTemp |= (unsigned long)(UBuffer[x+1] << 8);
            yTemp |= (unsigned long)(VBuffer[x+1] << 24);

            ((unsigned long *) ptrScrn)[x+1] = yTemp;

        } /* inner for */

        ptrScrn += thisPitch;
        YBuffer -= src->YStride;

        /* see mmx asm code on how to remove this branch */
        if(i & 1)
        {
            UBuffer -= src->UVStride;
            VBuffer -= src->UVStride;
        }
    } /* outer for */
}

