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
*   Module Title :     DCT_Encode.c
*
*   Description  :     Codec specific functions
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Frames
*****************************************************************************
*/


#define STRICT              /* Strict type checking. */
#include <math.h>

#include "compdll.h"
#include "SystemDependant.h"
#include "dct.h"
#include "Misc_common.h"
#include "Huffman.h"
#include "Quantize.h"
#include "stdio.h"
/****************************************************************************
*  Module constants.
*****************************************************************************
*/        
#define MAX_RUN_VAL     3

/****************************************************************************
*  Explicit Imports
*****************************************************************************
*/              
extern void PredictMotionBlock(
    PB_INSTANCE *pbi, 
    UINT32 FragmentNumber, 
    INT32 MvShift, 
    INT32 MvModMask, 
    UINT32 ReconPixelIndex, 
    INT16 *OutputPtr,
    UINT32 ReconPixelsPerLine);

extern void SubtractBlock(
    UINT8 *Src,
    INT16 *Dst,
    UINT32 ReconPixelsPerLine);


/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/


/* Last Inter frame DC values */
/****************************************************************************
*  Exported Functions
*****************************************************************************
*/              

/****************************************************************************
*  Module Statics
*****************************************************************************
*/              

static INT32    NewBetter = 0;
static INT32    OldBetter = 0;
static INT32    ScoreDiff = 0;
//static double	DCT_codes[64];      
//static INT32	DCTDataBuffer[64];

/****************************************************************************
*  Foreward References
*****************************************************************************
*/              

UINT8 TokenizeDctBlock( INT16 * RawData, UINT32 * TokenListPtr );
UINT8 TokenizeDctValue( INT16 DataValue, UINT32 * TokenListPtr  );
UINT8 TokenizeDctRunValue( UINT8 RunLength, INT16 DataValue, UINT32 * TokenListPtr  );

BOOL AllZeroDctData( Q_LIST_ENTRY * QuantList );

void SUB8( UINT8 *FiltPtr, UINT8 *ReconPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine );
void SUB8_128( UINT8 *FiltPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine );
void SUB8AV2( UINT8 *FiltPtr, UINT8 *ReconPtr1, UINT8 *ReconPtr2, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
              UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine );

/****************************************************************************
 * 
 *  ROUTINE       :     Sub8
 *
 *  INPUTS        :     
 *						
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Subtracts 2 8x8 blocks
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void SUB8( UINT8 *FiltPtr, UINT8 *ReconPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
          UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine )
{
    int i;

    // For each block row
    for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ )
    {
        DctInputPtr[0] = (INT16)((int)(FiltPtr[0]) - ((int)ReconPtr[0]) );
        DctInputPtr[1] = (INT16)((int)(FiltPtr[1]) - ((int)ReconPtr[1]) );
        DctInputPtr[2] = (INT16)((int)(FiltPtr[2]) - ((int)ReconPtr[2]) );
        DctInputPtr[3] = (INT16)((int)(FiltPtr[3]) - ((int)ReconPtr[3]) );
        DctInputPtr[4] = (INT16)((int)(FiltPtr[4]) - ((int)ReconPtr[4]) );
        DctInputPtr[5] = (INT16)((int)(FiltPtr[5]) - ((int)ReconPtr[5]) );
        DctInputPtr[6] = (INT16)((int)(FiltPtr[6]) - ((int)ReconPtr[6]) );
        DctInputPtr[7] = (INT16)((int)(FiltPtr[7]) - ((int)ReconPtr[7]) );
        
        // Update the screen canvas in one step
        ((UINT32*)old_ptr1)[0] = ((UINT32*)new_ptr1)[0];
        ((UINT32*)old_ptr1)[1] = ((UINT32*)new_ptr1)[1];
        
        // Start next row
        new_ptr1 += PixelsPerLine;
        old_ptr1 += PixelsPerLine;
        FiltPtr += PixelsPerLine;
        ReconPtr += ReconPixelsPerLine;
        DctInputPtr += BLOCK_HEIGHT_WIDTH;
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     Sub8_128
 *
 *  INPUTS        :     
 *						
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Subtracts 2 8x8 blocks
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void SUB8_128( UINT8 *FiltPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine )
{
    int i;
    // For each block row
    for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ )
    {
        // INTRA mode so code raw image data
        // We convert the data to 8 bit signed (by subtracting 128)
        // as this reduces the internal precision requirments in the
        // DCT transform.
        DctInputPtr[0] = (INT16)((int)(FiltPtr[0]) - 128);
        DctInputPtr[1] = (INT16)((int)(FiltPtr[1]) - 128);
        DctInputPtr[2] = (INT16)((int)(FiltPtr[2]) - 128);
        DctInputPtr[3] = (INT16)((int)(FiltPtr[3]) - 128);
        DctInputPtr[4] = (INT16)((int)(FiltPtr[4]) - 128);
        DctInputPtr[5] = (INT16)((int)(FiltPtr[5]) - 128);
        DctInputPtr[6] = (INT16)((int)(FiltPtr[6]) - 128);
        DctInputPtr[7] = (INT16)((int)(FiltPtr[7]) - 128);
        
        // Update the screen canvas in one step
        //memcpy( old_ptr1, new_ptr1, BLOCK_HEIGHT_WIDTH ); 
        ((UINT32*)old_ptr1)[0] = ((UINT32*)new_ptr1)[0];
        ((UINT32*)old_ptr1)[1] = ((UINT32*)new_ptr1)[1];
        
        // Start next row
        new_ptr1 += PixelsPerLine;
        old_ptr1 += PixelsPerLine;
        FiltPtr += PixelsPerLine;
        DctInputPtr += BLOCK_HEIGHT_WIDTH;
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     Sub8AV2
 *
 *  INPUTS        :     
 *						
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Subtracts 2 8x8 blocks
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void SUB8AV2( UINT8 *FiltPtr, UINT8 *ReconPtr1, UINT8 *ReconPtr2, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
              UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine )
{
    int i;

    // For each block row
    for ( i=0; i<BLOCK_HEIGHT_WIDTH; i++ )
    {   
        DctInputPtr[0] = (INT16)((int)(FiltPtr[0]) - (((int)ReconPtr1[0] + (int)ReconPtr2[0]) / 2) );
        DctInputPtr[1] = (INT16)((int)(FiltPtr[1]) - (((int)ReconPtr1[1] + (int)ReconPtr2[1]) / 2) );
        DctInputPtr[2] = (INT16)((int)(FiltPtr[2]) - (((int)ReconPtr1[2] + (int)ReconPtr2[2]) / 2) );
        DctInputPtr[3] = (INT16)((int)(FiltPtr[3]) - (((int)ReconPtr1[3] + (int)ReconPtr2[3]) / 2) );
        DctInputPtr[4] = (INT16)((int)(FiltPtr[4]) - (((int)ReconPtr1[4] + (int)ReconPtr2[4]) / 2) );
        DctInputPtr[5] = (INT16)((int)(FiltPtr[5]) - (((int)ReconPtr1[5] + (int)ReconPtr2[5]) / 2) );
        DctInputPtr[6] = (INT16)((int)(FiltPtr[6]) - (((int)ReconPtr1[6] + (int)ReconPtr2[6]) / 2) );
        DctInputPtr[7] = (INT16)((int)(FiltPtr[7]) - (((int)ReconPtr1[7] + (int)ReconPtr2[7]) / 2) );
        
        // Update the screen canvas in one step
        //memcpy( old_ptr1, new_ptr1, BLOCK_HEIGHT_WIDTH ); 
        ((UINT32*)old_ptr1)[0] = ((UINT32*)new_ptr1)[0];
        ((UINT32*)old_ptr1)[1] = ((UINT32*)new_ptr1)[1];
        
        // Start next row
        new_ptr1 += PixelsPerLine;
        old_ptr1 += PixelsPerLine;
        FiltPtr += PixelsPerLine;
        ReconPtr1 += ReconPixelsPerLine;
        ReconPtr2 += ReconPixelsPerLine;
        DctInputPtr += BLOCK_HEIGHT_WIDTH;
    }
}


/****************************************************************************
 * 
 *  ROUTINE       :     DPCMTokenizeBlock
 *
 *  INPUTS        :     UINT32	FragIndex
 *						UINT32	StepToNextRow
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Codes a DCT block
 *
 *                      Motion vectors and modes asumed to be defined at the MB level.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 DPCMTokenizeBlock  ( CP_INSTANCE *cpi, INT32 FragIndex, UINT32 PixelsPerLine )
{
    //Q_LIST_ENTRY *qlist_ptr;		// Pointer to quantised coefficient list
    //UINT32	*enc_frag_ptr;			// Pointer into coded data buffer
    UINT32  token_count;
	//UINT32	i;		        		// Loop counters
    Q_LIST_ENTRY  TempLastDC = 0;

		
	if ( GetFrameType(&cpi->pb) == BASE_FRAME )
	{
		// Key frame so code block in INTRA mode.
		cpi->pb.CodingMode = CODE_INTRA;
	}
	else
	{
	    // Get Motion vector and mode for this block.
        cpi->pb.CodingMode = cpi->pb.FragCodingMethod[FragIndex];
	}


    // Tokenise the dct data. 
    token_count = cpi->TokenizeDctBlock( cpi->pb.QFragData[FragIndex], cpi->pb.TokenList[FragIndex] );
    
    cpi->FragTokenCounts[FragIndex] = token_count;
    cpi->TotTokenCount += token_count;

    // Return number of pixels coded (i.e. 8x8).
	return BLOCK_SIZE;
}

/****************************************************************************
 * 
 *  ROUTINE       :     AllZeroDctData
 *
 *  INPUTS        :     Quantized DCT data
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     TRUE if there is no non zero dct data to code..
 *
 *  FUNCTION      :     Checks for case where all DCT data will be zero.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
BOOL AllZeroDctData( Q_LIST_ENTRY * QuantList )
{
    UINT32 i;
    BOOL ret_val = TRUE;

    for ( i = 0; i < 64; i ++ )
    {
        if ( QuantList[i] != 0 )
        {
            ret_val = FALSE;
            break;
        }
    }

    return ret_val;
}


/****************************************************************************
 * 
 *  ROUTINE       :     TokenizeDctBlock
 *
 *  INPUTS        :     UINT8 * RawData
 *                      INT16 * TokenListPtr
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     Number of tokens used including additional bits tokens.
 *
 *  FUNCTION      :     Encodes a DCT block into a stream of tokens.
 *                      most tokens are followed by an additional bits
 *                      token that can contain up to 8 bits of aditional data.
 *
 *  SPECIAL NOTES :     Only encodes run value for the value 0. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT8 TokenizeDctBlock( INT16 * RawData, UINT32 * TokenListPtr )
{
    UINT32 i;  
    UINT8  run_count;    
    UINT8  token_count = 0;     // Number of tokens crated. 
    UINT32 AbsData;
	

    // Tokenize the block 
    for( i = 0; i < BLOCK_SIZE; i++ )
    {   
        run_count = 0;  

        // Look for a zero run. 
		// NOTE the use of & instead of && which is faster (and equivalent) in this instance. 
        while( (i < BLOCK_SIZE) & (!RawData[i]) )
        {
            run_count++; 
            i++;
        }

        // If we have reached the end of the block then code EOB 
        if ( i == BLOCK_SIZE )
        {
            TokenListPtr[token_count] = DCT_EOB_TOKEN;    
            token_count++;
        }
        else
        {
            // If we have a short zero run followed by a low data value code the two as a composite token.
            if ( run_count )
            {
                AbsData = abs(RawData[i]);
    
                if ( ((AbsData == 1) && (run_count <= 17)) || 
                     ((AbsData <= 3) && (run_count <= 3)) )
                {
                    /* Tokenise the run and subsequent value combination value */
                    token_count += TokenizeDctRunValue( run_count, RawData[i], &TokenListPtr[token_count] );
                }

                // Else if we have a long non-EOB run or a run followed by a value token > MAX_RUN_VAL
                // then code the run and token seperately
                else 
                {
                    if ( run_count <= 8 )
                        TokenListPtr[token_count] = DCT_SHORT_ZRL_TOKEN;
                    else
                        TokenListPtr[token_count] = DCT_ZRL_TOKEN;

                    token_count++;
                    TokenListPtr[token_count] = run_count - 1;    
                    token_count++;

                    // Now tokenize the value
                    token_count += TokenizeDctValue( RawData[i], &TokenListPtr[token_count] );
                }
            }
            // Else there was NO zero run. 
            else
            {
                // Tokenise the value 
                token_count += TokenizeDctValue( RawData[i], &TokenListPtr[token_count] );
            }
        }
    }

    /* Return the total number of tokens (including additional bits tokens) used. */
    return token_count;
}


/****************************************************************************
 * 
 *  ROUTINE       :     TokenizeDctValue
 *
 *  INPUTS        :     INT32       DataValue
 *                      UINT32 *    TokenListPtr
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     The number of tokens used including any for additional bits.
 *
 *  FUNCTION      :     Encodes a value as a DCT token and any additional bits token.
 *
 *  SPECIAL NOTES :    
 *
 *
 *  ERRORS        :     None.
 *
 ***************************************************************************/
UINT8  TokenizeDctValue( INT16 DataValue, UINT32 * TokenListPtr  )
{
    UINT8 tokens_added = 0;
    UINT32 AbsDataVal = abs( (INT32)DataValue );

    /* Values are tokenised as category value and a number of additional bits
    *  that define the position within the category.
    */
    if ( DataValue == 0 )
    {
        // ERROR
        IssueWarning( "Bad Input to TokenizeDctValue" );
    } 
    else if ( AbsDataVal == 1 )
    {
        if ( DataValue == 1 )
            TokenListPtr[0] = ONE_TOKEN; 
        else
            TokenListPtr[0] = MINUS_ONE_TOKEN; 
        tokens_added = 1;
    }
    else if ( AbsDataVal == 2 )
    {
        if ( DataValue == 2 )
            TokenListPtr[0] = TWO_TOKEN; 
        else
            TokenListPtr[0] = MINUS_TWO_TOKEN; 
        tokens_added = 1;
    }
    else if ( AbsDataVal <= MAX_SINGLE_TOKEN_VALUE )
    {   
        TokenListPtr[0] = LOW_VAL_TOKENS + (AbsDataVal - DCT_VAL_CAT2_MIN); 
        if ( DataValue > 0 )
            TokenListPtr[1] = 0;
        else
            TokenListPtr[1] = 1;
        tokens_added = 2;
    }
    // Bit 1 determines sign, Bit 0 the value
    else if ( AbsDataVal <= 8 )
    {
        TokenListPtr[0] = DCT_VAL_CATEGORY3; 
        if ( DataValue > 0 )
            TokenListPtr[1] = (AbsDataVal - DCT_VAL_CAT3_MIN);
        else
            TokenListPtr[1] = (0x02) + (AbsDataVal - DCT_VAL_CAT3_MIN);
        tokens_added = 2;
    }
    // Bit 2 determines sign, Bit 0-2 the value
    else if ( AbsDataVal <= 12 )
    {
        TokenListPtr[0] = DCT_VAL_CATEGORY4; 
        if ( DataValue > 0 )
            TokenListPtr[1] = (AbsDataVal - DCT_VAL_CAT4_MIN);
        else
            TokenListPtr[1] = (0x04) + (AbsDataVal - DCT_VAL_CAT4_MIN);
        tokens_added = 2;
    }
    // Bit 3 determines sign, Bit 0-2 the value
    else if ( AbsDataVal <= 20 )
    {
        TokenListPtr[0] = DCT_VAL_CATEGORY5;    
        if ( DataValue > 0 )
            TokenListPtr[1] = (AbsDataVal - DCT_VAL_CAT5_MIN);
        else
            TokenListPtr[1] = (0x08) + (AbsDataVal - DCT_VAL_CAT5_MIN);
        tokens_added = 2;
    }
    // Bit 4 determines sign, Bit 0-3 the value
    else if ( AbsDataVal <= 36 )
    {
        TokenListPtr[0] = DCT_VAL_CATEGORY6;    
        if ( DataValue > 0 )
            TokenListPtr[1] = (AbsDataVal - DCT_VAL_CAT6_MIN);
        else
            TokenListPtr[1] = (0x010) + (AbsDataVal - DCT_VAL_CAT6_MIN);
        tokens_added = 2;
    }
    // Bit 5 determines sign, Bit 0-4 the value
    else if ( AbsDataVal <= 68 )
    {
        TokenListPtr[0] = DCT_VAL_CATEGORY7;    
        if ( DataValue > 0 )
            TokenListPtr[1] = (AbsDataVal - DCT_VAL_CAT7_MIN);
        else
            TokenListPtr[1] = (0x20) + (AbsDataVal - DCT_VAL_CAT7_MIN);
        tokens_added = 2;
    }
    // Bit 9 determines sign, Bit 0-8 the value
    else if ( AbsDataVal <= 511 )
    {
        TokenListPtr[0] = DCT_VAL_CATEGORY8;    
        if ( DataValue > 0 )
            TokenListPtr[1] = (AbsDataVal - DCT_VAL_CAT8_MIN);
        else
            TokenListPtr[1] = (0x200) + (AbsDataVal - DCT_VAL_CAT8_MIN);
        tokens_added = 2;
    }
    else 
    {
        TokenListPtr[0] = DCT_VAL_CATEGORY8;    
        if ( DataValue > 0 )
            TokenListPtr[1] = (511 - DCT_VAL_CAT8_MIN);
        else
            TokenListPtr[1] = (0x200) + (511 - DCT_VAL_CAT8_MIN);
        tokens_added = 2;

        tokens_added = 2;  // ERROR  
        IssueWarning( "Bad Input to TokenizeDctValue" );
    }

    /* Return the total number of tokens added */
    return tokens_added;
}


/****************************************************************************
 * 
 *  ROUTINE       :     TokenizeDctRunValue
 *
 *  INPUTS        :     UINT8    RunLength
 *                      INT32    DataValue
 *                      UINT32 * TokenListPtr
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     The number of tokens used including any for additional bits.
 *
 *  FUNCTION      :     Encodes a value as a DCT token and any additional bits token.
 *
 *  SPECIAL NOTES :     
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT8 TokenizeDctRunValue( UINT8 RunLength, INT16 DataValue, UINT32 * TokenListPtr  )
{
    UINT8 tokens_added = 0;
    UINT32 AbsDataVal = abs( (INT32)DataValue );

    /* Values are tokenised as category value and a number of additional bits
    *  that define the category.
    */
    if ( DataValue == 0 )
    {
        // ERROR
        IssueWarning( "Bad Input to TokenizeDctRunValue" );
    } 
    else if ( AbsDataVal == 1 )
    {   
        // Zero runs of 1-5
        if ( RunLength <= 5 )
        {
            TokenListPtr[0] = DCT_RUN_CATEGORY1 + (RunLength - 1);  
            if ( DataValue > 0 )
                TokenListPtr[1] = 0;
            else
                TokenListPtr[1] = 1;
        }
        // Zero runs of 6-9
        else if ( RunLength <= 9 ) 
        {
            TokenListPtr[0] = DCT_RUN_CATEGORY1B;  
            if ( DataValue > 0 )
                TokenListPtr[1] = (RunLength - 6);
            else
                TokenListPtr[1] = 0x04 + (RunLength - 6);
        }
        // Zero runs of 10-17
        else
        {
            TokenListPtr[0] = DCT_RUN_CATEGORY1C;  
            if ( DataValue > 0 )
                TokenListPtr[1] = (RunLength - 10);
            else
                TokenListPtr[1] = 0x08 + (RunLength - 10);
        }
        tokens_added = 2;
    }
    else if ( AbsDataVal <= 3 )
    {
        if ( RunLength == 1 )
        {
            TokenListPtr[0] = DCT_RUN_CATEGORY2; 

            // Extra bits token bit 1 indicates sign, bit 0 indicates value
            if ( DataValue > 0 )
                TokenListPtr[1] = (AbsDataVal - 2);
            else
                TokenListPtr[1] = (0x02) + (AbsDataVal - 2);
            tokens_added = 2;
        }
        else
        {
            TokenListPtr[0] = DCT_RUN_CATEGORY2 + 1; 

            // Extra bits token.
            // bit 2 indicates sign, bit 1 indicates value, bit 0 indicates run length
            if ( DataValue > 0 )
                TokenListPtr[1] = ((AbsDataVal - 2) << 1) + (RunLength - 2);
            else
                TokenListPtr[1] = (0x04) + ((AbsDataVal - 2) << 1) + (RunLength - 2);
            tokens_added = 2;
        }
    }
    else 
    {
        tokens_added = 2;  // ERROR  
        IssueWarning( "Bad Input to TokenizeDctRunValue" );
    }

    /* Return the total number of tokens added */
    return tokens_added;
}

/****************************************************************************
 * 
 *  ROUTINE       :     MotionBlockDifferrences
 *
 *  INPUTS        :     None
 *                               
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Compute the motion block residues, after filtering
 *                      internal edges in the reference block
 *                      
 *  SPECIAL NOTES :     
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************
 */

void MotionBlockDifference(CP_INSTANCE * cpi, UINT8 * FiltPtr, INT16 *DctInputPtr, INT32 MvDevisor, 
                           UINT8* old_ptr1, UINT8* new_ptr1, UINT32 FragIndex,UINT32 PixelsPerLine, 
                           UINT32 ReconPixelsPerLine)
{

    INT32 MvShift;
    INT32 MvModMask; 
    UINT32 ReconPixelIndex = GetFragIndex(cpi->pb.recon_pixel_index_table,FragIndex);


    switch(MvDevisor)
    {
        case(2):
            MvShift = 1;
            MvModMask = 1;
            break;
        case(4):
            MvShift = 2;
            MvModMask = 3;
            break;
        default:
            break;
    }

    {
        INT32   AbsRefOffset;
        INT32   AbsXOffset;             
        INT32   AbsYOffset;
        INT32   MVOffset;               // Baseline motion vector offset
        INT32   ReconPtr2Offset;        // Offset for second reconstruction in half pixel MC
    	UINT8   *ReconPtr1;             // DCT reconstructed image pointers
	    UINT8   *ReconPtr2;             // Pointer used in half pixel MC

        cpi->MVector.x = cpi->pb.FragMVect[FragIndex].x;
        cpi->MVector.y = cpi->pb.FragMVect[FragIndex].y;

        // Set up the baseline offset for the motion vector.
        MVOffset = ((cpi->MVector.y / MvDevisor) * ReconPixelsPerLine) + (cpi->MVector.x / MvDevisor);
        
        // Work out the offset of the second reference position for 1/2 pixel interpolation.
        // For the U and V planes the MV specifies 1/4 pixel accuracy. This is adjusted to
        // 1/2 pixel as follows ( 0->0, 1/4->1/2, 1/2->1/2, 3/4->1/2 ).
        ReconPtr2Offset = 0;
        AbsXOffset = cpi->MVector.x % MvDevisor;
        AbsYOffset = cpi->MVector.y % MvDevisor;

        if ( AbsXOffset )
        {
            if ( cpi->MVector.x > 0 )
                ReconPtr2Offset += 1;
            else 
                ReconPtr2Offset -= 1;
        }
        
        if ( AbsYOffset )
        {
            if ( cpi->MVector.y > 0 )
                ReconPtr2Offset += ReconPixelsPerLine;
            else
                ReconPtr2Offset -= ReconPixelsPerLine;
        }
        
        if ( cpi->pb.CodingMode==CODE_GOLDEN_MV )
        {
            ReconPtr1 = &cpi->pb.GoldenFrame[
                GetFragIndex(cpi->pb.recon_pixel_index_table, FragIndex)];
        }
        else
        {
            ReconPtr1 = &cpi->pb.LastFrameRecon[
                GetFragIndex(cpi->pb.recon_pixel_index_table, FragIndex)];
        }
        
        ReconPtr1 += MVOffset;
        ReconPtr2 =  ReconPtr1 + ReconPtr2Offset;
        
        AbsRefOffset = abs((int)(ReconPtr1 - ReconPtr2));
        
        // Is the MV offset exactly pixel alligned
        if ( AbsRefOffset == 0 )
        {
            cpi->Sub8( FiltPtr, ReconPtr1, DctInputPtr, old_ptr1, new_ptr1, PixelsPerLine, ReconPixelsPerLine );
        }
        // Fractional pixel MVs.
        // Note that we only use two pixel values even for the diagonal
        else
        {
            cpi->Sub8Av2(FiltPtr, ReconPtr1,ReconPtr2,DctInputPtr, old_ptr1, new_ptr1, PixelsPerLine, ReconPixelsPerLine );
        }
        
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     TransformQuantizeBlock
 *
 *  INPUTS        :     UINT32	FragIndex
 *						UINT32	StepToNextRow
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Codes a DCT block
 *
 *                      Motion vectors and modes asumed to be defined at the MB level.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void TransformQuantizeBlock( CP_INSTANCE *cpi, INT32 FragIndex, UINT32 PixelsPerLine )
{
	UINT8	*new_ptr1;	            // Pointers into current frame
    UINT8	*old_ptr1;	            // Pointers into old frame
    UINT8	*FiltPtr;               // Pointers to srf filtered pixels 
	//INT32   *DctInputPtr;		    // Pointer into buffer containing input to DCT
	INT16   *DctInputPtr;		    // Pointer into buffer containing input to DCT
	BOOL	LeftEdge;				// Flag if block at left edge of component
    UINT32  ReconPixelsPerLine;     // Line length for recon buffers.

	Q_LIST_ENTRY  TempLastDC = 0;
	double  GF_Error = 0.0;
	double  InterNMV_Error = 0.0;
	double  IntraScore = 0.0;
    double  BestInterError = 0.0;	// Measure of the best error score available
									// by application of a limited motion vector.
	UINT8   *ReconPtr1;             // DCT reconstructed image pointers
    INT32   MvDevisor;              // Defines MV resolution (2 = 1/2 pixel for Y or 4 = 1/4 for UV)

	// Initialise pointers
    new_ptr1 = &cpi->yuv1ptr[GetFragIndex(cpi->pb.pixel_index_table,FragIndex)];    
    old_ptr1 = &cpi->yuv0ptr[GetFragIndex(cpi->pb.pixel_index_table,FragIndex)];    

	DctInputPtr	= cpi->DCTDataBuffer;

  	// Set plane specific values
    if (  FragIndex < (INT32)cpi->pb.YPlaneFragments )
    {
        ReconPixelsPerLine = cpi->pb.Configuration.YStride;
        MvDevisor = 2;                  // 1/2 pixel accuracy in Y
    }
    else
    {
        ReconPixelsPerLine = cpi->pb.Configuration.UVStride;
        MvDevisor = 4;                  // UV planes at 1/2 resolution of Y
    }

    // adjusted / filtered pointers
    FiltPtr = &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,FragIndex)];

    if ( GetFrameType(&cpi->pb) == BASE_FRAME )
	{
		// Key frame so code block in INTRA mode.
		cpi->pb.CodingMode = CODE_INTRA;
	}
	else
	{
	    // Get Motion vector and mode for this block.
        cpi->pb.CodingMode = cpi->pb.FragCodingMethod[FragIndex];
	}

    // Selection of Quantiser matirx and set other plane related values.		
	if ( FragIndex < (INT32)cpi->pb.YPlaneFragments )
	{
		LeftEdge = !(FragIndex%cpi->pb.HFragments);

        // Select the approrpriate Y quantiser matrix
        if ( cpi->pb.CodingMode == CODE_INTRA )
		    select_Y_quantiser(&cpi->pb);
        else
		    select_Inter_quantiser(&cpi->pb);
	}
	else
	{
		LeftEdge = !((FragIndex-cpi->pb.YPlaneFragments)%(cpi->pb.HFragments>>1));

        // Select the approrpriate UV quantiser matrix
        if ( cpi->pb.CodingMode == CODE_INTRA )
		    select_UV_quantiser(&cpi->pb);
        else
		    select_Inter_quantiser(&cpi->pb);
	}
	
    if ( ModeUsesMC[cpi->pb.CodingMode] )
	{

        MotionBlockDifference(cpi, FiltPtr, DctInputPtr, MvDevisor,
            old_ptr1, new_ptr1, FragIndex, PixelsPerLine, ReconPixelsPerLine);

	}
    else if ( (cpi->pb.CodingMode==CODE_INTER_NO_MV ) || ( cpi->pb.CodingMode==CODE_USING_GOLDEN ) )
    {
        if ( cpi->pb.CodingMode==CODE_INTER_NO_MV )
        {
	        ReconPtr1 = &cpi->pb.LastFrameRecon[GetFragIndex(cpi->pb.recon_pixel_index_table,FragIndex)];
        }
        else
        {
	        ReconPtr1 = &cpi->pb.GoldenFrame[GetFragIndex(cpi->pb.recon_pixel_index_table,FragIndex)];
        }

		cpi->Sub8( FiltPtr, ReconPtr1, DctInputPtr, old_ptr1, new_ptr1, PixelsPerLine, ReconPixelsPerLine );
        
        
    }
	else if ( cpi->pb.CodingMode==CODE_INTRA )
	{
	    
		cpi->Sub8_128(FiltPtr, DctInputPtr, old_ptr1, new_ptr1, PixelsPerLine);

	}
    else
    {
        IssueWarning( "Illegal coding mode" );
    }

    // Proceed to encode the data into the encode buffer if the encoder is enabled.   
    // Perform a 2D DCT transform on the data.
	cpi->fdct_short( cpi->DCTDataBuffer, cpi->DCT_codes );

	// Quantize that transform data.
	quantize ( &cpi->pb, cpi->DCT_codes, cpi->pb.QFragData[FragIndex] );   

	if ( (cpi->pb.CodingMode == CODE_INTER_NO_MV) && ( AllZeroDctData(cpi->pb.QFragData[FragIndex]) ) )
	{
		cpi->pb.display_fragments[FragIndex] = 0;
	}

}

