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
*   Module Title :     MiscCommon.c
*
*   Description  :     Miscellaneous common routines
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

#include "compdll.h"
#include "BlockMapping.h"
#include "CFrameW.h"
#include "Quantize.h"

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
#define FIXED_Q					150
 
#define MAX_UP_REG_LOOPS        2    

/****************************************************************************
*  Module statics.
*****************************************************************************
*/        

              
/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
 * 
 *  ROUTINE       :     GetEstimatedBpb
 *
 *  INPUTS        :     CP_INSTANCE *cpi, A Value of Q. 
 *                      
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     The current estimate for the number of bytes per block
 *                      at the current Q.
 *
 *  FUNCTION      :     Returns an estimate of the bytes per block that will
 *                      be achieved at the given Q
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
double GetEstimatedBpb( CP_INSTANCE *cpi, UINT32 TargetQ )
{
	UINT32 i;
	INT32 ThreshTableIndex = Q_TABLE_SIZE - 1;
    double BytesPerBlock;

	/* Search for the Q table index that matches the given Q. */
	for ( i = 0; i < Q_TABLE_SIZE; i++ )
	{
		if ( TargetQ >= cpi->pb.QThreshTable[i] )
		{
			ThreshTableIndex = i;
			break;
		}
	}

    // Adjust according to Q shift and type of frame
    if ( GetFrameType(&cpi->pb) == BASE_FRAME )
    {
        // Get primary prediction
        BytesPerBlock = KfBpbTable[ThreshTableIndex];
    }
    else 
    {
        // Get primary prediction
        BytesPerBlock = BpbTable[ThreshTableIndex];
        BytesPerBlock = BytesPerBlock * cpi->BpbCorrectionFactor;
    }

	return BytesPerBlock;
}

/****************************************************************************
 * 
 *  ROUTINE       :     UpRegulateMB
 *
 *  INPUTS        :     UINT32 RegulationQ
 *						(Q at which we are updating.)
 *
 *						UINT32 SB,MB 
 *                      (MB and SB indentifiers)
 *
 *						BOOL NoCheck
 *						(Should we check previous update Q)
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Up regulate specified MB
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UpRegulateMB( CP_INSTANCE *cpi, UINT32 RegulationQ, UINT32 SB, UINT32 MB, BOOL NoCheck )
{
	INT32  FragIndex;
	UINT32 B;							// Block indicex

	// Variables used in calculating corresponding row,col and index in UV planes
    UINT32 UVRow;
    UINT32 UVColumn;
    UINT32 UVFragOffset;

	// There may be MB's lying out of frame
	// which must be ignored. For these MB's
	// Top left block will have a negative Fragment Index.
	if ( QuadMapToMBTopLeft(cpi->pb.BlockMap, SB, MB ) >= 0 )
	{
		// Up regulate the component blocks Y then UV.
		for ( B=0; B<4; B++ )
		{
			FragIndex = QuadMapToIndex1( cpi->pb.BlockMap, SB, MB, B );

            if ( ( !cpi->pb.display_fragments[FragIndex] ) &&
				 ( (NoCheck) || (cpi->FragmentLastQ[FragIndex] > RegulationQ) ) )
			{
				cpi->pb.display_fragments[FragIndex] = 1;
				cpi->extra_fragments[FragIndex] = 1;
  				cpi->FragmentLastQ[FragIndex] = RegulationQ;
				cpi->MotionScore++;
			}
		}

		// Check the two UV blocks
		FragIndex = QuadMapToMBTopLeft(cpi->pb.BlockMap, SB, MB );

		UVRow = (FragIndex / (cpi->pb.HFragments * 2));
		UVColumn = (FragIndex % cpi->pb.HFragments) / 2;
		UVFragOffset = (UVRow * (cpi->pb.HFragments / 2)) + UVColumn;
				
		FragIndex = cpi->pb.YPlaneFragments + UVFragOffset;
		if ( ( !cpi->pb.display_fragments[FragIndex] ) &&
			 ( (NoCheck) || (cpi->FragmentLastQ[FragIndex] > RegulationQ) ) )
		{
			cpi->pb.display_fragments[FragIndex] = 1;
			cpi->extra_fragments[FragIndex] = 1;
  			cpi->FragmentLastQ[FragIndex] = RegulationQ;
			cpi->MotionScore++;
		}

		FragIndex += cpi->pb.UVPlaneFragments;
		if ( ( !cpi->pb.display_fragments[FragIndex] ) &&
			 ( (NoCheck) || (cpi->FragmentLastQ[FragIndex] > RegulationQ) ) )
		{
			cpi->pb.display_fragments[FragIndex] = 1;
			cpi->extra_fragments[FragIndex] = 1;
  			cpi->FragmentLastQ[FragIndex] = RegulationQ;
			cpi->MotionScore++;
		}
	}
}

/****************************************************************************
 * 
 *  ROUTINE       :     UpRegulateBlocks
 *
 *  INPUTS        :     UINT32 Q 
 *						(Q at which to up regulate and target number of blocks)
 *
 *                      UINT32 RecoveryBlocks
 *						(Limit on total number of blocks for this frame)
 *
 *                      UINT32 LastSB, LastMB
 *                      (Where we had got up to in SB) and MB
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     See if we can add some blocks.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UpRegulateBlocks( CP_INSTANCE *cpi, UINT32 RegulationQ, INT32 RecoveryBlocks, 
					   UINT32 * LastSB, UINT32 * LastMB ) 
{
    UINT32 LoopTimesRound = 0;
	UINT32 MaxSB = cpi->pb.YSBRows * cpi->pb.YSBCols;   // Tot super blocks in image
	UINT32 SB, MB;						// Super-Block and macro block indices.

	// First scan for blocks for which a residue update is outstanding.
	while ( (cpi->MotionScore < RecoveryBlocks) && 
		    (LoopTimesRound < MAX_UP_REG_LOOPS) )
	{
		LoopTimesRound++;

		for ( SB = (*LastSB); SB < MaxSB; SB++ )
		{
			// Check its four Macro-Blocks
			for ( MB=(*LastMB); MB<4; MB++ )
			{
				// Mark relevant blocks for update
				UpRegulateMB( cpi, RegulationQ, SB, MB, FALSE );

				// Keep track of the last refresh MB.
				(*LastMB) += 1;
				if ( (*LastMB) == 4 )
					(*LastMB) = 0;

				// Termination clause
				if (cpi->MotionScore >= RecoveryBlocks)
                {
                    // Make sure we don't stall at SB level
                    if ( *LastMB == 0 )
                        SB++;
					break;
                }
			}

			// Termination clause
			if (cpi->MotionScore >= RecoveryBlocks)
				break;
		}

		// Update super block start index 
		if ( SB >= MaxSB)
		{
		  	(*LastSB) = 0;
		}
		else
		{
		  	(*LastSB) = SB;
		}
	}
}

/****************************************************************************
 * 
 *  ROUTINE       :     UpRegulateDataStream
 *
 *  INPUTS        :     Q at which to up regulate and target number of blocks
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     This function uses up spare bandwidth when not much is going
 *                      on to refresh quality.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UpRegulateDataStream( CP_INSTANCE *cpi, UINT32 RegulationQ, INT32 RecoveryBlocks ) 
{  
	UINT32 LastPassMBPos = 0;		// MB index used in final pass of refresh.
	UINT32 StdLastMBPos = 0;			// MB index used in std refresh.
    
	UINT32 i =	0;   
    UINT32 LoopTimesRound = 0;

	UINT32 MaxSB = cpi->pb.YSBRows * cpi->pb.YSBCols;       // Tot super blocks in image

	UINT32 SB=0;							// Super-Block index
	UINT32 MB;								// Macro-Block index

	// Decduct the number of blocks in an MB / 2 from the recover block count.
	// This will compensate for the fact that once we start checking an MB
	// we test every block in that macro block
	if ( RecoveryBlocks > 3 )
	{
		RecoveryBlocks -= 3;
	}

	// Up regulate blocks last coded at higher Q
	UpRegulateBlocks( cpi, RegulationQ, RecoveryBlocks, &cpi->LastEndSB, &StdLastMBPos );

    // If we have still not used up the minimum number of blocks
    // and are at the minimum Q then run through a final pass of 
    // the data to insure that each block gets a final refresh.
    if ( (RegulationQ == VERY_BEST_Q) &&
		 (cpi->MotionScore < RecoveryBlocks) )
    {
		if ( cpi->FinalPassLastPos < MaxSB )
		{
			for ( SB = cpi->FinalPassLastPos; SB < MaxSB; SB++ )
			{
				// Check its four Macro-Blocks
				for ( MB=LastPassMBPos; MB<4; MB++ )
				{
					// Mark relevant blocks for update
					UpRegulateMB( cpi, RegulationQ, SB, MB, TRUE );

					// Keep track of the last refresh MB.
					LastPassMBPos += 1;
					if ( LastPassMBPos == 4 )
                    {
						LastPassMBPos = 0;

				        // Increment SB index
				        cpi->FinalPassLastPos += 1;
                    }

					// Termination clause
					if (cpi->MotionScore >= RecoveryBlocks)
						break;
				}

				// Termination clause
				if (cpi->MotionScore >= RecoveryBlocks)
					break;

			}
		}
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     RegulateQ
 *
 *  INPUTS        :     INT32 BlocksToUpdate
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     If appropriate this function regulates the DCT
 *                      coefficients to match the stream size to the    
 *                      available bandwidth (within defined limits). 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void RegulateQ( CP_INSTANCE *cpi, INT32 UpdateScore ) 
{   
    double TargetUnitScoreBytes = (double)cpi->ThisFrameTargetBytes / (double)UpdateScore;
    double PredUnitScoreBytes;
    double LastBitError = 10000.0;       // Silly high number
    UINT32 QIndex = Q_TABLE_SIZE - 1;
    UINT32 i;

    // Search for the best Q for the target bitrate.
	for ( i = 0; i < Q_TABLE_SIZE; i++ )
	{
        PredUnitScoreBytes = GetEstimatedBpb( cpi, cpi->pb.QThreshTable[i] );
        if ( PredUnitScoreBytes > TargetUnitScoreBytes )
        {
            if ( (PredUnitScoreBytes - TargetUnitScoreBytes) <= LastBitError )
            {
                QIndex = i;
            }
            else
            {
                QIndex = i - 1;
            }
            break;
        }
        else
        {
            LastBitError = TargetUnitScoreBytes - PredUnitScoreBytes;
        }
    }

    // QIndex should now indicate the optimal Q.
    cpi->pb.ThisFrameQualityValue = cpi->pb.QThreshTable[QIndex];
    
    // Apply range restrictions for key frames.
    if ( GetFrameType(&cpi->pb) == BASE_FRAME )
    {
        if ( cpi->pb.ThisFrameQualityValue > cpi->pb.QThreshTable[20] )
            cpi->pb.ThisFrameQualityValue = cpi->pb.QThreshTable[20];
        else if ( cpi->pb.ThisFrameQualityValue < cpi->pb.QThreshTable[50] )
            cpi->pb.ThisFrameQualityValue = cpi->pb.QThreshTable[50];
    }
    
    // Limit the Q value to the maximum available value
    if (cpi->pb.ThisFrameQualityValue > cpi->pb.QThreshTable[cpi->Configuration.ActiveMaxQ])
        //if (cpi->pb.ThisFrameQualityValue > QThreshTable[cpi->Configuration.ActiveMaxQ])
    {
        cpi->pb.ThisFrameQualityValue = (UINT32)cpi->pb.QThreshTable[cpi->Configuration.ActiveMaxQ];
    }  
    
    if(cpi->FixedQ)
    {
        if ( GetFrameType(&cpi->pb) == BASE_FRAME )
        {
            cpi->pb.ThisFrameQualityValue = cpi->pb.QThreshTable[43];
            cpi->pb.ThisFrameQualityValue = cpi->FixedQ;
        }
        else
        {
            cpi->pb.ThisFrameQualityValue = cpi->FixedQ;
        }
    }
    
    // If th quantiser value has changed then re-initialise it
    if ( cpi->pb.ThisFrameQualityValue != cpi->pb.LastFrameQualityValue )
    {                    
        /* Initialise quality tables. */
        UpdateQC( cpi, cpi->pb.ThisFrameQualityValue );
        cpi->pb.LastFrameQualityValue = cpi->pb.ThisFrameQualityValue;
    }

}

/****************************************************************************
 * 
 *  ROUTINE       :     ConfigureQuality
 *
 *  INPUTS        :     QualityValue
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None .
 *
 *  FUNCTION      :     Configures all variables assosciated with a quality setting
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void ConfigureQuality( CP_INSTANCE *cpi, UINT32 QualityValue )
{
    //UINT32 i;

    // Default first frame quality

    // Set the worst case quality value.
    // Note that the actual quality is determined by lookup into the quantiser table QThreshTable[]
    cpi->Configuration.MaxQ = 63 - QualityValue;

    // Set the default Active MaxQ.
    cpi->Configuration.ActiveMaxQ = cpi->Configuration.MaxQ;
}



/****************************************************************************
 * 
 *  ROUTINE       :     CopyBackExtraFrags
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None .
 *
 *  FUNCTION      :     Copies the pixel data from any extra fragments
 *                      selected for update as part of the quality refresh process
 *                      from the source buffer into the conv. buffer.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CopyBackExtraFrags(CP_INSTANCE *cpi)
{
	UINT32  i,j;
    UINT8 * SrcPtr;
    UINT8 * DestPtr;
    UINT32  PlaneLineStep;   
	UINT32  PixelIndex;
    
	// Copy back for Y plane.
    PlaneLineStep = cpi->pb.Configuration.VideoFrameWidth;
    for ( i = 0; i < cpi->pb.YPlaneFragments; i++ )
    {
        /* We are only interested in updated fragments. */
        if ( cpi->extra_fragments[i] )
        {
            /* Get the start index for the fragment. */
			PixelIndex = GetFragIndex(cpi->pb.pixel_index_table, i);
            SrcPtr = &cpi->yuv1ptr[PixelIndex];
            DestPtr = &cpi->ConvDestBuffer[PixelIndex];

            for ( j = 0; j < cpi->pb.Configuration.VFragPixels; j++ )
            {
                memcpy( DestPtr, SrcPtr, cpi->pb.Configuration.HFragPixels );

                SrcPtr += PlaneLineStep;
                DestPtr += PlaneLineStep;
            }
        }
    }


	// Now the U and V planes
    PlaneLineStep = cpi->pb.Configuration.VideoFrameWidth / 2;
    for ( i = cpi->pb.YPlaneFragments; i < (cpi->pb.YPlaneFragments + (2 * cpi->pb.UVPlaneFragments)) ; i++ )
    {
        /* We are only interested in updated fragments. */
        if ( cpi->extra_fragments[i] )
        {
            /* Get the start index for the fragment. */
			PixelIndex = GetFragIndex(cpi->pb.pixel_index_table, i);
            SrcPtr = &cpi->yuv1ptr[PixelIndex];
            DestPtr = &cpi->ConvDestBuffer[PixelIndex];

            for ( j = 0; j < cpi->pb.Configuration.VFragPixels; j++ )
            {
                memcpy( DestPtr, SrcPtr, cpi->pb.Configuration.HFragPixels );
                SrcPtr += PlaneLineStep;
                DestPtr += PlaneLineStep;
            }

        }
    }

}

