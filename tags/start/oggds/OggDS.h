/*******************************************************************************
*                                                                              *
* This file is part of the Ogg Vorbis DirectShow filter collection             *
*                                                                              *
* Copyright (c) 2001, Tobias Waldvogel                                         *
* All rights reserved.                                                         *
*                                                                              *
* Redistribution and use in source and binary forms, with or without           *
* modification, are permitted provided that the following conditions are met:  *
*                                                                              *
*  - Redistributions of source code must retain the above copyright notice,    *
*    this list of conditions and the following disclaimer.                     *
*                                                                              *
*  - Redistributions in binary form must reproduce the above copyright notice, *
*    this list of conditions and the following disclaimer in the documentation *
*    and/or other materials provided with the distribution.                    *
*                                                                              *
*  - The names of the contributors may not be used to endorse or promote       *
*    products derived from this software without specific prior written        *
*    permission.                                                               *
*                                                                              *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"  *
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE    *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE   *
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE     *
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR          *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF         *
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS     *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN      *
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)      *
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE   *
* POSSIBILITY OF SUCH DAMAGE.                                                  *
*                                                                              *
*******************************************************************************/

// Contains all CLSIDs

#ifndef _OggDS_h_
#define _OggDS_h_

#include <mmreg.h>

// f07e245f-5a1f-4d1e-8bff-dc31d84a55ab
DEFINE_GUID(CLSID_OggSplitter,
0xf07e245f, 0x5a1f, 0x4d1e, 0x8b, 0xff, 0xdc, 0x31, 0xd8, 0x4a, 0x55, 0xab);

// {078C3DAA-9E58-4d42-9E1C-7C8EE79539C5}
DEFINE_GUID(CLSID_OggSplitPropPage,
0x78c3daa, 0x9e58, 0x4d42, 0x9e, 0x1c, 0x7c, 0x8e, 0xe7, 0x95, 0x39, 0xc5);

// 8cae96b7-85b1-4605-b23c-17ff5262b296 
DEFINE_GUID(CLSID_OggMux,
0x8cae96b7, 0x85b1, 0x4605, 0xb2, 0x3c, 0x17, 0xff, 0x52, 0x62, 0xb2, 0x96);

// {AB97AFC3-D08E-4e2d-98E0-AEE6D4634BA4}
DEFINE_GUID(CLSID_OggMuxPropPage,
0xab97afc3, 0xd08e, 0x4e2d, 0x98, 0xe0, 0xae, 0xe6, 0xd4, 0x63, 0x4b, 0xa4);

// {889EF574-0656-4B52-9091-072E52BB1B80}
DEFINE_GUID(CLSID_VorbisEnc,
0x889ef574, 0x0656, 0x4b52, 0x90, 0x91, 0x07, 0x2e, 0x52, 0xbb, 0x1b, 0x80);

// {c5379125-fd36-4277-a7cd-fab469ef3a2f}
DEFINE_GUID(CLSID_VorbisEncPropPage,
0xc5379125, 0xfd36, 0x4277, 0xa7, 0xcd, 0xfa, 0xb4, 0x69, 0xef, 0x3a, 0x2f);

// 02391f44-2767-4e6a-a484-9b47b506f3a4
DEFINE_GUID(CLSID_VorbisDec,
0x02391f44, 0x2767, 0x4e6a, 0xa4, 0x84, 0x9b, 0x47, 0xb5, 0x06, 0xf3, 0xa4);

// {915E0060-918C-4a02-A585-1B912BA71CEE}
DEFINE_GUID(CLSID_VorbisDecPropPage,
0x915e0060, 0x918c, 0x4a02, 0xa5, 0x85, 0x1b, 0x91, 0x2b, 0xa7, 0x1c, 0xee);

// 77983549-ffda-4a88-b48f-b924e8d1f01c
DEFINE_GUID(CLSID_OggDSAboutPage,
0x77983549, 0xffda, 0x4a88, 0xb4, 0x8f, 0xb9, 0x24, 0xe8, 0xd1, 0xf0, 0x1c);

// {D2855FA9-61A7-4db0-B979-71F297C17A04}
DEFINE_GUID(MEDIASUBTYPE_Ogg,
0xd2855fa9, 0x61a7, 0x4db0, 0xb9, 0x79, 0x71, 0xf2, 0x97, 0xc1, 0x7a, 0x4);

// cddca2d5-6d75-4f98-840e-737bedd5c63b
DEFINE_GUID(MEDIASUBTYPE_Vorbis,
0xcddca2d5, 0x6d75, 0x4f98, 0x84, 0x0e, 0x73, 0x7b, 0xed, 0xd5, 0xc6, 0x3b);

// 6bddfa7e-9f22-46a9-ab5e-884eff294d9f
DEFINE_GUID(FORMAT_VorbisFormat,
0x6bddfa7e, 0x9f22, 0x46a9, 0xab, 0x5e, 0x88, 0x4e, 0xff, 0x29, 0x4d, 0x9f);

DEFINE_GUID(MEDIASUBTYPE_EXTPCM,
WAVE_FORMAT_EXTENSIBLE, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71); 


typedef struct tagVORBISFORMAT
{
	WORD  nChannels;
	long  nSamplesPerSec;
	long  nMinBitsPerSec;
	long  nAvgBitsPerSec;
	long  nMaxBitsPerSec;
	float fQuality;
} VORBISFORMAT, *PVORBISFORMAT, FAR *LPVORBISFORMAT;

#endif _OggDS_h_
