//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//--------------------------------------------------------------------------



#if defined(POSTPROCESS)

#define STRICT              /* Strict type checking. */
#include <memory.h>

#include "pbdll.h"
#include "blockmapping.h"
#include <stdio.h>
#include <stdlib.h>


//this is necessary to keep CW5.3 from crashing
//I use all of the GPR's
#pragma global_optimizer on
#pragma optimization_level 2

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

extern vector unsigned char vPerm2;


/****************************************************************************
*  Explicit Imports
*****************************************************************************
*/              

extern UINT32 LoopFilterLimitValuesV2[];

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Exported Functions
*****************************************************************************
*/              

/****************************************************************************
*  Module Statics
*****************************************************************************
*/


/****************************************************************************
 * 
 *  ROUTINE       :     DeblockLoopFilteredBand2_Altivec
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filter both horizontal and vertical edge in a band
 *
 *  SPECIAL NOTES :     
 *
 *	REFERENCE	  :		
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void DeblockLoopFilteredBand2_Altivec(
							PB_INSTANCE *pbi,					/* r3 */ 
							UINT8 *SrcPtr, 				/* r4 */
							UINT8 *DesPtr,				/* r5 */
							UINT32 PlaneLineStep, 		/* r6 */
							UINT32 FragAcross,			/* r7 */
							UINT32 StartFrag,			/* r8 */
							UINT32 *QuantScale       /* r9 */
							)
{
//
//NOTE: for some reason there is a compiler error if v25 is used.  why???????
//
//compiler uses r28 for vrsave and r11 for SP
//
//
UINT32 *QuantScalePtr = QuantScale;

	vector unsigned short tempBuffer[2];
	vector unsigned short *tempBufferPtr = tempBuffer;
	
	INT32 *FragmentVariancesPtr = pbi->FragmentVariances;
	UINT32 *FragQIndexPtr = pbi->FragQIndex;
	
	asm
	{
    	mr			r31,r8							//aka CurrentFrag
		add			r30,r8,r7						//StartFrag + FragAcross
		
		mr			r14,r4							//save src for later
		mr			r13,r5							//save dst for later

		cmpw 		r31,r30
		bge			Ldone

	whileLoop:
        subf        r5,r8,r31                       //CurrentFrag - StartFrag
		add			r23,r6,r6						//PlaneLineStep * 2	aka w2
	
	    slwi        r5,r5,3                         //8 * (CurrentFrag - StartFrag)
		add			r21,r6,r6						//PlaneLineStep * 2
		
		add         r4,r14,r5                       //Src = SrcPtr + 8 * (CurrentFrag - StartFrag)
		add         r5,r13,r5                       //Des = DesPtr + 8 * (CurrentFrag - StartFrag)
		
		add			r22,r23,r6						//PlaneLineStep * 3 aka w3
		add			r21,r21,r21						//PlaneLineStep * 4 aka w4
		
		add			r20,r21,r6						//PlaneLineStep * 5 aka w5
		xor			r0,r0,r0
		
		subf		r19,r6,r0						//-w1
		subf		r18,r23,r0						//-w2
		
		subf		r17,r22,r0						//-w3
		subf		r16,r21,r0						//-w4
		
		lwz			r26,FragmentVariancesPtr
		subf		r15,r20,r0						//-w5
		
		
		lvx			v9,r4,r15						//src[-w5]

		lvx			v1,r4,r16						//src[-w4]
		
		lvx			v2,r4,r17						//src[-w3]
		
		lvx			v3,r4,r18						//src[-w2]
		
		lvx			v4,r4,r19						//src[-w1]
		
		lvx			v5,r4,r0						//src[0]

		lvx			v6,r4,r6						//src[w1]

		lvx			v7,r4,r23						//src[w2]

		lvx			v8,r4,r22						//src[w3]
		
		lvx			v10,r4,r21						//src[w4]

		lvsl		v20,r4,r21
		
		lvsl		v19,r4,r15

		lvsl		v11,r4,r16
		vperm		v10,v10,v10,v20

		lvsl		v12,r4,r17
		vperm		v9,v9,v9,v19

		lvsl		v13,r4,r18
		vperm		v1,v1,v1,v11
		
		lvsl		v14,r4,r19
		vperm		v2,v2,v2,v12

		lvsl		v15,r4,r0
		vperm		v3,v3,v3,v13

		lvsl		v16,r4,r6
		vperm		v4,v4,v4,v14

		lvsl		v17,r4,r23
		vperm		v5,v5,v5,v15

		lvsl		v18,r4,r22
		vperm		v6,v6,v6,v16

		vperm		v7,v7,v7,v17
		
		lwz			r25,FragQIndexPtr
		add			r29,r31,r7						//CurrentFrag + FragAcross
		vperm		v8,v8,v8,v18

		lwz			r27,QuantScalePtr
		slwi		r29,r29,2						//index into long table

		lwzx 		r25,r25,r29						

		slwi		r25,r25,2						//index into long table
//----->
		vxor		v0,v0,v0
		vxor	    v31,v31,v31						//Sum1 = 0
		vxor		v30,v30,v30						//Sum2 = 0

		vmrghb		v1,v0,v1						//unsigned byte to unsigned short
    	vmrghb		v2,v0,v2						//unsigned byte to unsigned short
		vmrghb		v3,v0,v3						//unsigned byte to unsigned short
		vmrghb		v4,v0,v4						//unsigned byte to unsigned short
		vmrghb		v5,v0,v5						//unsigned byte to unsigned short
		vmrghb		v6,v0,v6						//unsigned byte to unsigned short
		vmrghb		v7,v0,v7						//unsigned byte to unsigned short
		vmrghb		v8,v0,v8						//unsigned byte to unsigned short
		vmrghb		v9,v0,v9						//unsigned byte to unsigned short
		vmrghb		v10,v0,v10						//unsigned byte to unsigned short
//----->
   		vsubuhs		v11,v1,v9						//x[k] - x[k-1]
   		vsubuhs		v27,v9,v1						//x[k-1] - x[k]
		vor 		v11,v11,v27						//abs(x[k] - x[k-1]) aka temp1[1]

   		vsubuhs		v12,v2,v1						//x[k] - x[k-1]
   		vsubuhs		v27,v1,v2						//x[k-1] - x[k]
		vor 		v12,v12,v27						//abs(x[k] - x[k-1]) aka temp1[2]

   		vsubuhs		v13,v3,v2						//x[k] - x[k-1]
   		vsubuhs		v27,v2,v3						//x[k-1] - x[k]
		vor 		v13,v13,v27						//abs(x[k] - x[k-1]) aka temp1[3]

   		vsubuhs		v14,v4,v3						//x[k] - x[k-1]
   		vsubuhs		v27,v3,v4						//x[k-1] - x[k]
		vor 		v14,v14,v27						//abs(x[k] - x[k-1]) aka temp1[4]

   		vsubuhs		v15,v5,v6						//x[k+4] - x[k+5]
   		vsubuhs		v27,v6,v5						//x[k+5] - x[k+4]
		vor 		v15,v15,v27						//abs(x[k+4] - x[k+5]) aka temp2[1]

   		vsubuhs		v16,v6,v7						//x[k+4] - x[k+5]
   		vsubuhs		v27,v7,v6						//x[k+5] - x[k+4]
		vor 		v16,v16,v27						//abs(x[k+4] - x[k+5]) aka temp2[2]

   		vsubuhs		v17,v7,v8						//x[k+4] - x[k+5]
   		vsubuhs		v27,v8,v7						//x[k+5] - x[k+4]
		vor 		v17,v17,v27						//abs(x[k+4] - x[k+5]) aka temp2[3]

   		vsubuhs		v18,v8,v10						//x[k+4] - x[k+5]
   		vsubuhs		v27,v10,v8						//x[k+5] - x[k+4]
		vor 		v18,v18,v27						//abs(x[k+4] - x[k+5]) aka temp2[4]

		vaddubs		v31,v31,v11						//Sum1 += temp1[1] (saturate to 255)
		vaddubs		v31,v31,v12						//Sum1 += temp1[2] (saturate to 255)
		vaddubs		v31,v31,v13						//Sum1 += temp1[3] (saturate to 255)
		vaddubs		v31,v31,v14						//Sum1 += temp1[4] (saturate to 255)
		
		vaddubs		v30,v30,v15						//Sum2 += temp2[1] (saturate to 255)
		vaddubs		v30,v30,v16						//Sum2 += temp2[2] (saturate to 255)
		vaddubs		v30,v30,v17						//Sum2 += temp2[3] (saturate to 255)
		vaddubs		v30,v30,v18						//Sum2 += temp2[4] (saturate to 255)


    	lvsl		v29,r27,r25

		lvewx		v28,r27,r25                     //QuantScale[pbi->FragQIndex[CurrentFrag+FragsAcross]]
		
		vperm		v28,v28,v28,v29

		vsplth		v28,v28,1						//dup QStep

		vspltish	v27,3
		
		vmladduhm	v29,v28,v27,v0                  //QStep * 3

		vspltish	v27,2
		
		lwz			r10,tempBufferPtr

		vsrah		v29,v29,v27						//FLimit = (QStep * 3) >> 2	

// --> start writing sums to temp array
		stvx		v31,r10,r0
		addi		r0,r0,16
		
		stvx		v30,r10,r0

		lha			r12,0(r10)
   		vsubuhs		v11,v5,v4						//x[5] - x[4]
		
		lha			r0,2(r10)

		lha			r25,4(r10)
		vsubuhs		v12,v4,v5						//x[4] - x[5]

		add			r0,r0,r12						//Sum1[0]+Sum1[1]
		
		lha			r12,6(r10)
		add			r0,r0,r25						//var[0]+var[1]+var[2]	
		vor 		v19,v11,v12						//abs(x[5] - x[4])
		
		lha			r25,8(r10)
		vsubuhm		v19,v19,v28                     //abs(x[5] - x[4]) - QStep 		
		vspltish	v12,15
		
		add			r0,r0,r12						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]
		
		lha			r12,10(r10)						
		add			r0,r0,r25						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]+Sum1[4]
		vsrah		v19,v19,v12                     //FFFF/0000 for true/false
		
		lha			r25,12(r10)
		mr			r27,r31
		
		add			r0,r0,r12						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]+Sum1[4]+Sum1[5]

		slwi		r27,r27,2						//index into long table
		
		lha			r12,14(r10)
		add			r0,r0,r25						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]+Sum1[4]+Sum1[5]+Sum1[6]
		vsubuhm		v11,v31,v29						//Sum1 < FLimit?
		
		lwzx		r25,r26,r27						//pbi->FragmentVariances[CurrentFrag]
//...		
		vsubuhm		v13,v30,v29						//Sum2 < FLimit?

		vsrah		v11,v11,v12						//FFFF/0000 for true/false
		add			r0,r0,r12						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]+Sum1[4]+Sum1[5]+Sum1[6]+Sum1[7]
		
		vsrah		v13,v13,v12                     //FFFF/0000 for true/false
		add			r25,r25,r0
		
		stwx		r25,r26,r27						//pbi->FragmentVariances[CurrentFrag]
//...		
		lha			r12,16(r10)

		lha			r0,18(r10)
		
		lha			r25,20(r10)
		
		vand		v31,v11,v13						
		vand		v31,v31,v19						//bit mask for vsel

		add			r0,r0,r12						//Sum2[0]+Sum2[1]
		
		lha			r12,22(r10)
		add			r0,r0,r25						//Sum2[0]+Sum2[1]+Sum2[2]
		
		lha			r25,24(r10)
		
		add			r0,r0,r12						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]
		
		lha			r12,26(r10)
		add			r0,r0,r25						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]+Sum2[4]
		
		lha			r25,28(r10)

		add			r0,r0,r12						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]+Sum2[4]+Sum2[5]
		
		lha			r12,30(r10)
		add			r0,r0,r25						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]+Sum2[4]+Sum2[5]+Sum2[6]
		
		lwzx		r25,r26,r29						//pbi->FragmentVariances[CurrentFrag + FragAcross]
//...		
		
		add			r0,r0,r12						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]+Sum2[4]+Sum2[5]+Sum2[6]+Sum2[7]
				
		add			r25,r25,r0
		xor			r0,r0,r0
		
		stwx		r25,r26,r29						//pbi->FragmentVariances[CurrentFrag + FragAcross]
//...		
//<-- stop writing sums here

		/* low pass filtering (LPF7: 1 1 1 2 1 1 1) */
		vspltish    v27,4
        vadduhm     v27,v27,v9                      //4 + x[0]
        vadduhm     v27,v27,v9                      //4 + x[0] + x[0]
        vadduhm     v27,v27,v9                      //4 + x[0] + x[0] + x[0]
        vadduhm     v27,v27,v1                      //4 + x[0] + x[0] + x[0] + x[1]
        vadduhm     v27,v27,v2                      //4 + x[0] + x[0] + x[0] + x[1] + x[2]
        vadduhm     v27,v27,v3                      //4 + x[0] + x[0] + x[0] + x[1] + x[2] + x[3]
        vadduhm     v27,v27,v4                      //4 + x[0] + x[0] + x[0] + x[1] + x[2] + x[3] + x[4]
        vadduhm     v11,v27,v1                      //4 + x[0] + x[0] + x[0] + x[1] + x[2] + x[3] + x[4] + x[1]
        vsubuhm     v27,v27,v9                      //4 + x[0] + x[0] + x[1] + x[2] + x[3] + x[4]
        vadduhm     v27,v27,v5                      //4 + x[0] + x[0] + x[1] + x[2] + x[3] + x[4] + x[5]
        vadduhm     v12,v27,v2                      //4 + x[0] + x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[2]
        vsubuhm     v27,v27,v9                      //4 + x[0] + x[1] + x[2] + x[3] + x[4] + x[5]
        vadduhm     v27,v27,v6                      //4 + x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6]
        vadduhm     v13,v27,v3                      //4 + x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[3]
        vsubuhm     v27,v27,v9                      //4 + x[1] + x[2] + x[3] + x[4] + x[5] + x[6]
        vadduhm     v27,v27,v7                      //4 + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7]
        vadduhm     v14,v27,v4                      //4 + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7] + x[4]
        vsubuhm     v27,v27,v1                      //4 + x[2] + x[3] + x[4] + x[5] + x[6] + x[7]
        vadduhm     v27,v27,v8                      //4 + x[2] + x[3] + x[4] + x[5] + x[6] + x[7] + x[8]
        vadduhm     v15,v27,v5                      //4 + x[2] + x[3] + x[4] + x[5] + x[6] + x[7] + x[8] + x[5]
        vsubuhm     v27,v27,v2                      //4 + x[3] + x[4] + x[5] + x[6] + x[7] + x[8]
        vadduhm     v27,v27,v10                     //4 + x[3] + x[4] + x[5] + x[6] + x[7] + x[8] + x[9]
        vadduhm     v16,v27,v6                      //4 + x[3] + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] + x[6]
        vsubuhm     v27,v27,v3                      //4 + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] 
        vadduhm     v27,v27,v10                     //4 + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] + x[9]
        vadduhm     v17,v27,v7                      //4 + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] + x[9] + x[7]
        vsubuhm     v27,v27,v4                      //4 + x[5] + x[6] + x[7] + x[8] + x[9] + x[9]
        vadduhm     v27,v27,v10                     //4 + x[5] + x[6] + x[7] + x[8] + x[9] + x[9] + x[9]
        vadduhm     v18,v27,v8                      //4 + x[5] + x[6] + x[7] + x[8] + x[9] + x[9] + x[9] + x[8]

		vspltish    v27,3
		vsrah		v11,v11,v27
		vsrah		v12,v12,v27
		vsrah		v13,v13,v27
		vsrah		v14,v14,v27
		vsrah		v15,v15,v27
		vsrah		v16,v16,v27
		vsrah		v17,v17,v27
		vsrah		v18,v18,v27
//-----
		vsel		v11,v1,v11,v31

		vsel		v12,v2,v12,v31
		vpkuhus		v11,v11,v11

		vsel		v13,v3,v13,v31
		vpkuhus		v12,v12,v12

		vsel		v14,v4,v14,v31
		vpkuhus		v13,v13,v13

		vsel		v15,v5,v15,v31
		vpkuhus		v14,v14,v14

		vsel		v16,v6,v16,v31
		vpkuhus		v15,v15,v15

		vsel		v17,v7,v17,v31
		vpkuhus		v16,v16,v16

		vsel		v18,v8,v18,v31
		vpkuhus		v17,v17,v17
		
		lvsr		v1,r5,r16						//dst[-w4]
		vpkuhus		v18,v18,v18		
//<-----
		lvsr		v2,r5,r17						//dst[-w3]

		lvsr		v3,r5,r18						//dst[-w2]
		vperm		v11,v11,v11,v1

		lvsr		v4,r5,r19						//dst[-w1]
		vperm		v12,v12,v12,v2
		xor			r0,r0,r0

		lvsr		v5,r5,r0						//dst[0]
		vperm		v13,v13,v13,v3

		lvsr		v6,r5,r6						//dst[w1]
		vperm		v14,v14,v14,v4

		lvsr		v7,r5,r23						//dst[w2]
		vperm		v15,v15,v15,v5

		lvsr		v8,r5,r22						//dst[w3]
		vperm		v16,v16,v16,v6
    	cmpw 		r31,r8						    //CurrentFrag == StartFrag ????
		
		stvewx		v11,r5,r16
		addi		r16,r16,4
		vperm		v17,v17,v17,v7

		stvewx		v11,r5,r16
		addi		r16,r16,-4
		vperm		v18,v18,v18,v8
		
		stvewx		v12,r5,r17
		addi		r17,r17,4

		stvewx		v12,r5,r17
		addi		r17,r17,-4

		stvewx		v13,r5,r18
		addi		r18,r18,4

		stvewx		v13,r5,r18
		addi		r18,r18,-4

		stvewx		v14,r5,r19
		addi		r19,r19,4

		stvewx		v14,r5,r19
		addi		r19,r19,-4
//
		stvewx		v15,r5,r0
		addi		r0,r0,4
		
		stvewx		v15,r5,r0
		addi		r0,r0,-4
		
		stvewx		v16,r5,r6
		addi		r6,r6,4

		stvewx		v16,r5,r6
		addi		r6,r6,-4

		stvewx		v17,r5,r23
		addi		r23,r23,4

		stvewx		v17,r5,r23
		addi		r23,r23,-4

		stvewx		v18,r5,r22
		addi		r22,r22,4

		stvewx		v18,r5,r22
		xor			r0,r0,r0
    	bne         L12
    	
        addi        r31,r31,1                       //CurrentFrag++
    	b			LcheckLoop

	L12: 	
		slwi		r22,r6,3						//PlaneLineStep * 8
		add			r15,r6,r6						//PlaneLineStep * 2
		
        subf        r5,r8,r31                       //CurrentFrag - StartFrag
		add			r16,r15,r6						//PlaneLineStep * 3

        slwi        r5,r5,3                         //8 * (CurrentFrag - StartFrag)
		add			r17,r15,r15						//PlaneLineStep * 4

        subf        r5,r22,r5                       //8 * (CurrentFrag - StartFrag) - (PlaneLineStep * 8)
		add			r18,r17,r6						//PlaneLineStep * 5

        add         r5,r5,r13                       //add in DesPtr
nop

		addi		r4,r5,-5						//src is 5 bytes in from edge of block
		addi		r5,r5,-4						//dst is 4 bytes in from edge of block

		add			r19,r16,r16						//PlaneLineStep * 6
		add			r20,r17,r16						//PlaneLineStep * 7

//	filterHorizontal2:
	
	
/*
	from
		00 01 02 03 04 05 06 07 08 09
		10 11 12 13 14 15 16 17 18 19
		20 21 22 23 24 25 26 27 28 29
		30 31 32 33 34 35 36 37 38 39
		40 41 42 43 44 45 46 47 48 49
		50 51 52 53 54 55 56 57 58 59
		60 61 62 63 64 65 66 67 68 69
		70 71 72 73 74 75 76 77 78 79
	to
v9		70 60 50 40 30 20 10 00
v1		71 61 51 41 31 21 11 01
v2		72 62 52 42 32 22 12 02
v3		73 63 53 43 33 23 13 03
v4		74 64 54 44 34 24 14 04
v5		75 65 55 45 35 25 15 05
v6		76 66 56 46 36 26 16 06
v7		77 67 57 47 37 27 17 07
v8		78 68 58 48 38 28 18 08
v10		79 69 59 49 39 29 19 09
*/

//start of 10x8 transpose
		lvx			v0,r4,r0						
		addi		r0,r0,16
		
		lvx			v10,r4,r0

		lvx			v1,r4,r6									
		addi		r0,r6,16

		lvx			v11,r4,r0

		lvx			v2,r4,r15									
		addi		r0,r15,16

		lvx			v12,r4,r0

		lvx			v3,r4,r16									
		addi		r0,r16,16

		lvx			v13,r4,r0

		lvx			v4,r4,r17									
		addi		r0,r17,16

		lvx			v14,r4,r0

		lvx			v5,r4,r18									
		addi		r0,r18,16

		lvx			v15,r4,r0

		lvx			v6,r4,r19									
		addi		r0,r19,16

		lvx			v16,r4,r0

		lvx			v7,r4,r20									
		addi		r0,r20,16
		
		lvx			v17,r4,r0
		vxor		v31,v31,v31
		xor			r0,r0,r0

		lvsl		v20,r4,r0

		lvsl		v21,r4,r6

		lvsl		v22,r4,r15
		vperm		v0,v0,v10,v20					//00 01 02 03 04 05 06 07  08 09 xx xx xx xx xx xx	

		lvsl		v23,r4,r16
		vperm		v11,v1,v11,v21					//10 11 12 13 14 15 16 17  18 19 xx xx xx xx xx xx	

		lvsl		v24,r4,r17	
		vperm		v12,v2,v12,v22					//20 21 22 23 24 25 26 27  28 29 xx xx xx xx xx xx	

		lvsl		v28,r4,r18
		vperm		v13,v3,v13,v23					//30 31 32 33 34 35 36 37  38 39 xx xx xx xx xx xx	

		lvsl		v26,r4,r19
		vperm		v14,v4,v14,v24					//40 41 42 43 44 45 46 47  48 49 xx xx xx xx xx xx	

		lvsl		v27,r4,r20
		vperm		v15,v5,v15,v28					//50 51 52 53 54 55 56 57  58 59 xx xx xx xx xx xx	

		vperm		v16,v6,v16,v26					//60 61 62 63 64 65 66 67  68 69 xx xx xx xx xx xx	
		vperm		v17,v7,v17,v27					//70 71 72 73 74 75 76 77  78 79 xx xx xx xx xx xx	

		vmrglb		v8,v17,v16						//78 68 79 69 xx xx xx xx  xx xx xx xx xx xx xx xx
		vmrglb		v9,v15,v14						//58 48 59 49 xx xx xx xx  xx xx xx xx xx xx xx xx
		vmrghh		v8,v8,v9						//78 68 58 48 79 69 59 49  xx xx xx xx xx xx xx xx					
		vmrglb		v10,v13,v12						//38 28 39 29 xx xx xx xx  xx xx xx xx xx xx xx xx 
		vmrglb		v9,v11,v0						//18 08 19 09 xx xx xx xx  xx xx xx xx xx xx xx xx 
		vmrghh		v10,v10,v9						//38 28 18 08 39 29 19 09  xx xx xx xx xx xx xx xx
		vmrghw		v8,v8,v10						//78 68 58 48 38 28 39 29  79 69 59 49 39 29 19 09
		vmrglb		v10,v31,v8						//00 79 00 69 00 59 00 49  00 39 00 29 00 19 00 09
		vmrghb		v8,v31,v8						//00 78 00 68 00 58 00 48  00 38 00 28 00 18 00 08

		vmrghb		v17,v17,v16						//70 60 71 61 72 62 73 63  74 64 75 65 76 66 77 67
		vmrghb		v15,v15,v14						//50 40 51 41 52 42 53 43  54 44 55 45 56 46 57 47
		vmrghb		v13,v13,v12						//30 20 31 21 32 22 33 23  34 24 35 25 36 26 37 27
		vmrghb		v11,v11,v0						//10 00 11 01 12 02 13 03  14 04 15 05 16 06 17 07
		
		vmrglh		v20,v17,v15						//74 64 54 44 75 65 55 45  76 66 56 46 77 67 57 47
		vmrglh		v21,v13,v11						//34 24 14 04 35 25 15 05  36 26 16 06 37 27 17 07
		vmrglw		v22,v20,v21						//76 66 56 46 36 26 16 06  77 67 57 47 37 27 17 07
		vmrghb		v6,v31,v22						//00 76 00 66 00 56 00 46  00 36 00 26 00 16 00 06
		vmrglb		v7,v31,v22						//00 77 00 67 00 57 00 47  00 37 00 27 00 17 00 07
		vmrghw		v22,v20,v21						//74 64 54 44 34 24 14 04  75 65 55 45 35 25 15 05
		vmrghb		v4,v31,v22						//00 74 00 64 00 54 00 44  00 34 00 24 00 14 00 04
		vmrglb		v5,v31,v22						//00 75 00 65 00 55 00 45  00 35 00 25 00 15 00 05
		
		vmrghh		v20,v17,v15						//70 60 50 40 71 61 51 41  72 62 52 42 73 63 53 43
		vmrghh		v21,v13,v11						//30 20 10 00 31 21 11 01  32 22 12 02 33 23 13 03
		vmrglw		v22,v20,v21						//72 62 52 42 32 22 12 02  73 63 53 43 33 23 13 03
		vmrghb		v2,v31,v22						//00 72 00 62 00 52 00 42  00 32 00 22 00 12 00 02
		vmrglb		v3,v31,v22						//00 73 00 63 00 53 00 43  00 33 00 23 00 13 00 03
		vmrghw		v22,v20,v21						//70 60 50 40 30 20 10 00  71 61 51 41 31 21 11 01
		vmrghb		v9,v31,v22						//00 70 00 60 00 50 00 40  00 30 00 20 00 10 00 00
		vmrglb		v1,v31,v22						//00 71 00 61 00 51 00 41  00 31 00 21 00 11 00 01
//end of 10x8 transpose
//=======
		lwz			r25,FragQIndexPtr

		lwz			r27,QuantScalePtr
		mr  		r29,r31 					    //CurrentFrag

		slwi		r29,r29,2						//index into long table

		lwzx 		r25,r25,r29						

		slwi		r25,r25,2						//index into long table
//----->
//----->

		vxor		v0,v0,v0
		vxor	    v31,v31,v31						//Sum1 = 0
		vxor		v30,v30,v30						//Sum2 = 0

   		vsubuhs		v11,v1,v9						//x[k] - x[k-1]
   		vsubuhs		v27,v9,v1						//x[k-1] - x[k]
		vor 		v11,v11,v27						//abs(x[k] - x[k-1]) aka temp1[1]

   		vsubuhs		v12,v2,v1						//x[k] - x[k-1]
   		vsubuhs		v27,v1,v2						//x[k-1] - x[k]
		vor 		v12,v12,v27						//abs(x[k] - x[k-1]) aka temp1[2]

   		vsubuhs		v13,v3,v2						//x[k] - x[k-1]
   		vsubuhs		v27,v2,v3						//x[k-1] - x[k]
		vor 		v13,v13,v27						//abs(x[k] - x[k-1]) aka temp1[3]

   		vsubuhs		v14,v4,v3						//x[k] - x[k-1]
   		vsubuhs		v27,v3,v4						//x[k-1] - x[k]
		vor 		v14,v14,v27						//abs(x[k] - x[k-1]) aka temp1[4]

   		vsubuhs		v15,v5,v6						//x[k+4] - x[k+5]
   		vsubuhs		v27,v6,v5						//x[k+5] - x[k+4]
		vor 		v15,v15,v27						//abs(x[k+4] - x[k+5]) aka temp2[1]

   		vsubuhs		v16,v6,v7						//x[k+4] - x[k+5]
   		vsubuhs		v27,v7,v6						//x[k+5] - x[k+4]
		vor 		v16,v16,v27						//abs(x[k+4] - x[k+5]) aka temp2[2]

   		vsubuhs		v17,v7,v8						//x[k+4] - x[k+5]
   		vsubuhs		v27,v8,v7						//x[k+5] - x[k+4]
		vor 		v17,v17,v27						//abs(x[k+4] - x[k+5]) aka temp2[3]

   		vsubuhs		v18,v8,v10						//x[k+4] - x[k+5]
   		vsubuhs		v27,v10,v8						//x[k+5] - x[k+4]
		vor 		v18,v18,v27						//abs(x[k+4] - x[k+5]) aka temp2[4]

		vaddubs		v31,v31,v11						//Sum1 += temp1[1] (saturate to 255)
		vaddubs		v31,v31,v12						//Sum1 += temp1[2] (saturate to 255)
		vaddubs		v31,v31,v13						//Sum1 += temp1[3] (saturate to 255)
		vaddubs		v31,v31,v14						//Sum1 += temp1[4] (saturate to 255)
		
		vaddubs		v30,v30,v15						//Sum2 += temp2[1] (saturate to 255)
		vaddubs		v30,v30,v16						//Sum2 += temp2[2] (saturate to 255)
		vaddubs		v30,v30,v17						//Sum2 += temp2[3] (saturate to 255)
		vaddubs		v30,v30,v18						//Sum2 += temp2[4] (saturate to 255)


    	lvsl		v29,r27,r25

		lvewx		v28,r27,r25                     //QuantScale[pbi->FragQIndex[CurrentFrag]]
		
		vperm		v28,v28,v28,v29

		vsplth		v28,v28,1						//dup QStep

		vspltish	v27,3
		
		vmladduhm	v29,v28,v27,v0                  //QStep * 3

		vspltish	v27,2
		
		lwz			r10,tempBufferPtr

		vsrah		v29,v29,v27						//FLimit = (QStep * 3) >> 2	

// --> start writing sums to temp array
		stvx		v31,r10,r0
		addi		r0,r0,16
		
		stvx		v30,r10,r0

		lha			r12,0(r10)
   		vsubuhs		v11,v5,v4						//x[5] - x[4]
		
		lha			r0,2(r10)

		lha			r25,4(r10)
		vsubuhs		v12,v4,v5						//x[4] - x[5]

		add			r0,r0,r12						//Sum1[0]+Sum1[1]
		
		lha			r12,6(r10)
		add			r0,r0,r25						//var[0]+var[1]+var[2]	
		vor 		v19,v11,v12						//abs(x[5] - x[4])
		
		lha			r25,8(r10)
		vsubuhm		v19,v19,v28                     //abs(x[5] - x[4]) - QStep 		
		vspltish	v12,15
		
		add			r0,r0,r12						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]
		
		lha			r12,10(r10)						
		add			r0,r0,r25						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]+Sum1[4]
		vsrah		v19,v19,v12                     //FFFF/0000 for true/false
		
		lha			r25,12(r10)
		mr			r27,r31
addi r27,r27,-1
		
		add			r0,r0,r12						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]+Sum1[4]+Sum1[5]

		slwi		r27,r27,2						//index into long table
		
		lha			r12,14(r10)
		add			r0,r0,r25						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]+Sum1[4]+Sum1[5]+Sum1[6]
		vsubuhm		v11,v31,v29						//Sum1 < FLimit?
		
		lwzx		r25,r26,r27						//pbi->FragmentVariances[CurrentFrag - 1]
		vsubuhm		v13,v30,v29						//Sum2 < FLimit?

		vsrah		v11,v11,v12						//FFFF/0000 for true/false
		add			r0,r0,r12						//Sum1[0]+Sum1[1]+Sum1[2]+Sum1[3]+Sum1[4]+Sum1[5]+Sum1[6]+Sum1[7]
		
		vsrah		v13,v13,v12                     //FFFF/0000 for true/false
		add			r25,r25,r0
		
		stwx		r25,r26,r27						//pbi->FragmentVariances[CurrentFrag - 1]
		
		lha			r12,16(r10)

		lha			r0,18(r10)
		
		lha			r25,20(r10)
		
		vand		v31,v11,v13						
		vand		v31,v31,v19						//bit mask for vsel

		add			r0,r0,r12						//Sum2[0]+Sum2[1]
		
		lha			r12,22(r10)
		add			r0,r0,r25						//Sum2[0]+Sum2[1]+Sum2[2]
		
		lha			r25,24(r10)
		
		add			r0,r0,r12						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]
		
		lha			r12,26(r10)
		add			r0,r0,r25						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]+Sum2[4]
		
		lha			r25,28(r10)

		add			r0,r0,r12						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]+Sum2[4]+Sum2[5]
		
		lha			r12,30(r10)
		add			r0,r0,r25						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]+Sum2[4]+Sum2[5]+Sum2[6]
		
		lwzx		r25,r26,r29						//pbi->FragmentVariances[CurrentFrag]
		
		add			r0,r0,r12						//Sum2[0]+Sum2[1]+Sum2[2]+Sum2[3]+Sum2[4]+Sum2[5]+Sum2[6]+Sum2[7]
				
		add			r25,r25,r0
		xor			r0,r0,r0
		
		stwx		r25,r26,r29						//pbi->FragmentVariances[CurrentFrag]
//<-- stop writing sums here

		/* low pass filtering (LPF7: 1 1 1 2 1 1 1) */
		vspltish    v27,4
        vadduhm     v27,v27,v9                      //4 + x[0]
        vadduhm     v27,v27,v9                      //4 + x[0] + x[0]
        vadduhm     v27,v27,v9                      //4 + x[0] + x[0] + x[0]
        vadduhm     v27,v27,v1                      //4 + x[0] + x[0] + x[0] + x[1]
        vadduhm     v27,v27,v2                      //4 + x[0] + x[0] + x[0] + x[1] + x[2]
        vadduhm     v27,v27,v3                      //4 + x[0] + x[0] + x[0] + x[1] + x[2] + x[3]
        vadduhm     v27,v27,v4                      //4 + x[0] + x[0] + x[0] + x[1] + x[2] + x[3] + x[4]
        vadduhm     v11,v27,v1                      //4 + x[0] + x[0] + x[0] + x[1] + x[2] + x[3] + x[4] + x[1]
        vsubuhm     v27,v27,v9                      //4 + x[0] + x[0] + x[1] + x[2] + x[3] + x[4]
        vadduhm     v27,v27,v5                      //4 + x[0] + x[0] + x[1] + x[2] + x[3] + x[4] + x[5]
        vadduhm     v12,v27,v2                      //4 + x[0] + x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[2]
        vsubuhm     v27,v27,v9                      //4 + x[0] + x[1] + x[2] + x[3] + x[4] + x[5]
        vadduhm     v27,v27,v6                      //4 + x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6]
        vadduhm     v13,v27,v3                      //4 + x[0] + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[3]
        vsubuhm     v27,v27,v9                      //4 + x[1] + x[2] + x[3] + x[4] + x[5] + x[6]
        vadduhm     v27,v27,v7                      //4 + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7]
        vadduhm     v14,v27,v4                      //4 + x[1] + x[2] + x[3] + x[4] + x[5] + x[6] + x[7] + x[4]
        vsubuhm     v27,v27,v1                      //4 + x[2] + x[3] + x[4] + x[5] + x[6] + x[7]
        vadduhm     v27,v27,v8                      //4 + x[2] + x[3] + x[4] + x[5] + x[6] + x[7] + x[8]
        vadduhm     v15,v27,v5                      //4 + x[2] + x[3] + x[4] + x[5] + x[6] + x[7] + x[8] + x[5]
        vsubuhm     v27,v27,v2                      //4 + x[3] + x[4] + x[5] + x[6] + x[7] + x[8]
        vadduhm     v27,v27,v10                     //4 + x[3] + x[4] + x[5] + x[6] + x[7] + x[8] + x[9]
        vadduhm     v16,v27,v6                      //4 + x[3] + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] + x[6]
        vsubuhm     v27,v27,v3                      //4 + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] 
        vadduhm     v27,v27,v10                     //4 + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] + x[9]
        vadduhm     v17,v27,v7                      //4 + x[4] + x[5] + x[6] + x[7] + x[8] + x[9] + x[9] + x[7]
        vsubuhm     v27,v27,v4                      //4 + x[5] + x[6] + x[7] + x[8] + x[9] + x[9]
        vadduhm     v27,v27,v10                     //4 + x[5] + x[6] + x[7] + x[8] + x[9] + x[9] + x[9]
        vadduhm     v18,v27,v8                      //4 + x[5] + x[6] + x[7] + x[8] + x[9] + x[9] + x[9] + x[8]

		vspltish    v27,3
		vsrah		v11,v11,v27
		vsrah		v12,v12,v27
		vsrah		v13,v13,v27
		vsrah		v14,v14,v27
		vsrah		v15,v15,v27
		vsrah		v16,v16,v27
		vsrah		v17,v17,v27
		vsrah		v18,v18,v27
//-----
		vsel		v11,v1,v11,v31

		vsel		v12,v2,v12,v31
		vpkuhus		v11,v11,v11

		vsel		v13,v3,v13,v31
		vpkuhus		v12,v12,v12

		vsel		v14,v4,v14,v31
		vpkuhus		v13,v13,v13

		vsel		v15,v5,v15,v31
		vpkuhus		v14,v14,v14

		vsel		v16,v6,v16,v31
		vpkuhus		v15,v15,v15

		vsel		v17,v7,v17,v31
		vpkuhus		v16,v16,v16

		lwz			r21,vPerm2
		vsel		v18,v8,v18,v31
		vpkuhus		v17,v17,v17
		
		xor 		r0,r0,r0
		vpkuhus		v18,v18,v18		
//<-----

/*
	from
v11		71 61 51 41 31 21 11 01
v12		72 62 52 42 32 22 12 02

v13		73 63 53 43 33 23 13 03
v14		74 64 54 44 34 24 14 04

v15		75 65 55 45 35 25 15 05
v16		76 66 56 46 36 26 16 06

v17		77 67 57 47 37 27 17 07
v18		78 68 58 48 38 28 18 08
	to
v11		01 02 03 04 05 06 07 08
v12		11 12 13 14 15 16 17 18
v13		21 22 23 24 25 26 27 28
v14		31 32 33 34 35 36 37 38
v15		41 42 43 44 45 46 47 48
v16		51 52 53 54 55 56 57 58
v17		61 62 63 64 65 66 67 68
v18		71 72 73 74 75 76 77 78
	
*/

//start of 8x8 transpose
		lvx			v0,r21,r0


		vmrghb		v1,v11,v12						//71 72 61 62 51 52 41 42  31 32 21 22 11 12 01 02
		vmrghb		v3,v13,v14						//73 74 63 64 53 54 43 44  33 34 23 24 13 14 03 04
		vmrghb		v5,v15,v16						//75 76 65 66 55 56 45 46  35 36 25 26 15 16 05 06
		vmrghb		v7,v17,v18						//77 78 67 68 57 58 47 48  37 38 27 28 17 18 07 08
		
		vmrghh		v2,v1,v3						//71 72 73 74 61 62 63 64  51 52 53 54 41 42 43 44
		vmrglh		v4,v1,v3						//31 32 33 34 21 22 23 24  11 12 13 14 01 02 03 04
		vmrghh		v6,v5,v7						//75 76 77 78 65 66 67 68  55 56 57 58 45 46 47 48
		vmrglh		v8,v5,v7						//35 36 37 38 25 26 27 28  15 16 17 18 05 06 07 08
		
		vmrghw		v18,v2,v6						//71 72 73 74 75 76 77 78  61 62 63 64 65 66 67 68 
		vmrghw		v14,v4,v8						//31 32 33 34 35 36 37 38  21 22 23 24 25 26 27 28
		vmrglw		v16,v2,v6						//51 52 53 54 55 56 57 58  41 42 43 44 45 46 47 48
		vmrglw		v12,v4,v8						//11 12 13 14 15 16 17 18  01 02 03 04 05 06 07 08
		
		vperm		v11,v12,v12,v0					//01 02 03 04 05 06 07 08  11 12 13 14 15 16 17 18
		vperm		v13,v14,v14,v0					//21 22 23 24 25 26 27 28  31 32 33 34 35 36 37 38

		lvsr		v1,r5,r0
		vperm		v15,v16,v16,v0					//41 42 43 44 45 46 47 48  51 52 53 54 55 56 57 58

		lvsr		v2,r5,r6
		vperm		v17,v18,v18,v0					//61 62 63 64 65 66 67 68  71 72 73 74 75 76 77 78  
//end of 8x8 transpose
//=======
		lvsr		v3,r5,r15
		vperm		v11,v11,v11,v1

		lvsr		v4,r5,r16
		vperm		v12,v12,v12,v2

		lvsr		v5,r5,r17
		vperm		v13,v13,v13,v3

		lvsr		v6,r5,r18
		vperm		v14,v14,v14,v4

		lvsr		v7,r5,r19
		vperm		v15,v15,v15,v5

		lvsr		v8,r5,r20
		vperm		v16,v16,v16,v6
        addi        r31,r31,1                       //CurrentFrag++

		stvewx		v11,r5,r0
		vperm		v17,v17,v17,v7
		addi		r0,r0,4

		stvewx		v11,r5,r0
		vperm		v18,v18,v18,v8

		stvewx		v12,r5,r6
		addi		r0,r6,4

		stvewx		v12,r5,r0

		stvewx		v13,r5,r15
		addi		r0,r15,4

		stvewx		v13,r5,r0

		stvewx		v14,r5,r16
		addi		r0,r16,4

		stvewx		v14,r5,r0

		stvewx		v15,r5,r17
		addi		r0,r17,4

		stvewx		v15,r5,r0

		stvewx		v16,r5,r18
		addi		r0,r18,4

		stvewx		v16,r5,r0

		stvewx		v17,r5,r19
		addi		r0,r19,4

		stvewx		v17,r5,r0

		stvewx		v18,r5,r20
		addi		r0,r20,4

		stvewx		v18,r5,r0

LcheckLoop:
		cmpw 		r31,r30							//set flags for branch below
        blt         whileLoop

Ldone:
	}


}

#endif