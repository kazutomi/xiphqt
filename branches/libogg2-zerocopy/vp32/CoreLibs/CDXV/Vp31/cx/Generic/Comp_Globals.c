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
*   Module Title :     Comp_Globals.c
*
*   Description  :     Video CODEC Demo : Compression DLL global declarations
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
#include "Mcomp.h"
#include <stdlib.h>
#include <math.h>
#include "duck_mem.h"
#define ROUNDUP32(X) ( ( ( (unsigned long) X ) + 31 )&( 0xFFFFFFE0 ) )
/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
 

                
/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/


/****************************************************************************
*  Explicit Imports
*****************************************************************************
*/
extern INT32 *XX_LUT;
extern INT32 SquaredErrorTable[511];
extern INT32 AbsXTable[511];

extern void DeleteTmpBuffers(PB_INSTANCE * pbi);
extern BOOL AllocateTmpBuffers(PB_INSTANCE * pbi);
extern void VPInitLibrary(void);
extern void VPDeInitLibrary(void);
//CP_INSTANCE cpi_v;
//CP_INSTANCE *cpi;//=&cpi_v;
/****************************************************************************
 * 
 *  ROUTINE       :     EDeleteFragmentInfo
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
void EDeleteFragmentInfo(CP_INSTANCE * cpi)
{

	// duck_free prior allocs if present
	if(	cpi->extra_fragmentsAlloc)
		duck_free(cpi->extra_fragmentsAlloc);
	if(	cpi->FragmentLastQAlloc)
		duck_free(cpi->FragmentLastQAlloc);
	if(	cpi->FragTokensAlloc)
		duck_free(cpi->FragTokensAlloc);
	if(	cpi->FragTokenCountsAlloc)
		duck_free(cpi->FragTokenCountsAlloc);
	if(	cpi->RunHuffIndicesAlloc)
		duck_free(cpi->RunHuffIndicesAlloc);
	if(	cpi->LastCodedErrorScoreAlloc)
		duck_free(cpi->LastCodedErrorScoreAlloc);
	if(cpi->ModeListAlloc)
		duck_free(cpi->ModeListAlloc);
	if(cpi->MVListAlloc)
		duck_free(cpi->MVListAlloc);
	if( cpi->DCT_codesAlloc )
		duck_free( cpi->DCT_codesAlloc );
	if( cpi->DCTDataBufferAlloc )
		duck_free( cpi->DCTDataBufferAlloc);
	if( cpi->quantized_listAlloc);
		duck_free( cpi->quantized_listAlloc);
	if( cpi->OriginalDCAlloc);
		duck_free( cpi->OriginalDCAlloc);
   if( cpi->PartiallyCodedFlags)
        duck_free(cpi->PartiallyCodedFlags);
   if( cpi->PartiallyCodedMbPatterns)
        duck_free(cpi->PartiallyCodedMbPatterns);
   if( cpi->UncodedMbFlags)
        duck_free(cpi->UncodedMbFlags);

	if(	cpi->BlockCodedFlagsAlloc)
		duck_free(cpi->BlockCodedFlagsAlloc);

    cpi->extra_fragmentsAlloc = 0;
	cpi->FragmentLastQAlloc = 0;
	cpi->FragTokensAlloc = 0;
	cpi->FragTokenCountsAlloc = 0;
	cpi->RunHuffIndicesAlloc = 0;
	cpi->LastCodedErrorScoreAlloc = 0;
	cpi->ModeListAlloc = 0;
	cpi->MVListAlloc = 0;
	cpi->DCT_codesAlloc = 0;
	cpi->DCTDataBufferAlloc = 0;
	cpi->quantized_listAlloc = 0;
    cpi->OriginalDCAlloc = 0;

	cpi->extra_fragments = 0;
	cpi->FragmentLastQ = 0;
	cpi->FragTokens = 0;
	cpi->FragTokenCounts = 0;
	cpi->RunHuffIndices = 0;
	cpi->LastCodedErrorScore = 0;
	cpi->ModeList = 0;
	cpi->MVList = 0;
	cpi->DCT_codes = 0;
	cpi->DCTDataBuffer = 0;
	cpi->quantized_list = 0;
    cpi->OriginalDC = 0;
    cpi->FixedQ = 0;

	cpi->BlockCodedFlagsAlloc = 0;
	cpi->BlockCodedFlags = 0;
}

/****************************************************************************
 * 
 *  ROUTINE       :     EAllocateFragmentInfo
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
BOOL EAllocateFragmentInfo(CP_INSTANCE * cpi)
{

	// clear any existing info
	EDeleteFragmentInfo(cpi);

	// Perform Fragment Allocations
	cpi->extra_fragmentsAlloc =  duck_malloc(32+cpi->pb.UnitFragments*sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->extra_fragmentsAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->FragmentLastQAlloc =  duck_malloc(32+cpi->pb.UnitFragments*sizeof(UINT32), DMEM_GENERAL);
    if(!cpi->FragmentLastQAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->FragTokensAlloc =  duck_malloc(32+cpi->pb.UnitFragments*sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->FragTokensAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->OriginalDCAlloc=duck_malloc(32+cpi->pb.UnitFragments*sizeof(INT16), DMEM_GENERAL);
    if(!cpi->OriginalDCAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->FragTokenCountsAlloc =  duck_malloc(32+cpi->pb.UnitFragments * sizeof(UINT32), DMEM_GENERAL);
    if(!cpi->FragTokenCountsAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->RunHuffIndicesAlloc =  duck_malloc(32+cpi->pb.UnitFragments*sizeof(INT32), DMEM_GENERAL);
    if(!cpi->RunHuffIndicesAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->LastCodedErrorScoreAlloc =  duck_malloc(32+cpi->pb.UnitFragments*sizeof(UINT32), DMEM_GENERAL);
    if(!cpi->LastCodedErrorScoreAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->BlockCodedFlagsAlloc =  duck_malloc(32+cpi->pb.UnitFragments*sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->BlockCodedFlagsAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->ModeListAlloc =  duck_malloc(32+cpi->pb.UnitFragments * sizeof(UINT32 ), DMEM_GENERAL);
    if(!cpi->ModeListAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->MVListAlloc =  duck_malloc(32+cpi->pb.UnitFragments * sizeof(MOTION_VECTOR), DMEM_GENERAL);
    if(!cpi->MVListAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->DCT_codesAlloc=duck_malloc(32+64*sizeof(INT16), DMEM_GENERAL);
    if(!cpi->DCT_codesAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->DCTDataBufferAlloc=duck_malloc(32+64*sizeof(INT16), DMEM_GENERAL);
    if(!cpi->DCTDataBufferAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->quantized_listAlloc=duck_malloc(32+64*sizeof(Q_LIST_ENTRY), DMEM_GENERAL);
    if(!cpi->quantized_listAlloc) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->PartiallyCodedFlags = duck_malloc(32 + cpi->pb.MacroBlocks * sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->PartiallyCodedFlags) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->PartiallyCodedMbPatterns = duck_malloc(32 + cpi->pb.MacroBlocks * sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->PartiallyCodedMbPatterns) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->UncodedMbFlags = duck_malloc(32 + cpi->pb.MacroBlocks * sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->UncodedMbFlags) { EDeleteFragmentInfo(cpi); return FALSE; }

	cpi->extra_fragments = (UINT8 *) ROUNDUP32(cpi->extra_fragmentsAlloc);
	cpi->FragmentLastQ = (UINT32 *) ROUNDUP32(cpi->FragmentLastQAlloc);
	cpi->FragTokens = (UINT8 *) ROUNDUP32(cpi->FragTokensAlloc );
	cpi->OriginalDC = (INT16 *) ROUNDUP32(cpi->OriginalDCAlloc );
	cpi->FragTokenCounts = (UINT32 *) ROUNDUP32(cpi->FragTokenCountsAlloc); 
	cpi->RunHuffIndices = (UINT32 *) ROUNDUP32(cpi->RunHuffIndicesAlloc);
	cpi->LastCodedErrorScore = (UINT32 *) ROUNDUP32(cpi->LastCodedErrorScoreAlloc);
	cpi->ModeList = (UINT32 *) ROUNDUP32(cpi->ModeListAlloc);
	cpi->MVList = (MOTION_VECTOR *) ROUNDUP32(cpi->MVListAlloc); 
	cpi->DCT_codes=(INT16 *)ROUNDUP32(cpi->DCT_codesAlloc);
	cpi->DCTDataBuffer=(INT16 *)ROUNDUP32(cpi->DCTDataBufferAlloc);
	cpi->quantized_list =(Q_LIST_ENTRY *)ROUNDUP32(cpi->quantized_listAlloc);
	cpi->BlockCodedFlags = (UINT8 *) ROUNDUP32(cpi->BlockCodedFlagsAlloc); 

    return TRUE;
}

/****************************************************************************
 * 
 *  ROUTINE       :     EDeleteFrameInfo
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
void EDeleteFrameInfo(CP_INSTANCE * cpi)
{
	if(cpi->ConvDestBufferAlloc )
		duck_free(cpi->ConvDestBufferAlloc );
	cpi->ConvDestBufferAlloc = 0;
	cpi->ConvDestBuffer = 0;

    if(cpi->yuv0ptrAlloc)
		duck_free(cpi->yuv0ptrAlloc);
	cpi->yuv0ptrAlloc = 0;
	cpi->yuv0ptr = 0;

    if(cpi->yuv1ptrAlloc)
		duck_free(cpi->yuv1ptrAlloc);
	cpi->yuv1ptrAlloc = 0;
	cpi->yuv1ptr = 0;

    if(	cpi->OptimisedTokenListEbAlloc )
		duck_free(cpi->OptimisedTokenListEbAlloc);
	cpi->OptimisedTokenListEbAlloc = 0;
	cpi->OptimisedTokenListEb = 0;

    if(	cpi->OptimisedTokenListAlloc )
		duck_free(cpi->OptimisedTokenListAlloc);
	cpi->OptimisedTokenListAlloc = 0;
	cpi->OptimisedTokenList = 0;

    if(	cpi->OptimisedTokenListHiAlloc )
		duck_free(cpi->OptimisedTokenListHiAlloc);
	cpi->OptimisedTokenListHiAlloc = 0;
	cpi->OptimisedTokenListHi = 0;

    if(	cpi->OptimisedTokenListPlAlloc )
		duck_free(cpi->OptimisedTokenListPlAlloc);
	cpi->OptimisedTokenListPlAlloc = 0;
	cpi->OptimisedTokenListPl = 0;


}


/****************************************************************************
 * 
 *  ROUTINE       :     EAllocateFrameInfo
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
BOOL EAllocateFrameInfo(CP_INSTANCE * cpi)
{
	int FrameSize = cpi->pb.ReconYPlaneSize + 2 * cpi->pb.ReconUVPlaneSize;

	// clear any existing info
	EDeleteFrameInfo(cpi);

	// allocate frames
	cpi->ConvDestBufferAlloc = duck_malloc(32+FrameSize*sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->ConvDestBufferAlloc) { EDeleteFrameInfo(cpi); return FALSE; }
	cpi->ConvDestBuffer = (UINT8 *) ROUNDUP32(cpi->ConvDestBufferAlloc);

    cpi->yuv0ptrAlloc = duck_malloc(32+FrameSize*sizeof(YUV_BUFFER_ENTRY ), DMEM_GENERAL);
    if(!cpi->yuv0ptrAlloc) { EDeleteFrameInfo(cpi); return FALSE; }
	cpi->yuv0ptr = (UINT8 *) ROUNDUP32(cpi->yuv0ptrAlloc);

    cpi->yuv1ptrAlloc = duck_malloc(32+FrameSize*sizeof(YUV_BUFFER_ENTRY), DMEM_GENERAL);
    if(!cpi->yuv1ptrAlloc) { EDeleteFrameInfo(cpi); return FALSE; }
	cpi->yuv1ptr = (UINT8 *) ROUNDUP32(cpi->yuv1ptrAlloc);

    cpi->OptimisedTokenListEbAlloc = duck_malloc(32+FrameSize*sizeof(UINT32), DMEM_GENERAL);
    if(!cpi->OptimisedTokenListEbAlloc) { EDeleteFrameInfo(cpi); return FALSE; }
	cpi->OptimisedTokenListEb = (UINT32 *) ROUNDUP32(cpi->OptimisedTokenListEbAlloc);

    cpi->OptimisedTokenListAlloc = duck_malloc(32+FrameSize*sizeof(UINT8 ), DMEM_GENERAL);
    if(!cpi->OptimisedTokenListAlloc) { EDeleteFrameInfo(cpi); return FALSE; }
	cpi->OptimisedTokenList = (UINT8 *) ROUNDUP32(cpi->OptimisedTokenListAlloc); 

    cpi->OptimisedTokenListHiAlloc = duck_malloc(32 + FrameSize*sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->OptimisedTokenListHiAlloc) { EDeleteFrameInfo(cpi); return FALSE; }
	cpi->OptimisedTokenListHi = (UINT8 *) ROUNDUP32(cpi->OptimisedTokenListHiAlloc);

    cpi->OptimisedTokenListPlAlloc = duck_malloc(32 + FrameSize*sizeof(UINT8), DMEM_GENERAL);
    if(!cpi->OptimisedTokenListPlAlloc) { EDeleteFrameInfo(cpi); return FALSE; }
	cpi->OptimisedTokenListPl = (UINT8 *) ROUNDUP32(cpi->OptimisedTokenListPlAlloc);

    return TRUE;

}

/****************************************************************************
 * 
 *  ROUTINE       :     DeleteCPInstance
 *
 *
 *  INPUTS        :     Instance of PB to be deleted
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     duck_frees the Playback instance passed in
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void DeleteCPInstance(CP_INSTANCE **cpi)
{
	DeleteTmpBuffers(&(*cpi)->pb);
	DeletePPInstance(&(*cpi)->pp);
	duck_free(*cpi);
	*cpi=0;
}


/****************************************************************************
 * 
 *  ROUTINE       :     CreateCPInstance
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
CP_INSTANCE * CreateCPInstance(void)
{
	CP_INSTANCE *cpi;

    UINT32  i;

	// allocate structure
	int cpi_size = sizeof(CP_INSTANCE);
	cpi=duck_malloc(cpi_size, DMEM_GENERAL);
    if(!cpi)
        return 0;

	// initialize whole structure to 0
	memset((unsigned char *) cpi, 0, sizeof(CP_INSTANCE));
	if(!AllocateTmpBuffers(&cpi->pb))
    {
        DeleteCPInstance(&cpi);
        return 0;
    }

	// Create a pre-processor instance
	cpi->pp = CreatePPInstance();

	// Initialise Configuration structure to legal values
    cpi->Configuration.BaseQ = 32;
    cpi->Configuration.FirstFrameQ = 32;
    cpi->Configuration.MaxQ = 32;
    cpi->Configuration.ActiveMaxQ = 32;
    cpi->Configuration.OutputFrameRate = 30;
    cpi->Configuration.TargetBandwidth = 3000;

    cpi->MVChangeFactor    =    14;     
    cpi->FourMvChangeFactor =   8;           
    cpi->MinImprovementForNewMV = 25;   
    cpi->ExhaustiveSearchThresh = 2500;
    cpi->MinImprovementForFourMV = 100;   
    cpi->FourMVThreshold = 10000;
    cpi->BitRateCapFactor = 1.50;    
    cpi->InterTripOutThresh = 5000;
    cpi->MVEnabled = TRUE;
    cpi->InterCodeCount = 127;
    cpi->BpbCorrectionFactor = 1.0;
    cpi->GoldenFrameEnabled = TRUE;
    cpi->InterPrediction = TRUE;
    cpi->MotionCompensation = TRUE;
    cpi->ThreshMapThreshold = 5;
    cpi->QuickCompress = TRUE;
	cpi->MaxConsDroppedFrames = 1;
	cpi->Sharpness = 2;

    cpi->PreProcFilterLevel = 2;

    // Set up default values for QTargetModifier[Q_TABLE_SIZE] table
    for ( i = 0; i < Q_TABLE_SIZE; i++ )
    {
        cpi->QTargetModifier[Q_TABLE_SIZE] = 1.0;
    }

    return cpi;
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
void VPEInitLibrary(void)
{
	int i;

	// initialise the stuff in the decompressor
	VPInitLibrary();

	// Prepare table of squared error  values.
	XX_LUT = &SquaredErrorTable[255];
	for ( i=(-255); i<=255; i++ )
	{
		XX_LUT[i] = i*i;
	}

    // Prepare Abs difference lookup table
    AbsX_LUT = &AbsXTable[255];
	for ( i=(-255); i<=255; i++ )
    {
        AbsX_LUT[i] = abs(i);
    }

}

/*********************************************************/


/****************************************************************************
 * 
 *  ROUTINE       :     VPEDeinitLibrary
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
void VPEDeInitLibrary(void)
{

	// deinitialize decompressor libraries 
	VPDeInitLibrary();
}





