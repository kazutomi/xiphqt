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
#define INC_WIN_HEADER      1
#include <windows.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>   
#include <stdio.h>  
#include "pbdll.h"
#include "yuvtofromrgb.h"
#include "cpuidlib.h"

// Functions that should only be used in assembly versions of the code
extern unsigned long GetProcessorFrequency();

extern void ClearMmx(void);
extern void MMX_idct(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
//
extern void MMX_idct10(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
extern void MMX_idct1(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
//
extern void MmxYUVtoRGB ( PB_INSTANCE *pbi, YUV_BUFFER_ENTRY_PTR yblock,	YUV_BUFFER_ENTRY_PTR ublock,	
	                      YUV_BUFFER_ENTRY_PTR vblock,	int uvoffset, 
                          BGR_TYPE * RGBPtr, BOOL ReconBuffer );			
extern void MMXClearDownQFragData(PB_INSTANCE *pbi);

#include "compdll.h"

#include "YUVtofromRGB.h"


extern UINT32  WmtGetHalfPixelSAD( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
						          UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  );
extern UINT32 WmtGetInterErr(  UINT8 * NewDataPtr, UINT8 * RefDataPtr1,  UINT8 * RefDataPtr2, UINT32 PixelsPerLine );

// MMX optimised funtion prototypes
extern UINT32 GetSumAbsDiffs( UINT8 * NewDataPtr, UINT8  * RefDataPtr, 
			 		  UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  );
extern UINT32 GetNextSumAbsDiffs( UINT8 * NewDataPtr, UINT8  * RefDataPtr, 
			 		  UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  );
extern UINT32 GetHalfPixelSumAbsDiffs( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
							   UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  );

extern UINT32  MmxGetSAD( UINT8 * NewDataPtr, UINT8  * RefDataPtr, 
			 	         UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  );
extern UINT32  MmxGetHalfPixelSAD( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
						          UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  );
extern UINT32 MmxGetInterErr(  UINT8 * NewDataPtr, UINT8 * RefDataPtr1,  UINT8 * RefDataPtr2, UINT32 PixelsPerLine );
extern UINT32 XmmGetInterErr(  UINT8 * NewDataPtr, UINT8 * RefDataPtr1,  UINT8 * RefDataPtr2, UINT32 PixelsPerLine );

extern UINT32 GetIntraError( UINT8* DataPtr, UINT32 PixelsPerline);
extern UINT32 MmxGetIntraError( UINT8* DataPtr, UINT32 PixelsPerline);

extern void fdct_MMX ( INT16 * InputData, INT16 * OutputData );

// XMM optimised funtion prototypes
extern UINT32  XMMGetSAD( UINT8 * NewDataPtr, UINT8  * RefDataPtr, 
			 	         UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  );


extern INT32 GetBlockReconErrorXMM  ( CP_INSTANCE *cpi, INT32 BlockIndex );

extern UINT8 TokenizeDctBlockMmx( INT16 * RawData, UINT32 * TokenListPtr );
extern UINT8 TokenizeDctBlock( INT16 * RawData, UINT32 * TokenListPtr );
extern void MmxSUB8( UINT8 *FiltPtr, UINT8 *ReconPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine );
extern void MmxSUB8_128( UINT8 *FiltPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine );
extern void MmxSUB8AV2( UINT8 *FiltPtr, UINT8 *ReconPtr1, UINT8 *ReconPtr2, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
                  UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine );
extern void CopyBlockMMX(unsigned char *src, unsigned char *dest, unsigned int srcstride);

#if defined(POSTPROCESS)
extern void FDct1d4 (INT16 *InputData, INT16 * OutputData);
extern void FDct1D4Mmx (INT16 *InputData, INT16 * OutputData);
extern void IDct4( INT16 *InputData, INT16 *OutputData);
extern void MMX_idct3(INT16 *InputData, INT16 *OutputData);
#endif


#include "Quantize.h"
#include "dct.h"
#include "Reconstruct.h"


/****************************************************************************
*  Explicit imports
*****************************************************************************
*/

//extern MmxEnabled;          // Is MMX enabled flag


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
#define MMX_ENABLED 1
void CMachineSpecificConfig(CP_INSTANCE *cpi)
{
    UINT32 FeatureFlags = 0;
    BOOL   CPUID_Supported = TRUE;   // Is the CPUID instruction supported
    BOOL   TestMmx = TRUE;
	UINT32 i;

	// Should have function returning what features are supported
	//  but instead we have just cpuid so check for mmx, xmm 
	PROCTYPE CPUType = findCPUId();
	switch(CPUType)
	{
	case X86    :
	case PPRO   :
	case C6X86  :
	case C6X86MX:
	case AMDK5  :
	case MACG3	:
	case MAC68K	:
		cpi->pb.MmxEnabled = FALSE;
		cpi->pb.XmmEnabled = FALSE;
		cpi->pb.WmtEnabled = FALSE;
		break;
	case PII	:   
	case AMDK63D:
	case AMDK6  :
	case PMMX	:   
		cpi->pb.MmxEnabled = TRUE;
		cpi->pb.XmmEnabled = FALSE;
		cpi->pb.WmtEnabled = FALSE;
		break;
	case XMM    :
		cpi->pb.MmxEnabled = TRUE;
		cpi->pb.XmmEnabled = TRUE;
		cpi->pb.WmtEnabled = FALSE;
		break;
	case WMT	:
		cpi->pb.MmxEnabled = TRUE;
		cpi->pb.XmmEnabled = TRUE;
		cpi->pb.WmtEnabled = TRUE;
		break;
	}

	/* For test purpose, we force machine type here */
	//cpi->pb.MmxEnabled = FALSE;
	//cpi->pb.XmmEnabled = FALSE;
    //cpi->pb.WmtEnabled = FALSE;


	// If MMX supported then set to use MMX versions of functions else 
    // use original 'C' versions.
	cpi->pb.BuildQuantIndex=BuildQuantIndex_Generic;
	if( cpi->pb.WmtEnabled )
    {
    
		cpi->GetSAD = XMMGetSAD; //MmxGetSAD;
		cpi->GetNextSAD = XMMGetSAD; //MmxGetSAD;
		cpi->GetSadHalfPixel = WmtGetHalfPixelSAD;  		
        cpi->GetInterError = WmtGetInterErr;
		cpi->GetIntraError = MmxGetIntraError;
		cpi->GetBlockReconError=GetBlockReconErrorXMM;	//XMM version, Not MMX version


        cpi->fdct_short=fdct_MMX;
        cpi->pb.YUVtoRGB = MmxYUVtoRGB;
		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{
			if(i<=1)cpi->pb.idct[i]=MMX_idct1;
			else if(i<=10)cpi->pb.idct[i]=MMX_idct10;
			else cpi->pb.idct[i]=MMX_idct;
		}

        cpi->pb.ReconIntra = MMXReconIntra;
        cpi->pb.ReconInter = MmxReconInter;
        cpi->pb.ReconInterHalfPixel2 = MmxReconInterHalfPixel2;
		cpi->pb.ClearDownQFrag = MMXClearDownQFragData;
		cpi->pb.ClearSysState = ClearMmx;
		cpi->TokenizeDctBlock=TokenizeDctBlockMmx;

        cpi->Sub8 = MmxSUB8;
        cpi->Sub8_128 = MmxSUB8_128;
        cpi->Sub8Av2 = MmxSUB8AV2;//XmmSUB8AV2;
		
		cpi->pb.FilterHoriz = FilterHoriz_MMX;
        cpi->pb.FilterVert = FilterVert_MMX;
        cpi->pb.SetupBoundingValueArray = SetupBoundingValueArray_ForMMX;
		cpi->pb.CopyBlock = CopyBlockMMX;


#if	defined(POSTPROCESS)

		// check the CPU to see if postprocess is able to be done without
		// slow down the playback
		cpi->pb.ProcessorFrequency = GetProcessorFrequency();

#endif
    
    
    }
    else if ( cpi->pb.XmmEnabled )
	{
		cpi->GetSAD = XMMGetSAD; //MmxGetSAD;
		cpi->GetNextSAD = XMMGetSAD; //MmxGetSAD;
		cpi->GetSadHalfPixel = MmxGetHalfPixelSAD;  		
        cpi->GetInterError = MmxGetInterErr;
		cpi->GetIntraError = MmxGetIntraError;
		cpi->GetBlockReconError=GetBlockReconErrorXMM;	//XMM version, Not MMX version


        cpi->fdct_short=fdct_MMX;
        cpi->pb.YUVtoRGB = MmxYUVtoRGB;
		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{
			if(i<=1)cpi->pb.idct[i]=MMX_idct1;
			else if(i<=10)cpi->pb.idct[i]=MMX_idct10;
			else cpi->pb.idct[i]=MMX_idct;
		}

        cpi->pb.ReconIntra = MMXReconIntra;
        cpi->pb.ReconInter = MmxReconInter;
        cpi->pb.ReconInterHalfPixel2 = MmxReconInterHalfPixel2;
		cpi->pb.ClearDownQFrag = MMXClearDownQFragData;
		cpi->pb.ClearSysState = ClearMmx;
		cpi->TokenizeDctBlock=TokenizeDctBlockMmx;

        cpi->Sub8 = MmxSUB8;
        cpi->Sub8_128 = MmxSUB8_128;
        cpi->Sub8Av2 = MmxSUB8AV2;//XmmSUB8AV2;
		
		cpi->pb.FilterHoriz = FilterHoriz_MMX;
        cpi->pb.FilterVert = FilterVert_MMX;
        cpi->pb.SetupBoundingValueArray = SetupBoundingValueArray_ForMMX;
		cpi->pb.CopyBlock = CopyBlockMMX;


#if	defined(POSTPROCESS)

		// check the CPU to see if postprocess is able to be done without
		// slow down the playback
		cpi->pb.ProcessorFrequency = GetProcessorFrequency();

#endif



	}
	else if ( cpi->pb.MmxEnabled )
    {
        cpi->GetSAD = MmxGetSAD;
        cpi->GetNextSAD = MmxGetSAD;
        cpi->GetSadHalfPixel = MmxGetHalfPixelSAD;
        cpi->GetInterError = MmxGetInterErr;
		cpi->GetIntraError = MmxGetIntraError;
		cpi->GetBlockReconError = GetBlockReconErrorSlow;
        cpi->pb.YUVtoRGB = MmxYUVtoRGB;
		for(i=0;i<=64;i++)
		{
			if(i<=1)cpi->pb.idct[i]=MMX_idct1;
			else if(i<=10)cpi->pb.idct[i]=MMX_idct10;
			else cpi->pb.idct[i]=MMX_idct;
		}
        cpi->pb.ReconIntra = MMXReconIntra;
        cpi->pb.ReconInter = MmxReconInter;
        cpi->pb.ReconInterHalfPixel2 = MmxReconInterHalfPixel2;
		cpi->pb.ClearDownQFrag = MMXClearDownQFragData;
		cpi->fdct_short=fdct_MMX;
		cpi->pb.ClearSysState = ClearMmx;
		cpi->TokenizeDctBlock=TokenizeDctBlockMmx;

	    cpi->pb.FilterHoriz = FilterHoriz_MMX;
        cpi->pb.FilterVert = FilterVert_MMX;
        cpi->pb.SetupBoundingValueArray = SetupBoundingValueArray_ForMMX;
		cpi->pb.CopyBlock = CopyBlockMMX;

        cpi->Sub8 = MmxSUB8;
        cpi->Sub8_128 = MmxSUB8_128;
        cpi->Sub8Av2 = MmxSUB8AV2;

#if	defined(POSTPROCESS)

		// check the CPU to see if postprocess is able to be done without
		// slow down the playback
		cpi->pb.ProcessorFrequency = GetProcessorFrequency();

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

}
