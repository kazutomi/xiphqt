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

#ifdef PBDLL
#include "pbdll.h"
#include "yuvtofromrgb.h"
#include "cpuidlib.h"

// Functions that should only be used in assembly versions of the code
extern void MMX_idct(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
extern void MMX_idct10(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
extern void MMX_idct1(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );


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

extern void DeringBlockWeak_MMX( 
                       const PB_INSTANCE *pbi, 
                       const UINT8 *SrcPtr,
                       UINT8 *DstPtr,
                       const INT32 Pitch,
                       UINT32 FragQIndex,
                       UINT32 *QuantScale);

extern void DeringBlockStrong_MMX( 
                         const PB_INSTANCE *pbi, 
                         const UINT8 *SrcPtr,
                         UINT8 *DstPtr,
                         const INT32 Pitch,
                         UINT32 FragQIndex,
                         UINT32 *QuantScale);




extern void DeblockLoopFilteredBand_MMX(
                                    PB_INSTANCE *pbi, 
                                    UINT8 *SrcPtr, 
                                    UINT8 *DesPtr,
                                    UINT32 PlaneLineStep, 
                                    UINT32 FragsAcross,
                                    UINT32 StartFrag,
                                    UINT32 *QuantScale);

extern void DeblockNonFilteredBand_MMX(
                                   PB_INSTANCE *pbi, 
                                   UINT8 *SrcPtr, 
                                   UINT8 *DesPtr,
                                   UINT32 PlaneLineStep, 
                                   UINT32 FragsAcross,
                                   UINT32 StartFrag,
                                   UINT32 *QuantScale);

#endif


// these are only used by the dxer
extern void MMX_idct_DX(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
extern void MMX_idct10_DX(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
extern void BuildQuantIndex_ForMMX(PB_INSTANCE *pbi);
extern void BuildQuantIndex_ForWMT(PB_INSTANCE *pbi);
extern void Wmt_IDct_Dx( Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
extern void Wmt_IDct10_Dx(  Q_LIST_ENTRY * InputData, int16 *QuantMatrix, int16 * OutputData );
extern void BuildQuantIndex_ForWMT(PB_INSTANCE *pbi);

extern UINT32 MMXExtractToken(BITREADER *br,HUFF_ENTRY *h);
//
extern void MmxYUVtoRGB ( PB_INSTANCE * pbi, YUV_BUFFER_ENTRY_PTR yblock,	YUV_BUFFER_ENTRY_PTR ublock,	
	                      YUV_BUFFER_ENTRY_PTR vblock,	int uvoffset, 
                          BGR_TYPE * RGBPtr, BOOL ReconBuffer );			
extern void MMXClearDownQFragData(PB_INSTANCE *pbi);
extern void WmtClearDownQFragData(PB_INSTANCE *pbi);

extern void UnPackVideo(PB_INSTANCE *pbi);
extern void UnPackVideoAsm(PB_INSTANCE *pbi);
extern void UnPackVideoMmx(PB_INSTANCE *pbi);
extern void UnPackVideoMMX_LL (PB_INSTANCE *pbi);
extern void ClearMmx(void);

extern void CopyBlockMMX(unsigned char *src, unsigned char *dest, unsigned int srcstride);

#else
#include "compdll.h"
#include "mcomp.h"
#include "YUVtofromRGB.h"
#endif
#include "dct.h"

#include "Quantize.h"
#include "Reconstruct.h"

#include "resource.h"    /* Resource IDs. */  

/****************************************************************************
*  Explicit imports
*****************************************************************************
*/

extern unsigned int CPUFrequency;
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
 *  ROUTINE       :     readTSC
 *
 *  INPUTS        :     None
 *                   
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     read the cpu time stamp counter
 *
 *  SPECIAL NOTES :     Since this function uses RDTSC instruction, which is 
 *						introduced in Pentium processor, so this routine is 
 *						expected to work on Pentium and above.
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void readTSC(unsigned long *tsc)
{
	int time;
	
	__asm 
	{
        pushad
        cpuid
		rdtsc
		mov time,eax
        popad
	}

	*tsc=time;
	return;
}


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

	LARGE_INTEGER pf;						//Performance Counter Frequencey
	LARGE_INTEGER startcount, endcount;		
	unsigned long tsc1, tsc2;

	//If the cpu does not support the high resolution counter, return 0
    unsigned long time1, time2;
	unsigned long cpufreq=0;				
    unsigned long Mhz=0;
    unsigned long Nearest66Mhz, Nearest50Mhz;
    unsigned long Delta66, Delta50;

	if( QueryPerformanceFrequency(&pf))
	{
		
		// read the counter and TSC at start
		QueryPerformanceCounter(&startcount);
		readTSC(&tsc1);
		// delay for 10 ms to get enough accuracy
        time1 = timeGetTime();
        time2 = time1;

        while( time2 < time1+5 )
            time2 = timeGetTime();

		//read the counter and TSC at end
		QueryPerformanceCounter(&endcount);
		readTSC(&tsc2);
		
		//calculate the frequency
		cpufreq = (unsigned long )((double)( tsc2 - tsc1 ) 
			* (double)pf.LowPart 
			/ (double) ( endcount.LowPart - startcount.LowPart ) 
			/ 1000000);

	}
   
    Nearest66Mhz = ((cpufreq * 3 + 100)/200 * 200) / 3;
    Delta66 = abs(Nearest66Mhz - cpufreq);
    Nearest50Mhz = ((cpufreq + 25)/50 *50);
    Delta50 = abs(Nearest50Mhz - cpufreq);

    if(Delta50 < Delta66)
        cpufreq = Nearest50Mhz;
    else
    {
    
        cpufreq = Nearest66Mhz;
        if(cpufreq == 666)
            cpufreq = 667;
    }
    return cpufreq;

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
#define MMX_ENABLED 1
void DMachineSpecificConfig(PB_INSTANCE *pbi)
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
		pbi->MmxEnabled = FALSE;
		pbi->XmmEnabled = FALSE;
		pbi->WmtEnabled = FALSE;
		break;
	case PII	:   
	case AMDK63D:
	case AMDK6  :
	case PMMX	:   
		pbi->MmxEnabled = TRUE;
		pbi->XmmEnabled = FALSE;
		pbi->WmtEnabled = FALSE;
		break;
	case XMM    :
		pbi->MmxEnabled = TRUE;
		pbi->XmmEnabled = TRUE;
		pbi->WmtEnabled = FALSE;
		break;
	case WMT	:
		pbi->MmxEnabled = TRUE;
		pbi->XmmEnabled = TRUE;
		pbi->WmtEnabled = TRUE;
		break;
	}
	
	// For testing purposes, forcing the CPU type
	// pbi->MmxEnabled = FALSE;
	// pbi->XmmEnabled = FALSE;
	// pbi->WmtEnabled = FALSE;

	// If MMX supported then set to use MMX versions of functions else 
    // use original 'C' versions.
	
	if(pbi->WmtEnabled)		//Willamette
	{
        pbi->YUVtoRGB = MmxYUVtoRGB;
		//pbi->idct = MMX_idct;
		//pbi->idct = MMX_idct10;
        pbi->ReconIntra = WmtReconIntra;
        pbi->ReconInter = WmtReconInter;
        pbi->ReconInterHalfPixel2 = WmtReconInterHalfPixel2;
		pbi->ClearDownQFrag = WmtClearDownQFragData;
		pbi->ClearSysState = ClearMmx;

		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{

			if(i<=1)pbi->idct[i]=MMX_idct1;
			else if(i<=10)pbi->idct[i]=Wmt_IDct10_Dx;
			else pbi->idct[i]=Wmt_IDct_Dx;
		}
		pbi->ExtractToken=MMXExtractToken;
		pbi->UnPackVideo=UnPackVideoMMX_LL;
		
        pbi->BuildQuantIndex = BuildQuantIndex_ForWMT;

		pbi->CopyBlock = CopyBlockMMX;

        pbi->FilterHoriz = FilterHoriz_MMX;
        pbi->FilterVert = FilterVert_MMX;
        pbi->SetupBoundingValueArray = SetupBoundingValueArray_ForMMX;

#if	defined(POSTPROCESS)
        pbi->DeringBlockWeak = DeringBlockWeak_MMX;
        pbi->DeringBlockStrong = DeringBlockStrong_MMX;
		pbi->DeblockLoopFilteredBand = DeblockLoopFilteredBand_MMX;
        pbi->DeringBlockWeak = DeringBlockWeak_MMX;
        pbi->DeringBlockStrong = DeringBlockStrong_MMX;
		pbi->DeblockLoopFilteredBand = DeblockLoopFilteredBand_MMX;
		
		// check the CPU to see if postprocess is able to be done without
		// slow down the playback
		pbi->ProcessorFrequency = CPUFrequency;

#endif
	}
	else if ( pbi->MmxEnabled )
    {
        pbi->YUVtoRGB = MmxYUVtoRGB;
		//pbi->idct = MMX_idct;
		//pbi->idct = MMX_idct10;
        pbi->ReconIntra = MMXReconIntra;
        pbi->ReconInter = MmxReconInter;
        pbi->ReconInterHalfPixel2 = MmxReconInterHalfPixel2;
		pbi->ClearDownQFrag = MMXClearDownQFragData;
		pbi->ClearSysState = ClearMmx;


		//setup the function pointers for inverse dct
		for(i=0;i<=64;i++)
		{
			if(i<=1)pbi->idct[i]=MMX_idct1;
			else if(i<=10)pbi->idct[i]=MMX_idct10_DX;
			else pbi->idct[i]=MMX_idct_DX;
		}
		pbi->ExtractToken=MMXExtractToken;
		pbi->UnPackVideo=UnPackVideoMMX_LL;

        pbi->BuildQuantIndex = BuildQuantIndex_ForMMX;

		pbi->CopyBlock = CopyBlockMMX;

        pbi->FilterHoriz = FilterHoriz_MMX;
        pbi->FilterVert = FilterVert_MMX;
        pbi->SetupBoundingValueArray = SetupBoundingValueArray_ForMMX;

#if	defined(POSTPROCESS)
        pbi->DeringBlockWeak = DeringBlockWeak_MMX;
        pbi->DeringBlockStrong = DeringBlockStrong_MMX;
		pbi->DeblockLoopFilteredBand = DeblockLoopFilteredBand_MMX;
        pbi->DeringBlockWeak = DeringBlockWeak_MMX;
        pbi->DeringBlockStrong = DeringBlockStrong_MMX;
		pbi->DeblockLoopFilteredBand = DeblockLoopFilteredBand_MMX;

		// check the CPU to see if postprocess is able to be done without
		// slow down the playback
		pbi->ProcessorFrequency = CPUFrequency;

#endif
//        pbi->FilterHoriz = FilterHoriz_Generic;
//        pbi->FilterVert = FilterVert_Generic;
//        pbi->SetupBoundingValueArray = SetupBoundingValueArray_Generic;
    }
    else
    {
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

}

// Issues a warning message
void IssueWarning( char * WarningMessage )
{
    // Issue the warning messge
    MessageBox(NULL, WarningMessage, NULL, MB_ICONEXCLAMATION | MB_TASKMODAL );
}


char * SytemGlobalAlloc( unsigned int Size )  
{
    return GlobalAlloc( GPTR, Size );  
}

void SystemGlobalFree( char * MemPtr )
{
    GlobalFree( (HGLOBAL) MemPtr );
}

