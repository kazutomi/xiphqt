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

#include "OggSplitterDS.h"
#include <atlbase.h>

//
//
// COggSplitOutputPin methods ...
//
COggSplitOutputPin::COggSplitOutputPin(TCHAR *pObjectName, COggSplitter *pFilter,
					   CCritSec *pLock, HRESULT *phr, LPCWSTR pName):
	CBaseOutputPin(pObjectName, pFilter, pLock, phr, pName)
{
	m_pOggSplitter = pFilter;
};

STDMETHODIMP COggSplitOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IMediaSeeking)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IMediaSeeking*)this, ppv);
    }
    if(riid == IID_IPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPropertyBag*)(this), ppv);
    }
    if(riid == IID_IPersistPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPersistPropertyBag*)(this), ppv);
    }
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}


HRESULT	COggSplitOutputPin::Active()
{
    HRESULT hr;
    if (m_pOggSplitter->IsActive()) return S_FALSE;
    if (!IsConnected()) return NOERROR;
    hr = CBaseOutputPin::Active();
	if FAILED(hr) return hr;

	m_rtLastPos = _I64_MIN;
    return NOERROR;
}
	
HRESULT COggSplitOutputPin::Inactive()
{
    return CBaseOutputPin::Inactive();
}

HRESULT COggSplitOutputPin::BeginFlush()
{
	return DeliverBeginFlush();
}

HRESULT	COggSplitOutputPin::EndFlush()
{
	m_rtLastPos = _I64_MIN;
	return DeliverEndFlush();
}

HRESULT COggSplitOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    CAutoLock lock(&m_pOggSplitter->m_csFilter);

    if (iPosition<0) return E_INVALIDARG;
    if (iPosition>0) return VFW_S_NO_MORE_ITEMS;
	*pMediaType = m_pStream->m_mt; 
	return NOERROR;
}

HRESULT COggSplitOutputPin::CheckMediaType(const CMediaType* pmt)
{
	if (*pmt != m_pStream->m_mt) return E_FAIL;
	return	NOERROR;
}

HRESULT COggSplitOutputPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	ALLOCATOR_PROPERTIES CurrentProps;

	if (pProp->cBuffers < 1)
		pProp->cBuffers = 1;
	if (pProp->cbBuffer < m_pStream->m_iBufferSize)
		pProp->cbBuffer = m_pStream->m_iBufferSize;
	pProp->cbAlign  = 1;
	pProp->cbPrefix = 0;

	return pAlloc->SetProperties(pProp, &CurrentProps);
}

HRESULT	COggSplitOutputPin::SendVorbisHeaderPackets(ogg_packet *pVorbisHeader)
{
	// Send the Vorbis header packets
	HRESULT			hr;
	REFERENCE_TIME	start = 0;
	REFERENCE_TIME	stop = 0;
	IMediaSample	*pSample;
	BYTE			*buffer;
		
	for (int i=0; i<3; i++)
	{
		hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
		if FAILED(hr) return hr;
		pSample->GetPointer(&buffer);
		if (pVorbisHeader[i].bytes > pSample->GetSize()) return VFW_E_BUFFER_OVERFLOW;
		memcpy(buffer, pVorbisHeader[i].packet, pVorbisHeader[i].bytes);
		pSample->SetActualDataLength(pVorbisHeader[i].bytes);
		pSample->SetTime(&start, &start);
		hr = Deliver(pSample);
		pSample->Release();
		if FAILED(hr) return hr;
	}

	return NOERROR;
}

void COggSplitOutputPin::SetLastPos(REFERENCE_TIME *prtLastPos)
{
	CAutoLock lock(&m_csLastPos);
	m_rtLastPos = *prtLastPos;
};

void COggSplitOutputPin::GetLastPos(REFERENCE_TIME *prtLastPos)
{
	CAutoLock lock(&m_csLastPos);
	*prtLastPos = m_rtLastPos;
};


HRESULT	STDMETHODCALLTYPE COggSplitOutputPin::GetClassID(CLSID* pClassID)
{
	*pClassID = CLSID_OggSplitter;
	return NOERROR;
}

// IPersistPropertyPage methods
HRESULT STDMETHODCALLTYPE COggSplitOutputPin::Save(IPropertyBag* pPropBag, BOOL fClearDirty,
									   BOOL fSaveAllProperties)
{
	wchar_t		wPropName[4096];
	VARIANT		V;

	VariantInit(&V);
	for (int i=0; i< m_pStream->m_vc.comments; i++)
	{
		wsprintfW(wPropName, L"%s", m_pStream->m_vc.user_comments[i]);
		wchar_t* pPos = wcsstr(wPropName, L"=");
		if (pPos)
		{
			*pPos = '\0';
			Read(wPropName, &V, NULL);
			pPropBag->Write(wPropName, &V);
		}
	}
	VariantClear(&V);

	return NOERROR;
}

HRESULT STDMETHODCALLTYPE COggSplitOutputPin::Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog)
{
	char		propname[256];
	char*		pValue;

	wsprintf(propname, "%S", pszPropName);

	VariantClear(pVar);
	pValue = vorbis_comment_query(&m_pStream->m_vc, propname, 0);
	pVar->vt = VT_BSTR;
	pVar->bstrVal = SysAllocStringLen(NULL, strlen(pValue)+1);
	wsprintfW(pVar->bstrVal, L"%s", pValue);

	return pValue == NULL ? S_FALSE : NOERROR;
}

