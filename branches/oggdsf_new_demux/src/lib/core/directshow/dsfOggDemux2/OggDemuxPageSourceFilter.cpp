//===========================================================================
//Copyright (C) 2003, 2004, 2005 Zentaro Kavanagh
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//- Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//- Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//- Neither the name of Zentaro Kavanagh nor the names of contributors 
//  may be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//===========================================================================
#include "StdAfx.h"
#include "OggDemuxPageSourceFilter.h"


// This template lets the Object factory create us properly and work with COM infrastructure.
CFactoryTemplate g_Templates[] = 
{
    { 
		L"OggDemuxFilter",						// Name
	    &CLSID_OggDemuxPageSourceFilter,            // CLSID
	    OggDemuxPageSourceFilter::CreateInstance,	// Method to create an instance of MyComponent
        NULL,									// Initialization function
        NULL									// Set-up information (for filters)
    }
	
	//,

	//{ 
	//	L"illiminable About Page",				// Name
	//    &CLSID_PropsAbout,						// CLSID
	//    PropsAbout::CreateInstance,				// Method to create an instance of MyComponent
 //       NULL,									// Initialization function
 //       NULL									// Set-up information (for filters)
 //   }

};

// Generic way of determining the number of items in the template
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]); 

//COM Creator Function
CUnknown* WINAPI OggDemuxPageSourceFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
{
	OggDemuxPageSourceFilter *pNewObject = new OggDemuxPageSourceFilter();
    if (pNewObject == NULL) {
        *pHr = E_OUTOFMEMORY;
    }
    return pNewObject;
} 
//COM Interface query function
STDMETHODIMP OggDemuxPageSourceFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IFileSourceFilter) {
		*ppv = (IFileSourceFilter*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;
	/*} else if (riid == IID_IMediaSeeking) {
		*ppv = (IMediaSeeking*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;*/
	}/* else if (riid == IID_ISpecifyPropertyPages) {
		*ppv = (ISpecifyPropertyPages*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;
	}*/  else if (riid == IID_IAMFilterMiscFlags) {
		*ppv = (IAMFilterMiscFlags*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;
	//} else if (riid == IID_IAMMediaContent) {
	//	//debugLog<<"Queries for IAMMediaContent///"<<endl;
	//	*ppv = (IAMMediaContent*)this;
	//	((IUnknown*)*ppv)->AddRef();
	//	return NOERROR;
	}

	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv); 
}
OggDemuxPageSourceFilter::OggDemuxPageSourceFilter(void)
	:	CBaseFilter(NAME("OggDemuxPageSourceFilter"), NULL, m_pLock, CLSID_OggDemuxPageSourceFilter)
{
}

OggDemuxPageSourceFilter::~OggDemuxPageSourceFilter(void)
{
}

int OggDemuxPageSourceFilter::GetPinCount() 
{
	//TODO::: Implement
	return 0;//mStreamMapper->numStreams();
}
CBasePin* OggDemuxPageSourceFilter::GetPin(int inPinNo) 
{
	//TODO::: IMplement
	return NULL;
}

//IFileSource Interface
STDMETHODIMP OggDemuxPageSourceFilter::GetCurFile(LPOLESTR* outFileName, AM_MEDIA_TYPE* outMediaType) 
{
	////Return the filename and mediatype of the raw data
	//LPOLESTR x = SysAllocString(mFileName.c_str());
	//*outFileName = x;

	//TODO:::
	
	return S_OK;
}


STDMETHODIMP OggDemuxPageSourceFilter::Load(LPCOLESTR inFileName, const AM_MEDIA_TYPE* inMediaType) 
{
	////Initialise the file here and setup all the streams
	//CAutoLock locLock(m_pLock);
	//mFileName = inFileName;

	//debugLog<<"Loading : "<<StringHelper::toNarrowStr(mFileName)<<endl;

	//debugLog << "Opening source file : "<<StringHelper::toNarrowStr(mFileName)<<endl;
	//mSeekTable = new AutoOggSeekTable(StringHelper::toNarrowStr(mFileName));
	//mSeekTable->buildTable();
	//
	//return SetUpPins();

	//TODO:::
	return S_OK;
}
