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
*   Module Title :     CEncode.C
*
*   Description  :     Video CODEC: Encode .
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include "codec_common_interface.h"
#include "SystemDependant.h"
#include "BlockMapping.h"
#include "CBitman.h"
#include "CFrameW.h"
#include "CFrarray.h"
#include "Huffman.h"
#include "Mcomp.h"
#include "Misc_common.h"
#include "Quantize.h"
#include "compdll.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/****************************************************************************
*  Explicit imports
*****************************************************************************
*/ 
extern UINT32 GetMBSadInterError( CP_INSTANCE *cpi, UINT8 * SrcPtr, UINT8 * RefPtr, UINT32 FragIndex, INT32 LastXMV, INT32 LastYMV, UINT32 PixelsPerLine );
extern void UpdateFragQIndex(PB_INSTANCE *pbi);

extern void UpdateUMV_HBorders( PB_INSTANCE *pbi, UINT8 * DestReconPtr, UINT32  PlaneFragOffset );
extern void UpdateUMV_VBorders( PB_INSTANCE *pbi, UINT8 * DestReconPtr, UINT32  PlaneFragOffset );

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/


// MV coding Method 1
UINT32 MvPattern[(MAX_MV_EXTENT * 2) + 1] = 
    {   0x000000ff, 0x000000fd, 0x000000fb, 0x000000f9, 0x000000f7, 0x000000f5, 0x000000f3, 0x000000f1,
        0x000000ef, 0x000000ed, 0x000000eb, 0x000000e9, 0x000000e7, 0x000000e5, 0x000000e3, 0x000000e1,
        0x0000006f, 0x0000006d, 0x0000006b, 0x00000069, 0x00000067, 0x00000065, 0x00000063, 0x00000061,
        0x0000002f, 0x0000002d, 0x0000002b, 0x00000029, 0x00000009, 0x00000007, 0x00000002, 0x00000000,
        0x00000001, 0x00000006, 0x00000008, 0x00000028, 0x0000002a, 0x0000002c, 0x0000002e, 0x00000060, 
        0x00000062, 0x00000064, 0x00000066, 0x00000068, 0x0000006a, 0x0000006c, 0x0000006e, 0x000000e0,
        0x000000e2, 0x000000e4, 0x000000e6, 0x000000e8, 0x000000ea, 0x000000ec, 0x000000ee, 0x000000f0,
        0x000000f2, 0x000000f4, 0x000000f6, 0x000000f8, 0x000000fa, 0x000000fc, 0x000000fe,
    };
UINT32 MvBits[(MAX_MV_EXTENT * 2) + 1] = 
    {   8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        7, 7, 7, 7, 7, 7, 7, 7,
        6, 6, 6, 6, 4, 4, 3, 3,
        3, 4, 4, 6, 6, 6, 6, 7,
        7, 7, 7, 7, 7, 7, 7, 8,
        8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8,
    };

// Fallback MV coding method (Simple six bits per vector)
UINT32 MvPattern2[(MAX_MV_EXTENT * 2) + 1] = 
    {   0x0000003f, 0x0000003d, 0x0000003b, 0x00000039, 0x00000037, 0x00000035, 0x00000033, 0x00000031,
        0x0000002f, 0x0000002d, 0x0000002b, 0x00000029, 0x00000027, 0x00000025, 0x00000023, 0x00000021,
        0x0000001f, 0x0000001d, 0x0000001b, 0x00000019, 0x00000017, 0x00000015, 0x00000013, 0x00000011,
        0x0000000f, 0x0000000d, 0x0000000b, 0x00000009, 0x00000007, 0x00000005, 0x00000003, 0x00000000,
        0x00000002, 0x00000004, 0x00000006, 0x00000008, 0x0000000a, 0x0000000c, 0x0000000e, 0x00000010,
        0x00000012, 0x00000014, 0x00000016, 0x00000018, 0x0000001a, 0x0000001c, 0x0000001e, 0x00000020,
        0x00000022, 0x00000024, 0x00000026, 0x00000028, 0x0000002a, 0x0000002c, 0x0000002e, 0x00000030,
        0x00000032, 0x00000034, 0x00000036, 0x00000038, 0x0000003a, 0x0000003c, 0x0000003e, 
    };
UINT32 MvBits2[(MAX_MV_EXTENT * 2) + 1] = 
    {   6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6,
    };



//INT32   YSignRunHitCount[8192];


/****************************************************************************
*  Forward References
*****************************************************************************
*/              

//INT32 GetBlockReconError( INT32 BlockIndex );
/*
INT32 GetBlockReconErrorSlow ( INT32 BlockIndex );
INT32 GetBlockReconErrorXMM  ( INT32 BlockIndex );
INT32 (* GetBlockReconError ) ( INT32 ) = GetBlockReconErrorSlow;
*/
/****************************************************************************
*  Exported Functions
*****************************************************************************
*/              

/****************************************************************************
*  Module Statics
*****************************************************************************
*/    
         
void   PackCodedVideo (CP_INSTANCE *cpi);
void   EncodeDcTokenList (CP_INSTANCE *cpi);
void   EncodeAcTokenList (CP_INSTANCE *cpi);
void   PackToken ( CP_INSTANCE *cpi, INT32 FragmentNumber, UINT32 HuffIndex );
void   PackModes ( CP_INSTANCE *cpi);
void   PackMotionVectors (CP_INSTANCE *cpi);
void   PackEOBRun(CP_INSTANCE *cpi);

// Data structures defining the various coding schems for the modes.
UINT32 ModeBitPatterns[MAX_MODES] = { 0x00, 0x02, 0x06, 0x0E, 0x1E, 0x3E, 0x7E, 0x7F };
INT32  ModeBitLengths[MAX_MODES] =  { 1,    2,    3,    4,    5,    6,    7,    7 };

UINT8  ModeSchemes[MODE_METHODS-1][MAX_MODES] =  
    {   
        { 0,    0,    0,    0,    0,    0,    0,    0 },    // Reserved for optimal    

        // Last Mv dominates    
        { 3,    4,    2,    0,    1,    5,    6,    7 },    // L P  M N I G GM 4     
        { 2,    4,    3,    0,    1,    5,    6,    7 },    // L P  N M I G GM 4
        { 3,    4,    1,    0,    2,    5,    6,    7 },    // L M  P N I G GM 4
        { 2,    4,    1,    0,    3,    5,    6,    7 },    // L M  N P I G GM 4
 
        // No MV dominates
        { 0,    4,    3,    1,    2,    5,    6,    7 },    // N L P M I G GM 4 
        { 0,    5,    4,    2,    3,    1,    6,    7 },    // N G L P M I GM 4

    };


UINT32 MvThreshTable[Q_TABLE_SIZE] = 
    {   65, 65, 65, 65, 50, 50, 50, 50,
        40, 40, 40, 40, 40, 40, 40, 40,
        30, 30, 30, 30, 30, 30, 30, 30,
        20, 20, 20, 20, 20, 20, 20, 20,
        15, 15, 15, 15, 15, 15, 15, 15,
        10, 10, 10, 10, 10, 10, 10, 10,
         5,  5,  5,  5,  5,  5,  5,  5, 
         0,  0,  0,  0,  0,  0,  0,  0 
    };

UINT32 MVChangeFactorTable[Q_TABLE_SIZE] = 
    {   11, 11, 11, 11, 12, 12, 12, 12,
        13, 13, 13, 13, 13, 13, 13, 13,
        14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,
        14, 14, 14, 14, 14, 14, 14, 14,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15
     };  

/****************************************************************************
 * 
 *  ROUTINE       :     EncodeData
 *
 *  INPUTS        :     
 *                   
 *
 *  OUTPUTS       :     None.
 *
 *  RETURNS       :     Bytes encoded.
 *
 *  FUNCTION      :     Creates an encoded data stream stream for the core data.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 EncodeData(CP_INSTANCE *cpi)
{                                                       
    UINT32 coded_pixels = 0;

    // Zero the count of tokens so far this frame.
    cpi->TotTokenCount = 0;

    // Zero the mode and MV list indices.
    cpi->ModeListCount = 0;

    // Zero Decoder EOB run count 
    cpi->pb.EOB_Run = 0;

    /* Encode any fragments coded using DCT. */
    coded_pixels += QuadCodeDisplayFragments (cpi);           

    return coded_pixels;

}
/*
void SetMotionThresholds( CP_INSTANCE *cpi, 
   int      FixedQ,
   double   MVChangeFactor,  
   int      ExhaustiveSearchThresh,
   int      FourMVThreshold,
   int      MinImprovementForNewMV,
   int      MinImprovementForFourMV,
   double   FourMvChangeFactor,
   int      InterTripOutThresh 
   )
{
    cpi->FixedQ                     = FixedQ                     ;
    cpi->MVChangeFactor             = MVChangeFactor             ;
    cpi->ExhaustiveSearchThresh     = ExhaustiveSearchThresh     ;
    cpi->FourMVThreshold            = FourMVThreshold            ;
    cpi->MinImprovementForNewMV     = MinImprovementForNewMV     ;
    cpi->MinImprovementForFourMV    = MinImprovementForFourMV    ;
    cpi->FourMvChangeFactor         = FourMvChangeFactor         ;
    cpi->InterTripOutThresh         = InterTripOutThresh         ;
}
*/
/****************************************************************************
 * 
 *  ROUTINE       :     QuadCodeDisplayFragments
 *
 *  INPUTS        :     None.
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Codes the frame using the quad tree method.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 QuadCodeDisplayFragments_backup (CP_INSTANCE *cpi)
{
    INT32   i,j;

	UINT32	coded_pixels=0;
    UINT8   QIndex;

#define PUL 8
#define PU 4
#define PUR 2
#define PL 1

    int k,m,n;

    // predictor multiplier up-left, up, up-right,left, shift
	short pc[16][6]=
	{
		{0,0,0,0,0,0},	
		{0,0,0,1,0,0},		// PL
		{0,0,1,0,0,0},		// PUR
		{0,0,53,75,7,127},	// PUR|PL
		{0,1,0,0,0,0},		// PU
		{0,1,0,1,1,1},		// PU|PL
		{0,1,0,0,0,0},		// PU|PUR
		{0,0,53,75,7,127},	// PU|PUR|PL
		{1,0,0,0,0,0},		// PUL|
		{0,0,0,1,0,0},		// PUL|PL
		{1,0,1,0,1,1},		// PUL|PUR
		{0,0,53,75,7,127},	// PUL|PUR|PL
		{0,1,0,0,0,0},		// PUL|PU
		{-26,29,0,29,5,31}, // PUL|PU|PL
		{3,10,3,0,4,15},		// PUL|PU|PUR
		{-26,29,0,29,5,31}	// PUL|PU|PUR|PL
	};

	/* Search Points are ordered by distance from 0,0
		-4 -3 -2 -1  0  1  2  3  4 
	-4	         21 19 22         
	-3     24 15 11  9 12 16 25   
	-2     14  6  3  1  4  7 17   
	-1  20 10  2  z  z  z  5 13 23
	 0	18  8  0  z  z	z  z  z  z
	*/
	struct SearchPoints
	{
		int RowOffset;
		int ColOffset;
	} DCSearchPoints[]=
	{
		{0,-2},{-2,0},{-1,-2},{-2,-1},{-2,1},{-1,2},{-2,-2},{-2,2},{0,-3},
		{-3,0},{-1,-3},{-3,-1},{-3,1},{-1,3},{-2,-3},{-3,-2},{-3,2},{-2,3},
		{0,-4},{-4,0},{-1,-4},{-4,-1},{-4,1},{-1,4},{-3,-3},{-3,3}
	};
	//int DCSearchPointCount = sizeof(DCSearchPoints) / ( 2 * sizeof(int));
	int DCSearchPointCount = 0;

	// fragment left fragment up-left, fragment up, fragment up-right
	int fl,ful,fu,fur;

	// value left value up-left, value up, value up-right
	int vl,vul,vu,vur;

	// fragment number left, up-left, up, up-right
	int l,ul,u,ur;

	//which predictor constants to use
	short wpc;

	// last used inter predictor (Raster Order)
	short Last[3];	// last value used for given frame
	short TempInter = 0;

	int FragsAcross=cpi->pb.HFragments;	
	int FragsDown = cpi->pb.VFragments;
	int FromFragment,ToFragment;
	INT32	FragIndex;			    // Fragment number
	int WhichFrame;
	int WhichCase;

	short Mode2Frame[] =
	{
		1,	// CODE_INTER_NO_MV		0 => Encoded diff from same MB last frame 
		0,	// CODE_INTRA			1 => DCT Encoded Block
		1,	// CODE_INTER_PLUS_MV	2 => Encoded diff from included MV MB last frame
		1,	// CODE_INTER_LAST_MV	3 => Encoded diff from MRU MV MB last frame
		1,	// CODE_INTER_PRIOR_MV	4 => Encoded diff from included 4 separate MV blocks
		2,	// CODE_USING_GOLDEN	5 => Encoded diff from same MB golden frame
		2,	// CODE_GOLDEN_MV		6 => Encoded diff from included MV MB golden frame
		1	// CODE_INTER_FOUR_MV	7 => Encoded diff from included 4 separate MV blocks
	};

	short PredictedDC;

#define HIGHBITDUPPED(X) (((signed short) X)  >> 15)


    // Initialise the coded block indices variables. These allow
    // subsequent linear access to the quad tree ordered list of 
    // coded blocks
    cpi->pb.CodedBlockIndex = 0;

    // Set the inter/intra descision control variables.
    QIndex = Q_TABLE_SIZE - 1;
    while ( (INT32) QIndex >= 0 )
    {
        if ( (QIndex == 0) || ( cpi->pb.QThreshTable[QIndex] >= cpi->pb.ThisFrameQualityValue) )
            break;
        QIndex --;
    }


    // Encode and tokenise the Y, U and V components
	coded_pixels = QuadCodeComponent ( cpi, 0, cpi->pb.YSBRows, cpi->pb.YSBCols, cpi->pb.HFragments%4, cpi->pb.VFragments%4, cpi->pb.Configuration.VideoFrameWidth );
	coded_pixels += QuadCodeComponent ( cpi, cpi->pb.YSuperBlocks, cpi->pb.UVSBRows, cpi->pb.UVSBCols, (cpi->pb.HFragments/2)%4, (cpi->pb.VFragments/2)%4, cpi->pb.Configuration.VideoFrameWidth>>1 );
	coded_pixels += QuadCodeComponent ( cpi, cpi->pb.YSuperBlocks+cpi->pb.UVSuperBlocks, cpi->pb.UVSBRows, cpi->pb.UVSBCols, (cpi->pb.HFragments/2)%4, (cpi->pb.VFragments/2)%4, cpi->pb.Configuration.VideoFrameWidth>>1 );
    
	// for y,u,v
	for ( j = 0; j < 3 ; j++)
	{
		// pick which fragments based on Y, U, V
		switch(j)
		{
		case 0: // y
			FromFragment = 0;
			ToFragment = cpi->pb.YPlaneFragments;
			FragsAcross = cpi->pb.HFragments;
			FragsDown = cpi->pb.VFragments;
			break;
		case 1: // u
			FromFragment = cpi->pb.YPlaneFragments;
			ToFragment = cpi->pb.YPlaneFragments + cpi->pb.UVPlaneFragments ;
			FragsAcross = cpi->pb.HFragments >> 1;
			FragsDown = cpi->pb.VFragments >> 1;
			break;
		case 2:	// v
			FromFragment = cpi->pb.YPlaneFragments + cpi->pb.UVPlaneFragments;
			ToFragment = cpi->pb.YPlaneFragments + (2 * cpi->pb.UVPlaneFragments) ;
			FragsAcross = cpi->pb.HFragments >> 1;
			FragsDown = cpi->pb.VFragments >> 1;
			break;
		}

		// initialize our array of last used DC Components
		for(k=0;k<3;k++)
			Last[k]=0;

		i=FromFragment;

		// do prediction on all of Y, U or V
		for ( m = 0 ; m < FragsDown ; m++)
		{
			for ( n = 0 ; n < FragsAcross ; n++, i++)
			{
				cpi->OriginalDC[i] = cpi->pb.QFragData[i][0];
                //fprintf(f,"%d ",cpi->OriginalDC[i]);

				// only do 2 prediction if fragment coded and on non intra or if all fragments are intra 
				if( cpi->pb.display_fragments[i] || (GetFrameType(&cpi->pb) == BASE_FRAME) )
				{
					// Type of Fragment
					WhichFrame = Mode2Frame[cpi->pb.FragCodingMethod[i]];

					// Check Borderline Cases
					WhichCase = (n==0) + ((m==0) << 1) + ((n+1 == FragsAcross) << 2);

					switch(WhichCase)
					{
					case 0: // normal case no border condition

						// calculate values left, up, up-right and up-left
						l = i-1;
						u = i - FragsAcross;
						ur = i - FragsAcross + 1;
						ul = i - FragsAcross - 1;

						// calculate values
						vl = cpi->OriginalDC[l];
						vu = cpi->OriginalDC[u];
						vur = cpi->OriginalDC[ur];
						vul = cpi->OriginalDC[ul];
						
						// fragment valid for prediction use if coded and it comes from same frame as the one we are predicting
						fl = cpi->pb.display_fragments[l] && (Mode2Frame[cpi->pb.FragCodingMethod[l]] == WhichFrame);
						fu = cpi->pb.display_fragments[u] && (Mode2Frame[cpi->pb.FragCodingMethod[u]] == WhichFrame);
						fur = cpi->pb.display_fragments[ur] && (Mode2Frame[cpi->pb.FragCodingMethod[ur]] == WhichFrame);
						ful = cpi->pb.display_fragments[ul] && (Mode2Frame[cpi->pb.FragCodingMethod[ul]] == WhichFrame);

						// calculate which predictor to use 
						wpc = (fl*PL) | (fu*PU) | (ful*PUL) | (fur*PUR);

						break;

					case 1: // n == 0 Left Column

						// calculate values left, up, up-right and up-left
						u = i - FragsAcross;
						ur = i - FragsAcross + 1;

						// calculate values
						vu = cpi->OriginalDC[u];
						vur = cpi->OriginalDC[ur];

						// fragment valid for prediction if coded and it comes from same frame as the one we are predicting
						fu = cpi->pb.display_fragments[u] && (Mode2Frame[cpi->pb.FragCodingMethod[u]] == WhichFrame);
						fur = cpi->pb.display_fragments[ur] && (Mode2Frame[cpi->pb.FragCodingMethod[ur]] == WhichFrame);

						// calculate which predictor to use 
						wpc = (fu*PU) | (fur*PUR);

						break;

					case 2: // m == 0 Top Row 
					case 6: // m == 0 and n+1 == FragsAcross or Top Row Right Column

						// calculate values left, up, up-right and up-left
						l = i-1;

						// calculate values
						vl = cpi->OriginalDC[l];

						// fragment valid for prediction if coded and it comes from same frame as the one we are predicting
						fl = cpi->pb.display_fragments[l] && (Mode2Frame[cpi->pb.FragCodingMethod[l]] == WhichFrame);

						// calculate which predictor to use 
						wpc = (fl*PL) ;

						break;

					case 3: // n == 0 & m == 0 Top Row Left Column

						wpc = 0;

						break;

					case 4: // n+1 == FragsAcross : Right Column

						// calculate values left, up, up-right and up-left
						l = i-1;
						u = i - FragsAcross;
						ul = i - FragsAcross - 1;

						// calculate values
						vl = cpi->OriginalDC[l];
						vu = cpi->OriginalDC[u];
						vul = cpi->OriginalDC[ul];
						
						// fragment valid for prediction if coded and it comes from same frame as the one we are predicting
						fl = cpi->pb.display_fragments[l] && (Mode2Frame[cpi->pb.FragCodingMethod[l]] == WhichFrame);
						fu = cpi->pb.display_fragments[u] && (Mode2Frame[cpi->pb.FragCodingMethod[u]] == WhichFrame);
						ful = cpi->pb.display_fragments[ul] && (Mode2Frame[cpi->pb.FragCodingMethod[ul]] == WhichFrame);

						// calculate which predictor to use 
						wpc = (fl*PL) | (fu*PU) | (ful*PUL) ;

						break;

					}
					
					
					if(wpc==0)
					{
						FragIndex = 1;
						
						// find the nearest one that is coded 
						for( k = 0; k < DCSearchPointCount ; k++)
						{
							FragIndex = i + DCSearchPoints[k].RowOffset * FragsAcross + DCSearchPoints[k].ColOffset;
							
							if( FragIndex - FromFragment > 0 ) 
							{
								if(cpi->pb.display_fragments[FragIndex] && (Mode2Frame[cpi->pb.FragCodingMethod[FragIndex]] == WhichFrame))
								{
									cpi->pb.QFragData[i][0] -= cpi->OriginalDC[FragIndex];
									FragIndex = 0;
									break;
								}
							}
						}
						
						
						// if none matched fall back to the last one ever
						if(FragIndex)
						{
							cpi->pb.QFragData[i][0] -= Last[WhichFrame];
						}
						
					}
					else
					{
						
						// don't do divide if divisor is 1 or 0
						PredictedDC = (pc[wpc][0]*vul + pc[wpc][1] * vu + pc[wpc][2] * vur + pc[wpc][3] * vl );

						// if we need to do a shift
						if(pc[wpc][4] != 0 )
						{
							
							// If negative add in the negative correction factor
							PredictedDC += (HIGHBITDUPPED(PredictedDC) & pc[wpc][5]);
							
							// Shift in lieu of a divide
							PredictedDC >>= pc[wpc][4];
						}
						
                        // check for outranging on the two predictors that can outrange 
                        switch(wpc)
                        {
                        case 13: // pul pu pl
                        case 15: // pul pu pur pl
                            if( abs(PredictedDC - vu) > 128)
                                PredictedDC = vu;
                            else if( abs(PredictedDC - vl) > 128)
                                PredictedDC = vl;
                            else if( abs(PredictedDC - vul) > 128)
                                PredictedDC = vul;
                            break;
                        }

						
						cpi->pb.QFragData[i][0] -= PredictedDC;
					}
					
					// Save the last fragment coded for whatever frame we are predicting from
					Last[WhichFrame] = cpi->OriginalDC[i];

				} // if display fragments

			} // for n = 0 to columns across

		} // for m = 0 to rows down

	} // for j = 0 to 2 (y,u,v)

    // Pack DC tokens and adjust the ones we couldn't predict 2d
	for ( i = 0; i < cpi->pb.CodedBlockIndex; i++ )
	{
//		int NextFragIndex;
		// Get the linear index for the current coded fragment.
		FragIndex = cpi->pb.CodedBlockList[i];
//		NextFragIndex = cpi->pb.CodedBlockList[i+1];

//		XmmFetch(cpi->pb.QFragData[FragIndex], cpi->pb.QFragData[NextFragIndex]);

		coded_pixels += DPCMTokenizeBlock ( cpi, FragIndex, cpi->pb.Configuration.VideoFrameWidth  );

	}

    // Bit pack the video data data
    PackCodedVideo(cpi);

    // End the bit packing run. 
    EndAddBitsToBuffer(cpi);

    // Reconstruct the reference frames
    ReconRefFrames(&cpi->pb);
   
    // Measure the inter reconstruction error for all the blocks that were coded
    // for use as part of the recovery monitoring process in subsequent frames.
    for ( i = 0; i < cpi->pb.CodedBlockIndex; i++ )
    {
        cpi->LastCodedErrorScore[ cpi->pb.CodedBlockList[i] ] = cpi->GetBlockReconError( cpi, cpi->pb.CodedBlockList[i] );
    
	}
	cpi->pb.ClearSysState();

    // Return total number of coded pixels
	return coded_pixels;
}
/****************************************************************************
 * 
 *  ROUTINE       :     CalculateMotionErrorforFragments
 *
 *  INPUTS        :     UINT32	HFrags
 *						UINT32	VFrags
 *						UINT32	FirstMBIndex
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     sums the error for all fragments in list past in 
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
INT32 CalculateMotionErrorforFragments( 
    CP_INSTANCE *cpi,     
    INT32 CountUsingMV,
    INT32 *FragsUsing,
    MOTION_VECTOR MVect,
    INT32 PixelsPerLine
    )
{
    int i;
    INT32 NewError = 0;

    // for now 4 motion vector is to hard to recalculate so return huge error!!
    if( cpi->pb.FragCodingMethod[0] == CODE_INTER_FOURMV)
        return HUGE_ERROR;

    for(i = 0 ; i < CountUsingMV ; i++)
    {
        INT32 FragIndex = FragsUsing[i];

        INT32 ThisError =
            GetMBInterError( cpi, 
                cpi->ConvDestBuffer, 
                cpi->pb.LastFrameRecon, 
                FragIndex, MVect.x, MVect.y, PixelsPerLine );
        
        NewError += ThisError;

    }

    return NewError;

}



/****************************************************************************
 * 
 *  ROUTINE       :     PickIntra
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     A motion score 
 *
 *  FUNCTION      :     Picks intra coding for each macroblock 
 *                      of the image.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 PickIntra( CP_INSTANCE *cpi, UINT32 SBRows, UINT32 SBCols, UINT32 HExtra, UINT32 VExtra, UINT32 PixelsPerLine)
{
	INT32	FragIndex;			    // Fragment number
	UINT32	MB, B;				    // Macro-Block, Block indices
	UINT32	SBrow;				    // Super-Block row number
	UINT32	SBcol;				    // Super-Block row number
	UINT32	SB=0;			    // Super-Block index, initialised to first of this component

    UINT32 UVRow;
    UINT32 UVColumn;
    UINT32 UVFragOffset;

    // decide what block type and motion vectors to use on all of the frames
	for ( SBrow=0; SBrow<SBRows; SBrow++ )
	{
		for ( SBcol=0; SBcol<SBCols; SBcol++ )
		{
			// Check its four Macro-Blocks
			for ( MB=0; MB<4; MB++ )
			{
				// There may be MB's lying out of frame
				// which must be ignored. For these MB's
				// Top left block will have a negative Fragment Index.
				if ( QuadMapToMBTopLeft(cpi->pb.BlockMap,SB,MB) >= 0 )
				{
                    cpi->MBCodingMode = CODE_INTRA;
                    
                    // Now actually code the blocks.
                    for ( B=0; B<4; B++ )
                    {
                        FragIndex = QuadMapToIndex1( cpi->pb.BlockMap, SB, MB, B );
                        cpi->pb.FragCodingMethod[FragIndex] = cpi->MBCodingMode;
                    }
                    
                    // Matching fragments in the U and V planes
                    UVRow = (FragIndex / (cpi->pb.HFragments * 2));
                    UVColumn = (FragIndex % cpi->pb.HFragments) / 2;
                    UVFragOffset = (UVRow * (cpi->pb.HFragments / 2)) + UVColumn;
                    
                    cpi->pb.FragCodingMethod[cpi->pb.YPlaneFragments + UVFragOffset] = cpi->MBCodingMode;
                    cpi->pb.FragCodingMethod[cpi->pb.YPlaneFragments + cpi->pb.UVPlaneFragments + UVFragOffset] = cpi->MBCodingMode;
                            
                }
            }
            
            // Next Super-Block
			SB++;
		}
	}
    return 0;
}


/****************************************************************************
 * 
 *  ROUTINE       :     QuadCodeComponent
 *
 *  INPUTS        :     UINT32	HFrags
 *						UINT32	VFrags
 *						UINT32	FirstMBIndex
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Codes the display_fragments array as a quad-tree
 *						starting with 32x32 Super-Blocks, then 16x16 
 *						Macro-Blocks, and finally 8x8 Blocks.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 QuadCodeComponent ( CP_INSTANCE *cpi, UINT32 FirstSB, UINT32 SBRows, UINT32 SBCols, UINT32 HExtra, UINT32 VExtra, UINT32 PixelsPerLine )
{
	INT32	FragIndex;			    // Fragment number
	UINT32	MB, B;				    // Macro-Block, Block indices
	UINT32	SBrow;				    // Super-Block row number
	UINT32	SBcol;				    // Super-Block row number
	UINT32	SB=FirstSB;			    // Super-Block index, initialised to first of this component
	UINT32	coded_pixels=0;		    // Number of pixels coded
    BOOL    MBCodedFlag;

    // actually transform and quantize the image now that we've decided on the modes
    // Parse in quad-tree ordering
    SB=FirstSB;
    for ( SBrow=0; SBrow<SBRows; SBrow++ )
    {
        for ( SBcol=0; SBcol<SBCols; SBcol++ )
        {
            // Check its four Macro-Blocks 
            for ( MB=0; MB<4; MB++ )
            {
                
                if ( QuadMapToMBTopLeft(cpi->pb.BlockMap,SB,MB) >= 0 )
                {
                    
                    MBCodedFlag = FALSE;  

                    // Now actually code the blocks.
                    for ( B=0; B<4; B++ )
                    {
                        FragIndex = QuadMapToIndex1( cpi->pb.BlockMap, SB, MB, B );
                        
                        // Does Block lie in frame:
                        if ( FragIndex >= 0 )
                        {
                            // In Frame: Is it coded:
                            if ( cpi->pb.display_fragments[FragIndex] )
                            {

                                // transform and quantize block 
                                TransformQuantizeBlock( cpi, FragIndex, PixelsPerLine );

                                // Has the block got struck off (no MV and no data generated after DCT)
                                // If not then mark it and the assosciated MB as coded.
                                if ( cpi->pb.display_fragments[FragIndex] )
                                {
                                    // Create linear list of coded block indices
                                    cpi->pb.CodedBlockList[cpi->pb.CodedBlockIndex] = FragIndex;
                                    cpi->pb.CodedBlockIndex++;
                                    
                                    // MB is still coded
                                    MBCodedFlag = TRUE;
                                    cpi->MBCodingMode = cpi->pb.FragCodingMethod[FragIndex];

                                }
                            }
                        }				
                    }
                    // If the MB is marked as coded and we are in the Y plane then
                    // the mode list needs to be updated.
                    if ( MBCodedFlag && (FirstSB == 0) )
                    {
                        // Make a note of the selected mode in the mode list
                        cpi->ModeList[cpi->ModeListCount] = cpi->MBCodingMode;
                        cpi->ModeListCount++;
                    }

                } // valid macroblock

            } // Macroblocks

            // Next Super-Block
            SB++;

        } // SBCols

    } // SBRows

    // system state should be cleared here....
	cpi->pb.ClearSysState();

	// Return number of pixels coded
	return coded_pixels;
}

/****************************************************************************
 * 
 *  ROUTINE       :     PackCodedVideo
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Takes the encoded token lists etc. and creates an output 
 *                      bitstream. Additional restrictions to control bitrate can
 *                      also be applied at this point.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void PackCodedVideo (CP_INSTANCE *cpi)
{
    INT32 i;
    INT32 EncodedCoeffs = 1;
    INT32 TotalTokens = cpi->TotTokenCount;
    INT32 FragIndex;
    UINT32 HuffIndex;       // Index to groupof tables used to code a token

	cpi->pb.ClearSysState();

    // Reset the count of second order optimised tokens
    cpi->OptimisedTokenCount = 0;

    cpi->TokensToBeCoded = cpi->TotTokenCount;
    cpi->TokensCoded = 0;

    // Calculate the bit rate at which this frame should be capped.
    cpi->MaxBitTarget = (UINT32)((double)(cpi->ThisFrameTargetBytes * 8) * cpi->BitRateCapFactor);  

    // Blank the various fragment data structures before we start.
    memset(cpi->pb.FragCoeffs, 0, cpi->pb.UnitFragments);
    memset(cpi->FragTokens, 0, cpi->pb.UnitFragments);

    // Clear down the QFragData structure for all coded blocks.
    cpi->pb.ClearDownQFrag(&cpi->pb);
    
    // The tree is not needed (implicit) for key frames
    if ( GetFrameType(&cpi->pb) != BASE_FRAME )
    {
        // Pack the quad tree fragment mapping. 
        PackAndWriteDFArray( cpi );
    }

    // Note the number of bits used to code the tree itself.
    cpi->FrameBitCount = cpi->ThisFrameSize << 3;


    // Mode and MV data not needed for key frames.
    if ( GetFrameType(&cpi->pb) != BASE_FRAME )
    {
	    // Pack and code the mode list.
	    PackModes(cpi);


        // Pack the motion vectors
	    PackMotionVectors (cpi);

    }
    
    cpi->FrameBitCount = cpi->ThisFrameSize << 3;

    // Optimise the DC tokens
    for ( i = 0; i < cpi->pb.CodedBlockIndex; i++ )
    {
        // Get the linear index for the current fragment.
        FragIndex = cpi->pb.CodedBlockList[i];

		cpi->pb.FragCoefEOB[FragIndex]=EncodedCoeffs;
        PackToken(cpi, FragIndex, DC_HUFF_OFFSET );
        
    }

    // Pack any outstanding EOB tokens
    PackEOBRun(cpi);

    // Now output the optimised DC token list using the appropriate entropy tables.
    EncodeDcTokenList(cpi);

    // Work out the number of DC bits coded

    // Optimise the AC tokens
	while ( EncodedCoeffs < 64 )
	{
        // Huffman table adjustment based upon coefficient number.
        if ( EncodedCoeffs <= AC_TABLE_2_THRESH )
            HuffIndex = AC_HUFF_OFFSET;
        else if ( EncodedCoeffs <= AC_TABLE_3_THRESH )
            HuffIndex = AC_HUFF_OFFSET + AC_HUFF_CHOICES;
        else if ( EncodedCoeffs <= AC_TABLE_4_THRESH )
            HuffIndex = AC_HUFF_OFFSET + (AC_HUFF_CHOICES * 2);
        else
            HuffIndex = AC_HUFF_OFFSET + (AC_HUFF_CHOICES * 3);

        // Repeatedly scan through the list of blocks.
		for ( i = 0; i < cpi->pb.CodedBlockIndex; i++ )
		{
			// Get the linear index for the current fragment.
			FragIndex = cpi->pb.CodedBlockList[i];

			// Should we code a token for this block on this pass.
			if ( cpi->FragTokens[FragIndex] < cpi->FragTokenCounts[FragIndex]
				&& cpi->pb.FragCoeffs[FragIndex] <= EncodedCoeffs )
			{
				// Bit pack and a token for this block
				cpi->pb.FragCoefEOB[FragIndex]=EncodedCoeffs;
				PackToken( cpi, FragIndex, HuffIndex );
			}
        }

		EncodedCoeffs ++;
	}

	// Pack any outstanding EOB tokens
	PackEOBRun(cpi);


    // Now output the optimised AC token list using the appropriate entropy tables.
    EncodeAcTokenList(cpi);

}

/****************************************************************************
 * 
 *  ROUTINE       :     EncodeDcTokenList
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Output the DC token list using the selected entrpy method.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void EncodeDcTokenList (CP_INSTANCE *cpi)
{
    INT32   i,j;
    UINT32  Token;
    UINT32  ExtraBitsToken;
    UINT32  HuffIndex;

    UINT32  BestDcBits;
    UINT32  DcHuffChoice[2];
    UINT32  EntropyTableBits[2][DC_HUFF_CHOICES];

    // Clear table data structure
    memset ( EntropyTableBits, 0, sizeof(UINT32)*DC_HUFF_CHOICES*2 );

    // Analyse token list to see which is the best entropy table to use
    for ( i = 0; i < cpi->OptimisedTokenCount; i++ )
    {
        // Count number of bits for each table option
        Token = (UINT32)cpi->OptimisedTokenList[i];
        for ( j = 0; j < DC_HUFF_CHOICES; j++ )
        {
            EntropyTableBits[cpi->OptimisedTokenListPl[i]][j] += cpi->pb.HuffCodeLengthArray_VP3x[DC_HUFF_OFFSET + j][Token];
        }
    }

    // Work out which table option is best for Y
    BestDcBits = EntropyTableBits[0][0];
    DcHuffChoice[0] = 0;
    for ( j = 1; j < DC_HUFF_CHOICES; j++ )
    {
        if ( EntropyTableBits[0][j] < BestDcBits )
        {
            BestDcBits = EntropyTableBits[0][j];
            DcHuffChoice[0] = j;
        }
    }

    // Add the DC huffman table choice to the bitstream
    AddBitsToBuffer( cpi, DcHuffChoice[0], DC_HUFF_CHOICE_BITS );

    // Work out which table option is best for UV
    BestDcBits = EntropyTableBits[1][0];
    DcHuffChoice[1] = 0;
    for ( j = 1; j < DC_HUFF_CHOICES; j++ )
    {
        if ( EntropyTableBits[1][j] < BestDcBits )
        {
            BestDcBits = EntropyTableBits[1][j];
            DcHuffChoice[1] = j;
        }
    }

    // Add the DC huffman table choice to the bitstream
    AddBitsToBuffer( cpi, DcHuffChoice[1], DC_HUFF_CHOICE_BITS );

    // Encode the token list
    for ( i = 0; i < cpi->OptimisedTokenCount; i++ )
    {

        // Get the token and extra bits
        Token = (UINT32)cpi->OptimisedTokenList[i];
        ExtraBitsToken = (UINT32)cpi->OptimisedTokenListEb[i];

        // Select the huffman table
        //HuffIndex = (UINT32)DC_HUFF_OFFSET + (UINT32)DcHuffChoice[cpi->OptimisedTokenListPl[i]];
        if ( cpi->OptimisedTokenListPl[i] == 0)
            HuffIndex = (UINT32)DC_HUFF_OFFSET + (UINT32)DcHuffChoice[0];
        else
            HuffIndex = (UINT32)DC_HUFF_OFFSET + (UINT32)DcHuffChoice[1];

        // Add the bits to the encode holding buffer.     
        cpi->FrameBitCount += cpi->pb.HuffCodeLengthArray_VP3x[HuffIndex][Token];
        AddBitsToBuffer( cpi, cpi->pb.HuffCodeArray_VP3x[HuffIndex][Token], (UINT32)cpi->pb.HuffCodeLengthArray_VP3x[HuffIndex][Token] );

        // If the token is followed by an extra bits token then code it
		if ( ExtraBitLengths_VP31[Token] > 0 )
	    {
		    // Add the bits to the encode holding buffer. 
            cpi->FrameBitCount += ExtraBitLengths_VP31[Token];
			AddBitsToBuffer( cpi, ExtraBitsToken, (UINT32)ExtraBitLengths_VP31[Token] );
        }
    }

    // Reset the count of second order optimised tokens
    cpi->OptimisedTokenCount = 0;
}

/****************************************************************************
 * 
 *  ROUTINE       :     EncodeAcTokenList
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Output the AC token list using the selected entrpy method.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void EncodeAcTokenList (CP_INSTANCE *cpi)
{
    INT32   i,j;
    UINT32  Token;
    UINT32  ExtraBitsToken;
    UINT32  HuffIndex;

    UINT32  BestAcBits;
    UINT32  AcHuffChoice[2];
    UINT32  EntropyTableBits[2][AC_HUFF_CHOICES];

    // Clear table data structure
    memset ( EntropyTableBits, 0, sizeof(UINT32)*AC_HUFF_CHOICES*2 );

    // Analyse token list to see which is the best entropy table to use
    for ( i = 0; i < cpi->OptimisedTokenCount; i++ )
    {
        // Count number of bits for each table option
        Token = (UINT32)cpi->OptimisedTokenList[i];
        HuffIndex = cpi->OptimisedTokenListHi[i];
        for ( j = 0; j < AC_HUFF_CHOICES; j++ )
        {
            EntropyTableBits[cpi->OptimisedTokenListPl[i]][j] += cpi->pb.HuffCodeLengthArray_VP3x[HuffIndex + j][Token];
        }
    }

    // Select the best set of AC tables for Y
    BestAcBits = EntropyTableBits[0][0];
    AcHuffChoice[0] = 0;
    for ( j = 1; j < AC_HUFF_CHOICES; j++ )
    {
        if ( EntropyTableBits[0][j] < BestAcBits )
        {
            BestAcBits = EntropyTableBits[0][j];
            AcHuffChoice[0] = j;
        }
    }

    // Add the AC-Y huffman table choice to the bitstream
    AddBitsToBuffer( cpi, AcHuffChoice[0], AC_HUFF_CHOICE_BITS );

    // Select the best set of AC tables for UV
    BestAcBits = EntropyTableBits[1][0];
    AcHuffChoice[1] = 0;
    for ( j = 1; j < AC_HUFF_CHOICES; j++ )
    {
        if ( EntropyTableBits[1][j] < BestAcBits )
        {
            BestAcBits = EntropyTableBits[1][j];
            AcHuffChoice[1] = j;
        }
    }

    // Add the AC-UV huffman table choice to the bitstream
    AddBitsToBuffer( cpi, AcHuffChoice[1], AC_HUFF_CHOICE_BITS );

    // Encode the token list
    for ( i = 0; i < cpi->OptimisedTokenCount; i++ )
    {
        // Get the token and extra bits
        Token = (UINT32)cpi->OptimisedTokenList[i];
        ExtraBitsToken = (UINT32)cpi->OptimisedTokenListEb[i];

        // Select the huffman table
        HuffIndex = (UINT32)cpi->OptimisedTokenListHi[i] + AcHuffChoice[cpi->OptimisedTokenListPl[i]];

        // Add the bits to the encode holding buffer.     
        cpi->FrameBitCount += cpi->pb.HuffCodeLengthArray_VP3x[HuffIndex][Token];
        AddBitsToBuffer( cpi, cpi->pb.HuffCodeArray_VP3x[HuffIndex][Token], (UINT32)cpi->pb.HuffCodeLengthArray_VP3x[HuffIndex][Token] );

        // If the token is followed by an extra bits token then code it
	    if ( ExtraBitLengths_VP31[Token] > 0 )
	    {
		    // Add the bits to the encode holding buffer. 
            cpi->FrameBitCount += ExtraBitLengths_VP31[Token];
		    AddBitsToBuffer( cpi, ExtraBitsToken, (UINT32)ExtraBitLengths_VP31[Token] );
        }
    }

    // Reset the count of second order optimised tokens
    cpi->OptimisedTokenCount = 0;
}

/****************************************************************************
 * 
 *  ROUTINE       :     PackModes
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Encodes and packs the mode list.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void PackModes (CP_INSTANCE *cpi)
{
    UINT32  i,j;
    UINT8   ModeIndex;
    
    INT32   ModeCount[MAX_MODES];
    INT32   TmpFreq;
    INT32   TmpIndex;

    UINT8   BestScheme;
    UINT32  BestSchemeScore;
    UINT32  SchemeScore;

    __try
    {
        // Build a frequency map for the modes in this frame
        memset( ModeCount, 0, MAX_MODES*sizeof(INT32) );
        for ( i = 0; i < cpi->ModeListCount; i++ )
        {
            ModeCount[cpi->ModeList[i]] ++;
        }

        // Order the modes from most to least frequent.
        // Store result as scheme 0
        for ( j = 0; j < MAX_MODES; j++ )
        {
            // Find the most frequent
            TmpFreq = -1;
            for ( i = 0; i < MAX_MODES; i++ )
            {
                // Is this the best scheme so far ???
                if ( ModeCount[i] > TmpFreq )
                {
                    TmpFreq = ModeCount[i];
                    TmpIndex = i;
                }
            }
            ModeCount[TmpIndex] = -1;
            ModeSchemes[0][TmpIndex] = j;
        }

        // Default/ fallback scheme uses MODE_BITS bits per mode entry
        BestScheme = (MODE_METHODS - 1);
        BestSchemeScore = cpi->ModeListCount * 3;
        // Get a bit score for the available schemes.
        for (  j = 0; j < (MODE_METHODS - 1); j++ )
        {
            // Reset the scheme score
            if ( j == 0 )
                SchemeScore = 24;    // Scheme 0 additional cost of sending frequency order
            else
                SchemeScore = 0;

            // Find the total bits to code using each avaialable scheme
            for ( i = 0; i < cpi->ModeListCount; i++ )
            {
                SchemeScore += ModeBitLengths[ModeSchemes[j][cpi->ModeList[i]]];
            }

            // Is this the best scheme so far ???
            if ( SchemeScore < BestSchemeScore )
            {
                BestSchemeScore = SchemeScore;
                BestScheme = j;
            }
        }

        // Encode the best scheme.
        AddBitsToBuffer( cpi, BestScheme, (UINT32)MODE_METHOD_BITS );

        // If the chosen schems is scheme 0 send details of the mode frequency order
        if ( BestScheme == 0 )
        {
            for ( j = 0; j < MAX_MODES; j++ )
            {
                // Note that the last two entries are implicit
                AddBitsToBuffer( cpi, ModeSchemes[0][j], (UINT32)MODE_BITS );
            }
        }

        // Are we using one of the alphabet based schemes or the fallback scheme
        if ( BestScheme < (MODE_METHODS - 1))
        {
            // Pack and encode the Mode list
            for ( i = 0; i < cpi->ModeListCount; i++ )
            {
                // Add the appropriate mode entropy token.
                ModeIndex = ModeSchemes[BestScheme][cpi->ModeList[i]];
                AddBitsToBuffer( cpi, ModeBitPatterns[ModeIndex], (UINT32)ModeBitLengths[ModeIndex] );
            }
        }
        else
        {
            // Fall back to MODE_BITS per entry
            for ( i = 0; i < cpi->ModeListCount; i++ )
            {
                // Add the appropriate mode entropy token.
                AddBitsToBuffer( cpi, cpi->ModeList[i], MODE_BITS  );
            }
        }
    }
    __except ( TRUE )
    {
        // Signal an error
        ErrorTrap( &cpi->pb, EX_UNQUAL_ERROR );
    }
}

/****************************************************************************
 * 
 *  ROUTINE       :     PackMotionVectors
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Codes and packs the motion vector list.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void PackMotionVectors (CP_INSTANCE *cpi)
{
    INT32  i;
    UINT32 MethodBits[2] = {0,0};

    // MV entropy variables
    UINT32 * MvBitsPtr;
    UINT32 * MvPatternPtr;  

    INT32   LastXMVComponent = 0;
    INT32   LastYMVComponent = 0;

    __try
    {
        // Choose the coding method
        MvBitsPtr = &MvBits[MAX_MV_EXTENT];
        for ( i = 0; i < (INT32)cpi->MvListCount; i++ )
        {
            MethodBits[0] += MvBitsPtr[cpi->MVList[i].x]; 
            MethodBits[0] += MvBitsPtr[cpi->MVList[i].y];
            MethodBits[1] += 12;               // Simple six bits per mv component fallback mechanism
        }

        // Select entropy table
        if ( MethodBits[0] < MethodBits[1] )
        {
            AddBitsToBuffer( cpi, 0, 1 );
            MvBitsPtr = &MvBits[MAX_MV_EXTENT];
            MvPatternPtr = &MvPattern[MAX_MV_EXTENT];
        }
        else
        {
            AddBitsToBuffer( cpi, 1, 1 );
            MvBitsPtr = &MvBits2[MAX_MV_EXTENT];
            MvPatternPtr = &MvPattern2[MAX_MV_EXTENT];
        }

        // Pack and encode the motion vectors
        for ( i = 0; i < (INT32)cpi->MvListCount; i++ )
        {
            AddBitsToBuffer( cpi, MvPatternPtr[cpi->MVList[i].x], (UINT32)MvBitsPtr[cpi->MVList[i].x] );
        
            AddBitsToBuffer( cpi, MvPatternPtr[cpi->MVList[i].y], (UINT32)MvBitsPtr[cpi->MVList[i].y] );
        }
    }
    __except ( TRUE )
    {
        // Signal an error
        ErrorTrap( &cpi->pb, EX_UNQUAL_ERROR );
    }
}


/****************************************************************************
 * 
 *  ROUTINE       :     PackEOBRun
 *
 *  INPUTS        :     None
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None
 *
 *  FUNCTION      :     Code up a run of EOB tokens
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void PackEOBRun( CP_INSTANCE *cpi)
{
    if(cpi->RunLength == 0)
    {
        return;
    }

    // Note the appropriate EOB or EOB run token and any extra bits in the optimised token list
    // Use the huffman index assosciated with the first token in the run
    


    // Mark out which plane the block belonged to
    cpi->OptimisedTokenListPl[cpi->OptimisedTokenCount] = cpi->RunPlaneIndex;
    
    // Note the huffman index to be used
    cpi->OptimisedTokenListHi[cpi->OptimisedTokenCount] = (UINT8)cpi->RunHuffIndex;
    
    if ( cpi->RunLength <= 3 )
    {
        if ( cpi->RunLength == 1 )
        {
            cpi->OptimisedTokenList[cpi->OptimisedTokenCount] = DCT_EOB_TOKEN;
        }
        else if ( cpi->RunLength == 2 )
        {
            cpi->OptimisedTokenList[cpi->OptimisedTokenCount] = DCT_EOB_PAIR_TOKEN;
        }
        else
        {
            cpi->OptimisedTokenList[cpi->OptimisedTokenCount] = DCT_EOB_TRIPLE_TOKEN;
        }
        
        cpi->RunLength = 0;
    }
    else
    {
        // Choose a token appropriate to the run length.
        if ( cpi->RunLength < 8 )
        {
            cpi->OptimisedTokenList[cpi->OptimisedTokenCount] = DCT_REPEAT_RUN_TOKEN;
            cpi->OptimisedTokenListEb[cpi->OptimisedTokenCount] = cpi->RunLength - 4;
            cpi->RunLength = 0;
        }
        else if ( cpi->RunLength < 16 )
        {
            cpi->OptimisedTokenList[cpi->OptimisedTokenCount] = DCT_REPEAT_RUN2_TOKEN;
            cpi->OptimisedTokenListEb[cpi->OptimisedTokenCount] = cpi->RunLength - 8;
            cpi->RunLength = 0;
        }
        else if ( cpi->RunLength < 32 )
        {
            cpi->OptimisedTokenList[cpi->OptimisedTokenCount] = DCT_REPEAT_RUN3_TOKEN;
            cpi->OptimisedTokenListEb[cpi->OptimisedTokenCount] = cpi->RunLength - 16;
            cpi->RunLength = 0;
        }
        else if ( cpi->RunLength < 4096)
        {
            cpi->OptimisedTokenList[cpi->OptimisedTokenCount] = DCT_REPEAT_RUN4_TOKEN;
            cpi->OptimisedTokenListEb[cpi->OptimisedTokenCount] = cpi->RunLength;
            cpi->RunLength = 0;
        }
        else 
        {
            IssueWarning("PackEOBRun : RunLength > 4095");
        };
        
    }
     
    // Increment the cpi->Optimised token count
    cpi->OptimisedTokenCount++;

    // Reset run EOB length
    cpi->RunLength = 0;
}

/****************************************************************************
 * 
 *  ROUTINE       :     GetBlockReconErrorSlow
 *
 *  INPUTS        :     UINT32 BlockIndex
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     A reconstruction error score for the given block
 *
 *  FUNCTION      :     Calculates reconstruction error score for the given block.
 *
 *  SPECIAL NOTES :     This function was GetBlockReconError
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

UINT32 GetBlockReconErrorSlow( CP_INSTANCE *cpi, INT32 BlockIndex )
{
	UINT32	i;
	UINT32	ErrorVal = 0;

    UINT8 * SrcDataPtr = &cpi->ConvDestBuffer[GetFragIndex(cpi->pb.pixel_index_table,BlockIndex)];
    UINT8 * RecDataPtr = &cpi->pb.LastFrameRecon[GetFragIndex(cpi->pb.recon_pixel_index_table,BlockIndex)];
    INT32   SrcStride;
    INT32   RecStride;

    // Is the block a Y block or a UV block.
    if ( BlockIndex < (INT32)cpi->pb.YPlaneFragments )
    {
        SrcStride = cpi->pb.Configuration.VideoFrameWidth;
        RecStride = cpi->pb.Configuration.YStride;
    }
    else
    {
        SrcStride = cpi->pb.Configuration.VideoFrameWidth >> 1;
        RecStride = cpi->pb.Configuration.UVStride;
    }
    

	// Decide on standard or MMX implementation
	for ( i=0; i < BLOCK_HEIGHT_WIDTH; i++ )
	{
		ErrorVal += AbsX_LUT[ ((int)SrcDataPtr[0]) - ((int)RecDataPtr[0]) ];
		ErrorVal += AbsX_LUT[ ((int)SrcDataPtr[1]) - ((int)RecDataPtr[1]) ];
		ErrorVal += AbsX_LUT[ ((int)SrcDataPtr[2]) - ((int)RecDataPtr[2]) ];
		ErrorVal += AbsX_LUT[ ((int)SrcDataPtr[3]) - ((int)RecDataPtr[3]) ];
		ErrorVal += AbsX_LUT[ ((int)SrcDataPtr[4]) - ((int)RecDataPtr[4]) ];
		ErrorVal += AbsX_LUT[ ((int)SrcDataPtr[5]) - ((int)RecDataPtr[5]) ];
		ErrorVal += AbsX_LUT[ ((int)SrcDataPtr[6]) - ((int)RecDataPtr[6]) ];
		ErrorVal += AbsX_LUT[ ((int)SrcDataPtr[7]) - ((int)RecDataPtr[7]) ];
		// Step to next row of block.
		SrcDataPtr += SrcStride;
		RecDataPtr += RecStride;
	}
	return ErrorVal;
}


/****************************************************************************
 * 
 *  ROUTINE       :     PackToken
 *
 *  INPUTS        :     INT32 FragmentNumber
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Packs a token for the given fragment
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void PackToken ( CP_INSTANCE *cpi, INT32 FragmentNumber, UINT32 HuffIndex )
{
    UINT32 Token = cpi->pb.TokenList[FragmentNumber][cpi->FragTokens[FragmentNumber]];
    UINT32 ExtraBitsToken = cpi->pb.TokenList[FragmentNumber][cpi->FragTokens[FragmentNumber] + 1];
	UINT32 OneOrTwo;
	UINT32 OneOrZero;

    // Update the record of what coefficient we have got up to for this block
    // And unpack the encoded token back into the quantised data array.
    if ( Token == DCT_EOB_TOKEN )
        cpi->pb.FragCoeffs[FragmentNumber] = BLOCK_SIZE;
    else
        ExpandToken( &cpi->pb, cpi->pb.QFragData[FragmentNumber], &cpi->pb.FragCoeffs[FragmentNumber], Token, ExtraBitsToken );

    // Update record of tokens coded and where we are in this fragment.
	OneOrTwo= 1 + ( ExtraBitLengths_VP31[Token] > 0 );       // Is there an extra bits token
	// Advance to the next real token. 
	cpi->FragTokens[FragmentNumber] += OneOrTwo;
	
	// Update the counts of tokens coded
	cpi->TokensCoded += OneOrTwo;
	cpi->TokensToBeCoded -= OneOrTwo;
	
	OneOrZero = ( FragmentNumber < (INT32)cpi->pb.YPlaneFragments );

    if ( Token == DCT_EOB_TOKEN )
    {
        if ( cpi->RunLength == 0 )
        {
            cpi->RunHuffIndex = HuffIndex;
            cpi->RunPlaneIndex = 1 -  OneOrZero;
        }
        cpi->RunLength++;

        // we have exceeded our longest run length  xmit an eob run token;
        if ( cpi->RunLength == 4095 )
        {
            PackEOBRun(cpi);
        }
    }
    else 
    {
		// If we have an EOB run then code it up first
        if ( cpi->RunLength > 0 )
        {
            // Pack the EOB token
            PackEOBRun( cpi);
        }

        // Mark out which plane the block belonged to
        cpi->OptimisedTokenListPl[cpi->OptimisedTokenCount] = 1 - OneOrZero;

        // Note the token, extra bits and hufman table in the optimised token list
        cpi->OptimisedTokenList[cpi->OptimisedTokenCount] = (UINT8)Token;
        cpi->OptimisedTokenListEb[cpi->OptimisedTokenCount] = ExtraBitsToken;
        cpi->OptimisedTokenListHi[cpi->OptimisedTokenCount] = (UINT8)HuffIndex;

        cpi->OptimisedTokenCount++;
    }
}
/****************************************************************************
 * 
 *  ROUTINE       :     AddMotionVector
 *
 *  INPUTS        :     UINT32	HFrags
 *						UINT32	VFrags
 *						UINT32	FirstMBIndex
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Adds a motion vector to the motion vector list
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void AddMotionVector( 
    CP_INSTANCE *cpi,     
    MOTION_VECTOR *ThisMotionVector
    )
{
    cpi->MVList[cpi->MvListCount].x = ThisMotionVector->x;
    cpi->MVList[cpi->MvListCount].y = ThisMotionVector->y;
    cpi->MvListCount ++;
}

/****************************************************************************
 * 
 *  ROUTINE       :     SetFragMotionVectorandMode
 *
 *  INPUTS        :     UINT32	HFrags
 *						UINT32	VFrags
 *						UINT32	FirstMBIndex
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     sets the motion vector for a fragment to the given 
 *                      values; and the codeing mode
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void SetFragMotionVectorAndMode( 
    CP_INSTANCE *cpi,     
    INT32 FragIndex,
    MOTION_VECTOR *ThisMotionVector
    )
{
    // Note the coding mode and vector for each block
    cpi->pb.FragMVect[FragIndex].x = ThisMotionVector->x;
    cpi->pb.FragMVect[FragIndex].y = ThisMotionVector->y;
    cpi->pb.FragCodingMethod[FragIndex] = cpi->MBCodingMode;

}

/****************************************************************************
 * 
 *  ROUTINE       :     SetMBMotionVectorsandMode
 *
 *  INPUTS        :     UINT32	HFrags
 *						UINT32	VFrags
 *						UINT32	FirstMBIndex
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     sets all the motion vectors for mb fragments to the given 
 *                      values; and the codeing mode
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
void SetMBMotionVectorsAndMode( 
    CP_INSTANCE *cpi,     
    INT32 YFragIndex,
    INT32 UFragIndex,
    INT32 VFragIndex,
    MOTION_VECTOR *ThisMotionVector
    )
{
    SetFragMotionVectorAndMode(cpi, YFragIndex, ThisMotionVector);
    SetFragMotionVectorAndMode(cpi, YFragIndex + 1, ThisMotionVector);
    SetFragMotionVectorAndMode(cpi, YFragIndex + cpi->pb.HFragments, ThisMotionVector);
    SetFragMotionVectorAndMode(cpi, YFragIndex + cpi->pb.HFragments + 1, ThisMotionVector);
    SetFragMotionVectorAndMode(cpi, UFragIndex, ThisMotionVector);
    SetFragMotionVectorAndMode(cpi, VFragIndex, ThisMotionVector);
}


/****************************************************************************
 * 
 *  ROUTINE       :     PickModes
 *
 *  INPUTS        :     
 *
 *  OUTPUTS       :     
 *
 *  RETURNS       :     A motion score 
 *
 *  FUNCTION      :     Picks macroblock coding types for each macroblock 
 *                      of the image.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 PickModes( CP_INSTANCE *cpi, UINT32 SBRows, UINT32 SBCols, UINT32 HExtra, UINT32 VExtra, UINT32 PixelsPerLine, 
                 UINT32 *InterError, UINT32 *IntraError)
{
	INT32	YFragIndex;			    // Fragment number
	INT32	UFragIndex;			    // Fragment number
	INT32	VFragIndex;			    // Fragment number
	UINT32	MB, B;				    // Macro-Block, Block indices
	UINT32	SBrow;				    // Super-Block row number
	UINT32	SBcol;				    // Super-Block row number
	UINT32	SB=0;		    	    // Super-Block index, initialised to first of this component

    UINT32  MBIntraError;           // Intra error for macro block  
    UINT32  MBGFError;              // Golden frame macro block error
    UINT32  MBGF_MVError;           // Golden frame plus MV error
	UINT32  LastMBGF_MVError;       // Golden frame error with last used GF motion vector.
    UINT32  MBInterError;           // Inter no MV macro block error
    UINT32  MBLastInterError;       // Inter with last used MV
    UINT32  MBPriorLastInterError;  // Inter with prior last MV
    UINT32  MBInterMVError;         // Inter MV macro block error
    UINT32  MBInterMVExError;       // Inter MV (exhaustive search) macro block error
    UINT32  MBInterFOURMVError;     // Inter MV error when using 4 motion vectors per macro block
    UINT32  BestError;              // Best error so far.

    MOTION_VECTOR FourMVect[6];   // storage for last used vectors (one entry for each block in MB)
    MOTION_VECTOR LastInterMVect;   // storage for last used Inter frame MB motion vector
    MOTION_VECTOR PriorLastInterMVect;  // storage for prior last used Inter frame MB motion vector
    MOTION_VECTOR TmpMVect;         // Temporary MV storage
    MOTION_VECTOR LastGFMVect;      // storage for last used Golden Frame MB motion vector
    MOTION_VECTOR InterMVect;       // storage for motion vector
    MOTION_VECTOR InterMVectEx;     // storage for motion vector result from exhaustive search
    MOTION_VECTOR GFMVect;          // storage for motion vector
    MOTION_VECTOR ZeroVect;

    UINT32 UVRow;
    UINT32 UVColumn;
    UINT32 UVFragOffset;

    BOOL   MBCodedFlag;
    UINT8   QIndex;

    // initialize error scores
    *InterError = 0;
    *IntraError = 0;

    // clear down the default motion vector.
    cpi->MvListCount = 0;
    FourMVect[0].x = 0;
    FourMVect[0].y = 0;
    FourMVect[1].x = 0;
    FourMVect[1].y = 0;
    FourMVect[2].x = 0;
    FourMVect[2].y = 0;
    FourMVect[3].x = 0;
    FourMVect[3].y = 0;
    FourMVect[4].x = 0;
    FourMVect[4].y = 0;
    FourMVect[5].x = 0;
    FourMVect[5].y = 0;
    LastInterMVect.x = 0;
    LastInterMVect.y = 0;
    PriorLastInterMVect.x = 0;
    PriorLastInterMVect.y = 0;
    LastGFMVect.x = 0;
    LastGFMVect.y = 0;
    InterMVect.x = 0;
    InterMVect.y = 0;
    GFMVect.x = 0;
    GFMVect.y = 0;

    ZeroVect.x = 0;
    ZeroVect.y = 0;

    QIndex = cpi->pb.FrameQIndex;



	// change the quatization matrix to the one at best Q
	// to compute the new error score
    cpi->MinImprovementForNewMV = (MvThreshTable[QIndex] << 12); 
    cpi->InterTripOutThresh = (5000<<12);
    cpi->MVChangeFactor = MVChangeFactorTable[QIndex]; // 0.9;

    if ( cpi->QuickCompress )
    {
        cpi->ExhaustiveSearchThresh = (1000<<12);
        cpi->FourMVThreshold = (2500<<12);
    } 
    else
    {
        cpi->ExhaustiveSearchThresh = (250<<12);
        cpi->FourMVThreshold = (500<<12);
    }
    cpi->MinImprovementForFourMV = cpi->MinImprovementForNewMV * 4;     

    if(cpi->MinImprovementForFourMV < (40<<12))
        cpi->MinImprovementForFourMV = (40<<12);
    
    cpi->FourMvChangeFactor = 8;// cpi->MVChangeFactor - 0.05; // 0.85;

    // decide what block type and motion vectors to use on all of the frames
	for ( SBrow=0; SBrow<SBRows; SBrow++ )
	{
		for ( SBcol=0; SBcol<SBCols; SBcol++ )
		{
			// Check its four Macro-Blocks
			for ( MB=0; MB<4; MB++ )
			{
				// There may be MB's lying out of frame
				// which must be ignored. For these MB's
				// Top left block will have a negative Fragment Index.
				if ( QuadMapToMBTopLeft(cpi->pb.BlockMap,SB,MB) < 0 )
                {
                    continue;
                }
                
                // Is the current macro block coded (in part or in whole)
                MBCodedFlag = FALSE;  
                for ( B=0; B<4; B++ )
                {
                    YFragIndex = QuadMapToIndex1( cpi->pb.BlockMap, SB, MB, B );
                   
                    // Does Block lie in frame:
                    if ( YFragIndex >= 0 )
                    {
                        // In Frame: Is it coded:
                        if ( cpi->pb.display_fragments[YFragIndex] )
                        {
                            MBCodedFlag = TRUE;
                            break;
                        }
                    }	
                    else
                        MBCodedFlag = FALSE;
                }
                
                // This one isn't coded go to the next one
                if(!MBCodedFlag)
                {
                    continue;
                }

                // Calculate U and V FragIndex from YFragIndex
                YFragIndex = QuadMapToMBTopLeft(cpi->pb.BlockMap, SB,MB);
                UVRow = (YFragIndex / (cpi->pb.HFragments * 2));
                UVColumn = (YFragIndex % cpi->pb.HFragments) / 2;
                UVFragOffset = (UVRow * (cpi->pb.HFragments / 2)) + UVColumn;
                UFragIndex = cpi->pb.YPlaneFragments + UVFragOffset;
                VFragIndex = cpi->pb.YPlaneFragments + cpi->pb.UVPlaneFragments + UVFragOffset;

                
                //**************************************************************
                // Find the block choice with the lowest error 

                // NOTE THAT if U or V is coded but no Y from a macro block then
                // the mode will be CODE_INTER_NO_MV as this is the default state to which
                // the mode data structure is initialised in encoder and decoder 
                // at the start of each frame.

                BestError = HUGE_ERROR;
                

                // Look at the intra coding error.
                MBIntraError = GetMBIntraError( cpi, YFragIndex, PixelsPerLine );
                BestError = (BestError > MBIntraError) ? MBIntraError : BestError;
                
                // Get the golden frame error
                MBGFError = GetMBInterError( cpi, cpi->ConvDestBuffer, cpi->pb.GoldenFrame, YFragIndex, 0, 0, PixelsPerLine );
                BestError = (BestError > MBGFError) ? MBGFError : BestError;
                
                // Calculate the 0,0 case.
                MBInterError = GetMBInterError( cpi, cpi->ConvDestBuffer, cpi->pb.LastFrameRecon, YFragIndex, 0, 0, PixelsPerLine );
                BestError = (BestError > MBInterError) ? MBInterError : BestError;
                
                // Measure error for last MV
                MBLastInterError =  GetMBInterError( cpi, cpi->ConvDestBuffer, cpi->pb.LastFrameRecon, YFragIndex, LastInterMVect.x, LastInterMVect.y, PixelsPerLine );
                BestError = (BestError > MBLastInterError) ? MBLastInterError : BestError;
                
                // Measure error for prior last MV
                MBPriorLastInterError =  GetMBInterError( cpi, cpi->ConvDestBuffer, cpi->pb.LastFrameRecon, YFragIndex, PriorLastInterMVect.x, PriorLastInterMVect.y, PixelsPerLine );
                BestError = (BestError > MBPriorLastInterError) ? MBPriorLastInterError : BestError;

                // Temporarily force usage of no motionvector blocks
                MBInterMVError = HUGE_ERROR;
                InterMVect.x = 0;                     // Set 0,0 motion vector
                InterMVect.y = 0;
                
                // If the best error is above the required threshold search for a new inter MV
                if ( BestError > cpi->MinImprovementForNewMV ) 
                {
                    // Use a mix of heirachical and exhaustive searches for quick mode.
                    if ( cpi->QuickCompress )
                    {
                        MBInterMVError = GetMBMVInterError( cpi, cpi->pb.LastFrameRecon, YFragIndex, PixelsPerLine, cpi->MVPixelOffsetY, &InterMVect );
                        
                        // If we still do not have a good match try an exhaustive MBMV search
                        if ( (MBInterMVError > cpi->ExhaustiveSearchThresh) && 
                            (BestError > cpi->ExhaustiveSearchThresh) ) 
                        {
                            MBInterMVExError = GetMBMVExhaustiveSearch( cpi, cpi->pb.LastFrameRecon, YFragIndex, PixelsPerLine, &InterMVectEx );
                            
                            // Is the Variance measure for the EX search better... If so then use it.
                            if ( MBInterMVExError < MBInterMVError )
                            {
                                MBInterMVError = MBInterMVExError;   
                                InterMVect.x = InterMVectEx.x;
                                InterMVect.y = InterMVectEx.y;
                            }
                        }
                    }
                    else
                    {
                        // Use an exhaustive search
                        MBInterMVError = GetMBMVExhaustiveSearch( cpi, cpi->pb.LastFrameRecon, YFragIndex, PixelsPerLine, &InterMVect );
                    }
                    
                    
                    // Is the improvement, if any, good enough to justify a new MV
                    if ( (16 * MBInterMVError < (BestError * cpi->MVChangeFactor)) && 
                        ((MBInterMVError + cpi->MinImprovementForNewMV) < BestError) )
                    {
                        BestError = MBInterMVError;
                    }
                    
                }
                
                // If the best error is still above the required threshold search for a golden frame MV
                MBGF_MVError = HUGE_ERROR;
                GFMVect.x = 0;                          // Set 0,0 motion vector
                GFMVect.y = 0;
                if ( BestError > cpi->MinImprovementForNewMV )
                {
                    // Do an MV search in the golden reference frame
                    MBGF_MVError = GetMBMVInterError( cpi, cpi->pb.GoldenFrame, YFragIndex, PixelsPerLine, cpi->MVPixelOffsetY, &GFMVect );
                    
                    // Measure error for last GFMV
                    LastMBGF_MVError =  GetMBInterError( cpi, cpi->ConvDestBuffer, cpi->pb.GoldenFrame, YFragIndex, LastGFMVect.x, LastGFMVect.y, PixelsPerLine );
                    
                    // Check against last GF motion vector and reset if the search has thrown a worse result.
                    if ( LastMBGF_MVError < MBGF_MVError )
                    {
                        GFMVect.x = LastGFMVect.x;
                        GFMVect.y = LastGFMVect.y;
                        MBGF_MVError = LastMBGF_MVError;
                    }
                    else
                    {
                        LastGFMVect.x = GFMVect.x;
                        LastGFMVect.y = GFMVect.y;
                    }
                    // Is the improvement, if any, good enough to justify a new MV
                    if ( (16 * MBGF_MVError < (BestError * cpi->MVChangeFactor)) && 
                        ((MBGF_MVError + cpi->MinImprovementForNewMV) < BestError) )
                    {
                        BestError = MBGF_MVError;
                    }
                }

                // Finaly... If the best error is still to high then consider the 4MV mode
                MBInterFOURMVError = HUGE_ERROR;
                if ( BestError > cpi->FourMVThreshold ) 
                {
                    // Get the 4MV error.
                    MBInterFOURMVError = GetFOURMVExhaustiveSearch( cpi, cpi->pb.LastFrameRecon, YFragIndex, PixelsPerLine, FourMVect );
                    
                    // If the improvement is great enough then use the four MV mode
                    if ( ((MBInterFOURMVError + cpi->MinImprovementForFourMV) < BestError) &&
                        (16 * MBInterFOURMVError < (BestError * cpi->FourMvChangeFactor)))
                    {
                        BestError = MBInterFOURMVError;
                    }
                }
                // end finding the best error 
                //*******************************************************

                //*******************************************************
                // Figure out what to do with the block we chose

                // Over-ride and force intra if error high and Intra error similar
                // Now choose a mode based on lowest error (with bias towards no MV)
                if ( (BestError > cpi->InterTripOutThresh) && 
                     (10 * BestError > MBIntraError * 7 ) )
                {
                    cpi->MBCodingMode = CODE_INTRA;
                    SetMBMotionVectorsAndMode(cpi,YFragIndex,UFragIndex,VFragIndex,&ZeroVect);
                }
                
                else if ( BestError == MBInterError )
                {
                    cpi->MBCodingMode = CODE_INTER_NO_MV;
                    SetMBMotionVectorsAndMode(cpi,YFragIndex,UFragIndex,VFragIndex,&ZeroVect);
                }
                else if ( BestError == MBGFError )
                {
                    cpi->MBCodingMode = CODE_USING_GOLDEN;
                    SetMBMotionVectorsAndMode(cpi,YFragIndex,UFragIndex,VFragIndex,&ZeroVect);
                }
                else if ( BestError == MBLastInterError )
                {
                    cpi->MBCodingMode = CODE_INTER_LAST_MV;
                    SetMBMotionVectorsAndMode(cpi,YFragIndex,UFragIndex,VFragIndex,&LastInterMVect);
                }
                else if ( BestError == MBPriorLastInterError )
                {
                    cpi->MBCodingMode = CODE_INTER_PRIOR_LAST;
                    SetMBMotionVectorsAndMode(cpi,YFragIndex,UFragIndex,VFragIndex,&PriorLastInterMVect);
                    
                    // Swap the prior and last MV cases over
                    TmpMVect.x = PriorLastInterMVect.x;
                    TmpMVect.y = PriorLastInterMVect.y;
                    PriorLastInterMVect.x = LastInterMVect.x;
                    PriorLastInterMVect.y = LastInterMVect.y;
                    LastInterMVect.x = TmpMVect.x;
                    LastInterMVect.y = TmpMVect.y;
                }
                else if ( BestError == MBInterMVError )
                {
                    cpi->MBCodingMode = CODE_INTER_PLUS_MV;
                    SetMBMotionVectorsAndMode(cpi,YFragIndex,UFragIndex,VFragIndex,&InterMVect);

                    // Update Prior last mv with last mv
                    PriorLastInterMVect.x = LastInterMVect.x;
                    PriorLastInterMVect.y = LastInterMVect.y;
                    
                    // Note last inter MV for future use
                    LastInterMVect.x = InterMVect.x;
                    LastInterMVect.y = InterMVect.y;

                    AddMotionVector( cpi, &InterMVect);
                }
                else if ( BestError == MBGF_MVError )
                {
                    cpi->MBCodingMode = CODE_GOLDEN_MV;
                    SetMBMotionVectorsAndMode(cpi,YFragIndex,UFragIndex,VFragIndex,&GFMVect);
                    
                    // Note last inter GF MV for future use
                    LastGFMVect.x = GFMVect.x;
                    LastGFMVect.y = GFMVect.y;
                    
                    AddMotionVector( cpi, &GFMVect);
                }
                else if ( BestError == MBInterFOURMVError )
                {
                    cpi->MBCodingMode = CODE_INTER_FOURMV;

                    // Calculate the UV vectors as the average of the Y plane ones.
                    // First .x component
                    FourMVect[4].x = FourMVect[0].x + FourMVect[1].x + FourMVect[2].x + FourMVect[3].x;
                    if ( FourMVect[4].x >= 0 )
                        FourMVect[4].x = (FourMVect[4].x + 2) / 4;
                    else
                        FourMVect[4].x = (FourMVect[4].x - 2) / 4;
                    FourMVect[5].x = FourMVect[4].x;
                    
                    // Then .y component
                    FourMVect[4].y = FourMVect[0].y + FourMVect[1].y + FourMVect[2].y + FourMVect[3].y;
                    if ( FourMVect[4].y >= 0 )
                        FourMVect[4].y = (FourMVect[4].y + 2) / 4;
                    else
                        FourMVect[4].y = (FourMVect[4].y - 2) / 4;
                    FourMVect[5].y = FourMVect[4].y;

                    SetFragMotionVectorAndMode(cpi, YFragIndex, &FourMVect[0]);
                    SetFragMotionVectorAndMode(cpi, YFragIndex + 1, &FourMVect[1]);
                    SetFragMotionVectorAndMode(cpi, YFragIndex + cpi->pb.HFragments, &FourMVect[2]);
                    SetFragMotionVectorAndMode(cpi, YFragIndex + cpi->pb.HFragments + 1, &FourMVect[3]);
                    SetFragMotionVectorAndMode(cpi, UFragIndex, &FourMVect[4]);
                    SetFragMotionVectorAndMode(cpi, VFragIndex, &FourMVect[5]);

                    // Note the four MVs values for current macro-block.
                    AddMotionVector( cpi, &FourMVect[0]);
                    AddMotionVector( cpi, &FourMVect[1]);
                    AddMotionVector( cpi, &FourMVect[2]);
                    AddMotionVector( cpi, &FourMVect[3]);

                    // Update Prior last mv with last mv
                    PriorLastInterMVect.x = LastInterMVect.x;
                    PriorLastInterMVect.y = LastInterMVect.y;
                    
                    // Note last inter MV for future use
                    LastInterMVect.x = FourMVect[3].x;
                    LastInterMVect.y = FourMVect[3].y;
                    
                }
                else 
                {
                    cpi->MBCodingMode = CODE_INTRA;
                    SetMBMotionVectorsAndMode(cpi,YFragIndex,UFragIndex,VFragIndex,&ZeroVect);
                }


                // setting up mode specific block types
                //*******************************************************
                
                *InterError += (BestError>>8);
                *IntraError += (MBIntraError>>8);

                
            } // next macro block
            
            // Next Super-Block
			SB++;

        } // next super block column

    } // next super block row

    // system state should be cleared here....
	cpi->pb.ClearSysState();



	// Return number of pixels coded
	return 0;
}


/****************************************************************************
 * 
 *  ROUTINE       :     QuadCodeDisplayFragments
 *
 *  INPUTS        :     None.
 *
 *  OUTPUTS       :     None
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Codes the frame using the quad tree method.
 *
 *  SPECIAL NOTES :     None. 
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/
UINT32 QuadCodeDisplayFragments (CP_INSTANCE *cpi)
{
    INT32   i,j;

	UINT32	coded_pixels=0;
    UINT8   QIndex;

#define PUL 8
#define PU 4
#define PUR 2
#define PL 1

    int k,m,n;

    // predictor multiplier up-left, up, up-right,left, shift
	short pc[16][6]=
	{
		{0,0,0,0,0,0},	
		{0,0,0,1,0,0},		// PL
		{0,0,1,0,0,0},		// PUR
		{0,0,53,75,7,127},	// PUR|PL
		{0,1,0,0,0,0},		// PU
		{0,1,0,1,1,1},		// PU|PL
		{0,1,0,0,0,0},		// PU|PUR
		{0,0,53,75,7,127},	// PU|PUR|PL
		{1,0,0,0,0,0},		// PUL|
		{0,0,0,1,0,0},		// PUL|PL
		{1,0,1,0,1,1},		// PUL|PUR
		{0,0,53,75,7,127},	// PUL|PUR|PL
		{0,1,0,0,0,0},		// PUL|PU
		{-26,29,0,29,5,31}, // PUL|PU|PL
		{3,10,3,0,4,15},		// PUL|PU|PUR
		{-26,29,0,29,5,31}	// PUL|PU|PUR|PL
	};

	/* Search Points are ordered by distance from 0,0
		-4 -3 -2 -1  0  1  2  3  4 
	-4	         21 19 22         
	-3     24 15 11  9 12 16 25   
	-2     14  6  3  1  4  7 17   
	-1  20 10  2  z  z  z  5 13 23
	 0	18  8  0  z  z	z  z  z  z
	*/
	struct SearchPoints
	{
		int RowOffset;
		int ColOffset;
	} DCSearchPoints[]=
	{
		{0,-2},{-2,0},{-1,-2},{-2,-1},{-2,1},{-1,2},{-2,-2},{-2,2},{0,-3},
		{-3,0},{-1,-3},{-3,-1},{-3,1},{-1,3},{-2,-3},{-3,-2},{-3,2},{-2,3},
		{0,-4},{-4,0},{-1,-4},{-4,-1},{-4,1},{-1,4},{-3,-3},{-3,3}
	};
	//int DCSearchPointCount = sizeof(DCSearchPoints) / ( 2 * sizeof(int));
	int DCSearchPointCount = 0;

	// fragment left fragment up-left, fragment up, fragment up-right
	int fl,ful,fu,fur;

	// value left value up-left, value up, value up-right
	int vl,vul,vu,vur;

	// fragment number left, up-left, up, up-right
	int l,ul,u,ur;

	//which predictor constants to use
	short wpc;

	// last used inter predictor (Raster Order)
	short Last[3];	// last value used for given frame
	short TempInter = 0;

	int FragsAcross=cpi->pb.HFragments;	
	int FragsDown = cpi->pb.VFragments;
	int FromFragment,ToFragment;
	INT32	FragIndex;			    // Fragment number
	int WhichFrame;
	int WhichCase;

	short Mode2Frame[] =
	{
		1,	// CODE_INTER_NO_MV		0 => Encoded diff from same MB last frame 
		0,	// CODE_INTRA			1 => DCT Encoded Block
		1,	// CODE_INTER_PLUS_MV	2 => Encoded diff from included MV MB last frame
		1,	// CODE_INTER_LAST_MV	3 => Encoded diff from MRU MV MB last frame
		1,	// CODE_INTER_PRIOR_MV	4 => Encoded diff from included 4 separate MV blocks
		2,	// CODE_USING_GOLDEN	5 => Encoded diff from same MB golden frame
		2,	// CODE_GOLDEN_MV		6 => Encoded diff from included MV MB golden frame
		1	// CODE_INTER_FOUR_MV	7 => Encoded diff from included 4 separate MV blocks
	};

	short PredictedDC;

#define HIGHBITDUPPED(X) (((signed short) X)  >> 15)


    // Initialise the coded block indices variables. These allow
    // subsequent linear access to the quad tree ordered list of 
    // coded blocks
    cpi->pb.CodedBlockIndex = 0;

    // Set the inter/intra descision control variables.
    QIndex = Q_TABLE_SIZE - 1;
    while ( (INT32) QIndex >= 0 )
    {
        if ( (QIndex == 0) || ( cpi->pb.QThreshTable[QIndex] >= cpi->pb.ThisFrameQualityValue) )
            break;
        QIndex --;
    }


    // Encode and tokenise the Y, U and V components
	coded_pixels = QuadCodeComponent ( cpi, 0, cpi->pb.YSBRows, cpi->pb.YSBCols, cpi->pb.HFragments%4, cpi->pb.VFragments%4, cpi->pb.Configuration.VideoFrameWidth );
	coded_pixels += QuadCodeComponent ( cpi, cpi->pb.YSuperBlocks, cpi->pb.UVSBRows, cpi->pb.UVSBCols, (cpi->pb.HFragments/2)%4, (cpi->pb.VFragments/2)%4, cpi->pb.Configuration.VideoFrameWidth>>1 );
	coded_pixels += QuadCodeComponent ( cpi, cpi->pb.YSuperBlocks+cpi->pb.UVSuperBlocks, cpi->pb.UVSBRows, cpi->pb.UVSBCols, (cpi->pb.HFragments/2)%4, (cpi->pb.VFragments/2)%4, cpi->pb.Configuration.VideoFrameWidth>>1 );
    
	// for y,u,v
	for ( j = 0; j < 3 ; j++)
	{
		// pick which fragments based on Y, U, V
		switch(j)
		{
		case 0: // y
			FromFragment = 0;
			ToFragment = cpi->pb.YPlaneFragments;
			FragsAcross = cpi->pb.HFragments;
			FragsDown = cpi->pb.VFragments;
			break;
		case 1: // u
			FromFragment = cpi->pb.YPlaneFragments;
			ToFragment = cpi->pb.YPlaneFragments + cpi->pb.UVPlaneFragments ;
			FragsAcross = cpi->pb.HFragments >> 1;
			FragsDown = cpi->pb.VFragments >> 1;
			break;
		case 2:	// v
			FromFragment = cpi->pb.YPlaneFragments + cpi->pb.UVPlaneFragments;
			ToFragment = cpi->pb.YPlaneFragments + (2 * cpi->pb.UVPlaneFragments) ;
			FragsAcross = cpi->pb.HFragments >> 1;
			FragsDown = cpi->pb.VFragments >> 1;
			break;
		}

		// initialize our array of last used DC Components
		for(k=0;k<3;k++)
			Last[k]=0;

		i=FromFragment;

		// do prediction on all of Y, U or V
		for ( m = 0 ; m < FragsDown ; m++)
		{
			for ( n = 0 ; n < FragsAcross ; n++, i++)
			{
				cpi->OriginalDC[i] = cpi->pb.QFragData[i][0];
                //fprintf(f,"%d ",cpi->OriginalDC[i]);

				// only do 2 prediction if fragment coded and on non intra or if all fragments are intra 
				if( cpi->pb.display_fragments[i] || (GetFrameType(&cpi->pb) == BASE_FRAME) )
				{
					// Type of Fragment
					WhichFrame = Mode2Frame[cpi->pb.FragCodingMethod[i]];

					// Check Borderline Cases
					WhichCase = (n==0) + ((m==0) << 1) + ((n+1 == FragsAcross) << 2);

					switch(WhichCase)
					{
					case 0: // normal case no border condition

						// calculate values left, up, up-right and up-left
						l = i-1;
						u = i - FragsAcross;
						ur = i - FragsAcross + 1;
						ul = i - FragsAcross - 1;

						// calculate values
						vl = cpi->OriginalDC[l];
						vu = cpi->OriginalDC[u];
						vur = cpi->OriginalDC[ur];
						vul = cpi->OriginalDC[ul];
						
						// fragment valid for prediction use if coded and it comes from same frame as the one we are predicting
						fl = cpi->pb.display_fragments[l] && (Mode2Frame[cpi->pb.FragCodingMethod[l]] == WhichFrame);
						fu = cpi->pb.display_fragments[u] && (Mode2Frame[cpi->pb.FragCodingMethod[u]] == WhichFrame);
						fur = cpi->pb.display_fragments[ur] && (Mode2Frame[cpi->pb.FragCodingMethod[ur]] == WhichFrame);
						ful = cpi->pb.display_fragments[ul] && (Mode2Frame[cpi->pb.FragCodingMethod[ul]] == WhichFrame);

						// calculate which predictor to use 
						wpc = (fl*PL) | (fu*PU) | (ful*PUL) | (fur*PUR);

						break;

					case 1: // n == 0 Left Column

						// calculate values left, up, up-right and up-left
						u = i - FragsAcross;
						ur = i - FragsAcross + 1;

						// calculate values
						vu = cpi->OriginalDC[u];
						vur = cpi->OriginalDC[ur];

						// fragment valid for prediction if coded and it comes from same frame as the one we are predicting
						fu = cpi->pb.display_fragments[u] && (Mode2Frame[cpi->pb.FragCodingMethod[u]] == WhichFrame);
						fur = cpi->pb.display_fragments[ur] && (Mode2Frame[cpi->pb.FragCodingMethod[ur]] == WhichFrame);

						// calculate which predictor to use 
						wpc = (fu*PU) | (fur*PUR);

						break;

					case 2: // m == 0 Top Row 
					case 6: // m == 0 and n+1 == FragsAcross or Top Row Right Column

						// calculate values left, up, up-right and up-left
						l = i-1;

						// calculate values
						vl = cpi->OriginalDC[l];

						// fragment valid for prediction if coded and it comes from same frame as the one we are predicting
						fl = cpi->pb.display_fragments[l] && (Mode2Frame[cpi->pb.FragCodingMethod[l]] == WhichFrame);

						// calculate which predictor to use 
						wpc = (fl*PL) ;

						break;

					case 3: // n == 0 & m == 0 Top Row Left Column

						wpc = 0;

						break;

					case 4: // n+1 == FragsAcross : Right Column

						// calculate values left, up, up-right and up-left
						l = i-1;
						u = i - FragsAcross;
						ul = i - FragsAcross - 1;

						// calculate values
						vl = cpi->OriginalDC[l];
						vu = cpi->OriginalDC[u];
						vul = cpi->OriginalDC[ul];
						
						// fragment valid for prediction if coded and it comes from same frame as the one we are predicting
						fl = cpi->pb.display_fragments[l] && (Mode2Frame[cpi->pb.FragCodingMethod[l]] == WhichFrame);
						fu = cpi->pb.display_fragments[u] && (Mode2Frame[cpi->pb.FragCodingMethod[u]] == WhichFrame);
						ful = cpi->pb.display_fragments[ul] && (Mode2Frame[cpi->pb.FragCodingMethod[ul]] == WhichFrame);

						// calculate which predictor to use 
						wpc = (fl*PL) | (fu*PU) | (ful*PUL) ;

						break;

					}
					
					
					if(wpc==0)
					{
						FragIndex = 1;
						
						// find the nearest one that is coded 
						for( k = 0; k < DCSearchPointCount ; k++)
						{
							FragIndex = i + DCSearchPoints[k].RowOffset * FragsAcross + DCSearchPoints[k].ColOffset;
							
							if( FragIndex - FromFragment > 0 ) 
							{
								if(cpi->pb.display_fragments[FragIndex] && (Mode2Frame[cpi->pb.FragCodingMethod[FragIndex]] == WhichFrame))
								{
									cpi->pb.QFragData[i][0] -= cpi->OriginalDC[FragIndex];
									FragIndex = 0;
									break;
								}
							}
						}
						
						
						// if none matched fall back to the last one ever
						if(FragIndex)
						{
							cpi->pb.QFragData[i][0] -= Last[WhichFrame];
						}
						
					}
					else
					{
						
						// don't do divide if divisor is 1 or 0
						PredictedDC = (pc[wpc][0]*vul + pc[wpc][1] * vu + pc[wpc][2] * vur + pc[wpc][3] * vl );

						// if we need to do a shift
						if(pc[wpc][4] != 0 )
						{
							
							// If negative add in the negative correction factor
							PredictedDC += (HIGHBITDUPPED(PredictedDC) & pc[wpc][5]);
							
							// Shift in lieu of a divide
							PredictedDC >>= pc[wpc][4];
						}
						
                        // check for outranging on the two predictors that can outrange 
                        switch(wpc)
                        {
                        case 13: // pul pu pl
                        case 15: // pul pu pur pl
                            if( abs(PredictedDC - vu) > 128)
                                PredictedDC = vu;
                            else if( abs(PredictedDC - vl) > 128)
                                PredictedDC = vl;
                            else if( abs(PredictedDC - vul) > 128)
                                PredictedDC = vul;
                            break;
                        }

						
						cpi->pb.QFragData[i][0] -= PredictedDC;
					}
					
					// Save the last fragment coded for whatever frame we are predicting from
					Last[WhichFrame] = cpi->OriginalDC[i];

				} // if display fragments

			} // for n = 0 to columns across

		} // for m = 0 to rows down

	} // for j = 0 to 2 (y,u,v)

    // Pack DC tokens and adjust the ones we couldn't predict 2d
	for ( i = 0; i < cpi->pb.CodedBlockIndex; i++ )
	{
//		int NextFragIndex;
		// Get the linear index for the current coded fragment.
		FragIndex = cpi->pb.CodedBlockList[i];
//		NextFragIndex = cpi->pb.CodedBlockList[i+1];

//		XmmFetch(cpi->pb.QFragData[FragIndex], cpi->pb.QFragData[NextFragIndex]);

		coded_pixels += DPCMTokenizeBlock ( cpi, FragIndex, cpi->pb.Configuration.VideoFrameWidth  );

	}


    // Bit pack the video data data
    PackCodedVideo(cpi);

    // End the bit packing run. 
    EndAddBitsToBuffer(cpi);

    // Reconstruct the reference frames
    ReconRefFrames(&cpi->pb);

    UpdateFragQIndex(&cpi->pb);
    
    // Measure the inter reconstruction error for all the blocks that were coded
    // for use as part of the recovery monitoring process in subsequent frames.
    for ( i = 0; i < cpi->pb.CodedBlockIndex; i++ )
    {
        cpi->LastCodedErrorScore[ cpi->pb.CodedBlockList[i] ] = cpi->GetBlockReconError( cpi, cpi->pb.CodedBlockList[i] );
    
	}
	cpi->pb.ClearSysState();

    // Return total number of coded pixels
	return coded_pixels;
}
