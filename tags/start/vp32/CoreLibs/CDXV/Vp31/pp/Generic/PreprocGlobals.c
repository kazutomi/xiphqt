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
*   Module Title :     PreProcGlobals
*
*   Description  :     Pre-processor module globals.
*
*
*****************************************************************************
*/						

/****************************************************************************
*  Header Frames
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include "PreprocConf.h"
#include "Preproc.h"
#include <stdlib.h>
#include "duck_mem.h"

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/



//PP_INSTANCE *ppi;


/****************************************************************************
 * 
 *  ROUTINE       :     PDeleteFragmentInfo
 *
 *
 *  INPUTS        :     Instance of PB to be initialized
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Initializes the Playback instance passed in
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void PDeleteFragmentInfo(PP_INSTANCE * ppi)
{

	// duck_free prior allocs if present

}


/****************************************************************************
 * 
 *  ROUTINE       :     PAllocateFragmentInfo
 *
 *
 *  INPUTS        :     Instance of PB to be initialized
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Initializes the Playback instance passed in
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void PAllocateFragmentInfo(PP_INSTANCE * ppi)
{

	// clear any existing info
	PDeleteFragmentInfo(ppi);

	// Perform Fragment Allocations

}

/****************************************************************************
 * 
 *  ROUTINE       :     PDeleteFrameInfo
 *
 *
 *  INPUTS        :     Instance of PB to be initialized
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Initializes the Playback instance passed in
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void PDeleteFrameInfo(PP_INSTANCE * ppi)
{
    int i;

	if(	ppi->ScanPixelIndexTableAlloc )
		duck_free(ppi->ScanPixelIndexTableAlloc);
	ppi->ScanPixelIndexTableAlloc= 0;
	ppi->ScanPixelIndexTable= 0;

	if(	ppi->ScanDisplayFragmentsAlloc )
		duck_free(ppi->ScanDisplayFragmentsAlloc);
	ppi->ScanDisplayFragmentsAlloc= 0;
	ppi->ScanDisplayFragments= 0;

    for(i = 0 ; i < MAX_PREV_FRAMES ; i ++) 
    {
    	if(	ppi->PrevFragmentsAlloc[i] )
		    duck_free(ppi->PrevFragmentsAlloc[i]);
	    ppi->PrevFragmentsAlloc[i] = 0;
	    ppi->PrevFragments[i] = 0;
    }

	if(	ppi->FragScoresAlloc )
		duck_free(ppi->FragScoresAlloc);
	ppi->FragScoresAlloc= 0;
	ppi->FragScores= 0;

	if(	ppi->SameGreyDirPixelsAlloc )
		duck_free(ppi->SameGreyDirPixelsAlloc);
	ppi->SameGreyDirPixelsAlloc= 0;
	ppi->SameGreyDirPixels= 0;

	if(	ppi->FragDiffPixelsAlloc )
		duck_free(ppi->FragDiffPixelsAlloc);
	ppi->FragDiffPixelsAlloc= 0;
	ppi->FragDiffPixels= 0;

	if(	ppi->BarBlockMapAlloc )
		duck_free(ppi->BarBlockMapAlloc);
	ppi->BarBlockMapAlloc= 0;
	ppi->BarBlockMap= 0;

	if(	ppi->TmpCodedMapAlloc )
		duck_free(ppi->TmpCodedMapAlloc);
	ppi->TmpCodedMapAlloc= 0;
	ppi->TmpCodedMap= 0;

	if(	ppi->RowChangedPixelsAlloc )
		duck_free(ppi->RowChangedPixelsAlloc);
	ppi->RowChangedPixelsAlloc= 0;
	ppi->RowChangedPixels= 0;

	if(	ppi->PixelScoresAlloc )
		duck_free(ppi->PixelScoresAlloc);
	ppi->PixelScoresAlloc= 0;
	ppi->PixelScores= 0;

	if(	ppi->PixelChangedMapAlloc )
		duck_free(ppi->PixelChangedMapAlloc);
	ppi->PixelChangedMapAlloc= 0;
	ppi->PixelChangedMap= 0;

	if(	ppi->ChLocalsAlloc )
		duck_free(ppi->ChLocalsAlloc);
	ppi->ChLocalsAlloc= 0;
	ppi->ChLocals= 0;

	if(	ppi->yuv_differencesAlloc )
		duck_free(ppi->yuv_differencesAlloc);
	ppi->yuv_differencesAlloc= 0;
	ppi->yuv_differences= 0;

}


#define ROUNDUP32(X) ( ( ( (unsigned long) X ) + 31 )&( 0xFFFFFFE0 ) )
/****************************************************************************
 * 
 *  ROUTINE       :     PAllocateFrameInfo
 *
 *
 *  INPUTS        :     Instance of PB to be initialized
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Initializes the Playback instance passed in
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
BOOL PAllocateFrameInfo(PP_INSTANCE * ppi)
{
    int i;
    PDeleteFrameInfo(ppi);

	ppi->ScanPixelIndexTableAlloc = duck_malloc(32 + ppi->ScanFrameFragments*sizeof(UINT32), DMEM_GENERAL);
    if(!ppi->ScanPixelIndexTableAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->ScanPixelIndexTable = (UINT32 *) ROUNDUP32(ppi->ScanPixelIndexTableAlloc);

	ppi->ScanDisplayFragmentsAlloc = duck_malloc(32 + ppi->ScanFrameFragments*sizeof(INT8), DMEM_GENERAL);
    if(!ppi->ScanDisplayFragmentsAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->ScanDisplayFragments = (INT8 *) ROUNDUP32(ppi->ScanDisplayFragmentsAlloc);

    for(i = 0 ; i < MAX_PREV_FRAMES ; i ++) 
    {
	    ppi->PrevFragmentsAlloc[i] = duck_malloc(32 + ppi->ScanFrameFragments*sizeof(INT8), DMEM_GENERAL);
        if(!ppi->PrevFragmentsAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	    ppi->PrevFragments[i] = (INT8 *) ROUNDUP32(ppi->PrevFragmentsAlloc[i]);
    }

	ppi->FragScoresAlloc = duck_malloc(32 + ppi->ScanFrameFragments*sizeof(UINT32), DMEM_GENERAL);
    if(!ppi->FragScoresAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->FragScores = (UINT32 *) ROUNDUP32(ppi->FragScoresAlloc);

	ppi->SameGreyDirPixelsAlloc = duck_malloc(32 + ppi->ScanFrameFragments*sizeof(INT8), DMEM_GENERAL);
    if(!ppi->SameGreyDirPixelsAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->SameGreyDirPixels = (INT8 *) ROUNDUP32(ppi->SameGreyDirPixelsAlloc);

	ppi->FragDiffPixelsAlloc = duck_malloc(32 + ppi->ScanFrameFragments*sizeof(UINT8), DMEM_GENERAL);
    if(!ppi->FragDiffPixelsAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->FragDiffPixels = (UINT8 *) ROUNDUP32(ppi->FragDiffPixelsAlloc);

	ppi->BarBlockMapAlloc = duck_malloc(32 + 3 * ppi->ScanHFragments*sizeof(INT8), DMEM_GENERAL);
    if(!ppi->BarBlockMapAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->BarBlockMap = (INT8 *) ROUNDUP32(ppi->BarBlockMapAlloc);

	ppi->TmpCodedMapAlloc = duck_malloc(32 + ppi->ScanHFragments*sizeof(INT8), DMEM_GENERAL);
    if(!ppi->TmpCodedMapAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->TmpCodedMap = (INT8 *) ROUNDUP32(ppi->TmpCodedMapAlloc);

	ppi->RowChangedPixelsAlloc = duck_malloc(32 + 3 * ppi->ScanConfig.VideoFrameHeight *sizeof(INT32), DMEM_GENERAL);
    if(!ppi->RowChangedPixelsAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->RowChangedPixels = (INT32 *) ROUNDUP32(ppi->RowChangedPixelsAlloc);

	ppi->PixelScoresAlloc = duck_malloc(32 + ppi->ScanConfig.VideoFrameWidth* sizeof(UINT8) * PSCORE_CB_ROWS, DMEM_GENERAL);
    if(!ppi->PixelScoresAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->PixelScores = (UINT8 *) ROUNDUP32(ppi->PixelScoresAlloc);

    ppi->PixelChangedMapAlloc = duck_malloc(32 + ppi->ScanConfig.VideoFrameWidth*sizeof(UINT8) * PMAP_CB_ROWS, DMEM_GENERAL);
    if(!ppi->PixelChangedMapAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->PixelChangedMap = ( UINT8 *) ROUNDUP32(ppi->PixelChangedMapAlloc);

    ppi->ChLocalsAlloc = duck_malloc(32 + ppi->ScanConfig.VideoFrameWidth*sizeof(UINT8) * CHLOCALS_CB_ROWS, DMEM_GENERAL);
    if(!ppi->ChLocalsAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->ChLocals = (UINT8 *) ROUNDUP32(ppi->ChLocalsAlloc);

    ppi->yuv_differencesAlloc = duck_malloc(32 + ppi->ScanConfig.VideoFrameWidth*sizeof(INT16) * YDIFF_CB_ROWS, DMEM_GENERAL);
    if(!ppi->yuv_differencesAlloc) {PDeleteFrameInfo(ppi);return FALSE;}
	ppi->yuv_differences = (INT16 *) ROUNDUP32(ppi->yuv_differencesAlloc);

    return TRUE;
}

/****************************************************************************
 * 
 *  ROUTINE       :     DeletePPInstance
 *
 *
 *  INPUTS        :     Instance of PB to be deleted
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     frees the Playback instance passed in
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
DLL void DeletePPInstance(PP_INSTANCE **ppi)
{
    PDeleteFrameInfo(*ppi);
	duck_free(*ppi);
	*ppi=0;
}

/****************************************************************************
 * 
 *  ROUTINE       :     Createppinstance
 *
 *
 *  INPUTS        :     Instance of CP to be initialized
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Create and Initializes the Compression instance 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
DLL PP_INSTANCE * CreatePPInstance(void)
{
	PP_INSTANCE *ppi;

	/* The pre-processor configuration. */
	SCAN_CONFIG_DATA ScanConfigInit =
	{
		NULL, NULL, NULL, NULL, NULL,
			176, 144, 
			8,8,
	};
    int i;

	// allocate structure
	int ppi_size = sizeof(PP_INSTANCE);
	ppi=duck_calloc(1,ppi_size, DMEM_GENERAL);

	ppi->OutputBlocksUpdated = 0;
	ppi->KFIndicator = 0;

	// Initializations
	ppi->PrevFrameLimit = 3;                          // Must not exceed MAX_PREV_FRAMES (Note that this number includes the  current frame so "1 = no effect")

    ppi->VideoYPlaneWidth = 0;
    ppi->VideoYPlaneHeight = 0;
    ppi->VideoUVPlaneWidth = 0;
    ppi->VideoUVPlaneHeight = 0;
    
    ppi->VideoYPlaneStride = 0;
    ppi->VideoUPlaneStride = 0;
    ppi->VideoVPlaneStride = 0;
    
    /* Scan control variables. */
	ppi->HFragPixels = 8;
	ppi->VFragPixels = 8;
    
    ppi->ScanFrameFragments = 0 ;
    ppi->ScanYPlaneFragments = 0;
    ppi->ScanUVPlaneFragments= 0;
    ppi->ScanHFragments= 0;
    ppi->ScanVFragments= 0;
    
    ppi->YFramePixels = 0; 
    ppi->UVFramePixels = 0; 
	
	ppi->SRFGreyThresh = 4;
	ppi->SRFColThresh = 5;
	ppi->NoiseSupLevel = 3;
	ppi->SgcLevelThresh = 3;
	ppi->SuvcLevelThresh = 4;
	
	// Variables controlling S.A.D. break outs.
	ppi->GrpLowSadThresh = 10;
	ppi->GrpHighSadThresh = 64;
	ppi->PrimaryBlockThreshold = 5;
	ppi->SgcThresh = 16;			   // (Default values for 8x8 blocks).
    
    ppi->PAKEnabled = FALSE; 
    
    ppi->LevelThresh = 0; 
    ppi->NegLevelThresh = 0;
    ppi->SrfThresh = 0; 
    ppi->NegSrfThresh = 0;
    ppi->HighChange = 0; 
    ppi->NegHighChange = 0;
    
    ppi->ModifiedGrpLowSadThresh = 0;
    ppi->ModifiedGrpHighSadThresh = 0;
    
    ppi->PlaneHFragments = 0; 
    ppi->PlaneVFragments = 0;
    ppi->PlaneHeight = 0;
    ppi->PlaneWidth = 0;
    ppi->PlaneStride = 0;
    
    ppi->BlockThreshold = 0; 
    ppi->BlockSgcThresh = 0;
    ppi->UVBlockThreshCorrection = 1.25;
    ppi->UVSgcCorrection = 1.5;
    
    // PC specific variables
    ppi->MmxEnabled = FALSE;
    
    ppi->YUVPlaneCorrectionFactor = 0;
    ppi->MaxLineSearchLen = MAX_SEARCH_LINE_LEN;
    
    ppi->YuvDiffsCircularBufferSize = 0;
    ppi->ChLocalsCircularBufferSize = 0;
    ppi->PixelMapCircularBufferSize = 0;
    
    // Function pointers for mmx switches
    ppi->RowSAD = 0;            


	ppi->ScanPixelIndexTableAlloc= 0;
	ppi->ScanPixelIndexTable= 0;

	ppi->ScanDisplayFragmentsAlloc= 0;
	ppi->ScanDisplayFragments= 0;

    for(i = 0 ; i < MAX_PREV_FRAMES ; i ++) 
    {
	    ppi->PrevFragmentsAlloc[i] = 0;
	    ppi->PrevFragments[i] = 0;
    }

	ppi->FragScores= 0;
	ppi->FragScores= 0;

	ppi->ScanDisplayFragmentsAlloc= 0;
	ppi->ScanDisplayFragments= 0;

	ppi->SameGreyDirPixelsAlloc= 0;
	ppi->SameGreyDirPixels= 0;

	ppi->FragDiffPixelsAlloc= 0;
	ppi->FragDiffPixels= 0;

	ppi->BarBlockMapAlloc= 0;
	ppi->BarBlockMap= 0;

	ppi->TmpCodedMapAlloc= 0;
	ppi->TmpCodedMap= 0;

	ppi->RowChangedPixelsAlloc= 0;
	ppi->RowChangedPixels= 0;

	ppi->PixelScoresAlloc= 0;
	ppi->PixelScores= 0;

	ppi->PixelChangedMapAlloc= 0;
	ppi->PixelChangedMap= 0;

	ppi->ChLocalsAlloc= 0;
	ppi->ChLocals= 0;

	ppi->yuv_differencesAlloc= 0;
	ppi->yuv_differences= 0;


	return ppi;
}
/****************************************************************************
 * 
 *  ROUTINE       :     VPInitLibrary
 *
 *
 *  INPUTS        :     init VP library
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Fully initializes the playback library
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void VPPInitLibrary(void)
{

}

/*********************************************************/


/****************************************************************************
 * 
 *  ROUTINE       :     VPPDeinitLibrary
 *
 *
 *  INPUTS        :     init VP library
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Fully initializes the playback library
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void VPPDeInitLibrary(void)
{

}


