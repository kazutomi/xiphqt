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

#ifdef PBDLL
#include "pbdll.h"
#include "yuvtofromrgb.h"
#else
#include "compdll.h"
#include "mcomp.h"
#include "YUVtofromRGB.h"
#endif

#include "Quantize.h"
#include "Reconstruct.h"
#include "dct.h"

#include "resource.h"    /* Resource IDs. */  

/****************************************************************************
*  Explicit imports
*****************************************************************************
*/

#if defined(POSTPROCESS)

extern void DeringBlockWeak( 
                       const PB_INSTANCE *pbi, 
                       const UINT8 *SrcPtr,
                       UINT8 *DstPtr,
                       const INT32 Pitch,
                       UINT32 FragQIndex,
                       UINT32 *QuantScale);

extern void DeringBlockStrong( 
                         const PB_INSTANCE *pbi, 
                         const UINT8 *SrcPtr,
                         UINT8 *DstPtr,
                         const INT32 Pitch,
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

extern void DeblockNonFilteredBand(
                                   PB_INSTANCE *pbi, 
                                   UINT8 *SrcPtr, 
                                   UINT8 *DesPtr,
                                   UINT32 PlaneLineStep, 
                                   UINT32 FragsAcross,
                                   UINT32 StartFrag,
                                   UINT32 *QuantScale);

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
extern void UnPackVideo(PB_INSTANCE *pbi);

/****************************************************************************
 * 
 *  ROUTINE       :     GetProcessorFrequency()
 *
 *  INPUTS        :     None
 *                   
 *
 *  OUTPUTS       :     The Frequency in MHZ
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Check the Processor's working freqency 
 *
 *  SPECIAL NOTES :     This function should only be used here. Limited tests 
 *						has verified it works till 166MHz Pentium with MMX. 
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
unsigned long GetProcessorFrequency()
{

    return 0;

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
void fillidctconstants(void)
{
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
 *  FUNCTION      :     Checks for machine specifc features such as MMX support 
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
    UINT32 FeatureFlags = 0;
    BOOL   CPUID_Supported = TRUE;   // Is the CPUID instruction supported
    BOOL   TestMmx = TRUE;
	UINT32 i;
    
        pbi->YUVtoRGB = ScalarYUVtoRGB;
		pbi->ClearSysState = ClearSysState;

		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{
			if(i<=1)pbi->idct[i]=IDct1;
			else if(i<=10)pbi->idct[i]=IDct10;
			else pbi->idct[i]=IDctSlow;
		}

        // Reconstruction functions
        pbi->ReconIntra = ScalarReconIntra;
        pbi->ReconInter = ScalarReconInter;
        pbi->ReconInterHalfPixel2 = ScalarReconInterHalfPixel2;
		pbi->ClearDownQFrag = ClearDownQFragData;
		
		pbi->ExtractToken=ExtractToken;
		pbi->UnPackVideo=UnPackVideo;
		pbi->CopyBlock = CopyBlock;

        pbi->FilterHoriz = FilterHoriz_Generic;
        pbi->FilterVert = FilterVert_Generic;
        pbi->SetupBoundingValueArray = SetupBoundingValueArray_Generic;

        pbi->BuildQuantIndex = BuildQuantIndex_Generic;

#if	defined(POSTPROCESS)
        pbi->DeringBlockWeak = DeringBlockWeak;
        pbi->DeringBlockStrong = DeringBlockStrong;
		pbi->DeblockLoopFilteredBand = DeblockLoopFilteredBand;
		
		pbi->ProcessorFrequency = 0;		/* force it to 0 Mhz when no MMX support */

#endif 
        
}

// Issues a warning message
void IssueWarning( char * WarningMessage )
{
    // Issue the warning messge
    //MessageBox(NULL, WarningMessage, NULL, MB_ICONEXCLAMATION | MB_TASKMODAL );
}


char * SytemGlobalAlloc( unsigned int Size )  
{
    return malloc(Size);  
}

void SystemGlobalFree( char * MemPtr )
{
    free( MemPtr );
}
