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
// Purpose:  Inner part of codec - tinker interface.
//
/////////////////////////////////////////////////////////////////////////
#define MODNAME	"vp31vfw"
#include "vp31vfw.h" 
#include "regentry.h" 
  
#ifdef VP30_COMPRESS
#include <stdlib.h> 
#include <stdio.h>
#include "math.h"
 
extern "C" 
{  
#include "cclib.h"
}

#define COMPFOURCC MAKEFOURCC('V','P','3','1') 
#define REGACCESS "SOFTWARE\\On2 Technologies\\VFW Encoder/Decoder\\VP31"
#endif
#pragma comment(exestr, "\nON2.COM VERSION VP31VFW 3.2.1.0\n")
  
//*************************************************************************
//*************************************************************************
// statics and globals 
//****************************************************************************
HINSTANCE cfgDLL;


WCHAR*   szDescription() 
{
#ifdef VP30_COMPRESS
    return L"VP31® Compressor";
#endif 
}

WCHAR*   szName()   
{

#ifdef VP30_COMPRESS
	return L"VP31";
#endif

}
  
int vfwCodec::usagecount=0;
/*
#ifdef _DEBUG
    #define DXLVFW_LOGGING
#endif 
*/
#ifdef DXLVFW_LOGGING
  FILE *logfile;
  char logmsg[2048];
  #define DXLVFW_LOG(x) {x;logfile=fopen("vp31vfw.log","a");fprintf(logfile,logmsg);OutputDebugString(logmsg);fclose(logfile);};
#else
  #define DXLVFW_LOG(x) 
#endif 

//****************************************************************************


//****************************************************************************
//****************************************************************************
// Generic or relatively generic functions
//****************************************************************************


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


static enum BITDEPTH getDepth( const BMIH *p)  
{
	
	unsigned int t =  p->biCompression;
	unsigned int c =  p->biBitCount;
	const unsigned long *mp = (const unsigned long *)(p + 1);

	switch(t)
	{
	case BI_RGB:
		if( c == 32)							return DXRGB32;
		if( c == 24)							return DXRGB24;
		if( c == 16)							return DXRGB16_555;
		if( c == 16)							return DXRGB16;
		if( c == 8)								return DXHALFTONE8;
		break;

	case BI_BITFIELDS:
		if( t == BI_BITFIELDS  &&  c == 16  &&  mp[2]==0x1f) 
		{
			if( mp[0]==0x7c00  &&  mp[1]==0x3e0)	return DXRGB16_555;
			if( mp[0]==0xf800  &&  mp[1]==0x7e0)	return DXRGB16_565;
		}
		break;

	case mmioFOURCC('Y','U','Y','2'):
		return DXYUY2;				
	
	case mmioFOURCC('Y','V','U','9'):
		return DXYVU9;				

	case mmioFOURCC('Y','V','1','2'):
		return DXYV12;				

	case mmioFOURCC('U','Y','V','Y'):
		return DXUYVY;				
	}

	return DXRGBNULL;
}
void DrawRandom(char *addr,long w,long h,long p)
{
	int i,j,xp,yp;

	xp = w>>4;
	yp = h>>4;
	char x=rand()*256/RAND_MAX;
	for (i=0;i<h;i++)
		for(j=0;j<w;j++)
			addr[i*p+j]=x;
}


//****************************************************************************
// Save or read settings from registry or file!
//****************************************************************************

#ifdef VP30_COMPRESS

//****************************************************************************
// Default settings for every settable setting!
//****************************************************************************
void getCompConfigDefaultSettings(COMP_CONFIG *CompConfig)
{
	CompConfig->FrameSize						= 0;
	CompConfig->TargetBitRate					= 300;
	CompConfig->FrameRate 						= 25;
	CompConfig->KeyFrameFrequency 				= 120;
	CompConfig->KeyFrameDataTarget 				= 110;      
	CompConfig->Quality 						= 56;
	CompConfig->AllowDF 						= FALSE;
	CompConfig->QuickCompress					= TRUE;    
	CompConfig->AutoKeyFrameEnabled				= TRUE;    
	CompConfig->AutoKeyFrameThreshold			= 90;
	CompConfig->MinimumDistanceToKeyFrame		= 8;
	CompConfig->ForceKeyFrameEvery 				= 120;
    CompConfig->NoiseSensitivity				= 2;        
}

//****************************************************************************
// save the settings to the registry and if present the cfg file on the c drive
//****************************************************************************
void settings2registry(COMP_CONFIG &CompConfig)
{
	char vp3settings[120];

    sprintf( vp3settings, 
		"%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld ",
		CompConfig.TargetBitRate, 
		CompConfig.Quality, 
		CompConfig.KeyFrameDataTarget, 
		CompConfig.AllowDF, 
		CompConfig.QuickCompress,
		CompConfig.AutoKeyFrameEnabled,
		CompConfig.AutoKeyFrameThreshold,
		CompConfig.MinimumDistanceToKeyFrame,
		CompConfig.ForceKeyFrameEvery,
		CompConfig.NoiseSensitivity );

	Registry_SetEntry(vp3settings,REG_CSTRING,500,"strSettings",REGACCESS);
}

//****************************************************************************
// load the settings from the registry or if present the cfg file on the c drive
//****************************************************************************
void registry2settings(COMP_CONFIG &CompConfig)
{
	char vp3settings[120];
	unsigned long size;

	Registry_GetEntry(&vp3settings,REG_CSTRING,&size,"strSettings",REGACCESS);

    sscanf( vp3settings,
		"%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld ",
		&CompConfig.TargetBitRate,
		&CompConfig.Quality,
		&CompConfig.KeyFrameDataTarget,
		&CompConfig.AllowDF, 
		&CompConfig.QuickCompress,
		&CompConfig.AutoKeyFrameEnabled,
		&CompConfig.AutoKeyFrameThreshold,
		&CompConfig.MinimumDistanceToKeyFrame,
		&CompConfig.ForceKeyFrameEvery,
		&CompConfig.NoiseSensitivity);
}

#endif

 

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
	
#ifdef VP30_COMPRESS
	cpi=0;
	yuv_buffer = 0;
#endif
    // read defaults from registry 
    Registry_GetEntry(
        &(showWhiteDots),
        REG_INTEGER,
        &sizeItem,
        "strWhiteDots",
        REGACCESS);
    
    Registry_GetEntry(&PostProcessingLevel,
        REG_INTEGER,
        &sizeItem,
        "strPostProcessingLevel",
        REGACCESS);
    
    Registry_GetEntry(&CPUFree,
        REG_INTEGER,
        &sizeItem,
        "strCPUFree",
        REGACCESS);
    
	registry2settings(CompConfig);
}
//****************************************************************************

//****************************************************************************
// Destructor
//****************************************************************************
vfwCodec::~vfwCodec()  
{
	usagecount--;
} // destructor


//****************************************************************************
// Open : creates a new instance of the compressor/decompressor in memory
//****************************************************************************
vfwCodec *vfwCodec::Open(ICOPEN *op)  
{
    DXLVFW_LOG(sprintf(logmsg ," OPEN Called\n"););
    
    // refuse to open if we are not being opened as a Video compressor
	if( op->fccType != ICTYPE_VIDEO)
	{ op->dwError = ICERR_UNSUPPORTED;  return 0;}
	
	vfwCodec *p =  new vfwCodec;

    if( !p)     { op->dwError = ICERR_MEMORY;  return 0;}
	
	op->dwError = ICERR_OK;	   // return success.
    
    
	
	return p; 
} // open

//****************************************************************************
// Close : Closes an instance of the compressor/decompressor in memory
//****************************************************************************
void vfwCodec::Close()   
{
	DXLVFW_LOG(sprintf(logmsg ," CLOSE Called\n"););
#ifdef DXV_DECOMPRESS
    if( xim ) 
	{
        DXL_DestroyXImage( xim);
        xim = NULL;
    }
    if(vsc) 
	{
        DXL_DestroyVScreen( vsc);        
        vsc = NULL;
    }
#endif 
} // close

//****************************************************************************
// About : Opens the about window (from cfgwin.lib)
//****************************************************************************

int vfwCodec::About( HWND hwnd)  
{
	DXLVFW_LOG(sprintf(logmsg ," ABOUT Called\n"););

	return ICERR_UNSUPPORTED;

} // About


//****************************************************************************
// GetInfo returns information about the compressor/Decompressor
//****************************************************************************
int vfwCodec::GetInfo( ICINFO *icinfo, size_t siz)  
{
	DXLVFW_LOG(sprintf(logmsg ," GETINFO Called\n"););
	const int sz = sizeof( ICINFO);
	
	if( !icinfo)	return sz;
	if( siz < sz)	return 0;
	
	icinfo->dwSize			  = sz;
	icinfo->fccType			  = ICTYPE_VIDEO;
	icinfo->fccHandler		  = COMPFOURCC;
	icinfo->dwVersion		  = 0x00010000;
	icinfo->dwVersionICM	  = ICVERSION; 


#ifdef VP30_COMPRESS
	icinfo->dwFlags			  = VIDCF_TEMPORAL | VIDCF_COMPRESSFRAMES | VIDCF_CRUNCH; 
#endif
	//icinfo->dwFlags			  = VIDCF_QUALITY|VIDCF_TEMPORAL;
	wcscpy( icinfo->szDescription, szDescription());
	wcscpy( icinfo->szName, szName());
	
	return sz;
} // GetInfo

//****************************************************************************
// ConfigureFromFile: 
//****************************************************************************
long vfwCodec::ConfigureFromFile( const char *file2)  
{
	DXLVFW_LOG(sprintf(logmsg ," CONFIGUREFROMFILE Called\n"););
	return 1;
} // ConfigureFromFile


//****************************************************************************
// Configure:  Open our configure dialog box
//****************************************************************************

int vfwCodec::Configure( HWND hwnd)  
{
    DXLVFW_LOG(sprintf(logmsg ," CONFIGURE Called:Instance %x \n",ghModule););

#ifdef VP30_COMPRESS
	DialogBoxParam((struct HINSTANCE__ *) ghModule,"Configure", hwnd, Config_ParamsDlgProc, (LPARAM) &CompConfig);

	settings2registry(CompConfig);
	return ICERR_OK;
#endif

}   // configure                         


//****************************************************************************
// GetState return driver specific state information 
//   if null returns size required for state information
//   otherwise returns current values for compressor settings
//****************************************************************************

int vfwCodec::GetState( LPVOID pv, size_t siz)  
{
	DXLVFW_LOG(sprintf(logmsg ," GETSTATE Called\n"););

#ifdef VP30_COMPRESS
	if( !pv )
	{
		return 500;
	}
	else
	{
		unsigned long fourcc=COMPFOURCC;
		memcpy(pv,(void *) &fourcc,4);
	    sprintf( (char *) pv+4, 
			"%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld ",
			CompConfig.TargetBitRate, 
			CompConfig.Quality, 
			CompConfig.KeyFrameDataTarget, 
			CompConfig.AllowDF, 
			CompConfig.QuickCompress,
			CompConfig.AutoKeyFrameEnabled,
			CompConfig.AutoKeyFrameThreshold,
			CompConfig.MinimumDistanceToKeyFrame,
			CompConfig.ForceKeyFrameEvery,
			CompConfig.NoiseSensitivity);
	}
	return ICERR_OK;
#endif

	return ICERR_OK;

} // GetState

//****************************************************************************
// SetState setup driver specific state information  using passed in void * parm
//   if null use defaults
//****************************************************************************
int vfwCodec::SetState( LPVOID pv, size_t siz)  
{
	DXLVFW_LOG(sprintf(logmsg ," SETSTATE Called\n"););
#ifdef VP30_COMPRESS
	if( !pv  )
	{
		return 500;
	}
	else
	{
		unsigned long fourcc;
		memcpy((void *) &fourcc,pv,4);

		if ( fourcc == COMPFOURCC )
		{
			sscanf( (char *) pv+4,
				"%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld ",
				&CompConfig.TargetBitRate,
				&CompConfig.Quality,
				&CompConfig.KeyFrameDataTarget,
				&CompConfig.AllowDF,
				&CompConfig.QuickCompress,
				&CompConfig.AutoKeyFrameEnabled,
				&CompConfig.AutoKeyFrameThreshold,
				&CompConfig.MinimumDistanceToKeyFrame,
				&CompConfig.ForceKeyFrameEvery,
				&CompConfig.NoiseSensitivity);
		}
	}
	return ICERR_OK;
#endif

	return 0;	
} // SetState



//****************************************************************************
//****************************************************************************
// Decompression Code 
//****************************************************************************


//****************************************************************************
// dxQueryFormat :  Can I handle this format
//****************************************************************************
long vfwCodec::dxQueryFmt( const BMIH *in)  
{
	DXLVFW_LOG(sprintf(logmsg ," DXQUERYFMT Called\n"););

	HRESULT hr;

	hr=ICERR_BADFORMAT;

	if(in->biCompression==COMPFOURCC)
		hr=ICERR_OK;

	if(in->biCompression==MAKEFOURCC('V','P','3','1'))
		hr=ICERR_OK;

	return hr;
} // dxQueryFmt



//****************************************************************************
// dxGetFormat -> What format do I like to decompress to 
//****************************************************************************

long vfwCodec::dxGetFormat( const BMIH *In, BMIH *Out)  
{
	
	DXLVFW_LOG(sprintf(logmsg ," DXGETFORMAT Called\n"););
	const long dw = dxQueryFmt( In);
	if( dw)
		return dw;
	
	// if Out == NULL then, return the size required to hold an output
	// format.	Remember to copy the bitmapinfo header and the colors,
	// but not the decompression format which gives the driver information
	// on how to decompress the data.  
	
	if( !Out)
		return sizeof(BMIH) + In->biClrUsed*sizeof(RGBQUAD);

	// Set up Compression Options
	*Out = *In;
	Out->biSize		   = sizeof(BMIH);
	Out->biCompression = BI_RGB;
	Out->biBitCount    = 24; 
    Out->biPlanes      = 1;
	Out->biSizeImage   = Out->biWidth * Out->biHeight * Out->biBitCount/8;

	return ICERR_OK;
} //dxGetFormat

//****************************************************************************
//  dxQuery : Can I do this decompression
//****************************************************************************
long vfwCodec::dxQuery( const ICDECOMPRESSEX & x)  
{
	
	DXLVFW_LOG(sprintf(logmsg ," DXQUERY Called\n"););
	HRESULT hr;

	const long l = dxQueryFmt( x.lpbiSrc);
	if( l)
		return l;

	if(x.lpbiDst)
	{
		//	we can't clip the source or resize the output
		if(		x.xSrc  
			||  x.ySrc
			||  x.dxSrc != x.lpbiSrc->biWidth
			||  x.dySrc != x.lpbiSrc->biHeight
			||  x.dxSrc != x.dxDst  
			||  x.dySrc != abs( x.dyDst)
			)
			return ICERR_BADPARAM;			

		// convert desired output bitmap to bitdepth
		enum BITDEPTH Selected = getDepth( x.lpbiDst);
		
		if(
			   Selected == DXRGB24 
			|| Selected == DXRGB32
			|| Selected == DXRGB16
			|| Selected == DXRGB16_565
			|| Selected == DXYUY2
		  )
		  return ICERR_OK;
		else
		  return ICERR_BADPARAM;
		
		return hr;

	}
	else
		return ICERR_OK;
	
}  // dxQuery




//****************************************************************************
// dxBegin
//****************************************************************************

long vfwCodec::dxBegin( const ICDECOMPRESSEX & x)  
{
	DXLVFW_LOG(sprintf(logmsg ," DXBEGIN Called:"
	"x.dwFlags:%x, "
	"x.lpbiSrc->biSize:%x, "
	"x.lpbiSrc->biWidth:%x, "
	"x.lpbiSrc->biHeight:%x, "
	"x.lpbiSrc->biPlanes:%x, "
	"x.lpbiSrc->biBitCount:%x, "
	"x.lpbiSrc->biCompression:%x, "
	"x.lpbiSrc->biSizeImage:%x, "
	"x.lpbiSrc->biXPelsPerMeter:%x, "
	"x.lpbiSrc->biYPelsPerMeter:%x, "
	"x.lpbiSrc->biClrUsed:%x, "
	"x.lpbiSrc->biClrImportant:%x, "
	"x.lpSrc:%x, "
    "x.lpbiDst->biSize:%x, "
	"x.lpbiDst->biWidth:%x, "
	"x.lpbiDst->biHeight:%x, "
	"x.lpbiDst->biPlanes:%x, "
	"x.lpbiDst->biBitCount:%x, "
	"x.lpbiDst->biCompression:%x, "
	"x.lpbiDst->biSizeImage:%x, "
	"x.lpbiDst->biXPelsPerMeter:%x, "
	"x.lpbiDst->biYPelsPerMeter:%x, "
	"x.lpbiDst->biClrUsed:%x, "
	"x.lpbiDst->biClrImportant:%x, "
	"x.lpDst:%x, "
	"x.xDst:%x, "
	"x.yDst:%x, "
	"x.dxDst:%x, "
	"x.dyDst:%x, "
	"x.xSrc:%x, "
	"x.ySrc:%x, "
	"x.dxSrc:%x, "
	"x.dySrc:%x\n ",
	x.dwFlags	,
	x.lpbiSrc->biSize	    ,
	x.lpbiSrc->biWidth	    ,
	x.lpbiSrc->biHeight	,
	x.lpbiSrc->biPlanes	,
	x.lpbiSrc->biBitCount,	
	x.lpbiSrc->biCompression	,
	x.lpbiSrc->biSizeImage	,
	x.lpbiSrc->biXPelsPerMeter	,
	x.lpbiSrc->biYPelsPerMeter	,
	x.lpbiSrc->biClrUsed	,
	x.lpbiSrc->biClrImportant,	
	x.lpSrc	,
    x.lpbiDst->biSize	    ,
	x.lpbiDst->biWidth	    ,
	x.lpbiDst->biHeight	,
	x.lpbiDst->biPlanes	,
	x.lpbiDst->biBitCount,	
	x.lpbiDst->biCompression	,
	x.lpbiDst->biSizeImage	,
	x.lpbiDst->biXPelsPerMeter	,
	x.lpbiDst->biYPelsPerMeter	,
	x.lpbiDst->biClrUsed	,
	x.lpbiDst->biClrImportant	,
	x.lpDst	,
	x.xDst	,
	x.yDst	,
	x.dxDst	,
	x.dyDst	,
	x.xSrc	,
	x.ySrc	,
	x.dxSrc	,
	x.dySrc	););

	const long l = dxQuery( x);
	if( l)
		return l;

	//	make sure biSizeImage is set, the decompress code needs it to be
	BMIH &out = *x.lpbiDst;
	if( !out.biSizeImage)
		out.biSizeImage = out.biWidth * out.biHeight * out.biBitCount;
	
	dxEnd();
	
	return ICERR_OK;
	
} //dxBegin


//****************************************************************************
// dx : perform 1 frame worth of decompression
//****************************************************************************
//extern "C" void	DXL_SetXImageCSize(DXL_XIMAGE_HANDLE, int);
long vfwCodec::dx( const ICDECOMPRESSEX & x)  
{
	DXLVFW_LOG(sprintf(logmsg ," DX Called %x\n",xim););

#ifdef DXV_DECOMPRESS 
    if(!xim) 
	{
        DWORD fourCC=x.lpbiSrc->biCompression;

	    xim = DXL_CreateXImageOfType( (BYTE *)x.lpSrc , fourCC);
		if (!xim) 
		{
			DXLVFW_LOG(sprintf(logmsg," Failed DXL_CreateXImage\n"));
			return ICERR_OK;
		}
		xim = DXL_AlterXImage( xim, (BYTE *)x.lpSrc,fourCC,DXRGBNULL,abs(x.lpbiSrc->biWidth),abs(x.lpbiSrc->biHeight));
		if (!xim) 
		{
			DXLVFW_LOG(sprintf(logmsg," Failed DXL_AlterXImage\n"));
			return ICERR_OK;
		}
		DXL_AlterXImageData( xim, (BYTE *)x.lpSrc);

		if(CPUFree)
			vp31_SetParameter(xim,1, CPUFree);
		else
			vp31_SetParameter(xim,0, PostProcessingLevel);
	}
	else   
	{
		DXL_AlterXImageData( xim, (BYTE *)x.lpSrc);
	}
	DXL_SetXImageCSize(xim, x.lpbiSrc->biSizeImage );
 
	if(!(x.dwFlags & (ICDECOMPRESS_HURRYUP|ICDECOMPRESS_UPDATE|ICDECOMPRESS_PREROLL))) 
	{  // we are on schedule
		
		int w = x.lpbiDst->biWidth;
		int h = x.lpbiDst->biHeight;
		int d = x.lpbiDst->biBitCount/8;
		unsigned long offset;
		
		if( h > 0) 					// (upside-down) DIB
		{
			offset = x.xDst + (h - 1 - x.yDst) * w;
			w = -w;
		}
		else 
		{						// (rightside-up) direct screen buffer 
			h = -h;
			offset = x.xDst + (h - x.yDst - x.dyDst) * w;
		}
		if(!vsc) 
		{
			vsc = DXL_CreateVScreen((BYTE *)x.lpDst + offset*d,
				getDepth(x.lpbiDst), w * d ,abs(h) );
			if( !vsc) 
			{
				DXLVFW_LOG(sprintf(logmsg," Failed DXL_CreateVScreen"));
				return ICERR_OK;
			}
			DXL_SetVScreenBlitQuality(vsc,DXBLIT_SAME);
			DXL_AlterVScreen(vsc,(BYTE *)x.lpDst + offset*d,
				getDepth(x.lpbiDst), w * d ,abs(h) );
		}
		else   
		{
			DXL_AlterVScreen(vsc,(BYTE *)x.lpDst + offset*d,
				getDepth(x.lpbiDst), w * d ,abs(h) );

			DXL_SetVScreenBlitQuality(vsc,DXBLIT_SAME);

			DXL_AlterVScreenView(vsc,0,0,abs(w),abs(h));

		}
		if ( vsc && xim)  
		{

			DXL_VScreenSetInfoDotsFlag(vsc, showWhiteDots);
			
			int err=DXL_dxImageToVScreen( xim, vsc);

			DXLVFW_LOG(sprintf(logmsg," DXVDecompress:err=%d,pitch=%d,depth=%d,width=%d,height=%d,iskey=%d \n",
				err,w*d,getDepth(x.lpbiDst),w,h,DXL_IsXImageKeyFrame(xim)); );

		}
	}
	else   
	{
		DXL_dxImageToVScreen( xim, NULL);
		DXLVFW_LOG(sprintf(logmsg," NULL DXVDecompress:iskey=%d \n",DXL_IsXImageKeyFrame(xim)); );
	}
#else 
	int w = x.lpbiDst->biWidth;
	int h = x.lpbiDst->biHeight;
	int d = x.lpbiDst->biBitCount/8;
	DrawRandom((char *) x.lpDst,w,h,w*d);
#endif 
	return ICERR_OK;
} // dx

 
//****************************************************************************
// dxEnd
//****************************************************************************
long vfwCodec::dxEnd()  
{
	DXLVFW_LOG(sprintf(logmsg ," DXEND Called\n"););
	return ICERR_OK;
} // dxEnd



//****************************************************************************
//****************************************************************************
// Compression Code 
//****************************************************************************


//****************************************************************************
// cxGetFormat -> What format do I like to compress to 
//****************************************************************************
long vfwCodec::cxGetFormat( const BMIH *In, BMIH *Out)  
{
	DXLVFW_LOG(sprintf(logmsg ," CXGETFORMAT Called\n"););
	
#ifndef VP30_COMPRESS
	return ICERR_BADFORMAT;
#endif 

	
	const long dw = cxQuery( In, 0);
	if( dw)
		return dw;
	
	// if Out == NULL then, return the size required to hold an output
	// format.	Remember, if you have decompress format information
	// in the header, then make room for it.	
	
	if( !Out)
		return sizeof( BMIH);
	
	*Out = *In;

	Out->biCompression = COMPFOURCC;

	Out->biSizeImage = cxGetSize( In) + 4096; 

	return ICERR_OK;

} //cxGetFormat


//****************************************************************************
//  cxQuery : Can I do this compression
//****************************************************************************
long vfwCodec::cxQuery( const BMIH *in, const BMIH *out)  
{
	DXLVFW_LOG(sprintf(logmsg ," CXQUERY Called\n"););
	
#ifndef VP30_COMPRESS
	return ICERR_UNSUPPORTED;
#endif
 
	if( !in  ||  
		(in->biBitCount != 24 && in->biBitCount!=32 && in->biBitCount!=9))
		return ICERR_BADFORMAT;
	
	if( !out)
		return ICERR_OK;		// only checking input format

	if( out->biCompression != COMPFOURCC 
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
long vfwCodec::cxInfo( const ICCOMPRESSFRAMES *c)  
{
	DXLVFW_LOG(sprintf(logmsg ," CXINFO Called\n"););

	frameRate = (double)c->dwRate/c->dwScale;
#ifdef VP30_COMPRESS

	//Get the target key frame rate
	CompConfig.KeyFrameFrequency = c->lKeyRate;
	CompConfig.TargetBitRate = c->lDataRate * 8 / 1024;

	// Get the target frame rate and tell the compressor what it is.
	CompConfig.FrameRate = (UINT32)(0.5 + ( (double)c->dwRate / (double)c->dwScale));

	if (!CompConfig.TargetBitRate) 
	{
		CompConfig.Quality = 10;
		CompConfig.TargetBitRate = 999999;
	}
	else
		CompConfig.Quality = 56;


#endif 

	return ICERR_OK;

} // cxInfo



//****************************************************************************
// cxBegin : Start compressing
//****************************************************************************
long vfwCodec::cxBegin( const BMIH *In, const BMIH *Out)  
{
	DXLVFW_LOG(sprintf(logmsg ," CXBEGIN Called:Instance %x \n",ghModule););
	
# ifndef VP30_COMPRESS
	return ICERR_UNSUPPORTED;
# endif

	const long dw = cxQuery( In, Out);
	if( dw)
		return dw;

	if(frameRate==0.0)
		frameRate=15.0;

	// new stuff should move elsewhere 
	cxEnd();

# ifdef VP30_COMPRESS
	// Do not allow the very small test frame
	if ( ((DWORD)(In->biWidth) < 128) || ((DWORD)(In->biHeight) < 96) )
	{
		return ICERR_UNSUPPORTED;
	}
	
	// Set the frame size component
	CompConfig.FrameSize = (DWORD)(In->biWidth << 16) + (DWORD)(In->biHeight);
	CompConfig.KeyFrameDataTarget = (int) (sqrtf((float) ((In->biWidth * In->biHeight)/2)));

	// set up for yuv conversion
	yuv_buffer = new unsigned char [In->biWidth * In->biHeight * 3 / 2];
	yuv_config.UVStride = In->biWidth/2;
	yuv_config.UVWidth = In->biWidth/2;
	yuv_config.UVHeight = In->biHeight /2;
	yuv_config.YStride = In->biWidth;
	yuv_config.YWidth = In->biWidth;
	yuv_config.YHeight = In->biHeight;	
	yuv_config.YBuffer = (( char *) yuv_buffer) ;
	yuv_config.UBuffer = (( char *) yuv_buffer + yuv_config.YWidth * yuv_config.YHeight) ;
	yuv_config.VBuffer = (( char *) yuv_config.UBuffer + yuv_config.UVWidth * yuv_config.UVHeight) ;

    if(CompConfig.KeyFrameFrequency==0)
        CompConfig.KeyFrameFrequency = CompConfig.ForceKeyFrameEvery;

    StartEncoder( &cpi,&CompConfig );

	return ICERR_OK;
# endif 


} //cxBegin


//****************************************************************************
// cx : Compress one frame
//****************************************************************************

long vfwCodec::cx( const ICCOMPRESS *c)  
{
	
	DXLVFW_LOG(sprintf(logmsg ," CX Called\n"););

# ifndef VP30_COMPRESS
	return ICERR_UNSUPPORTED;
# endif

	unsigned int is_key;
	is_key =   *c->lpdwFlags & AVIIF_KEYFRAME 
			|| c->dwFlags & ICCOMPRESS_KEYFRAME? 1 : 0;

# ifdef VP30_COMPRESS

	if(is_key)
		ChangeCompressorSetting ( cpi, C_SET_KEY_FRAME, 0 );

	//	Pass the frame data to the encoder.
	if(c->lpbiInput->biBitCount==32)
		CC_RGB32toYV12( (unsigned char *) c->lpInput,
					yuv_config.YWidth,
					yuv_config.YHeight, 
                    (unsigned char *) yuv_config.YBuffer, 
					(unsigned char *) yuv_config.UBuffer, 
					(unsigned char *) yuv_config.VBuffer );
	else
		CC_RGB24toYV12( (unsigned char *) c->lpInput,
					yuv_config.YWidth,
					yuv_config.YHeight, 
                    (unsigned char *) yuv_config.YBuffer, 
					(unsigned char *) yuv_config.UBuffer, 
					(unsigned char *) yuv_config.VBuffer );
	

	c->lpbiOutput->biSizeImage =  EncodeFrameYuv( cpi, &yuv_config, 
				(unsigned char *) c->lpOutput, &is_key );

	*c->lpdwFlags = is_key?AVIIF_KEYFRAME:0;

	if( c->lpckid) *(c->lpckid) = 'C'<<16+'K';	// internal format ID
		
	DXLVFW_LOG(sprintf(logmsg,"%d bytes encoded,%d requested, %d quality, %d \n",
		c->lpbiOutput->biSizeImage ,
		(int) (c->dwFrameSize),
		(int) (c->dwQuality/100),
		(int) is_key));

# endif 

return ICERR_OK;

} // cx 

//****************************************************************************
// cxEnd :: end of compression 
//****************************************************************************
long vfwCodec::cxEnd()  
{
	DXLVFW_LOG(sprintf(logmsg ," CXEND Called\n"););
	
#ifndef VP30_COMPRESS
	return ICERR_OK;
#endif

#ifdef VP30_COMPRESS
	// Kill the encoder
	StopEncoder(&cpi);
	delete [] yuv_buffer;
#endif

	return ICERR_OK;
} // cxEnd

