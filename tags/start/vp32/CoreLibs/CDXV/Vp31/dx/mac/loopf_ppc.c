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


/****************************************************************************
*
*   Module Title :     loopf_asm.c
*
*   Description  :     Optimized version of the loop filter.
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Frames
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include <memory.h>

#include "pbdll.h"
#include <stdio.h>





/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

/****************************************************************************
*  Explicit Imports
*****************************************************************************
*/              

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
 *  ROUTINE       :     SetupBoundingValueArray_Altivec
 *
 *  INPUTS        :      
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to the edge pixels of coded blocks.
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
INT32 *SetupBoundingValueArray_Altivec(PB_INSTANCE *pbi, INT32 FLimit)
{
    INT32 * BoundingValuePtr;
    INT32 i;

    // Set up the bounding value array.
    for ( i = 0; i < 512; i++ )
	{
		pbi->FiltBoundingValue[i] = 0;
	}   
	
    /* 
        Since the FiltBoundingValue array is currently only used in the generic version, we are going
        to reuse this memory for our own purposes.
        4 longs for limit storage, the rest as a temp work area
    */
 	BoundingValuePtr = (INT32 *)( ((UINT32)(&pbi->FiltBoundingValue[256]) + 15) & 0xfffffff0);    

    //expand for altivec code
    BoundingValuePtr[0] = BoundingValuePtr[1] =
    BoundingValuePtr[2] = BoundingValuePtr[3] = FLimit * 0x00010001;


    return BoundingValuePtr;
}
/****************************************************************************
 * 
 *  ROUTINE       :     FilterHoriz_Altivec
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to the vertical edge horizontally
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/           
 										/*			r3					r4						r5  */
void FilterHoriz_Altivec(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr)
{

//return;

	//NOTE: compiler uses r12 for vrsave
	asm
	{
		mr			r3,r4
		mr			r4,r5
		mr			r5,r6
	
	
// -- start of 4x8 transpose --	
		xor			r8,r8,r8
		mr			r6,r3						//work ptr

		lvx			v1,r6,r8					//LSQ 0
		addi		r7,r3,16					//hi quadword ptr		
		
		lvsl		v0,r6,r8
		
		lvx			v2,r7,r8					//MSQ 0
		add			r8,r8,r4
		
		lvx			v3,r6,r8					//LSQ 1
		vperm		v1,v1,v2,v0					//00 10 20 30
				
		lvx			v4,r7,r8					//MSQ 1

		lvsl		v0,r6,r8
		add			r8,r8,r4
		
		lvx			v5,r6,r8					//LSQ 2
		vperm		v3,v3,v4,v0					//01 11 21 31
						
		lvx			v6,r7,r8					//MSQ 2
		vmrghb		v2,v1,v3					//00 01 10 11 20 21 30 31

		lvsl		v0,r6,r8
		add			r8,r8,r4

		lvx			v7,r6,r8					//LSQ 3
		vperm		v5,v5,v6,v0					//02 12 22 32
				
		lvx			v8,r7,r8					//MSQ 3

		lvsl		v0,r6,r8
		add			r8,r8,r4

		lvx			v9,r6,r8					//LSQ 4
		vperm		v7,v7,v8,v0					//03 13 23 33
				
		lvx			v10,r7,r8					//MSQ 4
		vmrghb		v6,v5,v7					//02 03 12 13 22 23 32 33

		lvsl		v0,r6,r8
		add			r8,r8,r4

		lvx			v11,r6,r8					//LSQ 5
		vperm		v9,v9,v10,v0				//04 14 24 34
				
		lvx			v12,r7,r8					//MSQ 5
		vmrghh		v2,v2,v6					//00 01 02 03 10 11 12 13 20 21 22 23 30 31 32 33

		lvsl		v0,r6,r8
		add			r8,r8,r4

		lvx			v13,r6,r8					//LSQ 6
		vperm		v11,v11,v12,v0				//05 15 25 35
				
		lvx			v14,r7,r8					//MSQ 6
		vmrghb		v10,v9,v11					//04 05 14 15 24 25 34 35

		lvsl		v0,r6,r8
		add			r8,r8,r4

		lvx			v15,r6,r8					//LSQ 7
		vperm		v13,v13,v14,v0				//06 16 26 36
				
		lvx			v16,r7,r8					//MSQ 7

		lvsl		v0,r6,r8
		xor			r8,r8,r8

		vperm		v15,v15,v16,v0				//07 17 27 37

		vmrghb		v14,v13,v15					//06 07 16 17 26 27 36 37

		vmrghh		v12,v10,v14					//04 05 06 07 14 15 16 17 24 25 26 27 34 35 36 37
// -- end of 4x8 transpose --	
		vxor		v6,v6,v6					//used for unpacking unsigned char

		vmrghw		v9,v2,v12					//00 01 02 03 04 05 06 07 10 11 12 13 14 15 16 17
		
		vmrglw		v10,v2,v12					//20 21 22 23 24 25 26 27 30 31 32 33 34 35 36 37

		vmrghb		v1,v6,v10					//unsigned byte -> unsigned half (PixelPtr[2])
	
		vmrghb		v2,v6,v9					//unsigned byte -> unsigned half (PixelPtr[0])
		vor			v7,v1,v1					//save for later aka PixelPtr[2]

		vadduhm		v5,v1,v1					//PixelPtr[2] * 2
		vmrglb		v3,v6,v9					//unsigned byte -> unsigned half (PixelPtr[1])

		vor			v8,v3,v3					//save for later aka PixelPtr[1]

		vadduhm		v1,v1,v5					//now have (PixelPtr[2] * 3)

		vmrglb		v4,v6,v10					//unsigned byte -> unsigned half (PixelPtr[3])
		vadduhm		v5,v3,v3					//PixelPtr[1] * 2

		vadduhm		v3,v3,v5					//now have (PixelPtr[1] * 3)
		vspltish	v0,3
		
		vsubuhm		v2,v2,v3					//PixelPtr[0] - (PixelPtr[1] * 3)
		vspltish	v5,4
		
		vadduhm		v1,v1,v2					//PixelPtr[0] - (PixelPtr[1] * 3) + (PixelPtr[2] * 3)

		vsubuhm		v1,v1,v4					//PixelPtr[0] - (PixelPtr[1] * 3) + (PixelPtr[2] * 3) - PixelPtr[3]
		
		vadduhm		v1,v1,v5					//(FiltVal + 4)
		vspltish	v5,15
				
		vsrah		v1,v1,v0					//(FiltVal + 4) >> 3
		
		vsrah		v2,v1,v5					//all ones or all zeros	-- keep for abs and final sign	
		
		vxor		v1,v1,v2
		xor			r0,r0,r0
		
		lvx			v0,r5,r0					//get FLimit
		vsubuhm		v1,v1,v2					//subtract -1 or 0 -- now have abs(i)
		
		vsubuhm		v3,v0,v1					//limit - abs(i)
				
		vsrah		v1,v3,v5					//all ones or all zeros		

		vxor		v3,v3,v1

		vsubuhm		v3,v3,v1					//subtract -1 or 0 -- now have abs(limit - abs(i))
		
		vsubuhs		v3,v0,v3					//SAT(limit - abs(limit - abs(i)))
		
		vxor		v3,v3,v2
		
		vsubuhm		v3,v3,v2					//aka FLimit
		addi		r6,r3,1						//work ptr
				
		vadduhm		v8,v8,v3
		addi		r7,r3,1						//work ptr 2
		
		vsubuhm		v7,v7,v3
		xor			r8,r8,r8
		addi		r5,r5,32					//point to temp work space

		vpkshus		v0,v8,v8
		addi		r3,r3,1						//PixelPtr[1] column
		
		vpkshus		v1,v7,v7
		addi		r6,r3,1						//PixelPtr[2] column
		
		stvewx		v0,r5,r8					//write to temp work space
		addi		r8,r8,4
		
		stvewx		v0,r5,r8
		addi		r8,r8,4

		stvewx		v1,r5,r8
		addi		r8,r8,4

		stvewx		v1,r5,r8
		xor			r7,r7,r7
// -- write back --	

//fix following code... kinda lame
		lwbrx		r8,r5,r7
		addi		r7,r7,4

		lwbrx		r9,r5,r7
		addi		r7,r7,4

		lwbrx		r10,r5,r7
		addi		r7,r7,4

		lwbrx		r11,r5,r7
//		addi		r7,r7,4

		stb			r8,0(r3)					//PixelPtr[1] -- row 0
		srwi		r8,r8,8
		add			r3,r3,r4
		
		stb			r10,0(r6)					//Pixleptr[2] -- row 0
		srwi		r10,r10,8
		add			r6,r6,r4
					
		stb			r8,0(r3)					//PixelPtr[1] -- row 1
		srwi		r8,r8,8
		add			r3,r3,r4
		
		stb			r10,0(r6)					//Pixleptr[2] -- row 1
		srwi		r10,r10,8
		add			r6,r6,r4

		stb			r8,0(r3)					//PixelPtr[1] -- row 2
		srwi		r8,r8,8
		add			r3,r3,r4
		
		stb			r10,0(r6)					//Pixleptr[2] -- row 2
		srwi		r10,r10,8
		add			r6,r6,r4

		stb			r8,0(r3)					//PixelPtr[1] -- row 3
		srwi		r8,r8,8
		add			r3,r3,r4
		
		stb			r10,0(r6)					//Pixleptr[2] -- row 3
//		srwi		r10,r10,8
		add			r6,r6,r4

		stb			r9,0(r3)					//PixelPtr[1] -- row 4
		srwi		r9,r9,8
		add			r3,r3,r4
		
		stb			r11,0(r6)					//Pixleptr[2] -- row 4
		srwi		r11,r11,8
		add			r6,r6,r4

		stb			r9,0(r3)					//PixelPtr[1] -- row 5
		srwi		r9,r9,8
		add			r3,r3,r4
		
		stb			r11,0(r6)					//Pixleptr[2] -- row 5
		srwi		r11,r11,8
		add			r6,r6,r4

		stb			r9,0(r3)					//PixelPtr[1] -- row 6
		srwi		r9,r9,8
		add			r3,r3,r4
		
		stb			r11,0(r6)					//Pixleptr[2] -- row 6
		srwi		r11,r11,8
		add			r6,r6,r4

		stb			r9,0(r3)					//PixelPtr[1] -- row 7
		srwi		r9,r9,8
		add			r3,r3,r4
		
		stb			r11,0(r6)					//Pixleptr[2] -- row 7
//		srwi		r11,r11,8
//		add			r6,r6,r4


	}
}
/****************************************************************************
 * 
 *  ROUTINE       :     FilterVert_Altivec
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to a horizontal edge vertically
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
 										/*			r3					r4						r5  */
void FilterVert_Altivec(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr)
{

//NOTE: (SAT(limit - abs(limit - abs(i))) * (-1 or 1))

	//NOTE: compiler uses r12 for vrsave
	asm
	{
		mr			r3,r4
		mr			r4,r5
		mr			r5,r6



		xor			r6,r6,r6					//current line
		xor			r9,r9,r9
		
		mr			r7,r4						//-2*LineLength
		addi		r9,r9,-1
		
		mr			r8,r4						//-1*LineLength
		slwi		r7,r7,1
		
		vxor		v6,v6,v6					//used for unpacking unsigned char
		xor			r7,r7,r9
		xor			r8,r8,r9
		
		lvx			v1,r3,r6					//get 16 bytes -- only interested in 8
		addi		r7,r7,1						//-2*LineLength

		lvsl		v0,r3,r6					//load the alignment vector
		addi		r8,r8,1						//-1*LineLength

		lvx			v2,r3,r7					//get 16 bytes -- only interested in 8
		vperm		v1,v1,v1,v0

		lvsl		v0,r3,r7					//load the alignment vector
		vmrghb		v1,v6,v1					//unsigned byte -> unsigned half
	
		lvx			v3,r3,r8					//get 16 bytes -- only interested in 8
		vperm		v2,v2,v2,v0

		lvsl		v0,r3,r8					//load the alignment vector
		vmrghb		v2,v6,v2					//unsigned byte -> unsigned half

		lvx			v4,r3,r4					//get 16 bytes -- only interested in 8
		vperm		v3,v3,v3,v0
		vor			v7,v1,v1					//save for later aka PixelPtr[2]

		vadduhm		v5,v1,v1					//PixelPtr[2] * 2
		vmrghb		v3,v6,v3					//unsigned byte -> unsigned half

		lvsl		v0,r3,r4					//load the alignment vector
		vor			v8,v3,v3					//save for later aka PixelPtr[1]

		vperm		v4,v4,v4,v0
		vadduhm		v1,v1,v5					//now have (PixelPtr[2] * 3)

		vmrghb		v4,v6,v4					//unsigned byte -> unsigned half
		vadduhm		v5,v3,v3					//PixelPtr[1] * 2

		vadduhm		v3,v3,v5					//now have (PixelPtr[1] * 3)
		vspltish	v0,3
		
		vsubuhm		v2,v2,v3					//PixelPtr[0] - (PixelPtr[1] * 3)
		vspltish	v5,4
		
		vadduhm		v1,v1,v2					//PixelPtr[0] - (PixelPtr[1] * 3) + (PixelPtr[2] * 3)

		vsubuhm		v1,v1,v4					//PixelPtr[0] - (PixelPtr[1] * 3) + (PixelPtr[2] * 3) - PixelPtr[3]
		
		vadduhm		v1,v1,v5					//(FiltVal + 4)
		vspltish	v5,15
				
		vsrah		v1,v1,v0					//(FiltVal + 4) >> 3
		
		vsrah		v2,v1,v5					//all ones or all zeros	-- keep for abs and final sign	
		
		vxor		v1,v1,v2
		xor			r0,r0,r0
		
		lvx			v0,r5,r0					//get FLimit
		vsubuhm		v1,v1,v2					//subtract -1 or 0 -- now have abs(i)
		
		vsubuhm		v3,v0,v1					//limit - abs(i)
				
		vsrah		v1,v3,v5					//all ones or all zeros		

		vxor		v3,v3,v1

		vsubuhm		v3,v3,v1					//subtract -1 or 0 -- now have abs(limit - abs(i))
		
		vsubuhs		v3,v0,v3					//SAT(limit - abs(limit - abs(i)))
		
		vxor		v3,v3,v2
		
		vsubuhm		v3,v3,v2					//aka FLimit
		
		vadduhm		v8,v8,v3
		
		vsubuhm		v7,v7,v3
		
//		vpkshss		v0,v8,v8
//		vpkshss		v1,v7,v7

		lvsr		v2,r3,r8
		vpkshus		v0,v8,v8
		
		lvsr		v3,r3,r6
		vpkshus		v1,v7,v7

		vperm		v0,v0,v0,v2
		
		stvewx		v0,r3,r8
		addi		r8,r8,4
		
		stvewx		v0,r3,r8
		vperm		v1,v1,v1,v3

		stvewx		v1,r3,r6
		addi		r6,r6,4

		stvewx		v1,r3,r6
	}
}

/*
	=================================================	
*/

#if 1
/****************************************************************************
 * 
 *  ROUTINE       :     SetupBoundingValueArray_ForPPC
 *
 *  INPUTS        :      
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to the edge pixels of coded blocks.
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
INT32 *SetupBoundingValueArray_ForPPC(PB_INSTANCE *pbi, INT32 FLimit)
{
    INT32 * BoundingValuePtr;
    INT32 i;

    /* 
        Since the FiltBoundingValue array is currently only used in the generic version, we are going
        to reuse this memory for our own purposes.
    */
   	BoundingValuePtr = (INT32 *)((UINT32)(&pbi->FiltBoundingValue[256]) & 0xffffffe0);    
	
	/* zero constant for fp conversion */
    BoundingValuePtr[0] = 0x43300000; 
    BoundingValuePtr[1] = 0x80000000;
    
	/* work area for fp conversion*/
    BoundingValuePtr[2] = 0x43300000; 
    BoundingValuePtr[3] = 0x00000000;

    BoundingValuePtr[4] = 0x0; 
    BoundingValuePtr[5] = 0x0;
    
    ((double *)BoundingValuePtr)[3] = (float) FLimit;
    ((double *)BoundingValuePtr)[4] = (float) 1;
    ((double *)BoundingValuePtr)[5] = (float) -1;

    return BoundingValuePtr;
}

#define ZERO 0
#define TEMP 8
#define FILTVAL 16

/****************************************************************************
 * 
 *  ROUTINE       :     FilterHoriz_PPC
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to the vertical edge horizontally
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
#if 1
 				/*         r3,               r4,               r5,                      r6    */		
void FilterHoriz_PPC(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr)
{

	(void) pbi;

// Bounding value formula
// sat( limit - abs(limit - abs(i)) ) * (1 or -1)

	 
	asm
	{
		lfd		fp0,24(r6)				;//get FLimit
//		li		r7,8					;//loop counter
		li		r7,4					;//loop counter
		addi	r12,r6,32				;//ptr to 1 or -1 fp values
		

		lfd		fp1,ZERO(r6)			;//load zero constant
		fsub	fp8,fp8,fp8
		

FilterHoriz_PPC_loop:

		lbz		r8,1(r4)				;//PixelPtr[1]
		add		r29,r4,r5				;//ptr to line below
//mr r29,r4		
		
		lbz		r9,2(r4)				;//PixelPtr[2]
		slwi	r31,r8,1
	
		lbz		r10,0(r4)				;//PixelPtr[0]
		add		r8,r31,r8				;//PixelPtr[1] * 3
		slwi	r30,r9,1

		lbz		r11,3(r4)				;//PixelPtr[3]
		add		r9,r30,r9				;//PixelPtr[2] * 3

		lbz		r28,1(r29)				;//PixelPtr[1 + LineLength]
		subf    r8,r8,r10			
		subf	r9,r11,r9
		
		lbz		r27,2(r29)				;//PixelPtr[2 + LineLength]
		add		r8,r8,r9				;//FiltVal
		slwi	r31,r28,1

		lbz		r26,0(r29)				;//PixelPtr[0 + LineLength]
		addi	r8,r8,4	
		add		r28,r28,r31				;//PixelPtr[1 + LineLength] * 3

		lbz		r25,3(r29)				;//PixelPtr[3 + LineLength]
		srawi	r8,r8,3					;//(FiltVal + 4) >> 3
		slwi	r30,r27,1

		srwi	r9,r8,31				;//save sign bit
		xoris	r8,r8,0x8000			;//invert sign bit for float conversion
		
		stw		r8,TEMP+4(r6)
		slwi	r9,r9,3					;//8 byte increments
		add		r27,r27,r30				;//PixelPtr[2 + LineLength] * 3

		lfdx	fp2,r12,r9				;//now have 1 or -1
		subf    r28,r28,r26			
		subf	r27,r25,r27
		
		lfd		fp3,TEMP(r6)
		add		r28,r28,r27				;//FiltVal #2

		fsub	fp3,fp3,fp1				;//(double) FiltVal
		addi	r28,r28,4	

		fabs	fp3,fp3					;//abs(i)
		srawi	r28,r28,3					;//(FiltVal + 4) >> 3

		fsub	fp3,fp0,fp3				;//(limit - abs(i))
		srwi	r27,r28,31				;//save sign bit #2
		xoris	r28,r28,0x8000			;//invert sign bit for float conversion #2

		stw		r28,TEMP+4(r6)
		slwi	r27,r27,3				;//8 byte increments
		fabs	fp3,fp3					;//abs(limit - abs(i))
		
		lfdx	fp4,r12,r27				;//now have 1 or -1
		fsub	fp3,fp0,fp3				;//limit - abs(limit - abs(i))

		lfd		fp5,TEMP(r6)
		fcmpo	fp3,fp8
		bgt+ 	L0

		fsub	fp2,fp2,fp2				;//used to saturate at 0
//branch and skip
//branch and skip
//branch and skip
//branch and skip
//branch and skip
//branch and skip
				
		
L0:		

		fsub	fp5,fp5,fp1				;//(double) FiltVal #2

		lbz		r10,1(r4)				;//PixelPtr[1]
		fmul	fp3,fp3,fp2				;//mult sign back in
		
		lbz		r11,2(r4)				;//PixelPtr[2]
		fctiw	fp3,fp3
		
		stfd	fp3,FILTVAL(r6)
		fabs	fp5,fp5					;//abs(i) #2
		

		lwz		r8,FILTVAL+4(r6)		;//new FiltVal	
		fsub	fp5,fp0,fp5				;//(limit - abs(i)) #2


		lbz		r25,1(r29)				;//PixelPtr[1 + LineLength]
		fabs	fp5,fp5					;//abs(limit - abs(i)) #2
		add		r10,r8,r10
		subf	r11,r8,r11

		lbz		r26,2(r29)				;//PixelPtr[2 + LineLength]
		fsub	fp5,fp0,fp5				;//limit - abs(limit - abs(i)) #2
		andi.	r0,r10,0xff00
		beq+		L1
		
		srawi	r0,r10,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r10,r0,0xff   			;//now have 00 or ff

L1:
		andi.	r0,r11,0xff00
		beq+		L2
		
		srawi	r0,r11,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r11,r0,0xff   			;//now have 00 or ff

L2:
		stb		r10,1(r4)
		fcmpo	fp5,fp8
		bgt+ 	L3

		fsub	fp4,fp4,fp4				;//used to saturate at 0 #2
//branch and skip		
		
L3:		
		stb		r11,2(r4)
		fmul	fp5,fp5,fp4				;//mult sign back in
		
		fctiw	fp5,fp5
		
		stfd	fp5,FILTVAL(r6)

		lwz		r28,FILTVAL+4(r6)		;//new FiltVal #2	
		li		r10,-1

		add		r25,r28,r25
		subf	r26,r28,r26


		andi.	r0,r25,0xff00			;//start clamping #2
		beq+		L4
		
		srawi	r0,r25,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r25,r0,0xff   			;//now have 00 or ff

L4:
		andi.	r0,r26,0xff00
		beq+		L5
		
		srawi	r0,r26,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r26,r0,0xff   			;//now have 00 or ff

L5:
		stb		r25,1(r29)
		add.	r7,r7,r10
		add		r4,r4,r5

		stb		r26,2(r29)
		add		r4,r4,r5
		bne		FilterHoriz_PPC_loop

	}


}

#else
 				/*         r3,               r4,               r5,                      r6    */		
void FilterHoriz_PPC(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr)
{

	(void) pbi;

// Bounding value formula
// sat( limit - abs(limit - abs(i)) ) * (1 or -1)

	 
	asm
	{
		lfd		fp0,24(r6)			//get FLimit
		li		r7,8				//loop counter
		addi	r12,r6,32			//ptr to 1 or -1 fp values
		

		lfd		fp1,ZERO(r6)		//load zero constant
		fsub	fp8,fp8,fp8
		

FilterHoriz_PPC_loop:

		lbz		r8,1(r4)			//PixelPtr[1]
		;
		;	
		
		lbz		r9,2(r4)			//PixelPtr[2]
		slwi	r31,r8,1
	
		lbz		r10,0(r4)			//PixelPtr[0]
		add		r8,r31,r8			//PixelPtr[1] * 3
		slwi	r30,r9,1

		lbz		r11,3(r4)			//PixelPtr[3]
		add		r9,r30,r9			//PixelPtr[2] * 3

		subf    r8,r8,r10			
		subf	r9,r11,r9
		
		add		r8,r8,r9			//FiltVal
		;	

		addi	r8,r8,4	
		;

		srawi	r8,r8,3				//(FiltVal + 4) >> 3
		;

		srwi	r9,r8,31			//save sign bit
		xoris	r8,r8,0x8000		//invert sign bit
		
		stw		r8,TEMP+4(r6)
		slwi	r9,r9,3				//8 byte increments



		lfdx	fp2,r12,r9			//now have 1 or -1
		

		lfd		fp3,TEMP(r6)
		

		fsub	fp3,fp3,fp1			//(double) FiltVal


		fabs	fp3,fp3				//abs(i)
		
		fsub	fp3,fp0,fp3			//(limit - abs(i))

		fabs	fp3,fp3				//abs(limit - abs(i))
		
		fsub	fp3,fp0,fp3			//limit - abs(limit - abs(i))


		fcmpo	fp3,fp8
		bgt+ 	L0

		fsub	fp2,fp2,fp2			//used to saturate at 0
		
		
L0:		
		lbz		r10,1(r4)			//PixelPtr[1]
		fmul	fp3,fp3,fp2			//mult sign back in
		
		lbz		r11,2(r4)			//PixelPtr[2]
		fctiw	fp3,fp3
		
		stfd	fp3,FILTVAL(r6)
		

		lwz		r8,FILTVAL+4(r6)	//new FiltVal	


		add		r10,r8,r10
		subf	r11,r8,r11

		andi.	r0,r10,0xff00
		beq+		L1
		
		srawi	r0,r10,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r10,r0,0xff   			;//now have 00 or ff

L1:
		andi.	r0,r11,0xff00
		beq+		L2
		
		srawi	r0,r11,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r11,r0,0xff   			;//now have 00 or ff

L2:
		stb		r10,1(r4)
		li		r0,-1
		
		stb		r11,2(r4)
		add		r4,r4,r5
		
		add.	r7,r7,r0
		bne		FilterHoriz_PPC_loop

	}
}
#endif
/****************************************************************************
 * 
 *  ROUTINE       :     FilterVert_PPC
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Applies a loop filter to a horizontal edge vertically
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void FilterVert_PPC(PB_INSTANCE *pbi, UINT8 * PixelPtr, INT32 LineLength, INT32 *BoundingValuePtr)
{

}

#endif


