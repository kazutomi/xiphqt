/////////////////////////////////////////////////////////////////////////// 
//   
// vpvfw.cpp   
//
// Copyright (c) 1998 The Duck Corporation.  All Rights Reserved.
//
// Authors: Blake Sloan, James Bankoski, Dave Silver, Tim Murthy
//
// Purpose:  video for windows codec
//
/////////////////////////////////////////////////////////////////////////

#include "vpvfw.h" 
#include "regentry.h" 
  
#include <stdio.h>
#include <math.h>
 
extern "C"  
{   
#include "vpvfwver.h" 
#include "cclib.h"
}

#include <sstream>
using namespace std;

#pragma comment(exestr, "\nON2.COM VERSION VP31VFW " versionString "\n")
  
//*************************************************************************
//*************************************************************************
// statics and globals 
//****************************************************************************

const long			vfwCodec::compFourCC=mmioFOURCC('V','P','3','1');
const long			vfwCodec::fourCCsSupported[] = { vfwCodec::compFourCC};
const enum BITDEPTH vfwCodec::bitDepthsSupported[] = {DXYV12, DXYUY2, DXRGB32, DXRGB24, DXRGB16, DXRGB16_565};
const WCHAR			vfwCodec::szDescription[] = L"VP31® Compressor";
const WCHAR			vfwCodec::szName[] = L"VP31";
const char *		vfwCodec::registryEntry = "SOFTWARE\\On2 Technologies\\VFW Encoder/Decoder Settings\\VP31";
const DWORD			vfwCodec::defaultQuality = 10000;
const DWORD			vfwCodec::defaultKeyFrameRate = 99999;

// convert compconfig to strings add a space between each and a fourcc. How about 256...
const long vfwCodec::configurationInfoSize = 256; 

// these are here to disable playback of non VP formats  this is an ugly dxv'ism we'll have to correct 
extern "C"
{
	int torq_Init(void){ return 0;}
	int tm1_Init( int i){ return 0;}
	int tm1_Exit(){ return 0;}
	int tm2_Init( int i){ return 0;}
	int tm2_Exit(){ return 0;}
	int tmrt_Init( int i){ return 0;}
	int tmrt_Exit(){ return 0;}
}

//****************************************************************************
#ifdef DXLVFW_LOGGING
ofstream file;
#endif 


//****************************************************************************
//****************************************************************************
// Generic or relatively generic functions
//****************************************************************************


//****************************************************************************
// helper function to default values
//****************************************************************************
void vfwDXspec::defaultValues() 
{
	xDst = 0;
	yDst = 0; 
	xSrc = 0;
	ySrc = 0;
	if( lpbiSrc != 0 ) 
	{
		dxSrc = lpbiSrc->biWidth;
		dySrc = lpbiSrc->biHeight;
	} 
	else
	{
		dxSrc = dySrc = 0;
	}
	if( lpbiDst != 0 ) 
	{
		dxDst = lpbiDst->biWidth;
		dyDst = lpbiDst->biHeight;
	} 
	else
	{
		dxDst = dyDst = 0;
	}
}
//****************************************************************************
// convert from two bitmapinfo headers to a icdecompressex
//****************************************************************************
vfwDXspec::vfwDXspec( BITMAPINFOHEADER *in, BITMAPINFOHEADER *out)
{
	dwFlags = 0;
	lpbiSrc = in;
	lpbiDst = out;
	lpSrc = lpDst = 0;
	defaultValues();
}
//****************************************************************************
// convert from icdecompress to icdecompressex
//****************************************************************************
vfwDXspec::vfwDXspec( const ICDECOMPRESS *p) 
{
	dwFlags = p->dwFlags;
	lpbiSrc = p->lpbiInput;
	lpbiDst = p->lpbiOutput;
	lpSrc = p->lpInput;
	lpDst = p->lpOutput;
	defaultValues();
}


//****************************************************************************
// getDepth : calculate bitdepth from bitmapinfoheader 
//****************************************************************************
BITDEPTH vfwCodec::getDepth
( 
	const BITMAPINFOHEADER *bitmapInfoHeader
)  
{
	
	const BITMAPV4HEADER *bitmapHeader = reinterpret_cast <const BITMAPV4HEADER *> (bitmapInfoHeader);
	unsigned int compression =  bitmapHeader->bV4V4Compression;
	unsigned int bits =  bitmapHeader->bV4BitCount;

	switch(compression)
	{
	case BI_RGB:
		switch(bits)
		{
		case 32:
			return DXRGB32;
		case 24:
			return DXRGB24;
		case 16:
			return DXRGB16_555;
		case 8:
			return DXHALFTONE8;
		};
		return DXRGBNULL;

	case BI_BITFIELDS:
		if( compression == BI_BITFIELDS  
			&& bits == 16  
			&& bitmapHeader->bV4BlueMask==0x1f)			 // 0000000000011111
		{

			if( bitmapHeader->bV4RedMask == 0x7c00       // 0111110000000000
				&&  bitmapHeader->bV4GreenMask == 0x3e0) // 0000001111100000
				return DXRGB16_555;

			if( bitmapHeader->bV4RedMask == 0xf800		 // 1111100000000000
				&&  bitmapHeader->bV4GreenMask ==0x7e0)	 // 0000011111100000
				return DXRGB16_565;
		}
		return DXRGBNULL;

	case mmioFOURCC('Y','U','Y','2'):
		return DXYUY2;				
	
	case mmioFOURCC('Y','V','U','9'):
		return DXYVU9;				

	case mmioFOURCC('Y','V','1','2'):
		return DXYV12;				

	case mmioFOURCC('U','Y','V','Y'):
		return DXUYVY;				

	default:
		return DXRGBNULL;
	}

}

//****************************************************************************
// DrawRandom : Debugging code to draw random crap on screen 
//****************************************************************************
void DrawRandom
(
	char *addr,
	long width,
	long height,
	long p
)
{
	int i,j,xp,yp;

	xp = width>>4;
	yp = height>>4;
	char x=rand()*256/RAND_MAX;
	for (i=0;i<height;i++)
		for(j=0;j<width;j++)
			addr[i*p+j]=x;
}


//****************************************************************************
// Save or read settings from registry or file!
//****************************************************************************


//****************************************************************************
// Default settings for every settable setting!
//****************************************************************************
void vfwCodec::getCompConfigDefaultSettings
(
	COMP_CONFIG &compConfig
)
{ 
	compConfig.FrameSize						= 0;
	compConfig.TargetBitRate					= 300;
	compConfig.FrameRate 						= 25;
	compConfig.KeyFrameFrequency 				= 120;
	compConfig.KeyFrameDataTarget 				= 110;      
	compConfig.Quality 							= 56;
	compConfig.AllowDF 							= FALSE;
	compConfig.QuickCompress					= TRUE;    
	compConfig.AutoKeyFrameEnabled				= TRUE;    
	compConfig.AutoKeyFrameThreshold			= 90;
	compConfig.MinimumDistanceToKeyFrame		= 8;
	compConfig.ForceKeyFrameEvery 				= 120;
    compConfig.NoiseSensitivity					= 2;        
    compConfig.Sharpness						= 1;        
}

//****************************************************************************
// save the settings to the registry and if present the cfg file on the c drive
//****************************************************************************
void vfwCodec::settings2registry
(
	COMP_CONFIG &compConfig
)
{

	ostringstream vpSettings;

	vpSettings 
		<< compConfig.TargetBitRate <<" "
		<< compConfig.Quality <<" "
		<< compConfig.KeyFrameDataTarget << " "
		<< compConfig.AllowDF << " "
		<< compConfig.QuickCompress << " "
		<< compConfig.AutoKeyFrameEnabled << " "
		<< compConfig.AutoKeyFrameThreshold << " "
		<< compConfig.MinimumDistanceToKeyFrame << " "
		<< compConfig.ForceKeyFrameEvery << " "
		<< compConfig.NoiseSensitivity << " "
		<< compConfig.Sharpness << " ";


	Registry_SetEntry( 
		const_cast <char*> (vpSettings.str().c_str()),
		REG_CSTRING,
		500,
		"strSettings",
		const_cast <char *> (registryEntry));
}

//****************************************************************************
// load the settings from the registry or if present the cfg file on the c drive
//****************************************************************************
void vfwCodec::registry2settings
(
	COMP_CONFIG &compConfig
)
{
	char buffer[configurationInfoSize];
	unsigned long size;

	Registry_GetEntry(
		buffer,
		REG_CSTRING,
		&size,
		"strSettings",
		const_cast <char*> (registryEntry));

	istringstream vpSettings(const_cast <char *> (buffer),configurationInfoSize); 

	vpSettings
		>> compConfig.TargetBitRate
		>> compConfig.Quality
		>> compConfig.KeyFrameDataTarget
		>> compConfig.AllowDF
		>> compConfig.QuickCompress
		>> compConfig.AutoKeyFrameEnabled
		>> compConfig.AutoKeyFrameThreshold
		>> compConfig.MinimumDistanceToKeyFrame
		>> compConfig.ForceKeyFrameEvery
		>> compConfig.NoiseSensitivity
		>> compConfig.Sharpness ;
}

 

//****************************************************************************

//****************************************************************************
//****************************************************************************
// Constructor Destructor and Initializers
//****************************************************************************


//****************************************************************************
// Constructor
//******************************************************************
// Do as little as possible during vfwCodec construction.
// Apparently, videdit is sometimes in a delicate state here.

vfwCodec::vfwCodec() 
{
    unsigned long sizeItem;
	xim=NULL;
	vsc=NULL;
	
	cpi=0;
	yuvBuffer = 0;

    // read defaults from registry 
    if(Registry_GetEntry(
        &(showWhiteDots),
        REG_INTEGER,
        &sizeItem,
        "strWhiteDots",
        const_cast <char *> (registryEntry)))
		showWhiteDots = 0;
    
    if(Registry_GetEntry(&postProcessingLevel,
        REG_INTEGER,
        &sizeItem,
        "strPostProcessingLevel",
        const_cast <char *> (registryEntry)))
		postProcessingLevel = 6;
    
    if(Registry_GetEntry(&cpuFree,
        REG_INTEGER,
        &sizeItem,
        "strCPUFree",
        const_cast <char *> (registryEntry)))
		cpuFree = 70;

	// make sure we have good defaults even if the registry entry isn't there
	getCompConfigDefaultSettings(compConfig);
    registry2settings(compConfig);
}
//****************************************************************************


//****************************************************************************
// Destructor
//****************************************************************************
vfwCodec::~vfwCodec()  
{
} // destructor


//****************************************************************************
// Main Driver function of the codec 
//****************************************************************************
LRESULT vfwCodec::driverProc
( 
	DWORD drivID, 
	HDRVR hD,
	UINT msg, 
	LPARAM parm1, 
	LPARAM parm2
)
{
	vfwCodec &thisCodec = *(reinterpret_cast <vfwCodec *>(drivID));
		
	switch( msg)  
	{
	
	case ICM_CONFIGURE:
		// -1 is a question (do we handle ICM_CONFIGURE message?)
		if( parm1 == -1)
			return ICERR_OK;
		else
			return thisCodec.configure( reinterpret_cast <HWND> (parm1));
		
	case ICM_ABOUT:
		// -1 is a question (do we handle ICM_ABOUT message?)
		if( parm1 == -1)
			return ICERR_OK;
		else
			return thisCodec.about( reinterpret_cast <HWND> (parm1));
		
	case ICM_GETSTATE: 
		return thisCodec.getState( reinterpret_cast <LPVOID> (parm1), parm2);
		
	case ICM_SETSTATE: 
		return thisCodec.setState( reinterpret_cast <LPVOID> (parm1), parm2);
		
	case ICM_GETINFO: 
		return thisCodec.getInfo( reinterpret_cast <ICINFO *> (parm1), parm2);
		
	case ICM_SET_STATUS_PROC: 
		return thisCodec.setStatusProc(reinterpret_cast <ICSETSTATUSPROC *>(parm1));
		
	case ICM_GETDEFAULTQUALITY:
        *(reinterpret_cast <DWORD *>(parm1)) = vfwCodec::defaultQuality;
		return ICERR_OK;
		
        /*********************************************************************
        *********************************************************************/
		
	case ICM_GETDEFAULTKEYFRAMERATE:
		*(reinterpret_cast <DWORD *>(parm1)) = vfwCodec::defaultKeyFrameRate;
		return ICERR_OK;
		
	case ICM_GETQUALITY:
	case ICM_SETQUALITY:
		return ICERR_UNSUPPORTED;
		
	case ICM_COMPRESS_FRAMES_INFO: 
		return thisCodec.cxInfo( reinterpret_cast <ICCOMPRESSFRAMES *>(parm1));
		
	case ICM_COMPRESS_QUERY: 
		return thisCodec.cxQuery( 
			reinterpret_cast <BITMAPINFOHEADER *>(parm1), 
			reinterpret_cast <BITMAPINFOHEADER *>(parm2));
		
	case ICM_COMPRESS_BEGIN: 
		return thisCodec.cxBegin( 
			reinterpret_cast <BITMAPINFOHEADER *>(parm1), 
			reinterpret_cast <BITMAPINFOHEADER *>(parm2));
		
	case ICM_COMPRESS_GET_FORMAT: 
		return thisCodec.cxGetFormat( 
			reinterpret_cast <BITMAPINFOHEADER *>(parm1), 
			reinterpret_cast <BITMAPINFOHEADER *>(parm2));
		
	case ICM_COMPRESS_GET_SIZE: 
		return thisCodec.cxGetSize( reinterpret_cast <BITMAPINFOHEADER *>(parm1));
		
	case ICM_COMPRESS: 
		return thisCodec.cx( reinterpret_cast <ICCOMPRESS *> (parm1));
		
	case ICM_COMPRESS_END: 
		return thisCodec.cxEnd();
		
	case ICM_DECOMPRESS_GET_FORMAT: 
		return thisCodec.dxGetFormat( 
			reinterpret_cast <BITMAPINFOHEADER *>(parm1), 
			reinterpret_cast <BITMAPINFOHEADER *>(parm2));
		
	case ICM_DECOMPRESS_QUERY: 
		return thisCodec.dxQuery(vfwDXspec( 
			reinterpret_cast <BITMAPINFOHEADER *>(parm1), 
			reinterpret_cast <BITMAPINFOHEADER *>(parm2)));
		
	case ICM_DECOMPRESS_BEGIN: 
		return thisCodec.dxBegin( vfwDXspec( 
			reinterpret_cast <BITMAPINFOHEADER *>(parm1), 
			reinterpret_cast <BITMAPINFOHEADER *>(parm2)));
		
	case ICM_DECOMPRESS: 
		return thisCodec.dx( vfwDXspec( reinterpret_cast <ICDECOMPRESS *>(parm1)));
		
	case ICM_DECOMPRESSEX_QUERY: 
		return thisCodec.dxQuery( *reinterpret_cast <ICDECOMPRESSEX *>(parm1));
		
	case ICM_DECOMPRESSEX_BEGIN: 
		return thisCodec.dxBegin( *reinterpret_cast <ICDECOMPRESSEX *>(parm1));
		
	case ICM_DECOMPRESSEX: 
		return thisCodec.dx( *reinterpret_cast <ICDECOMPRESSEX *>(parm1));
		
	case ICM_DECOMPRESS_END:
	case ICM_DECOMPRESSEX_END: 
		return thisCodec.dxEnd();
		
	default:
		
		if( msg < DRV_USER)
			return DefDriverProc( drivID, hD, msg, parm1, parm2);
		else
			return ICERR_UNSUPPORTED;
	}
}




//****************************************************************************
// Open : creates a new instance of the compressor/decompressor in memory
//****************************************************************************
vfwCodec *vfwCodec::Open
(
	ICOPEN *openStructure
)  
{
    DXLVFW_LOG( " OPEN Called\n");
     
    // refuse to open if we are not being opened as a Video compressor
	if( openStructure && openStructure->fccType != ICTYPE_VIDEO)
	{ 
		openStructure->dwError = ICERR_UNSUPPORTED;  
		return 0;
	}
	
	vfwCodec *newCodec =  new vfwCodec;

    if( newCodec == 0 )     
	{

		if( openStructure != 0 )
			openStructure->dwError = ICERR_MEMORY;  

		return 0;
	}
	
	if( openStructure != 0 )
		openStructure->dwError = ICERR_OK;	   // return success.

	return newCodec;
} // open 

//****************************************************************************
// Close : Closes an instance of the compressor/decompressor in memory
//****************************************************************************
void vfwCodec::Close(void)  
{
	DXLVFW_LOG(" CLOSE Called\n");

    if( xim != 0 ) 
	{
        DXL_DestroyXImage( xim);
        xim = NULL;
    }

    if( vsc != 0 ) 
	{
        DXL_DestroyVScreen( vsc);        
        vsc = NULL;
    }
} // close


//****************************************************************************
// cxGetSize: get the estimate max size of the  output frames given an input frame
//****************************************************************************
long vfwCodec::cxGetSize
( 
	const BITMAPINFOHEADER *in
)  
{
	// allow 16 bits/pixel + a buffer in case the framesize is minute. We are 
	// thus attempting to handle a maximum header size.  Warning the 
	// compressor is not guaranteed to be less than this number even though it is 
	// supposed to be. 
	return in->biWidth * in->biHeight * 2 + 4096;	
}


//****************************************************************************
// SetStatusProc ; save the status update proc for use by vfw 
//****************************************************************************
long vfwCodec::setStatusProc
( 
	const ICSETSTATUSPROC *s
)  
{
	Status = s->Status;  
	lParam = s->lParam;  
	return ICERR_OK;
}


//****************************************************************************
// About : Opens the about window (from cfgwin.lib)
//****************************************************************************
int vfwCodec::about
( 
	HWND hwnd
)  
{
	DXLVFW_LOG(" ABOUT Called\n");
	return ICERR_UNSUPPORTED;

} // About


//****************************************************************************
// GetInfo returns information about the compressor/Decompressor
//****************************************************************************
int vfwCodec::getInfo
( 
	ICINFO *icinfo, 
	size_t siz
)  
{
	DXLVFW_LOG(" GETINFO Called\n");
	const int sz = sizeof( ICINFO);
	
	// return the size of the info structure we return if a 0 is passed in
	if( icinfo == 0 )	
		return sz;

	// if they don't give us enough room don't fill in anything
	if( siz < sz)	
		return 0;
	
	// here is all the info we have to fill in about our codec
	icinfo->dwSize			  = sz;
	icinfo->fccType			  = ICTYPE_VIDEO;
	icinfo->fccHandler		  = compFourCC;
	icinfo->dwVersion		  = 0x00010000;
	icinfo->dwVersionICM	  = ICVERSION; 
	icinfo->dwFlags			  = VIDCF_TEMPORAL | VIDCF_COMPRESSFRAMES | VIDCF_CRUNCH | VIDCF_FASTTEMPORALC; 
	wcscpy( icinfo->szDescription, szDescription);
	wcscpy( icinfo->szName, szName);
	
	return sz; 
} // GetInfo


//****************************************************************************
// Configure:  Open our configure dialog box
//****************************************************************************
int vfwCodec::configure
( 
	HWND hwnd
)  
{

    DXLVFW_LOG(" CONFIGURE Called:\n");

	DialogBoxParam(hInstance,
		"Configure", 
		hwnd, 
		Config_ParamsDlgProc, 
		reinterpret_cast <LPARAM> (&compConfig));

	settings2registry(compConfig);

	return ICERR_OK;

}   // configure                         


//****************************************************************************
// GetState return driver specific state information 
//   if null returns size required for state information
//   otherwise returns current values for compressor settings
//****************************************************************************

int vfwCodec::getState
( 
	LPVOID configPtr, 
	size_t siz
)  
{
	DXLVFW_LOG(" GETSTATE Called\n");

	if( configPtr == 0 )
	{
		return configurationInfoSize; // size to make setting structure
	}
	else
	{
		ostringstream vpSettings;

		vpSettings 
			<< compFourCC << " "
			<< compConfig.TargetBitRate << " "
			<< compConfig.Quality << " " 
			<< compConfig.KeyFrameDataTarget << " " 
			<< compConfig.AllowDF << " " 
			<< compConfig.QuickCompress << " "
			<< compConfig.AutoKeyFrameEnabled << " "
			<< compConfig.AutoKeyFrameThreshold << " "
			<< compConfig.MinimumDistanceToKeyFrame << " "
			<< compConfig.ForceKeyFrameEvery << " "
			<< compConfig.NoiseSensitivity << " "
			<< compConfig.Sharpness << " ";

		strncpy( 
			reinterpret_cast <char *> (configPtr),
			vpSettings.str().c_str(),
			configurationInfoSize);
	}
	return ICERR_OK;

} // GetState

//****************************************************************************
// SetState setup driver specific state information  using passed in void * parm
//   if null use defaults
//****************************************************************************
int vfwCodec::setState
(	
	LPVOID configPtr, 
	size_t siz
)  
{
	DXLVFW_LOG(" SETSTATE Called\n");

	if( configPtr == 0  )
	{
		return configurationInfoSize; // size to make settings structure
	}
	else
	{
		istringstream vpSettings(const_cast <char *>(configPtr),siz);
		long checkFourCC;

		vpSettings >> checkFourCC;

		if ( checkFourCC == compFourCC )
			vpSettings
				>> compConfig.TargetBitRate
				>> compConfig.Quality
				>> compConfig.KeyFrameDataTarget
				>> compConfig.AllowDF
				>> compConfig.QuickCompress
				>> compConfig.AutoKeyFrameEnabled
				>> compConfig.AutoKeyFrameThreshold
				>> compConfig.MinimumDistanceToKeyFrame
				>> compConfig.ForceKeyFrameEvery
				>> compConfig.NoiseSensitivity
				>> compConfig.Sharpness;
	}
	return ICERR_OK;

} // SetState



//****************************************************************************
//****************************************************************************
// Decompression Code 
//****************************************************************************


//****************************************************************************
// dxQueryFormat :  Can I handle this format
//****************************************************************************
long vfwCodec::dxQueryFmt
( 
	const BITMAPINFOHEADER *in
)  
{
	DXLVFW_LOG(" DXQUERYFMT Called\n");

	// see if we can handle the input 
	for( int i=0;i<sizeof(fourCCsSupported)/sizeof(long);++i)
	{
		if(fourCCsSupported[i] == in->biCompression)
			return ICERR_OK;
	}
	return ICERR_BADFORMAT;

} // dxQueryFmt



//****************************************************************************
// dxGetFormat -> What format do I like to decompress to 
//****************************************************************************

long vfwCodec::dxGetFormat
( 
	const BITMAPINFOHEADER *in, 
	BITMAPINFOHEADER *out
)  
{
	
	DXLVFW_LOG(" DXGETFORMAT Called\n");
	const long dw = dxQueryFmt( in);
	if( dw )
		return dw;
	
	// if out == NULL then, return the size required to hold an output
	// format.	Remember to copy the bitmapinfo header and the colors,
	// but not the decompression format which gives the driver information
	// on how to decompress the data.  
	
	if( out == 0 )
		return sizeof(BITMAPINFOHEADER) + in->biClrUsed*sizeof(RGBQUAD);

	// Set up Compression Options
	*out = *in;
	out->biSize		   = sizeof(BITMAPINFOHEADER);
	out->biCompression = BI_RGB;
	out->biBitCount    = 24; 
    out->biPlanes      = 1;
	out->biSizeImage   = out->biWidth * out->biHeight * out->biBitCount/8;

	return ICERR_OK;
} //dxGetFormat

//****************************************************************************
//  dxQuery : Can I do this decompression
//****************************************************************************
long vfwCodec::dxQuery
( 
	const ICDECOMPRESSEX & dxParms
)  
{
	
	DXLVFW_LOG(" DXQUERY Called\n");

	// check if we can handle the input format if not say so
	const long l = dxQueryFmt( dxParms.lpbiSrc);
	if( l)
		return l;

	// without lpbidst we are just looking to see if there is anything we can 
	// decompress to since we've already checked it return ok
	if(dxParms.lpbiDst)
	{
		//	we can't clip the source or resize the output
		if(		dxParms.xSrc != 0 
			||  dxParms.ySrc != 0
			||  dxParms.dxSrc != dxParms.lpbiSrc->biWidth
			||  dxParms.dySrc != dxParms.lpbiSrc->biHeight
			||  dxParms.dxSrc != dxParms.dxDst  
			||  dxParms.dySrc != abs( dxParms.dyDst)
			)
			return ICERR_BADPARAM;			

		// convert desired output bitmap to bitdepth
		BITDEPTH Selected = getDepth( dxParms.lpbiDst);

		// see if we can handle the conversion from input to output (could probably have 
		// type cast a vector and used a template function to find it. I chose to go w/o
		for( int i=0;i<sizeof(bitDepthsSupported)/sizeof(enum BITDEPTH);++i)
		{
			if(bitDepthsSupported[i] == Selected)
				return ICERR_OK;
		}

		return ICERR_BADPARAM;

	}
	else
		return ICERR_OK;
	
}  // dxQuery




//****************************************************************************
// dxBegin
//****************************************************************************

long vfwCodec::dxBegin
( 
	const ICDECOMPRESSEX & dxParms
)  
{
	DXLVFW_LOG(" DXBEGIN Called:");

	const long l = dxQuery( dxParms);
	if( l)
		return l;

	//	make sure biSizeImage is set, the decompress code needs it to be
	BITMAPINFOHEADER &out = *dxParms.lpbiDst;

	if( out.biSizeImage == 0 )
		out.biSizeImage = out.biWidth * out.biHeight * out.biBitCount;
	
	dxEnd();
	
	return ICERR_OK;
	
} //dxBegin


//****************************************************************************
// dx : perform 1 frame worth of decompression
//****************************************************************************
//extern "C" void	DXL_SetXImageCSize(DXL_XIMAGE_HANDLE, int);
long vfwCodec::dx
( 
	const ICDECOMPRESSEX & dxParms
)  
{
	DXLVFW_LOG(" DX Called "<<xim<<"\n");
 
    if( xim == 0 ) 
	{

        DWORD fourCC=dxParms.lpbiSrc->biCompression;
		
	    xim = DXL_CreateXImageOfType( reinterpret_cast<BYTE *>(dxParms.lpSrc) , fourCC);
		if (!xim) 
		{
			DXLVFW_LOG(" Failed DXL_CreateXImage\n");
			return ICERR_OK;
		}

		xim = DXL_AlterXImage( 
			xim, 
			reinterpret_cast<BYTE *> (dxParms.lpSrc),
			fourCC,
			DXRGBNULL,
			abs(dxParms.lpbiSrc->biWidth),
			abs(dxParms.lpbiSrc->biHeight));

		if (!xim) 
		{
			DXLVFW_LOG(" Failed DXL_AlterXImage\n");
			return ICERR_OK;
		}

		DXL_AlterXImageData( xim, reinterpret_cast<BYTE *>(dxParms.lpSrc));

		if(cpuFree)
			vp31_SetParameter(xim, 1, cpuFree);
		else
			vp31_SetParameter(xim, 0, postProcessingLevel);

	}
	else   
	{
		DXL_AlterXImageData( xim, reinterpret_cast<BYTE *>(dxParms.lpSrc));
	}

	DXL_SetXImageCSize(xim, dxParms.lpbiSrc->biSizeImage );
 
	if((dxParms.dwFlags & (ICDECOMPRESS_HURRYUP|ICDECOMPRESS_UPDATE|ICDECOMPRESS_PREROLL)) == 0 ) 
	{  // we are on schedule
		
		int width = dxParms.lpbiDst->biWidth;
		int height = dxParms.lpbiDst->biHeight;
		int bytes = dxParms.lpbiDst->biBitCount/8;
		unsigned long offset;
		enum BITDEPTH bitDepth = getDepth(dxParms.lpbiDst);

		// insure that yuv surfaces are always rightside up (internal buffer is upside down)
		if(bitDepth==DXYV12||bitDepth == DXYUY2)
			height = -abs(height);

		if( height > 0) 					// (upside-down) DIB
		{
			offset = dxParms.xDst + (height - 1 - dxParms.yDst) * width;
			width = -width;
		}
		else 
		{						// (rightside-up) direct screen buffer 
			height = -height;
			offset = dxParms.xDst + (height - dxParms.yDst - dxParms.dyDst) * width;
		}
		
		if( vsc == 0 ) 
		{
			// ugly dxvism you can't create a vscreen with a width and height
			vsc = DXL_CreateVScreen(
				reinterpret_cast<BYTE *>(dxParms.lpDst) + offset*bytes,
				getDepth(dxParms.lpbiDst), 
				width * bytes ,
				abs(height) );

			if( !vsc) 
			{
				DXLVFW_LOG(" Failed DXL_CreateVScreen");
				return ICERR_OK;
			}

			DXL_SetVScreenBlitQuality(vsc,DXBLIT_SAME);

			// make sure the width and height are set right
			DXL_AlterVScreenView(vsc,0,0,abs(width),abs(height));

		}
		else   
		{
			DXL_AlterVScreen(vsc,
				reinterpret_cast<BYTE *>(dxParms.lpDst) + offset*bytes,
				getDepth(dxParms.lpbiDst), 
				width * bytes ,
				abs(height) );

			DXL_SetVScreenBlitQuality(vsc,DXBLIT_SAME);


		}
		if ( vsc != 0 && xim != 0 )  
		{

			DXL_VScreenSetInfoDotsFlag(vsc, showWhiteDots);
			
			int err=DXL_dxImageToVScreen( xim, vsc);

			DXLVFW_LOG(" DXVDecompress:err=" << err 
				<<" pitch=" << width*bytes
				<<",depth=" <<getDepth(dxParms.lpbiDst)
				<<",width=" <<width
				<<",height="<<height
				<<",iskey=" <<DXL_IsXImageKeyFrame(xim)
				<<"\n")

		}
	}
	else   
	{
		DXL_dxImageToVScreen( xim, NULL);
		DXLVFW_LOG(" NULL DXVDecompress:iskey="<< DXL_IsXImageKeyFrame(xim)<<"\n" );
	}
	return ICERR_OK;
} // dx

 
//****************************************************************************
// dxEnd
//****************************************************************************
long vfwCodec::dxEnd(void)  
{
	DXLVFW_LOG(" DXEND Called\n");
	return ICERR_OK;
} // dxEnd



//****************************************************************************
//****************************************************************************
// Compression Code 
//****************************************************************************


//****************************************************************************
// cxGetFormat -> What format do I like to compress to 
//****************************************************************************
long vfwCodec::cxGetFormat
( 
	const BITMAPINFOHEADER *in, 
	BITMAPINFOHEADER *out
)  
{
	DXLVFW_LOG(" CXGETFORMAT Called\n");
	
	
	const long dw = cxQuery( in, 0);
	if( dw)
		return dw;
	
	// if out == NULL then, return the size required to hold an output
	// format.	Remember, if you have decompress format information
	// in the header, then make room for it.	
	
	if( !out)
		return sizeof( BITMAPINFOHEADER);
	
	*out = *in;

	out->biCompression = compFourCC;

	out->biSizeImage = cxGetSize( in ); 

	return ICERR_OK;

} //cxGetFormat


//****************************************************************************
//  cxQuery : Can I do this compression
//****************************************************************************
long vfwCodec::cxQuery
( 
	const BITMAPINFOHEADER *in, 
	const BITMAPINFOHEADER *out
)  
{
	DXLVFW_LOG(" CXQUERY Called\n");
	
	// can only compress if bitcount is 24 or 32 bit rgb
	if( in == 0  ||  
		(  in->biBitCount != 24 
		&& in->biBitCount != 32 
		&& in->biCompression != mmioFOURCC('I','4','2','0') 
		&& in->biCompression != mmioFOURCC('Y','U','Y','2') ))
		return ICERR_BADFORMAT;
	
	if( out == 0 )
		return ICERR_OK;		// only checking input format

	if( out->biCompression != compFourCC 
		||  out->biWidth  != in->biWidth   // must be 1:1 (no stretch)
		||  out->biHeight != in->biHeight
        ||  (out->biWidth & 0x0F)  // must be multiple of 16
        ||  (out->biHeight & 0x0F)  // must be multiple of 16
		)
		return ICERR_BADFORMAT;

	return ICERR_OK;
} // cxQuery


//****************************************************************************
// cxInfo : Return information about the compressor
//****************************************************************************
long vfwCodec::cxInfo
( 
	const ICCOMPRESSFRAMES *compressParms
)  
{
	DXLVFW_LOG(" CXINFO Called\n");

	//Get the target key frame rate
	compConfig.KeyFrameFrequency = compressParms->lKeyRate;
	compConfig.TargetBitRate = compressParms->lDataRate * 8 / 1024;

	if (compConfig.TargetBitRate == 0 ) 
	{
		compConfig.TargetBitRate = 999999;
	}

	// Get the target frame rate and tell the compressor what it is.
	compConfig.FrameRate = static_cast <UINT32>(0.5 + ( 
		static_cast <double> (compressParms->dwRate) / 
		static_cast <double> (compressParms->dwScale)));

	return ICERR_OK;

} // cxInfo



//****************************************************************************
// cxBegin : Start compressing
//****************************************************************************
long vfwCodec::cxBegin
( 
	const BITMAPINFOHEADER *in, 
	const BITMAPINFOHEADER *out
)  
{
	DXLVFW_LOG(" CXBEGIN Called:Instance\n");
	

	const long dw = cxQuery( in, out);
	if( dw != 0 )
		return dw;

	// new stuff should move elsewhere 
	cxEnd();

	int width=in->biWidth;
	int height=in->biHeight;


	// just in case its been allocated already
	delete [] yuvBuffer;
	yuvBuffer = new unsigned char [width * height * 3 / 2];

	// Set the frame size component
	compConfig.FrameSize = static_cast <DWORD>((width << 16) + height);
	compConfig.KeyFrameDataTarget = static_cast <int> (2*(width + height) / 10) ;
	yuvConfig.UVStride = width/2;
	yuvConfig.UVWidth = width/2;
	yuvConfig.UVHeight = height /2;
	yuvConfig.YStride = width;
	yuvConfig.YWidth = width;
	yuvConfig.YHeight = height;	
	yuvConfig.YBuffer = reinterpret_cast <char *> (yuvBuffer) ;
	yuvConfig.UBuffer = reinterpret_cast <char *> (yuvBuffer + yuvConfig.YWidth * yuvConfig.YHeight);
	yuvConfig.VBuffer = reinterpret_cast <char *> (yuvConfig.UBuffer + yuvConfig.UVWidth * yuvConfig.UVHeight) ;
	
	if(in->biCompression == mmioFOURCC('I','4','2','0') ) 
	{
		yuvConfig.YStride *= -1;
		yuvConfig.UVStride *= -1;
	}

    if(compConfig.KeyFrameFrequency==0)
        compConfig.KeyFrameFrequency = compConfig.ForceKeyFrameEvery;


    StartEncoder( &cpi,&compConfig );

	return ICERR_OK;


} //cxBegin


//****************************************************************************
// cx : Compress one frame
//****************************************************************************

long vfwCodec::cx
( 
	const ICCOMPRESS *compressParms
)  
{
	 
	DXLVFW_LOG(" CX Called\n");

	unsigned int isKey;

	isKey =   *compressParms->lpdwFlags & AVIIF_KEYFRAME 
			|| compressParms->dwFlags & ICCOMPRESS_KEYFRAME? 1 : 0;


	if(isKey == 1)
		ChangeCompressorSetting ( cpi, C_SET_KEY_FRAME, 0 );

	if(compConfig.TargetBitRate == 999999 )
		ChangeCompressorSetting ( cpi, C_SET_FIXED_Q, compConfig.Quality );

	if(compressParms->lpbiInput->biCompression == mmioFOURCC('I','4','2','0') )
	{
		// our input parameters are flipped from what we want and U and V are 
		// reversed from what we normally expect! Correct this.
		yuvConfig.YBuffer = (reinterpret_cast<char *>(compressParms->lpInput) 
			+ yuvConfig.YWidth * (-1+yuvConfig.YHeight)) ;

		yuvConfig.UBuffer = (reinterpret_cast<char *>(compressParms->lpInput) 
			+ yuvConfig.YWidth * yuvConfig.YHeight
			+ yuvConfig.UVWidth * (-1+yuvConfig.UVHeight)) ;

		yuvConfig.VBuffer = (reinterpret_cast<char *>(yuvConfig.UBuffer) 
			+ yuvConfig.UVWidth * yuvConfig.UVHeight) ;
	}
	else if(compressParms->lpbiInput->biCompression == mmioFOURCC('Y','U','Y','2') )
	{
		// call our color conversion code 
		YUY2toYV12(reinterpret_cast<unsigned char *>(compressParms->lpInput)+yuvConfig.YWidth*2*(yuvConfig.YHeight-1),
					yuvConfig.YWidth,
					yuvConfig.YHeight, 
                    reinterpret_cast<unsigned char *>(yuvConfig.YBuffer), 
					reinterpret_cast<unsigned char *>(yuvConfig.UBuffer), 
					reinterpret_cast<unsigned char *>(yuvConfig.VBuffer),
					-yuvConfig.YWidth * 2,
					yuvConfig.YWidth);
	}
	else if(compressParms->lpbiInput->biBitCount==32)
	{
		// call our color conversion code
		CC_RGB32toYV12( reinterpret_cast<unsigned char *>(compressParms->lpInput),
					yuvConfig.YWidth,
					yuvConfig.YHeight, 
                    reinterpret_cast<unsigned char *>( yuvConfig.YBuffer), 
					reinterpret_cast<unsigned char *>(yuvConfig.UBuffer), 
					reinterpret_cast<unsigned char *>(yuvConfig.VBuffer) );

	}
	else
	{
		// call our color conversion code
		CC_RGB24toYV12( reinterpret_cast<unsigned char *>(compressParms->lpInput),
					yuvConfig.YWidth,
					yuvConfig.YHeight, 
                    reinterpret_cast<unsigned char *>(yuvConfig.YBuffer), 
					reinterpret_cast<unsigned char *>(yuvConfig.UBuffer), 
					reinterpret_cast<unsigned char *>(yuvConfig.VBuffer) );
	}
 
	compressParms->lpbiOutput->biSizeImage =  EncodeFrameYuv( cpi, &yuvConfig, 
				reinterpret_cast<unsigned char *>(compressParms->lpOutput), &isKey );

	*compressParms->lpdwFlags = isKey?AVIIF_KEYFRAME:0;

	if( compressParms->lpckid != 0 ) 
		*(compressParms->lpckid) = 'C'<<16+'K';	// internal format ID
		
	DXLVFW_LOG(compressParms->lpbiOutput->biSizeImage<<" bytes encoded,"
		<< compressParms->dwFrameSize << " requested,"
		<<compressParms->dwQuality/100 <<" quality,\n")


	return ICERR_OK;

} // cx 

//****************************************************************************
// cxEnd :: end of compression 
//****************************************************************************
long vfwCodec::cxEnd(void)  
{
	DXLVFW_LOG(" CXEND Called\n");

	// Kill the encoder
	StopEncoder(&cpi);
	delete [] yuvBuffer;
	yuvBuffer = 0;

	return ICERR_OK;
} // cxEnd

