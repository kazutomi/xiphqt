//===========================================================================
//Copyright (C) 2003-2006 Zentaro Kavanagh
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//- Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//- Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//- Neither the name of Zentaro Kavanagh nor the names of contributors 
//  may be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//===========================================================================
#pragma once

#include <vorbis/codec.h>

class VorbisDecoder
{
public:
	VorbisDecoder(void);
	~VorbisDecoder(void);



	enum eVorbisResult {
		VORBIS_DATA_OK = 0,
		VORBIS_HEADER_OK,
		VORBIS_COMMENT_OK,
		VORBIS_CODEBOOK_OK,
		VORBIS_ERROR_MIN = 64,
		VORBIS_HEADER_BAD,
		VORBIS_COMMENT_BAD,
		VORBIS_CODEBOOK_BAD,
		VORBIS_SYNTH_FAILED,
		VORBIS_BLOCKIN_FAILED
	};

	//bool setDecodeParams(SpeexDecodeSettings inSettings);
	eVorbisResult decodePacket(		const unsigned char* inPacket
								,	unsigned long inPacketSize
								,	short* outSamples
								,	unsigned long inOutputBufferSize
								,	unsigned long* outNumSamples); 

	int numChannels()	{	return mNumChannels;	}
	int sampleRate()	{	return mSampleRate;		}
protected:
	eVorbisResult decodeHeader();
	eVorbisResult decodeComment();
	eVorbisResult decodeCodebook();

	short clip16(int inVal)		{	return (short)((inVal > 32767) ? (32767) : ((inVal < -32768) ? (-32768) : (inVal)));	}
	unsigned long mPacketCount;

	int mNumChannels;
	int mSampleRate;
	//int mNumFrames;
	//int mNumExtraHeaders;
	//bool mIsVBR;

	vorbis_info mVorbisInfo;
	vorbis_comment mVorbisComment;
	vorbis_dsp_state mVorbisState;
	vorbis_block mVorbisBlock;

	ogg_packet mWorkPacket;


};
