#ifndef vpvfw_h
#define vpvfw_h 1

#include <windows.h>
#include <vfw.h>
#include <mmsystem.h>

extern "C"
{
#include "TYPE_ALIASES.h"
#include "vfw_comp_interface.h"
#include "duck_dxl.h"
}


//*********************************************************
// this debug logging code
#ifdef _DEBUG
    #define DXLVFW_LOGGING
#endif 

// ugly logging code.  
#ifdef DXLVFW_LOGGING
	#include <fstream>
	#include <sstream>
	using namespace std;
	extern ofstream file;
	#define DXLVFW_LOG(x) \
	  if(file.is_open()) \
		{ \
			ostringstream logmsg; \
			logmsg<<x; \
			file<<logmsg.str(); \
			OutputDebugString(logmsg.str().c_str()); \
		} \
	  else                \
		{ \
			ostringstream logmsg; \
			logmsg<<x; \
			OutputDebugString(logmsg.str().c_str()); \
		};
#else
	#define DXLVFW_LOG(x) 
#endif 
//*********************************************************

extern BOOL FAR PASCAL Config_ParamsDlgProc( HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam );
extern "C" HINSTANCE hInstance;        


//********************************************************
// vfwdxspec class used for converting to icdecompressex from
//   icdecompress or from 2 bitmap info parameters
class vfwDXspec : 
	public ICDECOMPRESSEX 
{
private:
	void defaultValues();
public:
    vfwDXspec( BITMAPINFOHEADER *in, BITMAPINFOHEADER *out);
	vfwDXspec( const ICDECOMPRESS *p) ;
};


//********************************************************
// main codec class describing the video for windows codec
class vfwCodec  
{
public:

	// factory functions
	static vfwCodec *Open( ICOPEN*);
    void Close();
	vfwCodec();
	~vfwCodec();

	// main exposed message handler
	static LRESULT driverProc( DWORD, HDRVR hD, UINT msg, LPARAM x, LPARAM y);

	// public so that the configuration dialog windows can access them( sorry )
	static void getCompConfigDefaultSettings(COMP_CONFIG &compConfig);
	static const char *registryEntry;

private:

	// private data
	static const long compFourCC;
	static const enum BITDEPTH bitDepthsSupported[];
	static const long fourCCsSupported[];
	static const WCHAR szDescription[];
	static const WCHAR szName[];
	static const DWORD defaultQuality;
	static const DWORD defaultKeyFrameRate;
	static const long configurationInfoSize;
	
	// call back function for set status proc (not sure we do the right thing here)
	LONG ( CALLBACK *Status)( LPARAM lParam, UINT message, LONG l);
	LPARAM	lParam;	
	
	// vp compressor specific stuff 
	xCP_INST cpi;			
	YUV_INPUT_BUFFER_CONFIG  yuvConfig;
	unsigned char *yuvBuffer;
	COMP_CONFIG compConfig;

	// dxv specific stuff 
    int showWhiteDots;
    int postProcessingLevel;
    int cpuFree;
    DXL_XIMAGE_HANDLE xim;
    DXL_VSCREEN_HANDLE vsc;

	//*********************************************************************************
	// Member Functions 
	static enum BITDEPTH getDepth( const BITMAPINFOHEADER *p);
	long setStatusProc( const ICSETSTATUSPROC *s);
	static int about( HWND);
	static int getInfo( ICINFO*, size_t);

	//*********************************************************
	// functions used only for compressing

	// helper functions for configuration
	static void settings2registry(	COMP_CONFIG &compConfig);
	static void registry2settings(	COMP_CONFIG &compConfig);

	// gather information about the compressor
	static long cxQuery( const BITMAPINFOHEADER *in, const BITMAPINFOHEADER *out);
	static long cxGetSize( const BITMAPINFOHEADER *in);
	static long cxGetFormat( const BITMAPINFOHEADER *in, BITMAPINFOHEADER *out);
    long cxInfo( const ICCOMPRESSFRAMES*);

	// set up and configure the compressor 
    int configure( HWND hwnd);
	int getState( LPVOID, size_t);
	int setState( LPVOID, size_t);

	// main compression functions
	long cxBegin( const BITMAPINFOHEADER *in, const BITMAPINFOHEADER *out);
	long cx( const ICCOMPRESS*);
	long cxEnd();

	//**********************************************************

	//**********************************************************
	// functions used only in decompression

	// gather information about the decompressor
	static long dxQueryFmt( const BITMAPINFOHEADER *in);
	static long dxQuery( const ICDECOMPRESSEX &);
	static long dxGetFormat( const BITMAPINFOHEADER *in, BITMAPINFOHEADER *out);

	// main decompression functions
	long dxBegin( const ICDECOMPRESSEX &);
	long dx( const ICDECOMPRESSEX &);
	long dxEnd();
	//**********************************************************
};

#endif	// vpvfw_h
