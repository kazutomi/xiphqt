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
*   Module Title :     PreProcIf.c
*
*   Description  :     Pre-processor dll interface module.
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
#include "type_aliases.h"
#include "Preproc.h"


/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
#define MIN_STEP_THRESH 6


/****************************************************************************
*  Explicit Imports
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
 *  ROUTINE       :     ScanYUVInit
 *
 *  INPUTS        :     SCAN_CONFIG_DATA * ScanConfigPtr
 *                          Configuration data.
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Initialises the scan process. 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
extern BOOL PAllocateFrameInfo(PP_INSTANCE * ppi);
DLL BOOL ScanYUVInit( PP_INSTANCE *  ppi, SCAN_CONFIG_DATA * ScanConfigPtr)
{  
    INT32 i;

    // Test machine specific features such as MMX support
    MachineSpecificConfig(ppi);

	/* Set up the various imported data structure pointers. */
	ppi->ScanConfig.Yuv0ptr = ScanConfigPtr->Yuv0ptr;
	ppi->ScanConfig.Yuv1ptr = ScanConfigPtr->Yuv1ptr;
	ppi->ScanConfig.SrfWorkSpcPtr = ScanConfigPtr->SrfWorkSpcPtr;
	ppi->ScanConfig.disp_fragments = ScanConfigPtr->disp_fragments;
	
    ppi->ScanConfig.RegionIndex = ScanConfigPtr->RegionIndex;
	ppi->ScanConfig.HFragPixels = ScanConfigPtr->HFragPixels;
	ppi->ScanConfig.VFragPixels = ScanConfigPtr->VFragPixels;

	ppi->ScanConfig.VideoFrameWidth = ScanConfigPtr->VideoFrameWidth;
	ppi->ScanConfig.VideoFrameHeight = ScanConfigPtr->VideoFrameHeight;

	// UV plane sizes.
	ppi->VideoUVPlaneWidth = ScanConfigPtr->VideoFrameWidth / 2;
	ppi->VideoUVPlaneHeight = ScanConfigPtr->VideoFrameHeight / 2;

    /* Note the size of the entire frame and plaes in pixels. */
    ppi->YFramePixels = ppi->ScanConfig.VideoFrameWidth * ppi->ScanConfig.VideoFrameHeight;
    ppi->UVFramePixels = ppi->VideoUVPlaneWidth * ppi->VideoUVPlaneHeight;

	/* Work out various fragment related values. */
	ppi->ScanYPlaneFragments = ppi->YFramePixels / (ppi->HFragPixels * ppi->VFragPixels);
	ppi->ScanUVPlaneFragments = ppi->UVFramePixels / (ppi->HFragPixels * ppi->VFragPixels);;
    ppi->ScanHFragments = ppi->ScanConfig.VideoFrameWidth / ppi->HFragPixels;
    ppi->ScanVFragments = ppi->ScanConfig.VideoFrameHeight / ppi->VFragPixels;
	ppi->ScanFrameFragments = ppi->ScanYPlaneFragments + (2 * ppi->ScanUVPlaneFragments);

    if(!PAllocateFrameInfo(ppi))
        return FALSE;

	/* Set up the scan pixel index table. */
	ScanCalcPixelIndexTable(ppi);

    // Initialise the previous frame block history lists
    for ( i = 0; i < MAX_PREV_FRAMES; i++ )
        memset( ppi->PrevFragments[i], BLOCK_NOT_CODED, ppi->ScanFrameFragments );

    // YUVAnalyseFrame() is not called for the first frame in a sequence (a key frame obviously).
	// This memset insures that for the second frame all blocks are marked for coding in line
	// with the behaviour for other key frames.
	memset( ppi->PrevFragments[ppi->PrevFrameLimit-1], BLOCK_CODED, ppi->ScanFrameFragments );

	/* Initialise scan arrays */
	InitScanMapArrays(ppi);

	return TRUE;
}


/****************************************************************************
 * 
 *  ROUTINE       :     YUVAnalyseFrame
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     Number of "output" blocks to be updated.
 *
 *  FUNCTION      :     Scores the fragments for the YUV planes 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
DLL UINT32 YUVAnalyseFrame( PP_INSTANCE *ppi, UINT32 * KFIndicator ) 
{  
    UINT32 UpdatedYBlocks = 0;
    UINT32 UpdatedUVBlocks = 0;

	/* Initialise the map arrays. */
	InitScanMapArrays(ppi);

    // If the motion level in the previous frame was high then adjust the high and low SAD 
    // thresholds to speed things up.
    ppi->ModifiedGrpLowSadThresh = ppi->GrpLowSadThresh;
    ppi->ModifiedGrpHighSadThresh = ppi->GrpHighSadThresh;


    // Set up the internal plane height and width variables.
    ppi->VideoYPlaneWidth = ppi->ScanConfig.VideoFrameWidth;
    ppi->VideoYPlaneHeight = ppi->ScanConfig.VideoFrameHeight;
	ppi->VideoUVPlaneWidth = ppi->ScanConfig.VideoFrameWidth / 2;
	ppi->VideoUVPlaneHeight = ppi->ScanConfig.VideoFrameHeight / 2;

    // To start with the strides will be set from the widths
    ppi->VideoYPlaneStride = ppi->VideoYPlaneWidth;
    ppi->VideoUPlaneStride = ppi->VideoUVPlaneWidth;
    ppi->VideoVPlaneStride = ppi->VideoUVPlaneWidth;
    
    // Set up the plane pointers
    ppi->YPlanePtr0 = ppi->ScanConfig.Yuv0ptr;
    ppi->YPlanePtr1 = ppi->ScanConfig.Yuv1ptr;
    ppi->UPlanePtr0 = (ppi->ScanConfig.Yuv0ptr + ppi->YFramePixels);
    ppi->UPlanePtr1 = (ppi->ScanConfig.Yuv1ptr + ppi->YFramePixels);
    ppi->VPlanePtr0 = (ppi->ScanConfig.Yuv0ptr + ppi->YFramePixels + ppi->UVFramePixels);
    ppi->VPlanePtr1 = (ppi->ScanConfig.Yuv1ptr + ppi->YFramePixels + ppi->UVFramePixels);

    // Check previous frame lists and if necessary mark extra blocks for update.
    SetFromPrevious(ppi);

    // Ananlyse the U and V palnes. 
    AnalysePlane( ppi, ppi->UPlanePtr0, ppi->UPlanePtr1, ppi->ScanYPlaneFragments, ppi->VideoUVPlaneWidth, ppi->VideoUVPlaneHeight, ppi->VideoUPlaneStride );
    AnalysePlane( ppi, ppi->VPlanePtr0, ppi->VPlanePtr1, (ppi->ScanYPlaneFragments + ppi->ScanUVPlaneFragments), ppi->VideoUVPlaneWidth, ppi->VideoUVPlaneHeight, ppi->VideoVPlaneStride );

    // Now analyse the Y plane.
    AnalysePlane( ppi, ppi->YPlanePtr0, ppi->YPlanePtr1, 0, ppi->VideoYPlaneWidth, ppi->VideoYPlaneHeight, ppi->VideoYPlaneStride );

    // Update the list of previous frame block updates.
    UpdatePreviousBlockLists(ppi);
    
    // Create an output block map for the calling process. 
	CreateOutputDisplayMap( ppi, ppi->ScanDisplayFragments, 
                            ppi->PrevFragments[0],
                            ppi->ScanConfig.disp_fragments );
	
	// Set the candidate key frame indicator (0-100)
	*KFIndicator = ppi->KFIndicator;

	// Return the normalised block count (this is actually a motion level 
    // weighting not a true block count).
	return ppi->OutputBlocksUpdated;
}

/****************************************************************************
 * 
 *  ROUTINE       :     SetScanParam
 *
 *  INPUTS        :     ParamID
 *                      ParamValue
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Sets a scan parameter. 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
DLL void SetScanParam( PP_INSTANCE *ppi, UINT32 ParamId, INT32 ParamValue ) 
{  
	switch (ParamId)
	{

	case SCP_CONFIGURE_PP:
		ConfigurePP(ppi, ParamValue);
        break;

	}

}

