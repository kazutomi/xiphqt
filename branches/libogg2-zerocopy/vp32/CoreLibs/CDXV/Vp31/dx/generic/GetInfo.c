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


#include "duck_dxl.h"

/****************************************************************************
 * 
 *  ROUTINE       :     Get Info About Frame
 *
 *  INPUTS        :     Nonex.
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Called to determine information about a frame without
 *                      doing any decompression at all!..
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void vp31_GetInfo(unsigned char * source, FrameInfo * frameInfo)
{

    // Is the frame and inter frame or a key frame
    frameInfo->KeyFrame = !(source[0] > 0x7f);
    frameInfo->Quality = source[0] >> 2;
    if(frameInfo->KeyFrame) 
        frameInfo->Version = ((source[2]>>3) & 0x1f );
    else
        frameInfo->Version = 0;

    frameInfo->vp30Flag = (int)source[1];

}
