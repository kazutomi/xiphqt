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
;??? bcs10.asm   
; yv12 to rgb16 same blitter
;\***********************************************/ 
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
		.MMX
 
        .CODE

NAME x86bcs10

PUBLIC bcs10_555_MMX_
PUBLIC _bcs10_555_MMX
 
PUBLIC bcs10_565_MMX_
PUBLIC _bcs10_565_MMX
 

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
;void bcs10_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
;
bcs10_555_MMX_:
_bcs10_555_MMX:
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

    movq        mm7,WK_RB_MASK_555
;-
    movq        mm6,WK_G_MASK_555
;-
    movq        mm5,WK_RGB_MULFACTOR_555
;-

hloop555:
    xor         eax,eax
    xor         ecx,ecx

wloop555:
    mov         cl,BYTE PTR 0[edx+ebx]          ;get v
nop

    mov         al,BYTE PTR 0[esi+ebx*2]        ;get y0
nop

    movd        mm0,_WK_VforRG_MMX[ecx*4]          ;0 0 CrforR CrforG
;-

    movd        mm2,_WK_YforY_MMX[eax*4]           ;Y0
    psllq       mm0,16                          ;0 CrforR CrforG 0

    mov         cl,BYTE PTR 0[ebp+ebx]          ;get u
nop

    mov         al,BYTE PTR 1[esi+ebx*2]        ;get y1
nop

    movd        mm1,_WK_UforBG_MMX[ecx*4]          ;0 0 CbforG CbforB
    punpcklwd   mm2,mm2                         ;Y0 Y0 Y0 Y0

    movd        mm3,_WK_YforY_MMX[eax*4]           ;Y1
    paddsw      mm1,mm0                         ;chromas

    mov         cl,BYTE PTR 1[edx+ebx]          ;get v
    punpcklwd   mm3,mm3                         ;Y1 Y1 Y1 Y1

    mov         al,BYTE PTR 1[ebp+ebx]          ;get u
    paddsw      mm2,mm1                         ;x r0 g0 b0

    movd        mm0,_WK_VforRG_MMX[ecx*4]          ;0 0 CrforR CrforG
    paddsw      mm3,mm1                         ;x r1 g1 b1

    movd        mm1,_WK_UforBG_MMX[eax*4]          ;0 0 CbforG CbforB
    psllq       mm0,16                          ;0 CrforR CrforG 0

    mov         eax,L_height[esp]
    packuswb    mm2,mm2                         ;x r0 g0 b0 x r0 g0 b0

    packuswb    mm3,mm3                         ;x r1 g1 b1 x r1 g1 b1
    and         eax,03h

    shl         eax,1
nop

    paddusb     mm2,QWORD PTR WK_johnsTable_MMX[eax*8]
    paddsw      mm1,mm0                         ;second pair chromas

    paddusb     mm3,QWORD PTR WK_johnsTable_MMX[8+eax*8]
    movq        mm0,mm2

    pand        mm2,mm7                         ;loose lower 3 bits of red and blue
    movq        mm4,mm3

    pand        mm3,mm7                         ;loose lower 3 bits of red and blue
    pmaddwd     mm2,mm5                         ;position red and blue

    pmaddwd     mm3,mm5                         ;position red and blue
    pand        mm0,mm6                         ;loose lower 3 bits of green

    xor         eax,eax
nop

    mov         al,BYTE PTR 2[esi+ebx*2]        ;get y2
    pand        mm4,mm6                         ;loose lower 3 bits of green

    por         mm2,mm0                         
    por         mm3,mm4                         

    mov         cl,BYTE PTR 3[esi+ebx*2]        ;get y3
    psrld       mm2,6                           ;0 0 0 0 R0 G0 B0 0 0 0 0 R0 G0 B0

    movd        mm4,_WK_YforY_MMX[eax*4]           ;Y2
    psrld       mm3,6                           ;0 0 0 0 R1 G1 B1 0 0 0 0 R1 G1 B1

    mov         eax,L_height[esp]
    packssdw    mm2,mm3                         ;combine all 4 pixels

    movd        mm3,_WK_YforY_MMX[ecx*4]           ;Y3
    punpcklwd   mm4,mm4                         ;Y2 Y2 Y2 Y2

    movq        QWORD PTR [edi+ebx*8],mm2
    punpcklwd   mm3,mm3                         ;Y3 Y3 Y3 Y3

    and         eax,03h
    paddsw      mm4,mm1                         ;x r2 g2 b2

    shl         eax,1
    paddsw      mm3,mm1                         ;x r3 g3 b3

    packuswb    mm4,mm4                         ;x r2 g2 b2 x r2 g2 b2
;-

    paddusb     mm4,QWORD PTR WK_johnsTable_MMX[eax*8] 
    packuswb    mm3,mm3                         ;x r3 g3 b3 x r3 g3 b3

    paddusb     mm3,QWORD PTR WK_johnsTable_MMX[8+eax*8] 
    movq        mm1,mm4

    movq        mm0,mm3
    pand        mm4,mm7                         ;loose lower 3 bits of red and blue

    pand        mm3,mm7                         ;loose lower 3 bits of red and blue
    pmaddwd     mm4,mm5                         ;position red and blue

    pmaddwd     mm3,mm5                         ;position red and blue
    pand        mm1,mm6                         ;loose lower 3 bits of green

    pand        mm0,mm6                         ;loose lower 3 bits of green
    por         mm1,mm4                         

    por         mm0,mm3                         
    psrld       mm1,6                           ;0 0 0 0 R2 G2 B2 0 0 0 0 R2 G2 B2

    psrld       mm0,6                           ;0 0 0 0 R3 G3 B3 0 0 0 0 R3 G3 B3
    add         ebx,2

    packssdw    mm1,mm0                         ;combine all 4 pixels
    xor         eax,eax

;+

    movq        QWORD PTR 8-16[edi+ebx*8],mm1
;-

    cmp         ebx,DWORD PTR L_blkWidth[esp]
    jne         wloop555

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
    jg          hloop555

;------------------------------------------------
theExit555:
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

;------------------------------------------------
;void bcs10_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
;
bcs10_565_MMX_:
_bcs10_565_MMX:
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

    movq        mm7,WK_RB_MASK_565
;-
    movq        mm6,WK_G_MASK_565
;-
    movq        mm5,WK_RGB_MULFACTOR_565
;-

hloop565:
    xor         eax,eax
    xor         ecx,ecx

wloop565:
    mov         cl,BYTE PTR 0[edx+ebx]          ;get v
nop

    mov         al,BYTE PTR 0[esi+ebx*2]        ;get y0
nop

    movd        mm0,_WK_VforRG_MMX[ecx*4]          ;0 0 CrforR CrforG
;-

    movd        mm2,_WK_YforY_MMX[eax*4]           ;Y0
    psllq       mm0,16                          ;0 CrforR CrforG 0

    mov         cl,BYTE PTR 0[ebp+ebx]          ;get u
nop

    mov         al,BYTE PTR 1[esi+ebx*2]        ;get y1
nop

    movd        mm1,_WK_UforBG_MMX[ecx*4]          ;0 0 CbforG CbforB
    punpcklwd   mm2,mm2                         ;Y0 Y0 Y0 Y0

    movd        mm3,_WK_YforY_MMX[eax*4]           ;Y1
    paddsw      mm1,mm0                         ;chromas

    mov         cl,BYTE PTR 1[edx+ebx]          ;get v
    punpcklwd   mm3,mm3                         ;Y1 Y1 Y1 Y1

    mov         al,BYTE PTR 1[ebp+ebx]          ;get u
    paddsw      mm2,mm1                         ;x r0 g0 b0

    movd        mm0,_WK_VforRG_MMX[ecx*4]          ;0 0 CrforR CrforG
    paddsw      mm3,mm1                         ;x r1 g1 b1

    movd        mm1,_WK_UforBG_MMX[eax*4]          ;0 0 CbforG CbforB
    psllq       mm0,16                          ;0 CrforR CrforG 0

    mov         eax,L_height[esp]
    packuswb    mm2,mm2                         ;x r0 g0 b0 x r0 g0 b0

    packuswb    mm3,mm3                         ;x r1 g1 b1 x r1 g1 b1
    and         eax,03h

    shl         eax,1
nop

    paddusb     mm2,QWORD PTR WK_johnsTable_MMX[eax*8]
    paddsw      mm1,mm0                         ;second pair chromas

    paddusb     mm3,QWORD PTR WK_johnsTable_MMX[8+eax*8]
    movq        mm0,mm2

    pand        mm2,mm7                         ;loose lower 3 bits of red and blue
    movq        mm4,mm3

    pand        mm3,mm7                         ;loose lower 3 bits of red and blue
    pmaddwd     mm2,mm5                         ;position red and blue

    pmaddwd     mm3,mm5                         ;position red and blue
    pand        mm0,mm6                         ;loose lower 3 bits of green

    xor         eax,eax
;-

    mov         al,BYTE PTR 2[esi+ebx*2]        ;get y2
    pand        mm4,mm6                         ;loose lower 3 bits of green

    por         mm2,mm0                         
    por         mm3,mm4                         

    mov         cl,BYTE PTR 3[esi+ebx*2]        ;get y3
    psrld       mm2,5                           ;0 0 0 0 R0 G0 B0 0 0 0 0 R0 G0 B0

    movd        mm4,_WK_YforY_MMX[eax*4]           ;Y2
    psrld       mm3,5                           ;0 0 0 0 R1 G1 B1 0 0 0 0 R1 G1 B1

    movq        mm0,mm2
    mov         eax,L_height[esp]

    punpckldq   mm2,mm3                         ;0 0 0 0 R1 G1 B1 0 0 0 0 R0 G0 B0
    and         eax,03h

    shl         eax,1
    punpckhdq   mm0,mm3                         ;0 0 0 0 R1 G1 B1 0 0 0 0 R0 G0 B0

    movd        mm3,_WK_YforY_MMX[ecx*4]           ;Y3
    pslld       mm2,16

    punpcklwd   mm4,mm4                         ;Y2 Y2 Y2 Y2
    por         mm2,mm0

    punpcklwd   mm3,mm3                         ;Y3 Y3 Y3 Y3
    paddsw      mm4,mm1                         ;x r2 g2 b2

    movq        QWORD PTR [edi+ebx*8],mm2
    paddsw      mm3,mm1                         ;x r3 g3 b3

    packuswb    mm4,mm4                         ;x r2 g2 b2 x r2 g2 b2
;-

    paddusb     mm4,QWORD PTR WK_johnsTable_MMX[eax*8] 
    packuswb    mm3,mm3                         ;x r3 g3 b3 x r3 g3 b3

    paddusb     mm3,QWORD PTR WK_johnsTable_MMX[8+eax*8] 
    movq        mm1,mm4

    movq        mm0,mm3
    pand        mm4,mm7                         ;loose lower 3 bits of red and blue

    pand        mm3,mm7                         ;loose lower 3 bits of red and blue
    pmaddwd     mm4,mm5                         ;position red and blue

    pmaddwd     mm3,mm5                         ;position red and blue
    pand        mm1,mm6                         ;loose lower 3 bits of green

    pand        mm0,mm6                         ;loose lower 3 bits of green
    por         mm1,mm4                         

    por         mm0,mm3                         
    xor         eax,eax

    psrld       mm1,5                           ;0 0 0 0 R2 G2 B2 0 0 0 0 R2 G2 B2
;-

    psrld       mm0,5                           ;0 0 0 0 R3 G3 B3 0 0 0 0 R3 G3 B3
    movq        mm4,mm1

    punpckldq   mm1,mm0                         ;0 0 0 0 R3 G3 B3 0 0 0 0 R2 G2 B2
    add         ebx,2

    punpckhdq   mm4,mm1                         ;0 0 0 0 R3 G3 B3 0 0 0 0 R2 G2 B2
;-
    
    pslld       mm4,16
;-

    por         mm4,mm1
;-

;+

    movq        QWORD PTR 8-16[edi+ebx*8],mm4
;-

    cmp         ebx,DWORD PTR L_blkWidth[esp]
    jne         wloop565

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
    jg          hloop565

;------------------------------------------------
theExit565:
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

