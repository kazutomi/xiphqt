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
// dxlqt_x.cpp
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
#include <ImageCodec.h>
#include "duck_dxl.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "dxlqt_helper.h"
#include "regentry.h"

#include "dxlqt_codec.h"

#ifdef DXLQT_LOGGING
 // char logmsg[2000];
 // ::ofstream logfile("Macintosh HD:vp3log.txt", ios::app);
#endif

#if !TARGET_OS_MAC
#include "resource.h"
#else
#include "duk_rsrc.h"
#endif 

#define kdxlqt_CodecVersion		(0x00020001)
#define kdxlqt_CodecVersionPPC	(0x00020002)		// PPC is +1


unsigned long instancenum;
char logmsg[2000];
FILE* logfile;


// ************************************************************
// Name				: Startup
// Description	    : class with a single global instance, whose 
//					: only purpose is initialization/DeInitialization
//					: of the DLL. There are other ways to do this!
// Return type		: pascal ComponentResult 
// Argument         : ComponentParameters *params
// Argument         : dxlqt_Globals glob
// ************************************************************

class Startup
{
public:
	Startup()
	{
		instancenum=0;

#if DECO_BUILD		// Decompressor only
#if DXV_DECOMPRESS	
		DXL_InitVideoEx(100,100);
#if COMP_BUILD									// compressor variables 
#ifdef VP3_COMPRESS
		VPEInitLibrary();
#endif
#endif
#endif
#endif 

	}
	~Startup() 
	{

#if DECO_BUILD		// Decompressor only
#if DXV_DECOMPRESS	
		DXL_ExitVideo();
#if COMP_BUILD									// compressor variables 
#ifdef VP3_COMPRESS
		VPEDeInitLibrary();
#endif
#endif
#endif
#endif 

#ifdef DXLQT_LOGGING

		if(logfile)
			fclose(logfile);

#endif 

	}
} theStartup;



// ************************************************************
// ** Start Dispatcher Macro Code
// ** 
// ** This ugly use of macros is how quicktime implements a pseudo
// ** base class.  ComponentDispatchHelper.C actually holds the 
// ** component dispatcher.  It uses these macros to set up the 
// ** proper function.
// ************************************************************


// dispatcher defines 
#define IMAGECODEC_BASENAME() 		dxlqt_CD
#define IMAGECODEC_GLOBALS() 		dxlqt_Globals storage

#define CALLCOMPONENT_BASENAME()	IMAGECODEC_BASENAME()
#define	CALLCOMPONENT_GLOBALS()		IMAGECODEC_GLOBALS()
#define COMPONENT_UPP_PREFIX()		uppImageCodec
#define COMPONENT_DISPATCH_FILE		"dxlqt_CodecDispatch.h"
#define COMPONENT_SELECT_PREFIX()  	kImageCodec
#define	GET_DELEGATE_COMPONENT()	((**storage).delegateComponent)

// QuickTime Dispatcher Helper
#include <Components.k.h>
#include <ImageCodec.k.h>

#if TARGET_OS_MAC
#include "ComponentDispatchHelper.h"
#else
#include "ComponentDispatchHelper.c"
#endif

// ** End Dispatcher Macro Code
// ************************************************************

#define kNumPixelFormatsSupported 5


#if !TARGET_OS_MAC
// ************************************************************
// Function name	: CDComponentDispatch
// Description	    : PC only component dispatcher.  Required because
//					: of evil AFX_MANAGE_STATE, w/o which we would 
//					: be unable to use AFX for our configure window.
// Return type		: pascal ComponentResult 
// Argument         : ComponentParameters *params
// Argument         : dxlqt_Globals glob
// ************************************************************
pascal ComponentResult 
CDComponentDispatch(ComponentParameters *params, dxlqt_Globals glob)
{
#if !defined(VP3_COMPRESS)
#if COMP_BUILD
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
#endif
#endif
	return dxlqt_CDComponentDispatch(params,glob);
}
#endif 


//******************************************************************
// Initialization/ Open and Close Functions 
//******************************************************************


// ************************************************************
// Function name	: dxlqt_CDOpen
// Description	    : Called whenever an app wants to find 
//					  information, compress a file, or decompress
//					  a file
// Return type		: pascal ComponentResult - error
// Argument         : dxlqt_Globals glob - allocated and returned
// Argument         : ComponentInstance self 
// ************************************************************
pascal ComponentResult 
dxlqt_CDOpen(dxlqt_Globals glob, ComponentInstance self)
{
	ComponentResult err;
	unsigned long sizeItem;

	glob = (dxlqt_Globals)NewHandleClear(sizeof(dxlqt_GlobalsRecord));
	if (err = MemError()) 
		return err;

	SetComponentInstanceStorage(self, (Handle)glob);
	(**glob).self = self;
	(**glob).target = self;
	(**glob).instancenum = instancenum++;
	(**glob).cpuFree = 70;

	// set up to read from the registry/resource fork
	(**glob).regAccess = (RegistryAccess) NewPtrClear(sizeof(RegistryAccessStruct));
	if(!(**glob).regAccess) 
		return(MemError());
	
#if TARGET_OS_MAC || SJLPCTEST
	(**glob).regAccess->theCodec = self;
	(**glob).regAccess->resFileID = 0;
	(**glob).regAccess->oldID = 0;	
#else
	sprintf((char *) (**glob).regAccess,REGACCESS);
#endif
	// end setup read from registy / resource fork

	// Get LogFile Information
#ifdef VP3_COMPRESS
	char maindirectory[512];

#if COMP_BUILD
	(**glob).yuv_buffer = 0;
#endif

#if TARGET_OS_MAC
 	GetSystemFolderPath(kExtensionFolderType,maindirectory);
	strcat(maindirectory,":VP31");
#else
	Registry_GetEntry(&maindirectory,
		REG_CSTRING,
		&sizeItem,
		"DuckDir",
		(**glob).regAccess);
#endif		

#endif 

// modified 9/11/2000 -TF
#ifdef DXLQT_LOGGING
	if(instancenum==1)
	{
		char filename[1024];

		Registry_GetEntry(&filename,
			REG_CSTRING,
			&sizeItem,
			"strLogFile",
			(**glob).regAccess);
		
		if(strlen(filename)>0)
		{
		    logfile = fopen(filename, "a");
		    //logfile.open(filename,ios::app);
		}
		else
		{
		    logfile = fopen("dxlqt.log", "a");
		}	
	}
#endif 


#if DECO_BUILD	// decompressor initialization
	// -------------------------------------------------------------------
	// Connect to the base image compressor
	err = OpenADefaultComponent(decompressorComponentType, kBaseCodecType, &(**glob).delegateComponent);
	if (err) 
		goto bail;
 
	ComponentSetTarget((**glob).delegateComponent, self);
	// -------------------------------------------------------------------

	(**glob).PixelFormatList = (OSType **)NewHandleClear(sizeof(OSType) * (kNumPixelFormatsSupported + 1));
	(**glob).lastframe=-1;
	(**glob).lastdrawn=-1;
	(**glob).startTime=0;
	(**glob).endTime=0;
	(**glob).dxvFrames=0;
	(**glob).showWhiteDots = 0;
	(**glob).softwareStretch = 0;
	(**glob).useBlackLine = 0;
	(**glob).stretchThis = 0; 
	Registry_GetEntry(&((**glob).showWhiteDots),REG_INTEGER,&sizeItem,"strWhiteDots",(**glob).regAccess);
	Registry_GetEntry(&((**glob).useBlackLine),REG_INTEGER,&sizeItem,"strUseBlackLine",(**glob).regAccess);
	Registry_GetEntry(&((**glob).softwareStretch),REG_INTEGER,&sizeItem,"strUseSoftwareStretch",(**glob).regAccess);
	Registry_GetEntry(&((**glob).cpuFree),REG_INTEGER,&sizeItem,"strCPUFree",(**glob).regAccess);
	if(Registry_GetEntry(&((**glob).postProcessLevel),REG_INTEGER,&sizeItem,"strPostProcessingLevel",(**glob).regAccess)!=0)
    {
        (**glob).postProcessLevel = 9;
    }


#endif 

#if COMP_BUILD	// Compressor initialization
#ifdef VP3_COMPRESS

#if USE_SHARED_COMPCONFIG
    COMP_CONFIG *sharedCompConfig;

    //lets check to see if the shared comp config settings have been initialized by
    //a previous instance.
    if( (sharedCompConfig = (COMP_CONFIG *)GetComponentRefcon((Component)self)) == nil)
    {
        if( (sharedCompConfig = (COMP_CONFIG *)NewPtrClear(sizeof(COMP_CONFIG))) == nil)
        {
            return (MemError());
        }

        SetComponentRefcon((Component)self, (long)sharedCompConfig);

	    //initialize comp config settings
	    getCompConfigDefaultSettings(sharedCompConfig);
    }
    
    //now lets update the instance's copy of CompConfig
	memcpy((void *)&(**glob).CompConfig, (void *)sharedCompConfig, sizeof(COMP_CONFIG));
#else
	//initialize comp config settings
	getCompConfigDefaultSettings(&(**glob).CompConfig);
#endif

#endif 
#endif 

	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDOpen\n",(**glob).instancenum));

bail:
	return err;
}


// ************************************************************
// Function name	: dxlqt_CDClose
// Description	    : Finished compressing, decompressing or 
//						collecting info. Clean out all the stuff
//						we allocated.
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : ComponentInstance self
// ************************************************************
pascal ComponentResult 
dxlqt_CDClose(dxlqt_Globals glob, ComponentInstance self)
{
	if ((glob)) 
	{
		
		DXLQT_LOG(sprintf(logmsg,"Instance:%d CDClose\n",(**glob).instancenum));
		
		DisposePtr((Ptr)(**glob).regAccess);
		
#if COMP_BUILD		// compressor deinitialization
#ifdef VP3_COMPRESS
		// Kill the encoder
		if((**glob).cpi)
		{
			StopEncoder(&(**glob).cpi);
		}
		if((**glob).yuv_buffer)
		{
			delete [] (**glob).yuv_buffer;
			(**glob).yuv_buffer = 0;
		}
		
		DXLQT_LOG(sprintf(logmsg,"Instance:%d Done with CDClose\n",(**glob).instancenum));
#endif 
#endif
		
#if DECO_BUILD	// decompressor deinitialization
#if DXV_DECOMPRESS	
		if(	(**glob).xImage!=0)
		{
			DXL_DestroyXImage((**glob).xImage);
			(**glob).xImage=0;
		}
		
		if(	(**glob).vScreen!=0)
		{
			DXL_DestroyVScreen((**glob).vScreen);
			(**glob).vScreen=0;
		}
#endif

		if ((**glob).PixelFormatList)
			DisposeHandle((Handle)(**glob).PixelFormatList);

#endif

		if ((**glob).delegateComponent)
			CloseComponent((**glob).delegateComponent);

#if USE_SHARED_COMPCONFIG
        if( CountComponentInstances((Component)self) == 1)
        {
            COMP_CONFIG *sharedCompConfig;
            if( (sharedCompConfig = (COMP_CONFIG *)GetComponentRefcon((Component)self)) != nil)
            {
                DisposePtr((Ptr)sharedCompConfig);
                SetComponentRefcon((Component)self, nil);
            }
        }
#endif

		DisposeHandle((Handle)glob);
	}

	return noErr;
}


// ************************************************************
// Function name	: dxlqt_CDTarget
// Description	    : Target component set, or changed
// Return type		: Store target in our globals
// Argument         : dxlqt_Globals glob
// Argument         : ComponentInstance target
// ************************************************************
pascal ComponentResult 
dxlqt_CDTarget(dxlqt_Globals glob, ComponentInstance target)
{
	(**glob).target = target;

	return noErr;
}


// ************************************************************
// Function name	: dxlqt_CDVersion
// Description	    : Tell the app, our version #
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// ************************************************************
pascal ComponentResult 
dxlqt_CDVersion(dxlqt_Globals glob)
{
#if TARGET_CPU_PPC
	return kdxlqt_CodecVersionPPC;
#else
	return kdxlqt_CodecVersion;
#endif
}


// ************************************************************
// Function name	: dxlqt_CDInitialize
// Description	    : initialization of our decompression record.
//					  here is where we tell the codec how big our 
//					  decompression structure is big. We also list
//                    everything we are capable of..
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : ImageSubCodecDecompressCapabilities *cap
// ************************************************************
pascal ComponentResult 
dxlqt_CDInitialize(dxlqt_Globals glob, ImageSubCodecDecompressCapabilities *cap)
{
	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDInitialize\n",(**glob).instancenum));
	cap->decompressRecordSize = sizeof(dxlqt_DecompressRecord);

#if ASYNC_DECODE 
	cap->canAsync = true;
#else 
	cap->canAsync = false;
#endif 

	return noErr;
}

#ifndef STAND_ALONE


// ************************************************************
// Function name	: dxlqt_CodecRegister
// Description	    : not sure ????
// Return type		: void 
// Argument         : void
// ************************************************************
void dxlqt_CodecRegister(void)
{
	ComponentDescription td;
	ComponentRoutineUPP componentEntryPoint = NewComponentRoutineProc(dxlqt_CDComponentDispatch);

	td.componentType = decompressorComponentType;
	td.componentManufacturer = kAppleManufacturer;
	td.componentFlags = codecInfoDoes32;
	td.componentFlagsMask = 0;
	
	td.componentSubType = FOUR_CHAR_CODE('VP31');
	RegisterComponent(&td,componentEntryPoint, 0, nil,nil, nil);
}

#endif

//** End Open/Close : Initialization / Registration Code 
//******************************************************************


//******************************************************************
//  Get Generic Information about The codec 
//******************************************************************


// ************************************************************
// Function name	: dxlqt_CDGetCompressedImageSize
// Description	    : Returns a rough estimate on the amount of 
//					  size used by a compressed image.
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : ImageDescriptionHandle desc
// Argument         : Ptr data
// Argument         : long dataSize
// Argument         : ICMDataProcRecordPtr dataProc
// Argument         : long *size
// ************************************************************
pascal ComponentResult 
dxlqt_CDGetCompressedImageSize(dxlqt_Globals glob, 
								ImageDescriptionHandle desc, 
								Ptr data, 
								long dataSize, 
								ICMDataProcRecordPtr dataProc, 
								long *size)
{
	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDGetCompressedImageSize\n",(**glob).instancenum));

	if (size == nil) 
		return paramErr;

	*size = (*desc)->width * (*desc)->height+4096;

	return noErr;
}


// ************************************************************
// Function name	: dxlqt_CDGetCodecInfo
// Description	    : Return information about the codec...
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : CodecInfo *info
// ************************************************************
pascal ComponentResult 
dxlqt_CDGetCodecInfo(dxlqt_Globals glob, CodecInfo *info)
{
	OSErr err = noErr;
	DXLQT_LOG(sprintf(logmsg,"Instance:%d CDGetCodecInfo\n",(**glob).instancenum))

	if (info == nil)
    {
		err = paramErr;
	}
	else
    {
		CodecInfo **tempCodecInfo;


#ifdef VP3_COMPRESS
		err = GetComponentResource((Component)(**glob).self, codecInfoResourceType, resid_Comp, (Handle *)&tempCodecInfo);
#else
		// read our information from the resource fork!!!
		err = GetComponentResource((Component)(**glob).self, codecInfoResourceType, resid_Deco, (Handle *)&tempCodecInfo);
#endif

		if (err == noErr)
        {
			*info = **tempCodecInfo;
			DisposeHandle((Handle)tempCodecInfo);
		}
	}

	return err;
}

// ************************************************************
// Function name	: dxlqt_CDEndBand
// Description	    : 
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// Argument         : ImageSubCodecDecompressRecord *drp
// Argument         : OSErr result
// Argument         : long flags
// ************************************************************
pascal ComponentResult 
dxlqt_CDEndBand(dxlqt_Globals glob, ImageSubCodecDecompressRecord *drp, OSErr result, long flags)
{
	return noErr;
}

// ************************************************************
// Function name	: dxlqt_CDQueueStarting
// Description	    : 
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// ************************************************************
pascal ComponentResult 
dxlqt_CDQueueStarting(dxlqt_Globals glob)
{
	return noErr;
}

// ************************************************************
// Function name	: dxlqt_CDQueueStopping
// Description	    : 
// Return type		: pascal ComponentResult 
// Argument         : dxlqt_Globals glob
// ************************************************************
pascal ComponentResult 
dxlqt_CDQueueStopping(dxlqt_Globals glob)
{
	return noErr;
}

//** get information about the codec 
//******************************************************************






