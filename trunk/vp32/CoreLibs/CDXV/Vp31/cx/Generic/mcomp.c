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
*   Module Title :     MComp.C
*
*   Description  :     Video CODEC: motion compensation module .
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include <math.h>

#include "compdll.h"
#include "mcomp.h"

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

#define MAX_ERROR       100000

/****************************************************************************
*  Exported data structures
*****************************************************************************
*/

/****************************************************************************
*  Forward Reference
*****************************************************************************
*/



/*
// Machine / platform specific function pointers.
// Function pointers for machine / platform specific functions
INT32 (*GetSAD)(UINT8 *, UINT8 *, UINT32, INT32, INT32 ) = GetSumAbsDiffs;            
INT32 (*GetNextSAD)(UINT8 *, UINT8 *, UINT32, INT32, INT32 ) = GetNextSumAbsDiffs;            
INT32 (*GetSadHalfPixel)(UINT8 *, UINT8 *, UINT8 *, UINT32, INT32, INT32  ) = GetHalfPixelSumAbsDiffs;
INT32 (*GetInterError)( UINT8 *, UINT8 *,  UINT8 *, UINT32 ) = GetInterErr;;
*/
// Motion compensation related variables

UINT32  MvMaxExtent = MAX_MV_EXTENT;
INT32 * XX_LUT=NULL;
INT32 SquaredErrorTable[511];
INT32 * AbsX_LUT = NULL;
INT32 AbsXTable[511];
/****************************************************************************
*  Exported Functions
*****************************************************************************
*/              

/****************************************************************************
*  Module Statics
*****************************************************************************
*/              

UINT32 GetBMVExhaustiveSearch( CP_INSTANCE *cpi, UINT8 * RefFramePtr, UINT32 FragIndex, UINT32 PixelsPerLine, MOTION_VECTOR *MV );



/* Initialises motion compentsation. */
void InitMotionCompensation ( CP_INSTANCE *cpi )
{
	int i;
	int SearchSite=0;
	int Len;
    int LineStepY = (INT32)cpi->pb.Configuration.YStride;
	
    Len=((MvMaxExtent/2)+1)/2;


	// How many search stages are there.
	cpi->MVSearchSteps = 0;

    // Set up offsets arrays used in half pixel correction.
    cpi->HalfPixelRef2Offset[0] = -LineStepY - 1;
    cpi->HalfPixelRef2Offset[1] = -LineStepY;
    cpi->HalfPixelRef2Offset[2] = -LineStepY + 1;
    cpi->HalfPixelRef2Offset[3] = - 1;
    cpi->HalfPixelRef2Offset[4] = 0;
    cpi->HalfPixelRef2Offset[5] = 1;
    cpi->HalfPixelRef2Offset[6] = LineStepY - 1;
    cpi->HalfPixelRef2Offset[7] = LineStepY;
    cpi->HalfPixelRef2Offset[8] = LineStepY + 1;

    cpi->HalfPixelXOffset[0] = -1;
    cpi->HalfPixelXOffset[1] = 0;
    cpi->HalfPixelXOffset[2] = 1;
    cpi->HalfPixelXOffset[3] = -1;
    cpi->HalfPixelXOffset[4] = 0;
    cpi->HalfPixelXOffset[5] = 1;
    cpi->HalfPixelXOffset[6] = -1;
    cpi->HalfPixelXOffset[7] = 0;
    cpi->HalfPixelXOffset[8] = 1;

    cpi->HalfPixelYOffset[0] = -1;
    cpi->HalfPixelYOffset[1] = -1;
    cpi->HalfPixelYOffset[2] = -1;
    cpi->HalfPixelYOffset[3] = 0;
    cpi->HalfPixelYOffset[4] = 0;
    cpi->HalfPixelYOffset[5] = 0;
    cpi->HalfPixelYOffset[6] = 1;
    cpi->HalfPixelYOffset[7] = 1;
    cpi->HalfPixelYOffset[8] = 1;


	// Generate offsets for 8 search sites per step.
	while ( Len>0 )
	{
		// Another step.
		cpi->MVSearchSteps += 1;

		// Compute offsets for search sites.
		cpi->MVOffsetX[SearchSite] = -Len;
		cpi->MVOffsetY[SearchSite++] = -Len;
		cpi->MVOffsetX[SearchSite] = 0;
		cpi->MVOffsetY[SearchSite++] = -Len;
		cpi->MVOffsetX[SearchSite] = Len;
		cpi->MVOffsetY[SearchSite++] = -Len;
		cpi->MVOffsetX[SearchSite] = -Len;
		cpi->MVOffsetY[SearchSite++] = 0;
		cpi->MVOffsetX[SearchSite] = Len;
		cpi->MVOffsetY[SearchSite++] = 0;
		cpi->MVOffsetX[SearchSite] = -Len;
		cpi->MVOffsetY[SearchSite++] = Len;
		cpi->MVOffsetX[SearchSite] = 0;
		cpi->MVOffsetY[SearchSite++] = Len;
		cpi->MVOffsetX[SearchSite] = Len;
		cpi->MVOffsetY[SearchSite++] = Len;

		// Contract.
		Len /= 2;
	}

	// Compute pixel index offsets.
	for ( i=SearchSite-1; i>=0; i-- )
	{
		cpi->MVPixelOffsetY[i] = (cpi->MVOffsetY[i]*LineStepY) + cpi->MVOffsetX[i];
	}
}


/****************************************************************************
 * 
 *  ROUTINE       :     GetInterErr
 *
 *  INPUTS        :     UINT8 * NewDataPtr	(New Data)
 *						UINT8 * RefDataPtr1
 *                      UINT8 * RefDataPtr2
 *						UINT32	PixelsPerLine
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Variance / error
 *
 *  FUNCTION      :     Calculates a difference error score for two blocks
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetInterErr( UINT8 * NewDataPtr, UINT8 * RefDataPtr1,  UINT8 * RefDataPtr2, UINT32 PixelsPerLine )
{
       
	UINT32	i;
	INT32	XSum=0;
	INT32	XXSum=0;
	INT32	DiffVal;
    INT32   AbsRefOffset = abs((int)(RefDataPtr1 - RefDataPtr2));
    
    // Mode of interpolation chosen based upon on the offset of the second reference pointer 
    if ( AbsRefOffset == 0 )
    {
	    for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ )
        {
            DiffVal = ((int)NewDataPtr[0]) - (int)RefDataPtr1[0];
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[1]) - (int)RefDataPtr1[1];
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[2]) - (int)RefDataPtr1[2];
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[3]) - (int)RefDataPtr1[3];
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[4]) - (int)RefDataPtr1[4];
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[5]) - (int)RefDataPtr1[5];
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[6]) - (int)RefDataPtr1[6];
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[7]) - (int)RefDataPtr1[7];
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

		    // Step to next row of block.
		    NewDataPtr += PixelsPerLine;
			RefDataPtr1 += STRIDE_EXTRA + PixelsPerLine;
	    }
    }
    
    // Simple two reference interpolation
    else 
    {
	    for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ )
        {
            DiffVal = ((int)NewDataPtr[0]) - (((int)RefDataPtr1[0] + (int)RefDataPtr2[0]) / 2);
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[1]) - (((int)RefDataPtr1[1] + (int)RefDataPtr2[1]) / 2);
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[2]) - (((int)RefDataPtr1[2] + (int)RefDataPtr2[2]) / 2);
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[3]) - (((int)RefDataPtr1[3] + (int)RefDataPtr2[3]) / 2);
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[4]) - (((int)RefDataPtr1[4] + (int)RefDataPtr2[4]) / 2);
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[5]) - (((int)RefDataPtr1[5] + (int)RefDataPtr2[5]) / 2);
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[6]) - (((int)RefDataPtr1[6] + (int)RefDataPtr2[6]) / 2);
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

            DiffVal = ((int)NewDataPtr[7]) - (((int)RefDataPtr1[7] + (int)RefDataPtr2[7]) / 2);
		    XSum += DiffVal;
		    XXSum += XX_LUT[DiffVal];

		    // Step to next row of block.
		    NewDataPtr += PixelsPerLine;
            RefDataPtr1 += STRIDE_EXTRA+PixelsPerLine;
            RefDataPtr2 += STRIDE_EXTRA+PixelsPerLine;
	    }
    }

	// Compute and return population variance as mis-match metric.
	return (( (XXSum<<6) - XSum*XSum ));
}

/****************************************************************************
 * 
 *  ROUTINE       :     GetSumAbsDiffs
 *
 *  INPUTS        :     UINT8 * NewDataPtr	(New Data)
 *						UINT8 * RefDataPtr
 *						UINT32	PixelsPerLine
 *                      INT32   ErrorSoFar, 
 *						INT32   BestSoFar 
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Sum absolute differences
 *
 *  FUNCTION      :     Calculates the sum of the absolute differences.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetSumAbsDiffs( UINT8 * NewDataPtr, UINT8  * RefDataPtr, 
			 		  UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  )
{
	UINT32	i;
	UINT32	DiffVal = ErrorSoFar;
	
	// Decide on standard or MMX implementation
	for ( i=0; i < BLOCK_HEIGHT_WIDTH; i++ )
	{
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[0]) - ((int)RefDataPtr[0]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[1]) - ((int)RefDataPtr[1]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[2]) - ((int)RefDataPtr[2]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[3]) - ((int)RefDataPtr[3]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[4]) - ((int)RefDataPtr[4]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[5]) - ((int)RefDataPtr[5]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[6]) - ((int)RefDataPtr[6]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[7]) - ((int)RefDataPtr[7]) ];

		// Step to next row of block.
		NewDataPtr += PixelsPerLine;
		RefDataPtr += STRIDE_EXTRA+PixelsPerLine;
	}

	return DiffVal;
    
}

/****************************************************************************
 * 
 *  ROUTINE       :     GetNextSumAbsDiffs
 *
 *  INPUTS        :     UINT8 * NewDataPtr	(New Data)
 *						UINT8 * RefDataPtr
 *						UINT32	PixelsPerLine
 *                      INT32   ErrorSoFar, 
 *						INT32   BestSoFar 
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Sum absolute differences with breakout clause
 *
 *  FUNCTION      :     Calculates the sum of the absolute differences.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetNextSumAbsDiffs( UINT8 * NewDataPtr, UINT8 * RefDataPtr, 
						  UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  )
{
	UINT32	i;
	UINT32	DiffVal = ErrorSoFar;

	for ( i=0; i < BLOCK_HEIGHT_WIDTH; i++ )
	{
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[0]) - ((int)RefDataPtr[0]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[1]) - ((int)RefDataPtr[1]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[2]) - ((int)RefDataPtr[2]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[3]) - ((int)RefDataPtr[3]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[4]) - ((int)RefDataPtr[4]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[5]) - ((int)RefDataPtr[5]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[6]) - ((int)RefDataPtr[6]) ];
		DiffVal += AbsX_LUT[ ((int)NewDataPtr[7]) - ((int)RefDataPtr[7]) ];

		if ( DiffVal > BestSoFar )
			break;

		// Step to next row of block.
		NewDataPtr += PixelsPerLine;
		RefDataPtr += STRIDE_EXTRA+PixelsPerLine;
	}

	return DiffVal;
    
}

/****************************************************************************
 * 
 *  ROUTINE       :     GetHalfPixelSumAbsDiffs
 *
 *  INPUTS        :     UINT8 * SrcData	(New Data)
 *						UINT8 * RefDataPtr1
 *						UINT8 * RefDataPtr2
 *						UINT32	PixelsPerLine
 *                      INT32   ErrorSoFar 
 *						INT32   BestSoFar 
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Sum of absolute differences at 1/2 pixel accuracy.
 *
 *  FUNCTION      :     Calculates the sum of the absolute differences against
 *                      half pixel interpolated references.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetHalfPixelSumAbsDiffs( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
							   UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  )
{
       
	UINT32	i;
	UINT32	DiffVal = ErrorSoFar;
    INT32   RefOffset = (int)(RefDataPtr1 - RefDataPtr2);
    UINT32  RefPixelsPerLine = PixelsPerLine + STRIDE_EXTRA;

    if ( RefOffset == 0 )
    {
		// Simple case as for non 0.5 pixel
		DiffVal += GetSumAbsDiffs( SrcData, RefDataPtr1, PixelsPerLine, ErrorSoFar, BestSoFar );
    }
    else 
    {
	    for ( i=0; i < BLOCK_HEIGHT_WIDTH; i++ )
        {
            DiffVal += AbsX_LUT[ ((int)SrcData[0]) - (((int)RefDataPtr1[0] + (int)RefDataPtr2[0]) / 2) ];
            DiffVal += AbsX_LUT[ ((int)SrcData[1]) - (((int)RefDataPtr1[1] + (int)RefDataPtr2[1]) / 2) ];
            DiffVal += AbsX_LUT[ ((int)SrcData[2]) - (((int)RefDataPtr1[2] + (int)RefDataPtr2[2]) / 2) ];
            DiffVal += AbsX_LUT[ ((int)SrcData[3]) - (((int)RefDataPtr1[3] + (int)RefDataPtr2[3]) / 2) ];
            DiffVal += AbsX_LUT[ ((int)SrcData[4]) - (((int)RefDataPtr1[4] + (int)RefDataPtr2[4]) / 2) ];
            DiffVal += AbsX_LUT[ ((int)SrcData[5]) - (((int)RefDataPtr1[5] + (int)RefDataPtr2[5]) / 2) ];
            DiffVal += AbsX_LUT[ ((int)SrcData[6]) - (((int)RefDataPtr1[6] + (int)RefDataPtr2[6]) / 2) ];
            DiffVal += AbsX_LUT[ ((int)SrcData[7]) - (((int)RefDataPtr1[7] + (int)RefDataPtr2[7]) / 2) ];

			if ( DiffVal > BestSoFar )
				break;

            // Step to next row of block.
		    SrcData += PixelsPerLine;
            RefDataPtr1 += RefPixelsPerLine;
            RefDataPtr2 += RefPixelsPerLine;
	    }
    }

	return DiffVal;
    
}
/****************************************************************************
 * 
 *  ROUTINE       :     GetIntraError
 *
 *  INPUTS        :     UINT8 * DataPtr	(New Data)
 *                      UINT32  Horizontal and vertical scaling factors
 *						UINT32	PixelsPerLine
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Intra frame variance
 *
 *  FUNCTION      :     Calculates a variance score for the block
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetIntraError( UINT8 * DataPtr, UINT32 PixelsPerLine )
{
	UINT32	i;
	UINT32	XSum=0;
	UINT32	XXSum=0;
	UINT8	*DiffPtr;
	//INT32	Variance=0.0;
    
	// Loop expanded out for speed. 
	DiffPtr = DataPtr;
 
	for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ )
    {
	
		
		// Examine alternate pixel locations.
		XSum += DiffPtr[0];
		XXSum += XX_LUT[DiffPtr[0]];
		XSum += DiffPtr[1];
		XXSum += XX_LUT[DiffPtr[1]];
		XSum += DiffPtr[2];
		XXSum += XX_LUT[DiffPtr[2]];
		XSum += DiffPtr[3];
		XXSum += XX_LUT[DiffPtr[3]];
		XSum += DiffPtr[4];
		XXSum += XX_LUT[DiffPtr[4]];
		XSum += DiffPtr[5];
		XXSum += XX_LUT[DiffPtr[5]];
		XSum += DiffPtr[6];
		XXSum += XX_LUT[DiffPtr[6]];
		XSum += DiffPtr[7];
		XXSum += XX_LUT[DiffPtr[7]];

		// Step to next row of block.
		DiffPtr += PixelsPerLine;
	}

	// Compute population variance as mis-match metric.
    return (( (XXSum<<6) - XSum*XSum ) );
}
/****************************************************************************
 * 
 *  ROUTINE       :     GetMBIntraError
 *
 *  INPUTS        :     UINT32  Fragment offset of first block in MB
 *						UINT32	PixelsPerLine
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Intra frame variance
 *
 *  FUNCTION      :     Calculates a variance score for an intra macro block
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetMBIntraError( CP_INSTANCE *cpi, UINT32 FragIndex, UINT32 PixelsPerLine )
{
	UINT32	LocalFragIndex = FragIndex;
    UINT32  IntraError = 0;
    UINT32  TmpError = 0;
    
    // Add together the intra errors for those blocks in the macro block that are coded (Y only)
    if ( cpi->pb.display_fragments[LocalFragIndex] )
    {
        IntraError += cpi->GetIntraError( &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,LocalFragIndex)], PixelsPerLine );
    }

    LocalFragIndex ++;
    if ( cpi->pb.display_fragments[LocalFragIndex] )
    {
        IntraError += cpi->GetIntraError( &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,LocalFragIndex)], PixelsPerLine );
    }

    LocalFragIndex = FragIndex + cpi->pb.HFragments;
    if ( cpi->pb.display_fragments[LocalFragIndex] )
    {
        IntraError += cpi->GetIntraError( &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,LocalFragIndex)], PixelsPerLine );
    }

    LocalFragIndex ++;
    if ( cpi->pb.display_fragments[LocalFragIndex] )
    {
        IntraError += cpi->GetIntraError( &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,LocalFragIndex)], PixelsPerLine );
    }
    //return IntraError * 0.25;
    return IntraError;
}


/****************************************************************************
 * 
 *  ROUTINE       :     GetMBInterError
 *
 *  INPUTS        :     UINT8 * Source and Reference frame pointers
 *                      UINT32  Fragment offset of first block in MB
 *                      INT32   LastXMV
 *                      INT32   LastYMV
 *						UINT32	PixelsPerLine
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Inter frame variance
 *
 *  FUNCTION      :     Calculates a variance score for an inter MB with motion vectors
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetMBInterError( CP_INSTANCE *cpi, UINT8 * SrcPtr, UINT8 * RefPtr, UINT32 FragIndex, INT32 LastXMV, INT32 LastYMV, UINT32 PixelsPerLine )
{
    UINT32  RefPixelsPerLine = cpi->pb.Configuration.YStride;
	UINT32	LocalFragIndex = FragIndex;
    INT32   PixelIndex;
    INT32   RefPixelIndex;
    INT32   RefPixelOffset;
	INT32   RefPtr2Offset;

    UINT32  TmpError = 0;
    UINT32  InterError = 0;

	UINT8 * SrcPtr1;
	UINT8 * RefPtr1;

    // Work out pixel offset into source buffer.
    PixelIndex = GetFragIndex(cpi->pb.pixel_index_table,LocalFragIndex);

    // Work out the pixel offset in reference buffer for the default motion vector
    RefPixelIndex = GetFragIndex(cpi->pb.recon_pixel_index_table,LocalFragIndex);
    RefPixelOffset = ((LastYMV/2) * RefPixelsPerLine) + (LastXMV/2);

	// Work out the second reference pointer offset.
	RefPtr2Offset = 0;
    if ( LastXMV % 2 )
    {
        if ( LastXMV > 0 )
            RefPtr2Offset += 1;
        else
            RefPtr2Offset -= 1;
    }
    if ( LastYMV % 2 )
    {
        if ( LastYMV > 0 )
           RefPtr2Offset += RefPixelsPerLine;
        else
           RefPtr2Offset -= RefPixelsPerLine;
    }

    // Add together the errors for those blocks in the macro block that are coded (Y only)
    if ( cpi->pb.display_fragments[LocalFragIndex] )
    {
		SrcPtr1 = &SrcPtr[PixelIndex];
		RefPtr1 = &RefPtr[RefPixelIndex + RefPixelOffset];
        InterError += cpi->GetInterError( SrcPtr1, RefPtr1, &RefPtr1[RefPtr2Offset], PixelsPerLine );
    }

    LocalFragIndex++;
    if ( cpi->pb.display_fragments[LocalFragIndex] )
    {
	    PixelIndex = GetFragIndex(cpi->pb.pixel_index_table,LocalFragIndex);
        RefPixelIndex = GetFragIndex(cpi->pb.recon_pixel_index_table,LocalFragIndex);
		SrcPtr1 = &SrcPtr[PixelIndex];
		RefPtr1 = &RefPtr[RefPixelIndex + RefPixelOffset];
        InterError += cpi->GetInterError( SrcPtr1, RefPtr1, &RefPtr1[RefPtr2Offset], PixelsPerLine );

    }

    LocalFragIndex = FragIndex + cpi->pb.HFragments;
    if ( cpi->pb.display_fragments[LocalFragIndex] )
    {
	    PixelIndex = GetFragIndex(cpi->pb.pixel_index_table,LocalFragIndex);
        RefPixelIndex = GetFragIndex(cpi->pb.recon_pixel_index_table,LocalFragIndex);
		SrcPtr1 = &SrcPtr[PixelIndex];
		RefPtr1 = &RefPtr[RefPixelIndex + RefPixelOffset];
        InterError += cpi->GetInterError( SrcPtr1, RefPtr1, &RefPtr1[RefPtr2Offset], PixelsPerLine );
    }

    LocalFragIndex++;
    if ( cpi->pb.display_fragments[LocalFragIndex] )
    {
	    PixelIndex = GetFragIndex(cpi->pb.pixel_index_table,LocalFragIndex);
        RefPixelIndex = GetFragIndex(cpi->pb.recon_pixel_index_table,LocalFragIndex);
		SrcPtr1 = &SrcPtr[PixelIndex];
		RefPtr1 = &RefPtr[RefPixelIndex + RefPixelOffset];
        InterError += cpi->GetInterError( SrcPtr1, RefPtr1, &RefPtr1[RefPtr2Offset], PixelsPerLine );
    }
    //return InterError * 0.25;
    return InterError;
}
/****************************************************************************
 * 
 *  ROUTINE       :     GetMBMVInterError
 *
 *  INPUTS        :     UINT8   *RefFramePtr (reference fram for search)
 *                      UINT32  Fragment offset of first block in MB
 *						UINT32	PixelsPerLine
 *                      INT32   *MVPixelOffset
 *                      MOTION_VECTOR *MV
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Inter frame variance
 *
 *  FUNCTION      :     Calculates a MV using a heirachical search.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetMBMVInterError( CP_INSTANCE *cpi, UINT8 * RefFramePtr, UINT32 FragIndex, UINT32 PixelsPerLine, INT32 *MVPixelOffset, MOTION_VECTOR *MV )
{
    UINT32	Error = 0;
    UINT32	MinError;
    UINT32  InterMVError = 0;
    
	INT32	i;
	INT32	x=0, y=0;
	INT32	step;
	INT32	SearchSite=0;
	
 	UINT8	*SrcPtr[4] = {NULL,NULL,NULL,NULL};
    UINT8	*RefPtr=NULL;
	UINT8	*CandidateBlockPtr=NULL;
	UINT8	*BestBlockPtr=NULL;

    UINT32  RefRow2Offset = cpi->pb.Configuration.YStride * 8;

    BOOL    MBlockDispFrags[4];

    // Half pixel variables
    INT32   HalfPixelError;
    INT32   BestHalfPixelError;
    UINT8   BestHalfOffset;
    UINT8 * RefDataPtr1;
    UINT8 * RefDataPtr2;

    // Note which of the four blocks in the macro block are to be included in the search.
    MBlockDispFrags[0] = cpi->pb.display_fragments[FragIndex];
    MBlockDispFrags[1] = cpi->pb.display_fragments[FragIndex + 1];
    MBlockDispFrags[2] = cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments];
    MBlockDispFrags[3] = cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments + 1];

    // Set up the source pointers for the four source blocks. 
    SrcPtr[0] = &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,FragIndex)];
    SrcPtr[1] = SrcPtr[0] + 8;
    SrcPtr[2] = SrcPtr[0] + (PixelsPerLine * 8);
    SrcPtr[3] = SrcPtr[2] + 8;

    // Set starting reference point for search.
    RefPtr = &RefFramePtr[GetFragIndex(cpi->pb.recon_pixel_index_table,FragIndex)];

    // Check the 0,0 candidate.
    if ( MBlockDispFrags[0] )
    {
        Error = cpi->GetSAD( SrcPtr[0], RefPtr, 
			            PixelsPerLine, Error, HUGE_ERROR );
    }
    if ( MBlockDispFrags[1] )
    {
        Error = cpi->GetSAD( SrcPtr[1], RefPtr + 8, 
						PixelsPerLine, Error, HUGE_ERROR );         
    }
    if ( MBlockDispFrags[2] )
    {
        Error = cpi->GetSAD( SrcPtr[2], RefPtr + RefRow2Offset, 
			            PixelsPerLine, Error, HUGE_ERROR );        
    }
    if ( MBlockDispFrags[3] )
    {
        Error = cpi->GetSAD( SrcPtr[3], RefPtr + RefRow2Offset + 8, 
			            PixelsPerLine, Error, HUGE_ERROR );
    }

	// Set starting values to results of 0, 0 vector.
	MinError = Error;
	BestBlockPtr = RefPtr;
	x = 0;
	y = 0;
	MV->x = 0;
	MV->y = 0;

    // Proceed through N-steps.
	for (  step=0; step<cpi->MVSearchSteps; step++ )
	{
		// Search the 8-neighbours at distance pertinent to current step.
		for ( i=0; i<8; i++ )
		{
            // Set pointer to next candidate matching block.
			CandidateBlockPtr = RefPtr + MVPixelOffset[SearchSite];

            // Reset error
            Error = 0;

			// Get the score for the current offset
            if ( MBlockDispFrags[0] )
            {
                Error = cpi->GetSAD( SrcPtr[0], CandidateBlockPtr, 
								PixelsPerLine, Error, MinError );
            }

            if ( MBlockDispFrags[1] && (Error < MinError) )
            {
                Error = cpi->GetNextSAD( SrcPtr[1], CandidateBlockPtr + 8, 
									PixelsPerLine, Error, MinError );         
            }

            if ( MBlockDispFrags[2] && (Error < MinError) )
            {
                Error = cpi->GetNextSAD( SrcPtr[2], CandidateBlockPtr + RefRow2Offset, 
									PixelsPerLine, Error, MinError );        
            }

            if ( MBlockDispFrags[3] && (Error < MinError) )
            {
                Error = cpi->GetNextSAD( SrcPtr[3], CandidateBlockPtr + RefRow2Offset + 8, 
									PixelsPerLine, Error, MinError );
            }

            if ( Error < MinError )
			{
				// Remember best match.
				MinError = Error;
				BestBlockPtr = CandidateBlockPtr;
				
				// Where is it.
				x = MV->x + cpi->MVOffsetX[SearchSite];
				y = MV->y + cpi->MVOffsetY[SearchSite];
			}

			// Move to next search location.
			SearchSite += 1;
		}

		// Move to best location this step.
		RefPtr = BestBlockPtr;
		MV->x = x;
		MV->y = y;
	}

    // Factor vectors to 1/2 pixel resoultion.
	MV->x = (MV->x * 2);
	MV->y = (MV->y * 2);

    // Now do the half pixel pass
    BestHalfOffset = 4;     // Default to the no offset case.
    BestHalfPixelError = MinError;

    // Get the half pixel error for each half pixel offset
	for ( i=0; i < 9; i++ )
    {
        HalfPixelError = 0;

        if ( MBlockDispFrags[0] )
        {
            RefDataPtr1 = BestBlockPtr;
            RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
            HalfPixelError = cpi->GetSadHalfPixel( SrcPtr[0], RefDataPtr1, RefDataPtr2, 
						                      PixelsPerLine, HalfPixelError, BestHalfPixelError );
        }

        if ( MBlockDispFrags[1]  && (HalfPixelError < BestHalfPixelError) )
        {
            RefDataPtr1 = BestBlockPtr + 8;
            RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
            HalfPixelError = cpi->GetSadHalfPixel( SrcPtr[1], RefDataPtr1, RefDataPtr2,
						                      PixelsPerLine, HalfPixelError, BestHalfPixelError );
        }

        if ( MBlockDispFrags[2] && (HalfPixelError < BestHalfPixelError) )
        {
            RefDataPtr1 = BestBlockPtr + RefRow2Offset;
            RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
            HalfPixelError = cpi->GetSadHalfPixel( SrcPtr[2], RefDataPtr1, RefDataPtr2,
						                      PixelsPerLine, HalfPixelError, BestHalfPixelError );
        }

        if ( MBlockDispFrags[3] && (HalfPixelError < BestHalfPixelError) )
        {
            RefDataPtr1 = BestBlockPtr + RefRow2Offset + 8;
            RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
            HalfPixelError = cpi->GetSadHalfPixel( SrcPtr[3], RefDataPtr1, RefDataPtr2, 
						                      PixelsPerLine, HalfPixelError, BestHalfPixelError );
        }

        if ( HalfPixelError < BestHalfPixelError )
        {
            BestHalfOffset = (UINT8)i;
            BestHalfPixelError = HalfPixelError;
        }
    }

    // Half pixel adjust the MV
	MV->x += cpi->HalfPixelXOffset[BestHalfOffset];
	MV->y += cpi->HalfPixelYOffset[BestHalfOffset];

	cpi->pb.ClearSysState();

	// Get the error score for the chosen 1/2 pixel offset as a variance.
	InterMVError = GetMBInterError( cpi, cpi->ConvDestBuffer, RefFramePtr, FragIndex, MV->x, MV->y, PixelsPerLine );
    
    // Return score of best matching block.
	return InterMVError;
}


/****************************************************************************
 * 
 *  ROUTINE       :     GetMBMV_ExhaustiveSearch
 *
 *  INPUTS        :     UINT8   *RefFramePtr (reference fram for search)
 *                      UINT32  Fragment offset of first block in MB
 *						UINT32	PixelsPerLine
 *                      MOTION_VECTOR *MV
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Inter frame variance
 *
 *  FUNCTION      :     Calculates a macro block MV using an exhaustive search.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetMBMVExhaustiveSearch( CP_INSTANCE *cpi, UINT8 * RefFramePtr, UINT32 FragIndex, UINT32 PixelsPerLine, MOTION_VECTOR *MV )
{
    UINT32	Error = 0;
    UINT32	MinError = HUGE_ERROR;
    UINT32  InterMVError = 0;
    
	INT32	i, j;
	INT32	x=0, y=0;
	
 	UINT8	*SrcPtr[4] = {NULL,NULL,NULL,NULL};
    UINT8	*RefPtr;
	UINT8	*CandidateBlockPtr=NULL;
	UINT8	*BestBlockPtr=NULL;

    UINT32  RefRow2Offset = cpi->pb.Configuration.YStride * 8;

    BOOL    MBlockDispFrags[4];

    // Half pixel variables
    INT32   HalfPixelError;
    INT32   BestHalfPixelError;
    UINT8   BestHalfOffset;
    UINT8 * RefDataPtr1;
    UINT8 * RefDataPtr2;

    // Note which of the four blocks in the macro block are to be included in the search.
    MBlockDispFrags[0] = cpi->pb.display_fragments[FragIndex];
    MBlockDispFrags[1] = cpi->pb.display_fragments[FragIndex + 1];
    MBlockDispFrags[2] = cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments];
    MBlockDispFrags[3] = cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments + 1];

    // Set up the source pointers for the four source blocks. 
    SrcPtr[0] = &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,FragIndex)];
    SrcPtr[1] = SrcPtr[0] + 8;
    SrcPtr[2] = SrcPtr[0] + (PixelsPerLine * 8);
    SrcPtr[3] = SrcPtr[2] + 8;

    RefPtr = &RefFramePtr[GetFragIndex(cpi->pb.recon_pixel_index_table,FragIndex)];
    RefPtr = RefPtr - ((MvMaxExtent/2) * cpi->pb.Configuration.YStride) - (MvMaxExtent/2);

    // Search each pixel alligned site
    for ( i = 0; i < (INT32)MvMaxExtent; i ++ )
    {
        // Starting position in row
        CandidateBlockPtr = RefPtr;

        for ( j = 0; j < (INT32)MvMaxExtent; j++ )
        {
            // Reset error
            Error = 0;

            // Summ errors for each block.
            if ( MBlockDispFrags[0] )
            {
                Error = cpi->GetSAD( SrcPtr[0], CandidateBlockPtr, 
			                    PixelsPerLine, Error, HUGE_ERROR );
            }
            if ( MBlockDispFrags[1] )
            {
                Error = cpi->GetSAD( SrcPtr[1], CandidateBlockPtr + 8, 
						        PixelsPerLine, Error, HUGE_ERROR );         
            }
            if ( MBlockDispFrags[2] )
            {
                Error = cpi->GetSAD( SrcPtr[2], CandidateBlockPtr + RefRow2Offset, 
			                    PixelsPerLine, Error, HUGE_ERROR );        
            }
            if ( MBlockDispFrags[3] )
            {
                Error = cpi->GetSAD( SrcPtr[3], CandidateBlockPtr + RefRow2Offset + 8, 
			                    PixelsPerLine, Error, HUGE_ERROR );
            }

            // Was this the best so far
            if ( Error < MinError )
            {
                MinError = Error;
                BestBlockPtr = CandidateBlockPtr;
				x = 16 + j - MvMaxExtent;
				y = 16 + i - MvMaxExtent;
            }

            // Move the the next site
            CandidateBlockPtr ++;
        }

        // Move on to the next row.
        RefPtr += cpi->pb.Configuration.YStride;

    }

    // Factor vectors to 1/2 pixel resoultion.
	MV->x = (x * 2);
	MV->y = (y * 2);

    // Now do the half pixel pass
    BestHalfOffset = 4;     // Default to the no offset case.
    BestHalfPixelError = MinError;

    // Get the half pixel error for each half pixel offset
	for ( i=0; i < 9; i++ )
    {
        HalfPixelError = 0;

        if ( MBlockDispFrags[0] )
        {
            RefDataPtr1 = BestBlockPtr;
            RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
            HalfPixelError = cpi->GetSadHalfPixel( SrcPtr[0], RefDataPtr1, RefDataPtr2, 
						                      PixelsPerLine, HalfPixelError, BestHalfPixelError );
        }

        if ( MBlockDispFrags[1]  && (HalfPixelError < BestHalfPixelError) )
        {
            RefDataPtr1 = BestBlockPtr + 8;
            RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
            HalfPixelError = cpi->GetSadHalfPixel( SrcPtr[1], RefDataPtr1, RefDataPtr2,
						                      PixelsPerLine, HalfPixelError, BestHalfPixelError );
        }

        if ( MBlockDispFrags[2] && (HalfPixelError < BestHalfPixelError) )
        {
            RefDataPtr1 = BestBlockPtr + RefRow2Offset;
            RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
            HalfPixelError = cpi->GetSadHalfPixel( SrcPtr[2], RefDataPtr1, RefDataPtr2,
						                      PixelsPerLine, HalfPixelError, BestHalfPixelError );
        }

        if ( MBlockDispFrags[3] && (HalfPixelError < BestHalfPixelError) )
        {
            RefDataPtr1 = BestBlockPtr + RefRow2Offset + 8;
            RefDataPtr2 = RefDataPtr1 + cpi->HalfPixelRef2Offset[i];
            HalfPixelError = cpi->GetSadHalfPixel( SrcPtr[3], RefDataPtr1, RefDataPtr2, 
						                      PixelsPerLine, HalfPixelError, BestHalfPixelError );
        }

        if ( HalfPixelError < BestHalfPixelError )
        {
            BestHalfOffset = (UINT8)i;
            BestHalfPixelError = HalfPixelError;
        }
    }

    // Half pixel adjust the MV
	MV->x += cpi->HalfPixelXOffset[BestHalfOffset];
	MV->y += cpi->HalfPixelYOffset[BestHalfOffset];

	// Get the error score for the chosen 1/2 pixel offset as a variance.
	InterMVError = GetMBInterError( cpi, cpi->ConvDestBuffer, RefFramePtr, FragIndex, MV->x, MV->y, PixelsPerLine );
    
    // Return score of best matching block.
	return InterMVError;
}

/****************************************************************************
 * 
 *  ROUTINE       :     GetFOURMVExhaustiveSearch
 *
 *  INPUTS        :     UINT8   *RefFramePtr 
 *                              (reference frame for search)
 *                      UINT32  FragIndex
 *                              Fragment offset of first block in MB
 *						UINT32	PixelsPerLine
 *
 *                      MOTION_VECTOR *MV
 *                              Data structure into which the motion vectors will be written
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Inter frame variance
 *
 *  FUNCTION      :     Calculates a MV for each Y block in a macro block using an exhaustive search.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetFOURMVExhaustiveSearch( CP_INSTANCE *cpi, UINT8 * RefFramePtr, UINT32 FragIndex, UINT32 PixelsPerLine, MOTION_VECTOR *MV )
{
    UINT32  InterMVError;
    
    // For the moment the 4MV mode is only deemd to be valid if all four Y blocks are to be updated
    // This May be adapted later.
    if ( cpi->pb.display_fragments[FragIndex] && cpi->pb.display_fragments[FragIndex + 1] &&
         cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments] && cpi->pb.display_fragments[FragIndex + cpi->pb.HFragments + 1] )
    {

        // Reset the error score.
        InterMVError = 0;

        // Get the error component from each coded block
        InterMVError += GetBMVExhaustiveSearch( cpi, RefFramePtr, FragIndex, PixelsPerLine, &(MV[0]) );
        InterMVError += GetBMVExhaustiveSearch( cpi, RefFramePtr, (FragIndex + 1), PixelsPerLine, &(MV[1]) );
        InterMVError += GetBMVExhaustiveSearch( cpi, RefFramePtr, (FragIndex + cpi->pb.HFragments), PixelsPerLine, &(MV[2]) );
        InterMVError += GetBMVExhaustiveSearch( cpi, RefFramePtr, (FragIndex + cpi->pb.HFragments + 1), PixelsPerLine, &(MV[3]) );
    }
    else
    {
        InterMVError = HUGE_ERROR;
    }

    // Return score of best matching block.
	return InterMVError;
}

/****************************************************************************
 * 
 *  ROUTINE       :     GetBMVExhaustiveSearch
 *
 *  INPUTS        :     UINT8   *RefFramePtr 
 *                              (reference frame for search)
 *                      UINT32  FragIndex
 *                              Fragment offset of first block in MB
 *						UINT32	PixelsPerLine
 *
 *                      MOTION_VECTOR *MV
 *                              Data structure into which the motion vector will be written
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Inter frame variance
 *
 *  FUNCTION      :     Calculates a MV for an individual 8x8 Y block.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 GetBMVExhaustiveSearch( CP_INSTANCE *cpi, UINT8 * RefFramePtr, UINT32 FragIndex, UINT32 PixelsPerLine, MOTION_VECTOR *MV )
{
    UINT32	Error = 0;
    UINT32	MinError = HUGE_ERROR;
    UINT32  InterMVError = 0;
    
	INT32	i, j;
	INT32	x=0, y=0;
	
 	UINT8	*SrcPtr = NULL;
    UINT8	*RefPtr;
	UINT8	*CandidateBlockPtr=NULL;
	UINT8	*BestBlockPtr=NULL;

    // Half pixel variables
    INT32   HalfPixelError;
    INT32   BestHalfPixelError;
    UINT8   BestHalfOffset;
    UINT8 * RefDataPtr2;

    // Set up the source pointer for the block. 
    SrcPtr = &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,FragIndex)];
 
    RefPtr = &RefFramePtr[GetFragIndex(cpi->pb.recon_pixel_index_table,FragIndex)];
    RefPtr = RefPtr - ((MvMaxExtent/2) * cpi->pb.Configuration.YStride) - (MvMaxExtent/2);

    // Search each pixel alligned site
    for ( i = 0; i < (INT32)MvMaxExtent; i ++ )
    {
        // Starting position in row
        CandidateBlockPtr = RefPtr;

        for ( j = 0; j < (INT32)MvMaxExtent; j++ )
        {
            // Get the block error score.
            Error = cpi->GetSAD( SrcPtr, CandidateBlockPtr, 
			                PixelsPerLine, 0, HUGE_ERROR );

            // Was this the best so far
            if ( Error < MinError )
            {
                MinError = Error;
                BestBlockPtr = CandidateBlockPtr;
				x = 16 + j - MvMaxExtent;
				y = 16 + i - MvMaxExtent;
            }

            // Move the the next site
            CandidateBlockPtr ++;
        }

        // Move on to the next row.
        RefPtr += cpi->pb.Configuration.YStride;
    }

    // Factor vectors to 1/2 pixel resoultion.
	MV->x = (x * 2);
	MV->y = (y * 2);

    // Now do the half pixel pass
    BestHalfOffset = 4;     // Default to the no offset case.
    BestHalfPixelError = MinError;

    // Get the half pixel error for each half pixel offset
	for ( i=0; i < 9; i++ )
    {
        RefDataPtr2 = BestBlockPtr + cpi->HalfPixelRef2Offset[i];
        HalfPixelError = cpi->GetSadHalfPixel( SrcPtr, BestBlockPtr, RefDataPtr2, 
						                  PixelsPerLine, 0, BestHalfPixelError );

        if ( HalfPixelError < BestHalfPixelError )
        {
            BestHalfOffset = (UINT8)i;
            BestHalfPixelError = HalfPixelError;
        }
    }

    // Half pixel adjust the MV
	MV->x += cpi->HalfPixelXOffset[BestHalfOffset];
	MV->y += cpi->HalfPixelYOffset[BestHalfOffset];

    // Get the variance score at the chosen offset
    RefDataPtr2 = BestBlockPtr + cpi->HalfPixelRef2Offset[BestHalfOffset];

#ifndef NEW_ERROR_METRIC
    InterMVError = cpi->GetInterError( SrcPtr, BestBlockPtr, RefDataPtr2, PixelsPerLine );
#else
	InterMVError = GetInterDCTErr(cpi, SrcPtr, BestBlockPtr, RefDataPtr2, PixelsPerLine );
#endif
 
    // Return score of best matching block.
	return InterMVError;
}

