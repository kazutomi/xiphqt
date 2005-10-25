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

#pragma once

#include "Theoradecoderdllstuff.h"
#include "theoradecodeoutputpin.h"
#include "theoradecodeinputpin.h"

#include <libilliCore/iBE_Math.h>
#include <math.h>
//#include "DSStringer.h"
#include "TheoraDecoder.h"
#include <fstream>
using namespace std;
class TheoraDecodeFilter 
	//:	public CVideoTransformFilter
	:	public CTransformFilter

{
public:
	friend class TheoraDecodeInputPin;
	TheoraDecodeFilter(void);
	virtual ~TheoraDecodeFilter(void);

	//COM Creator Function
	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr);

	//CTransfrom filter pure virtuals
	virtual HRESULT CheckInputType(const CMediaType* inMediaType);
	virtual HRESULT CheckTransform(const CMediaType* inInputMediaType, const CMediaType* inOutputMediaType);
	virtual HRESULT DecideBufferSize(IMemAllocator* inAllocator, ALLOCATOR_PROPERTIES* inPropertyRequest);
	virtual HRESULT GetMediaType(int inPosition, CMediaType* outOutputMediaType);
	virtual HRESULT Transform(IMediaSample* inInputSample, IMediaSample* outOutputSample);

	//Overrides
	virtual HRESULT Receive(IMediaSample* inSample);

	virtual HRESULT SetMediaType(PIN_DIRECTION inDirection, const CMediaType* inMediaType);
	virtual HRESULT NewSegment(REFERENCE_TIME inStart, REFERENCE_TIME inEnd, double inRate);
	//virtual BOOL ShouldSkipFrame(IMediaSample* inSample);
	virtual CBasePin* TheoraDecodeFilter::GetPin(int inPinNo);
	//Helpers
	sTheoraFormatBlock* getTheoraFormatBlock();
	void setTheoraFormat(BYTE* inFormatBlock);
protected:

	static const unsigned long THEORA_IDENT_HEADER_SIZE = 42;
	virtual void ResetFrameCount();
	void FillMediaType(CMediaType* outMediaType, unsigned long inSampleSize);
	bool FillVideoInfoHeader(VIDEOINFOHEADER* inFormatBuffer);
	bool SetSampleParams(IMediaSample* outMediaSample, unsigned long inDataSize, REFERENCE_TIME* inStartTime, REFERENCE_TIME* inEndTime, BOOL inIsSync);
	unsigned long mHeight;
	unsigned long mWidth;
	unsigned long mFrameSize;
	unsigned long mFrameCount;
	unsigned long mYOffset;
	unsigned long mXOffset;
	__int64 mFrameDuration;
	bool mBegun;
	TheoraDecoder* mTheoraDecoder;
	
	vector<StampedOggPacket*> mBufferedPackets;

	HRESULT TheoraDecoded (yuv_buffer* inYUVBuffer, IMediaSample* outSample, bool inIsKeyFrame, REFERENCE_TIME inStart, REFERENCE_TIME inEnd);


	__int64 mSeekTimeBase;
	__int64 mLastSeenStartGranPos;
	//Format Block
	sTheoraFormatBlock* mTheoraFormatInfo;
	fstream debugLog;
};
