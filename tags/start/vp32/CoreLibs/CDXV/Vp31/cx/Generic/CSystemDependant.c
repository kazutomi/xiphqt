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
*   Module Title :     SystemDependant.c
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

/****************************************************************************
*  Explicit imports
*****************************************************************************
*/

extern UINT8 TokenizeDctBlock( INT16 * RawData, UINT32 * TokenListPtr );

#if defined(POSTPROCESS)
extern void FDct1d4 (INT16 *InputData, INT16 * OutputData);
extern void IDct4( INT16 *InputData, INT16 *OutputData);

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
		if(i<=1)cpi->pb.idct[i]=IDct1;
		else if(i<=10)cpi->pb.idct[i]=IDct10;
		else cpi->pb.idct[i]=IDctSlow;
	}
	cpi->pb.ReconIntra = ScalarReconIntra;
	cpi->pb.ReconInter = ScalarReconInter;
	cpi->pb.ReconInterHalfPixel2 = ScalarReconInterHalfPixel2;
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
	cpi->pb.ProcessorFrequency = 0;		/* force it to 0 Mhz when no MMX support */
#endif
    
}

