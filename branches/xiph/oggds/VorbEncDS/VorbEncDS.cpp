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

//-----------------------------------------------------------------------------
//
//	Vorbis Encode Filter
//
//	Delivers Vorbis packets with start and stop time based
//  on the input stream time
//
//-----------------------------------------------------------------------------
#include "VorbEncDS.h"
#include "..\resource.h"
#include "..\OggDS.h"

CVorbisEnc::CVorbisEnc(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
    CTransformFilter(tszName, punk, CLSID_VorbisEnc),
	CPersistPropertyBag(arVorbisEncProps, SIZEOF_ARRAY(arVorbisEncProps), &CLSID_VorbisEnc),
	CRegistryStuff(&CLSID_VorbisEnc)
{
	*phr = NOERROR;

    CVorbisEncOutputPin *pOut = new CVorbisEncOutputPin("Ogg Vorbis Packets",
                                        this, phr, L"Ogg Vorbis Packets");
    if(pOut)
        if SUCCEEDED(*phr) m_pOutput = pOut; else delete pOut;
    else *phr = E_OUTOFMEMORY;

	CTransformInputPin *pIn = new CTransformInputPin("PCM Audio",
                                        this, phr, L"PCM Audio");
    if(pOut)
        if SUCCEEDED(*phr) m_pInput = pIn; else delete pIn;
    else *phr = E_OUTOFMEMORY;

	memset(&m_vi, 0, sizeof(m_vi));
	memset(&m_vc, 0, sizeof(m_vc));
	memset(&m_vb, 0, sizeof(m_vb));
	memset(&m_vd, 0, sizeof(m_vd));

	LoadFromRegistry(idMinBitrate, &m_nMinBitsPerSec, -1);
	LoadFromRegistry(idAvgBitrate, &m_nAvgBitsPerSec, -1);
	LoadFromRegistry(idMaxBitrate, &m_nMaxBitsPerSec, -1);
    LoadFromRegistry(idQuality, &m_fQuality, (float)0.1);

	m_pMapping = NULL;
	m_dwChannelMask = 0;
}

CVorbisEnc::~CVorbisEnc()
{
	if (m_pMapping) delete [] m_pMapping;
};


STDMETHODIMP CVorbisEnc::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if(riid == IID_ISpecifyPropertyPages)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((ISpecifyPropertyPages*)(this), ppv);
    }
	if (riid == IID_IPersistPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPersistPropertyBag*)(this), ppv);
	}
	if (riid == IID_IPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPropertyBag*)(this), ppv);
	}
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CVorbisEnc::GetPages(CAUUID *pPages)
{
	CheckPointer(pPages, E_POINTER);
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID)*2);
	pPages->pElems[0] = CLSID_VorbisEncPropPage;
	pPages->pElems[1] = CLSID_OggDSAboutPage;
	return NOERROR;
}

HRESULT	CVorbisEnc::CheckInputType(const CMediaType* pmt)
{
	if (*(pmt->FormatType()) != FORMAT_WaveFormatEx) return VFW_E_TYPE_NOT_ACCEPTED;

	WAVEFORMATEX*	pwfx = (WAVEFORMATEX*) pmt->Format();

	if (pwfx->wFormatTag != WAVE_FORMAT_PCM &&
		pwfx->wFormatTag != WAVE_FORMAT_IEEE_FLOAT &&
		pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE) return VFW_E_TYPE_NOT_ACCEPTED;

	return NOERROR;
}

HRESULT	CVorbisEnc::CheckTransform(const CMediaType* pmtIn, const CMediaType* pmtOut)
{
	if ((*(pmtOut->Type())       != MEDIATYPE_Audio) || 
		(*(pmtOut->Subtype())    != MEDIASUBTYPE_Vorbis) ||
	    (*(pmtOut->FormatType()) != FORMAT_VorbisFormat)) return VFW_E_TYPE_NOT_ACCEPTED;

	return CheckInputType(pmtIn);
};

HRESULT CVorbisEnc::GetMediaType(int iPosition, CMediaType *pmt)
{
  	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	pmt->InitMediaType();
	pmt->SetType(&MEDIATYPE_Audio);
	pmt->SetSubtype(&MEDIASUBTYPE_Vorbis);
	pmt->SetFormatType(&FORMAT_VorbisFormat);

	WAVEFORMATEX* pwfx = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	VORBISFORMAT *pfmt = (VORBISFORMAT*) pmt->AllocFormatBuffer(sizeof(VORBISFORMAT));
    if (pfmt == NULL) return E_OUTOFMEMORY;

	pfmt->nChannels = pwfx->nChannels;
	pfmt->nSamplesPerSec = pwfx->nSamplesPerSec;
	pfmt->fQuality		 = m_fQuality;
	pfmt->nMinBitsPerSec = m_nMinBitsPerSec;
	pfmt->nAvgBitsPerSec = m_nAvgBitsPerSec;
	pfmt->nMaxBitsPerSec = m_nMaxBitsPerSec;

	return NOERROR;
} // GetMediaType

HRESULT CVorbisEnc::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProps)
{
	ALLOCATOR_PROPERTIES CurrentProps;

	if ((pAlloc == NULL) || (pProps == NULL)) return E_POINTER;
	if (pProps->cBuffers < 1)     pProps->cBuffers = 1;
	if (pProps->cbBuffer < 16384) pProps->cbBuffer = 16384;
    pProps->cbAlign = 1;
	pProps->cbPrefix = 0;
	return pAlloc->SetProperties(pProps,&CurrentProps);
};

HRESULT	CVorbisEnc::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
	if (direction == PINDIR_OUTPUT) return NOERROR;
	if (m_pMapping) delete [] m_pMapping;

	WAVEFORMATEX*	pwfx = (WAVEFORMATEX* ) pmt->Format();

	m_pMapping = new int[pwfx->nChannels];

	if (pwfx->nChannels != 6)
	{
		for (int i=0; i<pwfx->nChannels; i++)
			m_pMapping[i] = i;
	}
	else
	{
		// AC3 5.1 mapping
		m_pMapping[0] = 0;
		m_pMapping[1] = 2;
		m_pMapping[2] = 1;
		m_pMapping[3] = 5;
		m_pMapping[4] = 3;
		m_pMapping[5] = 4;
	}

	return NOERROR;
}

// Provide the way for COM to create a CVorbisEnc object
CUnknown *CVorbisEnc::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CVorbisEnc *pNewObject = new CVorbisEnc("Vorbis Encoder", punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
} // CreateInstance


// IPropertyBag methods
HRESULT STDMETHODCALLTYPE CVorbisEnc::Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog)
{
	char	szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));
	VariantClear(pVar);

	if (strcmp(szName, idMinBitrate) == 0)
	{
		pVar->vt = VT_I4; pVar->lVal = m_nMinBitsPerSec;
		return NOERROR;
	}
	if (strcmp(szName, idAvgBitrate) == 0)
	{
		pVar->vt = VT_I4; pVar->lVal = m_nAvgBitsPerSec;
		return NOERROR;
	}
	if (strcmp(szName, idMaxBitrate) == 0)
	{
		pVar->vt = VT_I4; pVar->lVal = m_nMaxBitsPerSec;
		return NOERROR;
	}
	if (strcmp(szName, idQuality) == 0)
	{
		pVar->vt = VT_R4; pVar->fltVal = m_fQuality;
		return NOERROR;
	}

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CVorbisEnc::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	char	szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));

	if (strcmp(szName, idMinBitrate) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);
		m_nMinBitsPerSec = (pVar->lVal);
		SaveToRegistry(idMinBitrate, m_nMinBitsPerSec);
		return NOERROR;
	}
	if (strcmp(szName, idAvgBitrate) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);
		m_nAvgBitsPerSec = (pVar->lVal);
		SaveToRegistry(idAvgBitrate, m_nAvgBitsPerSec);
		return NOERROR;
	}
	if (strcmp(szName, idMaxBitrate) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);
		m_nMaxBitsPerSec = (pVar->lVal);
		SaveToRegistry(idMaxBitrate, m_nMaxBitsPerSec);
		return NOERROR;
	}
	if (strcmp(szName, idQuality) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_R4);
		m_fQuality = (pVar->fltVal);
		SaveToRegistry(idQuality, m_fQuality);
		return NOERROR;
	}
	
	return E_FAIL;
}
