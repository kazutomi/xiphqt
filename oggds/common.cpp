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

#include "common.h"

static wchar_t	wzCnvtBuf[256];
static char		szCnvtBuf[256];

// Used to convert the time from the chapter comments
bool StringToReferenceTime(char* sTime, REFERENCE_TIME* rtTime)
{
	int				i = strlen(sTime);
	REFERENCE_TIME	mult = 1;

	*rtTime = 0;

	while (i > 0)
	{
		i--;

		if (sTime[i] == '.')
		{
			if (*rtTime > 10000000 || mult > 10000000)		// if already bigger than one second in reference time
				return false;								// the '.' is unexpected
			*rtTime *= (REFERENCE_TIME)10000000;
			*rtTime /= mult;
			mult = 10000000;
		}
		else if (sTime[i] == ':')
		{
			if (mult < 10000000)
			{
				*rtTime *= 10000000;
				mult = 600000000;
			}
			else if (mult == 1000000000)
				mult = 600000000;
			else if (mult == 60000000000)
				mult = 36000000000;
			else return false;
		}
		else
		{
			if (sTime[i] < '0' || sTime[i] > '9')
				return false;

			*rtTime += (sTime[i] - '0') * mult;
			mult *= 10;
		}
	}
	return true;
}

void CopyOggPacket(ogg_packet* dop, ogg_packet* sop)
{
	dop->packetno	= sop->packetno;
	dop->granulepos	= sop->granulepos;
	dop->b_o_s		= sop->b_o_s;
	dop->e_o_s		= sop->e_o_s;
	dop->bytes		= sop->bytes;
	dop->packet		= (BYTE*) malloc(sop->bytes);
	memcpy(dop->packet, sop->packet, sop->bytes);
}

// Searches for the "LANGUAGE" tag and returns the corresponding CLSID
int GetLCIDFromComment(vorbis_comment* pvc)
{
	char*	pLang = vorbis_comment_query(pvc, "LANGUAGE", 0);
	if (!pLang)
		return 0;

	if (strcmp(pLang, "off") == 0)
		return 9; // Means English. If it would be 0 media player doesn't select the dummy stream

	char	sStr[128];
	int		iLen = strlen(pLang);

	// Bring to standard form: First letter capital
	strcpy(sStr, pLang);
	if ((*sStr >= 'a') && (*sStr <= 'z'))
		*sStr -= 'a' - 'A';
	if (iLen>1) _strlwr(sStr+1);

	for (int i=0; i<SIZEOF_ARRAY(aLCID); i++)
	{
		int iLCIDLen = strlen(aLCID[i].caption);
		if (iLCIDLen <= iLen)
			if (strncmp(aLCID[i].caption, sStr, iLCIDLen) == 0)
				return aLCID[i].id;
	}

	return 0;
}
