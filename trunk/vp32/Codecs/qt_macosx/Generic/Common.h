//============================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//----------------------------------------------------------------------------

#ifndef _Codec_H_
#define _Codec_H_ 1

//	Pointless?
#ifndef FALSE
	#define FALSE 0
	#define TRUE (!(FALSE))
#endif 
//	end Pointless?

#ifndef DECO_BUILD
	#define DECO_BUILD 1
#endif

#if TARGET_OS_MAC 
#define ASYNC_DECODE 1
#else
#define ASYNC_DECODE 0
#endif 

#define HAS_SETTINGS 0
#define HAS_EQUIV 0
#define HAS_HITTESTDATA 0
#define HAS_EXTRACTANDCOMBINE 0
#define QT_MP 0
#define WANT_REGISTER 0
#define WANT_UNREGISTER 0
#define WANT_TARGET 0
#define HAS_NEWIMAGEBUFFER 0
#define HAS_SUBCODECCALLS 1
#define HAS_RARECALLS	0

#define Codec_Version	1
#define Codec_Revision	1
#define Codec_FixVers	( (Fixed) Codec_Version<<16 + Codec_Revision )

#define	resid_Comp	128
#define resid_Deco	129

#define resid_CompInfo 131
#define resid_CompName 132
#define resid_DecoInfo 133
#define resid_DecoName 134


#endif
