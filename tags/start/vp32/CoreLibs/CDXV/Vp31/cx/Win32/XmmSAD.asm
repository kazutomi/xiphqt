;//==========================================================================
;//
;//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
;//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
;//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
;//  PURPOSE.
;//
;//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
;//
;//--------------------------------------------------------------------------


INCLUDE iaxmm.inc
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
        .MMX

; macros

STRIDE_EXTRA    equ 32

        .DATA
TORQ_CX_DATA SEGMENT PAGE PUBLIC USE32 'DATA' 

        ALIGN 32


        .CODE

NAME XmmGetSAD

PUBLIC XMMGetSAD_
PUBLIC _XMMGetSAD
 
INCLUDE XmmSAD.ash

;------------------------------------------------
; local vars
LOCAL_SPACE     EQU 0


;------------------------------------------------
;INT32 XMMGetSAD( UINT8 * NewDataPtr, UINT8  * RefDataPtr, 
;                 UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar  )
;
XMMGetSAD_:
_XMMGetSAD:

	    push    ecx
	    push    ebx 
	    push    edx

		mov     ecx,	(XMMGetSADParams PTR [esp-12]).PixelsPerLine
		mov     eax,	(XMMGetSADParams PTR [esp-12]).NewDataPtr	

	    push    esi
													; Load base addresses
		mov     ebx,	(XMMGetSADParams PTR [esp-8]).RefDataPtr
		movq	mm0,	[eax]					; Copy eight bytes to mm0


		push    edi

;		push    ebp


;
; ESP = Stack Pointer                      MM0 = Free
; ESI = Free                               MM1 = Free
; EDI = Free                               MM2 = Free
; EBP = Free                               MM3 = Free
; EBX = RefDataPtr                         MM4 = Free
; ECX = PixelsPerLine                      MM5 = Free
; EDX = PixelsPerLine + STRIDE_EXTRA       MM6 = Free
; EAX = NewDataPtr                         MM7 = Free
;



        ; Row 1
		lea			edx, [ecx+STRIDE_EXTRA]
		lea			esi, [eax+2*ecx];			; Calculate the source ptr for row4
        psadbw      mm0, [ebx]

        ; Row 2
		movq		mm1, [eax+ecx]				; Copy eight bytes to mm1
		lea			edi, [ebx+2*edx]			; Calculate the source ptr for row4
        psadbw      mm1, [ebx+edx]



        ; Row 3
		movq		mm2, [eax+2*ecx]			; Copy eight bytes to mm2
		add			esi, ecx;					; Calculate the source ptr for row4
        psadbw      mm2, [ebx+2*edx]


		add			edi, edx;					; Calculate the source ptr for row4
		        		
		; Row 4
		movq		mm3, [esi]					; Copy eight bytes to mm3
        psadbw      mm3, [edi]



        ; Row 5
		movq		mm4, [eax+4*ecx]			; Copy eight bytes to mm4
        paddd       mm0,mm1						
        psadbw      mm4, [ebx+4*edx]



        ; Row 6
		movq		mm5, [esi+2*ecx]			; Copy eight bytes to mm5
        lea			eax, [esi+2*ecx]
        psadbw      mm5, [edi+2*edx]


		lea			ebx, [edi+2*edx]

		; Row 7
		movq		mm6, [eax+ecx]					; Copy eight bytes to mm0
        psadbw      mm6, [ebx+edx]
        paddd       mm2,mm3



        ; Row 8
		movq		mm7, [esi+4*ecx]					; Copy eight bytes to mm0
        psadbw      mm7, [edi+4*edx]

        ; start accumulating differences

;        pop     ebp
        paddd       mm4,mm5
        paddd       mm6,mm7

        pop     edi
        paddd       mm0,mm2
        paddd       mm4,mm6

        pop     esi
        paddd       mm0,mm4
        movd        ecx,mm0

theExit:
        pop     edx
    	mov         eax,[esp+24]                ; load error so far
        add         eax,ecx                     ; add in calculated error

        pop     ebx
        pop     ecx


    ret

;************************************************
        END

