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
*   Module Title :     MComp.h
*
*   Description  :     Video CODEC: motion compensation module header .
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/


/****************************************************************************
*  Header Frames
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */

#ifndef MCOMP_H
#define MCOMP_H

#include "type_aliases.h"
#include "codec_common.h"
#include "compdll.h"
/****************************************************************************
*  Constants
*****************************************************************************
*/

// Compressor specific


/****************************************************************************
*  Types
*****************************************************************************
*/        

/****************************************************************************
*   Data structures
*****************************************************************************
*/
/*
// Function pointers for machine / platform specific functions
extern INT32 (*GetSAD)(UINT8 *, UINT8 *, UINT32, INT32, INT32 );            
extern INT32 (*GetNextSAD)(UINT8 *, UINT8 *, UINT32, INT32, INT32 );            
extern INT32 (*GetSadHalfPixel)(UINT8 *, UINT8 *, UINT8 *, UINT32, INT32, INT32  );
extern INT32 (*GetInterError)( UINT8 *, UINT8 *,  UINT8 *, UINT32 );

// Misc
extern UINT32  MvMaxExtent;
*/
extern INT32 * AbsX_LUT;

/****************************************************************************
*  Functions
*****************************************************************************
*/

// Function prototypes
extern void InitMotionCompensation ( CP_INSTANCE *cpi);
extern UINT32 GetIntraError( UINT8 * DataPtr, UINT32 PixelsPerLine );
extern UINT32 GetMBIntraError( CP_INSTANCE *cpi, UINT32 FragIndex, UINT32 PixelsPerLine );
extern UINT32 GetMBInterError( CP_INSTANCE *cpi, UINT8 * SrcPtr, UINT8 * RefPtr, UINT32 FragIndex, INT32 LastXMV, INT32 LastYMV, UINT32 PixelsPerLine );
extern UINT32 GetMBMVInterError( CP_INSTANCE *cpi, UINT8 * RefFramePtr, UINT32 FragIndex, UINT32 PixelsPerLine, INT32 *MVPixelOffset, MOTION_VECTOR *MV );
extern UINT32 GetMBMVExhaustiveSearch( CP_INSTANCE *cpi, UINT8 * RefFramePtr, UINT32 FragIndex, UINT32 PixelsPerLine, MOTION_VECTOR *MV );
extern UINT32 GetFOURMVExhaustiveSearch( CP_INSTANCE *cpi, UINT8 * RefFramePtr, UINT32 FragIndex, UINT32 PixelsPerLine, MOTION_VECTOR *MV );


/*
extern INT32 GetInterErr( UINT8 * NewDataPtr, UINT8 * RefDataPtr1,  UINT8 * RefDataPtr2, UINT32 PixelsPerLine );
extern INT32  GetSumAbsDiffs( UINT8 * NewDataPtr, UINT8 * RefDataPtr, 
							  UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar );
extern INT32  GetNextSumAbsDiffs( UINT8 * NewDataPtr, UINT8 * RefDataPtr, 
	  				          UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar );
extern INT32  GetHalfPixelSumAbsDiffs( UINT8 * SrcData, UINT8 * RefDataPtr1, UINT8 * RefDataPtr2, 
    						  UINT32 PixelsPerLine, INT32 ErrorSoFar, INT32 BestSoFar );
*/

#endif

