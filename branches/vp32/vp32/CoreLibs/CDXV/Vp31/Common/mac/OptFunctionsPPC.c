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
*   Module Title :     OptFunctions.c
*
*   Description  :     MMX or otherwise processor specific 
*                      optimised versions of functions
*
*
*****************************************************************************
*/



/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              // Strict type checking. 

#include "codec_common.h"

#include "pbdll.h"

//#pragma profile off


/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

/****************************************************************************
*  Imports.
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
*  Forward References
*****************************************************************************
*/  
/****************************************************************************
 * 
 *  ROUTINE       :     PPCReconIntra( PB_INSTANCE *pbi, UINT8 * dest, UINT16 * diff, UINT32 stride)
 *
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
 						/*	      r3,           r4,           r5,            r6  */
void PPCReconIntra( PB_INSTANCE *pbi, UINT8 * dest, UINT16 * diff, UINT32 stride)
{
	(void) pbi;

	asm
	{
		//temp kludge
		mr		r3,r4					;//adjust for extra unused parameter -- this code is copied from vp30
		mr		r4,r5
		mr		r5,r6
	
		lwz		r0,0(r3)				;//preload cache
		mr		r12,r4
		
		addi	r12,r12,128				;//end ptr 
		
doLoop1:
		lha		r7,0(r4)
		
		lha		r8,2(r4)
		addi	r7,r7,128
	
		lha		r9,4(r4)
		addi	r8,r8,128		
		andi.	r0,r7,0xff00
		beq+		L1
		
		srawi	r0,r7,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r7,r0,0xff   			;//now have 00 or ff

L1:			
		lha		r10,6(r4)
		addi	r9,r9,128
		andi.	r0,r8,0xff00	
		beq+		L2	
	
		srawi	r0,r8,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r8,r0,0xff   			;//now have 00 or ff

L2:			
		lha		r31,8(r4)
		addi 	r10,r10,128
		andi.	r0,r9,0xff00
		beq+		L3
		
		srawi	r0,r9,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r9,r0,0xff   			;//now have 00 or ff

L3:					
		lha		r30,10(r4)
		andi.	r0,r10,0xff00
		beq+		L4
		
		srawi	r0,r10,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r10,r0,0xff   			;//now have 00 or ff

L4:
		lha		r29,12(r4)
		insrwi	r10,r7,8,0
		addi	r31,r31,128
		
		lwz		r27,0(r3)				;//preload cache with dest
		addi 	r30,r30,128
		andi.	r0,r31,0xff00
		beq+		L5
		
		srawi	r0,r31,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r31,r0,0xff   			;//now have 00 or ff

L5:
		lha		r28,14(r4)
		addi 	r29,r29,128
		andi.	r0,r30,0xff00
		beq+		L6
		
		srawi	r0,r30,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r30,r0,0xff   			;//now have 00 or ff
		
L6:		
		addi 	r28,r28,128
		andi.	r0,r29,0xff00
		beq+		L7
		
		srawi	r0,r29,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r29,r0,0xff   			;//now have 00 or ff
		
L7:		
		insrwi	r10,r8,8,8
		andi.	r0,r28,0xff00
		beq+		L8
		
		srawi	r0,r28,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r28,r0,0xff   			;//now have 00 or ff
		
L8:		
		insrwi	r10,r9,8,16
		insrwi	r28,r31,8,0
		
		stw		r10,0(r3)
		insrwi	r28,r30,8,8
		addi 	r4,r4,16		
		
		cmpw	r4,r12
		insrwi	r28,r29,8,16

		stw		r28,4(r3)
		add 	r3,r3,r5 				;//add in stride
		bne		doLoop1

	}	
}

/****************************************************************************
 * 
 *  ROUTINE       :     PPCReconInter(  PB_INSTANCE *pbi, UINT8 * dest, UINT8 * ref, INT16 * diff, UINT32 stride)()
 *
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
 						/*	      r3,           r4,           r5,           r6,            r7 */
void PPCReconInter(  PB_INSTANCE *pbi, UINT8 * dest, UINT8 * ref, INT16 * diff, UINT32 stride)
{
	(void) pbi;

	asm
	{
		//temp kludge
		mr		r3,r4					;//adjust for extra unused parameter -- this code is copied from vp30
		mr		r4,r5
		mr		r5,r6
		mr		r6,r7



		mr		r26,r4
		mr		r4,r5					;//same reg usage as intra
			
		lwz		r0,0(r3)				;//preload cache
		mr		r12,r4
		
		addi	r12,r12,128				;//end ptr 
		mr		r5,r6					;//same reg usage as intra
		
doLoop1:
		lha		r7,0(r4)
		
		lbz		r25,0(r26)
		
		lha		r8,2(r4)
		add 	r7,r7,r25
	
		lbz		r25,1(r26)

		lha		r9,4(r4)
		add		r8,r8,r25		
		andi.	r0,r7,0xff00
		beq+		L1
		
		srawi	r0,r7,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r7,r0,0xff   			;//now have 00 or ff

L1:			
		lbz		r25,2(r26)

		lha		r10,6(r4)
		add 	r9,r9,r25
		andi.	r0,r8,0xff00	
		beq+		L2	
	
		srawi	r0,r8,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r8,r0,0xff   			;//now have 00 or ff

L2:			
		lbz		r25,3(r26)

		lha		r31,8(r4)
		add  	r10,r10,r25
		andi.	r0,r9,0xff00
		beq+		L3
		
		srawi	r0,r9,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r9,r0,0xff   			;//now have 00 or ff

L3:					
		lha		r30,10(r4)
		andi.	r0,r10,0xff00
		beq+		L4
		
		srawi	r0,r10,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r10,r0,0xff   			;//now have 00 or ff

L4:
		lbz		r25,4(r26)


		lha		r29,12(r4)
		insrwi	r10,r7,8,0
		add 	r31,r31,r25

		lbz		r25,5(r26)
		
		lwz		r27,0(r3)				;//preload cache with dest
		add  	r30,r30,r25
		andi.	r0,r31,0xff00
		beq+		L5
		
		srawi	r0,r31,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r31,r0,0xff   			;//now have 00 or ff

L5:
		lbz		r25,6(r26)

		lha		r28,14(r4)
		add  	r29,r29,r25
		andi.	r0,r30,0xff00
		beq+		L6
		
		srawi	r0,r30,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r30,r0,0xff   			;//now have 00 or ff
		
L6:		
		lbz		r25,7(r26)
		add		r26,r26,r5

		add  	r28,r28,r25
		andi.	r0,r29,0xff00
		beq+		L7
		
		srawi	r0,r29,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r29,r0,0xff   			;//now have 00 or ff
		
L7:		
		insrwi	r10,r8,8,8
		andi.	r0,r28,0xff00
		beq+		L8
		
		srawi	r0,r28,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r28,r0,0xff   			;//now have 00 or ff
		
L8:		
		insrwi	r10,r9,8,16
		insrwi	r28,r31,8,0
		
		stw		r10,0(r3)
		insrwi	r28,r30,8,8
		addi 	r4,r4,16		
		
		cmpw	r4,r12
		insrwi	r28,r29,8,16

		stw		r28,4(r3)
		add 	r3,r3,r5 				;//add in stride
		bne		doLoop1

	}	
}
/****************************************************************************
 * 
 *  ROUTINE       :     PPCReconInterHalfPixel2( PB_INSTANCE *pbi, UINT8* dest, UINT8* r, UINT8* s, INT16 * diff, UINT32 stride)
 *
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
									/*	    r3,          r4,       r5,       r6,           r7,            r8  */
void PPCReconInterHalfPixel2( PB_INSTANCE *pbi, UINT8* dest, UINT8* r, UINT8* s, INT16 * diff, UINT32 stride) 
{
	(void) pbi;
	
	asm
	{
		//temp kludge
		mr		r3,r4					;//adjust for extra unused parameter -- this code is copied from vp30
		mr		r4,r5
		mr		r5,r6
		mr		r6,r7
		mr		r7,r8


		mr		r26,r4
		mr		r4,r6					;//same reg usage as intra
			
		lwz		r0,0(r3)				;//preload cache
		mr		r25,r5	
		mr		r12,r4
		
		addi	r12,r12,128				;//end ptr 
		mr		r5,r7					;//same reg usage as intra
		
		li		r24,0x0101
		li		r23,0xfefe
		
		insrwi	r23,r23,16,0			;//0xfefefefe
		insrwi	r24,r24,16,0			;//0x01010101
		
doLoop1:
		lwz		r22,0(r26)				;//get 4 ref pels		

		lwz		r21,0(r25)				;//get 4 src pels
		
		lha		r7,0(r4)
		and		r20,r22,r21

		lha		r8,2(r4)
		and		r21,r21,r23				;//mask low bits
		and		r22,r22,r23				;//mask low bits
	
		srwi	r21,r21,1
		srwi	r22,r22,1
		
		and		r20,r20,r24				;//save low bits
		add		r21,r21,r22
		
		lwz		r22,4(r26)				;//get 4 ref pels		
//		or		r20,r21,r20				;//add in hot fudge		
		add		r20,r21,r20				;//add in hot fudge		

//xor r20,r20,r20

		lwz		r21,4(r25)				;//get 4 src pels
		rlwinm	r19,r20,8,24,31
		rlwinm	r18,r20,16,24,31		

		add 	r7,r7,r19
		
		lha		r9,4(r4)
		add		r8,r8,r18		
		andi.	r0,r7,0xff00
		beq+		L1
		
		srawi	r0,r7,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r7,r0,0xff   			;//now have 00 or ff

L1:			
		rlwinm	r19,r20,24,24,31
		rlwinm	r18,r20,0,24,31		

		lha		r10,6(r4)
		add 	r9,r9,r19
		andi.	r0,r8,0xff00	
		beq+		L2	
	
		srawi	r0,r8,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r8,r0,0xff   			;//now have 00 or ff

L2:			
		lha		r31,8(r4)
		add  	r10,r10,r18
		andi.	r0,r9,0xff00
		beq+		L3
		
		srawi	r0,r9,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r9,r0,0xff   			;//now have 00 or ff

L3:					
		lha		r30,10(r4)
		andi.	r0,r10,0xff00
		beq+		L4
		
		srawi	r0,r10,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r10,r0,0xff   			;//now have 00 or ff

L4:
		lha		r29,12(r4)
		insrwi	r10,r7,8,0
		and		r20,r22,r21

		and		r21,r21,r23				;//mask low bits
		and		r22,r22,r23				;//mask low bits
	
		srwi	r21,r21,1
		srwi	r22,r22,1
		
		and		r20,r20,r24				;//save low bits
		add		r21,r21,r22
		
//		or		r20,r21,r20				;//add in hot fudge		
		add		r20,r21,r20				;//add in hot fudge		
	
		rlwinm	r19,r20,8,24,31
		rlwinm	r18,r20,16,24,31		
	
	
		add 	r31,r31,r19

//xor r20,r20,r20

		lwz		r27,0(r3)				;//preload cache with dest
		add  	r30,r30,r18
		andi.	r0,r31,0xff00
		beq+		L5
		
		srawi	r0,r31,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r31,r0,0xff   			;//now have 00 or ff

L5:
		rlwinm	r19,r20,24,24,31
		rlwinm	r18,r20,0,24,31		

		lha		r28,14(r4)
		add  	r29,r29,r19
		andi.	r0,r30,0xff00
		beq+		L6
		
		srawi	r0,r30,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r30,r0,0xff   			;//now have 00 or ff
		
L6:		
		add		r26,r26,r5				;//add stride to ref pels
		add		r25,r25,r5				;//add stride to src pels

		add  	r28,r28,r18
		andi.	r0,r29,0xff00
		beq+		L7
		
		srawi	r0,r29,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r29,r0,0xff   			;//now have 00 or ff
		
L7:		
		insrwi	r10,r8,8,8
		andi.	r0,r28,0xff00
		beq+		L8
		
		srawi	r0,r28,15				;//generate ff or 00
		
		xori	r0,r0,0xff				;//flip the bits
		
		andi.	r28,r0,0xff   			;//now have 00 or ff
		
L8:		
		insrwi	r10,r9,8,16
		insrwi	r28,r31,8,0
		
		stw		r10,0(r3)
		insrwi	r28,r30,8,8
		addi 	r4,r4,16		
		
		cmpw	r4,r12
		insrwi	r28,r29,8,16

		stw		r28,4(r3)
		add 	r3,r3,r5 				;//add in stride
		bne		doLoop1

	}	
}
/****************************************************************************
 * 
 *  ROUTINE       :     PPCReconIntra_Altivec(PB_INSTANCE *pbi, UINT8 * dest, UINT16 * diff, UINT32 stride)
 *
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
 						/*	      r3,           r4,           r5,            r6  */
void PPCReconIntra_Altivec( PB_INSTANCE *pbi, UINT8 * dest, UINT16 * diff, UINT32 stride)
{

	(void) pbi;

	asm
	{

//trying cache hints
		lis			r7,0x0108
		or			r7,r7,r6
		dstst		r4,r7,0

		vspltish	v1,7

		vspltish	v8,1
		xor			r7,r7,r7
		
		lvx			v0,r5,r7				//get 8 shorts					
		vslh		v8,v8,v1				//now have 128
		addi		r7,r7,16
		
		lvx			v1,r5,r7				//get 8 shorts					
		vaddshs		v0,v0,v8				//+=128
		addi		r7,r7,16

		lvx			v2,r5,r7				//get 8 shorts					
		vaddshs		v1,v1,v8				//+=128
		addi		r7,r7,16
		vpkshus		v0,v0,v0				//convert to bytes

		lvx			v3,r5,r7				//get 8 shorts					
		vaddshs		v2,v2,v8				//+=128
		addi		r7,r7,16
		vpkshus		v1,v1,v1				//convert to bytes

		lvx			v4,r5,r7				//get 8 shorts					
		vaddshs		v3,v3,v8				//+=128
		addi		r7,r7,16
		vpkshus		v2,v2,v2				//convert to bytes

		lvx			v5,r5,r7				//get 8 shorts					
		vaddshs		v4,v4,v8				//+=128
		addi		r7,r7,16
		vpkshus		v3,v3,v3				//convert to bytes

		lvx			v6,r5,r7				//get 8 shorts					
		vaddshs		v5,v5,v8				//+=128
		addi		r7,r7,16
		vpkshus		v4,v4,v4				//convert to bytes

		lvx			v7,r5,r7				//get 8 shorts	
		xor			r7,r7,r7				
		vaddshs		v6,v6,v8				//+=128
		vpkshus		v5,v5,v5				//convert to bytes

		lvsr		v9,r4,r7				//load alignment vector for stores
		vaddshs		v7,v7,v8				//+=128
		vpkshus		v6,v6,v6				//convert to bytes

		vpkshus		v7,v7,v7				//convert to bytes

		li			r8,4
		vperm		v0,v0,v0,v9

		stvewx		v0,r4,r7
		add			r7,r7,r6

		lvsr		v9,r4,r7				//load alignment vector for stores

		stvewx		v0,r4,r8
		add			r8,r8,r6
		vperm		v1,v1,v1,v9

		stvewx		v1,r4,r7
		add			r7,r7,r6

		lvsr		v9,r4,r7				//load alignment vector for stores

		stvewx		v1,r4,r8
		add			r8,r8,r6
		vperm		v2,v2,v2,v9

		stvewx		v2,r4,r7
		add			r7,r7,r6

		lvsr		v9,r4,r7				//load alignment vector for stores

		stvewx		v2,r4,r8
		add			r8,r8,r6
		vperm		v3,v3,v3,v9

		stvewx		v3,r4,r7
		add			r7,r7,r6

		lvsr		v9,r4,r7				//load alignment vector for stores

		stvewx		v3,r4,r8
		add			r8,r8,r6
		vperm		v4,v4,v4,v9

		stvewx		v4,r4,r7
		add			r7,r7,r6

		lvsr		v9,r4,r7				//load alignment vector for stores

		stvewx		v4,r4,r8
		add			r8,r8,r6
		vperm		v5,v5,v5,v9

		stvewx		v5,r4,r7
		add			r7,r7,r6

		lvsr		v9,r4,r7				//load alignment vector for stores

		stvewx		v5,r4,r8
		add			r8,r8,r6
		vperm		v6,v6,v6,v9

		stvewx		v6,r4,r7
		add			r7,r7,r6

		lvsr		v9,r4,r7				//load alignment vector for stores

		stvewx		v6,r4,r8
		add			r8,r8,r6
		vperm		v7,v7,v7,v9

		stvewx		v7,r4,r7

		stvewx		v7,r4,r8

	}	
}
/****************************************************************************
 * 
 *  ROUTINE       :     PPCReconInter_Altivec(  PB_INSTANCE *pbi, UINT8 * dest, UINT8 * ref, INT16 * diff, UINT32 stride)()
 *
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
							/*	      r3,           r4,           r5,           r6,            r7 */
void PPCReconInter_Altivec(  PB_INSTANCE *pbi, UINT8 * dest, UINT8 * ref, INT16 * diff, UINT32 stride)
{

	(void) pbi;

	asm
	{
//trying cache hints
		lis			r8,0x0108
		or			r8,r8,r7
		dstst		r4,r8,0
		
		xor			r8,r8,r8
		li			r9,16
		
		lvsl		v8,r5,r8				//load alignment vector for refs
		vxor		v9,v9,v9
		
		lvx			v10,r5,r8				//get 8 refs
		add			r8,r8,r7					

		lvx			v0,r5,r9				//need another 16 bytes for misaligned data -- 0
		add			r9,r9,r7					

		lvx			v11,r5,r8				//get 8 refs
		vperm		v10,v10,v0,v8

		lvsl		v8,r5,r8				//load alignment vector for refs
		add			r8,r8,r7					

		lvx			v1,r5,r9				//need another 16 bytes for misaligned data -- 1
		add			r9,r9,r7					

		lvx			v12,r5,r8				//get 8 refs
		vperm		v11,v11,v1,v8

		lvsl		v8,r5,r8				//load alignment vector for refs
		add			r8,r8,r7					

		lvx			v2,r5,r9				//need another 16 bytes for misaligned data -- 2
		add			r9,r9,r7					

		lvx			v13,r5,r8				//get 8 refs
		vperm		v12,v12,v2,v8

		lvsl		v8,r5,r8				//load alignment vector for refs
		add			r8,r8,r7					

		lvx			v3,r5,r9				//need another 16 bytes for misaligned data -- 3
		add			r9,r9,r7					

		lvx			v14,r5,r8				//get 8 refs
		vperm		v13,v13,v3,v8

		lvsl		v8,r5,r8				//load alignment vector for refs
		add			r8,r8,r7					

		lvx			v4,r5,r9				//need another 16 bytes for misaligned data -- 4
		add			r9,r9,r7					

		lvx			v15,r5,r8				//get 8 refs
		vperm		v14,v14,v4,v8

		lvsl		v8,r5,r8				//load alignment vector for refs
		add			r8,r8,r7					

		lvx			v5,r5,r9				//need another 16 bytes for misaligned data -- 5
		add			r9,r9,r7					

		lvx			v16,r5,r8				//get 8 refs
		vperm		v15,v15,v5,v8

		lvsl		v8,r5,r8				//load alignment vector for refs
		add			r8,r8,r7					

		lvx			v6,r5,r9				//need another 16 bytes for misaligned data -- 6
		add			r9,r9,r7					

		lvx			v17,r5,r8				//get 8 refs
		vperm		v16,v16,v6,v8

		lvsl		v8,r5,r8				//load alignment vector for refs
		xor			r8,r8,r8

		lvx			v7,r5,r9				//need another 16 bytes for misaligned data -- 7
		add			r9,r9,r7					

		lvx			v0,r6,r8				//get 8 shorts 				
		vperm		v17,v17,v7,v8
		addi		r8,r8,16

		lvx			v1,r6,r8				//get 8 shorts 				
		vmrghb		v10,v9,v10				//unsigned byte -> unsigned half		
		addi		r8,r8,16

		lvx			v2,r6,r8				//get 8 shorts 				
		vmrghb		v11,v9,v11				//unsigned byte -> unsigned half		
		vaddshs		v0,v0,v10
		addi		r8,r8,16

		lvx			v3,r6,r8				//get 8 shorts 				
		vmrghb		v12,v9,v12				//unsigned byte -> unsigned half		
		vaddshs		v1,v1,v11
		addi		r8,r8,16

		lvx			v4,r6,r8				//get 8 shorts 				
		vmrghb		v13,v9,v13				//unsigned byte -> unsigned half		
		vaddshs		v2,v2,v12
		addi		r8,r8,16

		lvx			v5,r6,r8				//get 8 shorts 				
		vmrghb		v14,v9,v14				//unsigned byte -> unsigned half		
		vaddshs		v3,v3,v13
		addi		r8,r8,16

		lvx			v6,r6,r8				//get 8 shorts 				
		vmrghb		v15,v9,v15				//unsigned byte -> unsigned half		
		vaddshs		v4,v4,v14
		addi		r8,r8,16

		lvx			v7,r6,r8				//get 8 shorts 				
		vmrghb		v16,v9,v16				//unsigned byte -> unsigned half		
		vaddshs		v5,v5,v15
		
		vmrghb		v17,v9,v17				//unsigned byte -> unsigned half	
		vaddshs		v6,v6,v16
		
		vpkshus		v0,v0,v0				
		vaddshs		v7,v7,v17
			
		vpkshus		v1,v1,v1				
		xor			r8,r8,r8

		vpkshus		v2,v2,v2				

		vpkshus		v3,v3,v3				

		vpkshus		v4,v4,v4				

		vpkshus		v5,v5,v5				

		vpkshus		v6,v6,v6				

		lvsr		v9,r4,r8				//load alignment vector for stores
		vpkshus		v7,v7,v7

		li			r9,4
		vperm		v0,v0,v0,v9				//adjust for writes

		stvewx		v0,r4,r8
		add			r8,r8,r7	

		lvsr		v9,r4,r8				//load alignment vector for stores

		stvewx		v0,r4,r9
		add			r9,r9,r7	
		vperm		v1,v1,v1,v9

		stvewx		v1,r4,r8
		add			r8,r8,r7	

		lvsr		v9,r4,r8				//load alignment vector for stores

		stvewx		v1,r4,r9
		add			r9,r9,r7	
		vperm		v2,v2,v2,v9

		stvewx		v2,r4,r8
		add			r8,r8,r7	

		lvsr		v9,r4,r8				//load alignment vector for stores

		stvewx		v2,r4,r9
		add			r9,r9,r7	
		vperm		v3,v3,v3,v9

		stvewx		v3,r4,r8
		add			r8,r8,r7	

		lvsr		v9,r4,r8				//load alignment vector for stores

		stvewx		v3,r4,r9
		add			r9,r9,r7	
		vperm		v4,v4,v4,v9

		stvewx		v4,r4,r8
		add			r8,r8,r7	

		lvsr		v9,r4,r8				//load alignment vector for stores

		stvewx		v4,r4,r9
		add			r9,r9,r7	
		vperm		v5,v5,v5,v9

		stvewx		v5,r4,r8
		add			r8,r8,r7	

		lvsr		v9,r4,r8				//load alignment vector for stores

		stvewx		v5,r4,r9
		add			r9,r9,r7	
		vperm		v6,v6,v6,v9

		stvewx		v6,r4,r8
		add			r8,r8,r7	

		lvsr		v9,r4,r8				//load alignment vector for stores

		stvewx		v6,r4,r9
		add			r9,r9,r7	
		vperm		v7,v7,v7,v9

		stvewx		v7,r4,r8
						
		stvewx		v7,r4,r9

	}	
}

/****************************************************************************
 * 
 *  ROUTINE       :     PPCReconInterHalfPixel2_Altivec( PB_INSTANCE *pbi, UINT8* dest, UINT8* r, UINT8* s, INT16 * diff, UINT32 stride)
 *
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :    
 * 
 *
 *  FUNCTION      :     
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
									      /*	    r3,          r4,             r5,             r6,           r7,            r8  */
void PPCReconInterHalfPixel2_Altivec( PB_INSTANCE *pbi, UINT8* dest, UINT8* RefPtr1, UINT8* RefPtr2, INT16 * diff, UINT32 stride) 
{
	(void) pbi;
	
	asm
	{
//trying cache hints
		lis			r9,0x0108
		or			r9,r9,r8
		dstst		r4,r9,0

		xor			r9,r9,r9
		li			r10,16
		
		lvsl		v8,r5,r9				//load alignment vector for RefPtr1
		vxor		v9,v9,v9
		
		lvx			v10,r5,r9				//get 8 RefPtr1 -- 0
		add			r9,r9,r8					

		lvx			v0,r5,r10				//need another 16 bytes for misaligned data -- 0
		add			r10,r10,r8					

		lvx			v11,r5,r9				//get 8 RefPtr1 -- 1
		vperm		v10,v10,v0,v8

		lvsl		v8,r5,r9				//load alignment vector for RefPtr1
		add			r9,r9,r8					

		lvx			v1,r5,r10				//need another 16 bytes for misaligned data -- 1
		vmrghb		v10,v9,v10				//unsigned byte -> unsigned half		
		add			r10,r10,r8					

		lvx			v12,r5,r9				//get 8 RefPtr1 -- 2
		vperm		v11,v11,v1,v8

		lvsl		v8,r5,r9				//load alignment vector for RefPtr1
		add			r9,r9,r8					

		lvx			v2,r5,r10				//need another 16 bytes for misaligned data -- 2
		vmrghb		v11,v9,v11				//unsigned byte -> unsigned half		
		add			r10,r10,r8					

		lvx			v13,r5,r9				//get 8 RefPtr1 -- 3
		vperm		v12,v12,v2,v8

		lvsl		v8,r5,r9				//load alignment vector for RefPtr1
		add			r9,r9,r8					

		lvx			v3,r5,r10				//need another 16 bytes for misaligned data -- 3
		vmrghb		v12,v9,v12				//unsigned byte -> unsigned half		
		add			r10,r10,r8					

		lvx			v14,r5,r9				//get 8 RefPtr1 -- 4
		vperm		v13,v13,v3,v8

		lvsl		v8,r5,r9				//load alignment vector for RefPtr1
		add			r9,r9,r8					

		lvx			v4,r5,r10				//need another 16 bytes for misaligned data -- 4
		vmrghb		v13,v9,v13				//unsigned byte -> unsigned half		
		add			r10,r10,r8					

		lvx			v15,r5,r9				//get 8 RefPtr1 -- 5
		vperm		v14,v14,v4,v8

		lvsl		v8,r5,r9				//load alignment vector for RefPtr1
		add			r9,r9,r8					

		lvx			v5,r5,r10				//need another 16 bytes for misaligned data -- 5
		vmrghb		v14,v9,v14				//unsigned byte -> unsigned half		
		add			r10,r10,r8					

		lvx			v16,r5,r9				//get 8 RefPtr1 -- 6
		vperm		v15,v15,v5,v8

		lvsl		v8,r5,r9				//load alignment vector for RefPtr1
		add			r9,r9,r8					

		lvx			v6,r5,r10				//need another 16 bytes for misaligned data -- 6
		vmrghb		v15,v9,v15				//unsigned byte -> unsigned half		
		add			r10,r10,r8					

		lvx			v17,r5,r9				//get 8 RefPtr1 -- 7
		vperm		v16,v16,v6,v8

		lvsl		v8,r5,r9				//load alignment vector for RefPtr1
		add			r9,r9,r8					

		lvx			v7,r5,r10				//need another 16 bytes for misaligned data -- 7
		vmrghb		v16,v9,v16				//unsigned byte -> unsigned half		
		add			r10,r10,r8					
//--------
		vperm		v17,v17,v7,v8
		xor			r9,r9,r9
		li			r10,16

		lvsl		v18,r6,r9				//load alignment vector for RefPtr2
		vmrghb		v17,v9,v17				//unsigned byte -> unsigned half		
		
		lvx			v20,r6,r9				//get 8 RefPtr2 -- 0
		add			r9,r9,r8					

		lvx			v0,r6,r10				//need another 16 bytes for misaligned data -- 0
		add			r10,r10,r8					

		lvx			v21,r6,r9				//get 8 RefPtr2 -- 1
		vperm		v20,v20,v0,v18

		lvsl		v18,r6,r9				//load alignment vector for RefPtr2
		add			r9,r9,r8					

		lvx			v1,r6,r10				//need another 16 bytes for misaligned data -- 1
		vmrghb		v20,v9,v20				//unsigned byte -> unsigned half		
		add			r10,r10,r8					

		lvx			v22,r6,r9				//get 8 RefPtr2 -- 2
		vperm		v21,v21,v1,v18

		lvsl		v18,r6,r9				//load alignment vector for RefPtr2
		add			r9,r9,r8					

		lvx			v2,r6,r10				//need another 16 bytes for misaligned data -- 2
		vmrghb		v21,v9,v21				//unsigned byte -> unsigned half	
		vadduhm		v10,v10,v20	
		add			r10,r10,r8					

		lvx			v23,r6,r9				//get 8 RefPtr2 -- 3
		vperm		v22,v22,v2,v18

		lvsl		v18,r6,r9				//load alignment vector for RefPtr2
		add			r9,r9,r8					

		lvx			v3,r6,r10				//need another 16 bytes for misaligned data -- 3
		vmrghb		v22,v9,v22				//unsigned byte -> unsigned half		
		vadduhm		v11,v11,v21	
		add			r10,r10,r8					

		lvx			v24,r6,r9				//get 8 RefPtr2 -- 4
		vperm		v23,v23,v3,v18

		lvsl		v18,r6,r9				//load alignment vector for RefPtr2
		add			r9,r9,r8					

		lvx			v4,r6,r10				//need another 16 bytes for misaligned data -- 4
		vmrghb		v23,v9,v23				//unsigned byte -> unsigned half		
		vadduhm		v12,v12,v22	
		add			r10,r10,r8					

		lvx			v25,r6,r9				//get 8 RefPtr2 -- 5
		vperm		v24,v24,v4,v18

		lvsl		v18,r6,r9				//load alignment vector for RefPtr2
		add			r9,r9,r8					

		lvx			v5,r6,r10				//need another 16 bytes for misaligned data -- 5
		vmrghb		v24,v9,v24				//unsigned byte -> unsigned half		
		vadduhm		v13,v13,v23	
		add			r10,r10,r8					

		lvx			v26,r6,r9				//get 8 RefPtr2 -- 6
		vperm		v25,v25,v5,v18

		lvsl		v18,r6,r9				//load alignment vector for RefPtr2
		add			r9,r9,r8					

		lvx			v6,r6,r10				//need another 16 bytes for misaligned data -- 6
		vmrghb		v25,v9,v25				//unsigned byte -> unsigned half		
		vadduhm		v14,v14,v24	
		add			r10,r10,r8					

		lvx			v27,r6,r9				//get 8 RefPtr2 -- 7
		vperm		v26,v26,v6,v18

		lvsl		v18,r6,r9				//load alignment vector for RefPtr2
		add			r9,r9,r8					

		lvx			v7,r6,r10				//need another 16 bytes for misaligned data -- 7
		vmrghb		v26,v9,v26				//unsigned byte -> unsigned half		
		vadduhm		v15,v15,v25	
		add			r10,r10,r8					

		vperm		v27,v27,v7,v18
		xor			r9,r9,r9

		vmrghb		v27,v9,v27				//unsigned byte -> unsigned half		
		vadduhm		v16,v16,v26	

		vadduhm		v17,v17,v27	
		vspltish	v8,1
//--------
		lvx			v0,r7,r9				//get 8 shorts 				
		vsrh		v10,v10,v8
		addi		r9,r9,16

		lvx			v1,r7,r9				//get 8 shorts 				
		vsrh		v11,v11,v8
		addi		r9,r9,16

		lvx			v2,r7,r9				//get 8 shorts 				
		vsrh		v12,v12,v8
		addi		r9,r9,16

		lvx			v3,r7,r9				//get 8 shorts 				
		vsrh		v13,v13,v8
		addi		r9,r9,16

		lvx			v4,r7,r9				//get 8 shorts 				
		vsrh		v14,v14,v8
		addi		r9,r9,16

		lvx			v5,r7,r9				//get 8 shorts 				
		vsrh		v15,v15,v8
		addi		r9,r9,16

		lvx			v6,r7,r9				//get 8 shorts 				
		vsrh		v16,v16,v8
		addi		r9,r9,16

		lvx			v7,r7,r9				//get 8 shorts 				
		vsrh		v17,v17,v8
		xor			r9,r9,r9
//--------
		lvsr		v9,r4,r9				//load alignment vector for stores
		vaddshs		v0,v0,v10

		vaddshs		v1,v1,v11
		vpkshus		v0,v0,v0				

		vaddshs		v2,v2,v12
		vpkshus		v1,v1,v1				

		vaddshs		v3,v3,v13
		vpkshus		v2,v2,v2				

		vaddshs		v4,v4,v14
		vpkshus		v3,v3,v3				

		vaddshs		v5,v5,v15
		vpkshus		v4,v4,v4				

		vaddshs		v6,v6,v16
		vpkshus		v5,v5,v5				

		vaddshs		v7,v7,v17
		vpkshus		v6,v6,v6				

		vpkshus		v7,v7,v7

		li			r10,4
		vperm		v0,v0,v0,v9				//adjust for writes

		stvewx		v0,r4,r9
		add			r9,r9,r8	

		lvsr		v9,r4,r9				//load alignment vector for stores

		stvewx		v0,r4,r10
		add			r10,r10,r8	
		vperm		v1,v1,v1,v9

		stvewx		v1,r4,r9
		add			r9,r9,r8	

		lvsr		v9,r4,r9				//load alignment vector for stores

		stvewx		v1,r4,r10
		add			r10,r10,r8	
		vperm		v2,v2,v2,v9

		stvewx		v2,r4,r9
		add			r9,r9,r8	

		lvsr		v9,r4,r9				//load alignment vector for stores

		stvewx		v2,r4,r10
		add			r10,r10,r8	
		vperm		v3,v3,v3,v9

		stvewx		v3,r4,r9
		add			r9,r9,r8	

		lvsr		v9,r4,r9				//load alignment vector for stores

		stvewx		v3,r4,r10
		add			r10,r10,r8	
		vperm		v4,v4,v4,v9

		stvewx		v4,r4,r9
		add			r9,r9,r8	

		lvsr		v9,r4,r9				//load alignment vector for stores

		stvewx		v4,r4,r10
		add			r10,r10,r8	
		vperm		v5,v5,v5,v9

		stvewx		v5,r4,r9
		add			r9,r9,r8	

		lvsr		v9,r4,r9				//load alignment vector for stores

		stvewx		v5,r4,r10
		add			r10,r10,r8	
		vperm		v6,v6,v6,v9

		stvewx		v6,r4,r9
		add			r9,r9,r8	

		lvsr		v9,r4,r9				//load alignment vector for stores

		stvewx		v6,r4,r10
		add			r10,r10,r8	
		vperm		v7,v7,v7,v9

		stvewx		v7,r4,r9

		stvewx		v7,r4,r10






	}	
}

//#pragma profile on

