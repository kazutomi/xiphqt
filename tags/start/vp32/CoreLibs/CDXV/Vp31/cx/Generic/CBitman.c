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
*   Module Title :     BIT_MAN.C
*
*   Description  :     Video CODEC  : Bit manipulation routines.
*
*               
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */

#include "CBitman.h"
#include "CFrameW.h"

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
 
       
/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/


/****************************************************************************
*  Module Static Variables
*****************************************************************************
*/

static UINT32 ExBitMasks[33] = { 0x00000000, 
                                 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 
                                 0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF, 
                                 0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF, 
                                 0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF, 
                                 0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF, 
                                 0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF, 
                                 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 
                                 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF };
/*
// copied from cbitman.c
static INT32 byte_bit_offset;     

static UINT32 DataBlock;  
static UINT32 mybits; 
static UINT32 ByteBitsLeft;
*/

/****************************************************************************
 * 
 *  ROUTINE       :     InitAddBitsToBuffer
 *
 *  INPUTS        :     UINT8 * encode_buffer
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None
 *
 *
 *  FUNCTION      :     Initialises addition to the given buffer. 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
__inline void InitAddBitsToBuffer(CP_INSTANCE *cpi)
{                                       
    cpi->byte_bit_offset = 31;
    cpi->DataBlock = 0;
}

/****************************************************************************
 * 
 *  ROUTINE       :     AddBitsToBuffer
 *
 *  INPUTS        :     UINT32 data
 *                      UINT32 bits
 *                      
 *
 *  OUTPUTS       :    
 *
 *  RETURNS       :     None
 *
 *
 *  FUNCTION      :     Adds the given data bits to the encoded data buffer. 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
__inline void AddBitsToBuffer( CP_INSTANCE *cpi, UINT32 data, UINT32 bits )
{
    cpi->mybits = bits; 
    cpi->ByteBitsLeft = (cpi->byte_bit_offset + 1);
    
    /* If too many bits to fit in current byte. */
    if ( cpi->mybits > cpi->ByteBitsLeft )
    {
        /* Add in as many as will fit. */
        cpi->DataBlock |= (UINT32)(data >> (cpi->mybits - cpi->ByteBitsLeft));   
        cpi->mybits -= cpi->ByteBitsLeft;

      	Write32ToBuffer( cpi, (UINT8 *)&cpi->DataBlock );
        cpi->DataBlock = 0;
        
        /* Now add in the remainder. */ 
        cpi->byte_bit_offset = 31;      
        if ( cpi->mybits > 0 )
        {
            data &= ExBitMasks[cpi->mybits];
            cpi->DataBlock |= data << (32 - cpi->mybits); 
            cpi->byte_bit_offset -= cpi->mybits;
        }
    }    
    else  
    {
        cpi->DataBlock |= (UINT32)(data << (cpi->ByteBitsLeft - cpi->mybits)); 
        cpi->byte_bit_offset -= cpi->mybits;
        if ( cpi->byte_bit_offset < 0 )     
        {
            cpi->byte_bit_offset = 31;
      	    Write32ToBuffer( cpi, (UINT8 *)&cpi->DataBlock );
            cpi->DataBlock = 0;
        }       
    }

} 


/****************************************************************************
 * 
 *  ROUTINE       :     EndAddBitsToBuffer
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None
 *
 *
 *  FUNCTION      :     Terminates a bit packing run
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void EndAddBitsToBuffer(CP_INSTANCE *cpi)
{      
    if ( cpi->byte_bit_offset != 31 )
    {
        cpi->byte_bit_offset = 31;
      	Write32ToBuffer( cpi, (UINT8 *)&cpi->DataBlock );
        cpi->DataBlock = 0;
    }
}





