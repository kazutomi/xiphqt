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
#include "../PersistPropertyBag.h"
#include <qnetwork.h>

#define BUFFERSIZE 64*1024

#define cnts_VORBIS		0
#define cnts_OGGSTREAM	1
#define cnts_UNKNOWN	255

#define CHAPTER_GROUP 0xFFFFFFF0

#define	WM_CALLBACK WM_USER+1

enum Command {CMD_RESET, CMD_RUN, CMD_STOP, CMD_EXIT};

typedef struct
{
	char			cName[64];
	REFERENCE_TIME	rtStart;
} ChapterInfo, *pChapterInfo;

const char idSeekToKeyFrame[]			= "AlwaysSearchToKeyFrame";
const char idEnableAllStreams[]			= "EnableAllStreams";
const char idAlwaysEnableAllStreams[]	= "AlwaysEnableAllStreams";
const char idShowTrayIcon[]				= "ShowTrayIcon";

static const char* arOggSplitterProps[] =
{
	idSeekToKeyFrame,
	idEnableAllStreams,
	idAlwaysEnableAllStreams,
	idShowTrayIcon
};

const char idFourCCMapping[]			= "FOURCC Mapping";
const char szTrayWndClass[]				= "OggSplitterTrayNotifyWnd";


class COggSplitter;
class COggSplitOutputPin;
class COggSplitInputPin;
class COggSplitter;

class COggSplitPropPage :	public CBasePropertyPage,
							public IPropertyBag
{
public:
						COggSplitPropPage(LPUNKNOWN pUnk, HRESULT *phr);
	DECLARE_IUNKNOWN
	STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv);
    static CUnknown		*WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
private:
	IPersistPropertyBag	*m_pPersistPropertyBag;

	HRESULT				OnConnect(IUnknown *pUnknown);
	HRESULT				OnDisconnect();
	HRESULT				OnActivate(void);
	HRESULT				OnApplyChanges(void);
	BOOL				OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void				DeleteFourCC(char* cFOURCC);
	void				SetProperty(const char* pProp, char* pVal);

	// IPropertyBag methods for communication with the property page
	HRESULT STDMETHODCALLTYPE	Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
	HRESULT STDMETHODCALLTYPE	Write(LPCOLESTR pszPropName, VARIANT *pVar);
};

class COggStream :	public  CAMThread,
					private COggPageQueue
{
	friend class COggSplitInputPin;
	friend class COggSplitOutputPin;
	friend class COggSplitter;
private:
	void				SetMTFromSH(stream_header *sh, CMediaType *pmt, int* pHighWM, int* pLowWM);
	void				SetMTFromVI(vorbis_info *vi, CMediaType *pmt);
	void				TranslateFourCC(char* FOURCC);
	bool				PageInFromQueue();
	bool				GetSample(REFERENCE_TIME* prtStart, REFERENCE_TIME* prtStop,
							ogg_int64_t* pmtStart, ogg_int64_t* pmtStop,
							bool* pbSyncPoint, bool* pbEOS,
							unsigned char** ppbBuffer, int* pcbBuffer);

	COggSplitter		*m_pOggSplitter;
	
	CAMEvent			m_evWaitForGroup;
	CAMEvent			m_evWaitForData;
	bool				m_bAbort;
    bool				m_bDiscontinuity;

	CCritSec			m_csStream;
	stream_state		m_ss;
	stream_header*		m_sh;
	vorbis_comment		m_vc;
	bool				m_bAfterReset;
	
	// Vorbis specific
	ogg_packet			m_VorbisHeader[3];
	vorbis_info			m_vi;

	int					m_iHighWatermark;
	int					m_iLowWatermark;

	long				m_iBufferSize;
	bool				m_bIsDummyStream;

	int					m_iStreamID;
	int					m_iGroupID;
	bool				m_bEnabled;
	CMediaType			m_mt;
	COggSplitOutputPin	*m_pPin;
	int					m_iStreamType;
	bool				m_bIdentified;

public:
						COggStream::COggStream(int iStreamID, COggSplitter *pOggSplitter, bool bDummyStream);
						~COggStream();
	HRESULT				Active();
	HRESULT				Inactive();
	HRESULT				BeginFlush();
	HRESULT				EndFlush();
	bool				DeliverPage(ogg_page *og);
	HRESULT				IdentifyType(ogg_page *og);	
	__int64				MediaTimeToRefTime(__int64 iMediaTime);
	void				Enable(bool bEnabled);
	void				NotifyNewPosition()							{ m_evWaitForGroup.Set(); }
	void				NotifyEOF()									{ m_evWaitForData.Set(); }
	void				StopThread(Command com);

	bool				FindKeyFrame(ogg_page *og, CRefTime *rtFrame);

	DWORD				ThreadProc();
	void				SendDummySample();
	HRESULT				SendSampleLoop();
	HRESULT				SendDummySampleLoop();
	HRESULT				GetLCIDFromComment(vorbis_comment *vc, DWORD *lcid, char **caption);
};

class COggSplitOutputPin :	public CBaseOutputPin,
							public IMediaSeeking,
							public IPropertyBag,
							public IPersistPropertyBag
{
	friend class COggStream;
	friend class COggSplitter;
private:
	COggSplitter		*m_pOggSplitter;

	COggStream*			m_pStream;
	CCritSec			m_csLastPos;
	REFERENCE_TIME		m_rtLastPos;

	HRESULT				GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT				CheckMediaType(const CMediaType* pmt);
	HRESULT				DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);

	HRESULT				Active();
	HRESULT				Inactive();
    STDMETHODIMP		BeginFlush(void);
    STDMETHODIMP		EndFlush(void);

protected:
	HRESULT				DoBufferProcessingLoop();
	DWORD				ThreadProc();

public:
						COggSplitOutputPin(TCHAR *pObjectName, COggSplitter *pFilter,
										   CCritSec *pLock, HRESULT *phr, LPCWSTR pName);
	DECLARE_IUNKNOWN
    STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv); // For IMediaSeeking

	// IPersistPropertyBag methods
	HRESULT	STDMETHODCALLTYPE	GetClassID(CLSID* pClassID);
    HRESULT	STDMETHODCALLTYPE	InitNew(void)										  { return NOERROR; }
    HRESULT STDMETHODCALLTYPE	Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog)	  { return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE	Save(IPropertyBag* pPropBag, BOOL fClearDirty,
									   BOOL fSaveAllProperties);
	
	// IPropertyBag methods
	HRESULT				STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog);
	HRESULT				STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT *pVar) {return E_NOTIMPL;}

	// IMediaSeeking methods ...
	STDMETHODIMP		IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP		QueryPreferredFormat(GUID *pFormat);
    STDMETHODIMP		SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP		IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP		GetTimeFormat(GUID *pFormat);

    STDMETHODIMP		GetDuration(LONGLONG *pDuration);
    STDMETHODIMP		GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP		GetCurrentPosition(LONGLONG *pCurrent)		{return E_NOTIMPL;}
    STDMETHODIMP		GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP		CheckCapabilities( DWORD * pCapabilities );
    STDMETHODIMP		ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
								            LONGLONG    Source, const GUID * pSourceFormat );

    STDMETHODIMP		SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
						, LONGLONG * pStop,  DWORD StopFlags );

    STDMETHODIMP		GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );

    STDMETHODIMP		GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP		SetRate( double dRate);
    STDMETHODIMP		GetRate( double * pdRate);
    STDMETHODIMP		GetPreroll(LONGLONG *pPreroll);

	
	HRESULT				SendVorbisHeaderPackets(ogg_packet *pVorbisHeader);
	void				SetLastPos(REFERENCE_TIME *prtCurPos);
	void				GetLastPos(REFERENCE_TIME *prtCurPos);
};

class COggSplitInputPin : public CAMThread,
						  public CBaseInputPin
{
friend class COggSplitter;
public:
	DECLARE_IUNKNOWN

	COggSplitInputPin(TCHAR *pObjectName, COggSplitter *pFilter, CCritSec *pLock, HRESULT *phr, LPCWSTR pName);
	~COggSplitInputPin();

	HRESULT				CheckConnect(IPin* pPin); // Check for IAsyncReader
	HRESULT				BreakConnect(void);
	HRESULT				CompleteConnect(IPin *pReceivePin); // Identify the streams
	HRESULT				CheckMediaType(const CMediaType* pmt);
	HRESULT				Active();
	HRESULT				Inactive();
	HRESULT				StopThread();

	void				PerformSeek(REFERENCE_TIME rtSearchPos, REFERENCE_TIME rtDuration);
	void				FindNextKeyFrame(CRefTime *pSeekPos);
	void				NotifyLowFilllevel() { m_evWaitHighWatermark.Set(); }

    STDMETHODIMP		BeginFlush();
    STDMETHODIMP		EndFlush();
protected:
	COggSplitter		*m_pOggSplitter;
	IAsyncReader		*m_pReader;
	ogg_sync_state		m_oy; // sync and verify incoming physical bitstream
	LONGLONG			m_iStreamLen;
	LONGLONG			m_iStreamPos;
	CAMEvent			m_evWaitHighWatermark;

	BOOL				m_Abort;
	DWORD				ThreadProc();
	HRESULT				DoBufferProcessingLoop(void);
};

class COggSplitter :	public CPersistPropertyBag,
						public IPropertyBag,
					    public ISpecifyPropertyPages,
						public IAMStreamSelect,
						public CBaseFilter,
					    private CRegistryStuff

//						public IDvdControl2
{
	friend class COggStream;
	friend class COggSplitOutputPin;
	friend class COggSplitInputPin;
public:
	COggSplitter(LPUNKNOWN punk, HRESULT *phr);
    ~COggSplitter();
    DECLARE_IUNKNOWN
    STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv); // For IMediaSeeking
    static CUnknown		*WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	STDMETHODIMP		Stop();
	STDMETHODIMP		Pause();
	STDMETHODIMP		Run(REFERENCE_TIME tStart);
	
	// ISpecifyPropertyPages Methods
	STDMETHODIMP		GetPages(CAUUID *pPages);

	// IPersistPropertyPage methods
    HRESULT STDMETHODCALLTYPE	Save(IPropertyBag *pPropBag, BOOL fClearDirty,
								   BOOL fSaveAllProperties);
	// IPropertyBag methods
	HRESULT STDMETHODCALLTYPE	Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog);
	HRESULT STDMETHODCALLTYPE	Write(LPCOLESTR pszPropName, VARIANT *pVar);

	// IAMStreamSelect methods
	HRESULT STDMETHODCALLTYPE	Count(DWORD *pcStreams);
	HRESULT STDMETHODCALLTYPE	Info(long lIndex, AM_MEDIA_TYPE **ppmt,
									  DWORD *pdwFlags, LCID *plcid,
									  DWORD *pdwGroup, WCHAR **ppszName,
									  IUnknown **ppObject, IUnknown **ppUnk);
	HRESULT STDMETHODCALLTYPE	Enable(long lIndex, DWORD dwFlags);

    // Media Seeking methos
	STDMETHODIMP		GetDuration(LONGLONG *pDuration);
    STDMETHODIMP		GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP		SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
						, LONGLONG * pStop,  DWORD StopFlags, COggSplitOutputPin *pCaller );
	STDMETHODIMP		GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );
    STDMETHODIMP		GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP		SetRate( double dRate);
    STDMETHODIMP		GetRate( double * pdRate);

	// Other methods
	void				NotifyGroup(BYTE iGroupID, COggSplitOutputPin* pPin, REFERENCE_TIME rtTime);
	COggSplitInputPin	*GetInputPin()		{ return m_pInput; }
	HRESULT				AddStream(COggStream **ppStream, int iStreamID, bool bDummyStream);
	HRESULT				InsertStream(int iPos, COggStream **ppStream, int iStreamID, bool bDummyStream);
	void				DeleteStreams();
	HRESULT				CreateOutputPins();
	void				DeleteOutputPins();
	COggStream*			FindStreamByID(int iStreamID);
	COggStream*			FindStreamByType(const GUID *pGUID);
	COggSplitOutputPin*	FindPinByGroupID(int iGroupID);
	void				CreateChapterList();
	void				SetGroupID();
	int					GetStreamCount()	{ return m_iStreams; }
	void				SetDuration(REFERENCE_TIME rtDuration);
	bool				IsEOF()				{ return m_bEOF; }
	void				NotifyEOF();

	bool				BufferUnderrun();

	void				ShowPopupMenu();	// Click on the tray icon
	void				ShowPropertyPages(HWND hSender);
private:
	COggSplitInputPin	*m_pInput;
    int					m_iOutputs;
    COggSplitOutputPin	**m_paOutput;  // the pins on this filter.
	int					m_iStreams;
	COggStream			**m_paStream;
	int					m_iChapters;
	ChapterInfo*		m_pChapterInfo;

	bool				m_bStopPauseRunCalled;
	bool				m_bChaptersAsStreams;
	bool				m_bEnableAll;
	bool				m_bAlwaysEnableAllStreams;
	bool				m_bAlwaysSearchToKeyFrame;
	bool				m_bShowTrayIcon;

	CCritSec			m_csFilter;

	COggSplitOutputPin*	m_pLastSeek;
	REFERENCE_TIME		m_rtLastSeek;
    
	CRefTime			m_rtDuration;      // length of stream
    CRefTime			m_rtStart;         // source will start here
	CRefTime			m_rtStop;          // source will stop here
	double				m_dRate;

	bool				m_bEOF;

	HWND				m_hTrayWnd;

	int					GetPinCount(void);
    CBasePin			*GetPin(int n);
	HRESULT				BeginFlush();
	HRESULT				EndFlush();

	void				InitTrayIcon();
	void				DestroyTrayIcon();
};
