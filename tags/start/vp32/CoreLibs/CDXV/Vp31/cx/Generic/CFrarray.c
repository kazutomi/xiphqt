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
*   Module Title :     CFrarray.C
*
*   Description  :     Functions to read and write fragment arrays.
*
*****************************************************************************
*/

/****************************************************************************
*  Header Frames
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include <string.h> 

#include "SystemDependant.h"
#include "BlockMapping.h"
#include "CBitman.h"
#include "CFrameW.h"
#include "CFrarray.h"
#include "compdll.h"

#include <stdio.h>

/****************************************************************************
*  Module constants.
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
*  Forward references
*****************************************************************************
*/


UINT32 FrArrayCodeSBRun( CP_INSTANCE *cpi, UINT32 value );
UINT32 FrArrayCodeBlockRun( CP_INSTANCE *cpi, UINT32 value );

/****************************************************************************
*  Module Statics
*****************************************************************************
*/              

/****************************************************************************
*  Explicit imports
*****************************************************************************
*/  


/****************************************************************************
 * 
 *  ROUTINE       :     PackAndWriteDFArray
 *
 *  INPUTS        :     The compressor instance.
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Packs and writes the list of coded/uncoded blocks
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void PackAndWriteDFArray( CP_INSTANCE *cpi )
{
    UINT32  i;
    UINT8	val;
    UINT32  run_count; 
	
    UINT32	SB, MB, B;				   // Block, MB and SB loop variables
    
    UINT32  LastSbMbIndex = 0;         // Last SB's MbIndex value
    UINT32  BListIndex = 0;            // Block index in coding list
    UINT32  LastSbBIndex = 0;          // Last SB's BIndex value

    INT32   DfBlockIndex;              // Block index in display_fragments

    // Initialise workspaces
    memset( cpi->pb.SBFullyFlags, 1, cpi->pb.SuperBlocks);
    memset( cpi->pb.SBCodedFlags, 0, cpi->pb.SuperBlocks );
    memset( cpi->PartiallyCodedFlags, 0, cpi->pb.SuperBlocks );
    memset( cpi->BlockCodedFlags, 0, cpi->pb.UnitFragments);

	for( SB = 0; SB < cpi->pb.SuperBlocks; SB++ )
    {
        // Check for coded blocks and macro-blocks
		for ( MB=0; MB<4; MB++ )
		{
            // If MB in frame
			if ( QuadMapToMBTopLeft(cpi->pb.BlockMap,SB,MB) >= 0 )
			{
				for ( B=0; B<4; B++ )
				{
        			DfBlockIndex = QuadMapToIndex1( cpi->pb.BlockMap,SB, MB, B );

					// Does Block lie in frame:
					if ( DfBlockIndex >= 0 )
					{
                        // In Frame: If it is not coded then this SB is only partly coded.:
						if ( cpi->pb.display_fragments[DfBlockIndex] )
                        {
                            cpi->pb.SBCodedFlags[SB] = 1;           // SB at least partly coded
                            cpi->BlockCodedFlags[BListIndex] = 1;// Block is coded
                        }
                        else
                        {
                            cpi->pb.SBFullyFlags[SB] = 0;           // SB not fully coded
                            cpi->BlockCodedFlags[BListIndex] = 0;// Block is not coded  
                        }
                        
                        BListIndex++;
					}				
				}
            }
        }

        // Is the SB fully coded or uncoded. 
        // If so then backup BListIndex and MBListIndex
        if ( cpi->pb.SBFullyFlags[SB] || !cpi->pb.SBCodedFlags[SB] )
        {
            BListIndex = LastSbBIndex;                      // Reset to values from previous SB
        }
        else
        {
            cpi->PartiallyCodedFlags[SB] = 1;                         // Set up list of partially coded SBs
            LastSbBIndex = BListIndex;
        }
    }

    // Code list of partially coded Super-Block. 
    val = cpi->PartiallyCodedFlags[0];
    AddBitsToBuffer( cpi, (UINT32)val, 1);
    i = 0;
    while ( i < cpi->pb.SuperBlocks )
    {
        run_count = 0;
        while ( (i < cpi->pb.SuperBlocks) && (cpi->PartiallyCodedFlags[i] == val) )
        {
            i++;
            run_count++;
        }      
    
        // Code the run
        FrArrayCodeSBRun( cpi, run_count );
        val = ( val == 0 ) ? 1 : 0;
	}

    // RLC Super-Block fully/not coded. 
    i = 0;
    while( (i < cpi->pb.SuperBlocks) && cpi->PartiallyCodedFlags[i] )              // Skip partially coded blocks
        i++;

    if ( i < cpi->pb.SuperBlocks )
    {
        val = cpi->pb.SBFullyFlags[i];              
        AddBitsToBuffer( cpi, (UINT32)val, 1);

        while ( i < cpi->pb.SuperBlocks )
        {
            run_count = 0;
            while ( (i < cpi->pb.SuperBlocks) && (cpi->pb.SBFullyFlags[i] == val) )
            {
                i++;
                while( (i < cpi->pb.SuperBlocks) && cpi->PartiallyCodedFlags[i] )  // Skip partially coded blocks
                    i++;
                run_count++;
            }      
    
            // Code the run
            FrArrayCodeSBRun( cpi, run_count );
            val = ( val == 0 ) ? 1 : 0;
        }
    }
    
    //  Now code the block flags
    if ( BListIndex > 0 )
    {
        // Code the block flags start value
	    val = cpi->BlockCodedFlags[0];
	    AddBitsToBuffer( cpi, (UINT32)val, 1);

        // Now code the block flags.
        for ( i = 0; i < BListIndex; )
        {
			run_count = 0;
            while ( (cpi->BlockCodedFlags[i] == val) && (i < BListIndex) )
            {
                i++;
                run_count++;
            }   
            
            FrArrayCodeBlockRun( cpi, run_count );
            val = ( val == 0 ) ? 1 : 0;

        }
    }

}

/****************************************************************************
 * 
 *  ROUTINE       :     FrArrayCodeSBRun
 *
 *  INPUTS        :     UINT32 value
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     The number of bits encoded. 
 *
 *  FUNCTION      :     Encodes a SB run value
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 FrArrayCodeSBRun( CP_INSTANCE *cpi, UINT32 value )
{
    UINT32 CodedVal = 0;
    UINT32 CodedBits = 0;

    // Coding scheme:
	//	Codeword				RunLength
    //  0                       1
	//	10x					    2-3
	//	110x					4-5
	//	1110xx				    6-9
    //  11110xxx                10-17
    //  111110xxxx              18-33
    //  111111xxxxxxxxxxxx      34-4129
	if ( value == 1 )
	{
		CodedVal = 0;    
		CodedBits = 1;   
	}
    else if ( value <= 3 )
	{
		CodedVal = 0x0004 + (value - 2);    
		CodedBits = 3;   
	}         
	else if ( value <= 5 )
	{
		CodedVal = 0x000C + (value - 4);    
		CodedBits = 4;   
	}   
	else if ( value <= 9 )
	{
		CodedVal = 0x0038 + (value - 6);    
		CodedBits = 6;   
	}    
	else if ( value <= 17 )
	{
		CodedVal = 0x00F0 + (value - 10);    
		CodedBits = 8;   
	}    
	else if ( value <= 33 )
	{
		CodedVal = 0x03E0 + (value - 18);    
		CodedBits = 10;   
	}    
	else 
	{
		CodedVal = 0x3F000 + (value - 34);    
		CodedBits = 18;
	}

    /* Add the bits to the encode holding buffer. */    
    AddBitsToBuffer( cpi, CodedVal, (UINT32)CodedBits );

    return CodedBits;
}

/****************************************************************************
 * 
 *  ROUTINE       :     FrArrayCodeBlockRun
 *
 *  INPUTS        :     UINT32 value
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     The number of bits encoded. 
 *
 *  FUNCTION      :     Encodes a block run value
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 FrArrayCodeBlockRun( CP_INSTANCE *cpi, UINT32 value )
{
    UINT32 CodedVal = 0;
    UINT32 CodedBits = 0;

    // Coding scheme:
	//	Codeword				RunLength
	//	0x						1-2
	//	10x						3-4
	//	110x					5-6
	//	1110xx					7-10
	//	11110xx					11-14
	//	11111xxxx				15-30 	

	if ( value <= 2 )
	{
		CodedVal = value - 1;    
		CodedBits = 2;   
	}
    else if ( value <= 4 )
	{
		CodedVal = 0x0004 + (value - 3);    
		CodedBits = 3;   

	}         
    else if ( value <= 6 )
	{
		CodedVal = 0x000C + (value - 5);    
		CodedBits = 4;   

	}         
    else if ( value <= 10 )
	{
		CodedVal = 0x0038 + (value - 7);    
		CodedBits = 6;   

	}         
    else if ( value <= 14 )
	{
		CodedVal = 0x0078 + (value - 11);    
		CodedBits = 7;   
	} 
    else
    {
		CodedVal = 0x01F0 + (value - 15);    
		CodedBits = 9;   
    }

    /* Add the bits to the encode holding buffer. */    
    AddBitsToBuffer( cpi, CodedVal, (UINT32)CodedBits );

    return CodedBits;
}

