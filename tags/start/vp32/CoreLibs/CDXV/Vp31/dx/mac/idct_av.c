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
*   Module Title :     idct_av.c
*
*   Description  :     IDCT using Altivec instructions
*
*
*****************************************************************************
*/
#include "pbdll.h"
#include <stdio.h>


//typedef double cdouble;
//typedef int int32;
//typedef short int16;




//static 
vector signed short idctConst;

//static 
vector unsigned char vPerm1;

//PPfunctions_av.c uses this
vector unsigned char vPerm2;
		
//static 
vector unsigned char vPerm3;


void initIdctConstants_Altivec(void)
{
		idctConst =
		(vector signed short) (0, 64277, 60547, 54491, 46341, 36410, 25080, 12785);

		vPerm1 =
		(vector unsigned char) (0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23);

		vPerm2 =
		(vector unsigned char) (8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29, 30, 31);
		
		vPerm3 =
		(vector unsigned char) (0, 1, 16, 17, 4, 5, 20, 21, 8, 9, 24, 25, 12, 13, 28, 29);
}

/* 
   Our 1-D idct expansion uses constants C1 ... C7 given by

   	(*)  Ck = C(-k) = cos( pi * k/16) = S(8-k) = -S(k-8) = sin( pi * (8-k)/16) 

   and the following 1-D algorithm transforming I0 ... I7  to  R0 ... R7 :
  
   A = (C1 * I1) + (C7 * I7)		B = (C7 * I1) - (C1 * I7)
   C = (C3 * I3) + (C5 * I5)		D = (C3 * I5) - (C5 * I3)
   A. = C4 * (A - C)				B. = C4 * (B - D)
   C. = A + C						D. = B + D
   
   E = C4 * (I0 + I4)				F = C4 * (I0 - I4)
   G = (C2 * I2) + (C6 * I6)		H = (C6 * I2) - (C2 * I6)
   E. = E - G
   G. = E + G
   
   A.. = F + A.					B.. = B. - H
   F.  = F - A. 				H.  = B. + H
   
   R0 = G. + C.	R1 = A.. + H.	R3 = E. + D.	R5 = F. + B..
   R7 = G. - C.	R2 = A.. - H.	R4 = E. - D.	R6 = F. - B..

   It is due to Vetterli and Lightenberg and may be found in the JPEG
   reference book by Pennebaker and Mitchell.

   Correctness of the algorithm follows from (*) together with the
   addition formulas for sine and cosine:

	cos( A + B) = cos( A) * cos( B)  -  sin( A) * sin( B)
	sin( A + B) = sin( A) * cos( B)  +  cos( A) * sin( B)

   Note that this implementation absorbs the difference in normalization
   between the 0th and higher frequencies, although the results produced
   are actually twice as big as they should be.  Since we do this for each
   dimension, the 2-D idct results are 4x the desired results.  Finally,
   taking into account that the dequantization multiplies by 4 as well,
   our actual results are 16x too big.  We fix this by shifting the final
   results right by 4 bits.

   High precision version approximates C1 ... C7 to 16 bits.
   Since MMX only provides a signed multiply, C1 ... C5 appear to be
   negative and multiplies involving them must be adjusted to compensate
   for this.  C6 and C7 do not require this adjustment since
   they are < 1/2 and are correctly treated as positive numbers.
   
*/
   
/****************************************************************************/
					/*      r3,                 r4,                 r5 */
void IdctAltivec_DX(int16 * InputData, int16 *QuantMatrix, int16 * OutputData)
{

	/* it is assumed that the coeffs have already been transposed */	

	asm
	{
	    lwz         r9,idctConst
	    xor			r7,r7,r7
	   	xor			r8,r8,r8
	   	
//trying cache hints
		lis			r8,0x1001				//Block Size = 16, Block Count = 1, Block Stride = 0
		dstst		r5,r8,0	  
		dst 		r4,r8,1			
		dst	    	r3,r8,2	
		
		lvx			v8,r9,r7
		xor			r8,r8,r8
			
		lvx			v20,r4,r8
		vsplth		v0,v8,0										
		addi		r8,r8,16

		lvx			v21,r4,r8
		vsplth		v1,v8,1										
		addi		r8,r8,16

		lvx			v22,r4,r8
		vsplth		v2,v8,2										
		addi		r8,r8,16

		lvx			v23,r4,r8
		vsplth		v3,v8,3										
		addi		r8,r8,16

		lvx			v24,r4,r8
		vsplth		v4,v8,4										
		addi		r8,r8,16

		lvx			v25,r4,r8
		vsplth		v5,v8,5										
		addi		r8,r8,16

		lvx			v26,r4,r8
		vsplth		v6,v8,6										
		addi		r8,r8,16

		lvx			v27,r4,r8
		vsplth		v7,v8,7										

		lvx			v10,r3,r7
		addi		r7,r7,16
	
		lvx			v11,r3,r7
		addi		r7,r7,16
	
		lvx			v12,r3,r7
		vmladduhm	v10,v10,v20,v0				//dequant
		addi		r7,r7,16
	
		lvx			v13,r3,r7
		vmladduhm	v11,v11,v21,v0				//dequant
		addi		r7,r7,16
	
		lvx			v14,r3,r7
		vmladduhm	v12,v12,v22,v0				//dequant
		addi		r7,r7,16
	
		lvx			v15,r3,r7
		vmladduhm	v13,v13,v23,v0				//dequant
		addi		r7,r7,16
	
		lvx			v16,r3,r7
		vmladduhm	v14,v14,v24,v0				//dequant
		addi		r7,r7,16
	
		lvx			v17,r3,r7
		vmladduhm	v15,v15,v25,v0				//dequant

	    lwz         r10,vPerm1
		vmladduhm	v16,v16,v26,v0				//dequant
		xor			r7,r7,r7
		
	    lwz         r8,vPerm2

	    lwz         r9,vPerm3

	    lvx			v30,r10,r7
		vmladduhm	v17,v17,v27,v0				//dequant
												
	    lvx			v31,r8,r7

	    lvx			v29,r9,r7
												//order on entry	
												//00 10 20 30 40 50 60 70
												//01 11 21 31 41 51 61 71
												//02 12 22 32 42 52 62 72
												//03 13 23 33 43 53 63 73
												//04 14 24 34 44 54 64 74
												//05 15 25 35 45 55 65 75
												//06 16 26 36 46 56 66 76
												//07 17 27 37 47 57 67 77

//start of row idct
		vmulesh		v21,v11,v1
		
		vmulosh		v28,v11,v1
		
		vmulesh		v27,v17,v7
		vperm		v21,v21,v28,v29				//(c1 * i1) - i1
		
		vmulosh		v28,v17,v7	
		
		vaddshs		v21,v21,v11					//(c1 * i1)
		vperm		v27,v27,v28,v29				//(c7 * i7)
		
		vaddshs		v8,v21,v27					//A

		vmulesh		v23,v13,v3
		
		vmulosh		v28,v13,v3
		
		vmulesh		v25,v15,v5
		vperm		v23,v23,v28,v29				//(c3 * i3) - i3
		
		vmulosh		v28,v15,v5
		
		vaddshs		v23,v23,v13					//(c3 * i3)
		vperm		v25,v25,v28,v29				//(c5 * i5) - i5
		
		vaddshs		v25,v25,v15					//(c5 * i5)
		
		vaddshs		v9,v23,v25					//C
		
		vaddshs		v18,v8,v9					//C. = A + C
		
		vsubshs		v8,v8,v9					//(A - C)
		
		vmulesh		v9,v8,v4
		
		vmulosh		v28,v8,v4
		
		vmulesh		v21,v11,v7
		vperm		v9,v9,v28,v29				//(c4 * (A - C)) - (A - C)
		
		vaddshs		v9,v9,v8					//A. = c4 * (A - C)
//---
		
		vmulosh		v28,v11,v7
		
		vmulesh		v27,v17,v1
		vperm		v21,v21,v28,v29				//(c7 * i1)
		
		vmulosh		v28,v17,v1
		
		vmulesh		v25,v15,v3
		vperm		v27,v27,v28,v29				//(c1 * i7) - i7
		
		vaddshs		v27,v27,v17					//(c1 * i7)
		
		vsubshs		v21,v21,v27					//B

		vmulosh		v28,v15,v3
		
		vmulesh		v23,v13,v5
		vperm		v25,v25,v28,v29				//(c3 * i5) - i5

		vaddshs		v25,v25,v15					//(c3 * i5)
		
		vmulosh		v28,v13,v5
		
		vperm		v23,v23,v28,v29				//(c5 * i3) - i3
		
		vaddshs		v23,v23,v13					//(c5 * i3)

		vsubshs		v23,v25,v23					//D
		
		vaddshs		v19,v21,v23					//D. = B + D
		
		vsubshs		v21,v21,v23					//(B - D)
		
		vmulesh		v25,v21,v4
 		
		vmulosh		v28,v21,v4
		
		vperm		v25,v25,v28,v29				//(c4 * (B - D)) - (B - D)
		
		vaddshs		v21,v25,v21					//B. = c4 * (B - D)
//---
		vaddshs		v25,v10,v14					//(i0 + i4)
		
		vmulesh		v23,v25,v4
		
		vmulosh		v28,v25,v4
		
		vperm		v23,v23,v28,v29				//(c4 * (i0 + i4)) - (i0 + i4)
		
		vaddshs		v25,v23,v25					//E = c4 * (i0 + i4)
		
		vsubshs		v27,v10,v14					//(i0 - i4)
		
		vmulesh		v23,v27,v4
		
		vmulosh		v28,v27,v4
		
		vmulesh		v22,v12,v2
		vperm		v23,v23,v28,v29				//(c4 * (i0 - i4)) - (i0 - i4)

		vaddshs		v27,v23,v27					//F = c4 * (i0 - i4)
//---
		
		vmulosh		v28,v12,v2
		
		vmulesh		v26,v16,v6
		vperm		v22,v22,v28,v29				//(c2 * i2) - i2
		
		vaddshs		v22,v22,v12					//(c2 * i2)

		vmulosh		v28,v16,v6
		
		vperm		v26,v26,v28,v29				//(c6 * i6)		

		vaddshs		v24,v22,v26					//G
		
		vmulesh		v22,v12,v6
		
		vmulosh		v28,v12,v6
		
		vmulesh		v26,v16,v2
		vperm		v22,v22,v28,v29				//(c6 * i2)

		vmulosh		v28,v16,v2
		
		vperm		v26,v26,v28,v29				//(c2 * i6) - i6
		vsubshs		v13,v25,v24					//E. = E - G
		
		vaddshs		v26,v26,v16					//(c2 * i6)
		
		vsubshs		v22,v22,v26					//H

//---
		vaddshs		v10,v25,v24					//G. = E + G
		
		vaddshs		v11,v27,v9					//A.. = F + A.
		
		vsubshs		v15,v21,v22					//B.. = B. - H
		
		vsubshs		v27,v27,v9					//F. = F - A.
		
		vaddshs		v21,v21,v22					//H. = B. + H
//---
		vsubshs		v17,v10,v18					//R7 = G. - C.
		
		vaddshs		v10,v10,v18					//R0 = G. + C.
		
		vsubshs		v12,v11,v21					//R2 = A.. - H.
		
		vaddshs		v11,v11,v21					//R1 = A.. + H.
		
		vsubshs		v14,v13,v19					//R4 = E. - D.
		
		vaddshs		v13,v13,v19					//R3 = E. + D.
		
		vsubshs		v16,v27,v15					//R6 = F. - B..
		
		vaddshs		v15,v15,v27					//R5 = F. + B..	
//end of row idct		

//start of transpose
		vmrghh		v18,v10,v11					//00 01 10 11 20 21 30 31 
		vmrglh		v19,v10,v11					//40 41 50 51 60 61 70 71
		vmrghh		v20,v12,v13					//02 03 12 13 22 23 32 33
		vmrglh		v21,v12,v13					//42 43 52 53 62 63 72 73
		vmrghh		v22,v14,v15					//04 05 14 15 24 25 34 35
		vmrglh		v23,v14,v15					//44 45 54 55 64 65 74 75
		vmrghh		v24,v16,v17					//06 07 16 17 26 27 36 37
		vmrglh		v25,v16,v17					//46 47 56 57 66 67 76 77
		
		vmrghw		v8,v18,v20					//00 01 02 03 10 11 12 13
		vmrghw		v9,v22,v24					//04 05 06 07 14 15 16 17
		vmrghw		v26,v19,v21					//40 41 42 43 50 51 52 53
		vmrghw		v27,v23,v25					//44 45 46 47 54 55 56 57
		vmrglw		v18,v18,v20					//20 21 22 23 30 31 32 33
		vmrglw		v22,v22,v24					//24 25 26 27 34 35 36 37
		vmrglw		v19,v19,v21					//60 61 62 63 70 71 72 73
		vmrglw		v23,v23,v25					//64 65 66 67 74 75 76 77
		
		vperm		v10,v8,v9,v30				//00 01 02 03 04 05 06 07
		vperm		v11,v8,v9,v31				//10 11 12 13 14 15 16 17
		vperm		v12,v18,v22,v30				//20 21 22 23 24 25 26 27
		vperm		v13,v18,v22,v31				//30 31 32 33 34 35 36 37
		vperm		v14,v26,v27,v30				//40 41 42 43 44 45 46 47
		vperm		v15,v26,v27,v31				//50 51 52 53 54 55 56 57 
		vperm		v16,v19,v23,v30				//60 61 62 63 64 65 66 67
		vperm		v17,v19,v23,v31				//70 71 72 73 74 75 76 77
//end of transpose		
		

//start of col idct
		vmulesh		v21,v11,v1
		
		vmulosh		v28,v11,v1
		
		vmulesh		v27,v17,v7
		vperm		v21,v21,v28,v29				//(c1 * i1) - i1
		
		vmulosh		v28,v17,v7	
		
		vaddshs		v21,v21,v11					//(c1 * i1)
		vperm		v27,v27,v28,v29				//(c7 * i7)
		
		vaddshs		v8,v21,v27					//A

		vmulesh		v23,v13,v3
		
		vmulosh		v28,v13,v3
		
		vmulesh		v25,v15,v5
		vperm		v23,v23,v28,v29				//(c3 * i3) - i3
		
		vmulosh		v28,v15,v5
		
		vaddshs		v23,v23,v13					//(c3 * i3)
		vperm		v25,v25,v28,v29				//(c5 * i5) - i5
		
		vaddshs		v25,v25,v15					//(c5 * i5)
		
		vaddshs		v9,v23,v25					//C
		
		vaddshs		v18,v8,v9					//C. = A + C
		
		vsubshs		v8,v8,v9					//(A - C)
		
		vmulesh		v9,v8,v4
		
		vmulosh		v28,v8,v4
		
		vmulesh		v21,v11,v7
		vperm		v9,v9,v28,v29				//(c4 * (A - C)) - (A - C)
		
		vaddshs		v9,v9,v8					//A. = c4 * (A - C)
//---
		
		vmulosh		v28,v11,v7
		
		vmulesh		v27,v17,v1
		vperm		v21,v21,v28,v29				//(c7 * i1)
		
		vmulosh		v28,v17,v1
		
		vmulesh		v25,v15,v3
		vperm		v27,v27,v28,v29				//(c1 * i7) - i7
		
		vaddshs		v27,v27,v17					//(c1 * i7)
		
		vsubshs		v21,v21,v27					//B

		vmulosh		v28,v15,v3
		
		vmulesh		v23,v13,v5
		vperm		v25,v25,v28,v29				//(c3 * i5) - i5

		vaddshs		v25,v25,v15					//(c3 * i5)
		
		vmulosh		v28,v13,v5
		
		vperm		v23,v23,v28,v29				//(c5 * i3) - i3
		
		vaddshs		v23,v23,v13					//(c5 * i3)

		vsubshs		v23,v25,v23					//D
		
		vaddshs		v19,v21,v23					//D. = B + D
		
		vsubshs		v21,v21,v23					//(B - D)
		
		vmulesh		v25,v21,v4
 		
		vmulosh		v28,v21,v4
		
		vperm		v25,v25,v28,v29				//(c4 * (B - D)) - (B - D)
		
		vaddshs		v21,v25,v21					//B. = c4 * (B - D)
//---
		vaddshs		v25,v10,v14					//(i0 + i4)
		
		vmulesh		v23,v25,v4
		
		vmulosh		v28,v25,v4
		
		vperm		v23,v23,v28,v29				//(c4 * (i0 + i4)) - (i0 + i4)
		
		vaddshs		v25,v23,v25					//E = c4 * (i0 + i4)
		
		vsubshs		v27,v10,v14					//(i0 - i4)
		
		vmulesh		v23,v27,v4
		
		vmulosh		v28,v27,v4
		
		vmulesh		v22,v12,v2
		vperm		v23,v23,v28,v29				//(c4 * (i0 - i4)) - (i0 - i4)

		vaddshs		v27,v23,v27					//F = c4 * (i0 - i4)
//---
		
		vmulosh		v28,v12,v2
		
		vmulesh		v26,v16,v6
		vperm		v22,v22,v28,v29				//(c2 * i2) - i2
		
		vaddshs		v22,v22,v12					//(c2 * i2)

		vmulosh		v28,v16,v6
		
		vperm		v26,v26,v28,v29				//(c6 * i6)		

		vaddshs		v24,v22,v26					//G
		
		vmulesh		v22,v12,v6
		
		vmulosh		v28,v12,v6
		
		vmulesh		v26,v16,v2
		vperm		v22,v22,v28,v29				//(c6 * i2)

		vmulosh		v28,v16,v2
		
		vperm		v26,v26,v28,v29				//(c2 * i6) - i6
		vsubshs		v13,v25,v24					//E. = E - G
		
		vaddshs		v26,v26,v16					//(c2 * i6)
		
		vsubshs		v22,v22,v26					//H

//---
		vaddshs		v10,v25,v24					//G. = E + G
		
		vaddshs		v11,v27,v9					//A.. = F + A.
		
		vsubshs		v15,v21,v22					//B.. = B. - H
		
		vsubshs		v27,v27,v9					//F. = F - A.
		
		vaddshs		v21,v21,v22					//H. = B. + H
//---
		vsubshs		v17,v10,v18					//R7 = G. - C.
		
		vaddshs		v10,v10,v18					//R0 = G. + C.
		
		vsubshs		v12,v11,v21					//R2 = A.. - H.
		
		vaddshs		v11,v11,v21					//R1 = A.. + H.
		
		vsubshs		v14,v13,v19					//R4 = E. - D.
		
		vaddshs		v13,v13,v19					//R3 = E. + D.
		
		vsubshs		v16,v27,v15					//R6 = F. - B..
		
		vaddshs		v15,v15,v27					//R5 = F. + B..	
//end of col idct
		xor			r8,r8,r8
		vspltish	v0,8

		vaddshs		v10,v10,v0					//IdctAdjustBeforeShift 
		vaddshs		v11,v11,v0					//IdctAdjustBeforeShift 
		vaddshs		v12,v12,v0					//IdctAdjustBeforeShift 
		vaddshs		v13,v13,v0					//IdctAdjustBeforeShift 
		vaddshs		v14,v14,v0					//IdctAdjustBeforeShift 
		vaddshs		v15,v15,v0					//IdctAdjustBeforeShift 
		vaddshs		v16,v16,v0					//IdctAdjustBeforeShift 
		vaddshs		v17,v17,v0					//IdctAdjustBeforeShift 
		vspltish	v0,4
		
		vsrah		v10,v10,v0
		
		stvx		v10,r5,r8
		vsrah		v11,v11,v0
		addi		r8,r8,16

		stvx		v11,r5,r8
		vsrah		v12,v12,v0
		addi		r8,r8,16

		stvx		v12,r5,r8
		vsrah		v13,v13,v0
		addi		r8,r8,16

		stvx		v13,r5,r8
		vsrah		v14,v14,v0
		addi		r8,r8,16

		stvx		v14,r5,r8
		vsrah		v15,v15,v0
		addi		r8,r8,16

		stvx		v15,r5,r8
		vsrah		v16,v16,v0
		addi		r8,r8,16

		stvx		v16,r5,r8
		vsrah		v17,v17,v0
		addi		r8,r8,16

		stvx		v17,r5,r8
	}
	
}

/**************************************************************************************
 **************  Wmt_IDCT10_Dx   ******************************************************
 **************************************************************************************
 

	In IDCT10, we are dealing with only ten Non-Zero coefficients in the 8x8 block. 
	In the case that we work in the fashion RowIDCT -> ColumnIDCT, we only have to 
	do 1-D row idcts on the first four rows, the rest four rows remain zero anyway. 
	After row IDCTs, since every column could have nonzero coefficients, we need do
	eight 1-D column IDCT. However, for each column, there are at most two nonzero
	coefficients, coefficient 0 to coefficient 3. Same for the coefficents for the 
	two 1-d row idcts. For this reason, the process of a 1-D IDCT is simplified 
	
	from a full version:
	
	A = (C1 * I1) + (C7 * I7)		B = (C7 * I1) - (C1 * I7)
	C = (C3 * I3) + (C5 * I5)		D = (C3 * I5) - (C5 * I3)
	A. = C4 * (A - C)				B. = C4 * (B - D)
    C. = A + C						D. = B + D
   
    E = C4 * (I0 + I4)				F = C4 * (I0 - I4)
    G = (C2 * I2) + (C6 * I6)		H = (C6 * I2) - (C2 * I6)
    E. = E - G
    G. = E + G
   
    A.. = F + A.					B.. = B. - H
    F.  = F - A. 					H.  = B. + H
   
    R0 = G. + C.	R1 = A.. + H.	R3 = E. + D.	R5 = F. + B..
    R7 = G. - C.	R2 = A.. - H.	R4 = E. - D.	R6 = F. - B..


	To:

  	A = (C1 * I1)					B = (C7 * I1) 
	C = (C3 * I3)					D = - (C5 * I3)
	A. = C4 * (A - C)				B. = C4 * (B - D)
    C. = A + C						D. = B + D
   
    E = C4 * I0						F = E
    G = (C2 * I2)					H = (C6 * I2)
    E. = E - G
    G. = E + G
   
    A.. = F + A.					B.. = B. - H
    F.  = F - A. 					H.  = B. + H
   
    R0 = G. + C.	R1 = A.. + H.	R3 = E. + D.	R5 = F. + B..
    R7 = G. - C.	R2 = A.. - H.	R4 = E. - D.	R6 = F. - B..

	
******************************************************************************************/
/****************************************************************************/
					/*      r3,                 r4,                 r5 */
void IdctAltivec10_DX(int16 * InputData, int16 *QuantMatrix, int16 * OutputData)
{

	/* it is assumed that the coeffs have already been transposed */	

	asm
	{
	    lwz         r9,idctConst
	    xor			r7,r7,r7
	   	xor			r8,r8,r8



//trying cache hints
		lis			r8,0x1001				//Block Size = 16, Block Count = 1, Block Stride = 0
		dstst		r5,r8,0	  
		dst 		r4,r8,1			
		dst	    	r3,r8,2	
		
					   	
		lvx			v8,r9,r7
	   	xor			r8,r8,r8
			
		lvx			v20,r4,r8
		vsplth		v0,v8,0										
		addi		r8,r8,16

		lvx			v21,r4,r8
		vsplth		v1,v8,1										
		addi		r8,r8,16

		lvx			v22,r4,r8
		vsplth		v2,v8,2										
		addi		r8,r8,16

		lvx			v23,r4,r8
		vsplth		v3,v8,3										
		addi		r8,r8,16

		lvx			v24,r4,r8
		vsplth		v4,v8,4										
		addi		r8,r8,16

		lvx			v25,r4,r8
		vsplth		v5,v8,5										
		addi		r8,r8,16

		lvx			v26,r4,r8
		vsplth		v6,v8,6										
		addi		r8,r8,16

		lvx			v27,r4,r8
		vsplth		v7,v8,7										

		lvx			v10,r3,r7
		addi		r7,r7,16
	
		lvx			v11,r3,r7
		addi		r7,r7,16
	
		lvx			v12,r3,r7
		vmladduhm	v10,v10,v20,v0				//dequant
		addi		r7,r7,16
	
		lvx			v13,r3,r7
		vmladduhm	v11,v11,v21,v0				//dequant
		addi		r7,r7,16
	
		lvx			v14,r3,r7
		vmladduhm	v12,v12,v22,v0				//dequant
		addi		r7,r7,16
	
		lvx			v15,r3,r7
		vmladduhm	v13,v13,v23,v0				//dequant
		addi		r7,r7,16
	
		lvx			v16,r3,r7
		vmladduhm	v14,v14,v24,v0				//dequant
		addi		r7,r7,16
	
		lvx			v17,r3,r7
		vmladduhm	v15,v15,v25,v0				//dequant

	    lwz         r10,vPerm1
		vmladduhm	v16,v16,v26,v0				//dequant
		xor			r7,r7,r7
		
	    lwz         r8,vPerm2

	    lwz         r9,vPerm3

	    lvx			v30,r10,r7
		vmladduhm	v17,v17,v27,v0				//dequant
												
	    lvx			v31,r8,r7

	    lvx			v29,r9,r7
												//order on entry	
												//00 10 20 30 40 50 60 70
												//01 11 21 31 41 51 61 71
												//02 12 22 32 42 52 62 72
												//03 13 23 33 43 53 63 73
												//04 14 24 34 44 54 64 74
												//05 15 25 35 45 55 65 75
												//06 16 26 36 46 56 66 76
												//07 17 27 37 47 57 67 77

//start of row idct
		vmulesh		v21,v11,v1
		
		vmulosh		v28,v11,v1
		
		vperm		v21,v21,v28,v29				//(c1 * i1) - i1
		
		vaddshs		v8,v21,v11					//A = (c1 * i1)

		vmulesh		v23,v13,v3
		
		vmulosh		v28,v13,v3
		
		vperm		v23,v23,v28,v29				//(c3 * i3) - i3
		
		vaddshs		v9,v23,v13					//C = (c3 * i3)
		
		vaddshs		v18,v8,v9					//C. = A + C
		
		vsubshs		v8,v8,v9					//(A - C)
		
		vmulesh		v9,v8,v4
		
		vmulosh		v28,v8,v4
		
		vmulesh		v21,v11,v7
		vperm		v9,v9,v28,v29				//(c4 * (A - C)) - (A - C)
		
		vaddshs		v9,v9,v8					//A. = c4 * (A - C)
//---
		vmulosh		v28,v11,v7
		
		vperm		v21,v21,v28,v29				//B = (c7 * i1)
		
		vmulesh		v23,v13,v5
		
		vmulosh		v28,v13,v5
		vspltish	v25,0
		
		vperm		v23,v23,v28,v29				//(c5 * i3) - i3
		
		vaddshs		v23,v23,v13					//(c5 * i3)

		vsubshs		v23,v25,v23					//D
		
		vaddshs		v19,v21,v23					//D. = B + D
		
		vsubshs		v21,v21,v23					//(B - D)
		
		vmulesh		v25,v21,v4
 		
		vmulosh		v28,v21,v4
		
		vperm		v25,v25,v28,v29				//(c4 * (B - D)) - (B - D)
		
		vaddshs		v21,v25,v21					//B. = c4 * (B - D)
//---
		vmulesh		v23,v10,v4
		
		vmulosh		v28,v10,v4
		
		vperm		v23,v23,v28,v29				//(c4 * i0) - i0
		
		vaddshs		v25,v23,v10					//F = E = c4 * i0
		
		vmulesh		v22,v12,v2
//---
		vmulosh		v28,v12,v2
		
		vperm		v22,v22,v28,v29				//(c2 * i2) - i2
		
		vaddshs		v24,v22,v12					//G = (c2 * i2)

		vmulesh		v22,v12,v6
		
		vmulosh		v28,v12,v6
		
		vperm		v22,v22,v28,v29				//H = (c6 * i2)

		vsubshs		v13,v25,v24					//E. = E - G
//---
		vaddshs		v10,v25,v24					//G. = E + G
		
		vaddshs		v11,v25,v9					//A.. = F + A.
		
		vsubshs		v15,v21,v22					//B.. = B. - H
		
		vsubshs		v27,v25,v9					//F. = F - A.
		
		vaddshs		v21,v21,v22					//H. = B. + H
//---
		vsubshs		v17,v10,v18					//R7 = G. - C.
		
		vaddshs		v10,v10,v18					//R0 = G. + C.
		
		vsubshs		v12,v11,v21					//R2 = A.. - H.
		
		vaddshs		v11,v11,v21					//R1 = A.. + H.
		
		vsubshs		v14,v13,v19					//R4 = E. - D.
		
		vaddshs		v13,v13,v19					//R3 = E. + D.
		
		vsubshs		v16,v27,v15					//R6 = F. - B..
		
		vaddshs		v15,v15,v27					//R5 = F. + B..	
//end of row idct		

//start of transpose
		vmrghh		v18,v10,v11					//00 01 10 11 20 21 30 31 
		vmrglh		v19,v10,v11					//40 41 50 51 60 61 70 71
		vmrghh		v20,v12,v13					//02 03 12 13 22 23 32 33
		vmrglh		v21,v12,v13					//42 43 52 53 62 63 72 73
		vmrghh		v22,v14,v15					//04 05 14 15 24 25 34 35
		vmrglh		v23,v14,v15					//44 45 54 55 64 65 74 75
		vmrghh		v24,v16,v17					//06 07 16 17 26 27 36 37
		vmrglh		v25,v16,v17					//46 47 56 57 66 67 76 77
		
		vmrghw		v8,v18,v20					//00 01 02 03 10 11 12 13
		vmrghw		v9,v22,v24					//04 05 06 07 14 15 16 17
		vmrghw		v26,v19,v21					//40 41 42 43 50 51 52 53
		vmrghw		v27,v23,v25					//44 45 46 47 54 55 56 57
		vmrglw		v18,v18,v20					//20 21 22 23 30 31 32 33
		vmrglw		v22,v22,v24					//24 25 26 27 34 35 36 37
		vmrglw		v19,v19,v21					//60 61 62 63 70 71 72 73
		vmrglw		v23,v23,v25					//64 65 66 67 74 75 76 77
		
		vperm		v10,v8,v9,v30				//00 01 02 03 04 05 06 07
		vperm		v11,v8,v9,v31				//10 11 12 13 14 15 16 17
		vperm		v12,v18,v22,v30				//20 21 22 23 24 25 26 27
		vperm		v13,v18,v22,v31				//30 31 32 33 34 35 36 37
		vperm		v14,v26,v27,v30				//40 41 42 43 44 45 46 47
		vperm		v15,v26,v27,v31				//50 51 52 53 54 55 56 57 
		vperm		v16,v19,v23,v30				//60 61 62 63 64 65 66 67
		vperm		v17,v19,v23,v31				//70 71 72 73 74 75 76 77
//end of transpose		
		

//start of col idct
		vmulesh		v21,v11,v1
		
		vmulosh		v28,v11,v1
		
		vperm		v21,v21,v28,v29				//(c1 * i1) - i1
		
		vaddshs		v8,v21,v11					//A = (c1 * i1)

		vmulesh		v23,v13,v3
		
		vmulosh		v28,v13,v3
		
		vperm		v23,v23,v28,v29				//(c3 * i3) - i3
		
		vaddshs		v9,v23,v13					//C = (c3 * i3)
		
		vaddshs		v18,v8,v9					//C. = A + C
		
		vsubshs		v8,v8,v9					//(A - C)
		
		vmulesh		v9,v8,v4
		
		vmulosh		v28,v8,v4
		
		vmulesh		v21,v11,v7
		vperm		v9,v9,v28,v29				//(c4 * (A - C)) - (A - C)
		
		vaddshs		v9,v9,v8					//A. = c4 * (A - C)
//---
		vmulosh		v28,v11,v7
		
		vperm		v21,v21,v28,v29				//B = (c7 * i1)
		
		vmulesh		v23,v13,v5
		
		vmulosh		v28,v13,v5
		vspltish	v25,0
		
		vperm		v23,v23,v28,v29				//(c5 * i3) - i3
		
		vaddshs		v23,v23,v13					//(c5 * i3)

		vsubshs		v23,v25,v23					//D
		
		vaddshs		v19,v21,v23					//D. = B + D
		
		vsubshs		v21,v21,v23					//(B - D)
		
		vmulesh		v25,v21,v4
 		
		vmulosh		v28,v21,v4
		
		vperm		v25,v25,v28,v29				//(c4 * (B - D)) - (B - D)
		
		vaddshs		v21,v25,v21					//B. = c4 * (B - D)
//---
		vmulesh		v23,v10,v4
		
		vmulosh		v28,v10,v4
		
		vperm		v23,v23,v28,v29				//(c4 * i0) - i0
		
		vaddshs		v25,v23,v10					//F = E = c4 * i0
		
		vmulesh		v22,v12,v2
//---
		vmulosh		v28,v12,v2
		
		vperm		v22,v22,v28,v29				//(c2 * i2) - i2
		
		vaddshs		v24,v22,v12					//G = (c2 * i2)

		vmulesh		v22,v12,v6
		
		vmulosh		v28,v12,v6
		
		vperm		v22,v22,v28,v29				//H = (c6 * i2)

		vsubshs		v13,v25,v24					//E. = E - G
//---
		vaddshs		v10,v25,v24					//G. = E + G
		
		vaddshs		v11,v25,v9					//A.. = F + A.
		
		vsubshs		v15,v21,v22					//B.. = B. - H
		
		vsubshs		v27,v25,v9					//F. = F - A.
		
		vaddshs		v21,v21,v22					//H. = B. + H
//---
		vsubshs		v17,v10,v18					//R7 = G. - C.
		
		vaddshs		v10,v10,v18					//R0 = G. + C.
		
		vsubshs		v12,v11,v21					//R2 = A.. - H.
		
		vaddshs		v11,v11,v21					//R1 = A.. + H.
		
		vsubshs		v14,v13,v19					//R4 = E. - D.
		
		vaddshs		v13,v13,v19					//R3 = E. + D.
		
		vsubshs		v16,v27,v15					//R6 = F. - B..
		
		vaddshs		v15,v15,v27					//R5 = F. + B..	
//end of col idct
		xor			r8,r8,r8
		vspltish	v0,8

		vaddshs		v10,v10,v0					//IdctAdjustBeforeShift 
		vaddshs		v11,v11,v0					//IdctAdjustBeforeShift 
		vaddshs		v12,v12,v0					//IdctAdjustBeforeShift 
		vaddshs		v13,v13,v0					//IdctAdjustBeforeShift 
		vaddshs		v14,v14,v0					//IdctAdjustBeforeShift 
		vaddshs		v15,v15,v0					//IdctAdjustBeforeShift 
		vaddshs		v16,v16,v0					//IdctAdjustBeforeShift 
		vaddshs		v17,v17,v0					//IdctAdjustBeforeShift 
		vspltish	v0,4
		
		vsrah		v10,v10,v0
		
		stvx		v10,r5,r8
		vsrah		v11,v11,v0
		addi		r8,r8,16

		stvx		v11,r5,r8
		vsrah		v12,v12,v0
		addi		r8,r8,16

		stvx		v12,r5,r8
		vsrah		v13,v13,v0
		addi		r8,r8,16

		stvx		v13,r5,r8
		vsrah		v14,v14,v0
		addi		r8,r8,16

		stvx		v14,r5,r8
		vsrah		v15,v15,v0
		addi		r8,r8,16

		stvx		v15,r5,r8
		vsrah		v16,v16,v0
		addi		r8,r8,16

		stvx		v16,r5,r8
		vsrah		v17,v17,v0
		addi		r8,r8,16

		stvx		v17,r5,r8
	}
	
}

