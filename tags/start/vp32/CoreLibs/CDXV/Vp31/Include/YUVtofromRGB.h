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
*   Module Title :     YUVtofromRGB
*
*   Description  :     YUV/RGB conversion module header
*
*****************************************************************************
*/

#ifndef YUVRGB_H
#define YUVRGB_H

/****************************************************************************
*  Header Files
*****************************************************************************
*/

#ifdef PBDLL
#include "pbdll.h"
#endif

#include "rawTypes.h"

/****************************************************************************
*  Constants.
*****************************************************************************
*/        

/****************************************************************************
*  Types
*****************************************************************************
*/        

typedef struct BGR_TYPE
{
    unsigned char Blue;
    unsigned char Green;
    unsigned char Red;
} BGR_TYPE;

/****************************************************************************
*   Data structures
*****************************************************************************
*/
/****************************************************************************
*  Functions
*****************************************************************************
*/

#ifdef PBDLL
#endif

#ifdef COMPDLL
#endif


#endif