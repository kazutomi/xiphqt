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
*   Module Title :     BIT_MAN.h
*
*   Description  :     Video CODEC  : Bit manipulation routines header.
*
*               
*****************************************************************************
*/

/****************************************************************************
*  Header Frames
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */

#ifndef CBITMAN_H
#define CBITMAN_H

#include "compdll.h"

/****************************************************************************
*  Constants
*****************************************************************************
*/

/****************************************************************************
*  Types
*****************************************************************************
*/        

/****************************************************************************
*   Data structures
*****************************************************************************
*/

/****************************************************************************
*  Functions
*****************************************************************************
*/

extern void InitAddBitsToBuffer(CP_INSTANCE *cpi);
extern void AddBitsToBuffer( CP_INSTANCE *cpi, UINT32 data, UINT32 bits );
extern void EndAddBitsToBuffer( CP_INSTANCE *cpi );

#endif



