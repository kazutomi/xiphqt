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
// drvproci.cpp
//
// Purpose: Main message loop for codec.  All messages come through this 
// procedure.
//

#define  ICM_LOADCONFIGFILE (DRV_USER+0x100)

#include "vp31vfw.h"

#define BOGUS_DRIVER_ID		1 

//	if you want to have multiple compressors in a single module add them
//	to this list, for example you can have a video capture module and a
//	video codec in the same module.
static DRIVERPROC DriverProcs[] = { DriverProcVideo, 0};

// All outside calls go through here.
LRESULT CALLBACK _loadds DriverProc( DWORD drivID, HDRVR hD,
									UINT msg, LPARAM x, LPARAM y)
{
	int i = -1;
	DRIVERPROC dp;

	switch( msg)  
	{
	case DRV_LOAD:

		while( dp = DriverProcs[++i])

			if( !dp( drivID, hD, msg, x, y))
				return 0;
			
			return 1;
			
	case DRV_FREE:
		
		while( dp = DriverProcs[++i])
			dp( drivID, hD, msg, x, y);
		
		return 1;
		
	case DRV_OPEN:

		// Open a driver instance.
		// drivID = 0L.
		// x is a far pointer to a zero-terminated string
		// containing the name used to open the driver.
		// y is passed through from the drvOpen call. It is
		// NULL if this open is from the Drivers Applet in control.exe
		// It is LPVIDEO_OPEN_PARMS otherwise.
		// Return 0L to fail the open.
		
		//	if we were opened without an open structure then just
		//	return a phony (non zero) id so the OpenDriver() will work.
		
		if( (LPVOID)y == NULL)
			return BOGUS_DRIVER_ID;
		
		// use first DriverProc that accepts the input type
		
		while( dp = DriverProcs[++i])  
		{
			const LRESULT dw = dp( drivID, hD, msg, x, y);
			if( dw)
				return dw;
		}
		return 0;	// no takers
		
	case DRV_QUERYCONFIGURE:
		return DRV_OK;
		
	case DRV_CONFIGURE:
		return DRV_OK;
		
	case DRV_DISABLE:
	case DRV_ENABLE:
		
		while( dp = DriverProcs[++i])
			dp( drivID, hD, msg, x, y);
		return 1;
		
	case DRV_INSTALL:
	case DRV_REMOVE: return DRV_OK;
		
	default:  
		{
#ifndef UNDER_CE			
			if( !drivID  ||  drivID == BOGUS_DRIVER_ID)
				return DefDriverProc( drivID, hD, msg, x, y);
#endif			
            return ((vfwCodec *)drivID)->driverProc( drivID, hD, msg,x,y);
		}
	}
}


LRESULT CALLBACK DriverProcVideo( DWORD drivID, HDRVR hD,
								 UINT msg, LPARAM x, LPARAM y)
{
	vfwCodec * const pi = (vfwCodec *)drivID;
#	define I (*pi)
	
	BMIH * const inBM  = (BMIH *)x;
	BMIH * const outBM  = (BMIH *)y;
	
	const ICDECOMPRESSEX * const pdxex = (ICDECOMPRESSEX *)x;
	const ICDECOMPRESS * const pdx = (ICDECOMPRESS *)x;
	
    const char *pn = (const char*)x;
	
    DWORD * keyrate = (DWORD *)x; 
	
	switch( msg)  
	{
	case DRV_LOAD:
	case DRV_FREE: 
		return 1;

	case DRV_OPEN: 
		return (LRESULT)I.Open( (ICOPEN *)y);

	case DRV_CLOSE:  
		{
            pi->Close();
            delete pi;
            return 1;
        }

	case ICM_CONFIGURE:
		if( x == -1)
			return I.QueryConfigure()? ICERR_OK:ICERR_UNSUPPORTED;
		else
			return I.Configure( (HWND)x);

	case ICM_ABOUT:
		if( x == -1)
			return I.QueryAbout()? ICERR_OK:ICERR_UNSUPPORTED;
		else
			return I.About( (HWND)x);

	case ICM_GETSTATE: 
		return I.GetState( (LPVOID)x, y);

	case ICM_SETSTATE: 
		return I.SetState( (LPVOID)x, y);

	case ICM_GETINFO: 
		return I.GetInfo( (ICINFO *)x, y);

	case ICM_SET_STATUS_PROC: 
		return I.SetStatusProc((ICSETSTATUSPROC *)x);


	case ICM_GETDEFAULTQUALITY:
        *((DWORD *)x) = 10000;
		return ICERR_OK;
		
        /*********************************************************************
        *********************************************************************/
		
	case ICM_GETDEFAULTKEYFRAMERATE:
		*((DWORD *)x) = 99999;
		return ICERR_OK;

	case ICM_GETQUALITY:
	case ICM_SETQUALITY:
		return ICERR_UNSUPPORTED;

	case ICM_COMPRESS_FRAMES_INFO: 
		return I.cxInfo( (ICCOMPRESSFRAMES *)x);

	case ICM_COMPRESS_QUERY: 
		return I.cxQuery( inBM, outBM);

	case ICM_COMPRESS_BEGIN: 
		return I.cxBegin( inBM, outBM);

	case ICM_COMPRESS_GET_FORMAT: 
		return I.cxGetFormat( inBM, outBM);

	case ICM_COMPRESS_GET_SIZE: 
		return I.cxGetSize( inBM);

	case ICM_COMPRESS: 
		return I.cx( (ICCOMPRESS *)x);

	case ICM_COMPRESS_END: 
		return I.cxEnd();

	case ICM_DECOMPRESS_GET_FORMAT: 
		return I.dxGetFormat( inBM, outBM);

	case ICM_DECOMPRESS_GET_PALETTE: 
		return 1;

	case ICM_DECOMPRESS_QUERY: 
		return I.dxQuery( vfwDXspec( inBM, outBM));

	case ICM_DECOMPRESS_BEGIN: 
		return I.dxBegin( vfwDXspec( inBM, outBM));

	case ICM_DECOMPRESS: 
		return I.dx( vfwDXspec( pdx));

	case ICM_DECOMPRESSEX_QUERY: 
		return I.dxQuery( *pdxex);

	case ICM_DECOMPRESSEX_BEGIN: 
		return I.dxBegin( *pdxex);

	case ICM_DECOMPRESSEX: 
		return I.dx( *pdxex);

	case ICM_DECOMPRESS_END:
	case ICM_DECOMPRESSEX_END: 
		return I.dxEnd();

	case ICM_LOADCONFIGFILE:
		I.ConfigureFromFile( pn);
		return ICERR_OK;

	case ICM_DRAW_QUERY:
	case ICM_DRAW_BEGIN:
	case ICM_DRAW:
	case ICM_DRAW_END: 
		return ICERR_UNSUPPORTED;

	default:

#ifndef UNDER_CE			
		if( msg < DRV_USER)
			return DefDriverProc( drivID, hD, msg, x, y);
		else
#endif
			return ICERR_UNSUPPORTED;
	}
#	undef I
}

