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

#include "VorbEncDS.h"

CVorbisEncOutputPin::CVorbisEncOutputPin(TCHAR* pObjectName, CVorbisEnc* pFilter,
										HRESULT* phr, LPCWSTR pName ) :
	CTransformOutputPin(pObjectName, pFilter, phr, pName)
{
	m_pVorbisEnc = pFilter;
};

STDMETHODIMP CVorbisEncOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if(riid == IID_IAMStreamConfig)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IAMStreamConfig*)(this), ppv);
    }
    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CVorbisEncOutputPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
	if (!m_pFilter->GetPin(0)->IsConnected()) return VFW_E_NOT_CONNECTED;
	CMediaType		mt;
	mt.Set(*pmt);
	VORBISFORMAT	*pfmt	 = (VORBISFORMAT*) mt.Format();

	m_pVorbisEnc->m_fQuality		= pfmt->fQuality;
	m_pVorbisEnc->m_nMinBitsPerSec	= pfmt->nMinBitsPerSec;
	m_pVorbisEnc->m_nAvgBitsPerSec	= pfmt->nAvgBitsPerSec;
	m_pVorbisEnc->m_nMaxBitsPerSec	= pfmt->nMaxBitsPerSec;
	return CBaseOutputPin::SetMediaType(&mt);
}

STDMETHODIMP CVorbisEncOutputPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	if (!m_pFilter->GetPin(0)->IsConnected()) return VFW_E_NOT_CONNECTED;

	CMediaType mt = m_mt;
	if (mt.IsPartiallySpecified()) m_pVorbisEnc->GetMediaType(0, &mt);
	
	*ppmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
	**ppmt = mt;
	(*ppmt)->pbFormat = (BYTE*)CoTaskMemAlloc((*ppmt)->cbFormat);
	memcpy((*ppmt)->pbFormat, mt.Format(), (*ppmt)->cbFormat);
	return NOERROR;
}

STDMETHODIMP CVorbisEncOutputPin::GetNumberOfCapabilities(int *piCount, int *piSize)
{
	return E_NOTIMPL;
}

STDMETHODIMP CVorbisEncOutputPin::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC)
{
	return E_NOTIMPL;
}
