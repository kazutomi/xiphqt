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


;
; **-CC_YVYUtoYV12_MMX
; 
; This function will convert a YVYU (a.k.a. YUV4:2:2) buffer to a YV12 format.
;
; See C version of function for description of how this function behaves.
; The C version of the function is called CC_YVYUtoYV12 and can be found
; in colorconversions.c
;
; The only significant algorithmic difference from the C version is that this
; (the assembly version) assumes that the image width is a multiple of 8.
; The function will not check to see that the image width is a multiple of 8.
; If the width of the image passed in is not a multiple of 8 then this function
; will generate incorrect data.  It may also cause a protected memory area
; access violation.
;
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
        .MMX

        .DATA
TORQ_CX_DATA SEGMENT PAGE PUBLIC USE32 'DATA' 

        ALIGN 32
    YMask       QWORD   000ff00ff00ff00ffh                  ; mask used to mask off all values but Y
    UVMask      QWORD   0ff00ff00ff00ff00h                  ; mask used to mask off all values but U, V
    UMask       QWORD   000ff000000ff0000h                  ; mask used to mask off all values but U
    VMask       QWORD   0000000ff000000ffh                  ; mask used to mask off all values but V

        .CODE

NAME x86ColorConversions

PUBLIC CC_YVYUtoYV12_MMX_
PUBLIC _CC_YVYUtoYV12_MMX
 

INCLUDE colorconversions.ash

;------------------------------------------------
; local vars
WidthCtrInit    EQU 0
LOCAL_SPACE     EQU WidthCtrInit+4


;------------------------------------------------
;void CC_YVYUtoYV12( unsigned char *YVYUBuffer, int ImageWidth, int ImageHeight,
;                unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
;
CC_YVYUtoYV12_MMX_:
_CC_YVYUtoYV12_MMX:
    push    esi
    push    edi
    push    ebp
    push    ebx 
    push    ecx
    push    edx

;
; ESP = Stack Pointer                      MM0 = Free
; ESI = Free                               MM1 = Free
; EDI = Free                               MM2 = Free
; EBP = Free                               MM3 = Free
; EBX = Free                               MM4 = Free
; ECX = Free                               MM5 = Free
; EDX = Free                               MM6 = Free
; EAX = Free                               MM7 = Free
;

    mov     esi,(YVYUConvParams PTR [esp]).YVYUBuffer       ; input buffer ptr
    mov     edi,(YVYUConvParams PTR [esp]).ImageWidth
    mov     ebp,(YVYUConvParams PTR [esp]).ImageHeight
    mov     ebx,(YVYUConvParams PTR [esp]).YBuffer
    mov     ecx,(YVYUConvParams PTR [esp]).UBuffer
    mov     edx,(YVYUConvParams PTR [esp]).VBuffer

    sub     esp,LOCAL_SPACE                                 ; allocate space for local variables

    mov     eax,edi                                         
    shr     ebp,1                                           ; ImageHeight = ImageHeight / 2
                                                            ; needed because we process two image lines at a time
    shr     eax,4                                           ; ImageWidth = ImageWidth / 16
                                                            ; needed because we process 8 pixels across per loop
    mov     WidthCtrInit[esp],eax                           ; store calculated result
                                                            ; used to index to next input buffer line
    mov     eax,edi

;
; ESP = Stack Pointer                      MM0 = Free
; ESI = Input Buffer Ptr                   MM1 = Free
; EDI = ImageWidth                         MM2 = Scratch
; EBP = Height Ctr                         MM3 = Scratch
; EBX = YBuffer Ptr                        MM4 = Free
; ECX = UBuffer Ptr                        MM5 = Free
; EDX = VBuffer Ptr                        MM6 = Free
; EAX = Width Ctr                          MM7 = Free
;

HeightLoopStart:
    mov         eax,WidthCtrInit[esp]                           ; Initilize width loop ctr

WidthLoopStart:
    movq        mm0,QWORD PTR  [esi]                            ; load first 4 pixels on first image line
                                                                ; MM0 = u(0,1),y(0,3),v(0,1),y(0,2),u(0,0),y(0,1),v(0,0),y(0,0)
    movq        mm1,QWORD PTR 8[esi]                            ; load 2nd 4 pixels on first image line
                                                                ; MM1 = u(0,3),y(0,7),v(0,3),y(0,6),u(0,2),y(0,5),v(0,2),y(0,4)
    movq        mm2,mm0                                         ; copy Y's to prepare for mask
    movq        mm3,mm1                                         ; copy Y's to prepare for mask
    pand        mm2,YMask                                       ; mask off U, V
                                                                ; MM2 = 00,y(0,3),00,y(0,2),00,y(0,1),00,y(0,0)
    pand        mm3,YMask                                       ; mask off U, V
                                                                ; MM3 = 00,y(0,7),00,y(0,6),00,y(0,5),00,y(0,4)
    pand        mm0,UVMask                                      ; mask off Y values
                                                                ; MM0 = u(0,1),00,v(0,1),00,u(0,1),00,v(0,0),00
    pand        mm1,UVMask                                      ; mask off Y values
                                                                ; MM1 = u(0,3),00,v(0,3),00,u(0,2),00,v(0,2),00
    packuswb    mm2,mm3                                         ; pack Y values to prepare to write out to memory
    psrlw       mm0,8                                           ; shift u, v values down
    psrlw       mm1,8                                           ; shift u, v values down
    movq        QWORD PTR [ebx],mm2                             ; write Y values out
    
;
; ESP = Stack Pointer                      MM0 = 00,u(0,1),00,v(0,1),00,u(0,0),00,v(0,0)
; ESI = Input Buffer Ptr                   MM1 = 00,u(0,3),00,v(0,3),00,u(0,2),00,v(0,2)
; EDI = ImageWidth                         MM2 = Free
; EBP = Height Ctr                         MM3 = Free
; EBX = YBuffer Ptr                        MM4 = Free
; ECX = UBuffer Ptr                        MM5 = Free
; EDX = VBuffer Ptr                        MM6 = Free
; EAX = Width Ctr                          MM7 = Free
;

    movq        mm2,QWORD PTR  [esi+edi*2]                      ; load first 4 pixels on 2nd image line
                                                                ; MM2 = u(1,1),y(1,3),v(1,1),y(1,2),u(1,0),y(1,1),v(1,0),y(1,0)
    movq        mm3,QWORD PTR 8[esi+edi*2]                      ; load 2nd 4 pixels on 2nd image line
                                                                ; MM3 = u(1,3),y(1,7),v(1,3),y(1,6),u(1,2),y(1,5),v(1,2),y(1,4)
    movq        mm4,mm2                                         ; copy Y's to prepare for mask
    movq        mm5,mm3                                         ; copy Y's to prepare for mask
    pand        mm4,YMask                                       ; mask off U, V
                                                                ; MM4 = 00,y(1,3),00,y(1,2),00,y(1,1),00,y(1,0)
    pand        mm5,YMask                                       ; mask off U, V
                                                                ; MM5 = 00,y(1,7),00,y(1,6),00,y(1,5),00,y(1,4)
    pand        mm2,UVMask                                      ; mask off Y values
                                                                ; MM2 = u(1,1),00,v(1,1),00,u(1,1),00,v(1,0),00
    pand        mm3,UVMask                                      ; mask off Y values
                                                                ; MM3 = u(1,3),00,v(1,3),00,u(1,2),00,v(1,2),00
    packuswb    mm4,mm5                                         ; pack Y values to prepare to write out to memory
    psrlw       mm2,8                                           ; shift u, v values down
    psrlw       mm3,8                                           ; shift u, v values down
    movq        QWORD PTR [ebx+edi],mm4                         ; write Y values out
    paddw       mm0,mm2                                         ; sum u,v values to prepare for average
    paddw       mm1,mm3                                         ; sum u,v values to prepare for average

;
; ESP = Stack Pointer                      MM0 = u(0,1)+u(1,1),v(0,1)+v(1,1),u(0,0)+u(1,0),v(0,0)+v(1,0)
; ESI = Input Buffer Ptr                   MM1 = u(0,3)+u(1,3),v(0,3)+v(1,3),u(0,2)+u(1,2),v(0,2)+v(1,2)
; EDI = ImageWidth                         MM2 = Free
; EBP = Height Ctr                         MM3 = Free
; EBX = YBuffer Ptr                        MM4 = Free
; ECX = UBuffer Ptr                        MM5 = Free
; EDX = VBuffer Ptr                        MM6 = Free
; EAX = Width Ctr                          MM7 = Free
;

    movq        mm2,QWORD PTR 16[esi]                           ; load 3rd 4 pixels on first image line
                                                                ; MM2 = u(0,5),y(0,11),v(0,5),y(0,10),u(0,4),y(0,9),v(0,4),y(0,8)
    movq        mm3,QWORD PTR 24[esi]                           ; load 4th 4 pixels on first image line
                                                                ; MM3 = u(0,7),y(0,15),v(0,7),y(0,14),u(0,6),y(0,13),v(0,6),y(0,12)
    movq        mm4,mm2                                         ; copy Y's to prepare for mask
    movq        mm5,mm3                                         ; copy Y's to prepare for mask
    pand        mm4,YMask                                       ; mask off U, V
                                                                ; MM4 = 00,y(0,11),00,y(0,10),00,y(0,9),00,y(0,8)
    pand        mm5,YMask                                       ; mask off U, V
                                                                ; MM5 = 00,y(0,15),00,y(0,14),00,y(0,13),00,y(0,12)
    pand        mm2,UVMask                                      ; mask off Y values
                                                                ; MM2 = u(0,5),00,v(0,5),00,u(0,4),00,v(0,4),00
    pand        mm3,UVMask                                      ; mask off Y values
                                                                ; MM3 = u(0,7),00,v(0,7),00,u(0,6),00,v(0,6),00
    packuswb    mm4,mm5                                         ; pack Y values to prepare to write out to memory
    psrlw       mm2,8                                           ; shift u, v values down
    psrlw       mm3,8                                           ; shift u, v values down
    movq        QWORD PTR 8[ebx],mm4                            ; write Y values out

;
; ESP = Stack Pointer                      MM0 = u(0,1)+u(1,1),v(0,1)+v(1,1),u(0,0)+u(1,0),v(0,0)+v(1,0)
; ESI = Input Buffer Ptr                   MM1 = u(0,3)+u(1,3),v(0,3)+v(1,3),u(0,2)+u(1,2),v(0,2)+v(1,2)
; EDI = ImageWidth                         MM2 = 00,u(0,5),00,v(0,5),00,u(0,4),00,v(0,4)
; EBP = Height Ctr                         MM3 = 00,u(0,7),00,v(0,7),00,u(0,6),00,v(0,6)
; EBX = YBuffer Ptr                        MM4 = Free
; ECX = UBuffer Ptr                        MM5 = Free
; EDX = VBuffer Ptr                        MM6 = Free
; EAX = Width Ctr                          MM7 = Free
;

    movq        mm4,QWORD PTR 16[esi+edi*2]                     ; load 3rd 4 pixels on 2nd image line
                                                                ; MM2 = u(1,5),y(1,11),v(1,5),y(1,10),u(1,4),y(1,9),v(1,4),y(1,8)
    movq        mm5,QWORD PTR 24[esi+edi*2]                     ; load 4th 4 pixels on 2nd image line
                                                                ; MM3 = u(1,7),y(1,15),v(1,7),y(1,14),u(1,6),y(1,13),v(1,6),y(1,12)
    movq        mm6,mm4                                         ; copy Y's to prepare for mask
    movq        mm7,mm5                                         ; copy Y's to prepare for mask
    pand        mm6,YMask                                       ; mask off U, V
                                                                ; MM4 = 00,y(1,11),00,y(1,10),00,y(1,9),00,y(1,8)
    pand        mm7,YMask                                       ; mask off U, V
                                                                ; MM5 = 00,y(1,15),00,y(1,14),00,y(1,13),00,y(1,12)
    pand        mm4,UVMask                                      ; mask off Y values
                                                                ; MM2 = u(1,5),00,v(1,5),00,u(1,4),00,v(1,4),00
    pand        mm5,UVMask                                      ; mask off Y values
                                                                ; MM3 = u(1,7),00,v(1,7),00,u(1,6),00,v(1,6),00
    packuswb    mm6,mm7                                         ; pack Y values to prepare to write out to memory
    psrlw       mm4,8                                           ; shift u, v values down
    psrlw       mm5,8                                           ; shift u, v values down
    movq        QWORD PTR 8[ebx+edi],mm6                        ; write Y values out

    paddw       mm2,mm4                                         ; sum u, v values
    paddw       mm3,mm5                                         ; sum u, v values

    psrlw       mm0,1                                           ; divide summed u, v values by 2 to get average
    psrlw       mm1,1                                           ; divide summed u, v values by 2 to get average
    psrlw       mm2,1                                           ; divide summed u, v values by 2 to get average
    psrlw       mm3,1                                           ; divide summed u, v values by 2 to get average

;
; ESP = Stack Pointer                      MM0 = 00,avg(u(0,1)+u(1,1)),00,avg(v(0,1)+v(1,1)),00,avg(u(0,0)+u(1,0)),00,avg(v(0,0)+v(1,0))
; ESI = Input Buffer Ptr                   MM1 = 00,avg(u(0,3)+u(1,3)),00,avg(v(0,3)+v(1,3)),00,avg(u(0,2)+u(1,2)),00,avg(v(0,2)+v(1,2))
; EDI = ImageWidth                         MM2 = 00,avg(u(0,5)+u(1,5)),00,avg(v(0,5)+v(1,5)),00,avg(u(0,4)+u(1,4)),00,avg(v(0,4)+v(1,4))
; EBP = Height Ctr                         MM3 = 00,avg(u(0,7)+u(1,7)),00,avg(v(0,7)+v(1,7)),00,avg(u(0,6)+u(1,6)),00,avg(v(0,6)+v(1,6))
; EBX = YBuffer Ptr                        MM4 = Free
; ECX = UBuffer Ptr                        MM5 = Free
; EDX = VBuffer Ptr                        MM6 = Free
; EAX = Width Ctr                          MM7 = Free
;
    movq        mm4,mm0                                         ; create copy to prepare to write out
    movq        mm5,mm1
    movq        mm6,mm2
    movq        mm7,mm3

;
; ESP = Stack Pointer                      MM0 = 00,avg(u(0,1)+u(1,1)),00,avg(v(0,1)+v(1,1)),00,avg(u(0,0)+u(1,0)),00,avg(v(0,0)+v(1,0))
; ESI = Input Buffer Ptr                   MM1 = 00,avg(u(0,3)+u(1,3)),00,avg(v(0,3)+v(1,3)),00,avg(u(0,2)+u(1,2)),00,avg(v(0,2)+v(1,2))
; EDI = ImageWidth                         MM2 = 00,avg(u(0,5)+u(1,5)),00,avg(v(0,5)+v(1,5)),00,avg(u(0,4)+u(1,4)),00,avg(v(0,4)+v(1,4))
; EBP = Height Ctr                         MM3 = 00,avg(u(0,7)+u(1,7)),00,avg(v(0,7)+v(1,7)),00,avg(u(0,6)+u(1,6)),00,avg(v(0,6)+v(1,6))
; EBX = YBuffer Ptr                        MM4 = 00,avg(u(0,1)+u(1,1)),00,avg(v(0,1)+v(1,1)),00,avg(u(0,0)+u(1,0)),00,avg(v(0,0)+v(1,0))
; ECX = UBuffer Ptr                        MM5 = 00,avg(u(0,3)+u(1,3)),00,avg(v(0,3)+v(1,3)),00,avg(u(0,2)+u(1,2)),00,avg(v(0,2)+v(1,2))
; EDX = VBuffer Ptr                        MM6 = 00,avg(u(0,5)+u(1,5)),00,avg(v(0,5)+v(1,5)),00,avg(u(0,4)+u(1,4)),00,avg(v(0,4)+v(1,4))
; EAX = Width Ctr                          MM7 = 00,avg(u(0,7)+u(1,7)),00,avg(v(0,7)+v(1,7)),00,avg(u(0,6)+u(1,6)),00,avg(v(0,6)+v(1,6))
;

    pand        mm0,UMask                                   ; mask off all values but u
    pand        mm1,UMask
    pand        mm2,UMask
    pand        mm3,UMask

    pand        mm4,VMask                                   ; mask off all values but v
    pand        mm5,VMask
    pand        mm6,VMask
    pand        mm7,VMask

    psrld       mm0,16                                      ; shift u values down to proper position
    psrld       mm1,16                                      ; shift u values down to proper position
    psrld       mm2,16                                      ; shift u values down to proper position
    psrld       mm3,16                                      ; shift u values down to proper position

;
; ESP = Stack Pointer                      MM0 = 00,00,00,avg(u(0,1)+u(1,1)),00,00,00,avg(u(0,0)+u(1,0))
; ESI = Input Buffer Ptr                   MM1 = 00,00,00,avg(u(0,3)+u(1,3)),00,00,00,avg(u(0,2)+u(1,2))
; EDI = ImageWidth                         MM2 = 00,00,00,avg(u(0,5)+u(1,5)),00,00,00,avg(u(0,4)+u(1,4))
; EBP = Height Ctr                         MM3 = 00,00,00,avg(u(0,7)+u(1,7)),00,00,00,avg(u(0,6)+u(1,6))
; EBX = YBuffer Ptr                        MM4 = 00,00,00,avg(v(0,1)+v(1,1)),00,00,00,avg(v(0,0)+v(1,0))
; ECX = UBuffer Ptr                        MM5 = 00,00,00,avg(v(0,3)+v(1,3)),00,00,00,avg(v(0,2)+v(1,2))
; EDX = VBuffer Ptr                        MM6 = 00,00,00,avg(v(0,5)+v(1,5)),00,00,00,avg(v(0,4)+v(1,4))
; EAX = Width Ctr                          MM7 = 00,00,00,avg(v(0,7)+v(1,7)),00,00,00,avg(v(0,6)+v(1,6))
;

;
; convert from double words to words
;
    packssdw    mm0,mm1
    packssdw    mm2,mm3
    packssdw    mm4,mm5
    packssdw    mm6,mm7

;
; ESP = Stack Pointer                      MM0 = 00,avg(u(0,3)+u(1,3)),00,avg(u(0,2)+u(1,2)),00,avg(u(0,1)+u(1,1)),00avg(u(0,0)+u(1,0))
; ESI = Input Buffer Ptr                   MM1 = Free
; EDI = ImageWidth                         MM2 = 00,avg(u(0,7)+u(1,7)),00,avg(u(0,6)+u(1,6)),00,avg(u(0,5)+u(1,5)),00,avg(u(0,4)+u(1,4))
; EBP = Height Ctr                         MM3 = Free
; EBX = YBuffer Ptr                        MM4 = 00,avg(v(0,3)+v(1,3)),00,avg(v(0,2)+v(1,2)),00,avg(v(0,1)+v(1,1)),00,avg(v(0,0)+v(1,0))
; ECX = UBuffer Ptr                        MM5 = Free
; EDX = VBuffer Ptr                        MM6 = 00,avg(v(0,7)+v(1,7)),00,avg(v(0,6)+v(1,6)),00,avg(v(0,5)+v(1,5)),00,avg(v(0,4)+v(1,4))
; EAX = Width Ctr                          MM7 = Free
;

;
; convert from words to bytes
;
    packuswb    mm0,mm2
    packuswb    mm4,mm6

;
; ESP = Stack Pointer                      MM0 = avg(u(0,7)+u(1,7)),avg(u(0,6)+u(1,6)),avg(u(0,5)+u(1,5)),avg(u(0,4)+u(1,4)),avg(u(0,3)+u(1,3)),avg(u(0,2)+u(1,2)),avg(u(0,1)+u(1,1)),avg(u(0,0)+u(1,0))
; ESI = Input Buffer Ptr                   MM1 = Free
; EDI = ImageWidth                         MM2 = Free
; EBP = Height Ctr                         MM3 = Free
; EBX = YBuffer Ptr                        MM4 = avg(v(0,7)+v(1,7)),avg(v(0,6)+v(1,6)),avg(v(0,5)+v(1,5)),avg(v(0,4)+v(1,4)),avg(v(0,3)+v(1,3)),avg(v(0,2)+v(1,2)),avg(v(0,1)+v(1,1)),avg(v(0,0)+v(1,0))
; ECX = UBuffer Ptr                        MM5 = Free
; EDX = VBuffer Ptr                        MM6 = Free
; EAX = Width Ctr                          MM7 = Free
;

    movq        QWORD PTR [ecx],mm0                             ; write U values out
    movq        QWORD PTR [edx],mm4                             ; write V values out

    add         ebx,16                                          ; increment Y pointer
    add         esi,32                                          ; increment input buffer ptr
    add         ecx,8                                           ; increment u buffer ptr
    add         edx,8                                           ; increment v buffer ptr

    dec         eax                                             ; --WidthLoopCtr
    jne         WidthLoopStart                                  ; are we done yet?
WidthLoopEnd:

    add         esi,edi                                         ; need to increment input buffer ptr by 2 * ImageWidth
                                                                ; do that by using two seperate adds
    add         ebx,edi                                         ; step over line already processed
    add         esi,edi

    dec         ebp                                             ; --HeightCtr
    jne         HeightLoopStart                                 ; are we done yet?
HeightLoopEnd:


theExit:
    add         esp,LOCAL_SPACE    

    emms

    pop         edx
    pop         ecx
    pop         ebx
    pop         ebp
    pop         edi
    pop         esi

    ret

;************************************************
        END

