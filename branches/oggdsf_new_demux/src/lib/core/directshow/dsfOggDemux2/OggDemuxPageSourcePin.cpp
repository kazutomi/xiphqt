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
#include "StdAfx.h"
#include ".\oggdemuxpagesourcepin.h"

OggDemuxPageSourcePin::	OggDemuxPageSourcePin(		TCHAR* inObjectName
												,	OggDemuxPageSourceFilter* inParentFilter
												,	CCritSec* inFilterLock
												,	OggPage* inBOSPage)
	:	CBaseOutputPin(			NAME("Ogg Demux Output Pin")
							,	inParentFilter
							,	inFilterLock
							,	&mFilterHR
							,	L"Ogg Stream" )
	,	mBOSPage(inBOSPage)
	,	mIsStreamReady(false)
{

	mBOSAsFormatBlock = (BYTE*)inBOSPage->createRawPageData();
	
}

OggDemuxPageSourcePin::~OggDemuxPageSourcePin(void)
{
	delete[] mBOSAsFormatBlock;
	delete mBOSPage;
}

bool OggDemuxPageSourcePin::acceptOggPage(OggPage* inOggPage)
{
	//TODO:::
	return true;
}
BYTE* OggDemuxPageSourcePin::getBOSAsFormatBlock()
{
	return mBOSAsFormatBlock;
}

unsigned long OggDemuxPageSourcePin::getSerialNo()
{
	return mBOSPage->header()->StreamSerialNo();
}

IOggDecoder* OggDemuxPageSourcePin::getDecoderInterface()
{
	if (mDecoderInterface == NULL) {
		IOggDecoder* locDecoder = NULL;
		if (IsConnected()) {
			IPin* locPin = GetConnected();
			if (locPin != NULL) {
				locPin->QueryInterface(IID_IOggDecoder, (void**)&locDecoder);
			}
		}

		mDecoderInterface = locDecoder;
	}
	return mDecoderInterface;
	
}
HRESULT OggDemuxPageSourcePin::GetMediaType(int inPosition, CMediaType* outMediaType) 
{
	//Put it in from the info we got in the constructor.
	if (inPosition == 0) {
		AM_MEDIA_TYPE locAMMediaType;
		locAMMediaType.majortype = MEDIATYPE_OggPageStream;

		locAMMediaType.subtype = MEDIASUBTYPE_None;
		locAMMediaType.formattype = FORMAT_OggBOSPage;
		locAMMediaType.cbFormat = mBOSPage->pageSize();
		locAMMediaType.pbFormat = getBOSAsFormatBlock();
		locAMMediaType.pUnk = NULL;
	
			
	
		CMediaType locMediaType(locAMMediaType);		
		*outMediaType = locMediaType;
		return S_OK;
	} else {
		return VFW_S_NO_MORE_ITEMS;
	}
}
HRESULT OggDemuxPageSourcePin::CheckMediaType(const CMediaType* inMediaType) {
	if (		(inMediaType->majortype == MEDIATYPE_OggPageStream) 
			&&	(inMediaType->subtype == MEDIASUBTYPE_None)
			&&	(inMediaType->formattype == FORMAT_OggBOSPage)) {
			//&&	(inMediaType->cbFormat == mBOSPage->pageSize()) {

		return S_OK;
	} else {
		return E_FAIL;
	}
}
HRESULT OggDemuxPageSourcePin::DecideBufferSize(IMemAllocator* inoutAllocator, ALLOCATOR_PROPERTIES* inoutInputRequest) 
{
	HRESULT locHR = S_OK;

	ALLOCATOR_PROPERTIES locReqAlloc;
	ALLOCATOR_PROPERTIES locActualAlloc;

	locReqAlloc.cbAlign = 1;
	locReqAlloc.cbBuffer = 65536; //BUFFER_SIZE;
	locReqAlloc.cbPrefix = 0;
	locReqAlloc.cBuffers = NUM_PAGE_BUFFERS; //NUM_BUFFERS;

	locHR = inoutAllocator->SetProperties(&locReqAlloc, &locActualAlloc);

	if (locHR != S_OK) {
		return locHR;
	}
	
	locHR = inoutAllocator->Commit();

	return locHR;

}
