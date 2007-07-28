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

#include "stdafx.h"
#include "theoradecoder.h"

TheoraDecoder::TheoraDecoder(void)
	: mFirstPacket(true)
	, mFirstHeader(true)
	, mPacketCount(0)
#ifdef USE_THEORA_EXP
	, mTheoraSetup(NULL)
	, mTheoraState(NULL)
#endif
{
}

TheoraDecoder::~TheoraDecoder(void)
{
}

bool TheoraDecoder::initCodec() 
{
#ifdef USE_THEORA_EXP
	th_comment_init(&mTheoraComment);
	th_info_init(&mTheoraInfo);

#else
	theora_comment_init(&mTheoraComment);
	theora_info_init(&mTheoraInfo);
#endif
	
	return true;
 }



yuv_buffer* TheoraDecoder::decodeTheora(StampedOggPacket* inPacket) {		//Accepts packet and deletes it.

	if (mPacketCount < 3) {
		decodeHeader(inPacket);		//Accepts header and deletes it.

		if (mPacketCount == 3) {
#ifdef USE_THEORA_EXP
			mTheoraState = th_decode_alloc(&mTheoraInfo, mTheoraSetup);
#else
			theora_decode_init(&mTheoraState, &mTheoraInfo);
#endif
			//TODO::: Post processing http://people.xiph.org/~tterribe/doc/libtheora-exp/theoradec_8h.html#a1

		}

		
		return NULL;
	} else {
		//if (mFirstPacket) {
		//	theora_decode_init(&mTheoraState, &mTheoraInfo);
		//	mFirstPacket = false;
		//}
		if ((inPacket->packetSize() > 0) && ((inPacket->packetData()[0] & 128) != 0)) {
			//Ignore header packets
			delete inPacket;
			return NULL;
		}

		ogg_packet* locOldPack = simulateOldOggPacket(inPacket);		//Accepts the packet and deletes it.

#ifdef USE_THEORA_EXP

		th_decode_packetin(mTheoraState, locOldPack, NULL);
#else
		theora_decode_packetin(&mTheoraState, locOldPack);
#endif
		delete locOldPack->packet;
		delete locOldPack;
		
#ifdef USE_THEORA_EXP
		th_decode_ycbcr_out(mTheoraState, mYCbCrBuffer);

		//TODO:::
		//This is slightly nasty for now... since changing the return type
		// will screw with other stuff
		//
		//Need to probably use the theora-exp buffer type and change all the
		// uses of yuv_buffer to handle this, and avoid assumptions about
		// the relative size of the Y and U and V buffers

		if (	!	(	(mYCbCrBuffer[1].width == mYCbCrBuffer[2].width)
					&&	(mYCbCrBuffer[1].height == mYCbCrBuffer[2].height)
					&&	(mYCbCrBuffer[1].ystride == mYCbCrBuffer[2].ystride)
					)) {
			throw "Not 4:2:0 - OOTheora needs fixing";
		}

		mYUVBuffer.y_width = mYCbCrBuffer[0].width;
		mYUVBuffer.y_height = mYCbCrBuffer[0].height;
		mYUVBuffer.y_stride = mYCbCrBuffer[0].ystride;
		mYUVBuffer.y = mYCbCrBuffer[0].data;
		mYUVBuffer.uv_width = mYCbCrBuffer[1].width;
		mYUVBuffer.uv_height = mYCbCrBuffer[1].height;
		mYUVBuffer.uv_stride = mYCbCrBuffer[1].ystride;
		mYUVBuffer.u = mYCbCrBuffer[1].data;
		mYUVBuffer.v = mYCbCrBuffer[2].data;

#else
		//Ignore return value... always returns 0 (or crashes :)
		theora_decode_YUVout(&mTheoraState, &mYUVBuffer);
#endif
		
		return &mYUVBuffer;
	}

}

ogg_packet* TheoraDecoder::simulateOldOggPacket(StampedOggPacket* inPacket) {		//inPacket is accepted and deleted.
	const unsigned char NOT_USED = 0;
	ogg_packet* locOldPacket = new ogg_packet;		//Returns this... the caller is responsible for it.
	if (mFirstHeader) {
		locOldPacket->b_o_s = 1;
		mFirstHeader = false;
	} else {
		locOldPacket->b_o_s = NOT_USED;
	}
	locOldPacket->e_o_s = NOT_USED;
	locOldPacket->bytes = inPacket->packetSize();
	locOldPacket->granulepos = inPacket->endTime();
	locOldPacket->packet = inPacket->packetData();
	locOldPacket->packetno = NOT_USED;
	
	//Set this to NULL do it doesn't get deleted by the destructor we are about invoke.
	inPacket->setPacketData(NULL);
	delete inPacket;

	return locOldPacket;		//Gives a poitner to the caller.
}

bool TheoraDecoder::isKeyFrame(StampedOggPacket* inPacket)
{
	const unsigned char KEY_FRAME_FLAG = 0x40;
	if ((inPacket->packetSize() > 0) && (inPacket->packetData() != NULL)) {
		return ((inPacket->packetData()[0] & KEY_FRAME_FLAG) == KEY_FRAME_FLAG) ? false : true;
	} else {
		return false;
	}
}
bool TheoraDecoder::decodeHeader(StampedOggPacket* inHeaderPacket) 
{		//inHeaderPacket is accepted and deleted.
	//TODO::: Error handling

	ogg_packet* locOldPack = simulateOldOggPacket(inHeaderPacket);		//Accepts packet and deletes it.

#ifdef USE_THEORA_EXP
	th_decode_headerin(&mTheoraInfo, &mTheoraComment, &mTheoraSetup, locOldPack);
#else
	theora_decode_header(&mTheoraInfo, &mTheoraComment, locOldPack);
#endif

	delete locOldPack->packet;
	delete locOldPack;
	mPacketCount++;
	return true;
}