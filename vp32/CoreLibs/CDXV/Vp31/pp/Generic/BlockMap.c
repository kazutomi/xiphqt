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
*   Module Title :     BlockMap.c
*
*   Description  :     Contains functions used to create the block map
*
*
*****************************************************************************
*/						

/****************************************************************************
*  Header Frames
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */

#include <string.h>

#include "Preproc.h"


/****************************************************************************
*  Module constants.
*****************************************************************************
*/  

#define HISTORY_BLOCK_FACTOR    2

/****************************************************************************
*  Module Types
*****************************************************************************
*/              

/****************************************************************************
*  Imported Global Variables
*****************************************************************************
*/


/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Foreward References
*****************************************************************************
*/              


/****************************************************************************
*  Module Statics
*****************************************************************************
*/              


/****************************************************************************
 * 
 *  ROUTINE       :     RowBarEnhBlockMap
 *
 *  INPUTS        :     UINT32 * FragNoiseScorePtr 
 *                      INT8   * FragSgcPtr
 *                      UINT32   RowNumber
 *
 *  OUTPUTS       :     INT8   * UpdatedBlockMapPtr 
 *                      INT8   * BarBlockMapPtr
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     BAR Enhances block map on a row by row basis.
 *
 *  SPECIAL NOTES :     Note special cases for first and last row and first and last
 *                      block in each row. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void RowBarEnhBlockMap( PP_INSTANCE *ppi, 
					    UINT32 * FragScorePtr, 
						INT8   * FragSgcPtr,
						INT8   * UpdatedBlockMapPtr,
						INT8   * BarBlockMapPtr,
						UINT32 RowNumber )
{
	// For boundary blocks relax thresholds
	UINT32 BarBlockThresh = ppi->BlockThreshold / 10;
	UINT32 BarSGCThresh = ppi->BlockSgcThresh / 2;

	INT32 i;

    // Start by blanking the row in the bar block map structure.
	memset( BarBlockMapPtr, BLOCK_NOT_CODED, ppi->PlaneHFragments );

	// First row
	if ( RowNumber == 0 )
	{
        
		// For each fragment in the row.
		for ( i = 0; i < ppi->PlaneHFragments; i ++ )
		{
			// Test for CANDIDATE_BLOCK or CANDIDATE_BLOCK_LOW
			// Uncoded or coded blocks will be ignored.
            if ( UpdatedBlockMapPtr[i] <= CANDIDATE_BLOCK )
			{
				// Is one of the immediate neighbours updated in the main map.
				// Note special cases for blocks at the start and end of rows.
				if ( i == 0 )
				{
                    
					if ( (UpdatedBlockMapPtr[i+1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments+1] > BLOCK_NOT_CODED ) )
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
                    
				}
				else if ( i == (ppi->PlaneHFragments - 1) )
				{
                    
					if ( (UpdatedBlockMapPtr[i-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments] > BLOCK_NOT_CODED ) )
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
                    
				}
				else
				{
					if ( (UpdatedBlockMapPtr[i-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments+1] > BLOCK_NOT_CODED ) )
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
				}
			}
		}
        
	}
	// Last row
    //   Used to read PlaneHFragments
	else if ( RowNumber == (UINT32)(ppi->PlaneVFragments-1))
	{
        
		// For each fragment in the row.
		for ( i = 0; i < ppi->PlaneHFragments; i ++ )
		{
			// Test for CANDIDATE_BLOCK or CANDIDATE_BLOCK_LOW
			// Uncoded or coded blocks will be ignored.
            if ( UpdatedBlockMapPtr[i] <= CANDIDATE_BLOCK )
			{
				// Is one of the immediate neighbours updated in the main map.
				// Note special cases for blocks at the start and end of rows.
				if ( i == 0 )
				{
					if ( (UpdatedBlockMapPtr[i+1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments+1] > BLOCK_NOT_CODED ) )
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
                
				}
				else if ( i == (ppi->PlaneHFragments - 1) )
				{
					if ( (UpdatedBlockMapPtr[i-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments] > BLOCK_NOT_CODED ) )
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
				}
				else
				{
					if ( (UpdatedBlockMapPtr[i-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments+1] > BLOCK_NOT_CODED ) )
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
				}
			}
		}
        
	}
	// All other rows
	else
	{
		// For each fragment in the row.
		for ( i = 0; i < ppi->PlaneHFragments; i ++ )
		{
			// Test for CANDIDATE_BLOCK or CANDIDATE_BLOCK_LOW
			// Uncoded or coded blocks will be ignored.
            if ( UpdatedBlockMapPtr[i] <= CANDIDATE_BLOCK )
			{
				// Is one of the immediate neighbours updated in the main map.
				// Note special cases for blocks at the start and end of rows.
				if ( i == 0 )
				{
                    
					if ( (UpdatedBlockMapPtr[i+1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments+1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments+1] > BLOCK_NOT_CODED ) )
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
                    
				}
				else if ( i == (ppi->PlaneHFragments - 1) )
				{
                    
					if ( (UpdatedBlockMapPtr[i-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments] > BLOCK_NOT_CODED ) )
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
                    
				}
				else
				{
					if ( (UpdatedBlockMapPtr[i-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i-ppi->PlaneHFragments+1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments-1] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments] > BLOCK_NOT_CODED ) ||
						 (UpdatedBlockMapPtr[i+ppi->PlaneHFragments+1] > BLOCK_NOT_CODED ) )
                         
					{
						BarBlockMapPtr[i] = BLOCK_CODED_BAR;
					}
				}
			}
		}
	}
}

/****************************************************************************
 * 
 *  ROUTINE       :     BarCopyBack
 *
 *  INPUTS        :     INT8  * BarBlockMapPtr
 *
 *  OUTPUTS       :     INT8  * UpdatedBlockMapPtr 
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Copies BAR blocks back into main block map.
 *
 *  SPECIAL NOTES :     None.
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void BarCopyBack( PP_INSTANCE *ppi, 
				  INT8  * UpdatedBlockMapPtr,
				  INT8  * BarBlockMapPtr )
{
	INT32 i;

	// For each fragment in the row.
	for ( i = 0; i < ppi->PlaneHFragments; i ++ )
	{
		if ( BarBlockMapPtr[i] > BLOCK_NOT_CODED )
		{
			UpdatedBlockMapPtr[i] = BarBlockMapPtr[i];
		}
	}
}

/****************************************************************************
 * 
 *  ROUTINE       :     UpdatePreviousBlockLists
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Keep a copy of the block lists from previous frames.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void UpdatePreviousBlockLists(PP_INSTANCE *ppi) 
{ 
    UINT32  i;

    // Shift previous frame block lists along.
    for ( i = ppi->PrevFrameLimit; i > 1; i-- )
    {
        memcpy( ppi->PrevFragments[i], ppi->PrevFragments[i-1], ppi->ScanFrameFragments );
    }

    // Now copy in this frames block list
    memcpy( ppi->PrevFragments[1], ppi->ScanDisplayFragments, ppi->ScanFrameFragments );
}

/****************************************************************************
 * 
 *  ROUTINE       :     SetFromPrevious
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     This function marks as coded blocks that were marked 
 *                      as coded at above a certain category level in at least one
 *                      of the last X frames.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void SetFromPrevious(PP_INSTANCE *ppi) 
{ 
    UINT32  i,j;

    // We buld up the list of previously updated blocks in the zero index 
    // list of PrevFragments[] so we must start by reseting its contents
    memset( ppi->PrevFragments[0], BLOCK_NOT_CODED, ppi->ScanFrameFragments );

    if ( ppi->PrevFrameLimit > 1 )
    {
        // Now build up PrevFragments[0] from PrevFragments[1 to PrevFrameLimit]
        for ( i = 0; i < ppi->ScanFrameFragments; i++ )
        {
            for ( j = 1; j < ppi->PrevFrameLimit; j++ )
            {
                if ( ppi->PrevFragments[j][i] > BLOCK_CODED_BAR )
                {
                    ppi->PrevFragments[0][i] = BLOCK_CODED;
                    break;
                }
            }
        }
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     CreateOutputDisplayMap
 *
 *  INPUTS        :     INT8 *  InternalFragmentsPtr 
 *                              Fragment list using internal format.
 *                      INT8 *  RecentHistoryPtr
 *                              List of blocks that have been marked for update int he last few frames.
 * 
 *                      UINT8 * ExternalFragmentsPtr
 *                              Fragment list using external format.
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Creates a block update map in the format expected by the caller.
 *
 *  SPECIAL NOTES :     The output block height and width must be an integer
 *                      multiple of the internal value.  
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CreateOutputDisplayMap( PP_INSTANCE *ppi, 
							 INT8 *  InternalFragmentsPtr, 
                             INT8 *  RecentHistoryPtr,
                             UINT8 * ExternalFragmentsPtr ) 
{ 
    UINT32 i;
    UINT32 HistoryBlocksAdded = 0;
	UINT32 KFScore = 0;
	UINT32 YBand = 	(ppi->ScanYPlaneFragments/8);	// 1/8th of Y image.	

    
	ppi->OutputBlocksUpdated = 0;
    for ( i = 0; i < ppi->ScanFrameFragments; i++ )
    {
		if ( InternalFragmentsPtr[i] > BLOCK_NOT_CODED ) 
        {
            ppi->OutputBlocksUpdated ++;
			ExternalFragmentsPtr[i] = 1;
        }
        else if ( RecentHistoryPtr[i] == BLOCK_CODED )
        {
            HistoryBlocksAdded ++;
			ExternalFragmentsPtr[i] = 1;
        }
		else
		{
			ExternalFragmentsPtr[i] = 0;
		}
    }

    // Add in a weighting for the history blocks that have been added
    ppi->OutputBlocksUpdated += (HistoryBlocksAdded / HISTORY_BLOCK_FACTOR);

	// Now calculate a key frame candidate indicator.
	// This is based upon Y data only and only ignores the top and bottom 1/8 of the image.
	// Also ignore history blocks and BAR blocks.
    ppi->KFIndicator = 0;
    for ( i = YBand; i < (ppi->ScanYPlaneFragments - YBand); i++ )
    {
		if ( InternalFragmentsPtr[i] > BLOCK_CODED_BAR ) 
        {
            ppi->KFIndicator ++;
        }
    }

	// Convert the KF score to a range 0-100
	ppi->KFIndicator = ((ppi->KFIndicator*100)/((ppi->ScanYPlaneFragments*3)/4));
}

