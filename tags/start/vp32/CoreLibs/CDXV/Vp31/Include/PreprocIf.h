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
*   Module Title :     SCAN_IF.H
*
*   Description  :     Content analysis module interface header
*
*
*****************************************************************************
*/						

#ifndef SCAN_IF_HEADER
#define SCAN_IF_HEADER

#include "type_aliases.h"
#define DLL

/* Type definitions. */
typedef struct
{
	UINT8 * Yuv0ptr;
	UINT8 * Yuv1ptr;
	UINT8 * SrfWorkSpcPtr;
	UINT8 * disp_fragments;

    UINT32 * RegionIndex;      // Gives pixel index for top left of each block 
	UINT32 VideoFrameHeight;
	UINT32 VideoFrameWidth;
	UINT8 HFragPixels;
	UINT8 VFragPixels;

} SCAN_CONFIG_DATA;

typedef enum
{	
	SCP_CONFIGURE_PP

} SCP_SETTINGS;


typedef struct PP_INSTANCE * xPP_INST;
extern DLL void SetScanParam( xPP_INST ppi, UINT32 ParamId, INT32 ParamValue );
extern DLL UINT32 YUVAnalyseFrame( xPP_INST ppi, UINT32 * KFIndicator );

extern DLL void DeletePPInstance(xPP_INST *);
extern DLL xPP_INST CreatePPInstance(void);
extern DLL BOOL ScanYUVInit( xPP_INST , SCAN_CONFIG_DATA * ScanConfigPtr );

#endif  // Prevents nested re-includes

