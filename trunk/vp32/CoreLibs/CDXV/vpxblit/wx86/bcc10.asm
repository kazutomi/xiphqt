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
;??? bcc10.asm   
; yv12 black line blitter
;\***********************************************/ 
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
		.MMX
 
        .CODE

NAME x86bcc10

PUBLIC bcc10_MMX_
PUBLIC _bcc10_MMX
 
INCLUDE wilk.ash
INCLUDE wblit.ash

;------------------------------------------------
; local vars
L_blkWidth      EQU 0
L_vPtr          EQU L_blkWidth+4
L_uvStride      EQU L_vPtr+4
LOCAL_SPACE     EQU L_uvStride+4

;------------------------------------------------
;void bcc00_MMX(unsigned long *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 
;
bcc10_MMX_:
_bcc10_MMX:
    push    esi
    push    edi
    push    ebp
    push    ebx 
    push    ecx
    push    edx

    mov         edi,[esp].dst
    mov         ebp,[esp].buffConfig

    mov         eax,[esp].scrnPitch
    sub         esp,LOCAL_SPACE

    mov         ebx,[ebp].YHeight
    mov         ecx,[ebp].YWidth

    mov         L_vPtr[esp],edi
    add         edi,eax                         ;point to bottom of the y area

    shr         ecx,3                           ;blocks of 8 pixels
    mov         edx,[ebp].YStride
    
    mov         esi,[ebp].YBuffer
    mov         L_blkWidth[esp],ecx

    xor         ebp,ebp
nop

y_hloop:
y_wloop:
    movq        mm0,[esi+ebp]
;-
    movq        [edi+ebp],mm0
;-
    add         ebp,8

    dec         ecx
    jg          y_wloop

    sub         esi,edx 
    xor         ebp,ebp

    mov         ecx,L_blkWidth[esp]
    add         edi,eax                         ;add in pitch

    dec         ebx
    jg          y_hloop

;;jmp theExit 

;------------------------------------------------
; do the v's
;------------------------------------------------
    sar         eax,1                           ;uv pitch is half of y pitch
    mov         ebp,[esp+LOCAL_SPACE].buffConfig

    mov         edi,[ebp].uvStart
    mov         ecx,[ebp].UVWidth

    shr         ecx,3                           ;blocks of 8 pixels 
    mov         esi,[ebp].VBuffer

    mov         ebx,[ebp].UVHeight
    mov         edx,[ebp].UVStride

    mov         L_uvStride[esp],edx
    mov         L_blkWidth[esp],ecx

    mov         edx,ecx
    xor         ebp,ebp
 
    shl         edx,3                           ;convert back to bytes

v_hloop:
v_wloop:
    movq        mm0,[esi+ebp]
;-

    movq        [edi+ebp],mm0
;-

    movq        [edi+edx],mm0                   ;make duplicate 
;-

    add         ebp,8
    add         edx,8

    dec         ecx
    jg          v_wloop

    mov         ecx,L_blkWidth[esp]
    add         edi,eax                         ;add in pitch

    mov         edx,ecx
    sub         esi,DWORD PTR L_uvStride[esp]             ;add in stride

    shl         edx,3
    xor         ebp,ebp

    dec         ebx
    jg          v_hloop

;;jmp theExit

;------------------------------------------------
; do the u's
;------------------------------------------------
    mov         ebp,[esp+LOCAL_SPACE].buffConfig
    mov         ecx,L_blkWidth[esp]

    mov         edi,[ebp].uvStart
    mov         ebx,[ebp].UVHeight

    mov         esi,[ebp].UBuffer
nop

    mov         edx,ecx
    add         edi,DWORD PTR [ebp].uvDstArea   ;skip over v's

    shl         edx,3                           ;convert back to byte count
    xor         ebp,ebp

u_hloop:
u_wloop:
    movq        mm0,[esi+ebp]
;-

    movq        [edi+ebp],mm0
;-

    movq        [edi+edx],mm0                   ;make duplicate
;-

    add         ebp,8
    add         edx,8

    dec         ecx
    jg          u_wloop

    mov         ecx,L_blkWidth[esp]
    add         edi,eax                         ;add in pitch

    mov         edx,ecx
    sub         esi,DWORD PTR L_uvStride[esp]             ;add in stride

    shl         edx,3
    xor         ebp,ebp

    dec         ebx
    jg          u_hloop
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
