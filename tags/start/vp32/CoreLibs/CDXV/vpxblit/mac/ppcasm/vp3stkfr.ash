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
;??? stackfrm.ash   
;- ppc prolog and epilog
;\***********************************************/ 

;------------------------------------------------
;from ppc asm reference

	MACRO
	MakeCFunction 	&fnName
	export 			&fnName[DS]
	export 			.&fnName[PR]
	toc
	tc 				&fnName[TC], &fnName[DS]
	csect 			&fnName[DS]
	dc.l 			.&fnName[PR]
	dc.l 			TOC[tc0]
	csect 			.&fnName[PR]
	ENDM


;------------------------------------------------
; creates a TOC-relative address expression.
; use this to reference global data located in C modules.
; see ppc asm reference
	MACRO
	ImportGlobalData 	&gdName, &labelName
	IMPORT &gdName
	TOC
	&labelName: TC &gdName[TC], &gdName
	ENDM

;------------------------------------------------
;    Notes:
;        - Stack pointer must always be 16 byte aligned.
	MACRO 
	ASM_PROLOG
	;save link register
    mflr    r0 
    stw     r0,8(SP)

    ;save all GPR's  
    stw     r13,-76(SP) 
    stw     r14,-72(SP) 
    stw     r15,-68(SP) 
    stw     r16,-64(SP) 
    stw     r17,-60(SP) 
    stw     r18,-56(SP) 
    stw     r19,-52(SP) 
    stw     r20,-48(SP) 
    stw     r21,-44(SP) 
    stw     r22,-40(SP) 
    stw     r23,-36(SP) 
    stw     r24,-32(SP) 
    stw     r25,-28(SP) 
    stw     r26,-24(SP) 
    stw     r27,-20(SP) 
    stw     r28,-16(SP) 
    stw     r29,-12(SP) 
    stw     r30,-8(SP) 
    stw     r31,-4(SP)

    ;frame_size = link_size + gpr_size + padding
    stwu     SP,-112(SP) 
	ENDM


	MACRO 
	ASM_EPILOG
    ;restore link register
    lwz      r0,120(SP) 
    mtlr     r0 

    ;restore stack pointer
    addi     SP,SP,112 

    ;restored GPR's 
    lwz     r13,-76(SP)
    lwz     r14,-72(SP)
    lwz     r15,-68(SP)
    lwz     r16,-64(SP)
    lwz     r17,-60(SP)
    lwz     r18,-56(SP)
    lwz     r19,-52(SP)
    lwz     r20,-48(SP)
    lwz     r21,-44(SP)
    lwz     r22,-40(SP)
    lwz     r23,-36(SP)
    lwz     r24,-32(SP)
    lwz     r25,-28(SP)
    lwz     r26,-24(SP)
    lwz     r27,-20(SP)
    lwz     r28,-16(SP)
    lwz     r29,-12(SP)
    lwz     r30,-8(SP)
    lwz     r31,-4(SP)

    blr
	ENDM

;/*
;linkageArea:	set 24 				;run-time architecture dependent
;params: 		set 32 				;callee parameter area
;localVars: 		set 0 				;callee local variables
;numGPRs: 		set 0 				;volatile GPRs used by callee
;numFPRs: 		set 0 				;volatile FPRs used by callee)
;padding:		set 0				;make sure 16 byte aligned
;
;spaceToSave: 	set linkageArea + params + localVars
;spaceToSave: 	set spaceToSave + 4*numGPRs + 8*numFPRs
;
; PROLOG (callee responsibilities)
;	mflr 	r0, 					;get Link Register
;	stw 	r0,8(SP) 				;store Link Register on stack
;	stwu 	SP, -spaceToSave(SP) 	;skip over caller save area
;
;
; EPILOG (callee responsibilities)
;	lwz 	r0,spaceToSave(SP)+8 	;get saved Link Register
;	addic 	SP,SP,spaceToSave 		;reset stack pointer
;	mtlr	r0 						;reset Link Register
;*/
