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
;??? bct00.s  
; - yv12 to 32 stretch blitter
;\***********************************************/ 


;;/*
;;Note:
;;- try using floats to write to screen

;;- optimizations are not complete yet
;;*/

	INCLUDE 'vp3stkfr.ash'
	INCLUDE 'vp3blit.ash'



	ImportGlobalData WK_YforY, gWK_YforY
	ImportGlobalData WK_UforBG, gWK_UforBG
	ImportGlobalData WK_VforRG, gWK_VforRG
	ImportGlobalData WK_ClampOrigin, gWK_ClampOrigin

;------------------------------------------------
; void bct10(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
; 							  r3,          r4,             r5
;------------------------------------------------
	MakeCFunction bct10

	ASM_PROLOG

    lwz         r9,gWK_YforY(RTOC)
    xor         r18,r18,r18             ;clear hIndex
    
    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)

    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)

    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)

    lwz         r14, YUV_BUFFER_CONFIG.YWidth(r5)

    lwz         r10,gWK_UforBG(RTOC)
    slwi        r14,r14,3               ;convert to byte width
 
    lwz         r20,gWK_ClampOrigin(RTOC)
	subi r14,r14,16

    lwz         r11,gWK_VforRG(RTOC)

    lwz         r20,0(r20)
    xor         r30,r30,r30             ;yIndex = 0
    
    xor         r31,r31,r31             ;uvIndex = 0
    xor         r17,r17,r17             ;wIndex = 0
    b           h_loop

	ALIGN 5
h_loop:
    lhzx       r15,r7,r31              ;get 2 u's
	b			w_loop
	
	ALIGN 5
w_loop:
    lhzx       r16,r8,r31              ;get 2 v's
    addi        r31,r31,2
    rlwinm      r23,r15,26,22,29        ;get u0 * 4


    lwzx       r13,r6,r30              ;get 4 y's
    addi        r30,r30,4
    rlwinm      r24,r16,26,22,29        ;get v0 * 4

    lhax        r25,r10,r23             ;CbforB
    addi        r23,r23,2
    rlwinm      r22,r13,9,23,30         ;get y0 * 2

    lhax        r26,r11,r24             ;CrforR
    addi        r24,r24,2

    lhax        r23,r10,r23             ;CbforG
    rlwinm      r27,r13,17,23,30        ;get y1 * 2

    lhax        r22,r9,r22              ;Y0
	addis       r12,0,65280             ;r12 = 0xff000000 for solid alpha 

    lhax        r24,r11,r24             ;CrforG
    add         r29,r25,r22             ;B0
    add         r28,r26,r22             ;R0

    lbzx        r29,r29,r20             ;get the real B0
    add         r23,r23,r24             ;CrforG + CbforG

    lbzx        r28,r28,r20             ;get the real R0
    subf        r22,r23,r22             ;G0
    or          r29,r29,r12             ;set alpha to 0xff

    lbzx        r0,r22,r20              ;get the real G0
    insrwi      r29,r28,8,8             ;insert R0

    lhax        r22,r9,r27              ;Y1
    insrwi      r29,r0,8,16             ;insert G0

	stwx		r29,r3,r17				;write pixel 0
	addi 		r17,r17,4

	stwx		r29,r3,r17				;write pixel 0
	addi 		r17,r17,4
    add         r28,r26,r22             ;R1

    lbzx        r28,r28,r20             ;get the real R1
    add         r29,r25,r22             ;B1
    subf        r22,r23,r22             ;G1

    lbzx        r29,r29,r20             ;get the real B1
    insrwi      r12,r28,8,8             ;insert R1

    lbzx        r22,r22,r20             ;get the real G1
    insrwi      r12,r29,8,24            ;insert B1
    rlwinm      r23,r15,2,22,29         ;get u1 * 4

    insrwi      r12,r22,8,16            ;insert G1
    rlwinm      r24,r16,2,22,29         ;get v1 * 4

;;---
	stwx 		r12,r3,r17        		;write pixel 1
	addi 		r17,r17,4

	stwx 		r12,r3,r17        		;write pixel 1
	addi 		r17,r17,4

    lhax        r25,r10,r23             ;CbforB
    addi        r23,r23,2
    rlwinm      r22,r13,25,23,30         ;get y2 * 2

    lhax        r26,r11,r24             ;CrforR
    addi        r24,r24,2
	cmpw r14,r17

    lhax        r23,r10,r23             ;CbforG
    rlwinm      r27,r13,1,23,30         ;get y3 * 2

    lhax        r22,r9,r22              ;Y0
	addis       r12,0,65280             ;r12 = 0xff000000 for solid alpha 

    lhax        r24,r11,r24             ;CrforG
    add         r29,r25,r22             ;B0
    add         r28,r26,r22             ;R0

    lbzx        r29,r29,r20             ;get the real B0
    add         r23,r23,r24             ;CrforG + CbforG

    lbzx        r28,r28,r20             ;get the real R0
    subf        r22,r23,r22             ;G0
    or          r29,r29,r12             ;set alpha to 0xff

    lbzx        r0,r22,r20              ;get the real G0
    insrwi      r29,r28,8,8             ;insert R0

    lhax        r22,r9,r27              ;Y1
    insrwi      r29,r0,8,16             ;insert G0 -- pixel 2

	stwx		r29,r3,r17				;write pixel 2
    addi		r17,r17,4

	stwx		r29,r3,r17				;write pixel 2
    add         r28,r26,r22             ;R1
    add         r29,r25,r22             ;B1

    lbzx        r28,r28,r20             ;get the real R1
    subf        r22,r23,r22             ;G1
    addi		r17,r17,4


    lbzx        r29,r29,r20             ;get the real B1
    insrwi      r12,r28,8,8             ;insert R1

    lbzx        r22,r22,r20             ;get the real G1
    insrwi      r12,r29,8,24            ;insert B1
    
    lhzx       r15,r7,r31              ;get 2 u's
    insrwi      r12,r22,8,16            ;insert G1


	stwx 		r12,r3,r17				;write pixel 3
	addi 		r17,r17,4

	stwx 		r12,r3,r17				;write pixel 3
	addi 		r17,r17,4
	bne      	w_loop                   


;;>>>>>>


    lwz         r22,YUV_BUFFER_CONFIG.YHeight(r5)
    slwi        r25,r18,31

    lwz         r23,YUV_BUFFER_CONFIG.YStride(r5)
    addi        r18,r18,1               ;inc line ptr

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