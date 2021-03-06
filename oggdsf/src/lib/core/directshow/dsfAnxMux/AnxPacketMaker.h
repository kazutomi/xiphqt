//===========================================================================
//Copyright (C) 2003, 2004 Zentaro Kavanagh
//
//Copyright (C) 2003, 2004 Commonwealth Scientific and Industrial Research
//   Organisation (CSIRO) Australia
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

#include "StreamHeaders.h"

class AnxPacketMaker
{
public:
	AnxPacketMaker(void);
	~AnxPacketMaker(void);



	static const unsigned long ANX_2_0_ANNODEX_BOS_SIZE = 48;

	static OggPage* makeAnnodexBOS_2_0	(				unsigned long inSerialNo
												,	unsigned short inVersionMajor
												,	unsigned short inVersionMinor
												,	unsigned __int64 inTimebaseNum
												,	unsigned __int64 inTimebaseDenom
												,	const unsigned char* inUTC
											);

	static StampedOggPacket* makeAnxData_2_0	(		unsigned short inVersionMajor
												,	unsigned short inVersionMinor
												,	unsigned __int64 inGranuleRateNum
												,	unsigned __int64 inGranuleDenom
												,	unsigned long inNumSecHeaders
												,	vector<string> inMessageHeaders
											);

	static StampedOggPacket* makeAnxData_2_0 (OggMuxStream* inMuxStream, OggPaginator* inPaginator);
	//static StampedOggPacket* makeAnxData (OggMuxStream* inMuxStream, OggPaginator* inPaginator);

	//static StreamHeaders::eCodecType AnxPacketMaker::IdentifyCodec(OggPacket* inOggPacket);
	//static vector<string> AnxPacketMaker::makeMessageHeaders(OggMuxStream* inMuxStream);
	static vector<string> AnxPacketMaker::makeMessageHeaders(StreamHeaders::eCodecType inCodecType);

	static bool setChecksum(OggPage* inOggPage);

};
