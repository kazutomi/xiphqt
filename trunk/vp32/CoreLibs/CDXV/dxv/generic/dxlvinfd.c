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


/*/////////////////////////////////////////////////////////////////////////
//
// dxlvinfd.c
//
// Purpose: A list of helper functions to the quick time codec code
//
///////////////////////////////////////////////////////////////////////*/

//#include <stdio.h>
//#include <math.h>
//#include <string.h>
#include "dxl_main.h"

struct DisplaySetting {
	long dotOne;
	long dotTwo;
	long dotThree;
	long dotFour;
	long dotFive;
};

static struct DisplaySetting id_RGB24 ={0x00000000,0x00000000,0xffffffff,0x00000000,0xffffffff}; 
static struct DisplaySetting id_RGB32 ={0x00000000,0x00000000,0x00000000,0x00000000,0xffffffff}; 
static struct DisplaySetting id_RGB555={0xffffffff,0x00000000,0xffffffff,0x00000000,0xffffffff}; 
static struct DisplaySetting id_RGB565={0xffffffff,0x00000000,0x00000000,0x00000000,0xffffffff}; 
static struct DisplaySetting id_UYVY  ={0xff80ff80,0x00800080,0xff80ff80,0x00800080,0x00800080}; 
static struct DisplaySetting id_YUY2  ={0x80ff80ff,0x80008000,0x80008000,0x80008000,0x80008000}; 
static struct DisplaySetting id_YVU9  ={0x80008000,0x80008000,0xff80ff80,0xff80ff80,0xff80ff80}; 
static struct DisplaySetting id_RGB8  ={0x00000000,0xffffffff,0x00000000,0xffffffff,0x00000000}; 


static struct DisplaySetting id_STRETCH 		={0x00000000,0xffffffff,0x00000000,0x00000000,0x00000000}; 
static struct DisplaySetting id_STRETCH_BRIGHT ={0xffffffff,0xffffffff,0x00000000,0x00000000,0x00000000}; 
static struct DisplaySetting id_STRETCH_SAME   ={0xffffffff,0x00000000,0x00000000,0x00000000,0x00000000}; 

static struct DisplaySetting id_KEY 	= 	{0x00000000,0x00000000,0xffffffff,0x00000000,0x00000000}; 
static struct DisplaySetting id_NOTKEY 	=	{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}; 

static struct DisplaySetting id_CLEAR_ME 	=	{0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}; 


static void OrSettings(struct DisplaySetting *src1,struct DisplaySetting *src2, struct DisplaySetting *dst)
{
	if (dst) {
		dst->dotOne = src1->dotOne | src2->dotOne;
		dst->dotTwo = src1->dotTwo | src2->dotTwo;
		dst->dotThree = src1->dotThree | src2->dotThree;
		dst->dotFour = src1->dotFour | src2->dotFour;
		dst->dotFive = src1->dotFive | src2->dotFive;
	}
}


static void SetSettings(struct DisplaySetting *dst,struct DisplaySetting *src)
{
	if (dst) {
		dst->dotOne = src->dotOne ;
		dst->dotTwo = src->dotTwo ;
		dst->dotThree = src->dotThree ;
		dst->dotFour = src->dotFour ;
		dst->dotFive = src->dotFive ;
	}
}



/* ************************************************************
// Function name	: DXL_VScreenInfoDots
// Description	    : Draw Dots on the screen 
// Return type		: int  
// Argument         : DXL_VSCREEN_HANDLE vScreen
// ************************************************************/


int DXL_VScreenInfoDots(DXL_XIMAGE_HANDLE xImage,  DXL_VSCREEN_HANDLE vScreen)
{
    int retVal=0;
    if (vScreen) {
		unsigned char *addr = vScreen->addr;
		const struct DisplaySetting *resolutionType = 0;
		struct DisplaySetting combinedInfo;
     	int key;
	 

        key =  DXL_IsXImageKeyFrame( xImage);
        
        SetSettings(&combinedInfo,&id_CLEAR_ME);

		
		switch( vScreen->bd ) {
			case DXRGB24:		resolutionType = &id_RGB24;  break;
			case DXRGB32:		resolutionType = &id_RGB32;  break;
			case DXRGB16_555:	resolutionType = &id_RGB555;  break;
			case DXRGB16_565:	resolutionType = &id_RGB565;  break;
			case DXYUY2:		resolutionType = &id_YUY2;  break;
			case DXYVU9:		resolutionType = &id_YVU9;  break;
			case DXHALFTONE8:	resolutionType = &id_RGB8;  break;
			default: break;
		}


		*(unsigned long*)(addr) = resolutionType->dotOne;
		*(unsigned long*)(addr+32) = resolutionType->dotTwo;
		*(unsigned long*)(addr+64) = resolutionType->dotThree;
		*(unsigned long*)(addr+96) = resolutionType->dotFour;
		*(unsigned long*)(addr+128) = resolutionType->dotFive;
 
        addr += 2 * vScreen->pitch;
		
		switch (vScreen->bq) {
		    case DXBLIT_SAME :        		resolutionType = &id_STRETCH_SAME;
		    	break;
			case DXBLIT_STRETCH:			resolutionType = &id_STRETCH;
				break;
			case DXBLIT_STRETCH_BRIGHT :	resolutionType = &id_STRETCH_BRIGHT;
				break;
			default: break;
		}
		 
		if (key)
			OrSettings((struct DisplaySetting  *) resolutionType, &id_KEY, &combinedInfo);
	    else 
	    	SetSettings(&combinedInfo,(struct DisplaySetting  *)resolutionType);
		
		*(unsigned long*)(addr) 		= combinedInfo.dotOne;
		*(unsigned long*)(addr+32) 		= combinedInfo.dotTwo;
		*(unsigned long*)(addr+64) 		= combinedInfo.dotThree;
		*(unsigned long*)(addr+96) 		= combinedInfo.dotFour;
		*(unsigned long*)(addr+128) 	= combinedInfo.dotFive;
		

	}
	else {
		retVal = -1; /* some damn error ! */
	}

	return retVal;
}





/* Turn White Dots on or off */
void DXL_VScreenSetInfoDotsFlag(DXL_VSCREEN_HANDLE vScreen, int showDots)
{ 
     if (vScreen)
	 	vScreen->flags.showInfoDots = showDots;
	 
}

