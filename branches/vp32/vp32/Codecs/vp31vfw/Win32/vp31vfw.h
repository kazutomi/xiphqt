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


#ifndef tink_h
#define tink_h 1

#include <windows.h>
#include <vfw.h>

#include "duck_dxl.h"

#if VP30_COMPRESS
extern "C"
{
#include "TYPE_ALIASES.h"
#include "vfw_comp_interface.h"
}
#endif


extern BOOL FAR PASCAL Config_ParamsDlgProc(   HWND   hWndDlg,
                                        UINT   Message,
                                        WPARAM wParam,
                                        LPARAM lParam);

extern "C" HMODULE ghModule ;

typedef BITMAPINFOHEADER BMIH;
class vfwDXspec : public ICDECOMPRESSEX {
	void dft() {
		xDst = yDst = xSrc = ySrc = 0;
		if( lpbiSrc) {
			dxSrc = lpbiSrc->biWidth;
			dySrc = lpbiSrc->biHeight;
		} else
			dxSrc = dySrc = 0;
		if( lpbiDst) {
			dxDst = lpbiDst->biWidth;
			dyDst = lpbiDst->biHeight;
		} else
			dxDst = dyDst = 0;
	}
 public:
	vfwDXspec( BMIH *in, BMIH *out) {
		dwFlags = 0;
		lpbiSrc = in;
		lpbiDst = out;
		lpSrc = lpDst = 0;
		dft();
	}
	vfwDXspec( const ICDECOMPRESS *p) {
		dwFlags = p->dwFlags;
		lpbiSrc = p->lpbiInput;
		lpbiDst = p->lpbiOutput;
		lpSrc = p->lpInput;
		lpDst = p->lpOutput;
		dft();
	}
};

LRESULT CALLBACK DriverProcVideo( DWORD drivID, HDRVR hD,
								 UINT uiMessage, LPARAM, LPARAM);


class vfwCodec  
{
	
	LONG ( CALLBACK *Status)( LPARAM lParam, UINT message, LONG l);
	LPARAM	lParam;				// for status procedure
	
#ifdef VP30_COMPRESS
	xCP_INST cpi;
	YUV_INPUT_BUFFER_CONFIG  yuv_config;
	unsigned char *yuv_buffer;
	COMP_CONFIG CompConfig;

    int showWhiteDots;
    int PostProcessingLevel;
    int CPUFree;

#endif

	
    DXL_XIMAGE_HANDLE xim;
    DXL_VSCREEN_HANDLE vsc;
	
	double frameRate;   
	static int usagecount;
	
	vfwCodec();
	
public:
	~vfwCodec();
	
	LRESULT driverProc( DWORD, HDRVR hD, UINT msg, LPARAM x, LPARAM y)  
	{
		return DriverProcVideo( (DWORD)this, hD, msg, x, y);
	}
	
	static vfwCodec *Open( ICOPEN*);
    void Close();
	
    static int QueryAbout() { return 1;}
    unsigned long lastFrameNo;
    int isNextInSequence( char * x, unsigned long * fno);
	int About( HWND);
    int Configure( HWND hwnd);


#ifdef VP30_COMPRESS
    static int QueryConfigure() { return 1;} 
#endif 

#ifndef VP30_COMPRESS
    static int QueryConfigure() { return 0;}
#endif
	
	int GetState( LPVOID, size_t);
	int SetState( LPVOID, size_t);
	
	static int GetInfo( ICINFO*, size_t);
	
	long SetStatusProc( const ICSETSTATUSPROC *s)  
	{
		Status = s->Status;  lParam = s->lParam;  return ICERR_OK;
	}
	
	static long cxQuery( const BMIH *In, const BMIH *Out);
	
	static long cxGetSize( const BMIH *In)  
	{
		return In->biWidth * In->biHeight * 2 +4096;	// allow 8 bits/pixel
	}
	
	static long cxGetFormat( const BMIH *In, BMIH *Out);
	
	
    long cxInfo( const ICCOMPRESSFRAMES*);
	
	long cxBegin( const BMIH *In, const BMIH *Out);
	
	long cx( const ICCOMPRESS*);
	
	long cxEnd();
	
    long ConfigureFromFile( const char *scriptPath);
	
	static long dxQueryFmt( const BMIH *in);
	
	static long dxQuery( const ICDECOMPRESSEX &);
	
	static long dxGetFormat( const BMIH *In, BMIH *Out);

	long dxBegin( const ICDECOMPRESSEX &);
	
	long dx( const ICDECOMPRESSEX &);
	
	long dxEnd();
};

#endif	// tink_h
