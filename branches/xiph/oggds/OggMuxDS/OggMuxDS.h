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

#include "../common.h"
#include "../oggpagequeue.h"

class COggMuxPropPage :	public CBasePropertyPage,
						public IPropertyBag
{

public:
	DECLARE_IUNKNOWN
						COggMuxPropPage(LPUNKNOWN pUnk, HRESULT *phr);
    static CUnknown		*WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv); // For IMediaSeeking
	// IPropertyBag methods
	HRESULT				STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog);
	HRESULT				STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT *pVar);

private:
	IBaseFilter			*m_pFilter;
	HRESULT				OnConnect(IUnknown *pUnknown);
	HRESULT				OnDisconnect();
	HRESULT				OnActivate();
	HRESULT				OnDeactivate();
	HRESULT				OnApplyChanges(void);
	BOOL				OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void				SetProperty(char* pProp, char* pVal);
	void				LoadCurrentStreamComments();
	void				LoadLanguageSettings(IPropertyBag *pPropBag);
	void				UpdateLCID(IPropertyBag *pPropBag);
};

class COggMuxOutputPin : public CBaseOutputPin
{
	friend class COggMux;
	friend class COggMuxInputPin;
private:
	COggMux				*m_pOggMux;
	CCritSec			m_csFilePos;
	__int64				m_iFilePos;

    STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv); // For IMediaSeeking

	HRESULT				CheckMediaType(const CMediaType* pmt);
	HRESULT				DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
	HRESULT				GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT				GetSample(IMediaSample** ppSample, long len);
	HRESULT				Active();

	COggMuxOutputPin(TCHAR *pObjectName, COggMux *pFilter, CCritSec *pLock, HRESULT *phr, LPCWSTR pName);
};

class COggMuxInputPin : public CBaseInputPin,
						public IPropertyBag,
						public IPersistPropertyBag,
						private COggPageQueue
{
	friend class COggMux;
	friend class COggMuxOutputPin;
private:
	COggMux				*m_pOggMux;
	int					m_iStreamID;

	stream_state		m_ss;	

	REFERENCE_TIME		m_rtCurrent;

	stream_header*		m_sh;
	vorbis_info			m_vi;
	vorbis_comment		m_vc;

	CAMEvent			m_evWait;
	bool				m_bHeaderSent;
	bool				m_bEOSReceived;
	bool				m_bAbort;
	bool				m_bBlocked;
	bool				m_bCanBlock;

	CCritSec			m_csReceive;

	// Used to store a sample
	__int32				m_cbBufferSize;
	__int32				m_cbSampleSize;
	REFERENCE_TIME		m_rtStart;
	REFERENCE_TIME		m_rtStop;
	bool				m_bSyncPoint;
	bool				m_bFlush;
	bool				m_bSampleReady;


	HRESULT				Active();
	HRESULT				Inactive();
	HRESULT				CheckMediaType(const CMediaType* pmt);
	HRESULT				CompleteConnect(IPin *pReceivePin); // Overridden to create new pin if neccessary
	void				SetLangInVorbisComment(unsigned char* pbBuffer, __int32* cbBuffer);
	HRESULT				ProcessSample(bool bEOS);
	void				SendHeaderPackets();
	void				PagesOut(bool bFlush);


public:
    DECLARE_IUNKNOWN;
	COggMuxInputPin::COggMuxInputPin(TCHAR *pObjectName, COggMux *pFilter, CCritSec *pLock,
									 int StreamID, LPCWSTR pName, HRESULT *phr);
	~COggMuxInputPin(void);
    STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv); // For PropertyBag
    STDMETHODIMP		Receive(IMediaSample *pSample);
    STDMETHODIMP		EndOfStream(void);
	STDMETHODIMP		NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP		BeginFlush(void);
	STDMETHODIMP		EndFlush(void);
	STDMETHODIMP		NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);
	BOOL				EOSReceived()		{ return m_bEOSReceived; }

	tPageNode*			GetPage();
	ogg_int64_t			CurrentPos();
	void				ResetBlockedState()	{ m_bBlocked = false; }


	// IPropertyBag methods
	HRESULT				STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog);
	HRESULT				STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT *pVar);

	// IPersistPropertyPage methods
	HRESULT	STDMETHODCALLTYPE	GetClassID(CLSID* pClassID);
    HRESULT	STDMETHODCALLTYPE	InitNew(void)				{ return NOERROR; }
    HRESULT STDMETHODCALLTYPE	Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog);
    HRESULT STDMETHODCALLTYPE	Save(IPropertyBag* pPropBag, BOOL fClearDirty,
									   BOOL fSaveAllProperties);
};

class COggMux : public CBaseFilter,
				public ISpecifyPropertyPages,
				public IMediaSeeking
{
	friend class COggMuxInputPin;
	friend class COggMuxOutputPin;
private:
    CCritSec			m_csFilter;
	BOOL				m_bEOSDelivered;
	COggMuxOutputPin*	m_pOutput;
	int					m_iInputs;
	COggMuxInputPin**	m_paInput;

    CRefTime			m_rtPosition;
	CCritSec			m_csPosition;
	REFERENCE_TIME		m_rtSegStart;

	GUID				m_TimeCode;

	CCritSec			m_csWriteOutPages;

	int					GetPinCount();
	CBasePin*			GetPin(int n);
    HRESULT				GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT				CheckFreeInputPin();
	COggMuxInputPin*	GetEarliestPin(void);

public:
	COggMux(LPUNKNOWN punk, HRESULT *phr);
	~COggMux(void);
    DECLARE_IUNKNOWN;
    STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv); // For PropertyPages
    static CUnknown*	WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
	STDMETHODIMP		GetPages(CAUUID *pPages);
	HRESULT				NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP		Stop();


	// IMediaSeeking methods ...
	STDMETHODIMP		IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP		QueryPreferredFormat(GUID *pFormat);
    STDMETHODIMP		SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP		IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP		GetTimeFormat(GUID *pFormat);
    STDMETHODIMP		GetDuration(LONGLONG *pDuration);
    STDMETHODIMP		GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP		GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP		GetCapabilities( DWORD * pCapabilities);
    STDMETHODIMP		CheckCapabilities( DWORD * pCapabilities );
    STDMETHODIMP		ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
								            LONGLONG    Source, const GUID * pSourceFormat );
    STDMETHODIMP		SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
						, LONGLONG * pStop,  DWORD StopFlags );
    STDMETHODIMP		GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP		GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest);
    STDMETHODIMP		SetRate( double dRate) {return E_NOTIMPL;}
    STDMETHODIMP		GetRate( double * pdRate) {	return E_NOTIMPL;}
    STDMETHODIMP		GetPreroll(LONGLONG *pPreroll);

	// Others	
	HRESULT				EndOfStream(void);
	HRESULT				Interleave();
	void				ResetBlockedState();
};
