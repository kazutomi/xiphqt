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
*   Module Title :     quantindex_dx.c
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

/*
UINT32 dequant_index[64] = 
{	0,  1,  8,  16,  9,  2,  3, 10,
	17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

*/
#if 1 //defined(NEW_DEQUANTIZATION)

static UINT32 dequant_indexAltivec[64] = 
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


extern UINT32 dequant_index[64];
/*
    do not unravel
*/
static UINT32 transIndex_Altivec[64] = 
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

#define TI(x) transIndex_Altivec[ dequant_indexAltivec[x] ]



/****************************************************************************
 * 
 *  ROUTINE       :     BuildQuantIndex_ForAltivec
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
void BuildQuantIndex_ForAltivec(PB_INSTANCE *pbi)
{
    INT32 i,j;
    
    

    pbi->transIndex = transIndex_Altivec;

    // invert the dequant index into the quant index
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
        j = TI(i);
		pbi->quant_index[j] = i;

	}
}
#endif


/****************************************************************************
 * 
 *  ROUTINE       :     BuildQuantIndex_ForPPC
 *
 *  INPUTS        :     
 *                      
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Builds the quant_index table.  
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
 
static UINT32 transIndex_PPC[64];

extern UINT32 dequant_index[64];

 
void BuildQuantIndex_ForPPC(PB_INSTANCE *pbi)
{
    INT32 i,j;

    pbi->transIndex = transIndex_PPC;

    // invert the dequant index into the quant index
	for ( i = 0; i < BLOCK_SIZE; i++ )
	{	
		transIndex_PPC[i] = i;
		
        j = dequant_index[i];
		pbi->quant_index[j] = i;
	}
}