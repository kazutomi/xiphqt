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
; yv12 to rgb32 same blitter
;\***********************************************/ 
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
		.MMX
 
        .CODE

NAME x86bct00

PUBLIC bct00_MMX_
PUBLIC _bct00_MMX
 

INCLUDE wilk.ash
INCLUDE wblit.ash

;------------------------------------------------
; local vars
L_blkWidth      EQU 0
L_YStride       EQU L_blkWidth+4
L_UVStride      EQU L_YStride+4
L_height        EQU L_UVStride+4

LOCAL_SPACE     EQU L_height+4

;------------------------------------------------
;void bct00_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
;
bct00_MMX_:
_bct00_MMX:
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

    mov         L_YStride[esp],eax
    mov         L_UVStride[esp],ecx

    mov         eax,[ebp].YHeight
    mov         ecx,[ebp].YWidth

    mov         L_height[esp],eax
nop

    shr         ecx,1                           ;blocks of 2 pixels
    mov         esi,[ebp].YBuffer

    xor         ebx,ebx
    mov         L_blkWidth[esp],ecx

    mov         edx,[ebp].VBuffer
    mov         ebp,[ebp].UBuffer

hloop:
    xor         eax,eax
    xor         ecx,ecx

wloop:
    mov         cl,BYTE PTR 0[edx+ebx]          ;get v
nop

    mov         al,BYTE PTR 0[esi+ebx*2]        ;get y0
nop

    movd        mm0,_WK_VforRG_MMX[ecx*4]      ;0 0 CrforR CrforG
;-

    movd        mm2,_WK_YforY_MMX[eax*4]
    psllq       mm0,16                          ;0 CrforR CrforG 0

    mov         cl,BYTE PTR 0[ebp+ebx]          ;get u
nop

    mov         al,BYTE PTR 1[esi+ebx*2]        ;get y1
nop

    movd        mm1,_WK_UforBG_MMX[ecx*4]      ;0 0 CbforG CbforB
    punpcklwd   mm2,mm2                         ;y0 y0 y0 y0

    movd        mm3,_WK_YforY_MMX[eax*4]       ;get y1
    paddsw      mm0,mm1                         ;chromas

    punpcklwd   mm3,mm3                         ;y1 y1 y1 y1
    add         ebx,1

    paddsw      mm2,mm0                         ;x r g b
    paddsw      mm3,mm0                         ;x r1 g1 b1

    packuswb    mm2,mm3                         ;combine both pixels
;-

;-
;-

    movq        QWORD PTR -8[edi+ebx*8],mm2
;-

    cmp         ebx,DWORD PTR L_blkWidth[esp]
    jne         wloop

    mov         ecx,L_height[esp]
nop

    shl         ecx,31                      ;save low bit
    add         edi,[esp+LOCAL_SPACE].scrnPitch

    sar         ecx,31                      ;generate mask
    mov         ebx,L_UVStride[esp]

    and         ebx,ecx
nop

    sub         ebp,ebx
    sub         edx,ebx

    sub         esi,DWORD PTR L_YStride[esp]
    xor         ebx,ebx

    dec         DWORD PTR L_height[esp]
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

