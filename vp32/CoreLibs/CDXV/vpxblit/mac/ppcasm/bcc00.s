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
;??? bcc00.s  
; - yv12 to yuy2 same blitter
;\***********************************************/ 


;;/*
;;Note:
;;- try using floats to write to screen

;;- optimizations are not complete yet
;;*/

	INCLUDE 'vp3stkfr.ash'
	INCLUDE 'vp3blit.ash'


;------------------------------------------------
; void bcc00(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
; 							  r3,          r4,             r5
;------------------------------------------------
	MakeCFunction bcc00

	ASM_PROLOG

    
    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)
    xor         r18,r18,r18             ;clear hIndex

    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)

    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)
    xor         r30,r30,r30             ;yIndex = 0

    lwz         r14, YUV_BUFFER_CONFIG.YWidth(r5)
    xor         r31,r31,r31             ;uvIndex = 0

    slwi        r14,r14,1               ;convert to byte width
 
	subi r14,r14,8

    xor         r17,r17,r17             ;wIndex = 0
    b           h_loop

	ALIGN 5
h_loop:
;;	b			w_loop
	 
;;ALIGN 5
w_loop:
    lwzx        r13,r6,r30              ;get 4 y's
    addi        r30,r30,4
	cmpw 		r14,r17					

    lhzx        r15,r7,r31              ;get 2 u's
    mr			r20,r13					;position y0
    rlwimi		r21,r13,16,0,7			;position y2
    
    lhzx        r16,r8,r31              ;get 2 v's
    rlwimi		r20,r13,24,16,23		;position y1
    rlwimi		r21,r13,8,16,23			;position y3

    rlwimi		r20,r15,8,8,15  		;position u0
    rlwimi		r21,r15,16,8,15			;position u1
				
    rlwimi		r20,r16,24,24,31  		;position v0
    rlwimi		r21,r16,0,24,31			;position v1

	stwx		r29,r3,r17				;write pixel pair 0
	addi 		r17,r17,4
    addi        r31,r31,2

	stwx		r29,r3,r17				;write pixel pair 1
	addi 		r17,r17,4
	bne      	w_loop                   


;;>>>>>>


    lwz         r22,YUV_BUFFER_CONFIG.YHeight(r5)
    addi        r18,r18,1               ;inc line ptr

    lwz         r23,YUV_BUFFER_CONFIG.YStride(r5)
    slwi        r25,r18,31

    lwz         r24,YUV_BUFFER_CONFIG.UVStride(r5)
    srawi       r25,r25,31              ;generate mask
    subf        r6,r23,r6               ;next line of y's

    and         r25,r24,r25             ;will be 0 or UVStride
    add         r3,r3,r4                ;next scan line
    
    cmpw        r18,r22
    subf        r7,r25,r7               

    subf        r8,r25,r8
    xor         r30,r30,r30             ;yIndex = 0
    
    xor         r31,r31,r31             ;uvIndex = 0
    xor         r17,r17,r17             ;wIndex = 0
    bne         h_loop
    
    
	ASM_EPILOG
    
	END