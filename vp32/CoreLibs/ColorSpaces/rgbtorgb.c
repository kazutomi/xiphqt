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

#include <assert.h>
#include "cclib.h"

//------------------------------------------------------------------------------
//  This assumes 3 byte RGB data with the same component ordering in the source and destination
void ConvertRGBtoRGB(const unsigned char* const pucSource, long lWidth, long lHeight, long lStepIn, long lStrideIn,
    unsigned char* const pucDest, long lStepOut, long lStrideOut)
{
    const unsigned char* pRowIn = pucSource;
    unsigned char* pRowOut = pucDest;

    long y;

    for (y = 0; y < lHeight; ++y)
    {
        const unsigned char* pIn = pRowIn;
        unsigned char* pOut = pRowOut;

        long x;

        for (x = 0; x < lWidth; ++x)
        {
            pOut[0] = pIn[0];
            pOut[1] = pIn[1];
            pOut[2] = pIn[2];

            pIn += lStepIn;
            pOut += lStepOut;
        }

        pRowIn += lStrideIn;
        pRowOut += lStrideOut;
    }

    return;
}
