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
//#include "cpuidlib.h"


#include <gestalt.h>
#include "vfw_pb_interface.h"

// Functions that should only be used in assembly versions of the code


extern void PPCReconIntra( PB_INSTANCE *pbi, UINT8 * dest, UINT16 * diff, UINT32 stride);
extern void PPCReconInter(  PB_INSTANCE *pbi, UINT8 * dest, UINT8 * ref, INT16 * diff, UINT32 stride);
extern void PPCReconInterHalfPixel2( PB_INSTANCE *pbi, UINT8* dest, UINT8* r, UINT8* s, INT16* diff, UINT32 stride);

extern void PPCReconIntra_Altivec( PB_INSTANCE *pbi, UINT8 * dest, UINT16 * diff, UINT32 stride);
extern void PPCReconInter_Altivec(  PB_INSTANCE *pbi, UINT8 * dest, UINT8 * ref, INT16 * diff, UINT32 stride);
extern void PPCReconInterHalfPixel2_Altivec( PB_INSTANCE *pbi, UINT8* dest, UINT8* RefPtr1, UINT8* RefPtr12, INT16 * diff, UINT32 stride);

extern void UnPackVideo(PB_INSTANCE *pbi);
extern void UnPackVideoPPC(PB_INSTANCE *pbi);
extern void UnPackVideoPPC_LL (PB_INSTANCE *pbi);

extern void IDct3_DX(  INT16 * InputData, INT16 *QuantMatrix, INT16 * OutputData );
extern void IDct6_DX(  INT16 * InputData, INT16 *QuantMatrix, INT16 * OutputData );
extern void IDct10_DX(  INT16 * InputData, INT16 *QuantMatrix, INT16 * OutputData );
extern void IDctSlow_DX(  INT16 * InputData, INT16 *QuantMatrix, INT16 * OutputData );


extern void IDct3(  INT16 * InputData, INT16 *QuantMatrix, INT16 * OutputData );
extern void IDct6(  INT16 * InputData, INT16 *QuantMatrix, INT16 * OutputData );
extern void IDct10(  INT16 * InputData, INT16 *QuantMatrix, INT16 * OutputData );
extern void IDctSlow(  INT16 * InputData, INT16 *QuantMatrix, INT16 * OutputData );



extern void IdctAltivec_DX(int16 * InputData, int16 *QuantMatrix, int16 * OutputData);
extern void IdctAltivec10_DX(int16 * InputData, int16 *QuantMatrix, int16 * OutputData);


extern INT32 *SetupBoundingValueArray_ForPPC(PB_INSTANCE *pbi, INT32 FLimit);
extern void FilterHoriz_PPC(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void FilterVert_PPC(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);

extern INT32 *SetupBoundingValueArray_Altivec(PB_INSTANCE *pbi, INT32 FLimit);
extern void FilterHoriz_Altivec(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);
extern void FilterVert_Altivec(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr);

extern void BuildQuantIndex_ForPPC(PB_INSTANCE *pbi);

extern void BuildQuantIndex_ForAltivec(PB_INSTANCE *pbi);

#include "dct.h"
#include "Quantize.h"
#include "Reconstruct.h"


#if defined(POSTPROCESS)

extern void DeringBlockWeak( 
                       PB_INSTANCE *pbi, 
                       UINT8 *SrcPtr,
                       UINT8 *DstPtr,
                       INT32 Pitch,
                       UINT32 FragQIndex,
                       UINT32 *QuantScale);

extern void DeringBlockStrong( 
                         PB_INSTANCE *pbi, 
                         UINT8 *SrcPtr,
                         UINT8 *DstPtr,
                         INT32 Pitch,
                         UINT32 FragQIndex,
                         UINT32 *QuantScale);


extern void DeblockLoopFilteredBand(
                                    PB_INSTANCE *pbi, 
                                    UINT8 *SrcPtr, 
                                    UINT8 *DesPtr,
                                    UINT32 PlaneLineStep, 
                                    UINT32 FragsAcross,
                                    UINT32 StartFrag,
                                    UINT32 *QuantScale);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                                   

extern void DeringBlockWeak_Altivec( 
                       PB_INSTANCE *pbi, 
                       UINT8 *SrcPtr,
                       UINT8 *DstPtr,
                       INT32 Pitch,
                       UINT32 FragQIndex,
                       UINT32 *QuantScale);

extern void DeringBlockStrong_Altivec( 
                         PB_INSTANCE *pbi, 
                         UINT8 *SrcPtr,
                         UINT8 *DstPtr,
                         INT32 Pitch,
                         UINT32 FragQIndex,
                         UINT32 *QuantScale);      
                         
extern void DeblockLoopFilteredBand2_Altivec(
							PB_INSTANCE *pbi,
							UINT8 *SrcPtr,
							UINT8 *DesPtr,
							UINT32 PlaneLineStep,
							UINT32 FragAcross,
							UINT32 StartFrag,
							UINT32 *QuantScale
							);                                                      

#endif

/****************************************************************************
*  Explicit imports
*****************************************************************************
*/


/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
 

/****************************************************************************
*  Module statics.
*****************************************************************************
*/        
const unsigned long PP_MACHINE_LOWLIMIT = 350000000; //Lowest CPU (MHz) to enable PostProcess
//const unsigned long PP_MACHINE_MIDLIMIT = 440; //Lowest CPU (MHz) to enable PostProcess

              
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
 *  ROUTINE       :     SetPbParam
 *
 *  INPUTS        :     PB_COMMAND_TYPE Command
 *                      char *          Parameter
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *	
 *  FUNCTION      :     Generalised command interface to decoder.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CCONV SetPbParam( PB_INSTANCE *pbi, PB_COMMAND_TYPE Command, UINT32 Parameter )
{
#if defined(POSTPROCESS)

	OSErr err;
	long processorAttributes = 0;
	long hasAltiVec = 0;
	long cpuSpeed = 0;
	 
 	err = Gestalt(gestaltPowerPCProcessorFeatures, &processorAttributes);
	if (err == noErr)
    	hasAltiVec = (1 << gestaltPowerPCHasVectorInstructions) & processorAttributes;


	Gestalt(gestaltProcClkSpeed, &cpuSpeed);



	switch ( Command )
	{
	case PBC_SET_POSTPROC:

		if(hasAltiVec)
		{
			if( Parameter == 9 )				
			{
				if (pbi->Configuration.VideoFrameWidth * pbi->Configuration.VideoFrameHeight > 76800)
					pbi->PostProcessingLevel = 0;			//If Framesize > 320 x 240 disable it
				else if (cpuSpeed < PP_MACHINE_LOWLIMIT)			
					pbi->PostProcessingLevel = 4;	
				else			
					pbi->PostProcessingLevel = 5;

			}
			else
				pbi->PostProcessingLevel = Parameter;
		}
		else
			/* for now force post process off */
			pbi->PostProcessingLevel = 0;

		break;
    
	default:
        break;
	}
#endif
}

/****************************************************************************
 * 
 *  ROUTINE       :     fillidctconstants()
 *
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     DoesNothing
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void initIdctConstants_Altivec(void);

void fillidctconstants(void)
{
	OSErr err;
	long processorAttributes = 0;
	long hasAltiVec = 0;
 
 	err = Gestalt(gestaltPowerPCProcessorFeatures, &processorAttributes);
	if (err == noErr)
    	hasAltiVec = (1 << gestaltPowerPCHasVectorInstructions) & processorAttributes;



    // optimized versions.
  	if(hasAltiVec)
    {
		initIdctConstants_Altivec();
	}
}

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
 *  FUNCTION      :     Checks for machine specifc features and
 *                      sets approipriate flags and function pointers.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void DMachineSpecificConfig(PB_INSTANCE *pbi)
{
	UINT32 i;

	OSErr err;
	long processorAttributes = 0;
	long hasAltiVec = 0;
 
 	err = Gestalt(gestaltPowerPCProcessorFeatures, &processorAttributes);
	if (err == noErr)
    	hasAltiVec = (1 << gestaltPowerPCHasVectorInstructions) & processorAttributes;



    // optimized versions.
  	if(hasAltiVec)
    {

        pbi->YUVtoRGB = ScalarYUVtoRGB;
		pbi->ClearSysState = ClearSysState;

		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{
			if(i<=1)
				pbi->idct[i]=IDct1;
			else if(i<=10)
				pbi->idct[i]=IdctAltivec10_DX;
			else 
				pbi->idct[i]=IdctAltivec_DX;
		}


        pbi->ReconIntra = PPCReconIntra_Altivec;
        pbi->ReconInter = PPCReconInter_Altivec;
        pbi->ReconInterHalfPixel2 = PPCReconInterHalfPixel2_Altivec;

        
		pbi->ClearDownQFrag = ClearDownQFragData;	
		pbi->ExtractToken=ExtractToken;
		
		pbi->UnPackVideo=UnPackVideoPPC_LL;

		pbi->CopyBlock = CopyBlock;

        pbi->FilterHoriz = FilterHoriz_Altivec;
        pbi->FilterVert = FilterVert_Altivec;
        pbi->SetupBoundingValueArray = SetupBoundingValueArray_Altivec;


        pbi->BuildQuantIndex = BuildQuantIndex_ForAltivec;
        
#if	defined(POSTPROCESS)
        pbi->DeringBlockWeak = DeringBlockWeak_Altivec;
        pbi->DeringBlockStrong = DeringBlockStrong_Altivec;
		pbi->DeblockLoopFilteredBand = DeblockLoopFilteredBand;

		pbi->DeblockLoopFilteredBand = DeblockLoopFilteredBand2_Altivec;
		
		pbi->ProcessorFrequency = 0;		
#endif

    }
	else  	
    {
        pbi->YUVtoRGB = ScalarYUVtoRGB;
		pbi->ClearSysState = ClearSysState;

		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{
			if(i<=1)
				pbi->idct[i]=IDct1;
			else if(i<=3)
				pbi->idct[i]=IDct3_DX;
			else if(i<=6)
				pbi->idct[i]=IDct6_DX;
			else if(i<=10)
				pbi->idct[i]=IDct10_DX;
			else 
				pbi->idct[i]=IDctSlow_DX;
		}
        // Reconstruction functions
        pbi->ReconIntra = PPCReconIntra;
        pbi->ReconInter = PPCReconInter;
        pbi->ReconInterHalfPixel2 = PPCReconInterHalfPixel2;
		pbi->ClearDownQFrag = ClearDownQFragData;
		
		pbi->ExtractToken=ExtractToken;
		
		pbi->UnPackVideo=UnPackVideoPPC_LL;
		pbi->CopyBlock = CopyBlock;

        pbi->FilterHoriz = FilterHoriz_Generic;
        pbi->FilterVert = FilterVert_Generic;
        pbi->SetupBoundingValueArray = SetupBoundingValueArray_Generic;

//        pbi->BuildQuantIndex = BuildQuantIndex_Generic;

        pbi->BuildQuantIndex = BuildQuantIndex_ForPPC;

#if	defined(POSTPROCESS)
        pbi->DeringBlockWeak = DeringBlockWeak;
        pbi->DeringBlockStrong = DeringBlockStrong;
		pbi->DeblockLoopFilteredBand = DeblockLoopFilteredBand;
		
		pbi->ProcessorFrequency = 0;		
#endif

    }
    
}

// Issues a warning message
void IssueWarning( char * WarningMessage )
{
    // Issue the warning messge
}


char * SytemGlobalAlloc( unsigned int Size )  
{
    return (char *)malloc(Size);  
}

void SystemGlobalFree( char * MemPtr )
{
   free( MemPtr );
}
