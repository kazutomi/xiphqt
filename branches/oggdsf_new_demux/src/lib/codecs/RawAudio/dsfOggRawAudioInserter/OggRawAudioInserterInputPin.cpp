//===========================================================================
//Copyright (C) 2003, 2004, 2005 Zentaro Kavanagh
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
#include "OggRawAudioInserterInputPin.h"

OggRawAudioInserterInputPin::OggRawAudioInserterInputPin(AbstractTransformFilter* inParentFilter, CCritSec* inFilterLock, AbstractTransformOutputPin* inOutputPin, vector<CMediaType*> inAcceptableMediaTypes)
	:	AbstractTransformInputPin(inParentFilter, inFilterLock, inOutputPin, NAME("OggRawAudioInserterInputPin"), L"PCM In", inAcceptableMediaTypes)
	//,	mWaveFormat(NULL)
	,	mUptoFrame(0)
	,	mNumBufferedBytes(0)
	,	mWorkingBuffer(NULL)
	,	mFrameByteWidth(0)
	,	mFramesPerPacket(0)
	,	mBytesPerPacket(0)
	,	mSentHeaders(false)
{
	//debugLog.open("C:\\temp\\speexenc.log", ios_base::out);
	mWorkingBuffer = new unsigned char[WORKING_BUFFER_SIZE];
}

OggRawAudioInserterInputPin::~OggRawAudioInserterInputPin(void)
{
	//debugLog.close();
	DestroyCodec();

	delete[] mWorkingBuffer;
}


unsigned long OggRawAudioInserterInputPin::identifyFormat()
{
	switch(((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.bitsPerSample) {
		case 8:
			return FMT_U8;
		case 16:
			return FMT_S16_LE;
		default:
			return 0xffffffff;
	}
}
bool OggRawAudioInserterInputPin::makeMainHeader(unsigned char** outBuff, unsigned long* outHeaderSize)
{
	const unsigned long MAIN_HEADER_SIZE = 28;
	unsigned char* retBuff = new unsigned char[MAIN_HEADER_SIZE];

	//ID
	memcpy((void*)retBuff, "PCM     ", 8);
	//Versions
	retBuff[8] = 0;
	retBuff[9] = 0;
	retBuff[10] = 0;
	retBuff[11] = 0;

	//Format
	unsigned long locFormat = identifyFormat();
	iBE_Math::ULongToCharArr(locFormat, &retBuff[12]);

	//Sample Rate
	iBE_Math::ULongToCharArr(((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.samplesPerSec, &retBuff[16]);

	//Sig bits
	retBuff[20] = ((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.bitsPerSample;

	//Num Channels
	retBuff[21] = ((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.numChannels;

	//Frames per packet  (fixed to 1024 = 0x0400)
	retBuff[22] = (mFramesPerPacket & 0xff00) >> 8;
	retBuff[23] = (mFramesPerPacket & 0xff);

	//Extra Headers
	retBuff[24] = 0;
	retBuff[25] = 0;
	retBuff[26] = 0;
	retBuff[27] = 0;

	*outBuff = retBuff;
	*outHeaderSize = MAIN_HEADER_SIZE;

	return true;

}
bool OggRawAudioInserterInputPin::makeCommentHeader(unsigned char** outBuff, unsigned long* outHeaderSize)
{
	char* locVendor = "oggcodecs testbuild";
	unsigned long locVendorLength = strlen(locVendor);
	unsigned long locPacketLength = locVendorLength + 4 + 4 + 1;

	unsigned char* retBuff = new unsigned char[locPacketLength];
	iLE_Math::ULongToCharArr(locVendorLength, retBuff);

	memcpy((void*)(&retBuff[4]), (const void*)locVendor, locVendorLength);

	//No user comments
	iLE_Math::ULongToCharArr(0, &retBuff[locVendorLength + 4]);

	//Framing bit
	retBuff[locVendorLength+8] = 1;

	*outBuff = retBuff;
	*outHeaderSize = locPacketLength;

	return true;


}
//PURE VIRTUALS
HRESULT OggRawAudioInserterInputPin::TransformData(unsigned char* inBuf, long inNumBytes) 
{

	if (!mSentHeaders) {
		mFramesPerPacket = 1024;

		unsigned char* locHeader = NULL;
		unsigned long locHeaderLength = 0;
		mFrameByteWidth = (((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.numChannels * ((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.bitsPerSample) >> 3;
		mBytesPerPacket = mFramesPerPacket * mFrameByteWidth;

		makeMainHeader(&locHeader, &locHeaderLength);

		sendPacket(locHeader, locHeaderLength, true);

		delete[] locHeader;
		locHeader = NULL;
		locHeaderLength = 0;

		makeCommentHeader(&locHeader, &locHeaderLength);

		sendPacket(locHeader, locHeaderLength, true);
		delete[] locHeader;

		mSentHeaders = true;
	}

	//assert(inNumBytes < (WORKING_BUFFER_SIZE - mNumBufferedBytes));
	memcpy((void*)&mWorkingBuffer[mNumBufferedBytes], (const void*)inBuf, inNumBytes);
	mNumBufferedBytes+=inNumBytes;

	//How many full buffers full
	unsigned long locBuffersWorth = mNumBufferedBytes / mBytesPerPacket;

	for (int i = 0; i < locBuffersWorth; i++) {
		sendPacket(&mWorkingBuffer[i*mBytesPerPacket], mBytesPerPacket, false);
	}

	unsigned long locLeftovers = mNumBufferedBytes % mBytesPerPacket;

	//assert((locLeftovers % mFrameByteWidth) == 0);

	//TODO::: This can be improved. For now it's easier to always clear out the buffer.
	//	Though overall it would be better to wait for the next lot of input, though then
	//	we have to worry about ending of stream and the data getting left here, etc.
	sendPacket(&mWorkingBuffer[locBuffersWorth*mBytesPerPacket], locLeftovers, false);


	mNumBufferedBytes = 0;

	return S_OK;
}

HRESULT OggRawAudioInserterInputPin::sendPacket(unsigned char* inPacketData, unsigned long inNumBytes, bool inIsHeader)
{
	if (inIsHeader) {
		mUptoFrame = 0;

	}
	IMediaSample* locSample;
	REFERENCE_TIME locFrameStart = 0;
	REFERENCE_TIME locFrameEnd = 0;
	HRESULT locHR = mOutputPin->GetDeliveryBuffer(&locSample, &locFrameStart, &locFrameEnd, NULL);

	if (FAILED(locHR)) {
		return locHR;
	}	
	
	BYTE* locBuffer = NULL;
	
	//Make our pointers set to point to the samples buffer
	locSample->GetPointer(&locBuffer);
	

	if (locSample->GetSize() >= inNumBytes) {

		memcpy((void*)locBuffer, (const void*)inPacketData, inNumBytes);
		locSample->SetActualDataLength(inNumBytes);

		if (!inIsHeader) {
			//locFrameStart = (mUptoFrame * UNITS) / ((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.samplesPerSec;
			locFrameStart = mUptoFrame;
			mUptoFrame += (inNumBytes / mFrameByteWidth);

			locFrameEnd = mUptoFrame;
			//locFrameEnd = (mUptoFrame * UNITS) / ((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.samplesPerSec;
		} else {
			locFrameStart = 0;
			locFrameEnd = 0;

		}
		locSample->SetMediaTime(&locFrameStart, &locFrameEnd);
		locSample->SetTime(&locFrameStart, &locFrameEnd);

		locSample->SetSyncPoint(TRUE);
		

		{
			CAutoLock locLock(m_pLock);

			//TODO::: Need to propagate error states.
			HRESULT locHR = ((OggRawAudioInserterOutputPin*)(mOutputPin))->mDataQueue->Receive(locSample);						//->DownstreamFilter()->Receive(locSample);
			if (locHR != S_OK) {
				return locHR;
			} else {
				//locThis->debugLog<<"Sample Delivered"<<endl;
				return S_OK;
			}
		}
	} else {
		throw 0;
	}

}
bool OggRawAudioInserterInputPin::ConstructCodec() {
	//mFishInfo.channels = mWaveFormat->nChannels;
	//mFishInfo.format = FISH_SOUND_SPEEX;
	//mFishInfo.samplerate = mWaveFormat->nSamplesPerSec;

	////Change to fill in vorbis format block so muxer can work
	//((SpeexEncodeFilter*)mParentFilter)->mSpeexFormatBlock.numChannels = mWaveFormat->nChannels;
	//((SpeexEncodeFilter*)mParentFilter)->mSpeexFormatBlock.samplesPerSec = mWaveFormat->nSamplesPerSec;
	//
	////
	//
	//mFishSound = fish_sound_new (FISH_SOUND_ENCODE, &mFishInfo);

	//int i = 1;
	////FIX::: Use new API for interleave setting
	//fish_sound_command(mFishSound, FISH_SOUND_SET_INTERLEAVE, &i, sizeof(int));

	//fish_sound_set_encoded_callback (mFishSound, SpeexEncodeInputPin::SpeexEncoded, this);
	//FIX::: Proper return value
	return true;

}
void OggRawAudioInserterInputPin::DestroyCodec() {
	//fish_sound_delete(mFishSound);
	//mFishSound = NULL;
}


//Encoded callback
//int OggRawAudioInserterInputPin::SpeexEncoded (FishSound* inFishSound, unsigned char* inPacketData, long inNumBytes, void* inThisPointer) 
//{
//
//
//	SpeexEncodeInputPin* locThis = reinterpret_cast<SpeexEncodeInputPin*> (inThisPointer);
//	SpeexEncodeFilter* locFilter = reinterpret_cast<SpeexEncodeFilter*>(locThis->m_pFilter);
//	//locThis->debugLog << "SpeexEncoded called with "<<inNumBytes<< " byte of data"<<endl;
//
//	//Time stamps are granule pos not directshow times
//	LONGLONG locFrameStart = locThis->mUptoFrame;
//	LONGLONG locFrameEnd	= locThis->mUptoFrame
//							= fish_sound_get_frameno(locThis->mFishSound);
//
//	
//	//locThis->debugLog << "Stamping packet "<<locFrameStart<< " to "<<locFrameEnd<<endl;
//	//Get a pointer to a new sample stamped with our time
//	IMediaSample* locSample;
//	HRESULT locHR = locThis->mOutputPin->GetDeliveryBuffer(&locSample, &locFrameStart, &locFrameEnd, NULL);
//
//	if (FAILED(locHR)) {
//		//We get here when the application goes into stop mode usually.
//		//locThis->debugLog<<"Getting buffer failed"<<endl;
//		return locHR;
//	}	
//	
//	BYTE* locBuffer = NULL;
//
//	
//	//Make our pointers set to point to the samples buffer
//	locSample->GetPointer(&locBuffer);
//
//	
//
//	if (locSample->GetSize() >= inNumBytes) {
//
//		memcpy((void*)locBuffer, (const void*)inPacketData, inNumBytes);
//		
//		//Set the sample parameters.
//		locThis->SetSampleParams(locSample, inNumBytes, &locFrameStart, &locFrameEnd);
//
//		{
//			CAutoLock locLock(locThis->m_pLock);
//
//			//Add a reference so it isn't deleted en route.
//			//locSample->AddRef();
//			//NO - It alrady has a ref on it.
//
//			//TODO::: Need to propagate error states.
//			HRESULT locHR = ((SpeexEncodeOutputPin*)(locThis->mOutputPin))->mDataQueue->Receive(locSample);						//->DownstreamFilter()->Receive(locSample);
//			if (locHR != S_OK) {
//				//locThis->debugLog<<"Sample rejected"<<endl;
//			} else {
//				//locThis->debugLog<<"Sample Delivered"<<endl;
//			}
//		}
//
//		return 0;
//	} else {
//		throw 0;
//	}
//}


HRESULT OggRawAudioInserterInputPin::SetMediaType(const CMediaType* inMediaType) 
{
	
	if (	(inMediaType->subtype == MEDIASUBTYPE_PCM) &&
			(inMediaType->formattype == FORMAT_WaveFormatEx)) {

		WAVEFORMATEX* locWaveFormat = (WAVEFORMATEX*)inMediaType->pbFormat;
		//TODO::: This needs to change with channel conversion/mapping headers
		((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.numHeaders = 2;
		((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.samplesPerSec = locWaveFormat->nSamplesPerSec;
		((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.numChannels = locWaveFormat->nChannels;
		((OggRawAudioInserterFilter*)mParentFilter)->mOggRawAudioFormatBlock.bitsPerSample = locWaveFormat->wBitsPerSample;

		
	} else {
		//Failed... should never be here !
		throw 0;
	}
	//This is here and not the constructor because we need audio params from the
	// input pin to construct properly.	
	
	ConstructCodec();

	return CBaseInputPin::SetMediaType(inMediaType);

	
	
}