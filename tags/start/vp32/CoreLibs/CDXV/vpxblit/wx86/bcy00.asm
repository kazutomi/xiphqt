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


;/***********************************************\
;??? bcy00.asm   
; yv12 to yuy2 same blitter
;\***********************************************/ 
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
		.MMX
 
        .CODE

NAME x86bcy00

PUBLIC bcy00_MMX_
PUBLIC _bcy00_MMX
 

INCLUDE wilk.ash
INCLUDE wblit.ash

;------------------------------------------------
; local vars
L_blkWidth      EQU 0
L_YStride       EQU L_blkWidth+4
L_UVStride      EQU L_YStride+4

LOCAL_SPACE     EQU L_UVStride+4

;------------------------------------------------
;void bcy00_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
;
bcy00_MMX_:
_bcy00_MMX:
    push    esi
    push    edi
    push    ebp
    push    ebx 
    push    ecx
    push    edx

    mov         edi,[esp].dst
    mov         ebp,[esp].buffConfig

nop
    sub         esp,LOCAL_SPACE

    mov         eax,[ebp].YStride
    mov         ecx,[ebp].UVStride

;;add eax,DWORD PTR [ebp].YWidth
;;add ecx,DWORD PTR [ebp].UVWidth    

    mov         L_YStride[esp],eax
    mov         L_UVStride[esp],ecx

    mov         eax,[ebp].YHeight
    mov         ecx,[ebp].YWidth

    shr         ecx,3                           ;blocks of 8 pixels
    mov         esi,[ebp].YBuffer

    xor         ebx,ebx
    mov         L_blkWidth[esp],ecx

    mov         edx,[ebp].VBuffer
    mov         ebp,[ebp].UBuffer

hloop:
wloop:
    movd        mm2,[edx+ebx]               ;get the v's
;nop
;nop
;-

    movd        mm1,[ebp+ebx]               ;get the u's
;nop
;nop
;-

    movq        mm0,[esi+ebx*2]             ;get the y's
;nop    
    punpcklbw   mm1,mm2                     ;v3 u3 v2 u2 v1 u1 v0 u0

    movq        mm3,mm0                     ;save upper y's
    punpcklbw   mm0,mm1                     ;v1 y3 u1 y2 v0 y1 u0 y0

    punpckhbw   mm3,mm1                     ;v3 y7 u3 y6 v2 y5 u2 y4
    dec         ecx

    movq        [edi+ebx*4],mm0             ;write first 4 pixels
;-

    movq        8[edi+ebx*4],mm3
;-

    lea         ebx,[ebx+4]
    jg          wloop

    mov         ecx,eax                     ;get current line number
    mov         ebx,[esp+LOCAL_SPACE].scrnPitch

    shl         ecx,31                      ;save low bit
    add         edi,ebx

    sar         ecx,31                      ;generate mask
    mov         ebx,L_UVStride[esp]

    and         ebx,ecx
    mov         ecx,L_blkWidth[esp]

if 0
    add         ebp,ebx
    add         edx,ebx
else
    sub         ebp,ebx
    sub         edx,ebx
endif

if 0
    add         esi,L_YStride[esp]
else
    sub         esi,DWORD PTR L_YStride[esp]
endif
    xor         ebx,ebx

    dec         eax
    jg          hloop

;------------------------------------------------

theExit:
    add         esp,LOCAL_SPACE
nop

    emms

    pop     edx
    pop     ecx
    pop     ebx
    pop     ebp
    pop     edi
    pop     esi

    ret

;************************************************
        END