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
#include <qnetwork.h>

COggSplitter::COggSplitter(LPUNKNOWN pUnk, HRESULT *phr) :
    CBaseFilter(NAME("Ogg Splitter"), pUnk, &m_csFilter, CLSID_OggSplitter),
	CPersistPropertyBag(arOggSplitterProps, SIZEOF_ARRAY(arOggSplitterProps), &CLSID_OggSplitter),
	CRegistryStuff(&CLSID_OggSplitter),

	m_rtStart((REFERENCE_TIME)0),
	m_rtStop(_I64_MAX/2),
	m_rtDuration(_I64_MAX/2),
	m_dRate(1),
	m_pLastSeek(NULL)
{
	if (SUCCEEDED(*phr))
	{
		COggSplitInputPin *pIn = new COggSplitInputPin(NAME("In"), this,
									&m_csFilter, phr, L"In");
        if (!pIn) *phr = E_OUTOFMEMORY;
		else
		{
			if (SUCCEEDED(*phr)) m_pInput = pIn; else delete pIn;
		}
	}

	m_paOutput = NULL;
	m_iOutputs = 0;
	m_paStream = NULL;
	m_iStreams = 0;
	m_pChapterInfo = NULL;
	m_iChapters = 0;
	m_bStopPauseRunCalled = false;

	m_bChaptersAsStreams = true;
	
	LoadFromRegistry(idSeekToKeyFrame, &m_bAlwaysSearchToKeyFrame, true);
	LoadFromRegistry(idAlwaysEnableAllStreams, &m_bAlwaysEnableAllStreams, false);
	LoadFromRegistry(idShowTrayIcon, &m_bShowTrayIcon, true);

	m_bEnableAll = m_bAlwaysEnableAllStreams;

	if (m_bShowTrayIcon)
		InitTrayIcon();
}

COggSplitter::~COggSplitter()
{
	if (m_bShowTrayIcon)
		DestroyTrayIcon();
	delete m_pInput;
	if (m_pChapterInfo)
		delete m_pChapterInfo;
}

CUnknown *COggSplitter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new COggSplitter(pUnk, phr);
}

// Add interface for property pages
STDMETHODIMP COggSplitter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if(riid == IID_ISpecifyPropertyPages)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((ISpecifyPropertyPages*)(this), ppv);
    }
    if(riid == IID_IAMStreamSelect)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IAMStreamSelect*)(this), ppv);
    }
    if(riid == IID_IPersistPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPersistPropertyBag*)(this), ppv);
    }
    if(riid == IID_IPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPropertyBag*)(this), ppv);
    }
	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

// Return about property page
STDMETHODIMP COggSplitter::GetPages(CAUUID *pPages)
{
	CheckPointer(pPages, E_POINTER);
	pPages->cElems = 2;
	pPages->pElems = (GUID*)CoTaskMemAlloc(2 * sizeof(GUID));
	pPages->pElems[0] = CLSID_OggSplitPropPage;
	pPages->pElems[1] = CLSID_OggDSAboutPage;
	return NOERROR;
}

// Returns the number of pins this filter has
int COggSplitter::GetPinCount(void)
{
    CAutoLock lock(&m_csFilter);
    return m_iOutputs + 1;
}

// Return a non-addref'd pointer to pin n
// needed by CBaseFilter
CBasePin *COggSplitter::GetPin(int n)
{
    CAutoLock lock(&m_csFilter);

	if (n == 0) return m_pInput;
	else if ((n > 0) && (n <= m_iOutputs)) return m_paOutput[n-1];
    return NULL;
}

COggSplitOutputPin*	COggSplitter::FindPinByGroupID(int iGroupID)
{
	for (int i=0; i<m_iStreams; i++)
		if (m_paStream[i]->m_bEnabled)
			if (m_paStream[i]->m_iGroupID == iGroupID)
				return m_paStream[i]->m_pPin;
	return	NULL;
}

COggStream*	COggSplitter::FindStreamByID(int iStreamID)
{
	for (int i=0; i<m_iStreams; i++)
		if (m_paStream[i]->m_iStreamID == iStreamID)
			return m_paStream[i];
	return	NULL;
}

COggStream*	COggSplitter::FindStreamByType(const GUID *pGUID)
{
	for (int i=0; i<m_iStreams; i++)
		if (*(m_paStream[i]->m_mt.Type()) == *pGUID)
			return m_paStream[i];
	return	NULL;
}

void COggSplitter::DeleteOutputPins()
{
    CAutoLock lock(&m_csFilter);

    for (int i=0; i<m_iOutputs; i++)
	{
		if (m_paOutput[i]->IsConnected())
		{
			IPin	*pcPin;

			m_paOutput[i]->ConnectedTo(&pcPin);
			pcPin->Disconnect();
			m_paOutput[i]->Disconnect();
		} 
	}
	delete m_paOutput;
	m_paOutput = NULL;
	m_iOutputs = 0;
}

// Create a new stream
HRESULT COggSplitter::AddStream(COggStream **ppStream, int iStreamID, bool bDummyStream)
{
    CAutoLock	lock(&m_csFilter);

	// Create the stream and add it to the array ...
	*ppStream   = new COggStream(iStreamID, this, bDummyStream);
    COggStream **paStream = new COggStream *[m_iStreams+1];
    if (!paStream) return E_OUTOFMEMORY;
    // If there was already an array we must copy it ...
	if (m_paStream)
	{
        CopyMemory((void*)paStream, (void*)m_paStream, m_iStreams * sizeof(m_paStream[0]));
        delete [] m_paStream;
    }
    m_paStream = paStream;
    m_paStream[m_iStreams] = *ppStream;
    m_iStreams++;
    return S_OK;
}

// Create a new pin
HRESULT COggSplitter::InsertStream(int iPos, COggStream **ppStream, int iStreamID, bool bDummyStream)
{
    CAutoLock	lock(&m_csFilter);

	if (iPos > m_iStreams) return E_FAIL;

	// Create the stream and add it to the array ...
	*ppStream   = new COggStream(iStreamID, this, bDummyStream);
    COggStream **paStream = new COggStream *[m_iStreams+1];
    if (!paStream) return E_OUTOFMEMORY;

	if (m_paStream)
	{
		CopyMemory((void*)paStream, (void*)m_paStream, iPos * sizeof(m_paStream[0]));
		CopyMemory((void*)&paStream[iPos+1], (void*)&m_paStream[iPos], (m_iStreams - iPos) * sizeof(m_paStream[0]));
	    delete [] m_paStream;
	}

    m_paStream = paStream;
	m_paStream[iPos] = *ppStream;
    m_iStreams++;
    return S_OK;
}

void COggSplitter::DeleteStreams()
{
    CAutoLock lock(&m_csFilter);

	for (int i=0; i<m_iStreams; i++) delete m_paStream[i];
	delete m_paStream;
	m_paStream = NULL;
	m_iStreams = 0;
}

//
// Classify the streams and set the groupID
//
void COggSplitter::SetGroupID()
{
	int i = 0;

	while (i<m_iStreams)
	{
		CMediaType	*pmt = &(m_paStream[i]->m_mt);
		int			iMaxGroupID = -1;
		m_paStream[i]->m_iGroupID = -1;

		for (int j=0; j<i; j++)
		{
			CMediaType	*pmt_cmp = &(m_paStream[j]->m_mt);

			if (m_paStream[j]->m_iGroupID > iMaxGroupID)
				iMaxGroupID = m_paStream[j]->m_iGroupID;

			if (*(pmt->Type())    == *(pmt_cmp->Type()) &&
				*(pmt->Subtype()) == *(pmt_cmp->Subtype()))
			{
				m_paStream[i]->m_iGroupID = m_paStream[j]->m_iGroupID;
				m_paStream[i]->Enable(m_bEnableAll);
			}
		}
		
		if (m_paStream[i]->m_iGroupID == -1)
		{ // This is the first stream of this type
			if (*(m_paStream[i]->m_mt.Type()) == MEDIATYPE_Text && !m_bEnableAll)
			{
				// We must create dummy stream ...
				COggStream* pStream;
				InsertStream(i,&pStream, 65535, true);
				m_paStream[i]->m_iGroupID = iMaxGroupID + 1;
				m_paStream[i]->Enable(true);
				i++;
				m_paStream[i]->m_iGroupID = iMaxGroupID + 1;
				m_paStream[i]->Enable(false);
			}
			else
			{
				m_paStream[i]->m_iGroupID = iMaxGroupID + 1;
				m_paStream[i]->Enable(TRUE);
			}
		}

		i++;
	}
}

void COggSplitter::CreateChapterList()
{
	int		iStream;
	char	cProperty[32];
	int		iChapter;
	char*	pValue = NULL;

	m_iChapters = 0;
	if (m_pChapterInfo)
	{
		delete m_pChapterInfo;
		m_pChapterInfo = NULL;
	}

	// Count chapter info in all streams ...
	for (iStream=0; iStream<m_iStreams; iStream++)
	{
		iChapter = 1;
		do
		{
			wsprintf(cProperty, "CHAPTER%02d", iChapter);
			pValue = vorbis_comment_query(&m_paStream[iStream]->m_vc, cProperty, 0);
			if (pValue)
				m_iChapters++;
			iChapter++;
		} while (pValue);
	}

	if (!m_iChapters) return; // No chapter => No chapter list

	m_pChapterInfo = new ChapterInfo[m_iChapters];
	m_iChapters = 0;

	for (iStream=0; iStream<m_iStreams; iStream++)
	{
		iChapter = 1;
		do
		{
			wsprintf(cProperty, "CHAPTER%02d", iChapter);
			pValue = vorbis_comment_query(&m_paStream[iStream]->m_vc, cProperty, 0);
			if (pValue)
			{
				char*	pName;

				wsprintf(cProperty, "CHAPTER%02dNAME", iChapter);
				pName = vorbis_comment_query(&m_paStream[iStream]->m_vc, cProperty, 0);
				if (pName) strcpy(m_pChapterInfo[m_iChapters].cName, pName);
				else       wsprintf(m_pChapterInfo[m_iChapters].cName, "Chapter %d", iChapter);
				if (!StringToReferenceTime(pValue, &m_pChapterInfo[m_iChapters].rtStart))
					m_pChapterInfo[m_iChapters].rtStart = 0;
				m_iChapters++;
			}
			iChapter++;
		} while (pValue);
	}
}

HRESULT COggSplitter::CreateOutputPins()
{
    CAutoLock	lock(&m_csFilter);
	int			i;
	COggSplitOutputPin	*pPin;

	// Create a pin for the active streams ...
	for (i=0; i<m_iStreams; i++)
		if (m_paStream[i]->m_bEnabled)
		{
			CMediaType	*pmt = &(m_paStream[i]->m_mt);

			if (*(pmt->Type()) != MEDIATYPE_NULL)
			{
				
				HRESULT				hr = NOERROR;
				wchar_t				PinNameL[64];
				char				PinName[64];
				
				if (*(pmt->Type()) == MEDIATYPE_Video)
					wsprintf(PinName, "Video %d", m_paStream[i]->m_iStreamID);
				else if (*(pmt->Type()) == MEDIATYPE_Audio)
					wsprintf(PinName, "Audio %d", m_paStream[i]->m_iStreamID);
				else if (*(pmt->Type()) == MEDIATYPE_Text)
					wsprintf(PinName, "Subtitle %d", m_paStream[i]->m_iStreamID);
				else
					wsprintf(PinName, "Stream %d", m_paStream[i]->m_iStreamID);

				wsprintfW(PinNameL, L"%s", PinName);
				pPin = new COggSplitOutputPin(PinName, this, &m_csFilter, &hr, PinNameL);
				if FAILED(hr) return E_OUTOFMEMORY;
				{
					// Add the pin the array ...
					COggSplitOutputPin **paOutput = new COggSplitOutputPin *[m_iOutputs+1];
					if (!paOutput) return E_OUTOFMEMORY;
					// Copy the array if there was one before
					if (m_paOutput)
					{
						CopyMemory((void*)paOutput, (void*)m_paOutput, m_iOutputs * sizeof(m_paOutput[0]));
						delete [] m_paOutput;
					}
					m_paOutput = paOutput;
					m_paOutput[m_iOutputs] = pPin;
					m_iOutputs++;

					m_paStream[i]->m_pPin = pPin;
					pPin->m_pStream = m_paStream[i];
				}
			}
		}
	// Set the corresponding pin for inactive pins ...
	for (i=0; i<m_iStreams; i++)
		if (!(m_paStream[i]->m_bEnabled))
		{
			pPin = FindPinByGroupID(m_paStream[i]->m_iGroupID);
			m_paStream[i]->m_pPin = pPin;
		}

	return S_OK;
}

// inform all other pins of this group that the position has changed ...
void COggSplitter::NotifyGroup(BYTE iGroupID, COggSplitOutputPin* pPin, REFERENCE_TIME rtTime)
{
	char	iSubtitleGroup = -1;
	
	pPin->SetLastPos(&rtTime);

	for (int i=0; i<m_iStreams; i++)
	{
		int iStreamGroupID = m_paStream[i]->m_iGroupID;

		if (iStreamGroupID == iGroupID ||
			iStreamGroupID == iSubtitleGroup)
		{
			if (!(m_paStream[i]->m_bEnabled))
				m_paStream[i]->NotifyNewPosition();
		}
	}
}

HRESULT	COggSplitter::BeginFlush()
{
	CAutoLock	lock(m_pLock);
	int			i;

	// stop input pin
	m_pInput->BeginFlush();
	// Deliver Begin Flush
	for (i=0; i<m_iOutputs; i++) m_paOutput[i]->BeginFlush();
	// Stop the streams ...
	for (i=0; i<m_iStreams; i++) m_paStream[i]->BeginFlush();

	return NOERROR;
}

HRESULT	COggSplitter::EndFlush()
{
    CAutoLock		lock(m_pLock);
	int				i;

	m_bEOF = false;

	// Deliver end flush
	for (i=0; i<m_iOutputs; i++) m_paOutput[i]->EndFlush();
	// start the streams
	for (i=0; i<m_iStreams; i++) m_paStream[i]->EndFlush();
	// start input pin
	m_pInput->EndFlush();

	return NOERROR;
}

STDMETHODIMP COggSplitter::Stop()
{
	m_bStopPauseRunCalled = true;

	CAutoLock cObjectLock(m_pLock);
    HRESULT hr = NOERROR;
	int		i;

    if (m_State != State_Stopped)
	{
		// Stop the input pin ...
		m_pInput->Inactive();
		// the streams ...
		for (i=0; i<m_iStreams; i++)
			m_paStream[i]->Inactive();
		// and finally the pins
		for (i=0; i<m_iOutputs; i++)
			m_paOutput[i]->Inactive();
   }

    m_State = State_Stopped;
	m_pLastSeek = NULL;

    return hr;
}


STDMETHODIMP COggSplitter::Pause()
{
	m_bStopPauseRunCalled = true;

    CAutoLock cObjectLock(m_pLock);
	int		i;

	// notify all pins of the change to active state
    if (m_State == State_Stopped)
	{
		m_bEOF = false;
		// Reset the output pins ...
		for (i=0; i<m_iOutputs; i++)
			m_paOutput[i]->Active();
		// Start the streams
		for (i=0; i<m_iStreams; i++)
			m_paStream[i]->Active();
		// and finally the input pin
		m_pInput->Active();
    }
    m_State = State_Paused;
	m_pLastSeek = NULL;
    return S_OK;
}

STDMETHODIMP COggSplitter::Run(REFERENCE_TIME tStart)
{
	m_bStopPauseRunCalled = true;

	CAutoLock cObjectLock(m_pLock);
    // remember the stream time offset
    m_tStart = tStart;

	if (m_State == State_Stopped)
	{
		HRESULT hr = Pause();
	    if (FAILED(hr)) return hr;
    }

    // notify all pins of the change to active state
    if (m_State != State_Running)
	{
		m_pInput->Run(tStart);

		for (int i=0; i<m_iOutputs; i++)
			if (m_paOutput[i]->IsConnected())
				m_paOutput[i]->Run(tStart);
    }
    m_State = State_Running;
	m_pLastSeek = NULL;
    return S_OK;
}

void COggSplitter::SetDuration(REFERENCE_TIME rtDuration)
{
	m_rtStart = 0;
	m_rtStop = rtDuration;
	m_rtDuration = rtDuration;
}

void COggSplitter::NotifyEOF()
{
	m_bEOF = true;
	for (int i=0; i<m_iStreams; i++)
		m_paStream[i]->NotifyEOF();
}

HRESULT STDMETHODCALLTYPE COggSplitter::Save(IPropertyBag *pPropBag, BOOL fClearDirty,
								   BOOL fSaveAllProperties)
{
	CPersistPropertyBag::Save(pPropBag, fClearDirty, fSaveAllProperties);

	if (HKEY hReg = OpenRegistry())
	{
		HKEY	hFourCCReg;

		if (RegOpenKeyEx(hReg, idFourCCMapping, 0, KEY_READ, &hFourCCReg) == ERROR_SUCCESS)
		{
			VARIANT	V;

			char		szSrc[20];			
			DWORD		dwSrcSize = sizeof(szSrc);
			BYTE		cDst[4];
			DWORD		dwDstSize  = sizeof(cDst); 

			int iIndex = 0;
			
			while (RegEnumValue(hFourCCReg, iIndex, szSrc, &dwSrcSize, NULL, NULL,
								(BYTE*)&cDst, &dwDstSize) == ERROR_SUCCESS)
			{
				wchar_t	wcProp[5];

				wcProp[4] = L'\0';
				szSrc[10] = '\0';
				wcProp[0] = (wchar_t)strtol(szSrc+8, NULL, 16);
				szSrc[8] = '\0';
				wcProp[1] = (wchar_t)strtol(szSrc+6, NULL, 16);
				szSrc[6] = '\0';
				wcProp[2] = (wchar_t)strtol(szSrc+4, NULL, 16);
				szSrc[4] = '\0';
				wcProp[3] = (wchar_t)strtol(szSrc+2, NULL, 16);

				V.vt = VT_BSTR;
				V.bstrVal = SysAllocStringLen(NULL, 5);

				V.bstrVal[0] = cDst[0];
				V.bstrVal[1] = cDst[1];
				V.bstrVal[2] = cDst[2];
				V.bstrVal[3] = cDst[3];
				V.bstrVal[4] = L'\0';

				pPropBag->Write(wcProp, &V);
				VariantClear(&V);

				iIndex++;
				dwSrcSize = sizeof(szSrc);
				dwDstSize = sizeof(cDst); 
			}
			RegCloseKey(hFourCCReg);
		}
		RegCloseKey(hReg);
	}

	return NOERROR;
}

// IPropertyBag methods
HRESULT STDMETHODCALLTYPE COggSplitter::Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog)
{
	char	szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));
	VariantClear(pVar);

	if (strcmp(szName, idSeekToKeyFrame) == 0)
	{
		pVar->vt = VT_BOOL; pVar->boolVal = m_bAlwaysSearchToKeyFrame;
		return NOERROR;
	}

	if (strcmp(szName, idAlwaysEnableAllStreams) == 0)
	{
		pVar->vt = VT_BOOL; pVar->boolVal = m_bAlwaysEnableAllStreams;
		return NOERROR;
	}
	if (strcmp(szName, idShowTrayIcon) == 0)
	{
		pVar->vt = VT_BOOL; pVar->boolVal = m_bShowTrayIcon;
		return NOERROR;
	}

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE COggSplitter::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	char	szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));

	if (strcmp(szName, idShowTrayIcon) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);
		if (m_bShowTrayIcon)
			DestroyTrayIcon();
		m_bShowTrayIcon = (pVar->boolVal != 0);
		SaveToRegistry(szName, pVar->lVal != 0);
		if (m_bShowTrayIcon)
			InitTrayIcon();
		return NOERROR;
	}
	if (strcmp(szName, idSeekToKeyFrame) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);
		m_bAlwaysSearchToKeyFrame = (pVar->boolVal != 0);
		SaveToRegistry(szName, pVar->lVal != 0);
		return NOERROR;
	}
	if (strcmp(szName, idAlwaysEnableAllStreams) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);
		m_bAlwaysEnableAllStreams = (pVar->boolVal != 0);
		m_bEnableAll = m_bAlwaysEnableAllStreams;
		SaveToRegistry(szName, pVar->lVal != 0);
		return NOERROR;
	}
	if (strcmp(szName, idEnableAllStreams) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_BOOL);
		m_bEnableAll = (pVar->boolVal != 0);
		return NOERROR;
	}
	if (strlen(szName) == 4)  // FourCC mapping
	{
		HKEY	hReg;

		if (hReg = OpenRegistry())
		{
			HKEY	hFourCCReg;
			DWORD	dwDisp;

			if (RegCreateKeyEx(hReg, idFourCCMapping, NULL, NULL, 0, KEY_READ | KEY_WRITE,
							NULL, &hFourCCReg, &dwDisp) == ERROR_SUCCESS)
			{
				char	szValue[20];

				VariantChangeType(pVar, pVar, 0, VT_BSTR);
				wsprintf(szValue, "0x%02x%02x%02x%02x", pszPropName[3], pszPropName[2],
														pszPropName[1], pszPropName[0]);
				RegDeleteValue(hFourCCReg, szValue);

				if (wcslen(pVar->bstrVal) > 0)
				{
					BYTE	cDst[4];

					cDst[0] = (BYTE)pVar->bstrVal[0];
					cDst[1] = (BYTE)pVar->bstrVal[1];
					cDst[2] = (BYTE)pVar->bstrVal[2];
					cDst[3] = (BYTE)pVar->bstrVal[3];

					RegSetValueEx(hFourCCReg, szValue, 0, REG_DWORD, cDst, sizeof(cDst));
				}
				RegCloseKey(hFourCCReg);
			}
			RegCloseKey(hReg);
		}
		return NOERROR;
	}
	
	return E_FAIL;
}