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
*   Module Title :     OptFunctions.c
*
*   Description  :     Encoder system dependant functions
*
*
*****************************************************************************
*/

/*******************************************3*********************************
*  Header Files
*****************************************************************************
*/
#pragma warning(disable:4799)

#include <math.h>
#include "compdll.h"
#include <assert.h>

/****************************************************************************
*  Imports
******************************************************************************
*/
extern XMMGetSAD( 
                 UINT8 * NewDataPtr, 
                 UINT8  * RefDataPtr, 
                 UINT32 PixelsPerLine, 
                 UINT32 ErrorSoFar, 
                 UINT32 BestSoFar);

/*****************************************************************************/		
					

/****************************************************************************
 * 
 *  ROUTINE       :     WmtGetHalfPixelSumAbsDiffs
 *
 *  INPUTS        :     UINT8 * SrcData	(New Data)
 *						UINT8 * RefDataPtr1
 *						UINT8 * RefDataPtr2
 *						UINT32	PixelsPerLine
 *                      INT32   ErrorSoFar 
 *						INT32   BestSoFar 
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Sum of absolute differences at 1/2 pixel accuracy.
 *
 *  FUNCTION      :     Calculates the sum of the absolute differences against
 *                      half pixel interpolated references.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
INT32 WmtGetHalfPixelSAD( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
						  UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar  )
{
       
	INT32	DiffVal = ErrorSoFar;
    INT32   RefOffset = (int)(RefDataPtr1 - RefDataPtr2);
	INT16	DiffAcc[4] = {0,0,0,0};		// MMX accumulator.
	
    if ( RefOffset == 0 )
    {
		// Simple case as for non 0.5 pixel
		DiffVal += XMMGetSAD( SrcData, RefDataPtr1, PixelsPerLine, ErrorSoFar, BestSoFar );
    }
    else 
    {
        // WMT Code for HalfPixelSAD
        __asm
	    {

		    mov         eax,        dword ptr [SrcData]	        // Get Src Pointer
		    pxor        xmm6,       xmm6					    // clear mm6

            mov         ebx,        dword ptr [RefDataPtr1]     // Get Reference pointers
		    pxor        xmm7,       xmm7					
		    
            mov         edx,        dword ptr [PixelsPerLine]   // Width
            mov         ecx,        dword ptr [RefDataPtr2] 

            mov         esi,        edx                         // width
            add         edx,        STRIDE_EXTRA                // Src Pitch
            
            // Row 1 and 2
    		movq		xmm1,       QWORD ptr [ebx]				// Eight bytes from ref 1 
	    	movq		xmm2,       QWORD ptr [ecx]				// Eight Bytes from ref 2

		    punpcklbw   xmm1,       xmm6					    // unpack ref1 to shorts
            movq        xmm3,       QWORD ptr [ebx+edx]         // Eight bytes from ref 1

		    punpcklbw   xmm2,       xmm6                        // unpack ref2 to shorts
            movq        xmm4,       QWORD ptr [ecx+edx]         // Eight bytes from ref 2       

            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            paddw       xmm1,       xmm2					    // Add short values together.

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
            psrlw       xmm1,       1                           // Devided by two (shift right 1)
            
            paddw       xmm3,       xmm4                        // add short values togethter
            movq		xmm0,       QWORD PTR [eax]				// Copy eight of src data to xmm0

            psrlw       xmm3,       1                           // divided by 2
            punpcklbw   xmm0,       xmm6                        // unpack to shorts

            movq        xmm5,       QWORD PTR [eax+esi]         // get the source
            movdqa		xmm2,       xmm0					    // make a copy of xmm0  

            punpcklbw   xmm5,       xmm6                        // unpack to shorts
		    psubusw		xmm0,       xmm1					    // A-B to xmm0

            movdqa      xmm4,       xmm5                        // make a copy 
		    psubusw		xmm1,       xmm2					    // B-A to xmm1

            psubusw     xmm5,       xmm3                        // A-B to xmm5
            psubusw     xmm3,       xmm4                        // B-A to mm1

            por			xmm0,       xmm1					    // abs differences
            por         xmm5,       xmm3                        // abs differences

		    paddw       xmm7,       xmm0					    // accumulate difference...
            paddw       xmm7,       xmm5                        // accumulate difference...

            lea         ebx,        [ebx+edx*2]                 // two line below
            lea         ecx,        [ecx+edx*2]                 // two line below

            lea         eax,        [eax+esi*2]                 // two line below


            // Row 3 and 4
     		movq		xmm1,       QWORD PTR [ebx]				// Eight bytes from ref 1 
	    	movq		xmm2,       QWORD PTR [ecx]				// Eight Bytes from ref 2

		    punpcklbw   xmm1,       xmm6					    // unpack ref1 to shorts
            movq        xmm3,       QWORD PTR [ebx+edx]         // Eight bytes from ref 1

		    punpcklbw   xmm2,       xmm6                        // unpack ref2 to shorts
            movq        xmm4,       QWORD PTR [ecx+edx]         // Eight bytes from ref 2       

            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            paddw       xmm1,       xmm2					    // Add short values together.

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
            psrlw       xmm1,       1                           // Devided by two (shift right 1)
            
            paddw       xmm3,       xmm4                        // add short values togethter
            movq		xmm0,       QWORD PTR [eax]				// Copy eight of src data to xmm0

            psrlw       xmm3,       1                           // divided by 2
            punpcklbw   xmm0,       xmm6                        // unpack to shorts

            movq        xmm5,       QWORD PTR [eax+esi]         // get the source
            movdqa		xmm2,       xmm0					    // make a copy of xmm0  

            punpcklbw   xmm5,       xmm6                        // unpack to shorts
		    psubusw		xmm0,       xmm1					    // A-B to xmm0

            movdqa      xmm4,       xmm5                        // make a copy 
		    psubusw		xmm1,       xmm2					    // B-A to xmm1

            psubusw     xmm5,       xmm3                        // A-B to xmm5
            psubusw     xmm3,       xmm4                        // B-A to mm1

            por			xmm0,       xmm1					    // abs differences
            por         xmm5,       xmm3                        // abs differences

		    paddw       xmm7,       xmm0					    // accumulate difference...
            paddw       xmm7,       xmm5                        // accumulate difference...

            lea         ebx,        [ebx+edx*2]                 // two line below
            lea         ecx,        [ecx+edx*2]                 // two line below

            lea         eax,        [eax+esi*2]                 // two line below

            // Row 5 and 6
     		movq		xmm1,       QWORD PTR [ebx]				// Eight bytes from ref 1 
	    	movq		xmm2,       QWORD PTR [ecx]				// Eight Bytes from ref 2

		    punpcklbw   xmm1,       xmm6					    // unpack ref1 to shorts
            movq        xmm3,       QWORD PTR [ebx+edx]         // Eight bytes from ref 1

		    punpcklbw   xmm2,       xmm6                        // unpack ref2 to shorts
            movq        xmm4,       QWORD PTR [ecx+edx]         // Eight bytes from ref 2       

            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            paddw       xmm1,       xmm2					    // Add short values together.

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
            psrlw       xmm1,       1                           // Devided by two (shift right 1)
            
            paddw       xmm3,       xmm4                        // add short values togethter
            movq		xmm0,       QWORD PTR [eax]				// Copy eight of src data to xmm0

            psrlw       xmm3,       1                           // divided by 2
            punpcklbw   xmm0,       xmm6                        // unpack to shorts

            movq        xmm5,       QWORD PTR [eax+esi]         // get the source
            movdqa		xmm2,       xmm0					    // make a copy of xmm0  

            punpcklbw   xmm5,       xmm6                        // unpack to shorts
		    psubusw		xmm0,       xmm1					    // A-B to xmm0

            movdqa      xmm4,       xmm5                        // make a copy 
		    psubusw		xmm1,       xmm2					    // B-A to xmm1

            psubusw     xmm5,       xmm3                        // A-B to xmm5
            psubusw     xmm3,       xmm4                        // B-A to mm1

            por			xmm0,       xmm1					    // abs differences
            por         xmm5,       xmm3                        // abs differences

		    paddw       xmm7,       xmm0					    // accumulate difference...
            paddw       xmm7,       xmm5                        // accumulate difference...

            lea         ebx,        [ebx+edx*2]                 // two line below
            lea         ecx,        [ecx+edx*2]                 // two line below

            lea         eax,        [eax+esi*2]                 // two line below

            // Row 7 and 8
     		movq		xmm1,       QWORD PTR [ebx]				// Eight bytes from ref 1 
	    	movq		xmm2,       QWORD PTR [ecx]				// Eight Bytes from ref 2

		    punpcklbw   xmm1,       xmm6					    // unpack ref1 to shorts
            movq        xmm3,       QWORD PTR [ebx+edx]         // Eight bytes from ref 1

		    punpcklbw   xmm2,       xmm6                        // unpack ref2 to shorts
            movq        xmm4,       QWORD PTR [ecx+edx]         // Eight bytes from ref 2       

            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            paddw       xmm1,       xmm2					    // Add short values together.

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
            psrlw       xmm1,       1                           // Devided by two (shift right 1)
            
            paddw       xmm3,       xmm4                        // add short values togethter
            movq		xmm0,       QWORD PTR [eax]					    // Copy eight of src data to xmm0

            psrlw       xmm3,       1                           // divided by 2
            punpcklbw   xmm0,       xmm6                        // unpack to shorts

            movq        xmm5,       QWORD PTR [eax+esi]         // get the source
            movdqa		xmm2,       xmm0					    // make a copy of xmm0  

            punpcklbw   xmm5,       xmm6                        // unpack to shorts
		    psubusw		xmm0,       xmm1					    // A-B to xmm0

            movdqa      xmm4,       xmm5                        // make a copy 
		    psubusw		xmm1,       xmm2					    // B-A to xmm1

            psubusw     xmm5,       xmm3                        // A-B to xmm5
            psubusw     xmm3,       xmm4                        // B-A to mm1

            por			xmm0,       xmm1					    // abs differences
            por         xmm5,       xmm3                        // abs differences

		    paddw       xmm7,       xmm0					    // accumulate difference...
            paddw       xmm7,       xmm5                        // accumulate difference...

            // add the value to gether
  	    	movdqa      xmm0,       xmm7                        // low four words
            psrldq      xmm7,       8                           // shift 64 bits                           

            paddw       xmm0,       xmm7                        // add 
            movq		QWORD PTR [DiffAcc], xmm0	; copy back accumulated results into normal memory

        }

    	//	Accumulate the 4 word values in DiffAcc
	    DiffVal += DiffAcc[0] + DiffAcc[1] + DiffAcc[2] + DiffAcc[3];

    }

	return DiffVal;
    
}

/****************************************************************************
 * 
 *  ROUTINE       :     WmtGetInterErr
 *
 *  INPUTS        :     UINT8 * NewDataPtr	(New Data)
 *						UINT8 * RefDataPtr1
 *                      UINT8 * RefDataPtr2
 *						UINT32	PixelsPerLine
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Variance / error
 *
 *  FUNCTION      :     Calculates a difference error score for two blocks
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 WmtGetInterErr( UINT8 * NewDataPtr, UINT8 * RefDataPtr1,  UINT8 * RefDataPtr2, UINT32 PixelsPerLine )
{
	UINT32	XSum=0;
	UINT32	XXSum=0;
    INT16   MmxXSum[4] = {0,0,0,0};         // XSum accumulators
    INT32   MmxXXSum[2] = {0,0};            // XXSum accumulators

    INT32   AbsRefOffset = abs((int)(RefDataPtr1 - RefDataPtr2));
    
    // Mode of interpolation chosen based upon on the offset of the second reference pointer 
    if ( AbsRefOffset == 0 )
    {

		__asm
        {
		    mov         eax,        NewDataPtr          	    // Load base addresses
		    pxor        xmm5,       xmm5					    // Clear Xmm5

		    mov         ebx,        RefDataPtr1                 // Ref1
            pxor        xmm6,       xmm6                        // Clear Xmm6


		    mov         ecx,        PixelsPerLine               // Get Width
            pxor        xmm7,       xmm7					    // Clear Xmm7

		    lea			edx,        [ecx + STRIDE_EXTRA]        // Get Pitch

            // Row 1 and Row 2
		    movq		xmm0,       QWORD PTR [eax]				// Copy eight bytes to xmm0
		    movq		xmm1,       QWORD PTR [ebx]				// Copy eight bytes to xmm1

		    punpcklbw   xmm0,       xmm6					    // unpack to higher precision
            movq        xmm3,       QWORD Ptr [eax+ecx]         // Copy eight Bytes to xmm3

		    punpcklbw   xmm1,       xmm6					    // unpack to shorts
            movq        xmm4,       QWORD ptr [ebx+edx]         // Copy eight Bytes to xmm4

            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            psubsw		xmm0,       xmm1					    // A-B to xmm0

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
		    paddw       xmm5,       xmm0					    // accumulate differences in xmm5

            psubsw      xmm3,       xmm4                        // A-B to xmm3
            paddw       xmm5,       xmm3                        // accumulate the differences

            pmaddwd     xmm0,       xmm0					    // square and accumulate
            pmaddwd     xmm3,       xmm3                        // square and accumulate

            lea         ebx,        [ebx+edx*2]                 // mov forward two lines
            lea         eax,        [eax+ecx*2]                 // mov forward two lines

            paddd       xmm7,       xmm0                        // accumulate in xmm7
            paddd       xmm7,       xmm3                        // accumulate in xmm7

            // Row 3 and Row 4
		    movq		xmm0,       QWORD PTR [eax]				// Copy eight bytes to xmm0
		    movq		xmm1,       QWORD PTR [ebx]				// Copy eight bytes to xmm1

		    punpcklbw   xmm0,       xmm6					    // unpack to higher precision
            movq        xmm3,       QWORD Ptr [eax+ecx]         // Copy eight Bytes to xmm3

		    punpcklbw   xmm1,       xmm6					    // unpack to shorts
            movq        xmm4,       QWORD ptr [ebx+edx]         // Copy eight Bytes to xmm4

            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            psubsw		xmm0,       xmm1					    // A-B to xmm0

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
		    paddw       xmm5,       xmm0					    // accumulate differences in xmm5

            psubsw      xmm3,       xmm4                        // A-B to xmm3
            paddw       xmm5,       xmm3                        // accumulate the differences

            pmaddwd     xmm0,       xmm0					    // square and accumulate
            pmaddwd     xmm3,       xmm3                        // square and accumulate

            lea         ebx,        [ebx+edx*2]                 // mov forward two lines
            lea         eax,        [eax+ecx*2]                 // mov forward two lines

            paddd       xmm7,       xmm0                        // accumulate in xmm7
            paddd       xmm7,       xmm3                        // accumulate in xmm7

            // Row 5 and Row6
		    movq		xmm0,       QWORD PTR [eax]				// Copy eight bytes to xmm0
		    movq		xmm1,       QWORD PTR [ebx]				// Copy eight bytes to xmm1

		    punpcklbw   xmm0,       xmm6					    // unpack to higher precision
            movq        xmm3,       QWORD Ptr [eax+ecx]         // Copy eight Bytes to xmm3

		    punpcklbw   xmm1,       xmm6					    // unpack to shorts
            movq        xmm4,       QWORD ptr [ebx+edx]         // Copy eight Bytes to xmm4

            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            psubsw		xmm0,       xmm1					    // A-B to xmm0

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
		    paddw       xmm5,       xmm0					    // accumulate differences in xmm5

            psubsw      xmm3,       xmm4                        // A-B to xmm3
            paddw       xmm5,       xmm3                        // accumulate the differences

            pmaddwd     xmm0,       xmm0					    // square and accumulate
            pmaddwd     xmm3,       xmm3                        // square and accumulate

            lea         ebx,        [ebx+edx*2]                 // mov forward two lines
            lea         eax,        [eax+ecx*2]                 // mov forward two lines

            paddd       xmm7,       xmm0                        // accumulate in xmm7
            paddd       xmm7,       xmm3                        // accumulate in xmm7

            // Row 7 and Row 8
		    movq		xmm0,       QWORD PTR [eax]				// Copy eight bytes to xmm0
		    movq		xmm1,       QWORD PTR [ebx]				// Copy eight bytes to xmm1

		    punpcklbw   xmm0,       xmm6					    // unpack to higher precision
            movq        xmm3,       QWORD Ptr [eax+ecx]         // Copy eight Bytes to xmm3

		    punpcklbw   xmm1,       xmm6					    // unpack to shorts
            movq        xmm4,       QWORD ptr [ebx+edx]         // Copy eight Bytes to xmm4

            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            psubsw		xmm0,       xmm1					    // A-B to xmm0

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
		    paddw       xmm5,       xmm0					    // accumulate differences in xmm5

            psubsw      xmm3,       xmm4                        // A-B to xmm3
            paddw       xmm5,       xmm3                        // accumulate the differences

            pmaddwd     xmm0,       xmm0					    // square and accumulate
            pmaddwd     xmm3,       xmm3                        // square and accumulate

            paddd       xmm7,       xmm0                        // accumulate in xmm7
            paddd       xmm7,       xmm3                        // accumulate in xmm7

            
            movdqa      xmm0,       xmm5
            movdqa      xmm1,       xmm7

            psrldq      xmm5,       8
            psrldq      xmm7,       8

            paddw       xmm0,       xmm5
            paddd       xmm1,       xmm7


  	    	movq		QWORD PTR [MmxXSum], xmm0	; copy back accumulated results into normal memory
  	    	movq		QWORD PTR [MmxXXSum], xmm1	; copy back accumulated results into normal memory

        }

        // Now accumulate the final results.
        XSum = MmxXSum[0] + MmxXSum[1] + MmxXSum[2] + MmxXSum[3];
        XXSum = MmxXXSum[0] + MmxXXSum[1];
    }
    
    // Simple half pixel reference data
    else 
	{
        __asm
        {

    		mov         eax,        NewDataPtr          	    // Load base addresses
		    pxor        xmm5,       xmm5					    // Clear Xmm5

		    mov         ebx,        RefDataPtr1                 // Ref1
            pxor        xmm6,       xmm6                        // Clear Xmm6


		    mov         ecx,        PixelsPerLine               // Get Width
            pxor        xmm7,       xmm7					    // Clear Xmm7

		    mov         esi,        RefDataPtr2                 // Ref 2
            lea			edx,        [ecx + STRIDE_EXTRA]        // Get Pitch


            // Row 1 and Row 2
    		movq		xmm1,       QWORD PTR [ebx]				// Copy eight bytes from each of ref 1 
	    	movq		xmm2,       QWORD PTR [esi]				// Copy eight bytes from each of ref 2 	

		    punpcklbw   xmm1,       xmm6					    // unpack to shorts
            movq        xmm3,       QWORD PTR [ebx+edx]         // Copy eight bytes from each of ref 1 
		    
            punpcklbw   xmm2,       xmm6					    // unpack to shorts
            movq        xmm4,       QWORD PTR [esi+edx]         // Copy eight bytes from each of ref 2 
         	
            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            paddw       xmm1,       xmm2					    // Add word values together.

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
            psrlw       xmm1,       1                           // Devide by two (shift right 1)

            paddw       xmm3,       xmm4                        // add word values together
		    movq		xmm0,       QWORD PTR [eax]				// copy eight source bytes to xmm2

            psrlw       xmm3,       1                           // divided by two
            movq        xmm2,       QWORD PTR [eax+ecx]         // copy eight source bytes to xmm2
            
            punpcklbw   xmm0,       xmm6                        // unpack to words
            punpcklbw   xmm2,       xmm6                        // unpack to words

            psubsw      xmm0,       xmm1                        // the difference
            psubsw      xmm2,       xmm3                        // the difference 
            
            paddw       xmm5,       xmm0                        // accumulate the difference
            paddw       xmm5,       xmm2                        // accumulate the difference

            pmaddwd     xmm0,       xmm0                        // square and accumulate
            pmaddwd     xmm2,       xmm2                        // square and accumulate

            lea         eax,        [eax+ecx*2]
            lea         ebx,        [ebx+edx*2]
            
            lea         esi,        [esi+edx*2]
            paddd       xmm7,       xmm0					    // accumulate in mm7

		    paddd       xmm7,       xmm2					    // accumulate in mm7


            // Row 3 and Row 4
    		movq		xmm1,       QWORD PTR [ebx]				// Copy eight bytes from each of ref 1 
	    	movq		xmm2,       QWORD PTR [esi]				// Copy eight bytes from each of ref 2 	

		    punpcklbw   xmm1,       xmm6					    // unpack to shorts
            movq        xmm3,       QWORD PTR [ebx+edx]         // Copy eight bytes from each of ref 1 
		    
            punpcklbw   xmm2,       xmm6					    // unpack to shorts
            movq        xmm4,       QWORD PTR [esi+edx]         // Copy eight bytes from each of ref 2 
         	
            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            paddw       xmm1,       xmm2					    // Add word values together.

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
            psrlw       xmm1,       1                           // Devide by two (shift right 1)

            paddw       xmm3,       xmm4                        // add word values together
		    movq		xmm0,       QWORD PTR [eax]				// copy eight source bytes to xmm2

            psrlw       xmm3,       1                           // divided by two
            movq        xmm2,       QWORD PTR [eax+ecx]         // copy eight source bytes to xmm2
            
            punpcklbw   xmm0,       xmm6                        // unpack to words
            punpcklbw   xmm2,       xmm6                        // unpack to words

            psubsw      xmm0,       xmm1                        // the difference
            psubsw      xmm2,       xmm3                        // the difference 
            
            paddw       xmm5,       xmm0                        // accumulate the difference
            paddw       xmm5,       xmm2                        // accumulate the difference

            pmaddwd     xmm0,       xmm0                        // square and accumulate
            pmaddwd     xmm2,       xmm2                        // square and accumulate

            lea         eax,        [eax+ecx*2]
            lea         ebx,        [ebx+edx*2]
            
            lea         esi,        [esi+edx*2]
            paddd       xmm7,       xmm0					    // accumulate in mm7

		    paddd       xmm7,       xmm2					    // accumulate in mm7


            // Row 5 and Row 6
    		movq		xmm1,       QWORD PTR [ebx]				// Copy eight bytes from each of ref 1 
	    	movq		xmm2,       QWORD PTR [esi]				// Copy eight bytes from each of ref 2 	

		    punpcklbw   xmm1,       xmm6					    // unpack to shorts
            movq        xmm3,       QWORD PTR [ebx+edx]         // Copy eight bytes from each of ref 1 
		    
            punpcklbw   xmm2,       xmm6					    // unpack to shorts
            movq        xmm4,       QWORD PTR [esi+edx]         // Copy eight bytes from each of ref 2 
         	
            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            paddw       xmm1,       xmm2					    // Add word values together.

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
            psrlw       xmm1,       1                           // Devide by two (shift right 1)

            paddw       xmm3,       xmm4                        // add word values together
		    movq		xmm0,       QWORD PTR [eax]				// copy eight source bytes to xmm2

            psrlw       xmm3,       1                           // divided by two
            movq        xmm2,       QWORD PTR [eax+ecx]         // copy eight source bytes to xmm2
            
            punpcklbw   xmm0,       xmm6                        // unpack to words
            punpcklbw   xmm2,       xmm6                        // unpack to words

            psubsw      xmm0,       xmm1                        // the difference
            psubsw      xmm2,       xmm3                        // the difference 
            
            paddw       xmm5,       xmm0                        // accumulate the difference
            paddw       xmm5,       xmm2                        // accumulate the difference

            pmaddwd     xmm0,       xmm0                        // square and accumulate
            pmaddwd     xmm2,       xmm2                        // square and accumulate

            lea         eax,        [eax+ecx*2]
            lea         ebx,        [ebx+edx*2]
            
            lea         esi,        [esi+edx*2]
            paddd       xmm7,       xmm0					    // accumulate in mm7

		    paddd       xmm7,       xmm2					    // accumulate in mm7


            // Row 7 and Row 8
    		movq		xmm1,       QWORD PTR [ebx]				// Copy eight bytes from each of ref 1 
	    	movq		xmm2,       QWORD PTR [esi]				// Copy eight bytes from each of ref 2 	

		    punpcklbw   xmm1,       xmm6					    // unpack to shorts
            movq        xmm3,       QWORD PTR [ebx+edx]         // Copy eight bytes from each of ref 1 
		    
            punpcklbw   xmm2,       xmm6					    // unpack to shorts
            movq        xmm4,       QWORD PTR [esi+edx]         // Copy eight bytes from each of ref 2 
         	
            punpcklbw   xmm3,       xmm6                        // unpack to shorts
            paddw       xmm1,       xmm2					    // Add word values together.

            punpcklbw   xmm4,       xmm6                        // unpack to shorts
            psrlw       xmm1,       1                           // Devide by two (shift right 1)

            paddw       xmm3,       xmm4                        // add word values together
		    movq		xmm0,       QWORD PTR [eax]				// copy eight source bytes to xmm2

            psrlw       xmm3,       1                           // divided by two
            movq        xmm2,       QWORD PTR [eax+ecx]         // copy eight source bytes to xmm2
            
            punpcklbw   xmm0,       xmm6                        // unpack to words
            punpcklbw   xmm2,       xmm6                        // unpack to words

            psubsw      xmm0,       xmm1                        // the difference
            psubsw      xmm2,       xmm3                        // the difference 
            
            paddw       xmm5,       xmm0                        // accumulate the difference
            paddw       xmm5,       xmm2                        // accumulate the difference

            pmaddwd     xmm0,       xmm0                        // square and accumulate
            pmaddwd     xmm2,       xmm2                        // square and accumulate

            paddd       xmm7,       xmm0					    // accumulate in mm7
		    paddd       xmm7,       xmm2					    // accumulate in mm7

            movdqa      xmm0,       xmm5
            movdqa      xmm1,       xmm7

            psrldq      xmm5,       8
            psrldq      xmm7,       8

            paddw       xmm0,       xmm5
            paddd       xmm1,       xmm7


  	    	movq		QWORD Ptr [MmxXSum],    xmm0	        // copy back accumulated results into normal memory
  	    	movq		QWORD Ptr [MmxXXSum],   xmm1	        // copy back accumulated results into normal memory

        }

        // Now accumulate the final results.
        XSum = MmxXSum[0] + MmxXSum[1] + MmxXSum[2] + MmxXSum[3];
        XXSum = MmxXXSum[0] + MmxXXSum[1];
    }

	// Compute and return population variance as mis-match metric.
	return ( ((XXSum << 6) - XSum*XSum ) );
}


