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


/****************************************************************************
*
*   Module Title :     YUVtofromRGB
*
*   Description  :     YUV/RGB conversion functions
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */


#include <string.h>

#include "pbdll.h"


/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

#define YFACTOR         0.8588235
 
/****************************************************************************
*  Explicit imports
*****************************************************************************
*/

               
/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/
// Multiplication tables for TIMs (scalar version of) RGB -> smpte YVU
int RintoY[ 256];	// * 66
int GintoY[ 256];	// * 129
int BintoY[ 256];	// * 25
int RBintoVU[ 1024]; // * 28
int y_intoY[ 256];	// 149 * (i - 16) clamped to [ 0, 149 * 219]
int v_intoR[ 256]; 	// 204 * (i - 128)
int u_intoB[ 256]; 	// 258 * (i - 128)
int y_intoG[ 256];	// 254 * (i - 16) clamped to [ 0, 254 * 219]
int RintoG[ 256];	// *65
static int * const BintoG = BintoY;	// * 25

// RGB and YUV accelerator structures.
UINT8 CalcUTable[VAL_RANGE * 2];
INT32 CalcRUTable[VAL_RANGE];
UINT8 CalcVTable[VAL_RANGE * 2];
INT32 CalcRVTable[VAL_RANGE];
INT32 InvYScale[VAL_RANGE * 2];
UINT8 DivByTen[VAL_RANGE * 10];
UINT8 DivBy5p87[VAL_RANGE * 14];
INT32 Times2p99[VAL_RANGE];
INT32 Times5p87[VAL_RANGE];
INT32 Times1p14[VAL_RANGE];
INT32 TimesTen[VAL_RANGE];

UINT8 LimitVal_VP31[VAL_RANGE * 3];

/****************************************************************************
*  Forward References
*****************************************************************************
*/  

/****************************************************************************
*  Module Variables.
*****************************************************************************
*/  


/****************************************************************************
 * 
 *  ROUTINE       :     ScalarYUVtoRGB
 *
 *  INPUTS        :     yblock, ublock, vblock
 *                           Blocks of Y U and V data.
 *                      uvoffset
 *                           Offset of UV quadrant
 *                      RGBPtr
 *                           RGB structure to write into.
 *                      ReconBuffer
 *                           Is the YUV source in reconstruction buffer format.
 *
 *  OUTPUTS       :     
 *.           
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Converst one block to RGB
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void ScalarYUVtoRGB ( PB_INSTANCE * pbi,
					  YUV_BUFFER_ENTRY_PTR yblock,		// Y block to be decoded
		              YUV_BUFFER_ENTRY_PTR ublock,		// U block to be used
				      YUV_BUFFER_ENTRY_PTR vblock,		// V block to be used
			          int uvoffset,					    // Offset to UV quadrant to be used
				      BGR_TYPE * RGBPtr,                // RGB bitmap data pointer
                      BOOL ReconBuffer )				// YUV buffer format
{
	INT32 UFactor;
	INT32 VFactor;
    INT32 YVal;
    UINT8 RVal;
	UINT8 BVal;
	INT32 n;
    INT32 RGB_YStep = (pbi->Configuration.VideoFrameWidth * 2);
    INT32 YStep;
    INT32 UVStep;

    UINT32 * InvYScaleTable = (UINT32 *)&InvYScale[0];
    UINT8 * LimitTable = &LimitVal_VP31[VAL_RANGE];

    UINT8 * DivBy5p87Table = &DivBy5p87[VAL_RANGE * 4];

	YUV_BUFFER_ENTRY_PTR YPtr;
	YUV_BUFFER_ENTRY_PTR YPtr2;
	YUV_BUFFER_ENTRY_PTR UPtr;
	YUV_BUFFER_ENTRY_PTR VPtr;

	BGR_TYPE * RGBPtr2 = RGBPtr + pbi->Configuration.VideoFrameWidth;

    // Set up starting values for YUV pointers
    YPtr = yblock;
    UPtr = ublock + uvoffset;
    VPtr = vblock + uvoffset;

    // Set the line step for the Y and UV planes and YPtr2
    if ( ReconBuffer )
    {
        YStep = (pbi->Configuration.YStride * 2);
        UVStep = pbi->Configuration.UVStride;
        YPtr2 = YPtr + pbi->Configuration.YStride;
    }
    else
    {
        YStep = (pbi->Configuration.VideoFrameWidth * 2);
        UVStep = (pbi->Configuration.VideoFrameWidth / 2);
        YPtr2 = YPtr + pbi->Configuration.VideoFrameWidth;
    }

    for ( n = 0; n < BLOCK_HEIGHT_WIDTH/2; n++ )
    {
		// First group of four pixels
		UFactor = CalcRUTable[UPtr[0]];
		VFactor = CalcRVTable[VPtr[0]];

        YVal = InvYScaleTable[YPtr[0]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr[0].Blue	= BVal;
		RGBPtr[0].Red	= RVal;
		RGBPtr[0].Green	= (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr[1]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr[1].Blue	= BVal;
		RGBPtr[1].Red	= RVal;
		RGBPtr[1].Green	= (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr2[0]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr2[0].Blue	= BVal;
		RGBPtr2[0].Red	= RVal;
		RGBPtr2[0].Green = (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr2[1]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr2[1].Blue	= BVal;
		RGBPtr2[1].Red	= RVal;
		RGBPtr2[1].Green = (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

		// Group 2
		UFactor = CalcRUTable[UPtr[1]];
		VFactor = CalcRVTable[VPtr[1]];

        YVal = InvYScaleTable[YPtr[2]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr[2].Blue	= BVal;
		RGBPtr[2].Red	= RVal;
		RGBPtr[2].Green	= (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr[3]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr[3].Blue	= BVal;
		RGBPtr[3].Red	= RVal;
		RGBPtr[3].Green	= (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr2[2]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr2[2].Blue	= BVal;
		RGBPtr2[2].Red	= RVal;
		RGBPtr2[2].Green = (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr2[3]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr2[3].Blue	= BVal;
		RGBPtr2[3].Red	= RVal;
		RGBPtr2[3].Green = (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

		// Group 3
		UFactor = CalcRUTable[UPtr[2]];
		VFactor = CalcRVTable[VPtr[2]];

        YVal = InvYScaleTable[YPtr[4]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr[4].Blue	= BVal;
		RGBPtr[4].Red	= RVal;
		RGBPtr[4].Green	= (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr[5]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr[5].Blue	= BVal;
		RGBPtr[5].Red	= RVal;
		RGBPtr[5].Green	= (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr2[4]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr2[4].Blue	= BVal;
		RGBPtr2[4].Red	= RVal;
		RGBPtr2[4].Green = (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr2[5]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr2[5].Blue	= BVal;
		RGBPtr2[5].Red	= RVal;
		RGBPtr2[5].Green = (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];
		
		// Group 4
		UFactor = CalcRUTable[UPtr[3]];
		VFactor = CalcRVTable[VPtr[3]];

        YVal = InvYScaleTable[YPtr[6]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr[6].Blue	= BVal;
		RGBPtr[6].Red	= RVal;
		RGBPtr[6].Green	= (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr[7]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr[7].Blue	= BVal;
		RGBPtr[7].Red	= RVal;
		RGBPtr[7].Green	= (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr2[6]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr2[6].Blue	= BVal;
		RGBPtr2[6].Red	= RVal;
		RGBPtr2[6].Green = (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];

        YVal = InvYScaleTable[YPtr2[7]];
        RVal = LimitTable[ YVal + VFactor ];
		BVal = LimitTable[ YVal + UFactor ];
		RGBPtr2[7].Blue	= BVal;
		RGBPtr2[7].Red	= RVal;
		RGBPtr2[7].Green = (UINT8)DivBy5p87Table[ TimesTen[YVal] - Times2p99[RVal] - Times1p14[BVal] ];
	
		// Increment the various pointers
 		YPtr += YStep;
		YPtr2 += YStep;
		UPtr += UVStep;
		VPtr += UVStep;
		RGBPtr += RGB_YStep;
		RGBPtr2 += RGB_YStep;
	}
}

/****************************************************************************
 * 
 *  ROUTINE       :     CopyYUVtoBmp
 *
 *  INPUTS        :     YUV_BUFFER_ENTRY_PTR  YUVPtr
 *                      Pointers to the raw yuv planes.
 *
 *                      BOOL ConvertAll
 *                      Should all blocks be converted or only those marked for update.
 *                      
 *                      BOOL ReconBuffer
 *                      Is the input buffer fromatted as a reconstruction buffer or is 
 *                      it a simple YUV buffer.
 *
 *  OUTPUTS       :     UINT8 * BmpPtr, 
 *                      Pointer to buffer to contain the BGR data
 *.           
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Copies data in a YUV sturcture into a bitmaps data part. 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void CopyYUVtoBmp( PB_INSTANCE * pbi, YUV_BUFFER_ENTRY_PTR YUVPtr, UINT8 * BmpPtr, BOOL ConvertAll, BOOL ReconBuffer )
{
    unsigned x, y;
	//static UINT8 YDisp[MAX_FRAGMENTS], *UDisp, *VDisp;

    BOOL UVRow = TRUE;   
    BOOL UVColumn = TRUE;   

	int YFrag;								// fragment numbers being processed

	int Quadrant0;
	int Quadrant1;
	int Quadrant2;
	int Quadrant3;
	UINT32 YDispOffset = 0;

    // Set up UV quadrant offsets
	Quadrant0 = 0;
	Quadrant1 = 4;
    if ( ReconBuffer )
	    Quadrant2 = pbi->Configuration.UVStride * 4;         // (UV plane width * 4)
    else
	    Quadrant2 = pbi->Configuration.VideoFrameWidth * 2;  // (UV plane width * 4)
	Quadrant3 = Quadrant2 + 4;

	/*	Now scan Y list and update any blocks that need it						*/
	/*	This will update any Y blocks in need as well as any UV blocks marked	*/
	/*	above.																	*/

	YFrag = 0;
	for ( y = 0; y < pbi->VFragments; y++ )
	{	
		for ( x = 0; x < pbi->HFragments; x++ )
		{	
			if ( pbi->display_fragments[YFrag] 
                || pbi->display_fragments[pbi->YPlaneFragments + YFrag / 4] 
                || pbi->display_fragments[pbi->YPlaneFragments + pbi->UVPlaneFragments +YFrag / 4] )						/* Need to update Y */
			{	
				int ublock = pbi->YPlaneFragments + (( y >> 1 ) * ( pbi->HFragments >> 1 )) + ( x >> 1 );
				int vblock = ublock + pbi->UVPlaneFragments;
				int uvoffset;
				
				if ( x & 1 ) uvoffset = y & 1 ? Quadrant3: Quadrant1;
				else		 uvoffset = y & 1 ? Quadrant2: Quadrant0;

                if ( ReconBuffer )
                {
			        pbi->YUVtoRGB (	pbi, YUVPtr + ReconGetFragIndex ( pbi->recon_pixel_index_table,  YFrag ),
						        YUVPtr + ReconGetFragIndex ( pbi->recon_pixel_index_table,  ublock ),
						        YUVPtr + ReconGetFragIndex ( pbi->recon_pixel_index_table,  vblock ),
						        uvoffset,
						        (BGR_TYPE *) BmpPtr + GetFragIndex ( pbi->pixel_index_table,  YFrag ),
                                ReconBuffer );
                }
                else
                {
				    pbi->YUVtoRGB (	pbi, YUVPtr + GetFragIndex ( pbi->pixel_index_table,  YFrag ),
							    YUVPtr + GetFragIndex ( pbi->pixel_index_table,  ublock ),
							    YUVPtr + GetFragIndex ( pbi->pixel_index_table,  vblock ),
							    uvoffset,
							    (BGR_TYPE *) BmpPtr + GetFragIndex ( pbi->pixel_index_table,  YFrag ),
                                ReconBuffer );
                }
			}
			YFrag++;
		}
	}
}



/****************************************************************************
 * 
 *  ROUTINE       :     SetupRgbYuvAccelerators
 *
 *  INPUTS        :     Nonex.
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Pre-calculates numbers for acceleration of RGB<->YUV
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void SetupRgbYuvAccelerators()
{
    #define Clamp255( x)	(unsigned char) ( (x) < 0 ? 0 : ( (x) <= 255 ? (x) : 255 ) )

	INT32 i;

	LimitVal_VP31[0] = 0;
	for( i = 0; i < VAL_RANGE * 3; i++) {
		int x = i - VAL_RANGE;
		LimitVal_VP31[ i] = Clamp255( x);
	}

	// V range raw is +/- 179.
	// Correct to approx 16-240 by dividing by 1.596 and adding 128.

	// U range raw is +/- 226.
	// Correct to approx 16-240 by dividing by 2.018 and adding 128.

	for( i = -VAL_RANGE; i < VAL_RANGE; i++) 
    {
		INT32 x;
        if ( i < 0 )
        {
            x = (INT32)((i / 1.596) - 0.5) + 128;
		    CalcVTable[ i + VAL_RANGE] = Clamp255(x);
		    CalcUTable[ i + VAL_RANGE] = (signed char)((i / 2.018) - 0.5) + 128;
        }
        else
        {
            x = (INT32)((i / 1.596) + 0.5) + 128;
		    CalcVTable[ i + VAL_RANGE] = Clamp255( x);
		    CalcUTable[ i + VAL_RANGE] = (signed char)((i / 2.018) + 0.5) + 128;
        }
	}

	for( i = 0; i < VAL_RANGE * 10; i++)
		DivByTen[i] = (UINT8)((i + 5) / 10);

	for( i = 0; i < (VAL_RANGE * 14); i++) 
    {
		INT32 x = (i - (VAL_RANGE * 4));

        if ( x < 0 )
            x = (INT32)((x / 5.87) - 0.5);
        else
            x = (INT32)((x / 5.87) + 0.5);

		DivBy5p87[i] = Clamp255( x);
	}

	for( i = 0; i < VAL_RANGE; i++) 
    {
		InvYScale[i] = Clamp255( (i - 16) / YFACTOR);
        if ( i < 128 )
        {
		    CalcRVTable[i] = (INT32) ( ((i - 128) * 1.596) - 0.5 );
		    CalcRUTable[i] = (INT32) ( ((i - 128) * 2.018) - 0.5 );
        }
        else
        {
		    CalcRVTable[i] = (INT32) ( ((i - 128) * 1.596) + 0.5 );
		    CalcRUTable[i] = (INT32) ( ((i - 128) * 2.018) + 0.5);
        }
		Times2p99[i] = (INT32)((i * 2.99) + 0.5);
		Times5p87[i] = (INT32)((i * 5.87) + 0.5);
        Times1p14[i] = (INT32)((i * 1.14) + 0.5);
		TimesTen[i] = (INT32)(i * 10);
	}
    #undef Clamp255

}

/****************************************************************************
 * 
 *  ROUTINE       :     ConvertBmpToYUV
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Loads a frame from disk and converts it to YUV.
 *
 *  SPECIAL NOTES :     Matrix assumed to be:
 *
 *                      Y = .587G + .299R + .114B
 *                      U =  B - Y
 *                      V =  R - Y 
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void ConvertBmpToYUV( PB_INSTANCE *pbi, UINT8 * BmpDataPtr, UINT8 * YuvBufferPtr )
{   
    UINT32 i, j;

    YUV_ENTRY   YVal;       /* Workspace for calculating YU and V */

    YUV_BUFFER_ENTRY_PTR UPtr;
    YUV_BUFFER_ENTRY_PTR VPtr;

    YUV_BUFFER_ENTRY_PTR YPtr;
    YUV_BUFFER_ENTRY_PTR YPtrEven;
    YUV_BUFFER_ENTRY_PTR YPtrOdd;

    BGR_TYPE * RGBPtr;  
    BGR_TYPE * RGBPtrEven;  
    BGR_TYPE * RGBPtrOdd;  

    UINT8 * VLookupTable = &CalcVTable[256];
    UINT8 * ULookupTable = &CalcUTable[256];

    /* Set up pointers into the YUV data structure. */
    YPtr = YuvBufferPtr;
    UPtr = &YuvBufferPtr[(pbi->Configuration.VideoFrameHeight * pbi->Configuration.VideoFrameWidth)];
    VPtr = &YuvBufferPtr[((pbi->Configuration.VideoFrameHeight * pbi->Configuration.VideoFrameWidth)*5)/4];

    // Set up the pointers the the RGB data buffer (actually BGR). */
    RGBPtr = (BGR_TYPE *)BmpDataPtr;

    /* For the moment the U and V will be taken from the first pixel in the group of four. */
    for ( i = 0; i < pbi->Configuration.VideoFrameHeight; i += 2 )
    {
        RGBPtrEven = (BGR_TYPE *)RGBPtr;
        RGBPtrOdd = RGBPtrEven + pbi->Configuration.VideoFrameWidth;

        YPtrEven = YPtr;
        YPtrOdd = YPtrEven + pbi->Configuration.VideoFrameWidth;

        for (j = 0; j < pbi->Configuration.VideoFrameWidth; j += 2 )
        {
            int R,B,Y;

            // compute Y. 
            YVal =  Times1p14[RGBPtrEven->Blue] + Times5p87[RGBPtrEven->Green] + Times2p99[RGBPtrEven->Red];
            YPtrEven[0] = (UINT8)DivByTen[YVal];
            Y = YPtrEven[0];
            R = RGBPtrEven->Red;
            B = RGBPtrEven->Blue;
            YPtrEven[0] = (UINT8)(((double)(YPtrEven[0]) * YFACTOR) + 16);  // Scale the Y value to the approximate range 16-235.
            RGBPtrEven++;

            YVal =  Times1p14[RGBPtrEven->Blue] + Times5p87[RGBPtrEven->Green] + Times2p99[RGBPtrEven->Red];
            YPtrEven[1] = (UINT8)DivByTen[YVal];
            Y += YPtrEven[1];
            R += RGBPtrEven->Red;
            B += RGBPtrEven->Blue;
            YPtrEven[1] = (UINT8)(((double)(YPtrEven[1]) * YFACTOR) + 16);  // Scale the Y value to the approximate range 16-235.
            RGBPtrEven++;

            YVal =  Times1p14[RGBPtrOdd->Blue] + Times5p87[RGBPtrOdd->Green] + Times2p99[RGBPtrOdd->Red];
            YPtrOdd[0] = (UINT8)DivByTen[YVal];
            Y += YPtrOdd[0];
            R += RGBPtrOdd->Red;
            B += RGBPtrOdd->Blue;
            YPtrOdd[0] = (UINT8)(((double)(YPtrOdd[0]) * YFACTOR) + 16);  // Scale the Y value to the approximate range 16-235.
            RGBPtrOdd++;

            YVal =  Times1p14[RGBPtrOdd->Blue] + Times5p87[RGBPtrOdd->Green] + Times2p99[RGBPtrOdd->Red];
            YPtrOdd[1] = (UINT8)DivByTen[YVal];
            Y += YPtrOdd[1];
            R += RGBPtrOdd->Red;
            B += RGBPtrOdd->Blue;
            YPtrOdd[1] = (UINT8)(((double)(YPtrOdd[1]) * YFACTOR) + 16);  // Scale the Y value to the approximate range 16-235.
            RGBPtrOdd++;
            
            // Calculate U and V using average Y,R,B for the four pixels
            *UPtr = ULookupTable[(2 + B - Y) >> 2];   // +2 is for rounding      
            *VPtr = VLookupTable[(2 + R - Y) >> 2];   // +2 is for rounding
            UPtr++;
            VPtr++;

            // Step on Y pointers on
            YPtrEven += 2;
            YPtrOdd += 2;
        }

        // Step on to start of next pair of rows. 
        RGBPtr += (pbi->Configuration.VideoFrameWidth * 2);
        YPtr += (pbi->Configuration.VideoFrameWidth * 2);
    }
}

