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

#ifndef __COMMON__
#define __COMMON__

#include <streams.h>
#include <limits.h>
#include <winbase.h>
#include "OggDS.h"
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include "OggStream/OggStream.h"
//#include "debug.h"

#define SIZEOF_ARRAY(ar)            (sizeof(ar)/sizeof((ar)[0]))

bool StringToReferenceTime(char* sTime, REFERENCE_TIME* rtTime);
void CopyOggPacket(ogg_packet* dop, ogg_packet* sop);
int GetLCIDFromComment(vorbis_comment* pvc);

typedef struct tagLCID_REC
{
	char		*caption;
	unsigned __int16	id;
} LCID_REC;

static const LCID_REC aLCID[] =
{
	{ "Afrikaans",			0x0036 },
	{ "Albanian",			0x001c },
	{ "Arabic",				0x0001 },
	{ "Basque",				0x002d },
	{ "Belarusian",			0x0023 },
	{ "Bulgarian",			0x0002 },
	{ "Catalan",			0x0003 },
	{ "Chinese",			0x0004 },
	{ "Croatian",			0x001a },
	{ "Czech",				0x0005 },
	{ "Danish",				0x0006 },
	{ "Dutch",				0x0013 },
	{ "English",			0x0009 },
	{ "Estonian",			0x0025 },
	{ "Faeroese",			0x0038 },
	{ "Farsi",				0x0029 },
	{ "Finnish",			0x000b },
	{ "French",				0x000c },
	{ "German",				0x0007 },
	{ "Greek",				0x0008 },
	{ "Hebrew",				0x000d },
	{ "Hungarian",			0x000e },
	{ "Icelandic",			0x000f },
	{ "Indonesian",			0x0021 },
	{ "Italian",			0x0010 },
	{ "Japanese",			0x0011 },
	{ "Korean",				0x0012 },
	{ "Latvian",			0x0026 },
	{ "Lithuanian",			0x0027 },
	{ "Malay",				0x003e },
	{ "Norwegian",			0x0014 },
	{ "Polish",				0x0015 },
	{ "Portuguese",			0x0016 },
	{ "Romanian",			0x0018 },
	{ "Russian",			0x0019 },
	{ "Serbian",			0x001a },
	{ "Slovak",				0x001b },
	{ "Slovenian",			0x0024 },
	{ "Spanish",			0x000a },
	{ "Swahili",			0x0041 },
	{ "Swedish",			0x001d },
	{ "Thai",				0x001e },
	{ "Turkish",			0x001f },
	{ "Ukrainian",			0x0022 }
};

typedef struct
{
	char*				caption;
	char*				tag;
	unsigned __int32	id;
} tSpeaker;

static const tSpeaker arSpeakers[] =
{
	{ "Front Left",				"FL",	0x00000001 },
	{ "Front Right",			"FR",	0x00000002 },
	{ "Front Center",			"FC",	0x00000004 },
	{ "Low Frequency",			"LF",	0x00000008 },
	{ "Back Left",				"BL",	0x00000010 },
	{ "Back Right",				"BR",	0x00000020 },
	{ "Front Left of Center",	"FLC",	0x00000040 },
	{ "Front Right of Center",	"FRC",	0x00000080 },
	{ "Back Center",			"BC",	0x00000100 },
	{ "Side Left",				"SL",	0x00000200 },
	{ "Side Right",				"SR",	0x00000400 },
	{ "Top Center",				"TC",	0x00000800 },
	{ "Top Front Left",			"TFL",	0x00001000 },
	{ "Top Front Center",		"TFC",	0x00002000 },
	{ "Top Front Right",		"TFR",	0x00004000 },
	{ "Top Back Left",			"TBL",	0x00008000 },
	{ "Top Back Center",		"TBC",	0x00010000 },
	{ "Top Back Right",			"TBR",	0x00020000 }
};


#endif