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


///////////////////////////////////////////////////////////////////////////
//
// dxlqt_codec.cpp
//
// Purpose: Main code for the quick time codec.  All other files are 
// called from this. This codec is based upon the electricimage sample, 
// which uses the base image compressor.
//
/////////////////////////////////////////////////////////////////////////

///	dbm -- 6/12/02 -- changes made for QT6 release -- pixel format lists
///			saved as vp32_qt3.zip
///			vp32_qt4.zip -- changes released to QT team for Win testing 6/13/02 -- version bumped to 3.2.6.0
///		-- 7/15/02 -- 3.2.6.1 -- incorporating recent fixes (ebx bug, QT colorspace) for XIPH release

#include "common.h"
#include <fstream.h>
#if TARGET_OS_MAC 
#define COMPTARG QTMAC
#include "GetSysFolder.h"
#else
#define COMPTARG QTW

#if !defined(VP3_COMPRESS)
#include <afxwin.h>
#include <windowsx.h>
#else
#include <windows.h>
#endif

#define INC_WIN_HEADER
#include <time.h>
#endif 

#include <Endian.h>
#include <ImageCodec.h>
#include "duck_dxl.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "dxlqt_helper.h"
#include "regentry.h"

#include "dxlqt_codec.h"

extern "C" void DXL_watermark(DXL_XIMAGE_HANDLE src,int state);



#define DXL_MKFOURCC( ch0, ch1, ch2, ch3 ) \
		( (unsigned long)(unsigned char)(ch0) | ( (unsigned long)(unsigned char)(ch1) << 8 ) |    \
		( (unsigned long)(unsigned char)(ch2) << 16 ) | ( (unsigned long)(unsigned char)(ch3) << 24 ) )

#define COMPFOURCC DXL_MKFOURCC('V','P','3','1')

#if !TARGET_OS_MAC
#include "resource.h"
#else
#include "duk_rsrc.h"
#endif 



// define format equivalents (mac/pc)
#if TARGET_OS_MAC
	#define DUCK_YUVS		kYUVSPixelFormat
	#define DUCK_24RGB 		k32ARGBPixelFormat
	#define DUCK_32RGB 		k32ARGBPixelFormat
	#define DUCK_16RGB555	k16BE555PixelFormat
	#define DUCK_16RGB565   k16BE555PixelFormat
	#define DUCK_8RGB       k8IndexedPixelFormat
#else
	#define DUCK_YUVS		kYUVSPixelFormat
	#define DUCK_24RGB 		k24BGRPixelFormat
	#define DUCK_32RGB 		k32BGRAPixelFormat
	#define DUCK_16RGB555	k16LE555PixelFormat
	#define DUCK_16RGB565	k16LE565PixelFormat
	#define DUCK_8RGB       k8IndexedPixelFormat
#endif

//******************************************************************
// Decompression Code 
//******************************************************************
#if DECO_BUILD


// ************************************************************
// Function name	: dxlqt_CDPreflight
// Description	    : Called prior to decompressing a sequence of
//					  frames. Here we return what the size of 
//                    the bands we can decompress are (the whole 
//					  image), and a list of formats we can decompress
//					  to. In addition we can say whether we can scale
//                    to this size (blacklining).
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : CodecDecompressParams *p
// ************************************************************

///	dbm -- debugging stuff
pascal ComponentResult 
dxlqt_CDPreflight(dxlqt_Globals glob, CodecDecompressParams *p)
{

	CodecCapabilities *capabilities = p->capabilities;
	OSType *pft;
 OSType *temp;
	unsigned long tempPixelFormat;

	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDPreFlight\n",(**glob).instancenum))

	// Tell quick time that the band has to be the same size as the image
	capabilities->bandMin = (**p->imageDescription).height;
	capabilities->bandInc = capabilities->bandMin;

	// Tell Quick Time we want to do 32 Bit 

///	dbm -- as per skipper, set to zero:
/*
	capabilities->wantedPixelSize = (**p->imageDescription).depth;
	if (capabilities->wantedPixelSize == 24)
		capabilities->wantedPixelSize = 32;
*/
	capabilities->wantedPixelSize = 0;			///	QT should use wantedDestinationPixelTypes below


#if 0
	// In previous versions we did extend width and height to a 
	// multiple of 2 (no clue why)
	capabilities->extendWidth = 0;
	capabilities->extendHeight = 5;
#else
    // use 16x16 blocks (extend to multiple of 16)
    capabilities->extendWidth = (16 -  ((**p->imageDescription).width & 15))&15;
    capabilities->extendHeight = (16 - ((**p->imageDescription).height & 15))&15;
#endif

	// Tell quicktime if async is okay
	#if ASYNC_DECODE 
		capabilities->flags = codecCanAsync;
	#else 
		capabilities->flags = 0;
	#endif 


	//****************************************************
	// set up our order of preferences to pixel types

	p->wantedDestinationPixelTypes = (**glob).PixelFormatList;
	pft=(OSType *) *p->wantedDestinationPixelTypes;
 temp=pft;
	tempPixelFormat = GETPIXMAPPIXELFORMAT((&(p->dstPixMap)));

//	*pft=k32BGRAPixelFormat;
	*pft=kYUVSPixelFormat;
	
	tempPixelFormat = GETPIXMAPPIXELFORMAT((&(p->dstPixMap)));




//capabilities->flags |= codecImageBufferIsOnScreen;

	int mType = p->matrixType;

///	dbm -- new code for QT6 (David Eldred et al):
	*pft++ = DUCK_YUVS;						///	always support YUV
	*pft++ = DUCK_16RGB555;
#if !TARGET_OS_MAC
	*pft++ = DUCK_16RGB565;
	*pft++ = DUCK_24RGB;
#endif
	if(mType <= translateMatrixType) {		///	only in this case,
		*pft++ = DUCK_32RGB;				///	32-bit mode
	}
	*pft++ = 0;								///	null-terminate

#if 0
	
	switch(tempPixelFormat) 
	{

	case DUCK_YUVS: 
///		if(!(**glob).stretchThis/**/)
			*pft++ = DUCK_YUVS;				///	1
///		*pft++ = DUCK_32RGB;				///	2
#if !TARGET_OS_MAC
///		*pft++ = DUCK_24RGB;
#endif
///		*pft++ = DUCK_16RGB555;				///	3

#if !TARGET_OS_MAC
///		*pft++ = DUCK_16RGB565;
#endif
		*pft++ = 0;
		break;
/*
	case DUCK_8RGB: 
		*pft++ = DUCK_8RGB;
		*pft++ = DUCK_32RGB;
		*pft++ = DUCK_24RGB;
		*pft++ = DUCK_16RGB555;
		*pft++ = DUCK_16RGB565;
		*pft++ = 0;
		break;
*/
	case DUCK_16RGB555:
		*pft++ = DUCK_16RGB555;
///		if(!(**glob).stretchThis/**/)
			*pft++ = DUCK_YUVS;
///		*pft++ = DUCK_32RGB;
#if !TARGET_OS_MAC
		*pft++ = DUCK_24RGB;
#endif
		*pft++ = DUCK_16RGB565;
		*pft++ = 0;
		break;

#if !TARGET_OS_MAC

	case DUCK_16RGB565:
		*pft++ = DUCK_16RGB565;
///		if(!(**glob).stretchThis)
			*pft++ = DUCK_YUVS;
		*pft++ = DUCK_32RGB;
		*pft++ = DUCK_24RGB;
		*pft++ = DUCK_16RGB555;
		*pft++ = 0;
		break;

	case DUCK_24RGB: 
		*pft++ = DUCK_24RGB;
///		if(!(**glob).stretchThis)
			*pft++ = DUCK_YUVS;
///		*pft++ = DUCK_32RGB;
		*pft++ = DUCK_16RGB555;
		*pft++ = DUCK_16RGB565;
		*pft++ = 0;
		break;

#endif 

	case DUCK_32RGB:
///		*pft++ = DUCK_32RGB;
///		if(!(**glob).stretchThis/**/)
			*pft++ = DUCK_YUVS;
#if !TARGET_OS_MAC
		*pft++ = DUCK_24RGB;
#endif
		*pft++ = DUCK_16RGB555;
#if !TARGET_OS_MAC
		*pft++ = DUCK_16RGB565;
#endif		
		*pft++ = 0; 
		break;
		
	default:					// we don't know how to do these, so return the default
///		if(!(**glob).stretchThis/**/)
			*pft++ = DUCK_YUVS;
///		*pft++ = DUCK_32RGB;
#if !TARGET_OS_MAC
		*pft++ = DUCK_24RGB;
#endif
		*pft++ = DUCK_16RGB555;
#if !TARGET_OS_MAC
		*pft++ = DUCK_16RGB565;
#endif
		*pft++ = 0; 
		break;
	}
	//****************************************************
#endif
	return noErr;
}


// ************************************************************
// Function name	: dxlqt_CDBeginBand
// Description	    : called prior to decompressing each frame,
//						responsible for setting up decomp param
//						structure used by draw band
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob 
// Argument         : CodecDecompressParams *p -> 
// Argument         : ImageSubCodecDecompressRecord *drp -> ours
// Argument         : long flags
// ************************************************************
pascal ComponentResult 
dxlqt_CDBeginBand(dxlqt_Globals glob, CodecDecompressParams *p, ImageSubCodecDecompressRecord *drp, long flags)
{

	long offsetH, offsetV;
	unsigned long tempPixelFormat;
	CodecCapabilities *capabilities = p->capabilities;

	dxlqt_DecompressRecord *myDrp = (dxlqt_DecompressRecord *)drp->userDecompressRecord;

	// figure out where in memory everything starts
	offsetH = (long)(p->dstRect.left - p->dstPixMap.bounds.left) * 
		      (long)(p->dstPixMap.pixelSize >> 3);
	offsetV = (long)(p->dstRect.top - p->dstPixMap.bounds.top) * 
			  (long)drp->rowBytes;

	// pick our pixel format 
	tempPixelFormat = GETPIXMAPPIXELFORMAT((&(p->dstPixMap)));
	
	switch(tempPixelFormat) 
	{
	case DUCK_32RGB:
		myDrp->bitdepth = DXRGB32;
		offsetH <<=2;					// 1 pixel = 4 bytes
		break;
	case DUCK_YUVS:
		myDrp->bitdepth = DXYUY2;
		offsetH <<=1;					// 1 pixel = 2 bytes 
		break;
	case DUCK_16RGB555:
#if TARGET_OS_MAC
		if(p->dstPixMap.planeBytes=='yuvs')
			myDrp->bitdepth = DXYUY2;
		else
#endif 
			myDrp->bitdepth = DXRGB16;
		offsetH <<=1;					// 1 pixel = 2 bytes 
		break;

#if !TARGET_OS_MAC

	case DUCK_16RGB565:
		myDrp->bitdepth = DXRGB16_565;
		offsetH <<=1;					// 1 pixel = 2 bytes
		break;

	case DUCK_24RGB:
		myDrp->bitdepth = DXRGB24;
		offsetH *= 3;					// 1 pixel = 3 bytes
		break;

#endif
/*
	case DUCK_8RGB:
		myDrp->bitdepth = DXHALFTONE8;
		break;
*/
	default:
		return codecErr;
		break; // Default shouldn't get hit for the output type that we finally decide upon.
	}

	myDrp->width = p->dstRect.right-p->dstRect.left;
	myDrp->height = p->dstRect.bottom-p->dstRect.top;

    // we have to do something special for odd sized images
	myDrp->width += capabilities->extendWidth;
	myDrp->height += capabilities->extendHeight;


	myDrp->pitch = p->dstPixMap.rowBytes;
	drp->baseAddr = p->dstPixMap.baseAddr + offsetH + offsetV;
	myDrp->fourcc = (*p->imageDescription)->cType;
	myDrp->compressedsize = p->bufferSize;
	// check if either of our two flags have been changed  since the last frame
	myDrp->scrnchanged = (**glob).lastdrawn==(p->frameNumber|p->conditionFlags&codecConditionNewTransform);

	DXLQT_LOG(callflags2string(p->callerFlags,logmsg));
	DXLQT_LOG(condflags2string(p->conditionFlags,logmsg));

	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDBeginBand=>Frame:%d,Last:%d,Cond:%x,Call:%x \n",
		(**glob).instancenum,p->frameNumber,(**glob).lastframe,p->conditionFlags,p->callerFlags));

	(**glob).lastframe=p->frameNumber;

	return noErr;
}


// ************************************************************
// Function name	: dxlqt_CDDrawBand
// Description	    : Actually does the decompress and blit
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : ImageSubCodecDecompressRecord *drp
// ************************************************************
pascal ComponentResult 
dxlqt_CDDrawBand(dxlqt_Globals glob, ImageSubCodecDecompressRecord *drp)
{
	OSErr err = noErr;
	dxlqt_DecompressRecord *myDrp = (dxlqt_DecompressRecord *)drp->userDecompressRecord;
	unsigned char *dataPtr = (unsigned char *)drp->codecData;
	ICMDataProcRecordPtr dataProc = drp->dataProcRecord.dataProc ? &drp->dataProcRecord : nil;

	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDDrawBand=>",(**glob).instancenum));

	// start tracking time
	//if (!(**glob).startTime) (**glob).startTime = Milliseconds();

#if DXV_DECOMPRESS	

	// if there is no ximage already
	if(!(**glob).xImage)
	{

        unsigned long dxlFourcc = (
            ((myDrp->fourcc & 0x000000FF) << 24) |
            ((myDrp->fourcc & 0x0000FF00) << 8) |
            ((myDrp->fourcc & 0x00FF0000) >> 8) |
            ((myDrp->fourcc & 0xFF000000) >> 24) );

	
		// Build ximage / vscreen
		(**glob).xImage = DXL_CreateXImageOfType( (unsigned char *) dataPtr, dxlFourcc );

        //if we are black lining, we need to adjust the width and height
		if((**glob).stretchThis)
		{
			if((**glob).useBlackLine)
			{
        		(**glob).xImage = DXL_AlterXImage( (**glob).xImage, (unsigned char *) dataPtr, dxlFourcc ,DXRGBNULL,
        			abs(myDrp->width) >> 1,abs(myDrp->height) >> 1);
			}
		}
		else
		{
    		(**glob).xImage = DXL_AlterXImage( (**glob).xImage, (unsigned char *) dataPtr, dxlFourcc ,DXRGBNULL,
    			abs(myDrp->width),abs(myDrp->height));
		}


		//(**glob).xImage = DXL_CreateXImage((unsigned char *) dataPtr);

		if((**glob).xImage == NULL) return 1;

		DXL_SetXImageCSize((**glob).xImage, myDrp->compressedsize);


		(**glob).vScreen = DXL_CreateVScreen((unsigned char *)drp->baseAddr,
			(enum BITDEPTH)myDrp->bitdepth,(short)myDrp->pitch,(short)myDrp->height);

		if((**glob).vScreen == NULL) 
		{
			DXL_DestroyXImage((**glob).xImage);
			(**glob).xImage = NULL;
			return 1;
		}

		if((**glob).cpuFree==-1)
		{
	        vp31_SetParameter((**glob).xImage,1, 0);
		    vp31_SetParameter((**glob).xImage,0, (**glob).postProcessLevel);
		}
		else
		{
			vp31_SetParameter ((**glob).xImage,1, (**glob).cpuFree);
		}

		DXL_VScreenSetInfoDotsFlag((**glob).vScreen, (**glob).showWhiteDots);

	}

	// alter the attibutes of the vscreen
	DXL_AlterVScreen((**glob).vScreen,(unsigned char *)drp->baseAddr,
		(enum BITDEPTH)myDrp->bitdepth,(short)myDrp->pitch,(short)myDrp->height);

	// alter the viewing region of the vscreen 
	DXL_AlterVScreenView((**glob).vScreen,0,0,myDrp->width,myDrp->height);

	if(myDrp->scrnchanged)
	{
		DXL_SetVScreenBlitQuality((**glob).vScreen,DXBLIT_SAME);
		DXL_VScreenSetInfoDotsFlag((**glob).vScreen, (**glob).showWhiteDots);
	}


	if(DXL_AlterXImageData((**glob).xImage,(unsigned char *) dataPtr)) 
		return 2;

	if(DXL_dxImageToVScreen((**glob).xImage,(**glob).vScreen) < 0) 
		return 4; 

	(**glob).dxvFrames++;
	//(**glob).endTime = Milliseconds();

	DXLQT_LOG(char tmp[20];tmp[0]=0;bitdepth2string(myDrp->bitdepth,tmp);
	    sprintf(logmsg,"bd:%s,p:%d,w:%d,h:%d,k:%d,ss:%d,bl:%d,st:%d,fps:%f,:msec:%d \n",
		tmp,myDrp->pitch,myDrp->width,myDrp->height,
		DXL_IsXImageKeyFrame((**glob).xImage),(**glob).softwareStretch,(**glob).useBlackLine,(**glob).stretchThis,
		1000.0*(**glob).dxvFrames/((**glob).endTime-(**glob).startTime),
		((**glob).endTime-(**glob).startTime)))

#endif
 
#if FAKE_DECOMPRESS
	DrawRandom(drp->baseAddr,myDrp->width*2,myDrp->height,myDrp->pitch);
#endif

	(**glob).lastdrawn=(**glob).lastframe;

	return err;
}


#endif 
//** End decompress Code 
//******************************************************************



