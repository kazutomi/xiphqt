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
 *   Module Title :     WmtOptFunctions.c
 *
 *   Description  :     willamette processor specific 
 *                      optimised versions of functions
 *
 *
 *****************************************************************************
 */
 
/* 
    Use Tim's optimized version.
*/

/****************************************************************************
 *  Header Files
 *****************************************************************************
 */

#define STRICT              // Strict type checking. 

#include "codec_common.h"

#include "pbdll.h"

/****************************************************************************
 *  Module constants.
 *****************************************************************************
 */        

/**************************************************************************** 
 *  Imports.
 *****************************************************************************
 */   


/****************************************************************************
 *  Exported Global Variables
 *****************************************************************************
 */

/****************************************************************************
 *  Exported Functions 
 *****************************************************************************
 */              

/****************************************************************************
 *  Module Statics
 *****************************************************************************
 */  

_declspec(align(16)) static  INT16 Ones[]       =  {1,1,1,1, };
_declspec(align(16)) static  INT16 OneTwoEight[]=  {128,128,128,128,128,128,128,128};
_declspec(align(16)) static  UINT8 Eight128s[8] =  {128,128,128,128,128,128,128,128};

#pragma warning( disable : 4799 )  // Disable no emms instruction warning!
                                      
/****************************************************************************
*  Forward References
*****************************************************************************
*/  

/****************************************************************************
* 
*   Routine:    WmtDequant
*
*   Purpose:    The reverse of routine quantize, this routine takes a Q_LIST
*               list of quantized values and multipies by the relevant value in
*               quantization array. 
*
*   Parameters :  
*       Input :
*           quantized_list :: Q_LIST
*                      -- The quantized values in zig-zag order
*       Output :
*           DCT_block      :: INT32 *              
*                      -- The expanded values in a 2-D block
*
*   Return value :
*       None.
*
* 
****************************************************************************
*/
void WmtDequant( PB_INSTANCE *pbi, Q_LIST_ENTRY * quantized_list, INT32 * DCT_block )
{
    INT16 * DequantBufferPtr = pbi->DequantBuffer;
	INT16 * dequantcoeffbufferptr =pbi->dequant_coeffs; 

__asm
	{
		mov         eax,dword ptr [quantized_list]	; Qunatised data pointer.
		mov         ebx,dword ptr [dequantcoeffbufferptr]  ; Quantiser coeffs
		mov         ecx,dword ptr [DequantBufferPtr]; Temp data buffer

        // First group of Eight values
    	movdqa		xmm0, [eax]					    ; Copy eight words from each source.
	    movdqa		xmm1, [ebx]					    
        pmullw      xmm0, xmm1 
        movdqa      XMMWORD ptr [ecx],xmm0          ; Save results

        // Group 2
    	movdqa		xmm0, [eax + 16]				; Copy eight words from each source.
	    movdqa		xmm1, [ebx + 16]   
        pmullw      xmm0, xmm1 
        movdqa      XMMWORD ptr [ecx],xmm0          ; Save results


        // Group 3
    	movdqa		xmm0, [eax + 32]				; Copy eight words from each source.
	    movdqa		xmm1, [ebx + 32]					    
        pmullw      xmm0, xmm1 
        movdqa      XMMWORD ptr [ecx + 32],xmm0     ; Save results

        // Group 4
    	movdqa		xmm0, [eax + 48]				; Copy eight words from each source.
	    movdqa		xmm1, [ebx + 48]					    
        pmullw      xmm0, xmm1 
        movdqa      XMMWORD ptr [ecx + 48],xmm0     ; Save results

        // Group 5
    	movdqa		xmm0, [eax +64]					; Copy eight words from each source.
	    movdqa		xmm1, [ebx +64]					    
        pmullw      xmm0, xmm1 
        movdqa      XMMWORD ptr [ecx +64],xmm0      ; Save results

        // Group 6
    	movdqa		xmm0, [eax + 80]				; Copy eight words from each source.
	    movdqa		xmm1, [ebx + 80]					    
        pmullw      xmm0, xmm1 
        movdqa      XMMWORD ptr [ecx + 80],xmm0     ; Save results

        // Group 7
    	movdqa		xmm0, [eax + 96]				; Copy eight words from each source.
	    movdqa		xmm1, [ebx + 96]					    
        pmullw      xmm0, xmm1 
        movdqa      XMMWORD ptr [ecx + 96],xmm0     ; Save results

        // Group 8
    	movdqa		xmm0, [eax + 112]				; Copy eight words from each source.
	    movdqa		xmm1, [ebx + 112]					    
        pmullw      xmm0, xmm1 
        movdqa      XMMWORD ptr [ecx + 112],xmm0    ; Save results

    }

    // Copy data to output buffer undoing Zig-zag and Transposing order as we go.
    DCT_block[0] = pbi->DequantBuffer[0];
    DCT_block[1] = pbi->DequantBuffer[1];
    DCT_block[8] = pbi->DequantBuffer[2];
    DCT_block[16] = pbi->DequantBuffer[3];
    DCT_block[9] = pbi->DequantBuffer[4];
    DCT_block[2] = pbi->DequantBuffer[5];
    DCT_block[3] = pbi->DequantBuffer[6];
    DCT_block[10] = pbi->DequantBuffer[7];

    DCT_block[17] = pbi->DequantBuffer[8];
    DCT_block[24] = pbi->DequantBuffer[9];
    DCT_block[32] = pbi->DequantBuffer[10];
    DCT_block[25] = pbi->DequantBuffer[11];
    DCT_block[18] = pbi->DequantBuffer[12];
    DCT_block[11] = pbi->DequantBuffer[13];
    DCT_block[4] = pbi->DequantBuffer[14];
    DCT_block[5] = pbi->DequantBuffer[15];

    DCT_block[12] = pbi->DequantBuffer[16];
    DCT_block[19] = pbi->DequantBuffer[17];
    DCT_block[26] = pbi->DequantBuffer[18];
    DCT_block[33] = pbi->DequantBuffer[19];
    DCT_block[40] = pbi->DequantBuffer[20];
    DCT_block[48] = pbi->DequantBuffer[21];
    DCT_block[41] = pbi->DequantBuffer[22];
    DCT_block[34] = pbi->DequantBuffer[23];

    DCT_block[27] = pbi->DequantBuffer[24];
    DCT_block[20] = pbi->DequantBuffer[25];
    DCT_block[13] = pbi->DequantBuffer[26];
    DCT_block[6] = pbi->DequantBuffer[27];
    DCT_block[7] = pbi->DequantBuffer[28];
    DCT_block[14] = pbi->DequantBuffer[29];
    DCT_block[21] = pbi->DequantBuffer[30];
    DCT_block[28] = pbi->DequantBuffer[31];

    DCT_block[35] = pbi->DequantBuffer[32];
    DCT_block[42] = pbi->DequantBuffer[33];
    DCT_block[49] = pbi->DequantBuffer[34];
    DCT_block[56] = pbi->DequantBuffer[35];
    DCT_block[57] = pbi->DequantBuffer[36];
    DCT_block[50] = pbi->DequantBuffer[37];
    DCT_block[43] = pbi->DequantBuffer[38];
    DCT_block[36] = pbi->DequantBuffer[39];

    DCT_block[29] = pbi->DequantBuffer[40];
    DCT_block[22] = pbi->DequantBuffer[41];
    DCT_block[15] = pbi->DequantBuffer[42];
    DCT_block[23] = pbi->DequantBuffer[43];
    DCT_block[30] = pbi->DequantBuffer[44];
    DCT_block[37] = pbi->DequantBuffer[45];
    DCT_block[44] = pbi->DequantBuffer[46];
    DCT_block[51] = pbi->DequantBuffer[47];

    DCT_block[58] = pbi->DequantBuffer[48];
    DCT_block[59] = pbi->DequantBuffer[49];
    DCT_block[52] = pbi->DequantBuffer[50];
    DCT_block[45] = pbi->DequantBuffer[51];
    DCT_block[38] = pbi->DequantBuffer[52];
    DCT_block[31] = pbi->DequantBuffer[53];
    DCT_block[39] = pbi->DequantBuffer[54];
    DCT_block[46] = pbi->DequantBuffer[55];

    DCT_block[53] = pbi->DequantBuffer[56];
    DCT_block[60] = pbi->DequantBuffer[57];
    DCT_block[61] = pbi->DequantBuffer[58];
    DCT_block[54] = pbi->DequantBuffer[59];
    DCT_block[47] = pbi->DequantBuffer[60];
    DCT_block[55] = pbi->DequantBuffer[61];
    DCT_block[62] = pbi->DequantBuffer[62];
    DCT_block[63] = pbi->DequantBuffer[63];
}


/****************************************************************************
 * 
 *  ROUTINE       :     WmtReconIntra
 *
 *  INPUTS        :     INT16 *  idct
 *                               Pointer to the output from the idct for this block
 *
 *                      UINT32   stride
 *                               Line Length in pixels in recon and reference images
 *                               
 *
 *                     
 *
 *  OUTPUTS       :     UINT8 *  dest
 *                               The reconstruction buffer
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Reconstructs an intra block - wmt version
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void WmtReconIntra( PB_INSTANCE *pbi, UINT8 * dest, INT16 * idct, UINT32 stride )
{
    __asm
    {
		push		ebx								;/// dbm -- added 5/20/02 -- lack was causing block bug on P4's!!!!

        mov         eax,[idct]						; Signed 16 bit inputs
        mov         edx,[dest]						; Unsigned 8 bit outputs

        movq		xmm0,QWORD PTR [Eight128s]		; Set xmm0 to 0x000000000000008080808080808080
		pxor		xmm3, xmm3						; set xmm3 to 0
													;
        mov         ebx,[stride]					; Line stride in output buffer
        lea         ecx,[eax+128]					; Endpoint in input buffer

loop_label:                                 

        movdqa		xmm2,XMMWORD PTR [eax]			; Read the eight inputs
		packsswb	xmm2,xmm3						;		
		
		pxor        xmm2,xmm0						; Convert result to unsigned (same as add 128)
        lea         eax,[eax + 16]					; Step source buffer

        cmp         eax,ecx							; are we done
        movq		QWORD PTR [edx],xmm2			; store results

        lea         edx,[edx+ebx]					; Step output buffer
        jc          loop_label						; Loop back if we are not done

		pop			ebx								;/// dbm -- added 5/20/02 -- 
    }

}

/****************************************************************************
 * 
 *  ROUTINE       :     WmtReconInter
 *
 *  INPUTS        :     UINT8 *  RefPtr
 *                               The last frame reference
 *
 *                      INT16 *  ChangePtr
 *                               Pointer to the change data
 *
 *                      UINT32   LineStep
 *                               Line Length in pixels in recon and ref images
 *
 *  OUTPUTS       :     UINT8 *  ReconPtr
 *                               The reconstruction
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Reconstructs data from last data and change
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void WmtReconInter( PB_INSTANCE *pbi, UINT8 * ReconPtr, UINT8 * RefPtr, INT16 * ChangePtr, UINT32 LineStep )
{
    (void) pbi;

 _asm {
		push	edi
		
		mov		ebx, [RefPtr]
		mov		ecx, [ChangePtr]

		mov		eax, [ReconPtr]
		mov		edx, [LineStep]

		pxor	xmm0, xmm0
		lea		edi, [ecx + 128]
  L:
		movq	xmm2, QWORD ptr [ebx]		; (+3 misaligned) 8 reference pixels
		movdqa	xmm4, XMMWORD ptr [ecx]		; 8 changes
		
		punpcklbw xmm2, xmm0				; 

		add	ebx, edx						; next row of reference pixels
		paddsw	xmm2, xmm4					; add in first 4 changes

		lea		ecx, [ecx + 16]				; next row of changes
		packuswb xmm2, xmm0					; pack result to unsigned 8-bit values

		cmp		ecx, edi					; are we done?
		movq	QWORD PTR [eax], xmm2		; store result

		lea		eax, [eax+edx]				; next row of output
		jc		L							; 12c / 8 elts = 18c / 8 pixels = 2.25 c/pix

		pop		edi
 }

}
/****************************************************************************
 * 
 *  ROUTINE       :     WmtReconInterHalfPixel2
 *
 *  INPUTS        :     UINT8 *  RefPtr1, RefPtr2
 *                               The last frame reference
 *
 *                      INT16 *  ChangePtr
 *                               Pointer to the change data
 *
 *                      UINT32   LineStep
 *                               Line Length in pixels in recon and ref images
 *                               
 *
 *  OUTPUTS       :     UINT8 *  ReconPtr
 *                               The reconstruction
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Reconstructs data from half pixel reference data and change. 
 *                      Half pixel data interpolated from 2 references.
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void WmtReconInterHalfPixel2( PB_INSTANCE *pbi, UINT8 * ReconPtr, 
		    	              UINT8 * RefPtr1, UINT8 * RefPtr2, 
						      INT16 * ChangePtr, UINT32 LineStep )
{

 _asm {
	push	esi
	push	edi

	mov		ecx, [ChangePtr]
	mov		esi, [RefPtr1]

	mov		edi, [RefPtr2]
	mov		ebx, [ReconPtr]
	
	mov		edx, [LineStep]
	lea		eax, [ecx+128]

	pxor	xmm0, xmm0

  L:
	
	movq		xmm2, QWORD PTR [esi]		; (+3 misaligned) mm2 = row from ref1
	movq		xmm4, QWORD PTR [edi]		; (+3 misaligned) mm4 = row from ref2

	punpcklbw	xmm2, xmm0					;
	punpcklbw	xmm4, xmm0					;

	movdqa		xmm6, [ecx]					; mm6 = first 4 changes
	paddw		xmm2, xmm4					; mm2 = start (ref1 + ref2)


	psrlw		xmm2, 1						; mm2 = start (ref1 + ref2)/2
	paddw		xmm2, xmm6					; add changes to start

	lea			ecx, [ecx+16]				; next row idct
	packuswb	xmm2, xmm0					; pack start|end to unsigned 8-bit
	
	add			esi, edx					; next row ref1
	add			edi, edx					; next row ref2
	
	cmp			ecx, eax
	movq		QWORD PTR [ebx], xmm2		; store result
	 ;
	lea			ebx, [ebx+edx]
	jc		L				

	pop		edi
	pop		esi
 }
}



/****************************************************************************
 * 
 *  ROUTINE       :     WmtClearDownQFragData
 *
 *  INPUTS        :     None. 
 *                      
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Clears down the data structure that is used
 *                      to store quantised dct coefficients for each block.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void WmtClearDownQFragData(PB_INSTANCE *pbi)
{
    UINT32       i;
    UINT32 *    QFragPtr;
	
	for ( i = 0; i < pbi->UnitFragments; i++ )
	{
		// Get the linear index for the current fragment.
		QFragPtr = (UINT32*)(&pbi->QFragData[i]);
		__asm
		{
			// Set up data pointers
			pxor        xmm6, xmm6					    ; Blank xmm6
            mov         eax, dword ptr [QFragPtr]       ; Load the source data pointer
            movdqa      xmm7, xmm6                      ; Copy blank mm6 register to xmm7
            mov         ecx,eax                         ; Load a second source data pointer
            movdqa      xmmword ptr [eax],xmm6          ; Write 4 16 bit 0's back
            movdqa      xmmword ptr [ecx + 16 ],xmm7             
            movdqa      xmmword ptr [eax + 32 ],xmm6             
            movdqa      xmmword ptr [ecx + 48 ],xmm7             
            movdqa      xmmword ptr [eax + 64 ],xmm6             
            movdqa      xmmword ptr [ecx + 80 ],xmm7             
            movdqa      xmmword ptr [eax + 96 ],xmm6             
            movdqa      xmmword ptr [ecx + 112],xmm7             
		}
	}
}



/****************************************************************************
 * 
 *  ROUTINE       :     WmtReconPostProcess
 *
 *  INPUTS        :     UINT8 *  RefPtr
 *                               The last frame reference
 *
 *                      INT16 *  ChangePtr
 *                               Pointer to the change data
 *
 *                      UINT32   LineStep
 *                               Line Length in pixels in recon and ref images
 *
 *  OUTPUTS       :     UINT8 *  ReconPtr
 *                               The reconstruction
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Reconstructs data from last data and change
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void WmtReconPostProcess( PB_INSTANCE *pbi, 
						  UINT8 * DestPtr, 
						  UINT8 * SrcPtr, 
						  INT16 * ChangePtr, 
						  UINT32 PlaneLineStep )
{
    (void) pbi;

 _asm {
	push	edi
	mov		ebx, [SrcPtr]

	mov		ecx, [ChangePtr]
	mov		eax, [DestPtr]

	mov		edx, [PlaneLineStep]
	pxor	xmm0, xmm0					;	0000000000000000

	lea		edi, [ecx + 128]
	 ;
  L:
	movq		xmm2, QWORD PTR [ebx]	; (+3 misaligned) 8 reference pixels
	movdqa		xmm4, XMMWORD PTR [ecx]	; Eight changes

	punpcklbw	xmm2, xmm0				; change inot positive 16-bit #s
	paddsw		xmm2, xmm4				; add in 8 changes

	add			ebx, edx				; next row of reference pixels
	packuswb	xmm2, xmm0				; pack result to unsigned 8-bit values
	
	lea			ecx, [ecx + 16]			; next row of changes
	cmp			ecx, edi				; are we done?
	 ;
	movq		QWORD PTR [eax], xmm2	; store result
	lea			eax, [eax+edx]			; next row of output
	jc		L							; 

	pop		edi
 }
}

