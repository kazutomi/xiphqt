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
#include "../PersistPropertyBag.h"

static const char szFormatPCM[]			= "PCM int 16";
static const char szFormatIEEEFloat[]	= "IEEE float 32";

static const char idFloatModeFirst[]	= "FloatModeFirst";
static const char idChannels[]			= "Channels";
static const char idSamplesPerSec[]		= "SamplesPerSec";
static const char idOutputMode[]		= "OutputMode";
static const char idPostGain[]			= "PostGain";

static const char* arVorbisDecProps[] =
{
	idFloatModeFirst,
	idChannels,
	idSamplesPerSec,
	idOutputMode,
	idPostGain
};

class CVorbisDecPropPage :	public CBasePropertyPage,
							public IPropertyBag
{
public:
	DECLARE_IUNKNOWN
    static CUnknown		*WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
	// IPropertyBag Methods
	HRESULT STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
	HRESULT STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT *pVar);
private:
	IPersistPropertyBag*	m_pPropBag;

	STDMETHODIMP			NonDelegatingQueryInterface(REFIID riid, void **ppv);
	HRESULT					OnConnect(IUnknown *pUnknown);
	HRESULT					OnDisconnect();
	HRESULT					OnActivate();
	HRESULT					OnApplyChanges();
	BOOL					OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	CVorbisDecPropPage(LPUNKNOWN pUnk, HRESULT *phr);
};

class CVorbisDecInputPin : public CTransformInputPin
{
public:
	CVorbisDecInputPin(TCHAR *pObjectName, CTransformFilter *pTransformFilter,
						HRESULT * phr, LPCWSTR pName ) :
	CTransformInputPin(pObjectName, pTransformFilter, phr, pName ) { }
	STDMETHODIMP			GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);
};

class CVorbisDec :	public CTransformFilter,
					public CPersistPropertyBag,
					public IPropertyBag,
					public ISpecifyPropertyPages,
				    private CRegistryStuff
{

public:
    DECLARE_IUNKNOWN;
    STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv); // For IMediaSeeking
    static CUnknown		*WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
	STDMETHODIMP		GetPages(CAUUID *pPages);

	// IPropertyBag methods
	HRESULT STDMETHODCALLTYPE	Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog);
	HRESULT STDMETHODCALLTYPE	Write(LPCOLESTR pszPropName, VARIANT *pVar);
private:
	CCritSec			m_csReceive;

	unsigned char		*m_pbVorbisInfo;
	int					m_cbVorbisInfo;
	unsigned char		*m_pbVorbisComment;
	int					m_cbVorbisComment;
	unsigned char		*m_pbVorbisCodebook;
	int					m_cbVorbisCodebook;
	
	vorbis_info			m_vi;
	vorbis_comment		m_vc;
	vorbis_dsp_state	m_vd;
	vorbis_block		m_vb;

	bool				m_bVorbisInitialized;
	__int64				m_packetno;
	
	IMediaSample*		m_QueuedSample;

	bool				m_bFloatMode;
	bool				m_bFloatModeFirst;
	int*				m_pMapping;	// This array contains the current mapping
	DWORD				m_dwChannelMask;

	bool				m_bPostGain;
	float				m_fPostGain;

    // Constructor - just calls the base class constructor
	CVorbisDec(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	CVorbisDec::~CVorbisDec();

    HRESULT				Receive(IMediaSample *pSample);
    HRESULT				CheckInputType(const CMediaType *mtIn);
    HRESULT				CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
    HRESULT				DecideBufferSize(IMemAllocator *pAlloc,
							ALLOCATOR_PROPERTIES *pProperties);
    HRESULT				GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT				WriteOutPackets(void);
    HRESULT				StartStreaming();
    HRESULT				StopStreaming();
	void				VorbisClear();
	HRESULT				VorbisInit();
	HRESULT				ProcessHeaderPacket(unsigned char *pbBuffer, int cbBuffer);
	HRESULT				ProcessSample(IMediaSample *pSample, BOOL bEOS, BOOL bPreroll);
	HRESULT				EndOfStream(void);
	HRESULT				BeginFlush(void);

	HRESULT				SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);
	HRESULT				InterleaveFloat(void* pbBuffer, int iMaxSamples, int* pMapping,
										int* iSamplesWritten, int* iBytesWritten);
	HRESULT				InterleaveInt16(void* pbBuffer, int iMaxSamples, int* pMapping,
										int* iSamplesWritten, int* iBytesWritten);

};
