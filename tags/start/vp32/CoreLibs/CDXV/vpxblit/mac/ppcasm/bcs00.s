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
;??? bcs00.s  
; - yv12 to 16 same blitter
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
	ImportGlobalData WK_ditherPats, gWK_ditherPats
	ImportGlobalData WK_ClampOrigin555, gWK_ClampOrigin555

;------------------------------------------------
; void bcs00(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
; 							  r3,          r4,             r5
;------------------------------------------------
	MakeCFunction bcs00

	ASM_PROLOG

    lwz         r9,gWK_YforY(RTOC)
    xor         r18,r18,r18             ;clear hIndex
    
    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)

    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)

    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)

    lwz         r14, YUV_BUFFER_CONFIG.YWidth(r5)

    lwz         r10,gWK_UforBG(RTOC)
    slwi        r14,r14,1               ;convert to byte width
 
    lwz         r20,gWK_ClampOrigin555(RTOC)
	subi r14,r14,4

    lwz         r11,gWK_VforRG(RTOC)

    lwz         r20,0(r20)

    lwz         r12,gWK_ditherPats(RTOC)
    andi.        r0,r18,3

    slwi        r0,r0,2
    xor         r30,r30,r30             ;yIndex = 0
    
    lwzx        r19,r12,r0              ;dither pattern for current line
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
    rlwinm      r21,r19,8,24,31         ;d0

    lhax        r24,r11,r24             ;CrforG
    add         r28,r26,r22             ;R0
    add         r29,r25,r22             ;B0

    add         r28,r28,r21             ;R0 + d0    
    add         r23,r23,r24             ;CrforG + CbforG

    lbzx        r28,r28,r20             ;get the real R0
    subf        r22,r23,r22             ;G0
    add         r29,r29,r21             ;B0 + d0

    lbzx        r29,r29,r20             ;get the real B0
    add         r22,r22,r21             ;G0 + d0
    rlwinm      r21,r19,16,24,31        ;d1

    lbzx        r0,r22,r20              ;get the real G0
    insrwi      r29,r28,5,17            ;insert R0

    lhax        r22,r9,r27              ;Y1
    insrwi      r29,r0,5,22             ;insert G0

    slwi        r0,r29,16
    add         r28,r26,r22             ;R1

    add         r29,r25,r22             ;B1
    add         r28,r28,r21             ;R1 + d1    

    lbzx        r28,r28,r20             ;get the real R1
    subf        r22,r23,r22             ;G1
    add         r29,r29,r21             ;B1 + d1

    lbzx        r29,r29,r20             ;get the real B1
    add         r22,r22,r21             ;G1 + d1
    insrwi      r0,r28,5,17             ;insert R1

    lbzx        r22,r22,r20             ;get the real G1
    insrwi      r0,r29,5,27             ;insert B1
    rlwinm      r23,r15,2,22,29         ;get u1 * 4


    insrwi      r0,r22,5,22             ;insert G1
    rlwinm      r24,r16,2,22,29         ;get v1 * 4

	stwx r0,r3,r17        ;write 2 pixels
	addi r17,r17,4

    lhax        r25,r10,r23             ;CbforB
    addi        r23,r23,2
    rlwinm      r22,r13,25,23,30         ;get y2 * 2

    lhax        r26,r11,r24             ;CrforR
	cmpw r14,r17
    addi        r24,r24,2

    lhax        r23,r10,r23             ;CbforG
    rlwinm      r27,r13,1,23,30         ;get y3 * 2

    lhax        r22,r9,r22              ;Y0
    rlwinm      r21,r19,24,24,31        ;d2

    lhax        r24,r11,r24             ;CrforG
    add         r28,r26,r22             ;R0
    add         r29,r25,r22             ;B0

    add         r28,r28,r21             ;R0 + d2    
    add         r23,r23,r24             ;CrforG + CbforG

    lbzx        r28,r28,r20             ;get the real R0
    subf        r22,r23,r22             ;G0
    add         r29,r29,r21             ;B0 + d2

    lbzx        r29,r29,r20             ;get the real B0
    add         r22,r22,r21             ;G0 + d2
    rlwinm      r21,r19,0,24,31         ;d3

    lbzx        r0,r22,r20              ;get the real G0
    insrwi      r29,r28,5,17            ;insert R0

    lhax        r22,r9,r27              ;Y1
    insrwi      r29,r0,5,22             ;insert G0

    slwi        r0,r29,16
    add         r28,r26,r22             ;R1

    add         r29,r25,r22             ;B1
    add         r28,r28,r21             ;R1 + d3    

    lbzx        r28,r28,r20             ;get the real R1
    subf        r22,r23,r22             ;G1
    add         r29,r29,r21             ;B1 + d3

    lbzx        r29,r29,r20             ;get the real B1
    add         r22,r22,r21             ;G1 + d3
    insrwi      r0,r28,5,17             ;insert R1

    lbzx        r22,r22,r20             ;get the real G1
    insrwi      r0,r29,5,27             ;insert B1
    
    lhzx       r15,r7,r31              ;get 2 u's
    insrwi      r0,r22,5,22             ;insert G1


	stwx r0,r3,r17
	addi r17,r17,4
	bne      	w_loop                   

    lwz         r22,YUV_BUFFER_CONFIG.YHeight(r5)
    slwi        r25,r18,31
    addi        r18,r18,1               ;inc line ptr

    lwz         r23,YUV_BUFFER_CONFIG.YStride(r5)
    andi.        r0,r18,3

    lwz         r24,YUV_BUFFER_CONFIG.UVStride(r5)
    srawi       r25,r25,31              ;generate mask
    subf        r6,r23,r6               ;next line of y's

    and         r25,r24,r25             ;will be 0 or UVStride
    add         r3,r3,r4                ;next scan line
    
    cmpw        r18,r22
    subf        r7,r25,r7               

    slwi        r0,r0,2
    subf        r8,r25,r8

    lwzx        r19,r12,r0              ;dither pattern for current line
    xor         r30,r30,r30             ;yIndex = 0
    
    xor         r31,r31,r31             ;uvIndex = 0
    xor         r17,r17,r17             ;wIndex = 0
    bne         h_loop
    
    
	ASM_EPILOG
    
	END