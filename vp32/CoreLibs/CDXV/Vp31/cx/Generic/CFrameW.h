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
*   Module Title :     CFrameW.h
*
*   Description  :     Functions output buffering functions.
*
*****************************************************************************
*/

#ifndef CFRAMEW_H
#define CFRAMEW_H

#include "type_aliases.h"

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

extern void WriteFrameHeader(CP_INSTANCE *cpi);
//extern void SetFrameType( UINT8 FrType );
//extern UINT8 GetFrameType();
extern void WriteFrameSynch(CP_INSTANCE *cpi);
extern void Write32ToBuffer( CP_INSTANCE *cpi, UINT8 * Data );




#endif