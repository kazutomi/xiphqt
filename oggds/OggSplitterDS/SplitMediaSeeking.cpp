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
#include <limits.h>

const DWORD OGG_SPLITTER_SEEKING_CAPS =	AM_SEEKING_CanSeekForwards |
										AM_SEEKING_CanSeekBackwards |
										AM_SEEKING_CanSeekAbsolute |
										AM_SEEKING_CanGetStopPos |
										AM_SEEKING_CanGetDuration;

HRESULT COggSplitOutputPin::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    // only seeking in time (REFERENCE_TIME units) is supported
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT COggSplitOutputPin::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT COggSplitOutputPin::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : E_INVALIDARG;
}

HRESULT COggSplitOutputPin::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT COggSplitOutputPin::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT COggSplitOutputPin::GetDuration(LONGLONG *pDuration)
{
	return m_pOggSplitter->GetDuration(pDuration);
}

HRESULT COggSplitter::GetDuration(LONGLONG *pDuration)
{
    CheckPointer(pDuration, E_POINTER);
    CAutoLock lock(m_pLock);
    *pDuration = m_rtDuration;
    return S_OK;
}

HRESULT COggSplitOutputPin::GetStopPosition(LONGLONG *pStop)
{
	return m_pOggSplitter->GetStopPosition(pStop);
}

HRESULT COggSplitter::GetStopPosition(LONGLONG *pStop)
{
    CheckPointer(pStop, E_POINTER);
    CAutoLock lock(m_pLock);
    *pStop = m_rtStop;
    return S_OK;
}

HRESULT COggSplitOutputPin::GetCapabilities( DWORD * pCapabilities )
{
    CheckPointer(pCapabilities, E_POINTER);
    *pCapabilities = OGG_SPLITTER_SEEKING_CAPS;
    return S_OK;
}

HRESULT COggSplitOutputPin::CheckCapabilities( DWORD * pCapabilities )
{
    CheckPointer(pCapabilities, E_POINTER);
    return (~OGG_SPLITTER_SEEKING_CAPS & *pCapabilities) ? S_FALSE : S_OK;
}

HRESULT COggSplitOutputPin::ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
                           LONGLONG    Source, const GUID * pSourceFormat )
{
    CheckPointer(pTarget, E_POINTER);
    // since we only support TIME_FORMAT_MEDIA_TIME, we don't really
    // offer any conversions.
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

HRESULT COggSplitOutputPin::SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
                      , LONGLONG * pStop,  DWORD StopFlags )
{
	return m_pOggSplitter->SetPositions(pCurrent,CurrentFlags, pStop, StopFlags, this);
}

HRESULT COggSplitter::SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
                      , LONGLONG * pStop,  DWORD StopFlags, COggSplitOutputPin *pCaller )
{
	if (m_pLastSeek)
		if (m_pLastSeek != pCaller)
			if (*pCurrent == m_rtStart ||
				*pCurrent == m_rtLastSeek)
			{
				*pCurrent = m_rtStart;
				return NOERROR;
			}

	m_pLastSeek = pCaller;
	m_rtLastSeek = *pCurrent;

    DWORD StopPosBits = StopFlags & AM_SEEKING_PositioningBitsMask;
    DWORD StartPosBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;

    if(StopFlags)
	{
        CheckPointer(pStop, E_POINTER);
        // accept only relative, incremental, or absolute positioning
        if(StopPosBits != StopFlags) return E_INVALIDARG;
    }

    if(CurrentFlags) {
        CheckPointer(pCurrent, E_POINTER);
        if(StartPosBits != AM_SEEKING_AbsolutePositioning &&
           StartPosBits != AM_SEEKING_RelativePositioning) return E_INVALIDARG;
    }

    // scope for autolock

    {
	    CAutoLock lock(&m_csFilter);
		
		BeginFlush();

        // set start position
        if(StartPosBits == AM_SEEKING_AbsolutePositioning)
        {
            m_rtStart = *pCurrent;
        }
        else if(StartPosBits == AM_SEEKING_RelativePositioning)
        {
            m_rtStart += *pCurrent;
        }

        // set stop position
        if(StopPosBits == AM_SEEKING_AbsolutePositioning)
        {
            m_rtStop = *pStop;
        }
        else if(StopPosBits == AM_SEEKING_IncrementalPositioning)
        {
            m_rtStop = m_rtStart + *pStop;
        }
        else if(StopPosBits == AM_SEEKING_RelativePositioning)
        {
            m_rtStop += *pStop;
        }

		if (m_rtStart > m_rtDuration)
			m_rtStart = m_rtDuration;
		else
		{
			m_pInput->PerformSeek(m_rtStart, m_rtDuration);

			// Goto next keyframe if required
			if (m_bAlwaysSearchToKeyFrame || (CurrentFlags & AM_SEEKING_SeekToKeyFrame))
				m_pInput->FindNextKeyFrame(&m_rtStart);
		}

		*pCurrent = m_rtStart;

		EndFlush();
    }

    return NOERROR;
}


HRESULT COggSplitOutputPin::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
	return m_pOggSplitter->GetPositions(pCurrent, pStop);
}

HRESULT COggSplitter::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
    if(pCurrent)	*pCurrent = m_rtStart;
    if(pStop)		*pStop = m_rtStop;
    return S_OK;
}

STDMETHODIMP COggSplitOutputPin::SetRate( double dRate)
{
	return m_pOggSplitter->SetRate(dRate);
}

STDMETHODIMP COggSplitter::SetRate( double dRate)
{
	m_dRate = dRate;
	return S_OK;
}

STDMETHODIMP COggSplitOutputPin::GetRate( double * pdRate)
{
	return m_pOggSplitter->GetRate(pdRate);
}


STDMETHODIMP COggSplitter::GetRate( double * pdRate)
{
	if (pdRate) *pdRate = m_dRate;
	return S_OK;
}

HRESULT COggSplitOutputPin::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
	return m_pOggSplitter->GetAvailable(pEarliest, pLatest);
}

HRESULT COggSplitter::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    if(pEarliest) *pEarliest = 0;
    if(pLatest) 
	{
        CAutoLock lock(m_pLock);
        *pLatest = m_rtDuration;
    }
    return S_OK;
}

HRESULT COggSplitOutputPin::GetPreroll(LONGLONG *pPreroll)
{
    CheckPointer(pPreroll, E_POINTER);
    *pPreroll = 0;
    return S_OK;
}

//
// Seeks the last page 
//
// I´ve decided not to use the standard bisect algorithm because
// this might be quite slow on large files on CDs
// (CD-ROM are working with different RPM according the the
// position. Therefore accesses which are close together are much
// faster)
//
// Strategy:
// I'm constructing a line between two known points (granulepos, filepos)
// Now I´m using this line to estimate the position. Afterwards I adjust
// the low or high point accordingly.
//
// Return : New position in Reference Time
void COggSplitInputPin::PerformSeek(REFERENCE_TIME rtSeekPos, REFERENCE_TIME rtDuration)
{
	HRESULT	hr;

	__int64			iLow		= 0;
	__int64			iCurrent;
	__int64			iHigh		= m_iStreamLen;
	__int64			iFilePos;
	REFERENCE_TIME	rtLowPos	=  0;
	REFERENCE_TIME	rtHighPos	= rtDuration;
	REFERENCE_TIME	rtCurrLow;

	bool			bFirstPageOut;
	bool			bEOF;

	ogg_page		og;

	if (rtSeekPos == 0)
	{
		m_iStreamPos = 0;
		return;
	}
	else if (rtSeekPos >= rtDuration)
	{
		m_iStreamPos = m_iStreamLen;
		return;
	}
	
	do
	{
		ogg_sync_reset(&m_oy);
		bFirstPageOut = true;
		iCurrent = LONGLONG((float)iLow + (((float)rtSeekPos  - (float)rtLowPos) *
										 ((float)iHigh       - (float)iLow)    /
										 ((float)rtHighPos  - (float)rtLowPos)));
		if ((iHigh - iCurrent) < 4096)
		{
			iCurrent = iHigh - 4096;
			if (iCurrent < 0) iCurrent = 0;
		}
		else if ((iCurrent - iLow)  < 4096) iCurrent = iLow + 4096;

		iFilePos = iCurrent;

		rtCurrLow = _I64_MAX;
		bEOF = false;

		while (ogg_sync_pageout(&m_oy, &og) != 1 && !bEOF)
		{
			if (iFilePos >= m_iStreamLen)
			{
				if (bFirstPageOut) bFirstPageOut = false;
				else
				{
					bEOF = true;
					iHigh	  = iCurrent;
					rtHighPos = rtDuration;
				}
			}
			else
			{
				int len = min(4096, (int)(m_iStreamLen - iFilePos));
				char* buffer = ogg_sync_buffer(&m_oy, len);
				hr = m_pReader->SyncRead(iFilePos, len, (BYTE*)buffer);
				ogg_sync_wrote(&m_oy, len);
				iFilePos += len;
			}
		}

		if (!bEOF)
		{
			int iStreamID = ogg_page_serialno(&og);
			COggStream *pStream = m_pOggSplitter->FindStreamByID(iStreamID);
			if (pStream)
			{
				rtCurrLow = pStream->MediaTimeToRefTime(ogg_page_granulepos(&og));

				if (rtCurrLow >= rtSeekPos) 
				{
					iHigh	  = iCurrent;
					rtHighPos = rtCurrLow;
				}
				else
				{
					iLow	   = iCurrent;
					rtLowPos   = rtCurrLow;
				}
			}
		}
	}  while ((iHigh - iLow) > 8192);

	m_iStreamPos = iLow;
}

void COggSplitInputPin::FindNextKeyFrame(CRefTime *pSeekPos)
{
	ogg_page			og;
	int					or;

	COggStream			*pStream;
	bool				bFirstPageOut;
	
	pStream = m_pOggSplitter->FindStreamByType(&MEDIATYPE_Video);
	if (!pStream) return; // No video => nothing to do
	
	int					iStreamID		  = pStream->m_iStreamID;
	__int64				iCurrent		  = m_iStreamPos;
	
	ogg_sync_reset(&m_oy);
	bFirstPageOut = true;
	stream_state_reset(&pStream->m_ss);
	pStream->m_bAfterReset = true;

	do
	{
		or = ogg_sync_pageout(&m_oy, &og);
		if ((or <= 0) && bFirstPageOut)
		{
			or = ogg_sync_pageout(&m_oy, &og);
		}

		if (or == 1) // Got page
		{
			if (ogg_page_serialno(&og) == iStreamID)
			{
				if (pStream->m_bAfterReset)
					if (ogg_page_packets(&og) != 0)
						pStream->m_bAfterReset = false;

				if (!pStream->m_bAfterReset)
					if (pStream->FindKeyFrame(&og, pSeekPos))	return;
			}
		}
		
		else if (or == 0) // More data needed
		{
			if (iCurrent >= m_iStreamLen) return; // EOF reached
			else
			{
				int len = min(4096, (int)(m_iStreamLen - iCurrent));
				char* buffer = ogg_sync_buffer(&m_oy, len);
				m_pReader->SyncRead(iCurrent, len, (BYTE*)buffer);
				ogg_sync_wrote(&m_oy, len);
				iCurrent += len;
			}
		}

	} while (true);
}
