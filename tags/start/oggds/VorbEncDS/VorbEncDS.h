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

static const char idMinBitrate[]	= "MinBitrate";
static const char idAvgBitrate[]	= "AvgBitrate";
static const char idMaxBitrate[]	= "MaxBitrate";
static const char idQuality[]		= "Quality";

static const char* arVorbisEncProps[] =
{
	idMinBitrate,
	idAvgBitrate,
	idMaxBitrate,
	idQuality
};

#define ENCODE_BUFFER_SIZE 16384

class CVorbisEncPropPage : public CBasePropertyPage,
						   public IPropertyBag
{

public:
	DECLARE_IUNKNOWN
    static CUnknown		*WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// IPropertyBag Methods
	HRESULT STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
	HRESULT STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT *pVar);
private:
	VORBISFORMAT		m_CodecSetup;
	int					m_iMessagesFromSet;
	IPersistPropertyBag	*m_pPersistPropertyBag;

	STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv);
	HRESULT				OnConnect(IUnknown *pUnknown);
	HRESULT				OnDisconnect();
	HRESULT				OnActivate();
	HRESULT				OnApplyChanges(void);
	BOOL				OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	CVorbisEncPropPage(LPUNKNOWN pUnk, HRESULT *phr);
};

class CVorbisEncOutputPin : public CTransformOutputPin,
							public IAMStreamConfig
{
	friend class CVorbisEnc;
private:
	CVorbisEnc*			m_pVorbisEnc;

						CVorbisEncOutputPin(TCHAR* pObjectName, CVorbisEnc* pFilter,
											HRESULT* phr, LPCWSTR pName );
public:
	DECLARE_IUNKNOWN
	STDMETHODIMP		NonDelegatingQueryInterface(REFIID riid, void **ppv);

	STDMETHODIMP		SetFormat(AM_MEDIA_TYPE *pmt);
	STDMETHODIMP		GetFormat(AM_MEDIA_TYPE **ppmt);
	STDMETHODIMP		GetNumberOfCapabilities(int *piCount, int *piSize);
	STDMETHODIMP		GetStreamCaps(int iIndex, AM_MEDIA_TYPE **ppmt, BYTE *pSCC);
};

class CVorbisEnc : public CTransformFilter,
				   public ISpecifyPropertyPages,
				   public CPersistPropertyBag,
				   public IPropertyBag,
				   private CRegistryStuff
{
	friend class CVorbisEncInputPin;
	friend class CVorbisEncOutputPin;
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
	CCritSec				m_csFilter;
	CCritSec				m_csEncode;

	vorbis_info				m_vi;
	vorbis_comment			m_vc;
	vorbis_dsp_state		m_vd;
	vorbis_block			m_vb;
	ogg_int64_t				m_tBuffer;

	BOOL					m_bDiscontinuity;
	bool					m_bHeaderSent;

	__int32					m_nMinBitsPerSec;
	__int32					m_nAvgBitsPerSec;
	__int32					m_nMaxBitsPerSec;
	float					m_fQuality;

	int*					m_pMapping;	// This array contains the current mapping
	DWORD					m_dwChannelMask;

							CVorbisEnc(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
							~CVorbisEnc();

	HRESULT					Receive(IMediaSample* pSample);
	HRESULT					EndOfStream(void);
	HRESULT					CheckInputType(const CMediaType* pmt);
	HRESULT					CheckTransform(const CMediaType* pmtIn, const CMediaType* pmtOut);
	HRESULT					DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* pProps);
	HRESULT					GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT					SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);

	HRESULT					StartStreaming();
	HRESULT					StopStreaming();
	HRESULT					SendHeader();
	HRESULT					Encode();
};
