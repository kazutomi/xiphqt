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

#include "OggMuxDS.h"
#include <limits.h>

COggMuxOutputPin::COggMuxOutputPin(TCHAR *pObjectName, COggMux *pFilter, CCritSec *pLock,
								   HRESULT *phr, LPCWSTR pName):
  CBaseOutputPin(pObjectName, (CBaseFilter*) pFilter, pLock, phr, pName)
{
	m_pOggMux = pFilter;
}

STDMETHODIMP COggMuxOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if(riid == IID_IMediaSeeking)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IMediaSeeking*)(m_pOggMux), ppv);
    }
    else return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT COggMuxOutputPin::GetSample(IMediaSample** ppSample, long len)
{
	HRESULT	hr;

	CAutoLock lock(&m_csFilePos);
	__int64	iNewPos = m_iFilePos + len;
	
	hr = CBaseOutputPin::GetDeliveryBuffer(ppSample, &m_iFilePos, &iNewPos, 0);
	if FAILED(hr) return hr;

	(*ppSample)->SetTime(&m_iFilePos, &iNewPos);
	m_iFilePos = iNewPos;

	return NOERROR;
}

HRESULT	COggMuxOutputPin::Active()
{
	CAutoLock lock(&m_csFilePos);
	m_iFilePos = 0;
	return CBaseOutputPin::Active();;
}

//
// DecideBufferSize
//
HRESULT COggMuxOutputPin::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	ALLOCATOR_PROPERTIES CurrentProps;

	if (pAlloc == NULL || pProperties == NULL) return E_POINTER;
    if (pProperties->cBuffers < 1)     pProperties->cBuffers = 1;
	if (pProperties->cbBuffer < 65536) pProperties->cbBuffer = 65536;
    pProperties->cbAlign = 4;
	pProperties->cbPrefix = 0;
	return pAlloc->SetProperties(pProperties,&CurrentProps);
} // DecideBufferSize

HRESULT COggMuxOutputPin::CheckMediaType(const CMediaType* pmt)
{
    if (*(pmt->Type()) != MEDIATYPE_Stream ||
		*(pmt->Subtype()) != MEDIASUBTYPE_Ogg) return VFW_E_TYPE_NOT_ACCEPTED;

	return NOERROR;
}

//
// GetMediaType
//
HRESULT COggMuxOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
  	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pMediaType->SetType(&MEDIATYPE_Stream);
	pMediaType->SetSubtype(&MEDIASUBTYPE_Ogg);
	pMediaType->SetFormatType(&CLSID_NULL);
	pMediaType->SetSampleSize(0);
	pMediaType->SetVariableSize();
	pMediaType->SetTemporalCompression(TRUE);

	return S_OK;
}
