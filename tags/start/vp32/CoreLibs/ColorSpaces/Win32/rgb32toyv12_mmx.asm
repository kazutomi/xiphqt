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
; **-CC_RGB32toYV12_MMX
; 
; This function will convert a RGB32 buffer to planer YV12 output.
; Alpha is ignored.
;
; See C version of function for description of how this function behaves.
; The C version of the function is called CC_RGB32toYV12 and can be found
; in colorconversions.c
;
; Although this function will run on Pentium processors with MMX built it,
; performance may not be as good as anticipated.  This function was written
; for the Pentium II processor.  As such no attempt was made to pair the 
; instructions properly for the Pentium processor.
;
;
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
        .MMX

        .DATA
TORQ_CX_DATA SEGMENT PAGE PUBLIC USE32 'DATA' 

        ALIGN 32

    Zeros           QWORD   00000000000000000h      ; All Zeros for conversion from 8-bits to 16-bits
    ZRZRYGYB        QWORD   00000000040830C8Bh      ; Zero, Zero, 0.504 * ScaleFactor, 0.098 * ScaleFactor
    AvgRoundFact    QWORD   00002000200020002h      ; Round factor used for dividing by 2
    YGYBZRYR        QWORD   040830C8B000020E5h      ; 0.504 * ScaleFactor, 0.098 * ScaleFactor, Zero, 0.257 * ScaleFactor
    YAddRoundFact   QWORD   00008400000084000h      ; (16 * ScaleFactor) + (ScaleFactor / 2), (16 * ScaleFactor) + (ScaleFactor / 2)
    ZRYRZRZR        QWORD   0000020E500000000h      ; Zero, 0.257 * ScaleFactor, Zero, Zero
    ZRVRUGUB        QWORD   000003831DAC13831h      ; Zero, 0.439 * ScaleFactor, -0.291 * ScaleFactor, 0.439 * ScaleFactor
    VGVBZRZR        QWORD   0D0E6F6EA00000000h      ; -0.368 * ScaleFactor, -0.071 * ScaleFactor, Zero, Zero
    ZRZRZRUR        QWORD   0000000000000ED0Fh      ; Zero, Zero, Zero, -0.148 * ScaleFactor
    VARUAR          QWORD   00040400000404000h      ; (128 * ScaleFactor) + (ScaleFactor / 2), (128 * ScaleFactor) + (ScaleFactor / 2)

        .CODE

NAME x86ColorConversions

PUBLIC CC_RGB32toYV12_MMX_
PUBLIC _CC_RGB32toYV12_MMX
 

INCLUDE colorconversions.ash

;------------------------------------------------
; local vars
LOCAL_SPACE     EQU 0


;------------------------------------------------
; int CC_RGB32toYV12( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
;                     unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
;
CC_RGB32toYV12_MMX_:
_CC_RGB32toYV12_MMX:
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
; ECX = Free                               MM5 = Free
; EDX = Free                               MM6 = Free
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
; ECX = Free                               MM5 = Free
; EDX = Scratch                            MM6 = Free
; EAX = Y output buff ptr                  MM7 = Used to accumulate sum of R,G,B values so that they can be averaged
;

; process two pixels from first image row
    movq        mm0,QWORD PTR 0[edi]                                ; A1,R1,G1,B1,A0,R0,G0,B0
    movq        mm1,mm0                                             ; A1,R1,G1,B1,A0,R0,G0,B0
    movq        mm7,mm0                                             ; A1,R1,G1,B1,A0,R0,G0,B0
    psrlq       mm1,16                                              ; 00,00,A1,R1,G1,B1,A0,R0
    punpcklbw   mm0,Zeros                                           ; convert to 16-bits
                                                                    ; A0,R0,G0,B0
    movq        mm2,mm7                                             ; A1,R1,G1,B1,A0,R0,G0,B0
    punpckhbw   mm7,Zeros                                           ; unpack and covert to 16-bits
                                                                    ; A1,R1,G1,B1
    paddw       mm7,mm0                                             ; A1+A0,R1+R0,G1+G0,B1+B0
    punpcklbw   mm1,Zeros                                           ; convert to 16-bits
                                                                    ; G1,B1,A0,R0
    punpckhbw   mm2,Zeros                                           ; unpack and convert to 16-bits
                                                                    ; A1,R1,G1,B1
    pmaddwd     mm0,ZRZRYGYB                                        ; A0*0 + R0*0, G0*YG + B0*YB
    movq        mm3,QWORD PTR [edi+esi*4]                           ; load 2nd image line
                                                                    ; A1,R1,G1,B1,A0,R0,G0,B0
    pmaddwd     mm1,YGYBZRYR                                        ; G1*YG + B1*YB, A0*0 + R0*YR
    movq        mm4,mm3                                             ; A1,R1,G1,B1,A0,R0,G0,B0
    movq        mm6,mm3                                             ; A1,R1,G1,B1,A0,R0,G0,B0

    pmaddwd     mm2,ZRYRZRZR                                        ; A*0 + R1*YR, 0*0 + 0*0

    paddd       mm0,YAddRoundFact                                   ; YAddRoundFact, G0*YG + B0*YB + YAddRoundFact
                                                                    ; A1,R1,G1,B1,A0,R0,G0,B0
    paddd       mm1,mm2
    psrlq       mm4,16                                              ; 00,00,A1,R1,G1,B1,A0,R0

	movq		mm5,mm3
    paddd       mm0,mm1                                             ; Y1 * 8000h,Y0 * 8000h

;
; ESP = Stack Pointer                      MM0 = Y1 * 8000h,Y0 * 8000h
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Width Ctr                          MM3 = A1,R1,G1,B1,A0,R0,G0,B0 (2nd Line)
; EBX = Scratch                            MM4 = 00,00,A1,R1,G1,B1,A0,R0 (2nd Line)
; ECX = Free                               MM5 = A1,R1,G1,B1,A0,R0,G0,B0 (2nd Line)
; EDX = Scratch                            MM6 = A1,R1,G1,B1,A0,R0,G0,B0
; EAX = Y output buff ptr                  MM7 = A1+A0,R1+R0,G1+G0,B1+B0
;


    punpcklbw   mm3,Zeros                                           ; convert to 16-bits
                                                                    ; A0,R0,G0,B0
    punpckhbw   mm5,Zeros                                           ; Convert to 16-bits
                                                                    ; A1,R1,G1,B1
    paddw       mm7,mm3                                             ; A0+A1+A0,R0+R1+R0,G0+G1+G0,B0+B1+B0
    psrld       mm0,15                                              ; (Y1 * 8000h) / 8000h, (Y0 * 8000h) / 8000h
                                                                    ; Y1, Y0
    paddw       mm7,mm5                                             ; A1+A0+A1+A0,R1+R0+R1+R0,G1+G0+G1+G0,B1+B0+B1+B0
    punpcklbw   mm4,Zeros                                           ; convert to 16-bits
                                                                    ; G1,B1,A0,R0
    punpckhbw   mm6,Zeros                                           ; convert to 16-bits
                                                                    ; A1,R1,G1,B1
    pmaddwd     mm3,ZRZRYGYB                                        ; A0*0 + R0*0, G0*YG + B0*YB
    paddw       mm7,AvgRoundFact                                    ; 2+A1+A0+A1+A0,2+R1+R0+R1+R0,2+G1+G0+G1+G0,2+B1+B0+B1+B0

    pmaddwd     mm4,YGYBZRYR                                        ; G1*YG + B1*YB, A0*0 + R0*YR
    psrlw       mm7,2                                               ; divide by 4 to get average of 4 RGB values
                                                                    ; A, R, G, B
    paddd       mm3,YAddRoundFact                                   ; 8000h, G0*YG + B0*YB + 8000h

    pmaddwd     mm6,ZRYRZRZR                                        ; A1*0 + R1*YR, 0*0 + 0*0,
    movq        mm1,mm7                                             ; A, R, G, B

    paddd       mm3,mm4                                             ; G1*YG + B1*YB + 8000h, R0*YR + G0*YG + B0*YB + 8000h

    pmaddwd     mm7,ZRVRUGUB                                        ; A*0 + R*VR, G*UG + B*UB
    movq        mm2,mm1                                             ; A, R, G, B

    paddd       mm3,mm6                                             ; R1*YR + G1*YG + B1*YB + 8000h, R0*YR + G0*YG + B0*YB + 8000h
    psllq       mm1,32                                              ; G, B, 0, 0
    psrlq       mm2,32                                              ; 0, 0, A, R

;
; ESP = Stack Pointer                      MM0 = Y1,Y0
; ESI = ImageWidth                         MM1 = G, B, 0, 0
; EDI = Input Buffer Ptr                   MM2 = 0, 0, A, R
; EBP = Width Ctr                          MM3 = Y1 + 8000h, Y0 + 8000h
; EBX = Scratch                            MM4 = Free
; ECX = Free                               MM5 = Free
; EDX = Scratch                            MM6 = Free
; EAX = Y output buff ptr                  MM7 = A*0 + R*VR, G*UG + B*UB
;

    pmaddwd     mm1,VGVBZRZR                                        ; G*VG + B*VB, 0*0 + 0*0
    psrld       mm3,15                                              ; (Y1 * 8000h) / 8000h, (Y0 * 8000h) / 8000h
            
    pmaddwd     mm2,ZRZRZRUR                                        ; 0, A*0 + R*UR
    paddd       mm7,VARUAR                                          ; (R*VR + 128)* 8000h, (G*UG + B*UB + 128) * 8000h


;
; ESP = Stack Pointer                      MM0 = Y1,Y0
; ESI = ImageWidth                         MM1 = G*VG + B*VB, 0*0 + 0*0
; EDI = Input Buffer Ptr                   MM2 = 0, A*0 + R*UR
; EBP = Width Ctr                          MM3 = Y1,Y0 (2nd line)
; EBX = Scratch                            MM4 = Free
; ECX = Free                               MM5 = Free
; EDX = Scratch                            MM6 = Free
; EAX = Y output buff ptr                  MM7 = (R*VR + 128)* 8000h, (G*UG + B*UB + 128) * 8000h
;

    paddd       mm1,mm2                                             ; (G*VG + B*VB) * 8000h, (R*UR) * 8000h
    packssdw    mm0,mm3                                             ; Y1,Y0 (2nd line), Y1,Y0

    paddd       mm7,mm1                                             ; (G*VG + B*VB + R*VR + 128 + 4000h) * 8000h, (G*UG + B*UB + R*UR + 128 + 4000h) * 8000h
    packuswb    mm0,mm4                                             ; XX,XX,XX,XX,Y1,Y0 (2nd line), Y1,Y0

;
; ESP = Stack Pointer                      MM0 = XX,XX,XX,XX,Y1,Y0 (2nd line), Y1,Y0
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Width Ctr                          MM3 = Free
; EBX = Scratch                            MM4 = Free
; ECX = Free                               MM5 = Free
; EDX = Scratch                            MM6 = Free
; EAX = Y output buff ptr                  MM7 = (G*VG + B*VB + R*VR) * 8000h, (G*UG + B*UB + R*UR) * 8000h
;

    psrld       mm7,15                                              ; ((G*VG + B*VB + R*VR) * 8000h) / 8000h, ((G*UG + B*UB + R*UR) * 8000h) / 8000h
    movd        ebx,mm0                                             ; Y1,Y0 (2nd line), Y1,Y0

    packssdw    mm7,mm4                                             ; XX,XX, V, U
    mov         WORD PTR 0[eax],bx                                  ; write Y1, Y2 to memory
    shr         ebx,16                                              ; y's of 2nd line down
                                                                    ; 0,0, Y1, Y2 (2nd line)
    packuswb    mm7,mm4                                             ; XX,XX,XX,XX,XX,XX, V, U
    mov         WORD PTR 0[eax+esi],bx                              ; write Y1, Y2 to memory
    add         ebp,2                                               ; increment loop ctr
            
    movd        ebx,mm7                                             ; XX,XX, V, U
    add         edi,8                                               ; point to next block of input pixels

    mov         BYTE PTR 0[ecx],bl                                  ; write U to memory
    mov         BYTE PTR 0[edx],bh                                  ; write V to memory

    add         eax,2                                               ; increment Y output buffer ptr
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

