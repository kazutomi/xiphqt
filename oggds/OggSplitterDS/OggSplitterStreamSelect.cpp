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
//
//  Return the number of streams ....
//
HRESULT STDMETHODCALLTYPE COggSplitter::Count(DWORD *pcStreams)
{
	CAutoLock lock(&m_csFilter);
	*pcStreams = m_iStreams;
	if (m_bChaptersAsStreams)
		*pcStreams += m_iChapters;
	return NOERROR;
}

//
// Return stream info ...
//
HRESULT STDMETHODCALLTYPE COggSplitter::Info(long lIndex, AM_MEDIA_TYPE **ppmt,
									  DWORD *pdwFlags, LCID *plcid,
									  DWORD *pdwGroup, WCHAR **ppszName,
									  IUnknown **ppObject, IUnknown **ppUnk)
{
	if (lIndex < 0) return E_INVALIDARG;

	if (lIndex < m_iStreams)
	{
		// Create info for stream
		if (ppmt)
		{
			AM_MEDIA_TYPE*	pmt = &m_paStream[lIndex]->m_mt;
			*ppmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(**ppmt));
			memcpy(*ppmt, pmt, sizeof(*pmt));
			if (pmt->cbFormat)
			{
				(*ppmt)->pbFormat = (BYTE*)CoTaskMemAlloc(pmt->cbFormat);
				memcpy((*ppmt)->pbFormat, pmt->pbFormat, pmt->cbFormat);
			}
		}

		if (pdwFlags) *pdwFlags = m_paStream[lIndex]->m_bEnabled ? AMSTREAMSELECTINFO_ENABLED : 0;

		if (plcid) *plcid = GetLCIDFromComment(&m_paStream[lIndex]->m_vc);

		if (pdwGroup) *pdwGroup = m_paStream[lIndex]->m_iGroupID;

		if (ppszName)
		{
			char	szName[256];

			if (*(m_paStream[lIndex]->m_mt.Type()) == MEDIATYPE_Video)
				strcpy(szName, "Video ");
			else if (*(m_paStream[lIndex]->m_mt.Type()) == MEDIATYPE_Audio)
				strcpy(szName, "Audio ");
			else if (*(m_paStream[lIndex]->m_mt.Type()) == MEDIATYPE_Text)
				strcpy(szName, "Subtitle ");
			else
				strcpy(szName, "Stream ");

			char*	pLang = vorbis_comment_query(&m_paStream[lIndex]->m_vc, "LANGUAGE", 0);

			if (pLang)
				strcat(szName, pLang);
			else
				ltoa(m_paStream[lIndex]->m_iStreamID, szName+strlen(szName), 10);

			*ppszName = (WCHAR*) CoTaskMemAlloc((strlen(szName)+2)*2);
			wsprintfW(*ppszName, L"%s", szName);
		}

		if (ppObject)
			*ppObject = NULL;

		if (ppUnk)
			*ppUnk = NULL;

		return NOERROR;
	}

	lIndex -= m_iStreams;
	if (!m_bChaptersAsStreams || (lIndex >= m_iChapters)) return E_INVALIDARG;

	// Create info for chapter
	if (ppmt)
	{
		*ppmt = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
		memset(*ppmt, 0, sizeof(AM_MEDIA_TYPE));
		(*ppmt)->majortype = MEDIATYPE_Video;
	}

	if (pdwFlags)
	{
		*pdwFlags = 0;
		
		IMediaSeeking*	pSeek;
		REFERENCE_TIME	rtCurrent;

		if SUCCEEDED(m_pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeek))
		{
			pSeek->GetCurrentPosition(&rtCurrent);
			pSeek->Release();

			if (m_pChapterInfo[lIndex].rtStart <= rtCurrent)
			{
				*pdwFlags = AMSTREAMSELECTINFO_ENABLED;
				for (int i=0; i<m_iChapters; i++)
					if (i != lIndex &&
						(m_pChapterInfo[i].rtStart <= rtCurrent) &&
						(m_pChapterInfo[i].rtStart > m_pChapterInfo[lIndex].rtStart))
						*pdwFlags = 0;
			}
		}
	}

	if (plcid) *plcid = 0;

	if (pdwGroup) *pdwGroup = CHAPTER_GROUP;

	if (ppszName)
	{
		*ppszName = (WCHAR*) CoTaskMemAlloc((strlen(m_pChapterInfo[lIndex].cName)+16) * 2);
		wsprintfW(*ppszName, L"%02d:%02d:%02d.%03d %s",
			(int)(m_pChapterInfo[lIndex].rtStart / (__int64)36000000000),
			(int)(m_pChapterInfo[lIndex].rtStart / (__int64)  600000000) % 60,
			(int)(m_pChapterInfo[lIndex].rtStart / (__int64)   10000000) % 60,
			(int)(m_pChapterInfo[lIndex].rtStart / (__int64)      10000) % 1000,
			m_pChapterInfo[lIndex].cName);
	}
	
	if (ppObject) *ppObject = NULL;
	if (ppUnk) *ppUnk = NULL;

	return NOERROR; 
}

HRESULT STDMETHODCALLTYPE COggSplitter::Enable(long lIndex, DWORD dwFlags)
{
	if (dwFlags == (DWORD)-1)
	{
		m_bEnableAll = TRUE;
		return NOERROR;
	}

	// Otherwise MediaPlayer 9 will crash
	if (dwFlags != AMSTREAMSELECTENABLE_ENABLE)
		return NOERROR;

	if (lIndex < 0)
		return E_INVALIDARG;

	if (lIndex < m_iStreams)
	{

		int	iGroupID = m_paStream[lIndex]->m_iGroupID;

		for (long i=0; i<m_iStreams; i++)
			if (m_paStream[i]->m_iGroupID == iGroupID)
			{
				if (dwFlags == 0)
					m_paStream[i]->Enable(false);
 				else if (dwFlags == AMSTREAMSELECTENABLE_ENABLEALL)
					m_paStream[i]->Enable(true);
				else if (dwFlags == AMSTREAMSELECTENABLE_ENABLE)
					m_paStream[i]->Enable(i == lIndex);

				m_paStream[i]->NotifyNewPosition();
			}

		// if the flag is not AMSTREAMSELECTENABLE_ENABLE we must recreate the pins
		if (dwFlags != AMSTREAMSELECTENABLE_ENABLE)
		{
			DeleteOutputPins();
			CreateOutputPins();
		}
		return NOERROR;
	}

	lIndex -= m_iStreams;
	if (!m_bChaptersAsStreams || (lIndex >= m_iChapters)) return E_INVALIDARG;

	if (dwFlags != AMSTREAMSELECTENABLE_ENABLE || !m_bStopPauseRunCalled)
		return NOERROR;

	IMediaSeeking*	pSeek;

	if SUCCEEDED(m_pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeek))
	{
		pSeek->SetPositions(&m_pChapterInfo[lIndex].rtStart, AM_SEEKING_AbsolutePositioning,
							NULL, AM_SEEKING_NoPositioning);
		pSeek->Release();
	}
	return NOERROR;
}