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

;
; **-CC_RGB32toYV12_XMM
; 
; This function will convert a RGB32 buffer to planer YV12 output.
; Alpha is ignored.
;
; See C version of function for description of how this function behaves.
; The C version of the function is called CC_RGB32toYV12 and can be found
; in colorconversions.c
;
; This function will only run on Pentium III processors
;
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
        .MMX

        .DATA
TORQ_CX_DATA SEGMENT PAGE PUBLIC USE32 'DATA' 

        ALIGN 32

    Zeros           QWORD   00000000000000000h      ; All Zeros for conversion from 8-bits to 16-bits
    YGYBZRYR        QWORD   040830C8B000020E5h      ; 0.504 * ScaleFactor, 0.098 * ScaleFactor, Zero, 0.257 * ScaleFactor
    ZRYRYGYB        QWORD   0000020E540830C8Bh      ; Zeros, 0.257 * ScaleFactor, 0.504 * ScaleFactor, 0.098 * ScaleFactor
    YARYAR          QWORD   00008400000084000h      ; (16 * ScaleFactor) + (ScaleFactor / 2), (16 * ScaleFactor) +  (ScaleFactor / 2)
    AvgRoundFact    QWORD   00002000200020002h      ; Round factor used for dividing by 2
    VGVBUGUB        QWORD   0D0E6F6EADAC13831h      ; -0.368 * ScaleFactor, -0.071 * ScaleFactor, -0.291 * ScaleFactor, 0.439 * ScaleFactor
    ZRVRZRUR        QWORD   0000038310000ED0Fh      ; Zero, 0.439 * ScaleFactor, Zero, -0.148 * ScaleFactor
    VARUAR          QWORD   00040400000404000h      ; (128 * ScaleFactor) + (ScaleFactor / 2), (128 * ScaleFactor) + (ScaleFactor / 2)

        .CODE

NAME x86ColorConversions

PUBLIC CC_RGB32toYV12_XMM_
PUBLIC _CC_RGB32toYV12_XMM
 

INCLUDE colorconversions.ash

;------------------------------------------------
; local vars
LOCAL_SPACE     EQU 0


;------------------------------------------------
; int CC_RGB32toYV12( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
;                     unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
;
CC_RGB32toYV12_XMM_:
_CC_RGB32toYV12_XMM:
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

    mov         esi,(CConvParams PTR [esp]).ImageHeight             ; Load Image height
    shr         esi,1                                               ; divide by 2, we process image two lines at a time
    mov         (CConvParams PTR [esp]).ImageHeight,esi             ; Load Image height
    mov         edi,(CConvParams PTR [esp]).RGBABuffer              ; Input Buffer Ptr
    mov         esi,(CConvParams PTR [esp]).ImageWidth              ; Load Image width
    mov         eax,(CConvParams PTR [esp]).YBuffer                 ; Load Image width
    mov         ecx,(CConvParams PTR [esp]).UBuffer
    mov         edx,(CConvParams PTR [esp]).VBuffer

;
; ESP = Stack Pointer                      MM0 = Free
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Width Ctr                          MM3 = Free
; EBX = Free                               MM4 = Free
; ECX = U output buff ptr                  MM5 = Free
; EDX = V output buff ptr                  MM6 = Free
; EAX = Y output buff ptr                  MM7 = Free
;

;

HLoopStart:
    xor         ebp,ebp                                             ; setup width loop ctr

WLoopStart:
;
; ESP = Stack Pointer                      MM0 = Free
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Width Ctr                          MM3 = Free
; EBX = Scratch                            MM4 = Free
; ECX = U output buff ptr                  MM5 = Free
; EDX = V output buff ptr                  MM6 = Free
; EAX = Y output buff ptr                  MM7 = Used to accumulate sum of R,G,B values so that they can be averaged
;

; process two pixels from first image row
    prefetch0   8[edi]
    movq        mm0,QWORD PTR 0[edi]                                ; A1,R1,G1,B1,A0,R0,G0,B0
    movq        mm7,mm0                                             ; A1,R1,G1,B1,A0,R0,G0,B0

    punpcklbw   mm0,Zeros                                           ; A0,R0,G0,B0
    movq        mm1,mm7                                             ; A1,R1,G1,B1,A0,R0,G0,B0

    punpckhbw   mm7,Zeros                                           ; A1,R1,G1,B1
    movq        mm2,mm1                                             ; A1,R1,G1,B1,A0,R0,G0,B0
    psrlq       mm1,16                                              ; 00,00,A1,R1,G1,B1,A0,R0

    pshufw      mm2,mm2,0Ch                                         ; XX,XX,XX,XX,A1,R1,G0,B0

    punpcklbw   mm1,Zeros                                           ; G1,B1,A0,R0
    paddd       mm7,mm0                                             ; A1+A0,R1+R0,G1+G0,B1+B0

    punpcklbw   mm2,Zeros                                           ; A1,R1,G0,B0
    
    pmaddwd     mm1,YGYBZRYR                                        ; (G1*YG + B1*YB) * 8000h ,(A0*0 + R0*YR) * 8000h
    prefetch0   8[edi+esi]
    movq        mm6,QWORD PTR 0[edi+esi*4]                          ; A1,R1,G1,B1,A0,R0,G0,B0 (load 2nd row)

    pmaddwd     mm2,ZRYRYGYB                                        ; (A1*0 + R1*YR) * 8000h, (G0*YG + B0*YB) * 8000h
    movq        mm5,mm6                                             ; A1,R1,G1,B1,A0,R0,G0,B0 (2nd row)
    movq        mm4,mm5                                             ; A1,R1,G1,B1,A0,R0,G0,B0

    paddd       mm1,mm2                                             ; (R1*YR + G1*YG + B1*YB) * 8000h ,(R0*YR + G0*YG + B0*YB) * 8000h
    movq        mm3,mm5                                             ; A1,R1,G1,B1,A0,R0,G0,B0

    paddd       mm1,YARYAR                                          ; (R1*YR + G1*YG + B1*YB + 16 + 16384) * 8000h ,(R0*YR + G0*YG + B0*YB + 16 + 16384) * 8000h
    psrlq       mm5,16                                              ; 00,00,A1,R1,G1,B1,A0,R0

    psrld       mm1,15                                              ; ((R1*YR + G1*YG + B1*YB + 16 + 16384) * 8000h) / 8000h ,((R0*YR + G0*YG + B0*YB + 16 + 16384) * 8000h) / 8000h

;----------------------------------

    pshufw      mm6,mm6,0Ch                                         ; XX,XX,XX,XX,A1,R1,G0,B0

    punpcklbw   mm5,Zeros                                           ; G1,B1,A0,R0
    punpcklbw   mm6,Zeros                                           ; A1,R1,G0,B0
    
    pmaddwd     mm5,YGYBZRYR                                        ; (G1*YG + B1*YB) * 8000h ,(A0*0 + R0*YR) * 8000h
    punpcklbw   mm4,Zeros                                           ; A0,R0,G0,B0

    pmaddwd     mm6,ZRYRYGYB                                        ; (A1*0 + R1*YR) * 8000h, (G0*YG + B0*YB) * 8000h
    paddd       mm7,mm4                                             ; A0+A1+A0,R0+R1+R0,G0+G1+G0,B0+B1+B0
    punpckhbw   mm3,Zeros                                           ; A1,R1,G1,B1

    paddd       mm5,mm6                                             ; (R1*YR + G1*YG + B1*YB) * 8000h ,(R0*YR + G0*YG + B0*YB) * 8000h
    paddd       mm7,mm3                                             ; A1+A0+A1+A0,R1+R0+R1+R0,R1+G0+G1+G0,B1+B0+B1+B0
    paddd       mm5,YARYAR                                          ; (R1*YR + G1*YG + B1*YB + 16 + 4000h) * 8000h ,(R0*YR + G0*YG + B0*YB + 16 + 4000h) * 8000h
    paddd       mm7,AvgRoundFact                                    ; 2+A1+A0+A1+A0,2+R1+R0+R1+R0,2+R1+G0+G1+G0,2+B1+B0+B1+B0
    psrld       mm5,15                                              ; ((R1*YR + G1*YG + B1*YB + 16 + 4000h) * 8000h) / 8000h ,((R0*YR + G0*YG + B0*YB + 16 + 4000h) * 8000h) / 8000h
    psrlw       mm7,2                                               ; Average of A, R, G, B
    packssdw    mm1,mm5                                             ; Y1 (2nd line), Y0 (2nd line), Y1, YO
    movq        mm0,mm7                                             ; A, R, G, B

    pshufw      mm7,mm7,044h                                        ; G, B, G, B
    pshufw      mm0,mm0,022h                                        ; X, R, X, R

    pmaddwd     mm7,VGVBUGUB                                        ; (G*VG + B*VB) * 8000h , (G*UG + B*UB) * 8000h
    pmaddwd     mm0,ZRVRZRUR                                        ; (X*0 + R*VR) * 8000h, (X*0 + R*UR) * 8000h
    packuswb    mm1,mm6                                             ; X, X, X, X, Y1 (2nd), Y0 (2nd), Y1, Y0

    paddd       mm0,mm7                                             ; (R*VR + G*VG + B*VB) * 8000h , (R*UR + G*UG + B*UB) * 8000h
    movd        ebx,mm1                                             ; Y1 (2nd), Y0 (2nd), Y1, Y0

    paddd       mm0,VARUAR                                          ; (R*VR + G*VG + B*VB + 128 + 4000h) * 8000h , (R*UR + G*UG + B*UB + 128 + 4000h) * 8000h
    mov         WORD PTR 0[eax],bx                                  ; write to output buffer

    psrld       mm0,15                                              ; ((R*VR + G*VG + B*VB + 128 + 4000h) * 8000h) / 8000h , ((R*UR + G*UG + B*UB + 128 + 4000h) * 8000h) / 8000h
    shr         ebx,16                                              ; 00, 00, Y1 (2nd), Y0 (2nd)

; --------------------------
    
    packssdw    mm0,mm6                                             ; X, X, V, U
    mov         WORD PTR 0[eax+esi],bx                              ; write to output buffer

    packuswb    mm0,mm6                                             ; X, X, X, X, X, X, V, U
    add         ebp,2                                               ; increment loop ctr

    movd        ebx,mm0                                             ; X, X, V, U
    add         edi,8                                               ; point to next block of input pixels

    mov         BYTE PTR 0[ecx],bl                                  ; write u to output buffer
    add         eax,2                                               ; increment Y output buffer ptr

    mov         BYTE PTR 0[edx],bh                                  ; write v to output buffer
    inc         ecx                                                 ; point to next U

    inc         edx                                                 ; point to next V
    cmp         ebp,esi                                             ; are we done yet?
    jne         WLoopStart    

WLoopEnd:
    lea         edi,[edi+esi*4]                                     ; Increment input buffer ptr by image width to setp over
                                                                    ; line already processed
    add         eax,esi                                             ; Increment Y output buffer ptr by image widht to step over
                                                                    ; line already processed
    dec         (CConvParams PTR [esp]).ImageHeight                 ; decrement image height ctr
    jne         HLoopStart                                          ; are we done yet?

HLoopEnd:


theExit:

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

