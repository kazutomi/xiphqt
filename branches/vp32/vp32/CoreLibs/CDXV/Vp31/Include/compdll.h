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
*   Module Title :     COMPDLL.H
*
*   Description  :     Video CODEC demo compression DLL main header
*
*
*****************************************************************************
*/

#ifndef __INC_COMPDLL_H
#define __INC_COMPDLL_H

#include "codec_common.h"
#include "PreProcIf.h"
#include "pbdll.h"


/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

#define INTRA_THRESH_Q	1
#define VERY_BEST_Q     10
#define STILL_Q         65

#define NUM_RESIDUE_UPDATES     2

#define MIN_BPB_FACTOR          0.3
#define MAX_BPB_FACTOR          3.0

#define RESIDUE_BLOCK_FACTOR    4
#define KEY_FRAME_CONTEXT		5

/****************************************************************************
*  Types
*****************************************************************************
*/

typedef struct CONFIG_TYPE2
{
    UINT32 TargetBandwidth;
    UINT32 OutputFrameRate;
    
    UINT32 FirstFrameQ;
    UINT32 BaseQ;
    UINT32 MaxQ;            // Absolute Max Q allowed.
    UINT32 ActiveMaxQ;      // Currently active Max Q

} CONFIG_TYPE2;


/* Defines the largest positive integer expressable with a standard int type */
/****************************************************************************
* *     Type declarations
****************************************************************************
*/   
   

/****************************************************************************
*  MACROS
*****************************************************************************
*/

/****************************************************************************
*  Global Variables
*****************************************************************************
*/
#define MAX_MV_EXTENT           31      //  Max search distance in half pixel increments
#define HUGE_ERROR              (1<<28)  //  Out of range test value

#define MAX_SEARCH_SITES		33		//	Number of search sites for a 4-step search (at pixel accuracy)

typedef struct CP_INSTANCE * xCP_INST;
typedef struct CP_INSTANCE
{

	//****************************************************************************************************
	// Compressor Configuration		
	SCAN_CONFIG_DATA ScanConfig;
	CONFIG_TYPE2 Configuration;
	BOOL   QuickCompress;
	BOOL   GoldenFrameEnabled;
	BOOL   InterPrediction;
	BOOL   MotionCompensation;
	BOOL   AutoKeyFrameEnabled ;
	INT32  ForceKeyFrameEvery ;
	INT32  AutoKeyFrameThreshold ;
	UINT32 LastKeyFrame ;
	UINT32 MinimumDistanceToKeyFrame ;
	UINT32 KeyFrameDataTarget ;        // Data rate target for key frames
	UINT32 KeyFrameFrequency ;
	BOOL   DropFramesAllowed ; 
	INT32  DropCount ;
	INT32  MaxConsDroppedFrames ;
    INT32  DropFrameTriggerBytes;
    BOOL   DropFrameCandidate;
	UINT32 QualitySetting;
    UINT32 PreProcFilterLevel;
	BOOL   NoDrops;

	// Compressor Statistics
	double TotErrScore;
	INT64  KeyFrameCount ;                          // Count of key frames.
	INT64  TotKeyFrameBytes ;
	UINT32 LastKeyFrameSize ;
	UINT32 PriorKeyFrameSize[KEY_FRAME_CONTEXT];
	UINT32 PriorKeyFrameDistance[KEY_FRAME_CONTEXT];
	INT32  FrameQuality[6];
	int    DecoderErrorCode;		// Decoder error flag.
	INT32  ThreshMapThreshold;
	INT32  TotalMotionScore;
	INT64  TotalByteCount;
    INT32  FixedQ;
	
	// Frame Statistics 
	INT8   InterCodeCount;
	INT64  CurrentFrame;                                
	INT64  CarryOver ;
	UINT32 LastFrameSize;
	UINT32 ThisFrameSize;
	UINT32 BufferedOutputBytes;
	UINT32 FrameBitCount;
	BOOL   ThisIsFirstFrame;
	BOOL   ThisIsKeyFrame;
	

	INT32  MotionScore;   
	UINT32 RegulationBlocks;
	INT32  RecoveryMotionScore;   
	BOOL   RecoveryBlocksAdded ;
	double ProportionRecBlocks;
	double MaxRecFactor ;

	/* Rate Targeting variables. */
	UINT32 ThisFrameTargetBytes;
	double BpbCorrectionFactor;

	// Up regulation variables
	UINT32 FinalPassLastPos;  // Used to regulate a final unrestricted high quality pass. 
	UINT32 LastEndSB;	       // Where we were in the loop last time. 
	UINT32 ResidueLastEndSB;  // Where we were in the residue update loop last time.         
	
	// Controlling Block Selection
	UINT32 MVChangeFactor;     
	UINT32 FourMvChangeFactor;           
	UINT32 MinImprovementForNewMV;   
	UINT32 ExhaustiveSearchThresh;
	UINT32 MinImprovementForFourMV;   
	UINT32 FourMVThreshold;
	
	/* Module shared data structures. */            
	INT32  frame_target_rate;
    INT32  BaseLineFrameTargetRate;
	INT32  min_blocks_per_frame;
	UINT32 tot_bytes_old;
	
	//****************************************************************************************************
	

	//****************************************************************************************************
	// Frames
	// Used in the selecetive convolution filtering of the Y plane. */
	UINT8 *ConvDestBuffer;
	YUV_BUFFER_ENTRY *yuv0ptr;
	YUV_BUFFER_ENTRY *yuv1ptr;
	UINT8 *ConvDestBufferAlloc;
	YUV_BUFFER_ENTRY *yuv0ptrAlloc;
	YUV_BUFFER_ENTRY *yuv1ptrAlloc;
	//****************************************************************************************************

	//****************************************************************************************************
	// Token Buffers
	UINT32 *OptimisedTokenListEb;    // Optimised token list extra bits
	UINT8  *OptimisedTokenList;      // Optimised token list.
	UINT8  *OptimisedTokenListHi;    // Optimised token list huffman table index

	UINT32 *OptimisedTokenListEbAlloc;    // Optimised token list extra bits
	UINT8  *OptimisedTokenListAlloc;      // Optimised token list.
	UINT8  *OptimisedTokenListHiAlloc;    // Optimised token list huffman table index
	UINT8  *OptimisedTokenListPlAlloc;    // Optimised token list huffman table index
	
    UINT8  *OptimisedTokenListPl;    // Plane to which the token belongs Y = 0 or UV = 1
	INT32  OptimisedTokenCount;		 // Count of Optimized tokens
	UINT32 RunHuffIndex;             // Huffman table in force at the start of a run
	UINT32 RunPlaneIndex;            // The plane (Y=0 UV=1) to which the first token in an EOB run belonged.
	

	UINT32 TotTokenCount;
	INT32  TokensToBeCoded;
	INT32  TokensCoded;
	//****************************************************************************************************
	
	//****************************************************************************************************
	// SuperBlock, MacroBLock and Fragment Information
	// Coded flag arrays and counters for them
	UINT8  *PartiallyCodedFlags;
	UINT8  *PartiallyCodedMbPatterns;
	UINT8  *UncodedMbFlags;

	UINT8  *extra_fragments;   // extra updates not recommended by pre-processor
    INT16  *OriginalDC;

	UINT32 *FragmentLastQ;     // Array used to keep track of quality at which each fragment was last updated.
	UINT8  *FragTokens;
	UINT32 *FragTokenCounts;   // Number of tokens per fragment

	UINT32 *RunHuffIndices;
	UINT32 *LastCodedErrorScore; 
	UINT32 *ModeList;
	MOTION_VECTOR *MVList;
    	
	UINT8  *extra_fragmentsAlloc;   // extra updates not recommended by pre-processor
	UINT32 *FragmentLastQAlloc;     // Array used to keep track of quality at which each fragment was last updated.
	UINT8  *FragTokensAlloc;
	UINT32 *FragTokenCountsAlloc;   // Number of tokens per fragment
    INT16  *OriginalDCAlloc;

	UINT8  *BlockCodedFlags;
	UINT8  *BlockCodedFlagsAlloc;

	UINT32 *RunHuffIndicesAlloc;
	UINT32 *LastCodedErrorScoreAlloc; 
	UINT32 *ModeListAlloc;
	MOTION_VECTOR *MVListAlloc;
	
	UINT32 MvListCount;
	UINT32 ModeListCount;


	UINT8  *DataOutputBuffer;
	//****************************************************************************************************
	
	//****************************************************************************************
	// STATICS COPIED FROM C FILES (USED IN MULTIPLE FUNCTIONS BUT ARE NOT REALLY INSTANCE GLOBALS )
	// copied from cencode.c
	UINT32 RunLength;
	UINT32 MaxBitTarget;               // Cut off target for rate capping
	double BitRateCapFactor;    // Factor relating normal frame target to cut off target.
	
	UINT8  MBCodingMode;	    // Coding mode flags
	
	// copied from mcomp.c
	INT32  MVPixelOffsetY[MAX_SEARCH_SITES];
	UINT32  InterTripOutThresh;
	UINT8  MVEnabled;
	UINT32 MotionVectorSearchCount;
	UINT32 FrameMVSearcOunt;
	INT32  MVSearchSteps;
	INT32  MVOffsetX[MAX_SEARCH_SITES];
	INT32  MVOffsetY[MAX_SEARCH_SITES];
	INT32  HalfPixelRef2Offset[9];    // Offsets for half pixel compensation
	INT8   HalfPixelXOffset[9];       // Half pixel MV offsets for X
	INT8   HalfPixelYOffset[9];       // Half pixel MV offsets for Y

	//copied from cfrarray.c
	UINT32 bit_pattern ;
	UINT8  bits_so_far ; 
	UINT32 lastval ;
	UINT32 lastrun ;

	// copied from dct_encode c
	Q_LIST_ENTRY    *quantized_list;  
	Q_LIST_ENTRY	*quantized_listAlloc;

	MOTION_VECTOR   MVector;
	UINT32 TempBitCount;
	INT16  *DCT_codes;			//Buffer that stores the result of Forward DCT
	INT16  *DCTDataBuffer;		//Input data buffer for Forward DCT
	INT16  *DCT_codesAlloc;
	INT16  *DCTDataBufferAlloc;	

	
	// Motion compensation related variables
	UINT32  MvMaxExtent;

	// copied from cbitman.c
	INT32 byte_bit_offset;     
	UINT32 DataBlock;  
	UINT32 mybits; 
	UINT32 ByteBitsLeft;

    double QTargetModifier[Q_TABLE_SIZE];

	//****************************************************************
	// Function Pointers some probably could be library globals!
	UINT32 (*GetSAD)(UINT8 *, UINT8 *, UINT32, UINT32, UINT32) ;            
	UINT32 (*GetNextSAD)(UINT8 *, UINT8 *, UINT32, UINT32, UINT32 );            
	UINT32 (*GetSadHalfPixel)(UINT8 *, UINT8 *, UINT8 *, UINT32, UINT32, UINT32  );
	UINT32 (*GetInterError)( UINT8 *, UINT8 *,  UINT8 *, UINT32 );
	UINT32 (*GetIntraError)( UINT8 *, UINT32);
	UINT32 (* GetBlockReconError ) ( xCP_INST cpi,INT32 );
	void (*fdct_short) ( INT16 * InputData, INT16 * OutputData );
//	void (*TranxQuantBlock)  ( xCP_INST cpi, INT32 FragIndex, UINT32 PixelsPerLine );
	UINT8 (*TokenizeDctBlock)( INT16 *, UINT32 * );
    void (*Sub8)( UINT8 *FiltPtr, UINT8 *ReconPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine );
    void (*Sub8_128)( UINT8 *FiltPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine );
    void (*Sub8Av2)( UINT8 *FiltPtr, UINT8 *ReconPtr1, UINT8 *ReconPtr2, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
              UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine );

	//**************************************************************** 
    char FirstPassFileName[512];

	// instances (used for reconstructing buffers and to hold tokens etc.)
	xPP_INST pp;	// preprocessor
	PB_INSTANCE pb;	// playback

	
} CP_INSTANCE;

// Gives the initial bits per block estimate for each Q value
extern double KfBpbTable[Q_TABLE_SIZE];
extern double BpbTable[Q_TABLE_SIZE];

/****************************************************************************
*  Functions.
*****************************************************************************
*/  


extern void UpdateFrame(CP_INSTANCE *cpi);

extern UINT32 QuadCodeDisplayFragments (CP_INSTANCE *cpi);

extern UINT32 QuadCodeComponent ( CP_INSTANCE *cpi, UINT32 FirstSB, UINT32 SBRows, UINT32 SBCols, UINT32 HExtra, UINT32 VExtra, UINT32 PixelsPerLine );

extern UINT32 EncodeData(CP_INSTANCE *cpi);

// Codec
extern void TransformQuantizeBlock  ( CP_INSTANCE *cpi, INT32 FragIndex, UINT32 PixelsPerLine );
extern UINT32 DPCMTokenizeBlock  ( CP_INSTANCE *cpi, INT32 FragIndex, UINT32 PixelsPerLine );
extern void SUB8( UINT8 *FiltPtr, UINT8 *ReconPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine );
extern void SUB8_128( UINT8 *FiltPtr, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
               UINT32 PixelsPerLine );
extern void SUB8AV2( UINT8 *FiltPtr, UINT8 *ReconPtr1, UINT8 *ReconPtr2, INT16 *DctInputPtr, UINT8 *old_ptr1, UINT8 *new_ptr1, 
              UINT32 PixelsPerLine, UINT32 ReconPixelsPerLine );

extern void  PackEOBRun(CP_INSTANCE *cpi);
extern void quantize( PB_INSTANCE *pbi, INT16 * DCT_block, Q_LIST_ENTRY * quantized_list);
extern void ConvertBmpToYUV( PB_INSTANCE *pbi, UINT8 * BmpDataPtr, UINT8 * YuvBufferPtr );
extern CP_INSTANCE * CreateCPInstance(void);
extern void DeleteCPInstance(CP_INSTANCE **cpi);
extern UINT32 GetSumAbsDiffs( UINT8 * NewDataPtr, UINT8 * RefDataPtr, 
							  UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar );
extern UINT32 GetNextSumAbsDiffs( UINT8 * NewDataPtr, UINT8 * RefDataPtr, 
	  				          UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar );

extern UINT32 GetHalfPixelSumAbsDiffs( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
    						  UINT32 PixelsPerLine, UINT32 ErrorSoFar, UINT32 BestSoFar );
extern UINT32 GetInterErr( UINT8 * NewDataPtr, UINT8 * RefDataPtr1,  UINT8 * RefDataPtr2, UINT32 PixelsPerLine );
extern UINT32 GetBlockReconErrorSlow ( CP_INSTANCE *cpi, INT32 BlockIndex );
extern void CMachineSpecificConfig(CP_INSTANCE *cpi);
// extern void fdct_slow16 ( INT16 * InputData, INT16 * OutputData );
extern void fdct_slowf ( INT16 * InputData, INT16 * OutputData );
extern void fdct_short ( INT16 * InputData, INT16 * OutputData );
extern BOOL EAllocateFragmentInfo(CP_INSTANCE *cpi);
extern BOOL EAllocateFrameInfo(CP_INSTANCE *cpi);
extern void EDeleteFragmentInfo(CP_INSTANCE *cpi);
extern void EDeleteFrameInfo(CP_INSTANCE *cpi);
extern void UpdateQC( PB_INSTANCE *pbi, UINT32 NewQ );
extern UINT32 PickIntra( CP_INSTANCE *cpi, UINT32 SBRows, UINT32 SBCols, UINT32 HExtra, UINT32 VExtra, UINT32 PixelsPerLine);
extern UINT32 PickModes( CP_INSTANCE *cpi, UINT32 SBRows, UINT32 SBCols, UINT32 HExtra, UINT32 VExtra, UINT32 PixelsPerLine, 
                 UINT32 *InterError, UINT32 *IntraError);

extern INT32  GetSpeckSumAbsDiffs( UINT8 * NewDataPtr, UINT8 * RefDataPtr, 
							  UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar );
extern INT32  GetNextSpeckSumAbsDiffs( UINT8 * NewDataPtr, UINT8 * RefDataPtr, 
	  				          UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar );

extern INT32  GetHalfPixelSpeckSumAbsDiffs( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
    						  UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar );

#endif
