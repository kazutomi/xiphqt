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
*   Module Title :     COptFunctionsPPC.c
*
*   Description  :     Encoder system dependant functions
*
*
*****************************************************************************
*/

/*******************************************3*********************************
*  Header Files
*****************************************************************************
*/

//#include <math.h>
#include "compdll.h"
//#include <assert.h>

/****************************************************************************
*  Imports
******************************************************************************|
*/

/****************************************************************************
 * 
 *  ROUTINE       :     GetSAD_Altivec
 *
 *  INPUTS        :     UINT8 * NewDataPtr	(New Data)
 *						UINT8 * RefDataPtr
 *						UINT32	PixelsPerLine
 *                      INT32   ErrorSoFar, 
 *						INT32   BestSoFar 
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Sum absolute differences
 *
 *  FUNCTION      :     Calculates the sum of the absolute differences.
 *
 *  SPECIAL NOTES :     Assumption:  PixelsPerLine will always be a multiple of 16 and
 *							the NewDataPtr is aligned on a block boundary.
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
					/*			r3,                  r4,                   r5,               r6,                r7 */
UINT32 
GetSAD_Altivec( UINT8 * NewDataPtr, UINT8  * RefDataPtr, UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar)
{
	vector unsigned short DiffVal;
	unsigned short *DiffValPtr = (unsigned short *)&DiffVal;
	UINT32 retVal = ErrorSoFar;

	asm
	{
		li			r30,16					//used to get next 16 bytes
		xor			r31,r31,r31
		addi		r10,r5,32				//PixelsPerLine + 32

		lvsl		v8,r3,r31				//load vector alignment for NewData
		vxor		v31,v31,v31				//DiffVal starts at zero
		li			r8,8					//height counter
		
		lvsl		v9,r4,r31				//load vector alignment for RefData
		vxor		v30,v30,v30				//used for unpacking
		xor			r9,r9,r9
		
		vxor        v2,v2,v2
		
		vxor        v4,v4,v4
		
		vxor        v5,v5,v5
						
	GetSAD_loop:
		lvx			v0,r3,r31				//NewData[0]
		vor			v4,v4,v5
		addi		r9,r9,2					//bump line index
		
		lvx			v10,r4,r31				//RefData[0]
		vperm		v0,v0,v0,v8
		add			r3,r3,r5
		cmpw        r9,r8
		
		lvx			v11,r4,r30				//RefData[0] + 16 bytes (cuz RefData can be misaligned)
		vadduhm		v31,v31,v2				//add to DiffVal
		vmrghb		v4,v30,v4				//convert to unsigned shorts
		add			r4,r4,r10
		
		lvx			v1,r3,r31				//NewData[1]
		vadduhm		v31,v31,v4				//add to DiffVal
		vperm		v10,v10,v11,v9
		add			r3,r3,r5
	
		lvx			v12,r4,r31				//RefData[1]
		vsububs		v2,v0,v10
		
		lvx			v13,r4,r30				//RefData[1] + 16 bytes (cuz RefData can be misaliged)
		vsububs		v3,v10,v0
		vperm		v1,v1,v1,v8

		vor			v2,v2,v3				//now have absolute difference
		vperm		v12,v12,v13,v9
		add			r4,r4,r10
				
		vmrghb		v2,v30,v2				//convert to unsigned shorts
		vsububs		v4,v1,v12
		
		vsububs		v5,v12,v1				//now have absolute difference
		
//		vor			v4,v4,v5
		
//		vadduhm		v31,v31,v2				//add to DiffVal
//		vmrghb		v4,v30,v4				//convert to unsigned shorts


//		vadduhm		v31,v31,v4				//add to DiffVal
		bne			GetSAD_loop
		
		lwz			r9,DiffValPtr
		xor			r8,r8,r8

		vor			v4,v4,v5
		
		vadduhm		v31,v31,v2				//add to DiffVal
		vmrghb		v4,v30,v4				//convert to unsigned shorts


		vadduhm		v31,v31,v4				//add to DiffVal
		
		stvx		v31,r9,r8
	}
		
	retVal += DiffValPtr[0];
	retVal += DiffValPtr[1];
	retVal += DiffValPtr[2];
	retVal += DiffValPtr[3];
	retVal += DiffValPtr[4];
	retVal += DiffValPtr[5];
	retVal += DiffValPtr[6];
	retVal += DiffValPtr[7];
		
	return retVal;
}

/****************************************************************************
 * 
 *  ROUTINE       :     GetHalfPixelSAD_Altivec
 *
 *  INPUTS        :     UINT8 * SrcData	(New Data)
 *						UINT8 * RefDataPtr1
 *						UINT8 * RefDataPtr2
 *						UINT32	PixelsPerLine
 *                      INT32   ErrorSoFar 
 *						INT32   BestSoFar 
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     Sum of absolute differences at 1/2 pixel accuracy.
 *
 *  FUNCTION      :     Calculates the sum of the absolute differences against
 *                      half pixel interpolated references.
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
							/*		  r3,                  r4,                 r5,                    r6,               r7,                r8 */
UINT32 
GetHalfPixelSAD_Altivec( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar  )
{       
	UINT32	retVal = ErrorSoFar;
    INT32   RefOffset = (int)(RefDataPtr1 - RefDataPtr2);

    if ( RefOffset == 0 )
    {
		// Simple case as for non 0.5 pixel
		retVal += GetSAD_Altivec( SrcData, RefDataPtr1, PixelsPerLine, ErrorSoFar, BestSoFar );
    }
    else 
    {
		vector unsigned short DiffVal;
		unsigned short *DiffValPtr = (unsigned short *)&DiffVal;

		asm
		{
			li			r30,16					//used to get next 16 bytes
			xor			r31,r31,r31
			addi		r10,r6,32				//PixelsPerLine + 32
	
			lvsl		v7,r5,r31				//load vector alignment for RefData2
			vspltish	v29,1

			lvsl		v8,r3,r31				//load vector alignment for NewData
			vxor		v31,v31,v31				//DiffVal starts at zero
			li			r8,8					//height counter
			
			lvsl		v9,r4,r31				//load vector alignment for RefData1
			vxor		v30,v30,v30				//used for unpacking
			xor			r9,r9,r9
							
		GetSADHalfPixel_loop:
			lvx			v0,r3,r31				//NewData[0]
			addi		r9,r9,2					//bump line index
			add			r3,r3,r6
			
			lvx			v1,r4,r31				//RefData1[0]
			cmpw        r9,r8

			lvx			v2,r4,r30				//RefData1[0] + 16 bytes (cuz RefData can be misaligned)
			add			r4,r4,r10

			lvx			v3,r5,r31				//RefData2[0]
			vperm		v0,v0,v0,v8
			
			lvx			v4,r5,r30				//RefData2[0] + 16 bytes (cuz RefData can be misaligned)
			vperm		v1,v1,v2,v9
			add			r5,r5,r10

			lvx			v10,r3,r31				//NewData[1]
			add			r3,r3,r6
			vmrghb		v1,v30,v1				//convert to unsigned short

			lvx			v11,r4,r31				//RefData1[1]
			vperm		v3,v3,v4,v7

			lvx			v12,r4,r30				//RefData1[1] + 16 bytes (cuz RefData can be misaligned)
			vmrghb		v3,v30,v3				//convert to unsigned short
			add			r4,r4,r10

			lvx			v13,r5,r31				//RefData2[1]
			vadduhm		v1,v1,v3				//RefData1 + RefData2
			vmrghb		v0,v30,v0				//convert to unsigned short

			lvx			v14,r5,r30				//RefData2[1] + 16 bytes (cuz RefData can be misaligned)
			vsrh		v1,v1,v29				//(RefData1 + RefData2)>>1
			vperm       v10,v10,v10,v8
			add         r5,r5,r10
			
			vsubuhs		v2,v1,v0
			vperm       v11,v11,v12,v9
			
			vsubuhs		v3,v0,v1
			vperm       v13,v13,v14,v7
			
			vor			v2,v2,v3				//now have absolute difference
			vmrghb		v11,v30,v11				//convert to unsigned short

			vadduhm		v31,v31,v2				//add to DiffVal
			vmrghb		v13,v30,v13				//convert to unsigned short

			vadduhm		v11,v11,v13				//RefData1 + RefData2
			vmrghb		v10,v30,v10				//convert to unsigned short

			vsrh		v11,v11,v29				//(RefData1 + RefData2)>>1

			vsubuhs		v12,v11,v10
			
			vsubuhs		v13,v10,v11
			
			vor			v12,v12,v13				//now have absolute difference

			vadduhm		v31,v31,v12				//add to DiffVal
			bne			GetSADHalfPixel_loop
			
			lwz			r9,DiffValPtr
			xor			r8,r8,r8
			
			stvx		v31,r9,r8
		}
		retVal += DiffValPtr[0];
		retVal += DiffValPtr[1];
		retVal += DiffValPtr[2];
		retVal += DiffValPtr[3];
		retVal += DiffValPtr[4];
		retVal += DiffValPtr[5];
		retVal += DiffValPtr[6];
		retVal += DiffValPtr[7];
	}

	return retVal;
}