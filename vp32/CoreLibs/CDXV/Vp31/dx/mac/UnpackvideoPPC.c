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

#include "pbdll.h"
#include "Huffman.h"

#include "type_aliases.h"
#include "codec_common_interface.h"

#include <memory.h>
#include <string.h>


// write the coeffs in a transposed order.  
#define TI(x)  pbi->transIndex[x]


// for debugging purposes only
#define WC(x) x

                   /*        r3,            r4,              r5,            r6,               r7   */ 
void ReadToken(PB_INSTANCE *pbi, BITREADER *br, Q_LIST_ENTRY *Q, HUFF_ENTRY *h, UINT32 FragIndex) 
{
	
	INT32 t, a, c;		// token and optional argument
	HUFF_ENTRY *HH = (HUFF_ENTRY *)&h;

	// mac version of Tim's huffman decode
	// the C compiler automatically saves r13 - r31...  
	asm {
		lwz		r13,br 
		
		lwz		r14,HH
		li		r12,-1
		
		lwz		r15,BITREADER.bitsinremainder(r13)
		xor		r10,r10,r10
	
		lwz		r16,BITREADER.remainder(r13)
		xor		r19,r19,r19							//fork index
		b		nextB
		
exactW:
		stw		r18,t
		xor		r15,r15,r15		
		b		doneW

nextW:
		lwz		r17,BITREADER.position(r13)
		or.		r18,r18,r18		
		bge		exactW
		
		lwz		r16,0(r17)
		addi	r17,r17,4

		rlwinm  r10,r16,1,31,31						//rotate out bit and save
		li		r15,31
		
		stw		r17,BITREADER.position(r13)
		slwi	r16,r16,1
		rlwinm	r19,r10,2,29,29						//fork bit * 4

nextB:		
		lwzx	r14,r14,r19							//get next fork
		xor		r19,r19,r19
		rlwinm  r10,r16,1,31,31						//rotate out bit and save
		
		lwz		r18,HUFF_ENTRY.Value(r14)
		slwi	r16,r16,1								//make sure source gets updated

		add.	r15,r15,r12							//dec num bits left
		blt		nextW
		
		rlwinm	r19,r10,2,29,29						//fork bit * 4
		or.		r18,r18,r18
		blt		nextB		
		
 		add		r16,r16,r10
		addi	r15,r15,1

		stw		r18,t
		rotrwi	r16,r16,1								//put bit back in

doneW:
		stw		r15,BITREADER.bitsinremainder(r13)
	
		stw		r16,BITREADER.remainder(r13)
	}


//NOTE: may have to preserve registers....  C compiler may not do it
//

// mac version of Tim's get macro
#	define get( N, L1, L2) \
	 asm { \
		lwz		r13,br 						; \
		li		r19,N 						; \
 		lwz		r15,BITREADER.bitsinremainder(r13) 	; \
		lwz		r16,BITREADER.remainder(r13) 			; \
		cmpw	r15,r19						; \
		blt		L1							; \
		subf	r15,r19,r15					; \
		rlwinm  r10,r16,N,0,31			 	; \
		stw		r10,BITREADER.remainder(r13)			; \
		rlwinm  r16,r16,N,32-(N),31			; \
		stw		r15,BITREADER.bitsinremainder(r13)	; \
		b		L2							; \
L1: 										; \
		lwz		r17,BITREADER.position(r13) 			; \
		li		r0,32						; \
		subf	r19,r15,r19					/* N - bitsInY */; \
		lwz		r10,0(r17)					; \
		addi	r17,r17,4						; \
		subf	r11,r15,r0					/* used to shift old bits right */; \
		srw		r16,r16,r11					/* save old bits */; \
		subf	r11,r19,r0				    /* new bits in correct position */; \
		stw		r11,BITREADER.bitsinremainder(r13)	; \
		rlwnm	r16,r16,r19,0,31				/* old bits in correct position */; \
		subf	r0,r19,r0					/* num bits left */; \
		stw		r17,BITREADER.position(r13) 			; \
		srw		r18,r10,r11					; \
		rlwnm 	r10,r10,r19,0,31				/* in position */; \
		or 		r16,r18,r16					/* insert new bits with old */; \
		stw		r10,BITREADER.remainder(r13)			/* top bits for later */; \
L2: 										; \
		stw		r16,a 						; \
	} ;\
	
	
	c = pbi->FragCoefEOB[ FragIndex];
	// Process token t with optional argument a.
	//printf("t = %d\n", t);
	
	switch( t) {
	case DCT_EOB_TOKEN:
endB:  
		c = 63;  
		break;
		
	case DCT_EOB_PAIR_TOKEN: pbi->EOB_Run = 1;  goto endB;
	case DCT_EOB_TRIPLE_TOKEN: pbi->EOB_Run = 2;  goto endB;
		
	case DCT_REPEAT_RUN_TOKEN: get( 2, LA, LAA)  pbi->EOB_Run = a + 3;  goto endB;
	case DCT_REPEAT_RUN2_TOKEN: get( 3, LB, LBB)  pbi->EOB_Run = a + 7;  goto endB;
	case DCT_REPEAT_RUN3_TOKEN: get( 4, LC, LCC)  pbi->EOB_Run = a + 15;  goto endB;
	case DCT_REPEAT_RUN4_TOKEN: get( 12, LD, LDD)  pbi->EOB_Run = a - 1;  goto endB;
		
	case DCT_SHORT_ZRL_TOKEN: get( 3, LG, LGG)  c += a;  break;
	case DCT_ZRL_TOKEN: get( 6, LH, LHH)  c += a;  break;
		
	case ONE_TOKEN:		  WC(Q[ TI(c)]) = 1;  break;
	case MINUS_ONE_TOKEN: WC(Q[ TI(c)]) = -1;  break;
	case TWO_TOKEN:		  WC(Q[ TI(c)]) =  2;  break;
	case MINUS_TWO_TOKEN: WC(Q[ TI(c)]) = -2;  break;
		
		// We use the identity -x = (x ^ -1) + 1 repeatedly.
		
	case LOW_VAL_TOKENS:
	case LOW_VAL_TOKENS+1:
	case LOW_VAL_TOKENS+2:
	case LOW_VAL_TOKENS+3: 
		get( 1, LI, LII)
		 t += 3 - LOW_VAL_TOKENS;
		WC(Q[ TI(c)]) = (Q_LIST_ENTRY) ((t ^ -a) + a);
		break;
		
#		define doit( Min, Bits, L1, L2) \
	get( Bits + 1, L1, L2) \
	t = Min + (a & ( (1 << Bits) - 1)); \
	a >>= Bits; \
	WC(Q[ TI(c)]) = (Q_LIST_ENTRY) ((t ^ -a) + a); \
		break;
		
	case DCT_VAL_CATEGORY3: doit( 7, 1, LJ, LJJ)
	case DCT_VAL_CATEGORY4: doit( 9, 2, LK, LKK)
	case DCT_VAL_CATEGORY5: doit( 13, 3, LL, LLL)
	case DCT_VAL_CATEGORY6: doit( 21, 4, LM, LMM)
	case DCT_VAL_CATEGORY7: doit( 37, 5, LN, LNN)
	case DCT_VAL_CATEGORY8: doit( 69, 9, LO, LOO)
#		undef doit
					   
	case DCT_RUN_CATEGORY1:
	case DCT_RUN_CATEGORY1+1:
	case DCT_RUN_CATEGORY1+2:
	case DCT_RUN_CATEGORY1+3:
	case DCT_RUN_CATEGORY1+4: 
		get( 1, LP, LPP)
		c += 1 + t - 23;//Run_M_1;
		WC(Q[ TI(c)]) = (Q_LIST_ENTRY) (1 - a - a);
		break;
		
	case DCT_RUN_CATEGORY1B: 
		get( 3, LQ, LQQ)
		c += 6 + (a & 3);
		WC(Q[ TI(c)]) = 1 - ((a & 4) >> 1);
		break;
		
	case DCT_RUN_CATEGORY1C: 
		get( 4, LR, LRR)
		c += 10 + (a & 7);
		WC(Q[ TI(c)]) = 1 - ((a & 8) >> 2);
		break;
		
	case DCT_RUN_CATEGORY2: 
		get( 2, LU, LUU)
		t = 2 + (a & 1);
		a >>= 1;
		++c;
		WC(Q[ TI(c)]) = (Q_LIST_ENTRY) ((t ^ -a) + a);
		break;
		
	case DCT_RUN_CATEGORY2+1: 
		get( 3, LV, LVV)
		c += 2 + (a & 1);
		t = 2 + ( (a >> 1) & 1);
		a >>= 2;
		WC(Q[ TI(c)]) = (Q_LIST_ENTRY) ((t ^ -a) + a);
		break;
	}
	
	
	//move this out of here and put in readCoeffs ?????????
	if( ++c >= 64)
    {
		--pbi->BlocksToDecode;
    }
	
	
	pbi->FragCoefEOB[ FragIndex ] = (UINT8) c;

	
#	undef swab		// into the ashbin of history
#	undef get
	
}




/****************************************************************************
 * 
 *  ROUTINE       :     UnPackVideoPPC_LL
 *
 *  INPUTS        :     None. 
 *                      
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Upacks and decodes the video stream.
 *
 *  SPECIAL NOTES :     This function calls Assembly codes to read token. 
 *                  
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UnPackVideoPPC_LL (PB_INSTANCE *pbi)
{

    INT32       EncodedCoeffs = 1;
    INT32       FragIndex;
    INT32 *     CodedBlockListPtr;
    INT32 *     CodedBlockListEnd;

    UINT8       AcHuffIndex1;
    UINT8       AcHuffIndex2;

    UINT8       AcHuffChoice;
    UINT8       AcHuffChoice2;
    
    UINT8       DcHuffChoice;    
    UINT8       DcHuffChoice2;
	Q_LIST_ENTRY *	Q;

	
    COEFFNODE startNode;

    COEFFNODE *thisNode;
    COEFFNODE *lastNode;

    // Bail out immediately if a decode error has already been reported.
    if ( pbi->DecoderErrorCode != NO_DECODER_ERROR )
        return;

    // Clear down the array that indicates the current coefficient index for each block.
//    memset(pbi->FragCoeffs, 0, pbi->UnitFragments);
    memset(pbi->FragCoefEOB, 0, pbi->UnitFragments);

    // Clear down the pbi->QFragData structure for all coded blocks.
    // pbi->ClearDownQFrag(pbi);

    // Note the number of blocks to decode
    pbi->BlocksToDecode = pbi->CodedBlockIndex;

    // Get the DC huffman table choice for Y and then UV
    DcHuffChoice = (UINT8)bitread( &pbi->br,  DC_HUFF_CHOICE_BITS ) + DC_HUFF_OFFSET;
    DcHuffChoice2 = (UINT8)bitread( &pbi->br,  DC_HUFF_CHOICE_BITS ) + DC_HUFF_OFFSET;

    // UnPack DC coefficients / tokens
    CodedBlockListPtr = pbi->CodedBlockList;
    CodedBlockListEnd = &pbi->CodedBlockList[pbi->CodedBlockIndex];
 
    /* build link list of all fragments */
    {
        for(FragIndex = 0; FragIndex < pbi->CodedBlockIndex-1; FragIndex++)
        {
            pbi->_Nodes[FragIndex].i = CodedBlockListPtr[FragIndex];
            pbi->_Nodes[FragIndex].next = &pbi->_Nodes[FragIndex + 1];
        }
        pbi->_Nodes[FragIndex].i = CodedBlockListPtr[FragIndex];
        pbi->_Nodes[FragIndex].next = (COEFFNODE *)0;
        
        startNode.i = 0;
        startNode.next = pbi->_Nodes;

        lastNode = &startNode;
        thisNode = startNode.next;
    }

	// ugly tim hack convert bit buffer left justified    
	if( pbi->br.bitsinremainder)
	  	pbi->br.remainder <<= 32 - pbi->br.bitsinremainder;
	else
			pbi->br.remainder = 0;


	pbi->EOB_Run = 0;

	while(thisNode) 
    {
		FragIndex= thisNode->i;

		Q = pbi->QFragData[FragIndex];
        
        thisNode = thisNode->next;
		
        /* clear the block */
		memset( (void *) Q, 0, 128);
		
		if(pbi->EOB_Run) 
		{
			pbi->FragCoefEOB[FragIndex] = 1;
//            pbi->FragCoeffs[FragIndex] = 1;
			--pbi->EOB_Run;
			--pbi->BlocksToDecode; 

            lastNode->next = thisNode;
		} 
		else
        {
            pbi->bumpLast = 1;

            // Select the appropriate huffman table offset according to whether the token is fro am Y or UV block
            if ( FragIndex >= (INT32)pbi->YPlaneFragments )
                DcHuffChoice = DcHuffChoice2;

			ReadToken( pbi, &pbi->br, Q, pbi->HuffRoot_VP3x[DcHuffChoice], FragIndex);

//            if(pbi->FragCoeffs[FragIndex] >= 64)
            if(pbi->FragCoefEOB[FragIndex] >= 64)
            {
                pbi->FragCoefEOB[FragIndex] = 1;
//                pbi->FragCoeffs[FragIndex] = 1;

                pbi->bumpLast = 0;

                if(lastNode)
                    lastNode->next = thisNode;
            }

            if( pbi->bumpLast)
                lastNode = lastNode->next;

        }
	} //while(thisNode) 

	// ugly tim hack convert bit buffer left justified    
	if( pbi->br.bitsinremainder)
	  	pbi->br.remainder >>= 32 - pbi->br.bitsinremainder;
	else
			pbi->br.remainder = 0;


    // Get the AC huffman table choice for Y and then for UV.
    AcHuffIndex1 = (UINT8) bitread( &pbi->br,  AC_HUFF_CHOICE_BITS ) + AC_HUFF_OFFSET;
    AcHuffIndex2 = (unsigned char) bitread( &pbi->br,  AC_HUFF_CHOICE_BITS ) + AC_HUFF_OFFSET;

	// ugly tim hack convert bit buffer left justified    
	if( pbi->br.bitsinremainder)
	  	pbi->br.remainder <<= 32 - pbi->br.bitsinremainder;
	else
			pbi->br.remainder = 0;
	

	// Unpack AC coefficients.
	EncodedCoeffs = 1;					
	do {
        // Huffman table selection based upon which AC coefficient we are on
        AcHuffChoice = AcHuffIndex1;
        AcHuffChoice2 = AcHuffIndex2;

        if ( EncodedCoeffs > AC_TABLE_4_THRESH )
        {
            AcHuffChoice += (AC_HUFF_CHOICES * 3);
            AcHuffChoice2 += (AC_HUFF_CHOICES * 3);
        }
        else if ( EncodedCoeffs > AC_TABLE_3_THRESH )
        {
            AcHuffChoice += (AC_HUFF_CHOICES * 2);
            AcHuffChoice2 += (AC_HUFF_CHOICES * 2);
        }
        else if ( EncodedCoeffs > AC_TABLE_2_THRESH )
        {
            AcHuffChoice += AC_HUFF_CHOICES;
            AcHuffChoice2 += AC_HUFF_CHOICES;
        }

        lastNode = &startNode;
        thisNode = startNode.next;

		while(thisNode) 
        {
			// Get the linear index for the current fragment.
			FragIndex = thisNode->i;
            thisNode = thisNode->next;

//			if( pbi->FragCoeffs[FragIndex] <= EncodedCoeffs) 
			if( pbi->FragCoefEOB[FragIndex] <= EncodedCoeffs) 
            {

			    // If we are in the middle of an EOB run
			    if(pbi->EOB_Run) 
			    {
			        pbi->FragCoefEOB[FragIndex] = EncodedCoeffs + 1;
//			        pbi->FragCoeffs[FragIndex] = EncodedCoeffs + 1;
				    --pbi->EOB_Run;
				    --pbi->BlocksToDecode;
                    
                    lastNode->next = thisNode;
			    }
                else
                {
                    pbi->bumpLast = 1;

                    // Work out which huffman table to use, then decode a token
                    if ( FragIndex >= (INT32)pbi->YPlaneFragments )
				        AcHuffChoice = AcHuffChoice2;
			        
			        ReadToken( pbi, &pbi->br, pbi->QFragData[FragIndex], pbi->HuffRoot_VP3x[AcHuffChoice], FragIndex);
                    
//                    if(pbi->FragCoeffs[FragIndex] >= 64)
                    if(pbi->FragCoefEOB[FragIndex] >= 64)
                    {
                        pbi->FragCoefEOB[FragIndex] = EncodedCoeffs + 1;
//    			        pbi->FragCoeffs[FragIndex] = EncodedCoeffs + 1;

                        pbi->bumpLast = 0;

                        if(lastNode)
                            lastNode->next = thisNode;
                    }

                    if( pbi->bumpLast)
                        lastNode = lastNode->next;

                }
            }
            else
                lastNode = lastNode->next;			


		} //while(thisNode) 

	} while( ++ EncodedCoeffs < 64  &&  pbi->BlocksToDecode);
}

