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


#include <assert.h>
#include "ColorConversions.h"
#include "lutbl.h"
#include "ccstr.h"
#include <stdio.h>
#include "cpuidlib.h"
#include "asmcolorconversions.h"
#include "cclib.h"
#include "stdlib.h"

#ifdef _DEBUG
#include <windows.h>
#else
#define OutputDebugString
#endif

/*
 * constants needed for color conversion
 */


/*
 * function prototypes
 */
void DefaultFunction( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
                      unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer, int SrcPitch,int DstPitch );


/*
 * Global Function pointers
 * Once InitCCLib is called they should point to the fastest functions that are able
 * to run on the current machine
 */
void (*RGB32toYV12)( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight, 
                            unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer, int SrcPitch,int DstPitch ) = DefaultFunction;

void (*RGB24toYV12)( unsigned char *RGBBuffer, int ImageWidth, int ImageHeight,
                            unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer, int SrcPitch,int DstPitch )= DefaultFunction;

void (*UYVYtoYV12)( unsigned char *UYVYBuffer, int ImageWidth, int ImageHeight,
                    unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer, int SrcPitch, int DstPitch ) = DefaultFunction;

void (*YUY2toYV12)( unsigned char *YUY2Buffer, int ImageWidth, int ImageHeight,
                    unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer, int SrcPitch, int DstPitch ) = DefaultFunction;

void (*YVYUtoYV12)( unsigned char *YVYUBuffer, int ImageWidth, int ImageHeight,
                           unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer, int SrcPitch,int DstPitch ) = DefaultFunction;

#ifdef _DEBUG
char DebugString[132];  // for outputtting debug information
#else
#endif _DEBUG

/*
 * **-DefaultFunction
 *
 * Our function pointers will by default be initilized to point to this function.  The purpose
 * of this function is to prevent us from going off into the weeds if the init function is not
 * called.
 *
 * Assumptions:
 *  None
 *
 * Input:
 *  Does not matter
 *
 * Output:
 *  If init function not called will force an error
 */
void DefaultFunction( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
                      unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer, int SrcPitch,int DstPitch )
{
   char *CharPtr = 0;
   
#ifdef _DEBUG
   assert( 0 ); // InitCCLib MUST be called before using this function
#else
   // force error
   *CharPtr = 1;
#endif
}

/*
 * **-InitCCLib
 *
 * See cclib.h for a more detailed description of this function
 */
int InitCCLib( PROCTYPE CpuType )
{
   PROCTYPE DetectedCpuType;
   int ReturnValue = 0;  // assume will work

   if( CpuType == SpecialProc )
   {
      DetectedCpuType = findCPUId();
   }
   else
   {
      DetectedCpuType = CpuType;
   }

   switch( DetectedCpuType )
   {
      /*
       * The following processors supports the MMX instructions
       * point function pointers to MMX version of the functions
       */
      case PMMX:
      case PII:
      case AMDK63D:
      case AMDK6:
         RGB32toYV12 = CC_RGB32toYV12_MMX;
         RGB24toYV12 = CC_RGB24toYV12_MMX;
         UYVYtoYV12  = CC_UYVYtoYV12_MMX;
         YUY2toYV12  = CC_YUY2toYV12_MMX;
         YVYUtoYV12  = CC_YVYUtoYV12_MMX;
         break;

         
      /*
       * The following processors supports the XMM instructions
       * point function pointers to XMM version of the functions
       */
      case XMM:
      case WMT:
         RGB32toYV12 = CC_RGB32toYV12_XMM;
         RGB24toYV12 = CC_RGB24toYV12_XMM;
         UYVYtoYV12  = CC_UYVYtoYV12_MMX;
         YUY2toYV12  = CC_YUY2toYV12_MMX;
         YVYUtoYV12  = CC_YVYUtoYV12_MMX;         
         break;

      /*
       * No know optimizations are avaliable for the following processors.
       * use the generic C version of the functions
       */
      case X86:
      case PPRO:
      case C6X86:
      case C6X86MX:
      case AMDK5:
      case MACG3:
      case MAC68K:
      default:
         RGB32toYV12 = CC_RGB32toYV12_C;
         RGB24toYV12 = CC_RGB24toYV12_C;
         UYVYtoYV12  = CC_UYVYtoYV12_C;
         YUY2toYV12  = CC_YUY2toYV12_C;
         YVYUtoYV12  = CC_YVYUtoYV12_C;
         break;
   }

   return( ReturnValue );
}

/*
 * **-DeInitCCLib
 *
 * See cclib.h for a more detailed description of this function
 */
void DeInitCCLib( void )
{
}
