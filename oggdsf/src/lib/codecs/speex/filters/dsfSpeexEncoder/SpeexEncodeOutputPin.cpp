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
#include "Speexencodeoutputpin.h"

SpeexEncodeOutputPin::SpeexEncodeOutputPin(SpeexEncodeFilter* inParentFilter,CCritSec* inFilterLock, vector<CMediaType*> inAcceptableMediaTypes)
	:	AbstractTransformOutputPin(inParentFilter, inFilterLock,NAME("SpeexDecodeOutputPin"), L"Speex Out", 65536, 5, inAcceptableMediaTypes)
{
}

SpeexEncodeOutputPin::~SpeexEncodeOutputPin(void)
{
}

HRESULT SpeexEncodeOutputPin::CreateAndFillFormatBuffer(CMediaType* outMediaType, int inPosition)
{
	if (inPosition == 0) {
		sSpeexFormatBlock* locSpeexFormat = (sSpeexFormatBlock*)outMediaType->AllocFormatBuffer(sizeof(sSpeexFormatBlock));
		//TODO::: Check for null ?

		memcpy((void*)locSpeexFormat, (const void*) &(((SpeexEncodeFilter*)mParentFilter)->mSpeexFormatBlock), sizeof(sSpeexFormatBlock));
		return S_OK;
	} else {
        return S_FALSE;
	}
}
//bool SpeexEncodeOutputPin::FillFormatBuffer(BYTE* inFormatBuffer) {
//	SpeexEncodeFilter* locParentFilter = (SpeexEncodeFilter*)mParentFilter;
//	memcpy((void*)inFormatBuffer, (const void*) &(locParentFilter->mSpeexFormatBlock), sizeof(sSpeexFormatBlock));
//	return true;
//}
//unsigned long SpeexEncodeOutputPin::FormatBufferSize() {
//	return sizeof(sSpeexFormatBlock);
//}
