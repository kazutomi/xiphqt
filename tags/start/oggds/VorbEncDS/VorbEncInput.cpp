#include "VorbEncDS.h"
#include "../OggDS.h"
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <stdio.h>

CVorbisEncInputPin::CVorbisEncInputPin(TCHAR* pObjectName, CVorbisEnc* pFilter, CCritSec* pEncode,
									   CCritSec* pLock, HRESULT *phr, LPCWSTR pName) :
	CBaseInputPin(pObjectName, pFilter, pLock, phr, pName)
{
	m_pVorbisEnc = pFilter;
	m_pcsEncode = pEncode;
};

HRESULT CVorbisEncInputPin::CompleteConnect(IPin* pReceivePin)
{
	HRESULT hr = CBasePin::CompleteConnect(pReceivePin);
	if FAILED(hr) return hr;

//  This might be for multiple input pins in future
	m_pVorbisEnc->CheckFreeInputPin();

	return m_pVorbisEnc->ReconnectOutput();
}

HRESULT CVorbisEncInputPin::BreakConnect()
{
	HRESULT hr = CBasePin::BreakConnect();
	if FAILED(hr) return hr;
	
	WAVEFORMATEX*	pwfx = (WAVEFORMATEX*)m_mt.Format();
	pwfx->nChannels = 0;
	return m_pVorbisEnc->ReconnectOutput();
}


HRESULT CVorbisEncInputPin::CheckMediaType(const CMediaType *pmt)
{
	if (*(pmt->FormatType())	!= FORMAT_WaveFormatEx) return VFW_E_TYPE_NOT_ACCEPTED;

	WAVEFORMATEX*	pwfx = (WAVEFORMATEX*) pmt->Format();

	if (pwfx->wFormatTag != WAVE_FORMAT_PCM &&
		pwfx->wFormatTag != WAVE_FORMAT_IEEE_FLOAT &&
		pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE) return VFW_E_TYPE_NOT_ACCEPTED;

	CMediaType*	pmtCurr;

	if (!m_pVorbisEnc->GetCurrentMediaType(&pmtCurr)) return NOERROR;

	// if there is already a connected pin the sample rate must be the same
	WAVEFORMATEX*	pwfxcurr = (WAVEFORMATEX*) pmtCurr->Format();

	if (pwfx->nSamplesPerSec != pwfxcurr->nSamplesPerSec) return VFW_E_TYPE_NOT_ACCEPTED;

	return NOERROR;
}

HRESULT CVorbisEncInputPin::Active()
{
	m_bEOSReceived = false;
	m_bBufferFilled = false;
	m_iSamplesWritten = 0;
	return NOERROR;
}

STDMETHODIMP CVorbisEncInputPin::EndOfStream(void)
{
	m_bEOSReceived = true;
	return m_pVorbisEnc->EndOfStream();
}

STDMETHODIMP CVorbisEncInputPin::Receive(IMediaSample *pSample)
{
	// Samples written contains the number of samples written to the vorbis buffer
	HRESULT hr = NOERROR;
    CAutoLock lck(&m_csReceive);  // Serialize Receive() on this pin

    // check all is well with the base class
    hr = CBaseInputPin::Receive(pSample);
	if FAILED(hr) return hr;

    if (m_SampleProps.dwStreamId != AM_STREAM_MEDIA)
        return NOERROR;

	WAVEFORMATEX*	pwfx = (WAVEFORMATEX*)m_mt.Format();

	ogg_int64_t	tSample = reference_time_to_mediatime(SEC_IN_REFTIME,
										pwfx->nSamplesPerSec, m_SampleProps.tStart);
	int			iSamplesRead  = 0;
	int			iSamplesAvail = m_SampleProps.lActual / pwfx->nChannels / (pwfx->wBitsPerSample>>3);

	do
	{
		float**		ppfBuffer;
		ogg_int64_t	tBuffer;

		m_pVorbisEnc->GetBuffer(&ppfBuffer, &tBuffer);
   
		// If the buffer pos is before this sample we must pad with 0
		while ((m_iSamplesWritten<ENCODE_BUFFER_SIZE) &&
			   (m_iSamplesWritten-iSamplesRead < tSample-tBuffer))
		{
deb("Padding");
			for (int iChannel=m_iFirstChannel; iChannel<m_iFirstChannel+pwfx->nChannels; iChannel++)
				ppfBuffer[iChannel][m_iSamplesWritten] = 0;
			m_iSamplesWritten++;
		}

		// If the buffer pos if after this sample we must skip some samples
		while ((iSamplesRead<iSamplesAvail) &&
			   (m_iSamplesWritten-iSamplesRead > tSample-tBuffer))
		{		
deb("Adjusting");
			iSamplesRead++;
		}

		// Fill the buffer 
		if (pwfx->wBitsPerSample == 32)  // 32bit => float data
		{
			float*	pInBuffer = ((float*)m_SampleProps.pbBuffer)+ (iSamplesRead * pwfx->nChannels);

			while((m_iSamplesWritten<ENCODE_BUFFER_SIZE) && (iSamplesRead<iSamplesAvail))
			{
				for (int iChannel=m_iFirstChannel; iChannel<m_iFirstChannel+pwfx->nChannels; iChannel++)
				{
					ppfBuffer[iChannel][m_iSamplesWritten] = *pInBuffer;
					*pInBuffer++;
				}
				iSamplesRead++;
				m_iSamplesWritten++;
			}
		}
		else
		{
			ogg_int16_t*	pInBuffer = ((ogg_int16_t*)m_SampleProps.pbBuffer)+ (iSamplesRead * pwfx->nChannels);

			while((m_iSamplesWritten<ENCODE_BUFFER_SIZE) && (iSamplesRead<iSamplesAvail))
			{
				for (int iChannel=m_iFirstChannel; iChannel<m_iFirstChannel+pwfx->nChannels; iChannel++)
				{
					ppfBuffer[iChannel][m_iSamplesWritten] = *pInBuffer / 32768.f;
					*pInBuffer++;
				}
				iSamplesRead++;
				m_iSamplesWritten++;
			}
		}

		if (m_iSamplesWritten == ENCODE_BUFFER_SIZE)
		{
			if (!m_pVorbisEnc->ProcessBuffer())  // false means we have to wait for other pins
				m_evWaitBuffer.Wait(INFINITE);
			else
				m_evWaitBuffer.Reset();

			m_iSamplesWritten = 0;
		}

	} while (iSamplesRead < iSamplesAvail); // Not already all samples used

	return NOERROR;
}
