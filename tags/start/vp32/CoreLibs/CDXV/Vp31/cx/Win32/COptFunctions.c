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
******************************************************************************|
*/

extern INT32 XmmGetSAD8( UINT8 * NewDataPtr, 
					UINT8  * RefDataPtr, INT32 OffsetN, INT32 OffsetR) ;
extern void	 XmmGetError(UINT8*	NewDataPtr, 
					UINT8*	RefDataPtr1,UINT32	PixelPerLine, 
					INT32 *XSum, INT32 *XXSum);
extern void XmmGetAvError(UINT8*	NewDataPtr, 
					UINT8*	RefDataPtr1,UINT8*	RefDataPtr2,UINT32	PixelPerLine, 
					INT32*	XSum, INT32 *XXSum);
extern UINT8 TokenizeDctValue( INT16 DataValue, UINT32 * TokenListPtr  );
extern UINT8 TokenizeDctRunValue( UINT8 RunLength, INT16 DataValue, UINT32 * TokenListPtr  );
		
					
/****************************************************************************
 * 
 *  ROUTINE       :     GetBlockReconErrorXMM
 *
 *  INPUTS        :     UINT32 BlockIndex
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     A reconstruction error score for the given block
 *
 *  FUNCTION      :     Calculates reconstruction error score for the given block.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

UINT32 GetBlockReconErrorXMM( CP_INSTANCE *cpi, INT32 BlockIndex )
{
	INT32	ErrorVal = 0;

    UINT8 * SrcDataPtr = &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,BlockIndex)];
    UINT8 * RecDataPtr = &cpi->pb.LastFrameRecon[GetFragIndex(cpi->pb.recon_pixel_index_table, BlockIndex)];
    INT32   SrcStride;
    INT32   RecStride;

    // Is the block a Y block or a UV block.
    if ( BlockIndex < (INT32)cpi->pb.YPlaneFragments )
    {
        SrcStride = cpi->pb.Configuration.VideoFrameWidth;
        RecStride = cpi->pb.Configuration.YStride;
    }
    else
    {
        SrcStride = cpi->pb.Configuration.VideoFrameWidth >> 1;
        RecStride = cpi->pb.Configuration.UVStride;
    }
    
	ErrorVal=XmmGetSAD8(SrcDataPtr, RecDataPtr, SrcStride, RecStride);

	return ErrorVal;
}


/****************************************************************************
 * 
 *  ROUTINE       :     MmxGetSAD
 *
 *  INPUTS        :     UINT8 * NewDataPtr	(New Data)
 *						UINT8 * RefDataPtr
 *						UINT32	PixelsPerLine
 *                      INT32   ErrorSoFar, 
 *						INT32   BestSoFar 
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Sum absolute differences
 *
 *  FUNCTION      :     Calculates the sum of the absolute differences.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
INT32 MmxGetSAD( UINT8 * NewDataPtr, UINT8  * RefDataPtr, 
			 	 UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar  )
{
	INT32	DiffVal = ErrorSoFar;
	INT16	DiffAcc[4] = {0,0,0,0};		// MMX accumulator.
	
	// MMX code for SAD.	
__asm
	{
		pxor        mm6, mm6					; Blank mmx6
		pxor        mm7, mm7					; Blank mmx7

		mov         eax,dword ptr [NewDataPtr]	; Load base addresses
		mov         ebx,dword ptr [RefDataPtr]
		mov         ecx,dword ptr [PixelsPerLine]

        // Row 1
		movq		mm0, [eax]					; Copy eight bytes to mm0
		movq		mm1, [ebx]					; Copy eight bytes to mm1
		movq		mm2, mm0					; Take copy of MM0

		psubusb		mm0, mm1					; A-B to MM0
		psubusb		mm1, mm2					; B-A to MM1
		por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		movq		mm1, mm0					; keep a copy
		punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		paddw       mm7, mm0					; accumulate difference...
		punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		add         eax,ecx						; Inc pointer into the new data
		paddw       mm7, mm1					; accumulate difference...
		add         ebx,ecx						; Inc pointer into ref data
		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride

        // Row 2
		movq		mm0, [eax]					; Copy eight bytes to mm0
		movq		mm1, [ebx]					; Copy eight bytes to mm1
		movq		mm2, mm0					; Take copy of MM0

		psubusb		mm0, mm1					; A-B to MM0
		psubusb		mm1, mm2					; B-A to MM1
		por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		movq		mm1, mm0					; keep a copy
		punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		paddw       mm7, mm0					; accumulate difference...
		punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		add         eax,ecx						; Inc pointer into the new data
		paddw       mm7, mm1					; accumulate difference...
		add         ebx,ecx						; Inc pointer into ref data
		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride

        // Row 3
		movq		mm0, [eax]					; Copy eight bytes to mm0
		movq		mm1, [ebx]					; Copy eight bytes to mm1
		movq		mm2, mm0					; Take copy of MM0

		psubusb		mm0, mm1					; A-B to MM0
		psubusb		mm1, mm2					; B-A to MM1
		por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		movq		mm1, mm0					; keep a copy
		punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		paddw       mm7, mm0					; accumulate difference...
		punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		add         eax,ecx						; Inc pointer into the new data
		paddw       mm7, mm1					; accumulate difference...
		add         ebx,ecx						; Inc pointer into ref data
		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride

        // Row 4
		movq		mm0, [eax]					; Copy eight bytes to mm0
		movq		mm1, [ebx]					; Copy eight bytes to mm1
		movq		mm2, mm0					; Take copy of MM0

		psubusb		mm0, mm1					; A-B to MM0
		psubusb		mm1, mm2					; B-A to MM1
		por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		movq		mm1, mm0					; keep a copy
		punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		paddw       mm7, mm0					; accumulate difference...
		punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		add         eax,ecx						; Inc pointer into the new data
		paddw       mm7, mm1					; accumulate difference...
		add         ebx,ecx						; Inc pointer into ref data
		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride

        // Row 5
		movq		mm0, [eax]					; Copy eight bytes to mm0
		movq		mm1, [ebx]					; Copy eight bytes to mm1
		movq		mm2, mm0					; Take copy of MM0

		psubusb		mm0, mm1					; A-B to MM0
		psubusb		mm1, mm2					; B-A to MM1
		por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		movq		mm1, mm0					; keep a copy
		punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		paddw       mm7, mm0					; accumulate difference...
		punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		add         eax,ecx						; Inc pointer into the new data
		paddw       mm7, mm1					; accumulate difference...
		add         ebx,ecx						; Inc pointer into ref data
		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride

        // Row 6
		movq		mm0, [eax]					; Copy eight bytes to mm0
		movq		mm1, [ebx]					; Copy eight bytes to mm1
		movq		mm2, mm0					; Take copy of MM0

		psubusb		mm0, mm1					; A-B to MM0
		psubusb		mm1, mm2					; B-A to MM1
		por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		movq		mm1, mm0					; keep a copy
		punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		paddw       mm7, mm0					; accumulate difference...
		punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		add         eax,ecx						; Inc pointer into the new data
		paddw       mm7, mm1					; accumulate difference...
		add         ebx,ecx						; Inc pointer into ref data
		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride

        // Row 7
		movq		mm0, [eax]					; Copy eight bytes to mm0
		movq		mm1, [ebx]					; Copy eight bytes to mm1
		movq		mm2, mm0					; Take copy of MM0

		psubusb		mm0, mm1					; A-B to MM0
		psubusb		mm1, mm2					; B-A to MM1
		por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		movq		mm1, mm0					; keep a copy
		punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		paddw       mm7, mm0					; accumulate difference...
		punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		add         eax,ecx						; Inc pointer into the new data
		paddw       mm7, mm1					; accumulate difference...
		add         ebx,ecx						; Inc pointer into ref data
		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride

        // Row 8
		movq		mm0, [eax]					; Copy eight bytes to mm0
		movq		mm1, [ebx]					; Copy eight bytes to mm1
		movq		mm2, mm0					; Take copy of MM0

		psubusb		mm0, mm1					; A-B to MM0
		psubusb		mm1, mm2					; B-A to MM1
		por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		movq		mm1, mm0					; keep a copy
		punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		paddw       mm7, mm0					; accumulate difference...
		punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		paddw       mm7, mm1					; accumulate difference...

		movq		DWORD PTR [DiffAcc], mm7	; copy back accumulated results into normal memory
//		emms									; Clear the MMX state.
	}
	
	//	Accumulate the 4 resulting word values.
	DiffVal += DiffAcc[0] + DiffAcc[1] + DiffAcc[2] + DiffAcc[3];

	return DiffVal;
    
}

/****************************************************************************
 * 
 *  ROUTINE       :     GetHalfPixelSumAbsDiffs
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
INT32 MmxGetHalfPixelSAD( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
						  UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar  )
{
       
	INT32	DiffVal = ErrorSoFar;
    INT32   RefOffset = (int)(RefDataPtr1 - RefDataPtr2);
	INT16	DiffAcc[4] = {0,0,0,0};		// MMX accumulator.
	
    if ( RefOffset == 0 )
    {
		// Simple case as for non 0.5 pixel
		DiffVal += MmxGetSAD( SrcData, RefDataPtr1, PixelsPerLine, ErrorSoFar, BestSoFar );
    }
    else 
    {

__asm
    	// MMX code for SAD.	
	    {
		    pxor        mm6, mm6					; Blank mmx6
		    pxor        mm7, mm7					; Blank mmx7

		    mov         eax,dword ptr [SrcData]	    ; Load base addresses and line increment
		    mov         ebx,dword ptr [RefDataPtr1]
		    mov         ecx,dword ptr [RefDataPtr2]
		    mov         edx,dword ptr [PixelsPerLine]

            // Row 1
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
         	paddw       mm1, mm2					; Add word values together.
		    punpckhbw   mm4, mm6					
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
         	paddw       mm3, mm4					; Add word values together.
            movq		mm0, [eax]					; Copy eight of src data to mm0
            psrlw       mm3, 1                      ; Devide by two (shift right 1)
    		movq		mm2, mm0					; Take copy of MM0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data for SAD
		    psubusb		mm0, mm1					; A-B to MM0
		    psubusb		mm1, mm2					; B-A to MM1
		    por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		    movq		mm1, mm0					; keep a copy
		    punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		    paddw       mm7, mm0					; accumulate difference...
		    punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		    add         eax,edx						; Inc pointer into the src data
		    paddw       mm7, mm1					; accumulate difference...
		    add         ebx,edx						; Inc pointer into ref1 
		    add         ecx,edx						; Inc pointer into ref2 
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref1 data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 2
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					


		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
         	paddw       mm1, mm2					; Add word values together.
		    punpckhbw   mm4, mm6					
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
         	paddw       mm3, mm4					; Add word values together.
            movq		mm0, [eax]					; Copy eight of src data to mm0
            psrlw       mm3, 1                      ; Devide by two (shift right 1)
    		movq		mm2, mm0					; Take copy of MM0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data for SAD
		    psubusb		mm0, mm1					; A-B to MM0
		    psubusb		mm1, mm2					; B-A to MM1
		    por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		    movq		mm1, mm0					; keep a copy
		    punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		    paddw       mm7, mm0					; accumulate difference...
		    punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		    add         eax,edx						; Inc pointer into the src data
		    paddw       mm7, mm1					; accumulate difference...
		    add         ebx,edx						; Inc pointer into ref1 
		    add         ecx,edx						; Inc pointer into ref2 
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref1 data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 3
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
         	paddw       mm1, mm2					; Add word values together.
		    punpckhbw   mm4, mm6					
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
         	paddw       mm3, mm4					; Add word values together.
            movq		mm0, [eax]					; Copy eight of src data to mm0
            psrlw       mm3, 1                      ; Devide by two (shift right 1)
    		movq		mm2, mm0					; Take copy of MM0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data for SAD
		    psubusb		mm0, mm1					; A-B to MM0
		    psubusb		mm1, mm2					; B-A to MM1
		    por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		    movq		mm1, mm0					; keep a copy
		    punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		    paddw       mm7, mm0					; accumulate difference...
		    punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		    add         eax,edx						; Inc pointer into the src data
		    paddw       mm7, mm1					; accumulate difference...
		    add         ebx,edx						; Inc pointer into ref1 
		    add         ecx,edx						; Inc pointer into ref2 
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref1 data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 4
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
         	paddw       mm1, mm2					; Add word values together.
		    punpckhbw   mm4, mm6					
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
         	paddw       mm3, mm4					; Add word values together.
            movq		mm0, [eax]					; Copy eight of src data to mm0
            psrlw       mm3, 1                      ; Devide by two (shift right 1)
    		movq		mm2, mm0					; Take copy of MM0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data for SAD
		    psubusb		mm0, mm1					; A-B to MM0
		    psubusb		mm1, mm2					; B-A to MM1
		    por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		    movq		mm1, mm0					; keep a copy
		    punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		    paddw       mm7, mm0					; accumulate difference...
		    punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		    add         eax,edx						; Inc pointer into the src data
		    paddw       mm7, mm1					; accumulate difference...
		    add         ebx,edx						; Inc pointer into ref1 
		    add         ecx,edx						; Inc pointer into ref2 
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref1 data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 5
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
         	paddw       mm1, mm2					; Add word values together.
		    punpckhbw   mm4, mm6					
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
         	paddw       mm3, mm4					; Add word values together.
            movq		mm0, [eax]					; Copy eight of src data to mm0
            psrlw       mm3, 1                      ; Devide by two (shift right 1)
    		movq		mm2, mm0					; Take copy of MM0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data for SAD
		    psubusb		mm0, mm1					; A-B to MM0
		    psubusb		mm1, mm2					; B-A to MM1
		    por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		    movq		mm1, mm0					; keep a copy
		    punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		    paddw       mm7, mm0					; accumulate difference...
		    punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		    add         eax,edx						; Inc pointer into the src data
		    paddw       mm7, mm1					; accumulate difference...
		    add         ebx,edx						; Inc pointer into ref1 
		    add         ecx,edx						; Inc pointer into ref2 
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref1 data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 6
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
         	paddw       mm1, mm2					; Add word values together.
		    punpckhbw   mm4, mm6					
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
         	paddw       mm3, mm4					; Add word values together.
            movq		mm0, [eax]					; Copy eight of src data to mm0
            psrlw       mm3, 1                      ; Devide by two (shift right 1)
    		movq		mm2, mm0					; Take copy of MM0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data for SAD
		    psubusb		mm0, mm1					; A-B to MM0
		    psubusb		mm1, mm2					; B-A to MM1
		    por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		    movq		mm1, mm0					; keep a copy
		    punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		    paddw       mm7, mm0					; accumulate difference...
		    punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		    add         eax,edx						; Inc pointer into the src data
		    paddw       mm7, mm1					; accumulate difference...
		    add         ebx,edx						; Inc pointer into ref1 
		    add         ecx,edx						; Inc pointer into ref2 
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref1 data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 7
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
         	paddw       mm1, mm2					; Add word values together.
		    punpckhbw   mm4, mm6					
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
         	paddw       mm3, mm4					; Add word values together.
            movq		mm0, [eax]					; Copy eight of src data to mm0
            psrlw       mm3, 1                      ; Devide by two (shift right 1)
    		movq		mm2, mm0					; Take copy of MM0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data for SAD
		    psubusb		mm0, mm1					; A-B to MM0
		    psubusb		mm1, mm2					; B-A to MM1
		    por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		    movq		mm1, mm0					; keep a copy
		    punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		    paddw       mm7, mm0					; accumulate difference...
		    punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		    add         eax,edx						; Inc pointer into the src data
		    paddw       mm7, mm1					; accumulate difference...
		    add         ebx,edx						; Inc pointer into ref1 
		    add         ecx,edx						; Inc pointer into ref2 
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref1 data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 8
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
         	paddw       mm1, mm2					; Add word values together.
		    punpckhbw   mm4, mm6					
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
         	paddw       mm3, mm4					; Add word values together.
            movq		mm0, [eax]					; Copy eight of src data to mm0
            psrlw       mm3, 1                      ; Devide by two (shift right 1)
    		movq		mm2, mm0					; Take copy of MM0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data for SAD
		    psubusb		mm0, mm1					; A-B to MM0
		    psubusb		mm1, mm2					; B-A to MM1
		    por			mm0, mm1					; OR MM0 and MM1 gives abs differences in MM0

		    movq		mm1, mm0					; keep a copy
		    punpcklbw   mm0, mm6					; unpack to higher precision for accumulation
		    paddw       mm7, mm0					; accumulate difference...
		    punpckhbw   mm1, mm6					; unpack high four bytes to higher precision
		    paddw       mm7, mm1					; accumulate difference...

  	    	movq		DWORD PTR [DiffAcc], mm7	; copy back accumulated results into normal memory
//        	emms									; Clear the MMX state.

        }

    	//	Accumulate the 4 word values in DiffAcc
	    DiffVal += DiffAcc[0] + DiffAcc[1] + DiffAcc[2] + DiffAcc[3];

    }

	return DiffVal;
    
}

/****************************************************************************
 * 
 *  ROUTINE       :     MmxGetInterErr
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
UINT32 MmxGetInterErr( UINT8 * NewDataPtr, UINT8 * RefDataPtr1,  UINT8 * RefDataPtr2, UINT32 PixelsPerLine )
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
		    pxor        mm5, mm5					; Blank mmx6
		    pxor        mm6, mm6					; Blank mmx7
		    pxor        mm7, mm7					; Blank mmx7

		    mov         eax,dword ptr [NewDataPtr]	; Load base addresses
		    mov         ebx,dword ptr [RefDataPtr1]
		    mov         ecx,dword ptr [PixelsPerLine]
				lea			edx, DWORD ptr [ecx + STRIDE_EXTRA]

            // Row 1
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm1, [ebx]					; Copy eight bytes to mm1
//				movq		mm4, [ebx + edx]
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         ebx,ecx						; Inc pointer into ref data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
		    add         eax,ecx						; Inc pointer into the new data
		    movq		mm1, [ebx]					; Copy eight bytes to mm1
//				movq		mm4, [ebx + edx]
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7


            // Row 2
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
    	    punpcklbw   mm1, mm6					
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         ebx,ecx						; Inc pointer into ref data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
		    add         eax,ecx						; Inc pointer into the new data
		    movq		mm1, [ebx]					; Copy eight bytes to mm1
//				movq		mm4, [ebx + edx]
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 3
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
		    punpcklbw   mm1, mm6					
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         ebx,ecx						; Inc pointer into ref data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
		    add         eax,ecx						; Inc pointer into the new data
		    movq		mm1, [ebx]					; Copy eight bytes to mm1
//				movq		mm4, [ebx + edx]
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 4
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
		    punpcklbw   mm1, mm6					
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         ebx,ecx						; Inc pointer into ref data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
		    add         eax,ecx						; Inc pointer into the new data
		    movq		mm1, [ebx]					; Copy eight bytes to mm1
//				movq		mm4, [ebx + edx]
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 5
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
		    punpcklbw   mm1, mm6					
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         ebx,ecx						; Inc pointer into ref data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
		    add         eax,ecx						; Inc pointer into the new data
		    movq		mm1, [ebx]					; Copy eight bytes to mm1
//				movq		mm4, [ebx + edx]
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 6
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
		    punpcklbw   mm1, mm6					
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         ebx,ecx						; Inc pointer into ref data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
		    add         eax,ecx						; Inc pointer into the new data
		    movq		mm1, [ebx]					; Copy eight bytes to mm1
//				movq		mm4, [ebx + edx]
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 7
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
		    punpcklbw   mm1, mm6					
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         ebx,ecx						; Inc pointer into ref data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
		    add         eax,ecx						; Inc pointer into the new data
		    movq		mm1, [ebx]					; Copy eight bytes to mm1
//				movq		mm4, [ebx + edx]
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 8
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
		    punpcklbw   mm1, mm6					
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         ebx,ecx						; Inc pointer into ref data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
		    add         eax,ecx						; Inc pointer into the new data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory

//        	emms									; Clear the MMX state.
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
		    pxor        mm5, mm5					; Blank mmx6
		    pxor        mm6, mm6					; Blank mmx7
		    pxor        mm7, mm7					; Blank mmx7

		    mov         eax,dword ptr [NewDataPtr]	; Load base addresses
		    mov         ebx,dword ptr [RefDataPtr1]
		    mov         ecx,dword ptr [RefDataPtr2]
		    mov         edx,dword ptr [PixelsPerLine]

            // Row 1
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6					
         	paddw       mm1, mm2					; Add word values together.
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
		    punpckhbw   mm4, mm6					
         	paddw       mm3, mm4					; Add word values together.
            psrlw       mm3, 1                      ; Devide by two (shift right 1)

		    movq		mm0, [eax]					; Copy eight bytes to mm0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    add         eax,edx						; Inc pointer into the new data
		    add         ebx,edx						; Inc pointer into ref data
		    add         ecx,edx						; Inc pointer into ref2 data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 2
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6					
         	paddw       mm1, mm2					; Add word values together.
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
		    punpckhbw   mm4, mm6					
         	paddw       mm3, mm4					; Add word values together.
            psrlw       mm3, 1                      ; Devide by two (shift right 1)

		    movq		mm0, [eax]					; Copy eight bytes to mm0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    add         eax,edx						; Inc pointer into the new data
		    add         ebx,edx						; Inc pointer into ref data
		    add         ecx,edx						; Inc pointer into ref2 data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 3
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6					
         	paddw       mm1, mm2					; Add word values together.
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
		    punpckhbw   mm4, mm6					
         	paddw       mm3, mm4					; Add word values together.
            psrlw       mm3, 1                      ; Devide by two (shift right 1)

		    movq		mm0, [eax]					; Copy eight bytes to mm0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    add         eax,edx						; Inc pointer into the new data
		    add         ebx,edx						; Inc pointer into ref data
		    add         ecx,edx						; Inc pointer into ref2 data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 4
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6					
         	paddw       mm1, mm2					; Add word values together.
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
		    punpckhbw   mm4, mm6					
         	paddw       mm3, mm4					; Add word values together.
            psrlw       mm3, 1                      ; Devide by two (shift right 1)

		    movq		mm0, [eax]					; Copy eight bytes to mm0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    add         eax,edx						; Inc pointer into the new data
		    add         ebx,edx						; Inc pointer into ref data
		    add         ecx,edx						; Inc pointer into ref2 data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 5
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6					
         	paddw       mm1, mm2					; Add word values together.
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
		    punpckhbw   mm4, mm6					
         	paddw       mm3, mm4					; Add word values together.
            psrlw       mm3, 1                      ; Devide by two (shift right 1)

		    movq		mm0, [eax]					; Copy eight bytes to mm0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    add         eax,edx						; Inc pointer into the new data
		    add         ebx,edx						; Inc pointer into ref data
		    add         ecx,edx						; Inc pointer into ref2 data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 6
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6					
         	paddw       mm1, mm2					; Add word values together.
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
		    punpckhbw   mm4, mm6					
         	paddw       mm3, mm4					; Add word values together.
            psrlw       mm3, 1                      ; Devide by two (shift right 1)

		    movq		mm0, [eax]					; Copy eight bytes to mm0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory
		    add         eax,edx						; Inc pointer into the new data
		    add         ebx,edx						; Inc pointer into ref data
		    add         ecx,edx						; Inc pointer into ref2 data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 7
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6					
         	paddw       mm1, mm2					; Add word values together.
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
		    punpckhbw   mm4, mm6					
         	paddw       mm3, mm4					; Add word values together.
            psrlw       mm3, 1                      ; Devide by two (shift right 1)

		    movq		mm0, [eax]					; Copy eight bytes to mm0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory

		    add         eax,edx						; Inc pointer into the new data
		    add         ebx,edx						; Inc pointer into ref data
		    add         ecx,edx						; Inc pointer into ref2 data
    		add         ebx,STRIDE_EXTRA    		; Inc pointer into ref data by extra amount for UMV stride
    		add         ecx,STRIDE_EXTRA    		; Inc pointer into ref2 data by extra amount for UMV stride

            // Row 8
    		movq		mm1, [ebx]					; Copy eight bytes from each of ref 1 and ref 2.
	    	movq		mm2, [ecx]					
    		movq		mm3, mm1					; Take copies.
    		movq		mm4, mm2					

		    punpcklbw   mm1, mm6					; unpack low four bytes to higher precision
		    punpcklbw   mm2, mm6					
         	paddw       mm1, mm2					; Add word values together.
            psrlw       mm1, 1                      ; Devide by two (shift right 1)
		    punpckhbw   mm3, mm6					; unpack high four bytes to higher precision
		    punpckhbw   mm4, mm6					
         	paddw       mm3, mm4					; Add word values together.
            psrlw       mm3, 1                      ; Devide by two (shift right 1)

		    movq		mm0, [eax]					; Copy eight bytes to mm0
            packuswb    mm1, mm3                    ; Repack to give 1/2 pixel averaged reference data
		    movq		mm2, mm0					; Take copies
		    movq		mm3, mm1					; Take copies

		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpcklbw   mm1, mm6					
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    punpckhbw   mm3, mm6					
            psubsw		mm0, mm1					; A-B (low order) to MM0
            psubsw		mm2, mm3					; A-B (high order) to MM2

		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5

		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

  	    	movq		DWORD PTR [MmxXSum], mm5	; copy back accumulated results into normal memory
  	    	movq		DWORD PTR [MmxXXSum], mm7	; copy back accumulated results into normal memory

//        	emms									; Clear the MMX state.
        }

        // Now accumulate the final results.
        XSum = MmxXSum[0] + MmxXSum[1] + MmxXSum[2] + MmxXSum[3];
        XXSum = MmxXXSum[0] + MmxXXSum[1];
    }

	// Compute and return population variance as mis-match metric.
	return ( ((XXSum << 6) - XSum*XSum ) );
}






/****************************************************************************
 * 
 *  ROUTINE       :     MmxGetIntraError
 *
 *  INPUTS        :     UINT8 * DataPtr	(New Data)
 *                      UINT32  Horizontal and vertical scaling factors
 *						UINT32	PixelsPerLine
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Intra frame variance
 *
 *  FUNCTION      :     Calculates a variance score for the block
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 MmxGetIntraError( UINT8 * DataPtr, UINT32 PixelsPerLine )
{
	UINT32	XSum=0;
	UINT32	XXSum=0;
	UINT8	*DiffPtr;
    
	// Loop expanded out for speed. 
	DiffPtr = DataPtr;
 
	__asm
    {
		    pxor        mm5, mm5					; Blank mmx6
		    pxor        mm6, mm6					; Blank mmx7
		    pxor        mm7, mm7					; Blank mmx7

		    mov         eax,dword ptr [DiffPtr]	; Load base addresses
		    mov         ecx,dword ptr [PixelsPerLine]

            // Row 1
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5
			pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         eax,ecx						; Inc pointer into the new data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7


            // Row 2
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5
		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         eax,ecx						; Inc pointer into ref data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 3
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5
		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         eax,ecx						; Inc pointer into ref data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7


            // Row 4
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5
		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         eax,ecx						; Inc pointer into ref data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 5
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5
		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         eax,ecx						; Inc pointer into ref data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 6
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5
		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         eax,ecx						; Inc pointer into ref data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 7
		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5
		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         eax,ecx						; Inc pointer into ref data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7

            // Row 8
   		    movq		mm0, [eax]					; Copy eight bytes to mm0
		    movq		mm2, mm0					; Take copies
		    punpcklbw   mm0, mm6					; unpack to higher precision
		    punpckhbw   mm2, mm6					; unpack to higher precision
		    paddw       mm5, mm0					; accumulate differences in mm5
		    paddw       mm5, mm2					; accumulate differences in mm5
		    pmaddwd     mm0, mm0					; square and accumulate
		    pmaddwd     mm2, mm2					; square and accumulate
		    add         eax,ecx						; Inc pointer into ref data
		    paddd       mm7, mm0					; accumulate in mm7
		    paddd       mm7, mm2					; accumulate in mm7
		
			movq		mm4, mm5					; 
			punpcklwd	mm5, mm6		
			punpckhwd	mm4, mm6
			movq		mm0, mm7
			paddw		mm5, mm4

			punpckhdq	mm0, mm6
			punpckldq	mm7, mm6
			movq		mm4, mm5
			paddd		mm0, mm7	
			punpckhdq	mm4, mm6
			punpckldq	mm5, mm6
			movd		DWORD PTR [XXSum], mm0
			paddw	    mm4, mm5
			movd		DWORD ptr [XSum], mm4
//			emms									; Clear the MMX state.

	}
	// Compute population variance as mis-match metric.
    return ( ((XXSum<<6) - XSum*XSum ));
}


/****************************************************************************
 * 
 *  ROUTINE       :     TokenizeDctBlockMmx
 *
 *  INPUTS        :     UINT8 * RawData
 *                      INT16 * TokenListPtr
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     Number of tokens used including additional bits tokens.
 *
 *  FUNCTION      :     Encodes a DCT block into a stream of tokens.
 *                      most tokens are followed by an additional bits
 *                      token that can contain up to 8 bits of aditional data.
 *
 *  SPECIAL NOTES :     Only encodes run value for the value 0. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT8 TokenizeDctBlockMmx( INT16 * RawData, UINT32 * TokenListPtr )
{
    UINT32 i;  
    UINT8  run_count;    
    UINT8  token_count = 0;     // Number of tokens crated. 
    UINT32 AbsData;
	UINT32 BlockSize = 64;
	
	
	UINT32 Flag = 1	;

	_asm
	{
		mov		eax, DWORD ptr [RawData]			;
		movq	mm0, QWORD ptr [eax+64]				;
		movq	mm1, QWORD ptr [eax+80]				;
		movq	mm2, QWORD ptr [eax+96]				;
		movq	mm3, QWORD ptr [eax+112]			;
		por		mm0, QWORD ptr [eax+72]				;
		por		mm1, QWORD ptr [eax+88]				;
		por		mm2, QWORD ptr [eax+104]			;
		por		mm3, QWORD ptr [eax+120]			;
		por		mm2, QWORD ptr [eax+56]				;
		por		mm1, QWORD ptr [eax+48]				;
		por		mm0,	mm1					;
		por		mm2,	mm3					;
		por		mm0,	mm2					;
		movd	eax,	mm0					;
		psrlq	mm0,	32					;
		movd	ebx,	mm0					;
		or		eax,	ebx					;
		mov		DWORD ptr [Flag], eax		;
	}


	if(Flag==0)BlockSize = 25;

    // Tokenize the block 
    for( i = 0; i < BlockSize; i++ )
    {   
        run_count = 0;  

        // Look for a zero run. 
		// NOTE the use of & instead of && which is faster (and equivalent) in this instance. 
        while( (i < BlockSize) & (!RawData[i]) )
        {
            run_count++; 
            i++;
        }

        // If we have reached the end of the block then code EOB 
        if ( i == BlockSize )
        {
            TokenListPtr[token_count] = DCT_EOB_TOKEN;    
            token_count++;
        }
        else
        {
            // If we have a short zero run followed by a low data value code the two as a composite token.
            if ( run_count )
            {
                AbsData = abs(RawData[i]);
    
                if ( ((AbsData == 1) && (run_count <= 17)) || 
                     ((AbsData <= 3) && (run_count <= 3)) )
                {
                    /* Tokenise the run and subsequent value combination value */
                    token_count += TokenizeDctRunValue( run_count, RawData[i], &TokenListPtr[token_count] );
                }

                // Else if we have a long non-EOB run or a run followed by a value token > MAX_RUN_VAL
                // then code the run and token seperately
                else 
                {
                    if ( run_count <= 8 )
                        TokenListPtr[token_count] = DCT_SHORT_ZRL_TOKEN;
                    else
                        TokenListPtr[token_count] = DCT_ZRL_TOKEN;

                    token_count++;
                    TokenListPtr[token_count] = run_count - 1;    
                    token_count++;

                    // Now tokenize the value
                    token_count += TokenizeDctValue( RawData[i], &TokenListPtr[token_count] );
                }
            }
            // Else there was NO zero run. 
            else
            {
                // Tokenise the value 
                token_count += TokenizeDctValue( RawData[i], &TokenListPtr[token_count] );
            }
        }
    }

    /* Return the total number of tokens (including additional bits tokens) used. */
    return token_count;
}

