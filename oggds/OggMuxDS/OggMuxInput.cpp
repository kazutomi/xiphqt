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

#include <limits.h>
#include "OggMuxDS.h"

COggMuxInputPin::COggMuxInputPin(TCHAR* pObjectName, COggMux* pFilter, CCritSec* pLock,
								 int StreamID, LPCWSTR pName, HRESULT* phr):
	CBaseInputPin(pObjectName, (CBaseFilter*) pFilter, pLock, phr, pName)
{
	m_sh = NULL;
	m_pOggMux = pFilter;
	m_iStreamID = StreamID;
	vorbis_comment_init(&m_vc);
}

COggMuxInputPin::~COggMuxInputPin(void)
{
	vorbis_comment_clear(&m_vc);
}

STDMETHODIMP COggMuxInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv) // For PropertyPages
{
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
	return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP COggMuxInputPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{
	ALLOCATOR_PROPERTIES	AllocProps;

	pAllocator->GetProperties(&AllocProps);
	m_cbBufferSize = AllocProps.cbBuffer;  // Save the buffersize 
	return CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
}

ogg_int64_t COggMuxInputPin::CurrentPos()
{
	REFERENCE_TIME	rtTime;
	
	if (OggPageQueueCurrTime(&rtTime))
		return rtTime;
	
	if (m_bEOSReceived)	return _I64_MAX;	// => other streams don't have to wait for this one

	return m_rtCurrent;
}



void COggMuxInputPin::PagesOut(bool bFlush)
{
	int				or;
	ogg_page		og;
	ogg_int64_t		tPos;
	REFERENCE_TIME	rtPos;

	do
	{
		tPos = m_ss.os.granule_vals[0];

		if (bFlush)
			or = ogg_stream_flush(&m_ss.os, &og);
		else
			or = ogg_stream_pageout(&m_ss.os, &og);
		if (or <= 0) return;

		m_pOggMux->ResetBlockedState();					// Inform the muxer that there is still data flow

		int pageno = ogg_page_pageno(&og);

		if (*(m_mt.Subtype()) == MEDIASUBTYPE_Vorbis)
		{
			if (pageno == 0)		rtPos = -3; // Header
			else if (pageno == 1)	rtPos = -2; // Comment
			else if (pageno == 2)	rtPos = -1; // Codebook
			else rtPos = mediatime_to_reference_time(SEC_IN_REFTIME, m_vi.rate, tPos);
		}
		else
		{		
			if (pageno == 0)		rtPos = -3; // Header
			else if (pageno == 1)	rtPos = -2; // Comment
			else rtPos = mediatime_to_reference_time(m_sh->time_unit, m_sh->samples_per_unit, tPos);
		}

		OggPageEnqueue(&og, &rtPos);

	} while (true);
}

tPageNode* COggMuxInputPin::GetPage()
{
	tPageNode*	pPage;
	
	pPage = OggPageDequeue();
	if (!pPage) return NULL;

	if (OggPageQueuePages() == 0)
		m_evWait.Set();						// If the list is empty we resume the receive thread ...

	m_rtCurrent = pPage->rtPos;
	return pPage;
}

HRESULT	COggMuxInputPin::Active()
{
	HRESULT	hr;
	hr = CBaseInputPin::Active();
	if FAILED(hr) return hr;

	if (!IsConnected())
	{ // Deliver EOS if not connected;
		m_bEOSReceived = true;
		return EndOfStream();
	}

	m_bAbort = false;
	m_bCanBlock = true;
	
	m_rtCurrent = _I64_MIN;
	m_bEOSReceived = FALSE;

	stream_state_init(&m_ss, m_iStreamID);

	if (*(m_mt.Subtype()) == MEDIASUBTYPE_Vorbis)
	{	// Ogg Vorbis ..
		m_vi.rate = ((VORBISFORMAT*) m_mt.Format())->nSamplesPerSec;
		m_bHeaderSent = true;
	}
	else if (*(m_mt.FormatType()) == FORMAT_VideoInfo)
	{
		VIDEOINFO* vi = (VIDEOINFO*) m_mt.Format();
			
		stream_header_setup_video(&m_sh, (char*)(m_mt.Subtype()), vi->AvgTimePerFrame,
									vi->bmiHeader.biWidth, vi->bmiHeader.biHeight,
									vi->bmiHeader.biBitCount, m_cbBufferSize);
		m_bHeaderSent = false;
	}
	else if (*(m_mt.FormatType()) == FORMAT_WaveFormatEx)
	{
		WAVEFORMATEX* pwfx = (WAVEFORMATEX*) m_mt.Format();


		stream_header_setup_audio(&m_sh, pwfx->wFormatTag, pwfx->nChannels,
									pwfx->nBlockAlign, pwfx->nAvgBytesPerSec,
									pwfx->nSamplesPerSec, pwfx->wBitsPerSample,
									(unsigned char*)pwfx + sizeof(WAVEFORMATEX), pwfx->cbSize,
									m_cbBufferSize);
		m_bHeaderSent = false;
	}	
	else if (*(m_mt.Type()) == MEDIATYPE_Text)
	{
		stream_header_setup_text(&m_sh);
		m_bHeaderSent = false;
	}
	m_bSampleReady = FALSE;
	
	return NOERROR;
}

HRESULT	COggMuxInputPin::Inactive()
{
	m_bAbort = true;
	m_evWait.Set();
	
	// Wait until Receive() exits ...
    CAutoLock lck(&m_csReceive);

	return CBaseInputPin::Inactive();
}


STDMETHODIMP COggMuxInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	return m_pOggMux->NewSegment(tStart, tStop, dRate);
}

// We must signal the event to exit Receive()
// other wise upstream filters may be blocked
STDMETHODIMP COggMuxInputPin::BeginFlush(void)
{
	m_bAbort = true;
	m_evWait.Set();
	return NOERROR;
}

STDMETHODIMP COggMuxInputPin::EndFlush(void)
{
	if (m_pOggMux->m_State != State_Stopped) m_bAbort = false;
	return NOERROR;
}

// We delay the samples by one to set EOF
// on the last packet on EndOfStream
STDMETHODIMP COggMuxInputPin::Receive(IMediaSample *pSample)
{
    HRESULT hr = NOERROR;
    CAutoLock lck(&m_csReceive);  // Serialize Receive() on this pin

    // check all is well with the base class
    hr = CBaseInputPin::Receive(pSample);
	if FAILED(hr) return hr;

    if (m_SampleProps.dwStreamId != AM_STREAM_MEDIA)
        return m_pOggMux->m_pOutput->Deliver(pSample);

	// If we have a queued sample add it to the stream
	if (m_bSampleReady)
	{
		m_rtStop = m_SampleProps.tStart;
		ProcessSample(false);
	}
	
	// Save this sample for use in the next Receive()
	unsigned char*	pbSample;
	unsigned char*	pbBuffer;

	pSample->GetPointer(&pbSample);
	stream_samplein_getbuffer(&m_ss, &pbBuffer, m_cbBufferSize);
	m_cbSampleSize = m_SampleProps.lActual;
	memcpy(pbBuffer, pbSample, m_cbSampleSize);
	m_rtStart		= m_SampleProps.tStart;
	m_rtStop		= m_SampleProps.tStop;
	m_bSyncPoint	= (pSample->IsSyncPoint() == S_OK);

	if (*(m_mt.Subtype()) == MEDIASUBTYPE_Vorbis)
	{
		if (*pbSample & PACKET_TYPE_HEADER)
		{
			m_bFlush = true;
			if ((*pbSample & PACKET_TYPE_BITS) == PACKET_TYPE_COMMENT)
			{
				ogg_packet	op;
				vorbis_commentheader_out(&m_vc, &op);
				m_cbSampleSize = op.bytes;
				memcpy(pbBuffer, op.packet, op.bytes);
			}
		}
		else m_bFlush = false;
	}

	else // Not Vorbis
	{
		if (*(m_mt.Type()) == MEDIATYPE_Text)
		{
			// In case of subtitle we should use one page for each packet otherwise
			// it would take very long after seek to display subtitles again
			m_bSyncPoint = true;
			m_bFlush = true;
		}
		else
		{
			m_bFlush = false;
		}
	}

	m_bSampleReady = true;

	return hr;
}

HRESULT COggMuxInputPin::ProcessSample(bool bEOS)
{	
	if (!m_bHeaderSent)
	{
		stream_headerout_header(&m_ss, m_sh);
		PagesOut(true);
		stream_headerout_comment(&m_ss, &m_vc);
		PagesOut(true);
		m_bHeaderSent = true;
	}

	if (*(m_mt.Subtype()) == MEDIASUBTYPE_Vorbis)
		stream_samplein_vorbis(&m_ss, &m_vi, bEOS, NULL, &m_rtStart, m_cbSampleSize);
	else 
		stream_samplein(&m_ss, m_sh, bEOS, m_bSyncPoint,
						NULL, NULL, &m_rtStart, &m_rtStop, m_cbSampleSize);

	PagesOut(m_bFlush);
	m_pOggMux->Interleave();

	if (!OggPageQueuePageReady() || !m_bCanBlock)
		return NOERROR;

	do
	{
		m_bBlocked = true;
		if (m_evWait.Wait(2000))
			return NOERROR;
		if (m_bAbort) 
			return NOERROR;
	} while (!m_bBlocked);

	// if we get here it seems that blocking this stream blocks the whole graph
	m_bCanBlock = false;

	return NOERROR;
}

STDMETHODIMP COggMuxInputPin::EndOfStream(void)
{
	if (m_bSampleReady)
	{
		ProcessSample(true);
		m_bSampleReady = FALSE;
	}
	m_bEOSReceived = true;
	return m_pOggMux->EndOfStream();
}

// CheckInputType
HRESULT COggMuxInputPin::CheckMediaType(const CMediaType* pmt)
{
	if (*(pmt->Type()) == MEDIATYPE_Audio)
		if (*(pmt->Subtype()) == MEDIASUBTYPE_Vorbis)
			return NOERROR;

	if (*(pmt->Type()) == MEDIATYPE_Text)
		return NOERROR;
	
	// Videosubtype GUIDs have a common data2 and data3 part
	// therefore we can check data2 and data3 with any video GUID
	if (*(pmt->FormatType()) == FORMAT_VideoInfo)
		if (pmt->Subtype()->Data2 == MEDIASUBTYPE_YVYU.Data2 &&
			pmt->Subtype()->Data3 == MEDIASUBTYPE_YVYU.Data3)
			return NOERROR;

	if (*(pmt->FormatType()) == FORMAT_WaveFormatEx)
		if (pmt->Subtype()->Data2 == MEDIASUBTYPE_PCM.Data2 &&
			pmt->Subtype()->Data3 == MEDIASUBTYPE_PCM.Data3)
			return NOERROR;
	
	return VFW_E_TYPE_NOT_ACCEPTED;
}

// create a new Pin if neccessary
HRESULT COggMuxInputPin::CompleteConnect(IPin *pReceivePin)
{
	HRESULT hr = CBasePin::CompleteConnect(pReceivePin);
	if FAILED(hr) return hr;

	IPersistPropertyBag*	pPropBag;

	if (SUCCEEDED(pReceivePin->QueryInterface(IID_IPersistPropertyBag, (void**)&pPropBag)))
	{
		pPropBag->Save(this, TRUE, TRUE);
		pPropBag->Release();
	}

	return m_pOggMux->CheckFreeInputPin();
}

//
//  IPropertyPage methods
//
HRESULT STDMETHODCALLTYPE COggMuxInputPin::Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog)
{
	// properties are interpreted as vorbis comments

	char		szTag[128];
	char		szValue[2048];
	int			iCount;

	wsprintf(szTag, "%S", pszPropName);
	strupr(szTag);
	iCount = vorbis_comment_query_count(&m_vc, szTag);

	if (iCount == 0) return E_FAIL;

	*szValue = 0;

	for (int i=0; i<iCount; i++)
	{
		if (i) strcat(szValue, "\\n");
		strcat(szValue, vorbis_comment_query(&m_vc, szTag, i));
	}

	VariantClear(pVar);
	VariantInit(pVar);
	pVar->vt = VT_BSTR;
	pVar->bstrVal = SysAllocStringLen(NULL, strlen(szValue));
	wsprintfW(pVar->bstrVal, L"%s", szValue);

	return NOERROR;
};

HRESULT	STDMETHODCALLTYPE COggMuxInputPin::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	// properties are interpreted as vorbis comments
	char		szTag[128];

	wsprintf(szTag, "%S", pszPropName);
	strupr(szTag);

	if (vorbis_comment_query_count(&m_vc, szTag) != 0)
	{
		// There is already a tag and must be deleted
		vorbis_comment	vcTemp;

		vcTemp.comment_lengths	= m_vc.comment_lengths;
		vcTemp.comments			= m_vc.comments;
		vcTemp.user_comments	= m_vc.user_comments;
		vcTemp.vendor			= m_vc.vendor;

		memset(&m_vc, 0, sizeof(m_vc));
		vorbis_comment_init(&m_vc);

		char			srchPattern[128];
		int				srchLen;

		strcpy(szTag, srchPattern);
		strcat(srchPattern, "=");
		srchLen = strlen(srchPattern);

		for (int i=0; i< m_vc.comments; i++)
			if (strncmp(vcTemp.user_comments[i], srchPattern, srchLen) != 0)
				vorbis_comment_add(&m_vc, vcTemp.user_comments[i]);
		
		vorbis_comment_clear(&vcTemp);
	}

	char		szValue[2048];

	VariantChangeType(pVar, pVar, 0, VT_BSTR);
	wsprintf(szValue, "%S", pVar->bstrVal);

	if (*szValue)
		vorbis_comment_add_tag(&m_vc, szTag, szValue);
	
	return NOERROR;
};

HRESULT	STDMETHODCALLTYPE COggMuxInputPin::GetClassID(CLSID* pClassID)
{
	*pClassID = CLSID_OggMux;
	return NOERROR;
}

// IPersistPropertyPage methods
HRESULT STDMETHODCALLTYPE COggMuxInputPin::Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog)
{
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE COggMuxInputPin::Save(IPropertyBag* pPropBag, BOOL fClearDirty,
									   BOOL fSaveAllProperties)
{
	wchar_t		wzTag[2048];
	wchar_t*	pPos;
	VARIANT		V;

	VariantInit(&V);
	for (int i=0; i< m_vc.comments; i++)
	{
		wsprintfW(wzTag, L"%s", m_vc.user_comments[i]);
		pPos = wcsstr(wzTag, L"=");
		if (pPos)
		{
			*pPos = '\0';
			V.vt = VT_BSTR;
			V.bstrVal = SysAllocString(pPos+1);
			pPropBag->Write(wzTag, &V);
			VariantClear(&V);
		}
	}
	VariantClear(&V);

	return NOERROR;
}