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
*   Description  :     MMX or otherwise processor specific 
*                      optimised versions of functions
*
*****************************************************************************
*/

/* 
    Use Tim's optimized version.
*/
#define USING_TIMS 1

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

extern INT32 * XX_LUT;

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

INT16 Ones[4]               = {1,1,1,1};
INT16 OneTwoEight[4]        = {128,128,128,128};
UINT8 Eight128s[8]          = {128,128,128,128,128,128,128,128};

#pragma warning( disable : 4799 )  // Disable no emms instruction warning!
                                      
/****************************************************************************
*  Forward References
*****************************************************************************
*/  
/****************************************************************************
 * 
 *  ROUTINE       :     ClearSysState()
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
void ClearSysState(void)
{
}

/****************************************************************************
 * 
 *  ROUTINE       :     ClearMmx()
 *
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     Clears down the MMX state
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void ClearMmx(void)
{
	__asm
	{
		emms									; Clear the MMX state.
	}
}


/****************************************************************************
* 
*   Routine:    MmxDequant
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
void MmxDequant( PB_INSTANCE *pbi, Q_LIST_ENTRY * quantized_list, INT32 * DCT_block )
{
    INT16 * DequantBufferPtr = pbi->DequantBuffer;
	INT16 * dequantcoeffbufferptr =pbi->dequant_coeffs; 

__asm
	{
		mov         eax,dword ptr [quantized_list]	; Qunatised data pointer.
		mov         ebx,dword ptr [dequantcoeffbufferptr]  ; Quantiser coeffs
		mov         ecx,dword ptr [DequantBufferPtr]; Temp data buffer

        // First group of four values
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 2
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 3
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 4
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 5
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 6
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 7
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 8
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 9
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 10
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 11
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 12
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 13
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 14
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 15
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

        // Group 16
    	movq		mm0, [eax]					    ; Copy four words from each source.
	    movq		mm1, [ebx]					    
        pmullw      mm0, mm1 
        movq        dword ptr [ecx],mm0             ; Save results
        add         eax,8                           ; increment the buffer pointers
        add         ebx,8                           
        add         ecx,8                           

       	//emms									    ; Clear the MMX state.
    }

    // Copy data to output buffer undoing Zig-zag order as we go.
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
 *  ROUTINE       :     MMXReconIntra
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
 *  FUNCTION      :     Reconstructs an intra block - MMX version
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void MMXReconIntra( PB_INSTANCE *pbi, UINT8 * dest, INT16 * idct, UINT32 stride )
{
    __asm
    {
        // u    pipe
        //   v  pipe
        mov         eax,[idct]              ; Signed 16 bit inputs
          mov         edx,[dest]            ; Signed 8 bit outputs
        movq        mm0,[Eight128s]         ; Set mm0 to 0x8080808080808080
          ;
        mov         ebx,[stride]            ; Line stride in output buffer
          lea         ecx,[eax+128]         ; Endpoint in input buffer
loop_label:                                 ;
        movq        mm2,[eax]               ; First four input values
          ;
        packsswb    mm2,[eax+8]             ; pack with next(high) four values
          por         mm0,mm0               ; stall
        pxor        mm2,mm0                 ; Convert result to unsigned (same as add 128)
          lea         eax,[eax + 16]        ; Step source buffer
        cmp         eax,ecx                 ; are we done
          ;
        movq        [edx],mm2               ; store results
          ;
        lea         edx,[edx+ebx]           ; Step output buffer
          jc          loop_label            ; Loop back if we are not done
    }
    // 6c/8 elts = 9c/8 = 1.125 c/pix

}

/****************************************************************************
 * 
 *  ROUTINE       :     MmxReconInter
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
#if USING_TIMS
void MmxReconInter( PB_INSTANCE *pbi, UINT8 * ReconPtr, UINT8 * RefPtr, INT16 * ChangePtr, UINT32 LineStep )
{
    (void) pbi;

 _asm {
	push	edi
;;	 mov	ebx, [ref]
;;	mov		ecx, [diff]
;;	 mov	eax, [dest]
;;	mov		edx, [stride]
	 mov	ebx, [RefPtr]
	mov		ecx, [ChangePtr]
	 mov	eax, [ReconPtr]
	mov		edx, [LineStep]
	 pxor	mm0, mm0
	lea		edi, [ecx + 128]
	 ;
  L:
	movq	mm2, [ebx]			; (+3 misaligned) 8 reference pixels
	 ;
	movq	mm4, [ecx]			; first 4 changes
	 movq	mm3, mm2
	movq	mm5, [ecx + 8]		; last 4 changes
	 punpcklbw mm2, mm0			; turn first 4 refs into positive 16-bit #s
	paddsw	mm2, mm4			; add in first 4 changes
	 punpckhbw mm3, mm0			; turn last 4 refs into positive 16-bit #s
	paddsw	mm3, mm5			; add in last 4 changes
	 add	ebx, edx			; next row of reference pixels
	packuswb mm2, mm3			; pack result to unsigned 8-bit values
	 lea	ecx, [ecx + 16]		; next row of changes
	cmp		ecx, edi			; are we done?
	 ;
	movq	[eax], mm2			; store result
	 ;
	lea		eax, [eax+edx]		; next row of output
	 jc		L					; 12c / 8 elts = 18c / 8 pixels = 2.25 c/pix

	pop		edi
 }
}
#else
void MmxReconInter( PB_INSTANCE *pbi, UINT8 * ReconPtr, UINT8 * RefPtr, INT16 * ChangePtr, UINT32 LineStep )
{

    // Note that the line step for the change data is assumed to be 8 * 32 bits.
__asm
    {
        // Set up data pointers
        mov         eax,dword ptr [ReconPtr]  
        mov         ebx,dword ptr [RefPtr]      
        mov         ecx,dword ptr [ChangePtr]   
		mov         edx,dword ptr [LineStep]
		pxor        mm6, mm6					; Blank mmx6

        // Row 1
        // Load the data values. The change data needs to be unpacked to words
        movq        mm0,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data
        paddsw      mm0, mm2                    ; First 4 values
        paddsw      mm1, mm4                    ; Second 4 values

        // Pack and store
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [eax],mm0         ; Write the data out to the results buffer

		add         ebx,edx						; Step the reference pointer.
        add         ecx,16                      ; Step the change pointer.
        add         eax,edx                     ; Step the reconstruction pointer

        // Row 2
        // Load the data values. The change data needs to be unpacked to words
        movq        mm0,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data
        paddsw      mm0, mm2                    ; First 4 values
        paddsw      mm1, mm4                    ; Second 4 values

        // Pack and store
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [eax],mm0         ; Write the data out to the results buffer

		add         ebx,edx						; Step the reference pointer.
        add         ecx,16                      ; Step the change pointer.
        add         eax,edx                     ; Step the reconstruction pointer

        // Row 3
        // Load the data values. The change data needs to be unpacked to words
        movq        mm0,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data
        paddsw      mm0, mm2                    ; First 4 values
        paddsw      mm1, mm4                    ; Second 4 values

        // Pack and store
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [eax],mm0         ; Write the data out to the results buffer

		add         ebx,edx						; Step the reference pointer.
        add         ecx,16                      ; Step the change pointer.
        add         eax,edx                     ; Step the reconstruction pointer

        // Row 4
        // Load the data values. The change data needs to be unpacked to words
        movq        mm0,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data
        paddsw      mm0, mm2                    ; First 4 values
        paddsw      mm1, mm4                    ; Second 4 values

        // Pack and store
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [eax],mm0         ; Write the data out to the results buffer

		add         ebx,edx						; Step the reference pointer.
        add         ecx,16                      ; Step the change pointer.
        add         eax,edx                     ; Step the reconstruction pointer

        // Row 5
        // Load the data values. The change data needs to be unpacked to words
        movq        mm0,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data
        paddsw      mm0, mm2                    ; First 4 values
        paddsw      mm1, mm4                    ; Second 4 values

        // Pack and store
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [eax],mm0         ; Write the data out to the results buffer

		add         ebx,edx						; Step the reference pointer.
        add         ecx,16                      ; Step the change pointer.
        add         eax,edx                     ; Step the reconstruction pointer

        // Row 6
        // Load the data values. The change data needs to be unpacked to words
        movq        mm0,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data
        paddsw      mm0, mm2                    ; First 4 values
        paddsw      mm1, mm4                    ; Second 4 values

        // Pack and store
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [eax],mm0         ; Write the data out to the results buffer

		add         ebx,edx						; Step the reference pointer.
        add         ecx,16                      ; Step the change pointer.
        add         eax,edx                     ; Step the reconstruction pointer

        // Row 7
        // Load the data values. The change data needs to be unpacked to words
        movq        mm0,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data
        paddsw      mm0, mm2                    ; First 4 values
        paddsw      mm1, mm4                    ; Second 4 values

        // Pack and store
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [eax],mm0         ; Write the data out to the results buffer

		add         ebx,edx						; Step the reference pointer.
        add         ecx,16                      ; Step the change pointer.
        add         eax,edx                     ; Step the reconstruction pointer

        // Row 8
        // Load the data values. The change data needs to be unpacked to words
        movq        mm0,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data
        paddsw      mm0, mm2                    ; First 4 values
        paddsw      mm1, mm4                    ; Second 4 values

        // Pack and store
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [eax],mm0         ; Write the data out to the results buffer
   
        //emms									; Clear the MMX state.
    }
}
#endif

/****************************************************************************
 * 
 *  ROUTINE       :     MmxReconInterHalfPixel2
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
#if USING_TIMS

#define A 0

void MmxReconInterHalfPixel2( PB_INSTANCE *pbi, UINT8 * ReconPtr, 
		    	              UINT8 * RefPtr1, UINT8 * RefPtr2, 
						      INT16 * ChangePtr, UINT32 LineStep )
{
#	if A
		static culong FourOnes[2] = { 65537, 65537};	// only read once
#	endif

 _asm {
	push	esi
	 push	edi

;;	mov		ecx, [diff]
;;	 mov	esi, [ref1]
;;	mov		edi, [ref2]
;;	 mov	ebx, [dest]
;;	mov		edx, [stride]

	mov		ecx, [ChangePtr]
	 mov	esi, [RefPtr1]
	mov		edi, [RefPtr2]
	 mov	ebx, [ReconPtr]
	mov		edx, [LineStep]

	 lea	eax, [ecx+128]

#	if A
		movq	mm1, [FourOnes]
#	endif

	 pxor	mm0, mm0
  L:
	movq	mm2, [esi]		; (+3 misaligned) mm2 = row from ref1
	 ;
	movq	mm4, [edi]		; (+3 misaligned) mm4 = row from ref2
	 movq	mm3, mm2
	punpcklbw mm2, mm0		; mm2 = start ref1 as positive 16-bit #s
	 movq	mm5, mm4
	movq	mm6, [ecx]		; mm6 = first 4 changes
	 punpckhbw mm3, mm0		; mm3 = end ref1 as positive 16-bit #s
	movq	mm7, [ecx+8]	; mm7 = last 4 changes
	 punpcklbw mm4, mm0		; mm4 = start ref2 as positive 16-bit #s
	punpckhbw mm5, mm0		; mm5 = end ref2 as positive 16-bit #s
	 paddw	mm2, mm4		; mm2 = start (ref1 + ref2)
	paddw	mm3, mm5		; mm3 = end (ref1 + ref2)

#	if A
		 paddw	mm2, mm1		; rounding adjustment
		paddw	mm3, mm1
#	endif

	 psrlw	mm2, 1			; mm2 = start (ref1 + ref2)/2
	psrlw	mm3, 1			; mm3 = end (ref1 + ref2)/2
	 paddw	mm2, mm6		; add changes to start
	paddw	mm3, mm7		; add changes to end
	 lea	ecx, [ecx+16]	; next row idct
	packuswb mm2, mm3		; pack start|end to unsigned 8-bit
	 add	esi, edx		; next row ref1
	add		edi, edx		; next row ref2
	 cmp	ecx, eax
	movq	[ebx], mm2		; store result
	 ;
	lea		ebx, [ebx+edx]
	 jc		L				; 22c / 8 elts = 33c / 8 pixels = 4.125 c/pix

	pop		edi
	 pop	esi
 }
}

#undef A

#else
void MmxReconInterHalfPixel2( PB_INSTANCE *pbi, UINT8 * ReconPtr, 
		    	              UINT8 * RefPtr1, UINT8 * RefPtr2, 
						      INT16 * ChangePtr, UINT32 LineStep )
{
    UINT8 * TmpDataPtr = (UINT8 *)pbi->TmpReconBuffer;

    // Note that the line step for the change data is assumed to be 8 * 32 bits.
    __asm
    {
		pxor        mm6, mm6					; Blank mmx6

        // Set up data pointers
        mov         eax,dword ptr [RefPtr1]      
        mov         ebx,dword ptr [RefPtr2]      
        mov         edx,dword ptr [LineStep]

        // Row 1
        // Load the change pointer
        mov         ecx,dword ptr [ChangePtr]   

        // Load the data values (Ref1 and Ref2) and unpack to signed 16 bit values
        movq        mm0,dword ptr [eax]         ; Load 8 elements of source data
        movq        mm2,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
        movq        mm3, mm2                    ; Copy data

        punpcklbw   mm0, mm6					; Low bytes to words
		punpcklbw   mm2, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words
		punpckhbw   mm3, mm6					; High bytes to words

        // Average Ref1 and Ref2
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm3                    ; Second 4 values
        psrlw       mm0, 1
        psrlw       mm1, 1

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data reference and difference data
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm4                    ; Second 4 values

        // Pack and store
        mov         ecx,dword ptr [TmpDataPtr]  ; Load the temp results pointer 
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [ecx],mm0         ; Write the data out to the temporary results buffer
        add         eax,edx                     ; Step the reference pointers
        add         ebx,edx                    

        // Row 2
        // Load the change pointer
        mov         ecx,dword ptr [ChangePtr]   
        add         ecx,16                    

        // Load the data values (Ref1 and Ref2). 
        movq        mm0,dword ptr [eax]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words

        movq        mm2,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm3, mm2                    ; Copy data
		punpcklbw   mm2, mm6					; Low bytes to words
		punpckhbw   mm3, mm6					; High bytes to words

        // Average Ref1 and Ref2
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm3                    ; Second 4 values
        psrlw       mm0, 1
        psrlw       mm1, 1

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data reference and difference data
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm4                    ; Second 4 values

        // Pack and store
        mov         ecx,dword ptr [TmpDataPtr]  ; Load the temp results pointer 
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [ecx+8],mm0       ; Write the data out to the temporary results buffer
        add         eax,edx                     ; Step the reference pointers
        add         ebx,edx                    

        // Row 3
        // Load the change pointer
        mov         ecx,dword ptr [ChangePtr]   
        add         ecx,32                    

        // Load the data values (Ref1 and Ref2). 
        movq        mm0,dword ptr [eax]         ; Load 8 elements of source data
        movq        mm2,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
        movq        mm3, mm2                    ; Copy data

		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words
		punpcklbw   mm2, mm6					; Low bytes to words
		punpckhbw   mm3, mm6					; High bytes to words

        // Average Ref1 and Ref2
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm3                    ; Second 4 values
        psrlw       mm0, 1
        psrlw       mm1, 1

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data reference and difference data
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm4                    ; Second 4 values

        // Pack and store
        mov         ecx,dword ptr [TmpDataPtr]   
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [ecx+16],mm0         ; Write the data out to the temporary results buffer
        add         eax,edx                     ; Step the reference pointers
        add         ebx,edx                    

        // Row 4
        // Load the change pointer
        mov         ecx,dword ptr [ChangePtr]   
        add         ecx,48                    

        // Load the data values (Ref1 and Ref2). 
        movq        mm0,dword ptr [eax]         ; Load 8 elements of source data
        movq        mm2,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
        movq        mm3, mm2                    ; Copy data

		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words
		punpcklbw   mm2, mm6					; Low bytes to words
		punpckhbw   mm3, mm6					; High bytes to words

        // Average Ref1 and Ref2
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm3                    ; Second 4 values
        psrlw       mm0, 1
        psrlw       mm1, 1

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data reference and difference data
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm4                    ; Second 4 values

        // Pack and store
        mov         ecx,dword ptr [TmpDataPtr]   
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [ecx+24],mm0      ; Write the data out to the temporary results buffer
        add         eax,edx                     ; Step the reference pointers
        add         ebx,edx                    

        // Row 5
        // Load the change pointer
        mov         ecx,dword ptr [ChangePtr]   
        add         ecx,64                 

        // Load the data values (Ref1 and Ref2). 
        movq        mm0,dword ptr [eax]         ; Load 8 elements of source data
        movq        mm2,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
        movq        mm3, mm2                    ; Copy data

		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words
		punpcklbw   mm2, mm6					; Low bytes to words
		punpckhbw   mm3, mm6					; High bytes to words

        // Average Ref1 and Ref2
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm3                    ; Second 4 values
        psrlw       mm0, 1
        psrlw       mm1, 1

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data reference and difference data
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm4                    ; Second 4 values

        // Pack and store
        mov         ecx,dword ptr [TmpDataPtr]   
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [ecx+32],mm0      ; Write the data out to the temporary results buffer
        add         eax,edx                     ; Step the reference pointers
        add         ebx,edx                    

        // Row 6
        // Load the change pointer
        mov         ecx,dword ptr [ChangePtr]   
        add         ecx,80                    

        // Load the data values (Ref1 and Ref2). 
        movq        mm0,dword ptr [eax]         ; Load 8 elements of source data
        movq        mm2,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
        movq        mm3, mm2                    ; Copy data

		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words
		punpcklbw   mm2, mm6					; Low bytes to words
		punpckhbw   mm3, mm6					; High bytes to words

        // Average Ref1 and Ref2
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm3                    ; Second 4 values
        psrlw       mm0, 1
        psrlw       mm1, 1

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data reference and difference data
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm4                    ; Second 4 values

        // Pack and store
        mov         ecx,dword ptr [TmpDataPtr]   
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [ecx+40],mm0      ; Write the data out to the temporary results buffer
        add         eax,edx                     ; Step the reference pointers
        add         ebx,edx                    

        // Row 7
        // Load the change pointer
        mov         ecx,dword ptr [ChangePtr]   
        add         ecx,96                    

        // Load the data values (Ref1 and Ref2). 
        movq        mm0,dword ptr [eax]         ; Load 8 elements of source data
        movq        mm2,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
        movq        mm3, mm2                    ; Copy data

		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words
		punpcklbw   mm2, mm6					; Low bytes to words
		punpckhbw   mm3, mm6					; High bytes to words

        // Average Ref1 and Ref2
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm3                    ; Second 4 values
        psrlw       mm0, 1
        psrlw       mm1, 1

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data reference and difference data
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm4                    ; Second 4 values

        // Pack and store
        mov         ecx,dword ptr [TmpDataPtr]   
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [ecx+48],mm0      ; Write the data out to the temporary results buffer
        add         eax,edx                     ; Step the reference pointers
        add         ebx,edx                    

        // Row 8
        // Load the change pointer
        mov         ecx,dword ptr [ChangePtr]   
        add         ecx,112                    

        // Load the data values (Ref1 and Ref2). 
        movq        mm0,dword ptr [eax]         ; Load 8 elements of source data
        movq        mm2,dword ptr [ebx]         ; Load 8 elements of source data
        movq        mm1, mm0                    ; Copy data
        movq        mm3, mm2                    ; Copy data

		punpcklbw   mm0, mm6					; Low bytes to words
		punpckhbw   mm1, mm6					; High bytes to words
		punpcklbw   mm2, mm6					; Low bytes to words
		punpckhbw   mm3, mm6					; High bytes to words

        // Average Ref1 and Ref2
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm3                    ; Second 4 values
        psrlw       mm0, 1
        psrlw       mm1, 1

        // Load 8 elements of 16 bit change data
        movq        mm2,dword ptr [ecx]         ; Load 4 elements of change data
        movq        mm4,dword ptr [ecx+8]       ; Load next 4 elements of change data

        // Sum the data reference and difference data
        paddw       mm0, mm2                    ; First 4 values
        paddw       mm1, mm4                    ; Second 4 values

        // Pack and store
        mov         ecx,dword ptr [TmpDataPtr]   
        packuswb    mm0, mm1                    ; Then pack and saturate to unsigned bytes
        movq        dword ptr [ecx+56],mm0      ; Write the data out to the temporary results buffer


        // Now copy the results back to the reconstruction buffer.
        mov         eax,dword ptr [ReconPtr]    ; Load the reconstruction Pointer  
        mov         ecx,dword ptr [TmpDataPtr]  ; Load the temp results pointer 
        // Row 1
        movq        mm0,dword ptr [ecx]         ; Load 8 elements of results data
        movq        dword ptr [eax],mm0         ; Write the data tot he reconstruction buffer.
        add         eax,edx                     ; Step the reconstruction pointer
        // Row 2
        movq        mm0,dword ptr [ecx+8]       ; Load 8 elements of results data
        movq        dword ptr [eax],mm0         ; Write the data tot he reconstruction buffer.
        add         eax,edx                     ; Step the reconstruction pointer
        // Row 3
        movq        mm0,dword ptr [ecx+16]      ; Load 8 elements of results data
        movq        dword ptr [eax],mm0         ; Write the data tot he reconstruction buffer.
        add         eax,edx                     ; Step the reconstruction pointer
        // Row 4
        movq        mm0,dword ptr [ecx+24]      ; Load 8 elements of results data
        movq        dword ptr [eax],mm0         ; Write the data tot he reconstruction buffer.
        add         eax,edx                     ; Step the reconstruction pointer
        // Row 5
        movq        mm0,dword ptr [ecx+32]      ; Load 8 elements of results data
        movq        dword ptr [eax],mm0         ; Write the data tot he reconstruction buffer.
        add         eax,edx                     ; Step the reconstruction pointer
        // Row 6
        movq        mm0,dword ptr [ecx+40]      ; Load 8 elements of results data
        movq        dword ptr [eax],mm0         ; Write the data tot he reconstruction buffer.
        add         eax,edx                     ; Step the reconstruction pointer
        // Row 7
        movq        mm0,dword ptr [ecx+48]      ; Load 8 elements of results data
        movq        dword ptr [eax],mm0         ; Write the data tot he reconstruction buffer.
        add         eax,edx                     ; Step the reconstruction pointer
        // Row 8
        movq        mm0,dword ptr [ecx+56]      ; Load 8 elements of results data
        movq        dword ptr [eax],mm0         ; Write the data tot he reconstruction buffer.
        add         eax,edx                     ; Step the reconstruction pointer

        //emms
    }
}
#endif


/****************************************************************************
 * 
 *  ROUTINE       :     MMXClearDownQFragData
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
void MMXClearDownQFragData(PB_INSTANCE *pbi)
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
			pxor        mm6, mm6					    ; Blank mm6
            mov         eax, dword ptr [QFragPtr]       ; Load the source data pointer
            movq        mm7, mm6                        ; Copy blank mm6 register to mm7
            mov         ecx, eax                        ; Load a second source data pointer
            movq        dword ptr [eax],mm6             ; Write 4 16 bit 0's back
            movq        dword ptr [ecx + 8  ],mm7       ; Write next 4 16 bit 0's back
            movq        dword ptr [eax + 16 ],mm6             
            movq        dword ptr [ecx + 24 ],mm7             
            movq        dword ptr [eax + 32 ],mm6             
            movq        dword ptr [ecx + 40 ],mm7             
            movq        dword ptr [eax + 48 ],mm6             
            movq        dword ptr [ecx + 56 ],mm7             
            movq        dword ptr [eax + 64 ],mm6             
            movq        dword ptr [ecx + 72 ],mm7             
            movq        dword ptr [eax + 80 ],mm6             
            movq        dword ptr [ecx + 88 ],mm7             
            movq        dword ptr [eax + 96 ],mm6             
            movq        dword ptr [ecx + 104],mm7    
            movq        dword ptr [eax + 112],mm6             
            movq        dword ptr [ecx + 120],mm7   
		}
	}
	// Clear down mmx
//	__asm
//	{
//		emms    
//	}
}


/****************************************************************************
 * 
 *  ROUTINE       :     MMXExtractToken
 *
 *  INPUTS        :     INT8 * ExpandedBlock
 *                             Pointer to block structure into which the token
 *                             should be expanded.
 *
 *                      UINT32 * CoeffIndex
 *                             Where we are in the current block.
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     The number of bits decoded
 *
 *  FUNCTION      :     Unpacks and expands an DC DCT token.
 *
 *  SPECIAL NOTES :     PROBLEM !!!!!!!!!!!   right now handles only left 
 *                      justified bits in bitreader.  the c version keeps every
 *                      thing in place so I can't use it!!
 *                      
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
//__inline
UINT32 MMXExtractToken(BITREADER *br,HUFF_ENTRY *h)
{
	UINT32 t;
	br->remainder<<=32-br->bitsinremainder;

#define BitsAreBigEndian 1
#	if BitsAreBigEndian
#		define	swab( R)	bswap R
#	else
#		define	swab( R)
#	endif

		// huffman-decode next token, storing it in "t"

	__asm {
		push	edi					
		 mov	edi, [br]							; edi = address of bit reader 
		push	esi					
		 lea	esi, [h]							; esi = address of the root node of the huffman tree
		mov		ecx, [edi]BITREADER.bitsinremainder	; ecx = number of bits that are valid in bit reader
		 xor	eax, eax							; eax = First Fork is always Left ? in root
		mov		edx, [edi]BITREADER.remainder		; edx = value of bits in current bit reader value
		 jmp	nextB

	exactW:
		xor		ecx, ecx
		 mov	ebx, eax
		jmp		doneW

	nextW:											; need another input word, unless done already
		or		eax, ebx							; eax = ebx , carry = 0, sign = 1 
		 mov	ebx, [edi]BITREADER.position		; ebx = address of next long from BITREADER
		mov		ecx, 31								; 31 bits left in val since it is new
		 jns	exactW								; ????????????
		mov		edx, [ebx]							; edx = value of long from BITREADER
		 xor	eax, eax							; clear eax
		swab( edx)									; swap bytes in long if needed
		shl		edx, 1								; shift off 1 bit from edx into carry flag
		 lea	ebx, [ebx+4]						; ebx = next long 
		adc		eax, eax							; eax = carry flag
		 mov	[edi]BITREADER.position, ebx		; bit reader position is incremented

	nextB:				
		mov		esi, [esi + eax*4]					; esi = child node of esi selected by eax
		 xor	eax, eax							; eax = 0 
		shl		edx, 1								; shift off 1 bit from edx into carry flag
		 dec	ecx									; ecx = ecx -1 ( 1 less valid bit) (sets sign bit if 0)
		mov		ebx, [esi]HUFF_ENTRY.Value			; ebx = value of huffman node
		 js		nextW								; oops! edx was empty, carry flag bogus goto next word
		adc		eax, eax							; eax = carry flag 
		 or		ebx, ebx							; clear carry flag, set sign flag to sign of value
		js		nextB								; get next bit
		
		or		edx, eax							; put back unused input bit
		inc		ecx									; add 1 to count of bits in bit reader
		ror		edx, 1								; most significant <-> least sign bit 
		 ;		
	doneW:
		mov		[edi]BITREADER.bitsinremainder, ecx
		mov		[edi]BITREADER.remainder, edx
		mov		[t], ebx
		pop		esi
		pop		edi
	}
	br->remainder>>=32-br->bitsinremainder;
	return t;

}


/****************************************************************************
 * 
 *  ROUTINE       :     CopyBlockUsingMMX
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Copies a block from source to destination
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void CopyBlockMMX(unsigned char *src, unsigned char *dest, unsigned int srcstride)
{
	unsigned char *s = src;
	unsigned char *d = dest;
	unsigned int stride = srcstride;
	// recon copy 
	_asm
	{
			mov		ecx, [stride]
			mov		eax, [s]
			mov		ebx, [d]
			lea		edx, [ecx + ecx * 2]

			movq	mm0, [eax]
			movq	mm1, [eax + ecx]
			movq	mm2, [eax + ecx*2]
			movq	mm3, [eax + edx]

			lea		eax, [eax + ecx*4]

			movq	[ebx], mm0
			movq	[ebx + ecx], mm1
			movq	[ebx + ecx*2], mm2
			movq	[ebx + edx], mm3

			lea		ebx, [ebx + ecx * 4]

			movq	mm0, [eax]
			movq	mm1, [eax + ecx]
			movq	mm2, [eax + ecx*2]
			movq	mm3, [eax + edx]

			movq	[ebx], mm0
			movq	[ebx + ecx], mm1
			movq	[ebx + ecx*2], mm2
			movq	[ebx + edx], mm3
	}
}


/****************************************************************************
 * 
 *  ROUTINE       :     MmxReconPostProcess
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

void MmxReconPostProcess( PB_INSTANCE *pbi, UINT8 * DestPtr, UINT8 * SrcPtr, INT16 * ChangePtr, UINT32 PlaneLineStep )
{
    (void) pbi;

 _asm {
	push	edi
	 mov	ebx, [SrcPtr]
	mov		ecx, [ChangePtr]
	 mov	eax, [DestPtr]
	mov		edx, [PlaneLineStep]
	 pxor	mm0, mm0			;	0000000000000000
	lea		edi, [ecx + 128]
	 ;
  L:
	movq	mm2, [ebx]			; (+3 misaligned) 8 reference pixels
	 ;
	movq	mm4, [ecx]			; first 4 changes
	 movq	mm3, mm2
	movq	mm5, [ecx + 8]		; last 4 changes
	 punpcklbw mm2, mm0			; turn first 4 refs into positive 16-bit #s
	paddsw	mm2, mm4			; add in first 4 changes
	 punpckhbw mm3, mm0			; turn last 4 refs into positive 16-bit #s
	paddsw	mm3, mm5			; add in last 4 changes
	 add	ebx, edx			; next row of reference pixels
	packuswb mm2, mm3			; pack result to unsigned 8-bit values
	 lea	ecx, [ecx + 16]		; next row of changes
	cmp		ecx, edi			; are we done?
	 ;
	movq	[eax], mm2			; store result
	 ;
	lea		eax, [eax+edx]		; next row of output
	 jc		L					; 12c / 8 elts = 18c / 8 pixels = 2.25 c/pix

	pop		edi
 }
}

