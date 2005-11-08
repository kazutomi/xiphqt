//===========================================================================
//Copyright (C) 2003, 2004 Zentaro Kavanagh
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
#include "flacdecodeinputpin.h"

FLACDecodeInputPin::FLACDecodeInputPin(AbstractTransformFilter* inParentFilter, CCritSec* inFilterLock, AbstractTransformOutputPin* inOutputPin, vector<CMediaType*> inAcceptableMediaTypes)
	:	AbstractTransformInputPin(inParentFilter, inFilterLock, inOutputPin, NAME("FLACDecodeInputPin"), L"FLAC In", inAcceptableMediaTypes)
	,	mGotMetaData(false)
	,	mCodecLock(NULL)
	,	mFLACType(FT_UNKNOWN)
	,	mMetadataPacket(NULL)
	,	mSetupState(VSS_SEEN_NOTHING)
	,	mDecodedByteCount(0)
	,	mDecodedBuffer(NULL)
	,	mRateNumerator(RATE_DENOMINATOR)

	,	mUptoFrame(0)

{
	//debugLog.open("G:\\logs\\flacfilter.log", ios_base::out);
	mCodecLock = new CCritSec;			//Deleted in destructor.
	ConstructCodec();

	mDecodedBuffer = new unsigned char[DECODED_BUFFER_SIZE];
}

FLACDecodeInputPin::~FLACDecodeInputPin(void)
{
	//debugLog.close();
	delete mCodecLock;
	delete mMetadataPacket;
	delete mDecodedBuffer;
	
}

STDMETHODIMP FLACDecodeInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IMediaSeeking) {
		*ppv = (IMediaSeeking*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;
	} else if (riid == IID_IOggDecoder) {
		*ppv = (IOggDecoder*)this;
		//((IUnknown*)*ppv)->AddRef();
		return NOERROR;

	}

	return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv); 
}
bool FLACDecodeInputPin::ConstructCodec() 
{
	mFLACDecoder.initCodec();

	return true;
}
void FLACDecodeInputPin::DestroyCodec() 
{

}
STDMETHODIMP FLACDecodeInputPin::NewSegment(REFERENCE_TIME inStartTime, REFERENCE_TIME inStopTime, double inRate) 
{
	CAutoLock locLock(mStreamLock);
	//debugLog<<"New segment "<<inStartTime<<" - "<<inStopTime<<endl;
	mUptoFrame = 0;
	return AbstractTransformInputPin::NewSegment(inStartTime, inStopTime, inRate);
	
}



STDMETHODIMP FLACDecodeInputPin::Receive(IMediaSample* inSample) 
{
	CAutoLock locLock(mStreamLock);

	HRESULT locHR = CheckStreaming();

	if (locHR == S_OK) {
		BYTE* locBuff = NULL;
		locHR = inSample->GetPointer(&locBuff);

		if ((inSample->GetActualDataLength() > 1) && ((locBuff[0] != 0xff) || (locBuff[1] != 0xf8))) {
			//inInputSample->Release();

			//This is a header, so ignore it
			return S_OK;
		}



		if (locHR != S_OK) {
			//TODO::: Do a debug dump or something here with specific error info.
			return locHR;
		} else {
			REFERENCE_TIME locStart = -1;
			REFERENCE_TIME locEnd = -1;
			__int64 locSampleDuration = 0;
			inSample->GetTime(&locStart, &locEnd);

			HRESULT locResult = TransformData(locBuff, inSample->GetActualDataLength());
			if (locResult != S_OK) {
				return S_FALSE;
			}
			if (locEnd > 0) {
				//Can dump it all downstream now	
				IMediaSample* locSample;
				unsigned long locBytesCopied = 0;
				unsigned long locBytesToCopy = 0;

				locStart = convertGranuleToTime(locEnd) - (((mDecodedByteCount / mFLACDecoder.mFrameSize) * UNITS) / mFLACDecoder.mSampleRate);
				do {
					HRESULT locHR = mOutputPin->GetDeliveryBuffer(&locSample, NULL, NULL, NULL);
					if (locHR != S_OK) {
						return locHR;
					}

					BYTE* locBuffer = NULL;
					locHR = locSample->GetPointer(&locBuffer);
				
					if (locHR != S_OK) {
						return locHR;
					}

					locBytesToCopy = ((mDecodedByteCount - locBytesCopied) <= locSample->GetSize()) ? (mDecodedByteCount - locBytesCopied) : locSample->GetSize();
					//locBytesCopied += locBytesToCopy;

					locSampleDuration = (((locBytesToCopy/mFLACDecoder.mFrameSize) * UNITS) / mFLACDecoder.mSampleRate);
					locEnd = locStart + locSampleDuration;

					//Adjust the time stamps for rate and seeking
					REFERENCE_TIME locAdjustedStart = (locStart * RATE_DENOMINATOR) / mRateNumerator;
					REFERENCE_TIME locAdjustedEnd = (locEnd * RATE_DENOMINATOR) / mRateNumerator;
					locAdjustedStart -= m_tStart;
					locAdjustedEnd -= m_tStart;

					__int64 locSeekStripOffset = 0;
					if (locAdjustedEnd < 0) {
						locSample->Release();
					} else {
						if (locAdjustedStart < 0) {
							locSeekStripOffset = (-locAdjustedStart) * mFLACDecoder.mSampleRate;
							locSeekStripOffset *= mFLACDecoder.mFrameSize;
							locSeekStripOffset /= UNITS;
							locSeekStripOffset += (mFLACDecoder.mFrameSize - (locSeekStripOffset % mFLACDecoder.mFrameSize));
							__int64 locStrippedDuration = (((locSeekStripOffset/mFLACDecoder.mFrameSize) * UNITS) / mFLACDecoder.mSampleRate);
							locAdjustedStart += locStrippedDuration;
						}
							

					

						memcpy((void*)locBuffer, (const void*)&mDecodedBuffer[locBytesCopied + locSeekStripOffset], locBytesToCopy - locSeekStripOffset);

						locSample->SetTime(&locAdjustedStart, &locAdjustedEnd);
						locSample->SetMediaTime(&locStart, &locEnd);
						locSample->SetSyncPoint(TRUE);
						locSample->SetActualDataLength(locBytesToCopy - locSeekStripOffset);
						locHR = ((FLACDecodeOutputPin*)(mOutputPin))->mDataQueue->Receive(locSample);
						if (locHR != S_OK) {
							return locHR;
						}
						locStart += locSampleDuration;

					}
					locBytesCopied += locBytesToCopy;

				
				} while(locBytesCopied < mDecodedByteCount);

				mDecodedByteCount = 0;
				
			}
			return S_OK;

		}
	} else {
		//Not streaming - Bail out.
		return S_FALSE;
	}
}




HRESULT FLACDecodeInputPin::TransformData(BYTE* inBuf, long inNumBytes) 
{

	if (CheckStreaming() == S_OK) {
		unsigned char* locInBuff = new unsigned char[inNumBytes];
		memcpy((void*)locInBuff, (const void*)inBuf, inNumBytes);
		OggPacket* locInputPacket = new OggPacket(locInBuff, inNumBytes, false, false);

	
		StampedOggPacket* locStamped = (StampedOggPacket*)mFLACDecoder.decodeFLAC(locInputPacket)->clone();

		FLACDecodeFilter* locFilter = reinterpret_cast<FLACDecodeFilter*>(m_pFilter);

	

		unsigned long locActualSize = locStamped->packetSize();
		//unsigned long locTotalFrameCount = inFrames * locThis->mNumChannels;
		unsigned long locBufferRemaining = DECODED_BUFFER_SIZE - mDecodedByteCount;
		


		//Create a pointer into the buffer		
		//signed short* locShortBuffer = (signed short*)&mDecodedBuffer[mDecodedByteCount];
		
		
		//signed short tempInt = 0;
		//float tempFloat = 0;
		
		//FIX:::Move the clipping to the abstract function

		if (locBufferRemaining >= locActualSize) {
			
			memcpy((void*)&mDecodedBuffer[mDecodedByteCount], (const void*)locStamped->packetData(), locActualSize);

			mDecodedByteCount += locActualSize;
			delete locStamped;
			return S_OK;
		} else {
			throw 0;
		}
	} else {
		DbgLog((LOG_TRACE,1,TEXT("Not streaming")));
		return -1;
	}

















/*

	//What happens when another packet arrives and the other one is still there ?
	//delete mPendingPacket;
	//debugLog<<"decodeData : "<<endl;
	if(!m_bFlushing) {
		unsigned char* locBuff = new unsigned char[inNumBytes];			//Given to packet.
		memcpy((void*)locBuff, (const void*)inBuf, inNumBytes);

		OggPacket* locPacket = new OggPacket(locBuff, inNumBytes, false, false);	//We give this away.

		if (mGotMetaData) {
			StampedOggPacket* locStamped = NULL;
			{
				CAutoLock locCodecLock(mCodecLock);
				//for(unsigned long i = 0; i < mPendingPackets.size(); i++) {
				 locStamped = (StampedOggPacket*)mFLACDecoder.decodeFLAC(locPacket)->clone();			//clone deleted below, locpacket accepted by decoder.
			}

			if (locStamped != NULL) {
				//Do the directshow crap here....

				IMediaSample* locSample;

				HRESULT locHR = mOutputPin->GetDeliveryBuffer(&locSample, NULL, NULL, NULL);
				
				if (FAILED(locHR)) {
					//debugLog<<"Write_Callback : Get deliverybuffer failed. returning abort code."<<endl;
					//		//We get here when the application goes into stop mode usually.
					delete locStamped;
					return S_FALSE;
				}	


				BYTE* locBuffer = NULL;


				//	//Make our pointers set to point to the samples buffer
				locSample->GetPointer(&locBuffer);


				//*** WARNING 4018: Leave this.
				if (locSample->GetSize() >= locStamped->packetSize()) {
					REFERENCE_TIME locFrameStart = (((__int64)(mUptoFrame * UNITS)) / mFLACDecoder.mSampleRate);
					
					//Increment the frame counter
					//NOTE::: The returned packet is stamped 0-numSamples so endTime will be in long range.
					mUptoFrame += (unsigned long)locStamped->endTime();
					
					//	//Make the end frame counter

					REFERENCE_TIME locFrameEnd = (((__int64)(mUptoFrame * UNITS)) / mFLACDecoder.mSampleRate);

					memcpy((void*)locBuffer, (const void*)locStamped->packetData(), locStamped->packetSize());
					SetSampleParams(locSample, locStamped->packetSize(), &locFrameStart, &locFrameEnd);
					HRESULT locHR = ((FLACDecodeOutputPin*)(mOutputPin))->mDataQueue->Receive(locSample);
					if (locHR != S_OK) {
	
					} else {
						//debugLog<<"Write_Callback : Delivery of sample succeeded"<<endl;
					}
				} else {
					delete locStamped;
					throw 0;		//SAMPLE SIZE IS TOO SMALL TO FIT DATA
				}


				delete locStamped;
				return S_OK;
			} else {
				return S_FALSE;
			}
		} else {
			{
				CAutoLock locCodecLock(mCodecLock);
				mGotMetaData = mFLACDecoder.acceptMetadata(locPacket);		//Accepts the packet.
			}
			if (mGotMetaData) {
				return S_OK;
			} else {
				return S_FALSE;
			}
		}

	} else {
		//debugLog<<"decodeData : Filter flushing... bad things !!!"<<endl;
		return S_FALSE;
	}

*/	
}


STDMETHODIMP FLACDecodeInputPin::BeginFlush() {
	CAutoLock locLock(m_pLock);
	
	//debugLog<<"BeginFlush : Calling flush on the codec."<<endl;

	HRESULT locHR = AbstractTransformInputPin::BeginFlush();
	{	//PROTECT CODEC FROM IMPLODING
		CAutoLock locCodecLock(mCodecLock);
		mFLACDecoder.flushCodec();
	}	//END CRITICAL SECTION
	return locHR;
	
}

STDMETHODIMP FLACDecodeInputPin::EndFlush()
{
	CAutoLock locLock(m_pLock);
	
	HRESULT locHR = AbstractTransformInputPin::EndFlush();
	mDecodedByteCount = 0;
	return locHR;
}

STDMETHODIMP FLACDecodeInputPin::EndOfStream(void) {
	CAutoLock locStreamLock(mStreamLock);
	{	//PROTECT CODEC FROM IMPLODING
		CAutoLock locCodecLock(mCodecLock);
		mFLACDecoder.flushCodec();
	}	//END CRITICAL SECTION

	return AbstractTransformInputPin::EndOfStream();
}

HRESULT FLACDecodeInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *outRequestedProps)
{
	outRequestedProps->cbBuffer = FLAC_BUFFER_SIZE;
	outRequestedProps->cBuffers = FLAC_NUM_BUFFERS;
	outRequestedProps->cbAlign = 1;
	outRequestedProps->cbPrefix = 0;

	return S_OK;
}
HRESULT FLACDecodeInputPin::CheckMediaType(const CMediaType *inMediaType)
{
	if (AbstractTransformInputPin::CheckMediaType(inMediaType) == S_OK) {
		if (inMediaType->cbFormat == 4) {
			if (strncmp((char*)inMediaType->pbFormat, "fLaC", 4) == 0) {
				//TODO::: Possibly verify version
				return S_OK;
			}
		} else if (inMediaType->cbFormat > 4) {
			if (strncmp((char*)inMediaType->pbFormat, "\177FLAC", 5) == 0) {
				//TODO::: Possibly verify version
				return S_OK;
			}
		}
	}
	return S_FALSE;
	
}
HRESULT FLACDecodeInputPin::SetMediaType(const CMediaType* inMediaType) {
	//FIX:::Error checking
	//RESOLVED::: Bit better.
	if (CheckMediaType(inMediaType) == S_OK) {
		//((FLACDecodeFilter*)mParentFilter)->setFLACFormatBlock(inMediaType->pbFormat);
		if (inMediaType->cbFormat == 4) {
			if (strncmp((char*)inMediaType->pbFormat, "fLaC", 4) == 0) {
				mFLACType = FT_CLASSIC;
				return S_OK;
			}
		} else if (inMediaType->cbFormat > 4) {
			if (strncmp((char*)inMediaType->pbFormat, "\177FLAC", 5) == 0) {
				mFLACType = FT_OGG_FLAC_1;
				return S_OK;
				
			}
		}
		return S_FALSE;
		
	} else {
		throw 0;
	}

	//if (inMediaType->subtype == MEDIASUBTYPE_FLAC) {
	//	
	//	//Keep the format block
	//	
	//	((FLACDecodeFilter*)mParentFilter)->setFLACFormatBlock((sFLACFormatBlock*)inMediaType->pbFormat);		//Copies the format in the mutator

	//} else {
	//	throw 0;
	//}
	return CBaseInputPin::SetMediaType(inMediaType);
}

LOOG_INT64 FLACDecodeInputPin::convertGranuleToTime(LOOG_INT64 inGranule)
{
		
	return (inGranule * UNITS) / ((FLACDecodeFilter*)mParentFilter)->getFLACFormatBlock()->samplesPerSec;
	
}

LOOG_INT64 FLACDecodeInputPin::mustSeekBefore(LOOG_INT64 inGranule)
{
	//TODO::: Get adjustment from block size info... for now, it doesn't matter if no preroll
	return inGranule;
}
IOggDecoder::eAcceptHeaderResult FLACDecodeInputPin::showHeaderPacket(OggPacket* inCodecHeaderPacket)
{
	const unsigned char MORE_HEADERS_MASK = 128;   //10000000
	switch (mFLACType) {
		case FT_CLASSIC:
			switch (mSetupState) {
				case VSS_SEEN_NOTHING:
					if (strncmp((char*)inCodecHeaderPacket->packetData(), "fLaC", 4) == 0) {
						mSetupState = VSS_SEEN_BOS;
						delete mMetadataPacket;
						mMetadataPacket = inCodecHeaderPacket->clone();
						return IOggDecoder::AHR_MORE_HEADERS_TO_COME;
					}

					//TODO::: new flac format
					mSetupState = VSS_ERROR;
					return IOggDecoder::AHR_INVALID_HEADER;
				case VSS_SEEN_BOS:
					mMetadataPacket->merge(inCodecHeaderPacket);
					if ((inCodecHeaderPacket->packetData()[0] & MORE_HEADERS_MASK) != 0) {
						//Last packet
						mSetupState = VSS_ALL_HEADERS_SEEN; 
						((FLACDecodeFilter*)mParentFilter)->setFLACFormatBlock(mMetadataPacket->packetData());
						mFLACDecoder.acceptMetadata(mMetadataPacket);
						mMetadataPacket = NULL;
				
						//TODO::: Give it to the codec

						return IOggDecoder::AHR_ALL_HEADERS_RECEIVED;
					}

					return IOggDecoder::AHR_MORE_HEADERS_TO_COME;
				default:
					return IOggDecoder::AHR_UNEXPECTED;
			}
		default:
			return IOggDecoder::AHR_INVALID_HEADER;


	}
	//switch (mSetupState) {
	//	case VSS_SEEN_NOTHING:
	//		if (strncmp((char*)inCodecHeaderPacket->packetData(), "fLaC", 4) == 0) {
	//			//TODO::: Possibly verify version
	//			if (fish_sound_decode(mFishSound, inCodecHeaderPacket->packetData(), inCodecHeaderPacket->packetSize()) >= 0) {
	//				mSetupState = VSS_SEEN_BOS;
	//				return IOggDecoder::AHR_MORE_HEADERS_TO_COME;
	//			}
	//		}
	//		return IOggDecoder::AHR_INVALID_HEADER;
	//		
	//		
	//	case VSS_SEEN_BOS:
	//		//The comment packet can't be easily identified in speex.
	//		//Just ignore the second packet we see, and hope fishsound does better.

	//		//if (strncmp((char*)inCodecHeaderPacket->packetData(), "\003vorbis", 7) == 0) {
	//			if (fish_sound_decode(mFishSound, inCodecHeaderPacket->packetData(), inCodecHeaderPacket->packetSize()) >= 0) {
	//				mSetupState = VSS_ALL_HEADERS_SEEN;

	//				fish_sound_command (mFishSound, FISH_SOUND_GET_INFO, &(mFishInfo), sizeof (FishSoundInfo)); 
	//				mBegun = true;
	//		
	//				mNumChannels = mFishInfo.channels;
	//				mFrameSize = mNumChannels * SIZE_16_BITS;
	//				mSampleRate = mFishInfo.samplerate;

	//				return IOggDecoder::AHR_ALL_HEADERS_RECEIVED;
	//			}
	//			
	//			
	//		//}
	//		return IOggDecoder::AHR_INVALID_HEADER;
	//		
	//		
	//
	//	case VSS_ALL_HEADERS_SEEN:
	//	case VSS_ERROR:
	//	default:
	//		return IOggDecoder::AHR_UNEXPECTED;
	//}
}
string FLACDecodeInputPin::getCodecShortName()
{
	return "flac";
}
string FLACDecodeInputPin::getCodecIdentString()
{
	//TODO:::
	return "flac";
}

