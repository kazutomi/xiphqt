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
*   Module Title :     COptSystemDependant.c
*
*   Description  :     Miscellaneous system dependant functions
*
*
*****************************************************************************
*/

/*******************************************3*********************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include <string.h>
#include <time.h>
#include <stdlib.h>   
#include <stdio.h>  

#include "pbdll.h"
#include "yuvtofromrgb.h"
#include "compdll.h"
#include "mcomp.h"
#include "YUVtofromRGB.h"
#include "dct.h"

#include "Quantize.h"
#include "Reconstruct.h"

#include "resource.h"    /* Resource IDs. */  


#include <gestalt.h>


/****************************************************************************
*  Explicit imports
*****************************************************************************
*/
extern void 
fdct_Altivec( INT16 * InputData, INT16 * OutputData );

extern UINT32 
GetSAD_Altivec( UINT8 * NewDataPtr, UINT8  * RefDataPtr, UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar);

extern UINT32 
GetHalfPixelSAD_Altivec( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  );



extern void PPCReconIntra( PB_INSTANCE *pbi, UINT8 * dest, UINT16 * diff, UINT32 stride);
extern void PPCReconInter(  PB_INSTANCE *pbi, UINT8 * dest, UINT8 * ref, INT16 * diff, UINT32 stride);
extern void PPCReconInterHalfPixel2( PB_INSTANCE *pbi, UINT8* dest, UINT8* r, UINT8* s, INT16* diff, UINT32 stride);

extern void PPCReconIntra_Altivec( PB_INSTANCE *pbi, UINT8 * dest, UINT16 * diff, UINT32 stride);
extern void PPCReconInter_Altivec(  PB_INSTANCE *pbi, UINT8 * dest, UINT8 * ref, INT16 * diff, UINT32 stride);
extern void PPCReconInterHalfPixel2_Altivec( PB_INSTANCE *pbi, UINT8* dest, UINT8* RefPtr1, UINT8* RefPtr12, INT16 * diff, UINT32 stride);


extern INT32 *SetupBoundingValueArray_ForPPC(PB_INSTANCE *pbi, INT32 FLimit);
extern void FilterHoriz_PPC(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void FilterVert_PPC(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);

extern INT32 *SetupBoundingValueArray_Altivec(PB_INSTANCE *pbi, INT32 FLimit);
extern void FilterHoriz_Altivec(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void FilterVert_Altivec(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);






extern UINT8 TokenizeDctBlock( INT16 * RawData, UINT32 * TokenListPtr );

#if defined(POSTPROCESS)
extern void FDct1d4 (INT16 *InputData, INT16 * OutputData);
extern void IDct4( INT16 *InputData, INT16 *OutputData);
extern void ReconPostProcess( PB_INSTANCE *pbi, 
								 UINT8 * DestPtr, 
								 UINT8 * SrcPtr, 
								 INT16 * ChangePtr, 
								UINT32 PlaneLineStep );

#endif 
/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
 

/****************************************************************************
*  Module statics.
*****************************************************************************
*/        

              
/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/


/****************************************************************************
*  Functions
*****************************************************************************
*/

/****************************************************************************
 * 
 *  ROUTINE       :     MachineSpecificConfig
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Checks for machine specifc features such as MMX support 
 *                      sets approipriate flags and function pointers.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CMachineSpecificConfig(CP_INSTANCE *cpi)
{
	int i;
	
	 
	OSErr err; 
	long processorAttributes = 0;
	long hasAltiVec = 0;
 
 	err = Gestalt(gestaltPowerPCProcessorFeatures, &processorAttributes);
	if (err == noErr)
    	hasAltiVec = (1 << gestaltPowerPCHasVectorInstructions) & processorAttributes;

 
    cpi->pb.BuildQuantIndex = BuildQuantIndex_Generic;

    // optimized versions.
  	if(hasAltiVec)
    {
//        cpi->GetSAD = GetSumAbsDiffs;
//        cpi->GetNextSAD = GetNextSumAbsDiffs;
//        cpi->GetSadHalfPixel = GetHalfPixelSumAbsDiffs;
        
        cpi->GetSAD = GetSAD_Altivec;
        cpi->GetNextSAD = GetSAD_Altivec;
        cpi->GetSadHalfPixel = GetHalfPixelSAD_Altivec;

        cpi->GetInterError = GetInterErr;
		cpi->GetIntraError = GetIntraError;
		cpi->GetBlockReconError = GetBlockReconErrorSlow;
        cpi->pb.YUVtoRGB = ScalarYUVtoRGB;
		
		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{
			if(i<=1)
				cpi->pb.idct[i]=IDct1;
			else if(i<=10)
				cpi->pb.idct[i]=IDct10;
			else 
				cpi->pb.idct[i]=IDctSlow;
		}
		
        cpi->pb.ReconIntra = PPCReconIntra_Altivec;
        cpi->pb.ReconInter = PPCReconInter_Altivec;
        cpi->pb.ReconInterHalfPixel2 = PPCReconInterHalfPixel2_Altivec;
		cpi->pb.ClearDownQFrag = ClearDownQFragData;
		cpi->pb.ClearSysState = ClearSysState;
		cpi->fdct_short = fdct_Altivec;
//		cpi->fdct_short = fdct_short;

        cpi->Sub8 = SUB8;
        cpi->Sub8_128 = SUB8_128;
        cpi->Sub8Av2 = SUB8AV2;
		cpi->TokenizeDctBlock=TokenizeDctBlock;

		cpi->pb.FilterHoriz = FilterHoriz_Altivec;
        cpi->pb.FilterVert = FilterVert_Altivec;
        cpi->pb.SetupBoundingValueArray = SetupBoundingValueArray_Altivec;
		cpi->pb.CopyBlock = CopyBlock;


#if	defined(POSTPROCESS)
		cpi->pb.ProcessorFrequency = 0;		
#endif
	
	}
	else
	{	
        cpi->GetSAD = GetSumAbsDiffs;
        cpi->GetNextSAD = GetNextSumAbsDiffs;
        cpi->GetSadHalfPixel = GetHalfPixelSumAbsDiffs;
        cpi->GetInterError = GetInterErr;
		cpi->GetIntraError = GetIntraError;
		cpi->GetBlockReconError = GetBlockReconErrorSlow;
        cpi->pb.YUVtoRGB = ScalarYUVtoRGB;
		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{
			if(i<=1)
				cpi->pb.idct[i]=IDct1;
			else if(i<=10)
				cpi->pb.idct[i]=IDct10;
			else 
				cpi->pb.idct[i]=IDctSlow;
		}

//        cpi->pb.ReconIntra = ScalarReconIntra;
//        cpi->pb.ReconInter = ScalarReconInter;
//        cpi->pb.ReconInterHalfPixel2 = ScalarReconInterHalfPixel2;
        
        cpi->pb.ReconIntra = PPCReconIntra;
        cpi->pb.ReconInter = PPCReconInter;
        cpi->pb.ReconInterHalfPixel2 = PPCReconInterHalfPixel2;
        
                
		cpi->pb.ClearDownQFrag = ClearDownQFragData;
		cpi->pb.ClearSysState = ClearSysState;
		cpi->fdct_short=fdct_short;
        cpi->Sub8 = SUB8;
        cpi->Sub8_128 = SUB8_128;
        cpi->Sub8Av2 = SUB8AV2;
		cpi->TokenizeDctBlock=TokenizeDctBlock;

		cpi->pb.FilterHoriz = FilterHoriz_Generic;
        cpi->pb.FilterVert = FilterVert_Generic;
        cpi->pb.SetupBoundingValueArray = SetupBoundingValueArray_Generic;
		cpi->pb.CopyBlock = CopyBlock;

#if	defined(POSTPROCESS)
		cpi->pb.ProcessorFrequency = 0;		
#endif

	}
}

