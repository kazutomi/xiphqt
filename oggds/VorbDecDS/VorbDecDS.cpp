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

#include "VorbDecDS.h"
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

CVorbisDec::CVorbisDec(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
    CTransformFilter(tszName, punk, CLSID_VorbisDec),
	CPersistPropertyBag(arVorbisDecProps, SIZEOF_ARRAY(arVorbisDecProps), &CLSID_VorbisDec),
	CRegistryStuff(&CLSID_VorbisDec)
{
	if SUCCEEDED(*phr)
    {	// Create the Pins to set the more meaningful pin name
        CVorbisDecInputPin *pIn = new CVorbisDecInputPin("Transform input pin",
                                            this, phr, L"Ogg Vorbis Packets");   
        if (pIn)
            if SUCCEEDED(*phr) m_pInput = pIn; else delete pIn;
        else *phr = E_OUTOFMEMORY;

        CTransformOutputPin *pOut = new CTransformOutputPin("Transform output pin",
                                            this, phr, L"PCM Audio");
        if(pOut)
            if SUCCEEDED(*phr) m_pOutput = pOut; else delete pOut;
        else *phr = E_OUTOFMEMORY;
    }

	OSVERSIONINFO	osvi;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);

	LoadFromRegistry(idFloatModeFirst, &m_bFloatModeFirst, 
					osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);

	m_pMapping = NULL;
	m_dwChannelMask = 0;
}

CVorbisDec::~CVorbisDec()
{
	if (m_pMapping) delete [] m_pMapping;
}

STDMETHODIMP CVorbisDec::NonDelegatingQueryInterface(REFIID riid, void **ppv)
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

STDMETHODIMP CVorbisDec::GetPages(CAUUID *pPages)
{
	CheckPointer(pPages, E_POINTER);
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID)*2);
	pPages->pElems[0] = CLSID_VorbisDecPropPage;
	pPages->pElems[1] = CLSID_OggDSAboutPage;
	return NOERROR;
}

void CVorbisDec::VorbisClear()
{
	if (m_bVorbisInitialized )
	{
		vorbis_block_clear(&m_vb);
	    vorbis_dsp_clear(&m_vd);
		vorbis_comment_clear(&m_vc);
		vorbis_info_clear(&m_vi);
		m_bVorbisInitialized = false;
	}
}

HRESULT	CVorbisDec::VorbisInit()
{
	ogg_packet	op;

	if (!m_pbVorbisInfo || !m_pbVorbisComment || !m_pbVorbisCodebook)
		return E_FAIL;

	VorbisClear();

	vorbis_info_init(&m_vi);
	vorbis_comment_init(&m_vc);

	op.b_o_s = 1;
	op.e_o_s = 0;
	op.granulepos = 0;
	op.packetno = 0;
	op.packet = m_pbVorbisInfo;
	op.bytes = m_cbVorbisInfo;
	vorbis_synthesis_headerin(&m_vi, &m_vc, &op);

	op.b_o_s = 0;
	op.e_o_s = 0;
	op.granulepos = 0;
	op.packetno = 1;
	op.packet = m_pbVorbisComment;
	op.bytes = m_cbVorbisComment;
	vorbis_synthesis_headerin(&m_vi, &m_vc, &op);

	op.b_o_s = 0;
	op.e_o_s = 0;
	op.granulepos = 0;
	op.packetno = 2;
	op.packet = m_pbVorbisCodebook;
	op.bytes = m_cbVorbisCodebook;
	vorbis_synthesis_headerin(&m_vi, &m_vc, &op);

	vorbis_synthesis_init(&m_vd,&m_vi);
	vorbis_block_init(&m_vd,&m_vb);

	m_bVorbisInitialized = true;
	m_packetno = 3;

	// Check if there is a postgain tag ...
	m_bPostGain = false;
	if (vorbis_comment_query_count(&m_vc, "LWING_GAIN"))
	{
		m_bPostGain = true;
		m_fPostGain = (float)atof(vorbis_comment_query(&m_vc, "LWING_GAIN", 0));
	}
	else if (vorbis_comment_query_count(&m_vc, "POSTGAIN"))
	{
		m_bPostGain = true;
		m_fPostGain = (float)atof(vorbis_comment_query(&m_vc, "POSTGAIN", 0));
	}

	return NOERROR;
}

HRESULT CVorbisDec::StartStreaming()
{
	m_QueuedSample = NULL;
	m_pbVorbisInfo = NULL;
	m_pbVorbisComment = NULL;
	m_pbVorbisCodebook = NULL;
	m_bVorbisInitialized = false;
	return NOERROR;
}

HRESULT CVorbisDec::StopStreaming()
{
	CAutoLock lck(&m_csReceive);

	if (m_QueuedSample)
	{
		m_QueuedSample->Release();
		m_QueuedSample = NULL;
	}

	VorbisClear();

	if (m_pbVorbisInfo)
	{
		free(m_pbVorbisInfo);
		m_pbVorbisInfo = NULL;
	}
	if (m_pbVorbisComment)
	{
		free(m_pbVorbisComment);
		m_pbVorbisComment = NULL;
	}
	if (m_pbVorbisCodebook)
	{
		free(m_pbVorbisCodebook);
		m_pbVorbisCodebook = NULL;
	}
	return NOERROR;
}

HRESULT CVorbisDec::BeginFlush()
{
	HRESULT	hr = CTransformFilter::BeginFlush();

	CAutoLock		lck(&m_csReceive);

	if (m_QueuedSample)
	{
		ProcessSample(m_QueuedSample, TRUE, TRUE);
		m_QueuedSample->Release();
		m_QueuedSample = NULL;
	}
	VorbisClear();

	return hr;
}

HRESULT CVorbisDec::ProcessHeaderPacket(unsigned char *pbBuffer, int cbBuffer)
{
	switch (*pbBuffer & PACKET_TYPE_BITS)
	{
	case PACKET_TYPE_HEADER: // Vorbis header ...
		if (m_pbVorbisInfo) free(m_pbVorbisInfo);
		m_pbVorbisInfo = (unsigned char*) malloc(cbBuffer);
		m_cbVorbisInfo = cbBuffer;
		memcpy(m_pbVorbisInfo, pbBuffer, m_cbVorbisInfo);
		return NOERROR;

	case PACKET_TYPE_COMMENT: // Vorbis Comment
		if (m_pbVorbisComment) free(m_pbVorbisComment);
		m_pbVorbisComment = (unsigned char*) malloc(cbBuffer);
		m_cbVorbisComment = cbBuffer;
		memcpy(m_pbVorbisComment, pbBuffer, m_cbVorbisComment);
		return NOERROR;
	
	case PACKET_TYPE_CODEBOOK: // Vorbis Codebook
		if (m_pbVorbisCodebook) free(m_pbVorbisCodebook);
		m_pbVorbisCodebook = (unsigned char*) malloc(cbBuffer);
		m_cbVorbisCodebook = cbBuffer;
		memcpy(m_pbVorbisCodebook, pbBuffer, m_cbVorbisCodebook);
		return NOERROR;
	}
	return NOERROR;
}

HRESULT CVorbisDec::InterleaveFloat(void* pbBuffer, int iMaxSamples, int* pMapping,
									int* iSamplesWritten, int* iBytesWritten)
{
	HRESULT			hr = NOERROR;
	int				iSamplesOut;
	float			**ppInBuffer;
	float			*pOutBuffer = (float*)pbBuffer;

	*iSamplesWritten = 0;

	while (iSamplesOut = vorbis_synthesis_pcmout(&m_vd, &ppInBuffer) > 0)
	{
		if (*iSamplesWritten + iSamplesOut > iMaxSamples) hr = VFW_E_BUFFER_OVERFLOW;
		else
		{
			for (int iSamples=0; iSamples<iSamplesOut; iSamples++)
			{
				for (int iChannels=0; iChannels<m_vi.channels; iChannels++)
					if (!m_bPostGain)
						pOutBuffer[m_pMapping[iChannels]] = ppInBuffer[iChannels][iSamples];
					else
						pOutBuffer[m_pMapping[iChannels]] = ppInBuffer[iChannels][iSamples] * m_fPostGain;
				pOutBuffer += m_vi.channels;
			}
		}
		*iSamplesWritten += iSamplesOut;
		vorbis_synthesis_read(&m_vd, iSamplesOut); // tell libvorbis how many samples read
	}

	*iBytesWritten = (BYTE*)pOutBuffer - (BYTE*)pbBuffer;
	
	return hr;
}

HRESULT CVorbisDec::InterleaveInt16(void* pbBuffer, int iMaxSamples, int* pMapping,
									int* iSamplesWritten, int* iBytesWritten)
{
	HRESULT			hr = NOERROR;
	int				iSamplesOut;
	float			**ppInBuffer;
	__int16			*pOutBuffer = (__int16*)pbBuffer;

	*iSamplesWritten = 0;

	while (iSamplesOut = vorbis_synthesis_pcmout(&m_vd, &ppInBuffer) > 0)
	{
		if (*iSamplesWritten + iSamplesOut > iMaxSamples) hr = VFW_E_BUFFER_OVERFLOW;
		else
		{
			for (int iSamples=0; iSamples<iSamplesOut; iSamples++)
			{
				for (int iChannels=0; iChannels<m_vi.channels; iChannels++)
				{
					__int32 val;

					if (!m_bPostGain)
						val = (__int32) (ppInBuffer[iChannels][iSamples] * 32767.f);
					else
						val = (__int32) (ppInBuffer[iChannels][iSamples] * 32767.f * m_fPostGain);
					// Are we out of reange ?
					if (val > 32767) val = 32767;
					else if (val < -32768) val = -32768;

					pOutBuffer[m_pMapping[iChannels]] = (__int16)val; 
				}
				pOutBuffer += m_vi.channels;
			}
		}
		*iSamplesWritten += iSamplesOut;
		vorbis_synthesis_read(&m_vd, iSamplesOut); // tell libvorbis how many samples read
	}

	*iBytesWritten = (BYTE*)pOutBuffer - (BYTE*)pbBuffer;
	
	return hr;
}

HRESULT CVorbisDec::ProcessSample(IMediaSample *pSample, BOOL bEOS, BOOL bPreroll)
{
	HRESULT			hr;
	int				vr;
	ogg_packet		op;

	pSample->GetPointer(&op.packet);
	op.bytes  = pSample->GetActualDataLength();
	if (*op.packet & PACKET_TYPE_HEADER) return ProcessHeaderPacket(op.packet, op.bytes);
	
	if (pSample->IsDiscontinuity() == S_OK)	VorbisClear();

	if (bPreroll) return NOERROR;
	
	if (!m_bVorbisInitialized)
	{
		hr = VorbisInit();
		if FAILED(hr) return hr;
	}

	REFERENCE_TIME	rtStart;
	REFERENCE_TIME	rtStop;

	hr = pSample->GetTime(&rtStart, &rtStop);
	if FAILED(hr) return hr;

	op.b_o_s = 0;
	op.e_o_s = bEOS ? 1 : 0;
	op.packetno = m_packetno++;
	op.granulepos = reference_time_to_mediatime(SEC_IN_REFTIME, m_vi.rate, rtStart);

	// Deliver packet to vorbis
	vr = vorbis_synthesis(&m_vb, &op);
	if (vr >= 0) vorbis_synthesis_blockin(&m_vd, &m_vb);

	if (vorbis_synthesis_pcmout(&m_vd, NULL) == 0) return NOERROR; // No pending samples
	
	// Otherwise get the samples and send them downstream

	IMediaSample	*pOutSample;

	hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
	if FAILED(hr) // We must clear the buffer in this case ....
	{
		int		iSamplesOut;
		float**	ppInBuffer;

		while (iSamplesOut = vorbis_synthesis_pcmout(&m_vd, &ppInBuffer) > 0)
		vorbis_synthesis_read(&m_vd, iSamplesOut);
		return hr;
	}

	void*			pOutBuffer;
	__int32			iMaxSamples;
	int				iSamplesWritten;
	int				iBytesWritten;

	pOutSample->GetPointer((BYTE**)&pOutBuffer);
	iMaxSamples = pOutSample->GetSize() / m_vi.channels / 4;
	
	if (m_bFloatMode)
		hr = InterleaveFloat(pOutBuffer, iMaxSamples, NULL, &iSamplesWritten, &iBytesWritten);
	else
		hr = InterleaveInt16(pOutBuffer, iMaxSamples, NULL, &iSamplesWritten, &iBytesWritten);

	if (FAILED(hr))
	{
		pSample->Release();
		return hr;			// In case of buffer overrun
	}
	
	pOutSample->SetSyncPoint(TRUE);
	pOutSample->SetDiscontinuity(FALSE);
	pOutSample->SetPreroll(FALSE);
	pOutSample->SetActualDataLength(iBytesWritten);

	// Calculate the time ...
	rtStart  = mediatime_to_reference_time(SEC_IN_REFTIME, m_vi.rate,
											m_vd.granulepos - (ogg_int64_t)iSamplesWritten);
	rtStop   = mediatime_to_reference_time(SEC_IN_REFTIME, m_vi.rate, m_vd.granulepos);
//	rtStart -= rtStreamStart;
//	rtStop  -= rtStreamStart;

	pOutSample->SetTime(&rtStart, &rtStop);
	hr = m_pOutput->Deliver(pOutSample);
	pOutSample->Release();

	return hr;
}

HRESULT CVorbisDec::Receive(IMediaSample *pSample)
{
	CAutoLock		lck(&m_csReceive);
	HRESULT			hr = NOERROR;

	//  Check for other streams and pass them on
	AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
        return m_pOutput->Deliver(pSample);

	// Save this sample for use in the next Receive()
	IMediaSample *QueuedSample = m_QueuedSample;
	pSample->AddRef();
	m_QueuedSample = pSample;
	// Process the sample from the last Receive() call
	if (QueuedSample)
	{
		hr = ProcessSample(QueuedSample, FALSE, pProps->tStart < 0);
		QueuedSample->Release();
	}

	return hr;
}

HRESULT	CVorbisDec::EndOfStream(void)
{
	CAutoLock		lck(&m_csReceive);

	if (m_QueuedSample)
	{
		ProcessSample(m_QueuedSample, TRUE, FALSE);
		m_QueuedSample->Release();
		m_QueuedSample = NULL;
	}
	return CTransformFilter::EndOfStream();
}

HRESULT CVorbisDec::CheckInputType(const CMediaType *mtIn)
{
	if ((*(mtIn->Type())       != MEDIATYPE_Audio) || 
		(*(mtIn->Subtype())    != MEDIASUBTYPE_Vorbis) ||
		(*(mtIn->FormatType()) != FORMAT_VorbisFormat))
		return VFW_E_TYPE_NOT_ACCEPTED;
	return NOERROR;
}

HRESULT CVorbisDec::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	ALLOCATOR_PROPERTIES	CurrentProps;
	WAVEFORMATEX*			pwfx = (WAVEFORMATEX*) m_pOutput->CurrentMediaType().Format();

	int	iSizeNeeded = 4096 * pwfx->nBlockAlign;

	if ((pAlloc == NULL) || (pProperties == NULL)) return E_POINTER;
	if (pProperties->cBuffers < 1)     pProperties->cBuffers = 1;
	if (pProperties->cbBuffer < iSizeNeeded) pProperties->cbBuffer = iSizeNeeded;
    pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;
	return pAlloc->SetProperties(pProperties,&CurrentProps);
}

HRESULT CVorbisDec::GetMediaType(int iPosition, CMediaType *pmt)
{
    if (m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;
  	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 1) return VFW_S_NO_MORE_ITEMS;

	bool bFloat = iPosition == 0 ? m_bFloatModeFirst : !m_bFloatModeFirst;

	VORBISFORMAT*	pvfx = (VORBISFORMAT*) m_pInput->CurrentMediaType().Format();

	pmt->SetType(&MEDIATYPE_Audio);
	pmt->SetSubtype(&MEDIASUBTYPE_PCM);
    pmt->SetFormatType(&FORMAT_WaveFormatEx);
	pmt->SetTemporalCompression(FALSE);
    pmt->SetSampleSize(0);

	WAVEFORMATEXTENSIBLE*	pwfex;
	WAVEFORMATEX*			pwfx;

	if (pvfx->nChannels == 1 || pvfx->nChannels == 2) // Standard waveformatex
	{
		pwfx					= (WAVEFORMATEX*) pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));

		pwfx->wFormatTag		= bFloat ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
		pwfx->cbSize			= 0;
	}
	else
	{
		pwfex					= (WAVEFORMATEXTENSIBLE*) pmt->AllocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE));

		pwfex->SubFormat		= bFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
		pwfex->Samples.wValidBitsPerSample = bFloat ? 32 : 16;
		pwfex->dwChannelMask	= 0;
		pwfx					= &pwfex->Format;
		pwfx->wFormatTag		= WAVE_FORMAT_EXTENSIBLE;
		pwfx->cbSize			= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	}

	pwfx->nChannels				= pvfx->nChannels;
	pwfx->wBitsPerSample		= bFloat ? 32 : 16;
	pwfx->nSamplesPerSec		= pvfx->nSamplesPerSec;
	pwfx->nBlockAlign			= pwfx->nChannels * (pwfx->wBitsPerSample>>3);
	pwfx->nAvgBytesPerSec		= pwfx->nChannels * (pwfx->wBitsPerSample>>3) * pwfx->nSamplesPerSec;

	return NOERROR;
} // GetMediaType

HRESULT	CVorbisDec::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
	if (direction == PINDIR_INPUT) return NOERROR;
	if (m_pMapping) delete [] m_pMapping;

	WAVEFORMATEX*	pwfx = (WAVEFORMATEX* ) pmt->Format();

	m_bFloatMode = pwfx->wBitsPerSample == 32;
	m_pMapping = new int[pwfx->nChannels];

	if (pwfx->nChannels != 6)
	{
		for (int i=0; i<pwfx->nChannels; i++)
			m_pMapping[i] = i;
	}
	else
	{
		m_pMapping[0] = 0;
		m_pMapping[1] = 2;
		m_pMapping[2] = 1;
		m_pMapping[3] = 4;
		m_pMapping[4] = 5;
		m_pMapping[5] = 3;
	}

	return NOERROR;
}

HRESULT CVorbisDec::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	if ((mtOut->majortype != MEDIATYPE_Audio) ||
		(mtOut->subtype != MEDIASUBTYPE_PCM)) return VFW_E_TYPE_NOT_ACCEPTED;

	LPPCMWAVEFORMAT		pwfx = (LPPCMWAVEFORMAT) mtOut->Format();

	if (pwfx->wf.nSamplesPerSec < 1) return VFW_E_TYPE_NOT_ACCEPTED;

	return CheckInputType(mtIn);
}

// Provide the way for COM to create a CVorbisDec
CUnknown *CVorbisDec::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CVorbisDec *pNewObject = new CVorbisDec(NAME("Vorbis Decoder"), punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
} // CreateInstance

STDMETHODIMP CVorbisDecInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
	memset(pProps, 0, sizeof(*pProps));
	pProps->cBuffers = 2;
	return NOERROR;
}

// IPropertyBag methods
HRESULT STDMETHODCALLTYPE CVorbisDec::Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog)
{
	char					szName[128];
	WAVEFORMATEXTENSIBLE*	pwfx = NULL;

	wcstombs(szName, pszPropName, sizeof(szName));
	if (*(m_pOutput->CurrentMediaType().FormatType()) == FORMAT_WaveFormatEx)
		pwfx = (WAVEFORMATEXTENSIBLE*) m_pOutput->CurrentMediaType().Format();
	VariantClear(pVar);

	if (strcmp(szName, idFloatModeFirst) == 0)
	{
		pVar->vt = VT_I4; pVar->lVal = m_bFloatModeFirst ? 1 : 0;
		return NOERROR;
	}
	if (strcmp(szName, idPostGain) == 0)
	{
		pVar->vt = VT_R4; pVar->fltVal = m_bPostGain ? m_fPostGain : 1;
		return NOERROR;
	}
	if (strcmp(szName, idFloatModeFirst) == 0)
	{
		pVar->vt = VT_BOOL; pVar->boolVal = m_bFloatModeFirst;
		return NOERROR;
	}
	if (strcmp(szName, idChannels) == 0)
	{
		pVar->vt = VT_I4; pVar->lVal = pwfx ? pwfx->Format.nChannels : 0;
		return NOERROR;
	}
	if (strcmp(szName, idSamplesPerSec) == 0)
	{
		pVar->vt = VT_I4; pVar->lVal = pwfx ? pwfx->Format.nSamplesPerSec : 0;
		return NOERROR;
	}
	if (strcmp(szName, idOutputMode) == 0)
	{
		pVar->vt = VT_I4;

		if (!pwfx)
			pVar->lVal = -1;
		else if (pwfx->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		{
			if (pwfx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
				pVar->lVal = (long)WAVE_FORMAT_IEEE_FLOAT;
			else
				pVar->lVal = (long)WAVE_FORMAT_PCM;
		}
		else pVar->lVal = (long)pwfx->Format.wFormatTag;

		return NOERROR;
	}

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CVorbisDec::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	char					szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));

	if (strcmp(szName, idFloatModeFirst) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);
		m_bFloatModeFirst = pVar->lVal != 0;
		SaveToRegistry(szName, pVar->lVal != 0);
		return NOERROR;
	}

	return E_FAIL;
}
