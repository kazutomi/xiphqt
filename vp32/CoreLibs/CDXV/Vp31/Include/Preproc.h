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
*   Module Title :     preproc.h
*
*   Description  :     Content analysis module header
*
*****************************************************************************
*/						

#include "PreprocConf.h"
#include "type_aliases.h"
#include "PreProcIf.h"

/* Constants. */
#define INTERNAL_BLOCK_HEIGHT   8
#define INTERNAL_BLOCK_WIDTH	8


/* NEW Line search values. */ 
#define UP      0
#define DOWN    1
#define LEFT    2
#define RIGHT   3

#define FIRST_ROW           0
#define NOT_EDGE_ROW        1
#define LAST_ROW            2      

#define YDIFF_CB_ROWS			(INTERNAL_BLOCK_HEIGHT * 3)
#define CHLOCALS_CB_ROWS		(INTERNAL_BLOCK_HEIGHT * 3)
#define PMAP_CB_ROWS			(INTERNAL_BLOCK_HEIGHT * 3)
#define PSCORE_CB_ROWS			(INTERNAL_BLOCK_HEIGHT * 4)

// Status values in block coding map
#define CANDIDATE_BLOCK_LOW			-2
#define CANDIDATE_BLOCK				-1
#define BLOCK_NOT_CODED				0
#define BLOCK_CODED_BAR 			3	
#define BLOCK_CODED_SGC				4	
#define BLOCK_CODED_LOW				4	
#define BLOCK_CODED 				5	

#define MAX_PREV_FRAMES             16
#define MAX_SEARCH_LINE_LEN			7   

/******************************************************************/
/* Type definitions. */
/******************************************************************/

typedef INT32 YUV_ENTRY;
typedef UINT8 YUV_BUFFER_ENTRY;
typedef UINT8 * YUV_BUFFER_ENTRY_PTR;

typedef struct PP_INSTANCE * xPP_INST;
typedef struct PP_INSTANCE
{
     UINT32 PrevFrameLimit;
     UINT32 *ScanPixelIndexTableAlloc;		
     INT8   *ScanDisplayFragmentsAlloc;

     INT8   *PrevFragmentsAlloc[MAX_PREV_FRAMES];

     UINT32 *FragScoresAlloc;               // The individual frame difference ratings.    
     INT8   *SameGreyDirPixelsAlloc;
     INT8   *BarBlockMapAlloc;

     // Number of pixels changed by diff threshold in row of a fragment. 
     UINT8  *FragDiffPixelsAlloc;  

     UINT8  *PixelScoresAlloc;  
     UINT8  *PixelChangedMapAlloc;
     UINT8  *ChLocalsAlloc;
     INT16  *yuv_differencesAlloc;  
     INT32  *RowChangedPixelsAlloc;
	 INT8   *TmpCodedMapAlloc;

     UINT32 *ScanPixelIndexTable;		
     INT8   *ScanDisplayFragments;

     INT8   *PrevFragments[MAX_PREV_FRAMES];

     UINT32 *FragScores;               // The individual frame difference ratings.    
     INT8   *SameGreyDirPixels;
     INT8   *BarBlockMap;

     // Number of pixels changed by diff threshold in row of a fragment. 
     UINT8  *FragDiffPixels;  

     UINT8  *PixelScores;  
     UINT8  *PixelChangedMap;
     UINT8  *ChLocals;
     INT16  *yuv_differences;  
     INT32  *RowChangedPixels;
	 INT8   *TmpCodedMap;

     // Plane pointers and dimension variables
     UINT8 * YPlanePtr0;
     UINT8 * YPlanePtr1;
     UINT8 * UPlanePtr0;
     UINT8 * UPlanePtr1;
     UINT8 * VPlanePtr0;
     UINT8 * VPlanePtr1;

     UINT32  VideoYPlaneWidth;
     UINT32  VideoYPlaneHeight;
     UINT32  VideoUVPlaneWidth;
     UINT32  VideoUVPlaneHeight;

     UINT32  VideoYPlaneStride;
     UINT32  VideoUPlaneStride;
     UINT32  VideoVPlaneStride;

	 /* Scan control variables. */
     UINT8   HFragPixels;
     UINT8   VFragPixels;

     UINT32  ScanFrameFragments;
     UINT32  ScanYPlaneFragments;
     UINT32  ScanUVPlaneFragments;
     UINT32  ScanHFragments;
     UINT32  ScanVFragments;

     UINT32  YFramePixels; 
     UINT32  UVFramePixels; 

     UINT32  SgcThresh;

     UINT32  OutputBlocksUpdated;
	 UINT32  KFIndicator;

	 /* The pre-processor scan configuration. */
     SCAN_CONFIG_DATA ScanConfig;

     INT32 SRFGreyThresh;
     INT32 SRFColThresh;
     INT32 SgcLevelThresh;
     INT32 SuvcLevelThresh;

     UINT32 NoiseSupLevel;

	 /* Block Thresholds. */
     UINT32 PrimaryBlockThreshold;

     BOOL   PAKEnabled;

     int    LevelThresh; 
     int    NegLevelThresh; 
     int    SrfThresh;
     int    NegSrfThresh;
     int    HighChange;
     int    NegHighChange;     

     // Threshold lookup tables
	 UINT8 SrfPakThreshTable[512];
	 UINT8 * SrfPakThreshTablePtr;
	 UINT8 SrfThreshTable[512];
	 UINT8 * SrfThreshTablePtr;
	 UINT8 SgcThreshTable[512];
	 UINT8 * SgcThreshTablePtr;

     // Variables controlling S.A.D. break outs.
     UINT32 GrpLowSadThresh;
     UINT32 GrpHighSadThresh;
     UINT32 ModifiedGrpLowSadThresh;
     UINT32 ModifiedGrpHighSadThresh;

     INT32  PlaneHFragments;
     INT32  PlaneVFragments;
     INT32  PlaneHeight;
     INT32  PlaneWidth;
     INT32  PlaneStride;

     UINT32 BlockThreshold;
     UINT32 BlockSgcThresh;
     double UVBlockThreshCorrection;
     double UVSgcCorrection;


	// PC specific variables
	BOOL  MmxEnabled;
	BOOL  XmmEnabled;
	
	double YUVPlaneCorrectionFactor;	
	double AbsDiff_ScoreMultiplierTable[256];
	UINT8  NoiseScoreBoostTable[256];
	UINT8  MaxLineSearchLen;

	INT32 YuvDiffsCircularBufferSize;
	INT32 ChLocalsCircularBufferSize;
	INT32 PixelMapCircularBufferSize;

	// Function pointers for mmx switches
	UINT32 (*RowSAD)(UINT8 *, UINT8 * );            
	UINT32 (*ColSAD)(xPP_INST ppi, UINT8 *, UINT8 * );            

} PP_INSTANCE;

/******************************************************************/
/* Function prototypes. */
/******************************************************************/


extern __inline UINT32 ScanGetFragIndex( PP_INSTANCE *ppi, UINT32 FragmentNo );

extern void InitScanMapArrays(PP_INSTANCE *ppi);

extern void AnalysePlane( PP_INSTANCE *ppi, UINT8 * PlanePtr0, UINT8 * PlanePtr1, UINT32 FragArrayOffset, UINT32 PWidth, UINT32 PHeight, UINT32 PStride );

extern void ScanCalcPixelIndexTable(PP_INSTANCE *ppi);

extern void CreateOutputDisplayMap( PP_INSTANCE *ppi, 
								    INT8  * InternalFragmentsPtr, 
                                    INT8  * RecentHistoryPtr,
                                    UINT8 * ExternalFragmentsPtr );

extern void SetFromPrevious(PP_INSTANCE *ppi);
extern void UpdatePreviousBlockLists(PP_INSTANCE *ppi);
extern void ConfigurePP( PP_INSTANCE *ppi, INT32 LevelOffset );

//  Analysis functions
extern void RowBarEnhBlockMap( PP_INSTANCE *ppi, 
							   UINT32 * FragScorePtr, 
						       INT8   * FragSgcPtr,
						       INT8   * UpdatedBlockMapPtr,
						       INT8   * BarBlockMapPtr,
						       UINT32 RowNumber );

extern void BarCopyBack( PP_INSTANCE *ppi, 
						 INT8  * UpdatedBlockMapPtr,
						 INT8  * BarBlockMapPtr );

// Secondary filter functions
extern UINT8 ApplyLowPass( PP_INSTANCE *ppi, UINT8 * SrcPtr, UINT32 PlaneLineLength, INT32 Level );

// PC specific functions
extern void MachineSpecificConfig();
extern void ClearMmx( PP_INSTANCE *ppi);

extern UINT32 ScalarRowSAD( UINT8 * Src1, UINT8 * Src2 );
extern UINT32 ScalarColSAD( PP_INSTANCE *ppi, UINT8 * Src1, UINT8 * Src2 );

extern PP_INSTANCE * CreatePPInstance(void);
extern void DeletePPInstance(PP_INSTANCE **ppi);




