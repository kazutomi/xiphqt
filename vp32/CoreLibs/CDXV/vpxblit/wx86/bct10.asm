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
;??? bct10.asm   
; yv12 to rgb32 stretch blitter
;\***********************************************/ 
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
		.MMX
 
        .CODE

NAME x86bct10

PUBLIC bct10_MMX_
PUBLIC _bct10_MMX
 
INCLUDE wilk.ash
INCLUDE wblit.ash

;------------------------------------------------
; local vars
L_blkWidth      EQU 0
L_YStride       EQU L_blkWidth+4
L_UVStride      EQU L_YStride+4
L_height        EQU L_UVStride+4
L_scrn          EQU L_height+4

LOCAL_SPACE     EQU L_scrn+4

;------------------------------------------------
;void bct10_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
;
bct10_MMX_:
_bct10_MMX:
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
    mov         L_scrn[esp],edi

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
    add         edi,16

    movd        mm1,_WK_UforBG_MMX[ecx*4]          ;0 0 CbforG CbforB
    punpcklwd   mm2,mm2                         ;y0 y0 y0 y0

    movd        mm3,_WK_YforY_MMX[eax*4]           ;get y1
    paddsw      mm0,mm1                         ;chromas

    punpcklwd   mm3,mm3                         ;y1 y1 y1 y1
    paddsw      mm2,mm0                         ;x r g b

    movq        mm4,mm2                         ;copy of pixel 0
    paddsw      mm3,mm0                         ;x r1 g1 b1

    movq        mm5,mm3                         ;copy of pixel 1
    packuswb    mm2,mm4                         ;combine both pixels

    packuswb    mm3,mm5                         ;combine both pixels
    add         ebx,1

    movq        QWORD PTR -16[edi],mm2
;-

    movq        QWORD PTR -8[edi],mm3
;-
    cmp         ebx,DWORD PTR L_blkWidth[esp]
    jne         wloop

    mov         ecx,L_height[esp]
    mov         edi,L_scrn[esp]

    shl         ecx,31                      ;save low bit
    add         edi,[esp+LOCAL_SPACE].scrnPitch

    sar         ecx,31                      ;generate mask
    mov         ebx,L_UVStride[esp]

    and         ebx,ecx
    mov         L_scrn[esp],edi

    sub         ebp,ebx
    sub         edx,ebx

    sub         esi,DWORD PTR L_YStride[esp]
    xor         ebx,ebx

    dec         DWORD PTR L_height[esp]
    jg          hloop

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

