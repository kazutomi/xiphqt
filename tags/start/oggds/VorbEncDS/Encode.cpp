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

HRESULT CVorbisEnc::StopStreaming()
{
	// Cleanup resources ...
	vorbis_block_clear(&m_vb);
	vorbis_dsp_clear(&m_vd);
	vorbis_comment_clear(&m_vc);
	vorbis_info_clear(&m_vi);
	return NOERROR;
}

HRESULT CVorbisEnc::StartStreaming()
{
	VORBISFORMAT	*pvfx = (VORBISFORMAT*) m_pOutput->CurrentMediaType().Format();

	vorbis_info_init(&m_vi);
	if (m_fQuality >= 0 && m_fQuality <= 10)
	{
		if (vorbis_encode_init_vbr(&m_vi,
								pvfx->nChannels,
								pvfx->nSamplesPerSec,
								m_fQuality) < 0)
			return S_FALSE;
	}
	else
	{
		if (vorbis_encode_init(&m_vi,
							pvfx->nChannels,
							pvfx->nSamplesPerSec,
							pvfx->nMinBitsPerSec,
							pvfx->nAvgBitsPerSec,
							pvfx->nMaxBitsPerSec) <0)
			return S_FALSE;
	}

	vorbis_comment_init(&m_vc);
	vorbis_analysis_init(&m_vd, &m_vi);
	vorbis_block_init(&m_vd, &m_vb);

	m_bHeaderSent = false;
	m_tBuffer = 0;
	return NOERROR;
}

HRESULT CVorbisEnc::SendHeader()
{
	IMediaSample	*pSample;
	BYTE			*buffer;
	REFERENCE_TIME	start = 0;
	REFERENCE_TIME	stop = 0;
	ogg_packet		hd[3];

	vorbis_analysis_headerout(&m_vd, &m_vc, &hd[0], &hd[1], &hd[2]);

	for (int i=0; i<3; i++)
	{
		if SUCCEEDED(m_pOutput->GetDeliveryBuffer(&pSample, &start, &stop, 0))
		{
			pSample->GetPointer(&buffer);
			if (hd[i].bytes > pSample->GetSize()) return VFW_E_BUFFER_OVERFLOW;
			memcpy(buffer, hd[i].packet, hd[i].bytes);
			pSample->SetActualDataLength(hd[i].bytes);
			pSample->SetTime(&start, &start);
			m_pOutput->Deliver(pSample);
			pSample->Release();
		}
	}

	return NOERROR;
}

HRESULT CVorbisEnc::EndOfStream(void)
{
	CAutoLock	lock(&m_csEncode);

	vorbis_analysis_wrote(&m_vd, 0);
	Encode();

	return CTransformFilter::EndOfStream();
}

HRESULT CVorbisEnc::Encode()
{
    HRESULT			hr = NOERROR;
	IMediaSample	*pSample;
	BYTE			*buffer;
	REFERENCE_TIME	start;
    REFERENCE_TIME	stop;

	if (!m_bHeaderSent)
	{
		hr = SendHeader();
		if FAILED(hr) return hr;
		m_bHeaderSent = true;
	}

	while (vorbis_analysis_blockout(&m_vd, &m_vb) == 1)
	{
		ogg_packet		op;

		vorbis_analysis(&m_vb, NULL);
		vorbis_bitrate_addblock(&m_vb);
		while (vorbis_bitrate_flushpacket(&m_vd, &op))
		{
			start = mediatime_to_reference_time(SEC_IN_REFTIME, m_vi.rate, op.granulepos);
			stop  = mediatime_to_reference_time(SEC_IN_REFTIME, m_vi.rate, m_vd.granulepos);
			if FAILED(hr = m_pOutput->GetDeliveryBuffer(&pSample,
									&start, &stop, AM_GBF_NOTASYNCPOINT)) return hr;
			// Buffer overflow shoudn't occur but to be on the safe side ...
			if (op.bytes > pSample->GetSize()) return VFW_E_BUFFER_OVERFLOW;
			pSample->SetTime(&start, &stop);
			pSample->GetPointer(&buffer);				
			pSample->SetDiscontinuity(false);
			memcpy(buffer, op.packet, op.bytes);
			pSample->SetActualDataLength(op.bytes);

			if FAILED(hr = m_pOutput->Deliver(pSample)) return hr;
			pSample->Release();
		}
	}
	return hr;
}

HRESULT CVorbisEnc::Receive(IMediaSample* pSample)
{
    //  Check for other streams and pass them on
    AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
    if (pProps->dwStreamId != AM_STREAM_MEDIA)
        return m_pOutput->Deliver(pSample);

	WAVEFORMATEX*	pwfx = (WAVEFORMATEX*)m_pInput->CurrentMediaType().Format();
	REFERENCE_TIME	rtStart, rtStop;
	ogg_int64_t		tSample;
	void*			pInBuffer;

	pSample->GetTime(&rtStart, &rtStop);
	tSample = reference_time_to_mediatime(SEC_IN_REFTIME, pwfx->nSamplesPerSec, rtStart);
	pSample->GetPointer((BYTE**)&pInBuffer);

	int			iSamplesRead = 0;
	int			iSamplesAvail = pSample->GetActualDataLength() / pwfx->nChannels / (pwfx->wBitsPerSample>>3);

	// If the buffer pos if after this sample we must skip some samples
	while (m_tBuffer > tSample + iSamplesRead)
	{		
		iSamplesRead++;
		if (iSamplesRead == iSamplesAvail) return NOERROR; // No samples left
	}

	while (iSamplesRead < iSamplesAvail)
	{
		int			iSamplesWritten = 0;
		float**		ppfBuffer = vorbis_analysis_buffer(&m_vd, ENCODE_BUFFER_SIZE);

		// If the buffer pos is before this sample we must pad with 0
		while ((iSamplesWritten<ENCODE_BUFFER_SIZE) &&
			   (iSamplesWritten-iSamplesRead < tSample-m_tBuffer))
		{
			for (int iChannel=0; iChannel<pwfx->nChannels; iChannel++)
				ppfBuffer[iChannel][iSamplesWritten] = 0;
			iSamplesWritten++;
		}

		// Fill the buffer 
		if (pwfx->wBitsPerSample == 32)  // 32bit => float data
		{
			float*	pFloatBuffer = ((float*)pInBuffer)+ (iSamplesRead * pwfx->nChannels);

			while((iSamplesWritten<ENCODE_BUFFER_SIZE) && (iSamplesRead<iSamplesAvail))
			{
				for (int iChannel=0; iChannel<pwfx->nChannels; iChannel++)
				{
					ppfBuffer[m_pMapping[iChannel]][iSamplesWritten] = *pFloatBuffer;
					pFloatBuffer++;
				}
				iSamplesRead++;
				iSamplesWritten++;
			}
		}
		else if (pwfx->wBitsPerSample == 16)  // 16bit => 16bit integer
		{
			__int16*	pInt16Buffer = ((__int16*)pInBuffer)+ (iSamplesRead * pwfx->nChannels);

			while((iSamplesWritten<ENCODE_BUFFER_SIZE) && (iSamplesRead<iSamplesAvail))
			{
				for (int iChannel=0; iChannel<pwfx->nChannels; iChannel++)
				{
					ppfBuffer[m_pMapping[iChannel]][iSamplesWritten] = *pInt16Buffer / 32768.f;
					pInt16Buffer++;
				}
				iSamplesRead++;
				iSamplesWritten++;
			}
		}

		m_tBuffer += iSamplesWritten;
		vorbis_analysis_wrote(&m_vd, iSamplesWritten);
		Encode();
	}

	return NOERROR;
}
