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
// dxlqt_cx.cpp
//
// Purpose: Main code for the quick time codec.  All other files are 
// called from this. This codec is based upon the electricimage sample, 
// which uses the base image compressor.
//
/////////////////////////////////////////////////////////////////////////

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
#include "ImageCodec.h"
#include "duck_dxl.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <timer.h>
#include "dxlqt_helper.h"
#include "regentry.h"

#include "dxlqt_codec.h"

#if TARGET_OS_MAC || SJLPCTEST
#include "dxlqt_config.h"
#endif

#if !TARGET_OS_MAC
#if !defined(VP3_COMPRESS)
#if COMP_BUILD
#include "stdafx.h"
#endif
#endif

#include "resource.h"

#else

#include "duk_rsrc.h"


#endif 

//******************************************************************
//** Compression Config Settings
//******************************************************************


#ifdef VP3_COMPRESS

  //char logmsg[2000];
  //::ofstream logfile;


#if !TARGET_OS_MAC

extern BOOL FAR PASCAL Config_ParamsDlgProc(   HWND   hWndDlg,
                                        UINT   Message,
                                        WPARAM wParam,
                                        LPARAM lParam);

extern "C"
{
extern HMODULE ghModule;
}
#endif



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqt_copyVP3SettingsToHandle(COMP_CONFIG *CompConfig, Handle settings)
{ 
	COMP_CONFIG *configSettings;
	
	// make sure our pointer is big enough
	SetHandleSize(settings, sizeof(COMP_CONFIG));
	
	configSettings = (COMP_CONFIG *) *settings;

	// store our settings into the handle
    HLock(settings);
	memcpy((void *)configSettings, (void *)CompConfig, sizeof(COMP_CONFIG));
    HUnlock(settings);
}
#endif

//******************************************************************
//** Start Compression Code
//******************************************************************
void writeFile(int a, unsigned char *x, unsigned int length)
{
    char name[255];
    sprintf(name, "test%d.raw",a);
    FILE *f = fopen(name,"wb");
    fwrite(x,1,length,f);
    fclose(f);
};

#if COMP_BUILD
// ************************************************************
// Function name	: dxlqt_CDPreCompress
// Description	    : CDPreCompress gets called before an image is compressed, 
//					: or whenever the source pixmap
//					: pixel size changes when compressing a sequence. 
//					: We return information about
//					: how we can compress the image to the codec manager, 
//					: so that it can fit the source data
//					: to our requirements. The ImageDescriptor is already 
//					: filled in, so we can use it for reference (or even 
//					: add to it ). PixelSize is the pixel depth of the 
//					: source pixmap, we use this as a reference for deciding 
//					: what we can do. The other parameters return information
//					: to the CodecManager about what we can do. We can also 
//					: do setup here if we want to.
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : register CodecCompressParams *p
// ************************************************************
pascal ComponentResult 
dxlqt_CDPreCompress(dxlqt_Globals glob, register CodecCompressParams *p)
{
	CodecCapabilities *capabilities = p->capabilities;
//	ImageDescription	**desc = p->imageDescription;

	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDPreCompress=>",(**glob).instancenum));

	// tell quicktime we handle the previous buffer, and are sequence sensitive
	capabilities->flags = codecCanManagePrevBuffer|codecIsSequenceSensitive;

	// we want 32 bit input !!!
	capabilities->wantedPixelSize = 32;

#ifdef VP3_COMPRESS	 
	int width, height;

	try
	{
		width = (*p->imageDescription)->width;
		height = (*p->imageDescription)->height;

        // Tell quick time that the band has to be the same size as the image
        capabilities->bandMin = (*p->imageDescription)->height;
        capabilities->bandInc = capabilities->bandMin;

		// use 16x16 blocks (extend to multiple of 16)
		capabilities->extendWidth = (16 -  (width & 15))&15;
		capabilities->extendHeight = (16 - (height & 15))&15;

		width += capabilities->extendWidth;
		height += capabilities->extendHeight;

        // Frame rate 1/frameduration
		int fr;
		if (p->dataRateParams->frameDuration != 0)
			fr = (int) (((1.0 / p->dataRateParams->frameDuration) * 1000.0) + 0.5);
		else
			fr = 30; 
		
		// key frame frequency  
	    long kf;
    	GetCSequenceKeyFrameRate(p->sequenceID, &kf);
		if (kf==0)	
            kf=99999;

		// Set the frame size component
        if(p->dataRateParams->dataRate == 0)
        {
            (**glob).CompConfig.TargetBitRate = 3*1024;
        }
        else
        {
            (**glob).CompConfig.TargetBitRate = 8 * p->dataRateParams->dataRate/1024;
        }

        (**glob).CompConfig.FrameRate = (int) fr;
		(**glob).CompConfig.FrameSize = (INT32)(width << 16) + (INT32) height;

		
		// hardcoding "best" noise sensitivity for most cases (aka temporal quality) -tjf 2/8/2001
		(**glob).CompConfig.NoiseSensitivity = 1;
		(**glob).CompConfig.Quality = 56;

        //set the key frame target data rate 
        (**glob).CompConfig.KeyFrameDataTarget = (unsigned int)((width * height * 1.5)/1024);
        (**glob).CompConfig.KeyFrameFrequency = 120;

		// if we have a yuv buffer already delete it in case the size changed		
		if((**glob).yuv_buffer)
		{
			delete [] (**glob).yuv_buffer;
			(**glob).yuv_buffer = 0;
		}
		
		// set up for yuv conversion
		(**glob).yuv_buffer = new unsigned char [width * height * 4 ];
		(**glob).yuv_config.UVStride = width >> 1;
		(**glob).yuv_config.UVWidth = width >> 1;
		(**glob).yuv_config.UVHeight = height >> 1;
		(**glob).yuv_config.YStride = width;
		(**glob).yuv_config.YWidth = width;
		(**glob).yuv_config.YHeight = height;	
		(**glob).yuv_config.YBuffer = (( char *) (**glob).yuv_buffer) ;
		(**glob).yuv_config.UBuffer = (( char *) (**glob).yuv_buffer + (**glob).yuv_config.YWidth * (**glob).yuv_config.YHeight) ;
		(**glob).yuv_config.VBuffer = (( char *) (**glob).yuv_config.UBuffer + (**glob).yuv_config.UVWidth * (**glob).yuv_config.UVHeight) ;
				

		StartEncoder( &(**glob).cpi, &(**glob).CompConfig );

		ChangeCompressorSetting ( (**glob).cpi, C_SET_NODROPS, 1 );
		
		DXLQT_LOG(sprintf(logmsg,"%d width,%d height,%d pitch \n",
			(*p->imageDescription)->width,
			(*p->imageDescription)->height,
			(*p->imageDescription)->width*4));
	}
	catch (...)
	{
		return MemError();
	}
		
#endif 
	return(noErr);
}


// ************************************************************
// Function name	: dxlqt_CDBandCompress
// Description	    : Compress one Frame. Note that we are first
//					: converting to rgb24 from the heinous qt 
//                  : internal format.
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : register CodecCompressParams *p
// ************************************************************
pascal ComponentResult 
dxlqt_CDBandCompress(dxlqt_Globals glob,register CodecCompressParams *p)
{
	short				width,height;
	Ptr					cDataPtr;
	Rect				sRect;
	unsigned char		*baseAddr;
	short				rowBytes;
	long				stripBytes;
	ImageDescription	**desc = p->imageDescription;
	OSErr				result = noErr;

	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDBandCompress\n",(**glob).instancenum));
	DXLQT_LOG(callflags2string(p->callerFlags,logmsg));
	DXLQT_LOG(condflags2string(p->conditionFlags,logmsg));

	//	If there is a progress proc, give it an open call at the start of this band. 
	if (p->progressProcRecord.progressProc)
	{	
        // InterfaceLib way:
        // CallICMProgressProc(p->progressProcRecord.progressProc,codecProgressOpen,0,
		//	p->progressProcRecord.progressRefCon);
		
		// CarbonLib/OS X way:
		InvokeICMProgressUPP(codecProgressOpen, 0, p->progressProcRecord.progressRefCon, 
			p->progressProcRecord.progressProc);
	}

	// make sure width and height are divisible by 16!
	#ifdef VP3_COMPRESS	
		width = ((*desc)->width >> 4 )<<4;
		height = ((*desc)->height >> 4) << 4;
	#endif 

	// figure out offset to first pixel in baseAddr from the pixelsize and bounds 
	rowBytes = p->srcPixMap.rowBytes & 0x3fff;
	sRect =  p->srcPixMap.bounds;
	stripBytes = ((width+1)>>1) * 5;
	cDataPtr = p->data;

	baseAddr = (unsigned char *) p->srcPixMap.baseAddr + (sRect.left<<2) + (sRect.top * rowBytes);
    HLock((char **) &baseAddr);
    HLock(&cDataPtr);


	// if there is not a flush proc, adjust the pointer to the next band 
	if (  p->flushProcRecord.flushProc == nil )
		cDataPtr += (p->startLine>>1) * stripBytes;
	else {
		if ( p->bufferSize < stripBytes ) {
			result = codecSpoolErr;
			goto bail;
		}
	}


#ifdef VP3_COMPRESS
	unsigned int is_key;
	unsigned char *InvY;
	unsigned char *InvU;
	unsigned char *InvV;
	InvY = (unsigned char *) (**glob).yuv_config.YBuffer + (**glob).yuv_config.YWidth * ((**glob).yuv_config.YHeight-1);
	InvU = (unsigned char *) (**glob).yuv_config.UBuffer + (((**glob).yuv_config.YWidth >> 1 ) * (((**glob).yuv_config.YHeight>>1) - 1));
	InvV = (unsigned char *) (**glob).yuv_config.VBuffer + (((**glob).yuv_config.YWidth >> 1 ) * (((**glob).yuv_config.YHeight>>1) - 1));

	is_key = p->callerFlags&codecFlagForceKeyFrame;//||(p->temporalQuality==0);

	int fr;
	if (p->dataRateParams->frameDuration != 0)
		fr = (int) ((1.0 / p->dataRateParams->frameDuration) * 1000.0);
	else
		fr = 15; 
	
	try
	{

		// keep in mind YUV is 12
		ConvertRGBtoYUV(baseAddr+1, baseAddr+2,baseAddr+3,
			(**glob).yuv_config.YWidth, (**glob).yuv_config.YHeight, 4, rowBytes,
			InvY, InvU, InvV,
			1, 1, 1 , -(**glob).yuv_config.YWidth, 1, -((**glob).yuv_config.YWidth >> 1));


	    if(is_key)
		    ChangeCompressorSetting ( (**glob).cpi, C_SET_KEY_FRAME, 0 );
		
		(*p->imageDescription)->dataSize =  EncodeFrameYuv( (**glob).cpi, &(**glob).yuv_config, 
			(unsigned char *) cDataPtr, &is_key );


	}
	catch (...)
	{
		return MemError();
	}

	// similarity tells qt if its a key frame or not
	if (is_key)
    {
        // Key Frame
		p->similarity = 0;  
	}
    else
    {
	    if((*p->imageDescription)->dataSize > 0) 
        {
            // non-Key Frame
            p->similarity = 0x7FFEFFFF;	
        }
        else
        {
            // this fixes the Premiere -50 error msg
            // when we drop a frame we have to tell quicktime that this 
            // frame is identical to the last
            p->similarity = 0x10000;	
        }
    }

	DXLQT_LOG(sprintf(logmsg,"%d bytes encoded,%d datarate, %d quality, %s , %d frameNumber\n",
		(*p->imageDescription)->dataSize,
		p->dataRateParams->dataRate/1024,
		 ((int) p->spatialQuality*100/1024),
		 (is_key?"KeyFrame":"Non KeyFrame"),
         p->frameNumber));


#endif 

#if FAKE_COMPRESS 
	(*p->imageDescription)->dataSize = 50;
#endif 

    HUnlock(&cDataPtr);
    HUnlock((char **) &baseAddr);

bail:
	//	If there is a progress proc, give it a close call at the end of this band. 
	if (p->progressProcRecord.progressProc)
	{
		// InterfaceLib way:
		//  CallICMProgressProc(p->progressProcRecord.progressProc,codecProgressClose,0,
		//	p->progressProcRecord.progressRefCon);
		
		// CarbonLib/OS X way:
		InvokeICMProgressUPP(codecProgressClose, 0, p->progressProcRecord.progressRefCon, 
			p->progressProcRecord.progressProc);
	}


	return(result);
}

// ************************************************************
// Function name	: dxlqt_CDRequestSettings
// Description	    : Called to Ask us to display our config 
//					: window..
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : Handle settings
// Argument         : Rect * rp
// Argument         : ModalFilterUPP filterProc
// ************************************************************
pascal ComponentResult 
dxlqt_CDRequestSettings(dxlqt_Globals glob, Handle settings, Rect *, ModalFilterUPP)
{
	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDRequestSettings\n",(**glob).instancenum));

#ifdef VP3_COMPRESS
	COMP_CONFIG *configSettings;

	// get the settings from the settings they passed in
	if(GetHandleSize(settings))
	{
		configSettings = (COMP_CONFIG *) *settings;

		// store current settings into the glob
		memcpy((void *)&(**glob).CompConfig, (void *)configSettings, sizeof(COMP_CONFIG));
	}

#if TARGET_OS_MAC || SJLPCTEST
    dxlqtConfigDialog dxlqtConfig(glob);
#else 
	HWND hwnd = GetActiveWindow(); 
    int dialogErr;

	dialogErr = DialogBoxParam((struct HINSTANCE__ *) ghModule,"Configure", hwnd, Config_ParamsDlgProc, (LPARAM) &(**glob).CompConfig);
#endif

#endif

	dxlqt_copyVP3SettingsToHandle(&(**glob).CompConfig, settings);

#if USE_SHARED_COMPCONFIG
    COMP_CONFIG *sharedCompConfig;
    if( (sharedCompConfig = (COMP_CONFIG *)GetComponentRefcon((Component)(**glob).self)) != nil)
    {
        //now lets update the shared copy of CompConfig
	    memcpy((void *)sharedCompConfig, (void *)&(**glob).CompConfig, sizeof(COMP_CONFIG));
    }
#endif
       
	return(noErr);
}


// ************************************************************
// Function name	: dxlqt_CDSetSettings
// Description	    : Called to store our settings into a handle
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : Handle settings
// ************************************************************
pascal ComponentResult 
dxlqt_CDSetSettings(dxlqt_Globals glob, Handle settings)
{ 

#ifdef VP3_COMPRESS	

	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDSetSettings\n",(**glob).instancenum));

	//if empty we need to set to a default state
	if (GetHandleSize(settings) == 0)
	{
		getCompConfigDefaultSettings(&(**glob).CompConfig);

    	dxlqt_copyVP3SettingsToHandle(&(**glob).CompConfig, settings);
	}

	//load internal settings 	
	COMP_CONFIG *configSettings;
	configSettings = (COMP_CONFIG *) *settings;
	memcpy((void *)&(**glob).CompConfig, (void *)configSettings, sizeof(COMP_CONFIG));

#if USE_SHARED_COMPCONFIG
    COMP_CONFIG *sharedCompConfig;
    if( (sharedCompConfig = (COMP_CONFIG *)GetComponentRefcon((Component)(**glob).self)) != nil)
    {
        //now lets update the shared copy of CompConfig
	    memcpy((void *)sharedCompConfig, (void *)&(**glob).CompConfig, sizeof(COMP_CONFIG));
    }
	DXLQT_LOG(sprintf(logmsg,"SHARED %d\n",sharedCompConfig->MinimumDistanceToKeyFrame));
#endif

#endif 

	return(noErr);
}

// ************************************************************
// Function name	: dxlqt_CDGetSettings
// Description	    : Store Settings -> UIS
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : Handle settings
// ************************************************************
#include <assert.h>

pascal ComponentResult 
dxlqt_CDGetSettings(dxlqt_Globals glob, Handle settings)
{
#ifdef VP3_COMPRESS	
	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDGetSettings\n",(**glob).instancenum));


	// get the settings from the settings they passed in
    if(settings != nil)
	{
		COMP_CONFIG *configSettings;

		// make sure our pointer is big enough
		SetHandleSize(settings, sizeof(COMP_CONFIG));
        HLock(settings);				
		configSettings = (COMP_CONFIG *) *settings;

		// store current settings into the glob
		memcpy((void *)configSettings, (void *)&(**glob).CompConfig, sizeof(COMP_CONFIG));
        HUnlock(settings);	
	}
#endif 
	return(noErr);

}

// ************************************************************
// Function name	: dxlqt_CDGetMaxCompressionSize
// Description	    : Returns maximum compressed size 1 byte per pixel + 4096 overhead
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals storage
// Argument         : PixMapHandle src
// Argument         : const Rect *srcRect
// Argument         : short depth
// Argument         : CodecQ quality
// Argument         : long *size
// ************************************************************
pascal ComponentResult 
dxlqt_CDGetMaxCompressionSize(dxlqt_Globals,
							  PixMapHandle,
							  const Rect *srcRect,
							  short, 
							  CodecQ,
							  long *size)
{
	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDGetMaxCompressionSize\n",(**glob).instancenum));

	short width = srcRect->right - srcRect->left;
	short height = srcRect->bottom - srcRect->top;

	*size = width * height + 4096;

	return(noErr);
}

// ************************************************************
// Function name	: dxlqt_CDGetCompressionTime
// Description	    : When dxlqt_CDGetCompressionTime is called, we return 
//					: the approximate time for compressing the given image 
//					: in milliseconds. We also return the closest actual 
//					: quality we can handle for the requested value.
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals storage
// Argument         : PixMapHandle src
// Argument         : const Rect *srcRect
// Argument         : short depth
// Argument         : CodecQ *spatialQuality
// Argument         : CodecQ *temporalQuality
// Argument         : unsigned long *time
// ************************************************************
pascal ComponentResult 
dxlqt_CDGetCompressionTime(dxlqt_Globals,
							  PixMapHandle,
							  const Rect *,
							  short,
							  CodecQ *spatialQuality,
							  CodecQ *temporalQuality,
							  unsigned long *time)
{
	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDGetCompressionTime\n",(**glob).instancenum));

	if (time)	
    {
		*time = 1;			//lightning fast
    }

    if(spatialQuality)
    {
        // we dont use this anyway, set it to max for that warm and fuzzy feeling 
		*spatialQuality = 1024;
    }

    if(temporalQuality)
    {
        // we dont use this anyway, set it to max for that warm and fuzzy feeling
		*temporalQuality = 1024;
    }

	return (noErr);
}

#endif 
//** End compress Code 
//******************************************************************
