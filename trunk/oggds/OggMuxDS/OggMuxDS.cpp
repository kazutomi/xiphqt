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
#include <atlbase.h>

//
//  Strategy:
//  
//  To interleave all the streams and to save memory a stream
//  is blocked in Receive() if other streams are behind.
//  If we detect that blocking leads to a dead lock the corresponding
//  stream is not blocked anymore and a queue is used
//  (Basically if all streams are from the same source)

COggMux::COggMux(LPUNKNOWN pUnk, HRESULT* phr) :
    CBaseFilter(NAME("Ogg Mux"), pUnk, &m_csFilter, CLSID_OggMux)
{
	m_iInputs = 0;
	m_TimeCode = TIME_FORMAT_MEDIA_TIME;

	if (SUCCEEDED(*phr))
	{
        COggMuxOutputPin* pOut = new COggMuxOutputPin(NAME("Output pin"), this,
									&(this->m_csFilter), phr, L"Ogg Stream");
        if(pOut)
            if(SUCCEEDED(*phr)) m_pOutput = pOut; else delete pOut;
        else *phr = E_OUTOFMEMORY;

		*phr = CheckFreeInputPin();
	}
}

COggMux::~COggMux(void)
{
}

STDMETHODIMP COggMux::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_ISpecifyPropertyPages)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((ISpecifyPropertyPages*)(this), ppv);
    }
    if(riid == IID_IMediaSeeking)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IMediaSeeking*)(this), ppv);
    }
	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

// Provide the about property page
STDMETHODIMP COggMux::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	pPages->pElems[0] = CLSID_OggMuxPropPage;
	pPages->pElems[1] = CLSID_OggDSAboutPage;
	return NOERROR;
}

// Create a new input pin if there is no more left,
// which is not already connected
HRESULT	COggMux::CheckFreeInputPin()
{
	int	FreePins = 0;

	for (int i=0; i<m_iInputs; i++)
		if (!m_paInput[i]->IsConnected()) FreePins++;

	if (FreePins < 1)
	{ // Create new pin
		HRESULT			hr = NOERROR;
		WCHAR			PinName[256];
		CHAR			PinID[256];
		COggMuxInputPin *pIn;
		COggMuxInputPin **paInput;

		wsprintfW(PinName,L"Stream %d", m_iInputs);
		wsprintf(PinID, "Stream %d", m_iInputs);
		pIn = new COggMuxInputPin(PinID, this, &m_csFilter, m_iInputs, PinName, &hr);
		if FAILED(hr) return hr;
		paInput = new COggMuxInputPin *[m_iInputs+1];
		if (!paInput)
		{
			delete pIn;
			return E_OUTOFMEMORY;
		}
		if (m_paInput)
		{
			CopyMemory((void*)paInput, (void*)m_paInput, m_iInputs * sizeof(m_paInput[0]));
			delete [] m_paInput;
		}
		m_paInput = paInput;
		m_paInput[m_iInputs] = pIn;
		m_iInputs++;
	}  
	return NOERROR;
}

// GetPinCount
// Returns the number of pins this filter has
int COggMux::GetPinCount(void)
{
    CAutoLock lock(&m_csFilter);
    return m_iInputs + 1;
}

// Return a non-addref'd pointer to pin n
// needed by CBaseFilter
CBasePin* COggMux::GetPin(int n)
{
    CAutoLock lock(&m_csFilter);

	if (n == 0) return m_pOutput;
	else if ((n > 0) && (n <= m_iInputs)) return m_paInput[n-1];
    return NULL;
}

COggMuxInputPin *COggMux::GetEarliestPin(void)
{
	REFERENCE_TIME	TimeStamp = _I64_MAX;
	REFERENCE_TIME	rtCurrent;
	COggMuxInputPin *pEarliestPin;

	// This loop starts with the highest stream number
	// to give lower stream numbers higher priority
	for (int i=m_iInputs-1; i>=0; i--)
		if (m_paInput[i]->IsConnected())
		{
			rtCurrent = m_paInput[i]->CurrentPos();
			if (rtCurrent <= TimeStamp)
			{
				TimeStamp = rtCurrent;
				pEarliestPin = m_paInput[i]->OggPageQueuePages() != 0 ? m_paInput[i] : NULL;
			}
		}

	return pEarliestPin;
}

HRESULT COggMux::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_rtSegStart = tStart;
	return NOERROR;
}

HRESULT COggMux::EndOfStream(void)
{
	CAutoLock lock(&m_csFilter);
	int	ActivePins = 0;
	for (int i=0; i<m_iInputs; i++)
		if (m_paInput[i]->IsConnected() && !m_paInput[i]->EOSReceived())
			ActivePins++;

	// Send EOS if all pins have received already EOS
	if (!ActivePins) return m_pOutput->DeliverEndOfStream();
	return NOERROR;
}

HRESULT COggMux::Interleave()
{
	CAutoLock			lock(&m_csWriteOutPages);
	
	HRESULT				hr;
	COggMuxInputPin*	pPin;

	while (pPin = GetEarliestPin())
	{
		IMediaSample	*pOutSample;
		BYTE			*buffer;
		tPageNode*		pPage = pPin->GetPage();

		{
			CAutoLock lock(&m_csPosition);
			m_rtPosition = pPage->rtPos;
		}
		hr = m_pOutput->GetSample(&pOutSample, pPage->og.header_len+pPage->og.body_len);
		if FAILED(hr) return hr;

		// Buffer overflow shoudn't occur but to be on the safe side ...
		if (pPage->og.header_len+pPage->og.body_len > pOutSample->GetSize())
			return VFW_E_BUFFER_OVERFLOW;

		pOutSample->GetPointer(&buffer);
		memcpy(buffer,                      pPage->og.header, pPage->og.header_len);
		memcpy(buffer+pPage->og.header_len, pPage->og.body,   pPage->og.body_len);
		pOutSample->SetActualDataLength(pPage->og.header_len+pPage->og.body_len);
		pPin->OggPageRelease(pPage);
		pOutSample->SetDiscontinuity(FALSE);
		pOutSample->SetPreroll(FALSE);
		pOutSample->SetSyncPoint(TRUE);
		hr = m_pOutput->Deliver(pOutSample);
		pOutSample->Release();
		if FAILED(hr) return hr;
	}
	return NOERROR;
}

void COggMux::ResetBlockedState()
{
	for (int i=0; i<m_iInputs; i++)
		m_paInput[i]->ResetBlockedState();
}

STDMETHODIMP COggMux::Stop()
{
	HRESULT	hr;
	hr = CBaseFilter::Stop();

	for (int i=0; i<m_iInputs; i++)
	{
		// Delete all page buffers ...
		m_paInput[i]->OggPageQueueFlush();
		stream_state_clear(&m_paInput[i]->m_ss);
		if (m_paInput[i]->m_sh)
		{
			free(m_paInput[i]->m_sh);
			m_paInput[i]->m_sh = NULL;
		}

	}

	return hr;
}

CUnknown* COggMux::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr)
{
	return new COggMux(pUnk, phr);
}
