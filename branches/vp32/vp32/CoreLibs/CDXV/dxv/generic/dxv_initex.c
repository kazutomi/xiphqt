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


#include "dkpltfrm.h" /* platform specifics */
#include "duck_mem.h" /* interface to memory manager */
#include "dxl_main.h" /* interface to dxv */

extern void vp31_Init(void);


int DXL_InitVideoEx(int lmaxScreens,int lmaxImages)
{
	DXL_InitVideo(lmaxScreens+4,lmaxImages);

	vp31_Init();

	return DXL_OK;       

}