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

COggStream::COggStream(int iStreamID, COggSplitter *pOggSplitter, bool bDummyStream) :
		m_evWaitForData(FALSE)
{
	m_pOggSplitter = pOggSplitter;
	m_iStreamID = iStreamID;
	m_bIdentified = false;
	m_iStreamType = cnts_UNKNOWN;
	m_pPin = NULL;

	stream_state_init(&m_ss, iStreamID);
	m_sh = NULL;
	vorbis_info_init(&m_vi);
	vorbis_comment_init(&m_vc);
	memset(&m_VorbisHeader, 0, sizeof(ogg_packet) * 3);
	m_bIsDummyStream = bDummyStream;

	if (bDummyStream)
	{
		m_mt.InitMediaType();
		m_mt.SetType(&MEDIATYPE_Text);
		m_iBufferSize = 1024;
		vorbis_comment_add_tag(&m_vc, "LANGUAGE", "off");
	}
}

COggStream::~COggStream()
{
	stream_state_clear(&m_ss);
	vorbis_info_clear(&m_vi);
	vorbis_comment_clear(&m_vc);
	if (m_sh) free(m_sh);
}

HRESULT COggStream::IdentifyType(ogg_page *og)
{
	if (m_bIdentified) return S_OK; // Everything is already done

	ogg_packet	op;
	
	ogg_stream_pagein(&m_ss.os, og);
	while (ogg_stream_packetout(&m_ss.os, &op) > 0)
	{
		if ((*op.packet & PACKET_TYPE_HEADER) == 0)
		{
			m_bIdentified = true;	// This is already a data packet
			return S_OK;			// We are done
		}

		switch (*op.packet & PACKET_TYPE_BITS)
		{
		case PACKET_TYPE_HEADER:
			if (stream_header_in(&m_sh, &op) == E_SUCCESS) // => it is our stream format
			{
				SetMTFromSH(m_sh, &m_mt, &m_iLowWatermark, &m_iHighWatermark);
				m_iBufferSize = m_sh->buffersize;

				m_vi.rate = 1; // otherwhise vorbis_synthesis_headerin will fail
				m_iStreamType = cnts_OGGSTREAM;
			}				
			else if (vorbis_synthesis_headerin(&m_vi, &m_vc, &op) >= 0) // => it is Ogg/Vorbis
			{
				CopyOggPacket(&m_VorbisHeader[0], &op);

				SetMTFromVI(&m_vi, &m_mt);
				m_iBufferSize = 8192; // Vorbis packets shoudn't contain more than
				
				m_iHighWatermark = 32;
				m_iLowWatermark = 16;

				m_iStreamType = cnts_VORBIS;
			}
			else // No idea what's that
			{
				m_mt.SetType(&MEDIATYPE_NULL);
				m_bIdentified = true;	// Unknown type
				m_iStreamType = cnts_UNKNOWN;
				return S_FALSE;
			}
			break;

		case PACKET_TYPE_COMMENT:
			vorbis_synthesis_headerin(&m_vi, &m_vc, &op);
			CopyOggPacket(&m_VorbisHeader[1], &op);
			break;

		case PACKET_TYPE_CODEBOOK:
			if (m_iStreamType == cnts_VORBIS)
			{
				vorbis_synthesis_headerin(&m_vi, &m_vc, &op);
				CopyOggPacket(&m_VorbisHeader[2], &op);
				m_bIdentified = true;
				return S_OK;
			}
		} // case
	}
	return S_OK;
}

bool COggStream::PageInFromQueue()
{
	tPageNode*	pPage;

	pPage = OggPageDequeue();
	if (!pPage) return false;

	ogg_stream_pagein(&m_ss.os, &pPage->og);
	OggPageRelease(pPage);

	if (OggPageQueuePages() < m_iLowWatermark)
		m_pOggSplitter->m_pInput->NotifyLowFilllevel();

	return true;
}

// Add a page in the list and return if HighWatermark was reached
bool COggStream::DeliverPage(ogg_page *pog)
{
	// If the group is not connected we are not interested in the page
	if (!(m_pPin->IsConnected())) return false;

	if (m_bAfterReset)
	{
		if (ogg_page_packets(pog) == 0) return false;  // This would cause a problem in the Ogg lib
		m_bAfterReset = false;
	}

	OggPageEnqueue(pog, NULL);
	m_evWaitForData.Set();			// Signal the stream thread that we have received a page

	return OggPageQueuePages() >= m_iHighWatermark;
}

HRESULT COggStream::Active()
{
	if (m_pOggSplitter->IsActive())
		return S_FALSE;
    if (!m_pPin->IsConnected())
		return NOERROR;
	if (!Create())
		return E_FAIL;
	CallWorker(CMD_RUN);

	return NOERROR;
}

HRESULT COggStream::Inactive()
{
	if (!ThreadExists())
		return NOERROR;

	m_bAbort = true;
	m_evWaitForData.Set();		// signal the events to be sure that
	m_evWaitForGroup.Set();		//  the thread is not blocked anymore
	CallWorker(CMD_EXIT);
	Close();		// Wait for the thread to exit, then tidy up.

	OggPageQueueFlush();
	return NOERROR;
}

HRESULT COggStream::BeginFlush()
{
    if (!ThreadExists())
		return NOERROR;

	m_bAbort = true;
	m_evWaitForData.Set();
	m_evWaitForGroup.Set();
	CallWorker(CMD_STOP);
	return NOERROR;
}

HRESULT COggStream::EndFlush()
{
	OggPageQueueFlush();

	if (ThreadExists())
		CallWorker(CMD_RUN);

	return NOERROR;
}

DWORD COggStream::ThreadProc(void)
{
    Command com;
    do
	{
		com = (Command)GetRequest();

		switch (com)
		{
		case CMD_EXIT:
		    Reply(NOERROR);
			break;

		case CMD_STOP:
		    Reply(NOERROR);
		    break;

		case CMD_RUN:
			if (!m_bIsDummyStream)
				SendSampleLoop();
			else
				SendDummySampleLoop();
			break;

		default:
		    Reply((DWORD) E_NOTIMPL);
			break;
		}
	} while (com != CMD_EXIT);

	return NOERROR;
}

bool COggStream::GetSample(REFERENCE_TIME* prtStart, REFERENCE_TIME* prtStop,
						  ogg_int64_t* pmtStart, ogg_int64_t* pmtStop,
						  bool* pbSyncPoint, bool* pbEOS,
						  unsigned char** ppbBuffer, int* pcbBuffer)
{
	do
	{
		int	or;

		if (m_iStreamType == cnts_VORBIS)
		{
			or = stream_sampleout_vorbis(&m_ss, &m_vi, pbEOS,
										  pmtStart, prtStart, ppbBuffer, pcbBuffer);
			*prtStop = *prtStart;
			*pmtStop = *pmtStart;
			*pbSyncPoint = true;
		}
		else
		{
			or = stream_sampleout(&m_ss, m_sh, pbEOS, pbSyncPoint,
										 pmtStart, pmtStop, prtStart, prtStop, ppbBuffer, pcbBuffer);
			*pmtStop += *pmtStart;
		}

		if (or > 0)
			return true;							// Got a sample => we are done

		if (or == 0)
			if (!PageInFromQueue())
				return false;						// If there are no more pages in the queue then
													// there is no sample available
	
	} while(true);
}

void COggStream::SendDummySample()
{
	IMediaSample*	pSample;
	BYTE*			pSampBuffer;
	REFERENCE_TIME	rtStart = 0;
	REFERENCE_TIME	rtStop = 1;

	if FAILED(m_pPin->GetDeliveryBuffer(&pSample,NULL,NULL, 0)) return;

	pSample->GetPointer(&pSampBuffer);
	*pSampBuffer = 0;
	pSample->SetActualDataLength(1);

	pSample->SetTime(&rtStart,&rtStop);
	pSample->SetPreroll(FALSE);
	pSample->SetDiscontinuity(TRUE);
	pSample->SetSyncPoint(TRUE);
	m_pPin->Deliver(pSample);
	pSample->Release();
}

HRESULT COggStream::SendSampleLoop()
{
    HRESULT			hr;
	Command			com;

	stream_state_reset(&m_ss);
	m_bAfterReset = true;

	m_bAbort = false;
	m_evWaitForData.Reset();
	m_evWaitForGroup.Reset();
	m_bDiscontinuity = true;

	REFERENCE_TIME	rtSegLen = m_pOggSplitter->m_rtStop - m_pOggSplitter->m_rtStart;

    Reply(NOERROR);

	if (m_bEnabled)
	{
		m_pPin->DeliverNewSegment(m_pOggSplitter->m_rtStart, m_pOggSplitter->m_rtStop,
								  m_pOggSplitter->m_dRate);
		// If there is currently no subtitle then no sample is sent to the 
		// subsequent filter. But DirectShow expect at least one sample to
		// go into pause state and the graph would block. This is a work around
		// We send just a 100ns sample with just a 0 Byte.
		if (m_iStreamType == cnts_OGGSTREAM)
			if (strcmp(m_sh->streamtype, "text") == 0)
				SendDummySample();
	}

	do
	{
		REFERENCE_TIME	rtLastPos;
		REFERENCE_TIME	rtStart, rtStop;
		__int64			mtStart, mtStop;

		bool			bSyncPoint;
		bool			bEOS;

		unsigned char	*pbBuffer;
		int				cbBuffer;

		if (GetSample(&rtStart, &rtStop, &mtStart, &mtStop, &bSyncPoint, &bEOS, &pbBuffer, &cbBuffer))
		{
			if (!m_bDiscontinuity || bSyncPoint)  // => we have valid times
			{
				rtStart -= m_pOggSplitter->m_rtStart;
				rtStop  -= m_pOggSplitter->m_rtStart;
				m_pPin->GetLastPos(&rtLastPos);

				if ((rtStart >= 0) && (rtStart > rtLastPos))
				{
					if (m_bEnabled)
					{
						if (m_bDiscontinuity && m_iStreamType == cnts_VORBIS)
							m_pPin->SendVorbisHeaderPackets(&(m_VorbisHeader[0]));
						
						IMediaSample*	pSample;
						BYTE*			pSampBuffer;

						hr = m_pPin->GetDeliveryBuffer(&pSample,NULL,NULL, bSyncPoint ? 0 : AM_GBF_NOTASYNCPOINT);
						if FAILED(hr) return hr;

						pSample->GetPointer(&pSampBuffer);
						memcpy(pSampBuffer, pbBuffer, cbBuffer);
						pSample->SetActualDataLength(cbBuffer);

						pSample->SetTime(&rtStart,&rtStop);
						pSample->SetMediaTime(&mtStart, &mtStop);
						pSample->SetPreroll(FALSE);
						pSample->SetDiscontinuity(m_bDiscontinuity ? TRUE : FALSE);
						pSample->SetSyncPoint(bSyncPoint ? TRUE : FALSE);
						m_bDiscontinuity = false;
						hr = m_pPin->Deliver(pSample);
						pSample->Release();
						if FAILED(hr) return hr;

						// inform the other streams ...
						m_pOggSplitter->NotifyGroup(m_iGroupID, m_pPin, rtStart);

						if ((rtStop > rtSegLen) || bEOS)
							return m_pPin->DeliverEndOfStream();

					}
					
					else // Stream is not enabled
					{
						m_evWaitForGroup.Wait(INFINITE);
						m_bDiscontinuity = TRUE;
						if (rtStop > rtSegLen)
							return NOERROR;
					}
				}
			}
		}

		else 
		{
			// There was no sample available
			if (m_pOggSplitter->IsEOF())
			{
				if (m_bEnabled)
					return m_pPin->DeliverEndOfStream();
				else return NOERROR;
			}
			m_evWaitForData.Wait(INFINITE); // wait until we get more data
		}

	} while (!CheckRequest((DWORD*)&com) && !m_bAbort);
    return NOERROR;
}

//  Just sends empty samples
//  Used for the dummy subtitle stream otherwise
//  The internal script renderer would block
HRESULT COggStream::SendDummySampleLoop()
{
    HRESULT			hr;
	Command			com;
	REFERENCE_TIME	rtStreamStart, rtStreamStop;
	double			dRate;

	m_bAbort = false;
	m_evWaitForGroup.Reset();
	m_bDiscontinuity = TRUE;

	m_pOggSplitter->GetPositions(&rtStreamStart, &rtStreamStop);
	m_pOggSplitter->GetRate(&dRate);
    Reply(NOERROR);

	if (m_bEnabled)
		m_pPin->DeliverNewSegment(rtStreamStart, rtStreamStop, dRate);

	REFERENCE_TIME	rtStart, rtStop;

	rtStart = 0;

	do
	{
		rtStop = rtStart + SEC_IN_REFTIME;

		REFERENCE_TIME rtLastPos;
		m_pPin->GetLastPos(&rtLastPos);

		if (m_bEnabled)
		{
			if (rtStart > rtLastPos)
			{
				IMediaSample *pSample;
				BYTE*		pSampBuffer;
	
				hr = m_pPin->GetDeliveryBuffer(&pSample,NULL,NULL,0);
				if FAILED(hr) return (hr);

				pSample->GetPointer(&pSampBuffer);
				*pSampBuffer = '\0';
				
				pSample->SetDiscontinuity(m_bDiscontinuity);
				pSample->SetSyncPoint(true);
				pSample->SetTime(&rtStart,&rtStop);
				pSample->SetPreroll(rtStart < 0);
				pSample->SetActualDataLength(1);
				hr = m_pPin->Deliver(pSample);
				pSample->Release();
				if FAILED(hr) return (hr);
				m_bDiscontinuity = FALSE;

				// inform the other streams ...
				m_pOggSplitter->NotifyGroup(m_iGroupID, m_pPin, rtStart);
			}

			if ((rtStop > (rtStreamStop - rtStreamStart)) || m_pOggSplitter->IsEOF())
				return m_pPin->DeliverEndOfStream();
		}
		else // Not enabled
		{
			if (rtStart > rtLastPos)
				m_evWaitForGroup.Wait(INFINITE);
			m_bDiscontinuity = TRUE;
			if ((rtStop > (rtStreamStop - rtStreamStart)) || m_pOggSplitter->IsEOF())
				return NOERROR;
		}

		rtStart = rtStop;

	} while (!CheckRequest((DWORD*)&com) && !m_bAbort);
    return NOERROR;
}

void COggStream::Enable(bool bEnabled)
{
	m_bDiscontinuity = TRUE;
	m_bEnabled = bEnabled;
	if (m_pPin)
		m_pPin->m_pStream = this;
}

__int64	COggStream::MediaTimeToRefTime(__int64 iMediaTime)
{
	if (m_iStreamType == cnts_UNKNOWN)
		return -1;
	else if (m_iStreamType == cnts_VORBIS)
		return mediatime_to_reference_time(SEC_IN_REFTIME, m_vi.rate, iMediaTime);
	return mediatime_to_reference_time(m_sh->time_unit, m_sh->samples_per_unit, iMediaTime);
}

// This is the fourcc mapper feature
void COggStream::TranslateFourCC(char* FOURCC)
{
	HKEY	hReg;
	HKEY	hFourCCReg;

	hReg = m_pOggSplitter->OpenRegistry();
	if (!hReg)	return;

	if (RegOpenKeyEx(hReg, idFourCCMapping, 0, KEY_READ, &hFourCCReg) != ERROR_SUCCESS)
	{
		RegCloseKey(hReg);
		return;
	}

	// Now the registry is opened

	char	srcFOURCC[11];
	DWORD	dstFOURCC;
	DWORD*	pdwFOURCC = (DWORD*) FOURCC;
	DWORD	dwSize;
	int		iReturn;

	wsprintf(srcFOURCC, "0x%08x", *pdwFOURCC);
	dstFOURCC = *pdwFOURCC;

	do
	{
		dwSize = sizeof(dstFOURCC);

		iReturn = RegQueryValueEx(hFourCCReg, srcFOURCC, NULL, NULL, (BYTE*)&dstFOURCC, &dwSize);
		if (iReturn == ERROR_SUCCESS)
		{
			if (dstFOURCC == *pdwFOURCC)  // To avoid circular conversions
			{
				*pdwFOURCC = strtol(srcFOURCC, NULL, 16);
				RegCloseKey(hFourCCReg);
				RegCloseKey(hReg);
				return;
			}
		}
			
		if (iReturn != ERROR_SUCCESS)
		{
			// There was no entry => we are done
			*pdwFOURCC = dstFOURCC;
			RegCloseKey(hFourCCReg);
			RegCloseKey(hReg);
			return;
		}

		wsprintf(srcFOURCC, "0x%08x", dstFOURCC);
	} while (true);
}

void COggStream::SetMTFromSH(stream_header *sh, CMediaType *pmt, int* pHighWM, int* pLowWM)
{
	if (strncmp((char*)&sh->streamtype, MT_Video, strlen(MT_Video)) == 0)
	{
		TranslateFourCC(&sh->subtype[0]);
		
		pmt->InitMediaType();
		pmt->SetType(&MEDIATYPE_Video);
		pmt->SetSubtype(&MEDIASUBTYPE_YVYU);
		pmt->subtype.Data1 = *(ogg_int32_t*)&sh->subtype;

		if (*(pmt->Subtype()) != MEDIASUBTYPE_RGB565 &&
			*(pmt->Subtype()) != MEDIASUBTYPE_RGB555 &&
			*(pmt->Subtype()) != MEDIASUBTYPE_RGB24 &&
			*(pmt->Subtype()) != MEDIASUBTYPE_RGB32 &&
			*(pmt->Subtype()) != MEDIASUBTYPE_ARGB32 && 
			*(pmt->Subtype()) != MEDIASUBTYPE_YUY2 &&
			*(pmt->Subtype()) != MEDIASUBTYPE_UYVY &&
			*(pmt->Subtype()) != MEDIASUBTYPE_YVYU)
		{
			pmt->SetTemporalCompression(TRUE);
			pmt->SetVariableSize();
		}
		pmt->SetFormatType(&FORMAT_VideoInfo);
		VIDEOINFO* pvi = (VIDEOINFO*)pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
		memset(pvi, 0, sizeof(*pvi));
		pvi->AvgTimePerFrame = sh->time_unit;
		pvi->bmiHeader.biBitCount = sh->bits_per_sample;
		pvi->bmiHeader.biPlanes = 1;
		pvi->bmiHeader.biSize = sizeof(pvi->bmiHeader);
		pvi->bmiHeader.biWidth = sh->video.width;
		pvi->bmiHeader.biHeight = sh->video.height;
		pvi->bmiHeader.biCompression = *(ogg_int32_t*)&sh->subtype;

		*pHighWM = sh->buffersize * 10 / 4096;
		*pLowWM  = *pHighWM / 2;
		
		return;
	}

	if (strncmp((char*)&sh->streamtype, MT_Audio, strlen(MT_Audio)) == 0)
	{
		unsigned __int16	dwFormatTag = 0;
		unsigned __int16	mult = 0x1000;

		for (int i=0; i<4; i++)
		{
			if (sh->subtype[i] >= '0' && sh->subtype[i] <= '9')
				dwFormatTag += (sh->subtype[i] - '0') * mult;
			else if (sh->subtype[i] >= 'A' && sh->subtype[i] <= 'F')
				dwFormatTag += (sh->subtype[i] - 'A' + 10) * mult;
			else if (sh->subtype[i] >= 'a' && sh->subtype[i] <= 'f')
				dwFormatTag += (sh->subtype[i] - 'a' + 10) * mult;
			mult >>= 4;
		}
		
		pmt->InitMediaType();
		pmt->SetType(&MEDIATYPE_Audio);
		pmt->SetSubtype(&MEDIASUBTYPE_PCM);
		pmt->subtype.Data1 = dwFormatTag;

		pmt->SetFormatType(&FORMAT_WaveFormatEx);

		int				extralen = sh->size - sizeof(stream_header);
		WAVEFORMATEX*	pwfx = (WAVEFORMATEX*)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX) + extralen);
		memset(pwfx, 0, sizeof(WAVEFORMATEX));

		pwfx->cbSize = extralen;
		memcpy(pwfx+1, sh+1, extralen);

		pwfx->nAvgBytesPerSec = sh->audio.avgbytespersec;
		pwfx->nBlockAlign = sh->audio.blockalign;
		pwfx->nChannels = sh->audio.channels;
		pwfx->nSamplesPerSec = (__int32)sh->samples_per_unit;
		pwfx->wBitsPerSample = sh->bits_per_sample;
		pwfx->wFormatTag = dwFormatTag;
	
		*pHighWM = sh->buffersize * 10 / 4096;
		*pLowWM  = *pHighWM / 2;

		return;
	}

	if (strncmp((char*)&sh->streamtype, MT_Text, strlen(MT_Text)) == 0)
	{
		pmt->InitMediaType();
		pmt->SetType(&MEDIATYPE_Text);

		*pHighWM = 20;
		*pLowWM  = -1;
		return;
	}
}

void COggStream::SetMTFromVI(vorbis_info *vi, CMediaType *pmt)
{
	pmt->InitMediaType();
	pmt->SetType(&MEDIATYPE_Audio);
	pmt->SetSubtype(&MEDIASUBTYPE_Vorbis);
	pmt->SetFormatType(&FORMAT_VorbisFormat);

	VORBISFORMAT *pfmt = (VORBISFORMAT*) (pmt->AllocFormatBuffer(sizeof(VORBISFORMAT)));

	pfmt->nChannels			= vi->channels;
	pfmt->nSamplesPerSec	= vi->rate;
	pfmt->nMinBitsPerSec	= vi->bitrate_lower;
	pfmt->nAvgBitsPerSec	= vi->bitrate_nominal;
	pfmt->nMaxBitsPerSec	= vi->bitrate_upper;
}

// used by the seek function
bool COggStream::FindKeyFrame(ogg_page *og, CRefTime *rtFrame)
{
	bool			bSyncPoint;
	REFERENCE_TIME	rtStart;

	ogg_stream_pagein(&m_ss.os, og);

	while (stream_sampleout(&m_ss, m_sh, NULL, &bSyncPoint, NULL, NULL,
								   &rtStart, NULL, NULL, NULL) > 0)
	{
		if (bSyncPoint && (rtStart >= *rtFrame))
		{
			*rtFrame = rtStart;
			return true;
		}
	}
	return false;		
}
