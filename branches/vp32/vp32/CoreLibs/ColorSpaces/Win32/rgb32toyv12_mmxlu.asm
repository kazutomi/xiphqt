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
; **-CC_RGB32toYV12_MMXLU
; 
; This function will convert a RGB32 buffer to planer YV12 output.
; Alpha is ignored.
;
; See C version of function for description of how this function behaves.
; The C version of the function is called CC_RGB32toYV12 and can be found
; in colorconversions.c
;
; The color space conversion is done via look up tables not direct calculations
; on the test machine the direct calculations using MMX instructions was around 10%
; faster YMMM.
;
 
        .586
        .387
        .MODEL  flat, SYSCALL, os_dos
        .MMX

        .DATA
TORQ_CX_DATA SEGMENT PAGE PUBLIC USE32 'DATA' 

        ALIGN 32


        .CODE

NAME x86ColorConversions

PUBLIC CC_RGB32toYV12_MMXLU_
PUBLIC _CC_RGB32toYV12_MMXLU
 

INCLUDE colorconversions.ash
INCLUDE lookuptable.ash

;-----------------------------------------------
; Local Constants
Div4Round       EQU 2
Div4            EQU 2
ScaleFactDiv    EQU 15

;-----------------------------------------------

;------------------------------------------------
; local vars
L_WidthIndex    EQU 0
LOCAL_SPACE     EQU L_WidthIndex+4


;------------------------------------------------
; int CC_RGB32toYV12( unsigned char *RGBABuffer, int ImageWidth, int ImageHeight,
;                     unsigned char *YBuffer, unsigned char *UBuffer, unsigned char *VBuffer )
;
CC_RGB32toYV12_MMXLU_:
_CC_RGB32toYV12_MMXLU:
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
    mov         ebp,(CConvParams PTR [esp]).YBuffer                 ; Load Image width

    sub         esp,LOCAL_SPACE                                     ; allocate local variables on stack

;
; ESP = Stack Pointer                      MM0 = Free
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Y output buff ptr                  MM3 = Free
; EBX = Free                               MM4 = Free
; ECX = Free                               MM5 = Free
; EDX = Free                               MM6 = Free
; EAX = Free                               MM7 = Free
;

;

HLoopStart:
    mov         ebx,esi                                             ; initilize our width ctr
    shr         ebx,1                                               ; divide by 2 process two pixels at once
    mov         L_WidthIndex[esp],ebx                               ; store

WLoopStart:
    xor         ebx,ebx
    mov         ecx,Div4Round                                       ; place round factors in registers
    mov         edx,Div4Round                                       ; place round factors in registers
    mov         eax,Div4Round                                       ; place round factors in registers


;
; ESP = Stack Pointer                      MM0 = Will be XX,Y(0,0)
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Y output buff ptr                  MM3 = Scratch
; EBX = Scratch                            MM4 = Free
; ECX = BlueSum                            MM5 = Free
; EDX = GreenSum                           MM6 = Free
; EAX = RedSum                             MM7 = Free
;

    mov         bl,BYTE PTR 0[edi]                                  ; load Blue of first line
    movd        mm0,_YBMult[ebx*4]                                  ; multiply blue
    add         ecx,ebx                                             ; sum blue for avg
                                                                    
    mov         bl,BYTE PTR 1[edi]                                  ; load Green of first line
    movd        mm3,_YGMult[ebx*4]                                  ; multiply green
    add         edx,ebx                                             ; sum green for avg
    paddd       mm0,mm3                                             ; sum y components
                                                                    
    mov         bl,BYTE PTR 2[edi]                                  ; load Red of 1st line
    movd        mm3,_YRMult[ebx*4]                                  ; multiply red
    add         eax,ebx                                             ; sum red for avg
    paddd       mm0,mm3                                             ; sum y components
                                                                    
    psrld       mm0,ScaleFactDiv                                    ; divide y component by scale factor
    movd        ebx,mm0                                             ; prepare to write Y out
    mov         BYTE PTR 0[ebp],bl                                  ; write Y out
    
;
; ESP = Stack Pointer                      MM0 = Will be XX,Y(0,0)
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Y output buff ptr                  MM3 = Scratch
; EBX = Scratch                            MM4 = Free
; ECX = BlueSum                            MM5 = Free
; EDX = GreenSum                           MM6 = Free
; EAX = RedSum                             MM7 = Free
;

    mov         bl,BYTE PTR 0[edi+esi*4]                            ; load Blue of first line
    movd        mm0,_YBMult[ebx*4]                                  ; multiply blue
    add         ecx,ebx                                             ; sum blue for avg
                                                                    
    mov         bl,BYTE PTR 1[edi+esi*4]                            ; load Green of first line
    movd        mm3,_YGMult[ebx*4]                                  ; multiply green
    add         edx,ebx                                             ; sum green for avg
    paddd       mm0,mm3                                             ; sum y components
                                                                    
    mov         bl,BYTE PTR 2[edi+esi*4]                            ; load Red of 1st line
    movd        mm3,_YRMult[ebx*4]                                  ; multiply red
    add         eax,ebx                                             ; sum red for avg
    paddd       mm0,mm3                                             ; sum y components
                                                                    
    psrld       mm0,ScaleFactDiv                                    ; divide y component by scale factor
    movd        ebx,mm0                                             ; prepare to write Y out
    mov         BYTE PTR 0[ebp+esi],bl                              ; write Y out

;
; ESP = Stack Pointer                      MM0 = Will be XX,Y(0,0)
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Y output buff ptr                  MM3 = Scratch
; EBX = Scratch                            MM4 = Free
; ECX = BlueSum                            MM5 = Free
; EDX = GreenSum                           MM6 = Free
; EAX = RedSum                             MM7 = Free
;

    mov         bl,BYTE PTR 4[edi]                                  ; load Blue of first line
    movd        mm0,_YBMult[ebx*4]                                  ; multiply blue
    add         ecx,ebx                                             ; sum blue for avg
                                                                    
    mov         bl,BYTE PTR 5[edi]                                  ; load Green of first line
    movd        mm3,_YGMult[ebx*4]                                  ; multiply green
    add         edx,ebx                                             ; sum green for avg
    paddd       mm0,mm3                                             ; sum y components
                                                                    
    mov         bl,BYTE PTR 6[edi]                                  ; load Red of 1st line
    movd        mm3,_YRMult[ebx*4]                                  ; multiply red
    add         eax,ebx                                             ; sum red for avg
    paddd       mm0,mm3                                             ; sum y components
                                                                    
    psrld       mm0,ScaleFactDiv                                    ; divide y component by scale factor
    movd        ebx,mm0                                             ; prepare to write Y out
    mov         BYTE PTR 1[ebp],bl                                  ; write Y out

;
; ESP = Stack Pointer                      MM0 = Will be XX,Y(0,0)
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Y output buff ptr                  MM3 = Scratch
; EBX = Scratch                            MM4 = Free
; ECX = BlueSum                            MM5 = Free
; EDX = GreenSum                           MM6 = Free
; EAX = RedSum                             MM7 = Free
;

    mov         bl,BYTE PTR 4[edi+esi*4]                            ; load Blue of first line
    movd        mm0,_YBMult[ebx*4]                                  ; multiply blue
    add         ecx,ebx                                             ; sum blue for avg
                                                                    
    mov         bl,BYTE PTR 5[edi+esi*4]                            ; load Green of first line
    movd        mm3,_YGMult[ebx*4]                                  ; multiply green
    add         edx,ebx                                             ; sum green for avg
    paddd       mm0,mm3                                             ; sum y components
                                                                    
    mov         bl,BYTE PTR 6[edi+esi*4]                            ; load Red of 1st line
    movd        mm3,_YRMult[ebx*4]                                  ; multiply red
    add         eax,ebx                                             ; sum red for avg
    paddd       mm0,mm3                                             ; sum y components
                                                                    
    psrld       mm0,ScaleFactDiv                                    ; divide y component by scale factor
    movd        ebx,mm0                                             ; prepare to write Y out
    mov         BYTE PTR 1[ebp+esi],bl                              ; write Y out

;
; ESP = Stack Pointer                      MM0 = Free
; ESI = ImageWidth                         MM1 = Free
; EDI = Input Buffer Ptr                   MM2 = Free
; EBP = Y output buff ptr                  MM3 = Free
; EBX = Scratch                            MM4 = Free
; ECX = BlueSum                            MM5 = Free
; EDX = GreenSum                           MM6 = Free
; EAX = RedSum                             MM7 = Free
;
    
; calculate U, V values
    shr         ecx,Div4                                            ; divide by 4 to get average blues
    shr         edx,Div4                                            ; divide by 4 to get average green
    shr         eax,Div4                                            ; divide by 4 to get average red
                                                                    
    movd        mm1,_UBVRMult[ecx*4]                                ; multiply blue
    movd        mm2,_VBMult[ecx*4]                                  ; multiply blue
                                                                    
    movd        mm4,_UGMult[edx*4]                                  ; multiply green
    movd        mm5,_VGMult[edx*4]                                  ; multiply green
                                                                    
    paddd       mm1,mm4                                             ; sum U components
    paddd       mm2,mm5                                             ; sum V components
                                                                    
    movd        mm4,_URMult[eax*4]                                  ; multiply red
    movd        mm5,_UBVRMult[eax*4]                                ; multiply red

    paddd       mm1,mm4                                             ; sum U components
    paddd       mm2,mm5                                             ; sum V components

    movd        edx,mm1                                             ;
    movd        eax,mm2                                             ; 

    mov         ebx,(CConvParams PTR [esp+LOCAL_SPACE]).UBuffer     ; get U buffer ptr off of stack
    mov         ecx,(CConvParams PTR [esp+LOCAL_SPACE]).VBuffer     ; get V buffer ptr off of stack

    shr         edx,ScaleFactDiv                                    ; divide by scale factor
    shr         eax,ScaleFactDiv                                    ; divide by scale factor

    mov         BYTE PTR 0[ebx],dl                                  ; write U out
    mov         BYTE PTR 0[ecx],al                                  ; write V out

    inc         ebx                                                 ; point to next U
    inc         ecx                                                 ; point to next V

    mov         (CConvParams PTR [esp+LOCAL_SPACE]).UBuffer,ebx     ; save back on stack
    mov         (CConvParams PTR [esp+LOCAL_SPACE]).VBuffer,ecx     ; save back on stack

    add         edi,8                                               ; point to next block of input pixels
    add         ebp,2                                               ; increment Y output buffer ptr

    dec         DWORD PTR L_WidthIndex[esp]                         ; decrement loop ctr
    jne         WLoopStart    

WLoopEnd:
    lea         edi,[edi+esi*4]                                     ; Increment input buffer ptr by image width to setp over
                                                                    ; line already processed
    add         ebp,esi                                             ; Increment Y output buffer ptr by image widht to step over
                                                                    ; line already processed
    dec         (CConvParams PTR [esp+LOCAL_SPACE]).ImageHeight     ; decrement image height ctr
    jne         HLoopStart                                          ; are we done yet?

HLoopEnd:


theExit:
    add         esp,LOCAL_SPACE
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

