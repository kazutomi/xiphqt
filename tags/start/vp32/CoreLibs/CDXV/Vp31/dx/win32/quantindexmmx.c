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
*   Module Title :     quantindexmmx.c
*
*   Description  :     
*
*
*****************************************************************************
*/						

/****************************************************************************
*  Header Frames
*****************************************************************************
*/
#define STRICT              /* Strict type checking. */
//#include <memory.h>  
#include "pbdll.h"

/****************************************************************************
*  Module constants.
*****************************************************************************
*/ 
       
/****************************************************************************
*  Imported Functions
*****************************************************************************
*/

/****************************************************************************
*  Imported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Foreward References
*****************************************************************************
*/    
          

/****************************************************************************
*  Module Statics
*****************************************************************************
*/
 
static UINT32 dequant_indexMMX[64] = 
{
    0,  1,   5,  6, 14, 15, 27, 28,
    2,  4,   7, 13, 16, 26, 29, 42,
    3,  8,  12, 17, 25, 30, 41, 43,
    9,  11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54, 
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};
/*
    used to unravel the coeffs in the proper order required by MMX_idct 
    see mmxidct.cxx
*/
static UINT32 transIndexMMX[64] = 
{
     0,  8,  1,  2,    9, 16, 24, 17,
    10,  3, 32, 11,   18, 25,  4, 12,
     5, 26, 19, 40,   33, 34, 41, 48,
    27,  6, 13, 20,   28, 21, 14,  7,

    56, 49, 42, 35,   43, 50, 57, 36, 
    15, 22, 29, 30,   23, 44, 37, 58,
    51, 59, 38, 45,   52, 31, 60, 53,
    46, 39, 47, 54,   61, 62, 55, 63
};

static UINT32 transIndexWMT[64] = 
{	
	 0,  8,  1,  2,   9, 16, 24, 17,
	10,  3,  4, 11,	 18, 25, 32, 40,
    33, 26, 19, 12,   5,  6, 13, 20,
    27, 34, 41, 48,  56, 49, 42, 35,
    28, 21, 14,  7,  15, 22, 29, 36, 
    43, 50, 57, 58,  51, 44, 37, 30,
    23, 31, 38, 45,  52, 59, 60, 53,
    46, 39, 47, 54,  61, 62, 55, 63
};



/****************************************************************************
 * 
 *  ROUTINE       :     BuildQuantIndex_ForMMX
 *
 *  INPUTS        :     
 *                      
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Builds the quant_index table in a transposed order.  
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void BuildQuantIndex_ForMMX(PB_INSTANCE *pbi)
{
    INT32 i,j;

    pbi->transIndex = transIndexMMX;

    // invert the dequant index into the quant index
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
        j = transIndexMMX[ dequant_indexMMX[i] ];
		pbi->quant_index[j] = i;
	}
}


/****************************************************************************
 * 
 *  ROUTINE       :     BuildQuantIndex_ForWMT
 *
 *  INPUTS        :     
 *                      
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Builds the quant_index table in a transposed order.  
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void BuildQuantIndex_ForWMT(PB_INSTANCE *pbi)
{
    INT32 i,j;

    pbi->transIndex = transIndexWMT;

    // invert the dequant index into the quant index
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
        j = transIndexWMT[ dequant_indexMMX[i] ];
		pbi->quant_index[j] = i;
	}
}


