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

//
//  We only offer the current position in IMediaSeeking
//  This enables applications to show the progress ...
//
#include "OggMuxDS.h"

const DWORD OGG_MUXER_SEEKING_CAPS =	AM_SEEKING_CanGetCurrentPos |
										AM_SEEKING_CanGetStopPos |
										AM_SEEKING_CanGetDuration;


// We only offer reference time and byte
// Byte mode was added to support stopping at a particular file size
STDMETHODIMP COggMux::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    if (*pFormat != TIME_FORMAT_MEDIA_TIME &&
		*pFormat != TIME_FORMAT_BYTE) return S_FALSE;
	return S_OK;
}

STDMETHODIMP COggMux::QueryPreferredFormat(GUID* pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP COggMux::SetTimeFormat(const GUID* pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    if (*pFormat != TIME_FORMAT_MEDIA_TIME &&
		*pFormat != TIME_FORMAT_BYTE) return S_FALSE;
	m_TimeCode = *pFormat;
	return S_OK;
}

STDMETHODIMP COggMux::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    return *pFormat == m_TimeCode ? S_OK : S_FALSE;
}

STDMETHODIMP COggMux::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = m_TimeCode;
    return S_OK;
}

STDMETHODIMP COggMux::GetCapabilities( DWORD * pCapabilities )
{
    CheckPointer(pCapabilities, E_POINTER);
    *pCapabilities = OGG_MUXER_SEEKING_CAPS;
    return S_OK;
}

STDMETHODIMP COggMux::CheckCapabilities( DWORD * pCapabilities)
{
    CheckPointer(pCapabilities, E_POINTER);
    return (~(OGG_MUXER_SEEKING_CAPS) & *pCapabilities) ? S_FALSE : S_OK;
}

STDMETHODIMP COggMux::GetCurrentPosition(LONGLONG *pCurrent)
{
    CheckPointer(pCurrent, E_POINTER);
	
	if (m_TimeCode == TIME_FORMAT_MEDIA_TIME)
	{
		CAutoLock	lock(&m_csPosition);
        *pCurrent = m_rtPosition + m_rtSegStart;
	}
	else
	{
		CAutoLock	lock(&m_pOutput->m_csFilePos);
		*pCurrent = m_pOutput->m_iFilePos;
	}
	return NOERROR;
}

STDMETHODIMP COggMux::GetDuration(LONGLONG *pDuration)
{
    CheckPointer(pDuration, E_POINTER);
    
	// unit byte doesn´t make sense with multiple source streams
	if (m_TimeCode != TIME_FORMAT_MEDIA_TIME)
		return E_FAIL;

	CAutoLock lock(m_pLock);
    BOOL	bTimeSet = FALSE;
	
	*pDuration = 0;

	// Let's ask the upstream filters ...
	for (int i=0; i<m_iInputs; i++)
	{
		if (m_paInput[i]->IsConnected())
		{
			IMediaSeeking	*pSeek = NULL;

			IPin *pPrevPin;
			m_paInput[i]->ConnectedTo(&pPrevPin);
			if SUCCEEDED(pPrevPin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek))
			{
				LONGLONG		Duration;

				pSeek->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
				if SUCCEEDED(pSeek->GetDuration(&Duration))
				{
					bTimeSet = TRUE;
					if (Duration > *pDuration) *pDuration = Duration;
					// Let's take the duration of the longest stream
				}
				pSeek->Release();
			}
			pPrevPin->Release();
		}
	}

	if (!bTimeSet) return E_NOTIMPL;
	return NOERROR;
}

STDMETHODIMP COggMux::GetStopPosition(LONGLONG *pStop)
{
	// unit byte doesn´t make sense with multiple source streams
	if (m_TimeCode != TIME_FORMAT_MEDIA_TIME)
		return E_FAIL;

	int		i = 0;
	bool	bDone = false;

	// Let's ask the upstream filters until we get the first answer ...
	while ((i<m_iInputs) && !bDone)
	{
		if (m_paInput[i]->IsConnected())
		{
			IMediaSeeking	*pSeek = NULL;

			IPin *pPrevPin;
			m_paInput[i]->ConnectedTo(&pPrevPin);
			if SUCCEEDED(pPrevPin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek))
			{
				pSeek->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
				pSeek->GetStopPosition(pStop);
				pSeek->Release();
				bDone = true;
			}
			pPrevPin->Release();
		}
		i++;
	}

	return NOERROR;
}

STDMETHODIMP COggMux::SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
								, LONGLONG * pStop,  DWORD StopFlags )
{
	// unit byte doesn´t make sense with multiple source streams
	if (m_TimeCode != TIME_FORMAT_MEDIA_TIME)
		return E_FAIL;

	// We must inform all upstream filters ...
	for (int i=0; i<m_iInputs; i++)
	{
		if (m_paInput[i]->IsConnected())
		{
			IMediaSeeking	*pSeek = NULL;

			IPin *pPrevPin;
			m_paInput[i]->ConnectedTo(&pPrevPin);
			if SUCCEEDED(pPrevPin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek))
			{
				pSeek->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
				pSeek->SetPositions(pCurrent, CurrentFlags, pStop, StopFlags);
				pSeek->Release();
			}
			pPrevPin->Release();
		}
	}
	return NOERROR;
}



STDMETHODIMP COggMux::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
	// unit byte doesn´t make sense with multiple source streams
	if (m_TimeCode != TIME_FORMAT_MEDIA_TIME)
		return E_FAIL;

	int		i = 0;
	bool	bDone = false;

	// Let's ask the upstream filters until we get the first answer ...
	while ((i<m_iInputs) && !bDone)
	{
		if (m_paInput[i]->IsConnected())
		{
			IMediaSeeking	*pSeek = NULL;

			IPin *pPrevPin;
			m_paInput[i]->ConnectedTo(&pPrevPin);
			if SUCCEEDED(pPrevPin->QueryInterface(IID_IMediaSeeking, (void**)&pSeek))
			{
				pSeek->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
				pSeek->GetStopPosition(pStop);
				pSeek->Release();
				bDone = true;
			}
			pPrevPin->Release();
		}
		i++;
	}

	return NOERROR;
}

STDMETHODIMP COggMux::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest)
{
	*pEarliest = 0;
	return GetDuration(pLatest);
}

STDMETHODIMP COggMux::ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
                           LONGLONG    Source, const GUID * pSourceFormat )
{
    CheckPointer(pTarget, E_POINTER);

	if(pTargetFormat == 0 || *pTargetFormat == TIME_FORMAT_MEDIA_TIME)
    {
        if(pSourceFormat == 0 || *pSourceFormat == TIME_FORMAT_MEDIA_TIME)
        {
            *pTarget = Source;
            return S_OK;
        }
    }
    return E_INVALIDARG;
}

STDMETHODIMP COggMux::GetPreroll(LONGLONG *pPreroll)
{
    CheckPointer(pPreroll, E_POINTER);
    *pPreroll = 0;
    return S_OK;
}
