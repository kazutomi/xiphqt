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
 *   Module Title :     PPoptfunctions.c
 *
 *   Description  :     Optimized functions for PostProcessor
 *
 *
 *****************************************************************************
 */


/****************************************************************************
*  Header Frames
*****************************************************************************
*/

#if defined(POSTPROCESS)

#define STRICT              /* Strict type checking. */
#include <memory.h>

#include "pbdll.h"
#include "blockmapping.h"
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

extern vector unsigned char vPerm2;

/****************************************************************************
*  Explicit Imports
*****************************************************************************
*/              

extern INT32 SharpenModifier[];

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
 *  ROUTINE       :     DeringBlockStrong_Altivec()
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering a block for deringing purpose
 *
 *  SPECIAL NOTES :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void DeringBlockStrong_Altivec( 
							  PB_INSTANCE *pbi, 			    /* r3 */
							  unsigned char *SrcPtr,	/* r4 */
							 unsigned char *DstPtr,		/* r5 */
							  INT32 Pitch,				/* r6 */
							  UINT32 FragQIndex,		/* r7 */
							  UINT32 *QuantScalePtr)    /* r8 */
{

	/*Note: compiler uses r28 for vrsave and r11 for SP */
	asm
	{
        lwz	        r9,SharpenModifier
		vspltish	v27,1
		
		slwi		r7,r7,2		    				//index into long table
		vspltish	v26,7

		lvewx		v29,r8,r7
		vslh		v18,v27,v26						//128 aka atot
		vspltish	v23,12

		lvsl		v24,r8,r7
		vslh		v23,v23,v27						//24
		vspltish	v26,6
		
		lvewx		v16,r9,r7
		vspltish	v21,8
		vxor		v0,v0,v0
				
		lvsl		v6,r9,r7
		vadduhm		v21,v21,v23						//32
		vperm		v29,v29,v29,v24	

		vslh		v7,v27,v26						//64 aka B
		vspltish	v31,3

		vperm		v16,v16,v16,v6	
		vxor		v15,v15,v15

        vsubuhm     v26,v15,v7                       //-64
		vsplth		v29,v29,1						//dup QStep aka QValue
		li			r31,8							//loop counter

		vsplth		v16,v16,1	   					//dup Sharpen
		vmladduhm	v31,v29,v31,v0					//High = 3*QValue
		xor			r9,r9,r9

        vxor        v30,v30,v30                     //Low = 0
		subf		r29,r6,r9						//-pitch
		
		vadduhm		v29,v29,v21						//QValue + 32
		vspltish	v19,8
		li			r10,-1
		
		vslh		v19,v7,v19						//16384
		li			r12,1

		vcmpgtsh	v0,v31,v21						//High > 32

		vsel		v31,v31,v21,v0					//use 32 if true
		
		lvsl		v20,r4,r9						//alignment vector for p
		lvsl		v21,r4,r10						//alignment vector for pl
		lvsl		v22,r4,r12						//alignment vector for pr
		lvsl		v23,r4,r29						//alignment vector for pu
		lvsl		v24,r4,r6						//alignment vector for pd

	doRow:
		lvx			v0,r4,r9						//p
		vadduhm		v28,v18,v15						//atot = 128
		add.		r31,r31,r10						//dec counter

		lvx			v1,r4,r10						//pl
		vadduhm		v27,v7,v15						//B = round = 64

		lvx			v2,r4,r12						//pr
		vperm		v0,v0,v0,v20					//aligned p
		addi		r12,r12,8
		
		lvx			v5,r4,r12
		vperm		v1,v1,v0,v21					//aligned pl 
		addi		r12,r12,-8

		lvx			v3,r4,r29						//pu
		vmrghb		v0,v15,v0
		
		lvx			v4,r4,r6						//pd
		vmrghb		v1,v15,v1

		vsubuhs		v8,v0,v1						//p - pl
		vperm		v2,v2,v5,v22					//aligned pr

		vsubuhs		v9,v1,v0						//pl - p
		vperm		v3,v3,v3,v23					//aligned pu

		vadduhm     v11,v8,v9                       //abs(p - pl)
		vmrghb		v3,v15,v3

		vsubuhs		v8,v0,v3						//p - pu
		vperm		v4,v4,v4,v24					//aligned pd

		vsubuhs		v9,v3,v0						//pu - p
		vmrghb		v2,v15,v2

		vadduhm     v12,v8,v9                       //abs(p - pu)
		vmrghb		v4,v15,v4

		vsubuhs		v8,v0,v4						//p - pd
		vsubuhs		v9,v4,v0						//pd - p
		vadduhm     v13,v8,v9                       //abs(p - pd)

		vsubuhs		v8,v0,v2						//p - pr
		vsubuhs		v9,v2,v0						//pr - p
		vadduhm     v14,v8,v9                       //abs(p - pr)
// -->
		vsubuhm		v11,v29,v11                     //aka TmpMod
		vcmpgtsh	v17,v11,v31						//check high clamp
		vsel		v8,v11,v31,v17					
		vcmpgtsh	v17,v30,v11						//check < 0
		vsel		v8,v8,v30,v17						
		vcmpgtsh	v17,v26,v11						//check < -64
		vsel		v8,v8,v16,v17						
		vsubuhm		v28,v28,v8						//atot -= al				
		vmladduhm	v27,v8,v1,v27					//B += al * pl
// <--
// -->
		vsubuhm		v12,v29,v12                     //aka TmpMod
		vcmpgtsh	v17,v12,v31						//check high clamp
		vsel		v8,v12,v31,v17					
		vcmpgtsh	v17,v30,v12						//check < 0
		vsel		v8,v8,v30,v17						
		vcmpgtsh	v17,v26,v12						//check < -64
		vsel		v8,v8,v16,v17						
		vsubuhm		v28,v28,v8						//atot -= au				
		vmladduhm	v27,v8,v3,v27					//B += au * pu
// <--
// -->
		vsubuhm		v13,v29,v13                     //aka TmpMod
		vcmpgtsh	v17,v13,v31						//check high clamp
		vsel		v8,v13,v31,v17					
		vcmpgtsh	v17,v30,v13						//check < 0
		vsel		v8,v8,v30,v17						
		vcmpgtsh	v17,v26,v13						//check < -64
		vsel		v8,v8,v16,v17						
		vsubuhm		v28,v28,v8						//atot -= ad				
		vmladduhm	v27,v8,v4,v27					//B += ad * pd
// <--
// -->
		vsubuhm		v14,v29,v14                     //aka TmpMod
		vcmpgtsh	v17,v14,v31						//check high clamp
		vsel		v8,v14,v31,v17					
		vcmpgtsh	v17,v30,v14						//check < 0
		vsel		v8,v8,v30,v17						
		vcmpgtsh	v17,v26,v14						//check < -64
		vsel		v8,v8,v16,v17						
		vsubuhm		v28,v28,v8						//atot -= ar				
		vmladduhm	v27,v8,v2,v27					//B += ar * pr
// <--

		xor 		r0,r0,r0
		add			r4,r4,r6		

		lvsr		v1,r5,r0
		vmladduhm	v28,v28,v0,v27					//(atot * p) + B
		vspltish	v0,7

		lvsl		v20,r4,r9						//alignment vector for p
		vadduhm		v28,v28,v19						//add 16384

		lvsl		v21,r4,r10						//alignment vector for pl
		vsubuhs		v28,v28,v19						//sub 16384 and clamp to zero
		
		lvsl		v22,r4,r12						//alignment vector for pr
		vsrh		v28,v28,v0						//((atot * p) + B) >> 7		

		lvsl		v23,r4,r29						//alignment vector for pu
		vpkshus		v28,v28,v28						//newVal
		
		lvsl		v24,r4,r6						//alignment vector for pd
		vperm		v28,v28,v28,v1

		stvewx		v28,r5,r0
		addi		r0,r0,4
		
		stvewx		v28,r5,r0
		add			r5,r5,r6
		bne			doRow
	}
}
/****************************************************************************
 * 
 *  ROUTINE       :     DeringBlockWeak_Altivec()
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Filtering a block for deringing purpose
 *
 *  SPECIAL NOTES :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void DeringBlockWeak_Altivec( 
							  PB_INSTANCE *pbi, 			    /* r3 */
							  unsigned char *SrcPtr,	/* r4 */
							 unsigned char *DstPtr,		/* r5 */
							  INT32 Pitch,				/* r6 */
							  UINT32 FragQIndex,		/* r7 */
							  UINT32 *QuantScalePtr)    /* r8 */
{

	/*Note: compiler uses r28 for vrsave and r11 for SP */
	asm
	{
        lwz	        r9,SharpenModifier
		vspltish	v27,1
		
		slwi		r7,r7,2		    				//index into long table
		vspltish	v26,7

		lvewx		v29,r8,r7
		vslh		v18,v27,v26						//128 aka atot
		vspltish	v23,12

		lvsl		v24,r8,r7
		vslh		v23,v23,v27						//24
		vspltish	v26,6
		
		lvewx		v16,r9,r7
		vspltish	v21,8
		vxor		v0,v0,v0
				
		lvsl		v6,r9,r7
		vadduhm		v21,v21,v23						//32
		vperm		v29,v29,v29,v24	

		vslh		v7,v27,v26						//64 aka B
		vspltish	v31,3

		vperm		v16,v16,v16,v6	
		vxor		v15,v15,v15

        vsubuhm     v26,v15,v7                       //-64
		vsplth		v29,v29,1						//dup QStep aka QValue
		li			r31,8							//loop counter

		vsplth		v16,v16,1	   					//dup Sharpen
		vmladduhm	v31,v29,v31,v0					//High = 3*QValue
		xor			r9,r9,r9

        vxor        v30,v30,v30                     //Low = 0
		subf		r29,r6,r9						//-pitch
		
		vadduhm		v29,v29,v21						//QValue + 32
		vspltish	v19,8
		li			r10,-1
		
		vslh		v19,v7,v19						//16384
		li			r12,1

		vcmpgtsh	v0,v31,v23						//High > 24

		vsel		v31,v31,v23,v0					//use 24 if true
		
		lvsl		v20,r4,r9						//alignment vector for p
		lvsl		v21,r4,r10						//alignment vector for pl
		lvsl		v22,r4,r12						//alignment vector for pr
		lvsl		v23,r4,r29						//alignment vector for pu
		lvsl		v24,r4,r6						//alignment vector for pd

	doRow:
		lvx			v0,r4,r9						//p
		vadduhm		v28,v18,v15						//atot = 128
		add.		r31,r31,r10						//dec counter

		lvx			v1,r4,r10						//pl
		vadduhm		v27,v7,v15						//B = round = 64

		lvx			v2,r4,r12						//pr
		vperm		v0,v0,v0,v20					//aligned p
		addi		r12,r12,8
		
		lvx			v5,r4,r12
		vperm		v1,v1,v0,v21					//aligned pl 
		addi		r12,r12,-8

		lvx			v3,r4,r29						//pu
		vmrghb		v0,v15,v0
		
		lvx			v4,r4,r6						//pd
		vmrghb		v1,v15,v1

		vsubuhs		v8,v0,v1						//p - pl
		vperm		v2,v2,v5,v22					//aligned pr

		vsubuhs		v9,v1,v0						//pl - p
		vperm		v3,v3,v3,v23					//aligned pu

		vadduhm     v11,v8,v9                       //abs(p - pl)
		vmrghb		v3,v15,v3

		vsubuhs		v8,v0,v3						//p - pu
		vperm		v4,v4,v4,v24					//aligned pd

		vsubuhs		v9,v3,v0						//pu - p
		vmrghb		v2,v15,v2

		vadduhm     v12,v8,v9                       //abs(p - pu)
		vmrghb		v4,v15,v4

		vsubuhs		v8,v0,v4						//p - pd
		vsubuhs		v9,v4,v0						//pd - p
		vadduhm     v13,v8,v9                       //abs(p - pd)

		vsubuhs		v8,v0,v2						//p - pr
		vsubuhs		v9,v2,v0						//pr - p
		vadduhm     v14,v8,v9                       //abs(p - pr)

        vadduhm     v11,v11,v11                     //2 * abs(p - pl)       
        vadduhm     v12,v12,v12                     //2 * abs(p - pu)       
        vadduhm     v13,v13,v13                     //2 * abs(p - pd)       
        vadduhm     v14,v14,v14                     //2 * abs(p - pr)       

// -->
		vsubuhm		v11,v29,v11                     //aka TmpMod
		vcmpgtsh	v17,v11,v31						//check high clamp
		vsel		v8,v11,v31,v17					
		vcmpgtsh	v17,v30,v11						//check < 0
		vsel		v8,v8,v30,v17						
		vcmpgtsh	v17,v26,v11						//check < -64
		vsel		v8,v8,v16,v17						
		vsubuhm		v28,v28,v8						//atot -= al				
		vmladduhm	v27,v8,v1,v27					//B += al * pl
// <--
// -->
		vsubuhm		v12,v29,v12                     //aka TmpMod
		vcmpgtsh	v17,v12,v31						//check high clamp
		vsel		v8,v12,v31,v17					
		vcmpgtsh	v17,v30,v12						//check < 0
		vsel		v8,v8,v30,v17						
		vcmpgtsh	v17,v26,v12						//check < -64
		vsel		v8,v8,v16,v17						
		vsubuhm		v28,v28,v8						//atot -= au				
		vmladduhm	v27,v8,v3,v27					//B += au * pu
// <--
// -->
		vsubuhm		v13,v29,v13                     //aka TmpMod
		vcmpgtsh	v17,v13,v31						//check high clamp
		vsel		v8,v13,v31,v17					
		vcmpgtsh	v17,v30,v13						//check < 0
		vsel		v8,v8,v30,v17						
		vcmpgtsh	v17,v26,v13						//check < -64
		vsel		v8,v8,v16,v17						
		vsubuhm		v28,v28,v8						//atot -= ad				
		vmladduhm	v27,v8,v4,v27					//B += ad * pd
// <--
// -->
		vsubuhm		v14,v29,v14                     //aka TmpMod
		vcmpgtsh	v17,v14,v31						//check high clamp
		vsel		v8,v14,v31,v17					
		vcmpgtsh	v17,v30,v14						//check < 0
		vsel		v8,v8,v30,v17						
		vcmpgtsh	v17,v26,v14						//check < -64
		vsel		v8,v8,v16,v17						
		vsubuhm		v28,v28,v8						//atot -= ar				
		vmladduhm	v27,v8,v2,v27					//B += ar * pr
// <--

		xor 		r0,r0,r0
		add			r4,r4,r6		

		lvsr		v1,r5,r0
		vmladduhm	v28,v28,v0,v27					//(atot * p) + B
		vspltish	v0,7

		lvsl		v20,r4,r9						//alignment vector for p
		vadduhm		v28,v28,v19						//add 16384

		lvsl		v21,r4,r10						//alignment vector for pl
		vsubuhs		v28,v28,v19						//sub 16384 and clamp to zero
		
		lvsl		v22,r4,r12						//alignment vector for pr
		vsrh		v28,v28,v0						//((atot * p) + B) >> 7		

		lvsl		v23,r4,r29						//alignment vector for pu
		vpkshus		v28,v28,v28						//newVal
		
		lvsl		v24,r4,r6						//alignment vector for pd
		vperm		v28,v28,v28,v1

		stvewx		v28,r5,r0
		addi		r0,r0,4
		
		stvewx		v28,r5,r0
		add			r5,r5,r6
		bne			doRow
	}
}

#endif