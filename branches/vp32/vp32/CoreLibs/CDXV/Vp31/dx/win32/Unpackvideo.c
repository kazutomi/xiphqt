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



// write the coeffs in a transposed order.  
#define TI(x)  pbi->transIndex[x]


// for debugging purposes only
#define WC(x) x


#pragma warning( disable : 4799 )  // Disable no emms instruction warning!

static const UINT32 loMaskTbl[] = { 0,
	1, 3, 7, 15,
	31, 63, 127, 255,
	0x1ff, 0x3ff, 0x7ff, 0xfff,
	0x1fff, 0x3fff, 0x7fff, 0xffff,
	0x1FFFF, 0x3FFFF, 0x7FFFF, 0xfFFFF,
	0x1fFFFF, 0x3fFFFF, 0x7fFFFF, 0xffFFFF,
	0x1ffFFFF, 0x3ffFFFF, 0x7ffFFFF, 0xfffFFFF,
	0x1fffFFFF, 0x3fffFFFF, 0x7fffFFFF, 0xffffFFFF
};

static const UINT32 hiMaskTbl[] = { 0,
	0x80000000, 0xC0000000, 0xE0000000, 0xF0000000,
	0xF8000000, 0xFC000000, 0xFE000000, 0xFF000000,
	0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000,
	0xFFF80000, 0xFFFC0000, 0xFFFE0000, 0xFFFF0000,
	0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000,
	0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00,
	0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0, 0xFFFFFFF0,
	0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE, 0xFFFFFFFF
};

void ReadToken(
	PB_INSTANCE *pbi,
	BITREADER *br, 
	Q_LIST_ENTRY *Q, 
	HUFF_ENTRY *h, 
	UINT32 FragIndex) 
{
	
	INT32 t, a, c;		// token and optional argument
	
	// Need to swap bytes in a word if input format is big-endian
	// (as it should be).  "swab" is a PDP-11 mnemonic;
	// it's been a while since I programmed one of those.
	// BTW, a bswap instruction runs in one cycle, but is not pairable.
#define BitsAreBigEndian 1	
#	if BitsAreBigEndian
#		define	swab( R)	bswap R
#	else
#		define	swab( R)
#	endif
	
	// huffman-decode next token, storing it in "t"
	
	__asm {
			push	edi
			 mov	edi, [br]
			push	esi
			 lea	esi, [h]
			mov		ecx, [edi]BITREADER.bitsinremainder
			 xor	eax, eax
			mov		edx, [edi]BITREADER.remainder
			 jmp	nextB
			
		exactW:
			xor		ecx, ecx
			 mov	ebx, eax
			jmp		doneW
			
		nextW:
			; need another input word, unless done already
			or		eax, ebx
			 mov	ebx, [edi]BITREADER.position
			mov		ecx, 31
			 jns	exactW
			mov		edx, [ebx]
			 xor	eax, eax
			swab( edx)
			 shl	edx, 1
			lea		ebx, [ebx+4]
			 adc	eax, eax
			mov		[edi]BITREADER.position, ebx
						
nextB:
			mov		esi, [esi + eax*4]		; get fork selected by eax
			 xor	eax, eax
			shl		edx, 1					; carry = next input bit
			 dec	ecx
			mov		ebx, [esi]HUFF_ENTRY.Value
			 js		nextW					; oops! edx was empty, carry bogus
			adc		eax, eax				; next fork
			 or		ebx, ebx
			js		nextB
			
			or		edx, eax				; put back unused input bit
			 inc	ecx
			ror		edx, 1
			;
doneW:
			mov		[edi]BITREADER.bitsinremainder, ecx
			mov		[edi]BITREADER.remainder, edx
			mov		[t], ebx
			pop		esi
			pop		edi
	}
	
	// Read N bits, storing them in "a".
	// Sorry about the meaningless user-supplied labels, but msvc's inline
	// assembler is kinda pathetic (as is the C preprocessor).
	
#	define get( N, L1, L2) { \
	__asm { \
	__asm mov	eax, [br] \
	__asm  mov	ecx, N \
	__asm mov	ebx, [eax]BITREADER.bitsinremainder /* ebx = what we have */ \
	__asm  mov	edx, [eax]BITREADER.remainder		/* edx = bits at top */ \
	__asm cmp	ebx, ecx \
	__asm  jc	L1 \
	\
	__asm rol	edx, N						/* have enough */ \
	__asm sub	ebx, ecx					/* what's left */ \
	__asm  mov	ecx, [loMaskTbl + ecx*4] \
	__asm mov	[eax]BITREADER.remainder, edx		/* save top bits */ \
	__asm  and	edx, ecx \
	__asm mov	[eax]BITREADER.bitsinremainder, ebx \
	__asm  jmp	L2 \
	\
	__asm L1:								/* need another word */ \
	__asm push	esi \
	__asm  mov	esi, [hiMaskTbl + ebx*4]	/* mask old bits */ \
	__asm and	edx, esi \
	__asm  mov	esi, [eax]BITREADER.position \
	__asm rol	edx, N						/* get in position */ \
	__asm sub	ecx, ebx					/* still need ecx */ \
	__asm  mov	ebx, [esi] \
	__asm swab( ebx) \
	__asm rol	ebx, cl						/* in position */ \
	__asm mov	[eax]BITREADER.remainder, ebx		/* top bits for later */ \
	__asm  lea	esi, [esi + 4] \
	__asm mov	[eax]BITREADER.position, esi \
	__asm  mov	esi, [loMaskTbl + ecx*4]	/* mask off new bits */ \
	__asm and	esi, ebx \
	__asm  mov	ebx, 32 \
	__asm or	edx, esi					/* old | new */ \
	__asm  sub	ebx, ecx					/* what's left */ \
	__asm mov	[eax]BITREADER.bitsinremainder, ebx \
	__asm  pop	esi \
	__asm  L2: \
	__asm  mov	[a], edx \
		} \
		}
	///printf("t = %d, a = %d\n",t,a); \
	
	
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
 *  ROUTINE       :     UnPackVideoAsm
 *
 *  INPUTS        :     None. 
 *                      
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Upacks and decodes the video stream.
 *
 *  SPECIAL NOTES :     This function call Assembly codes to read token
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UnPackVideoAsm (PB_INSTANCE *pbi)
{

    INT32       EncodedCoeffs = 1;
    INT32       FragIndex;
    INT32 *     CodedBlockListPtr;
    INT32 *     CodedBlockListEnd;

    UINT8       AcHuffIndex1;
    UINT8       AcHuffIndex2;
    UINT8       AcHuffChoice1;
    UINT8       AcHuffChoice2;
    
    UINT8       DcHuffChoice1;
    UINT8       DcHuffChoice2;
	Q_LIST_ENTRY *	Q;

	
	
    // Bail out immediately if a decode error has already been reported.
    if ( pbi->DecoderErrorCode != NO_DECODER_ERROR )
        return;

    // Clear down the array that indicates the current coefficient index for each block.
    memset(pbi->FragCoeffs, 0, pbi->UnitFragments);
    memset(pbi->FragCoefEOB, 0, pbi->UnitFragments);

    // Clear down the pbi->QFragData structure for all coded blocks.
    // pbi->ClearDownQFrag(pbi);

    // Note the number of blocks to decode
    pbi->BlocksToDecode = pbi->CodedBlockIndex;

    // Get the DC huffman table choice for Y and then UV
    DcHuffChoice1 = (UINT8)bitread( &pbi->br,  DC_HUFF_CHOICE_BITS ) + DC_HUFF_OFFSET;
    DcHuffChoice2 = (UINT8)bitread( &pbi->br,  DC_HUFF_CHOICE_BITS ) + DC_HUFF_OFFSET;

    // UnPack DC coefficients / tokens
    CodedBlockListPtr = pbi->CodedBlockList;
    CodedBlockListEnd = &pbi->CodedBlockList[pbi->CodedBlockIndex];

	// ugly tim hack convert bit buffer left justified    
	if( pbi->br.bitsinremainder)
	  	pbi->br.remainder <<= 32 - pbi->br.bitsinremainder;
	else
			pbi->br.remainder = 0;


	pbi->EOB_Run = 0;

	do {
		FragIndex= *CodedBlockListPtr;

        // Select the appropriate huffman table offset according to whether the token is fro am Y or UV block
        if ( FragIndex < (INT32)pbi->YPlaneFragments )
            pbi->DcHuffChoice = DcHuffChoice1;
        else
            pbi->DcHuffChoice = DcHuffChoice2;

		Q = pbi->QFragData[FragIndex];

		memset( (void *) Q, 0, 128);

		if(pbi->EOB_Run) 
		{
			pbi->FragCoeffs[FragIndex] = 64;
			--pbi->EOB_Run;
			--pbi->BlocksToDecode;
		} 
		else
			ReadToken( pbi, &pbi->br, Q, pbi->HuffRoot_VP3x[pbi->DcHuffChoice], FragIndex);
	} 
	while( ++CodedBlockListPtr < CodedBlockListEnd);

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
		CodedBlockListPtr = pbi->CodedBlockList;
		CodedBlockListEnd = &pbi->CodedBlockList[pbi->CodedBlockIndex];
		
        // Huffman table selection based upon which AC coefficient we are on
        if ( EncodedCoeffs <= AC_TABLE_2_THRESH )
        {
            AcHuffChoice1 = AcHuffIndex1;
            AcHuffChoice2 = AcHuffIndex2;
        }
        else if ( EncodedCoeffs <= AC_TABLE_3_THRESH )
        {
            AcHuffChoice1 = AcHuffIndex1 + AC_HUFF_CHOICES;
            AcHuffChoice2 = AcHuffIndex2 + AC_HUFF_CHOICES;
        }
        else if ( EncodedCoeffs <= AC_TABLE_4_THRESH )
        {
            AcHuffChoice1 = AcHuffIndex1 + (AC_HUFF_CHOICES * 2);
            AcHuffChoice2 = AcHuffIndex2 + (AC_HUFF_CHOICES * 2);
        }
        else
        {
            AcHuffChoice1 = AcHuffIndex1 + (AC_HUFF_CHOICES * 3);
            AcHuffChoice2 = AcHuffIndex2 + (AC_HUFF_CHOICES * 3);
        }

		do {
			// Get the linear index for the current fragment.
			FragIndex = *CodedBlockListPtr;


			if( pbi->FragCoeffs[FragIndex] > EncodedCoeffs) continue;

			pbi->FragCoefEOB[FragIndex]=pbi->FragCoeffs[FragIndex];
			
			// If we are in the middle of an EOB run
			if(pbi->EOB_Run) 
			{
				pbi->FragCoeffs[FragIndex] = 64;
				--pbi->EOB_Run;
				--pbi->BlocksToDecode;
				continue;
			}

            // Work out which huffman table to use, then decode a token
            if ( FragIndex < (INT32)pbi->YPlaneFragments )
				pbi->ACHuffChoice = AcHuffChoice1;
			else
				pbi->ACHuffChoice = AcHuffChoice2;
			
			ReadToken( pbi, &pbi->br, pbi->QFragData[FragIndex], pbi->HuffRoot_VP3x[pbi->ACHuffChoice], FragIndex);

		} 
		while( ++CodedBlockListPtr < CodedBlockListEnd);

	} while( ++ EncodedCoeffs < 64  &&  pbi->BlocksToDecode);
}

/****************************************************************************
 * 
 *  ROUTINE       :     UnPackVideoMMX_LL
 *
 *  INPUTS        :     None. 
 *                      
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Upacks and decodes the video stream.
 *
 *  SPECIAL NOTES :     This function calls Assembly codes to read token 
 *                      and has some MMX code. 
 *                  
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void UnPackVideoMMX_LL (PB_INSTANCE *pbi)
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
		__asm 
		{
				mov		eax, [Q]
				pxor	mm0, mm0
				
				movq	[eax], mm0
				movq	[eax+8], mm0
				movq	[eax+16], mm0
				movq	[eax+24], mm0
				movq	[eax+32], mm0
				movq	[eax+40], mm0
				movq	[eax+48], mm0
				movq	[eax+56], mm0
				movq	[eax+64], mm0
				movq	[eax+72], mm0
				movq	[eax+80], mm0
				movq	[eax+88], mm0
				movq	[eax+96], mm0
				movq	[eax+104], mm0
				movq	[eax+112], mm0
				movq	[eax+120], mm0
		}
		
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

