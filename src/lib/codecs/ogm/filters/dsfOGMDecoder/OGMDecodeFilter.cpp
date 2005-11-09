
//===========================================================================
//Copyright (C) 2003, 2004 Zentaro Kavanagh
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
#include "OGMDecodeFilter.h"



//COM Factory Template
CFactoryTemplate g_Templates[] = 
{
    { 
		L"OGM Decode Filter",					// Name
	    &CLSID_OGMDecodeFilter,				// CLSID
	    OGMDecodeFilter::CreateInstance,		// Method to create an instance of Speex Decoder
        NULL,									// Initialization function
        NULL									// Set-up information (for filters)
    }

};

// Generic way of determining the number of items in the template
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]); 

OGMDecodeFilter::OGMDecodeFilter()
	:	CTransformFilter(NAME("OGM Video Decoder"), NULL, CLSID_OGMDecodeFilter)
	,	mInputPin(NULL)
	,	mOutputPin(NULL)
	
{

	
}



OGMDecodeFilter::~OGMDecodeFilter(void)
{

}

CUnknown* WINAPI OGMDecodeFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
{
	//This routine is the COM implementation to create a new Filter
	OGMDecodeFilter *pNewObject = new OGMDecodeFilter();
    if (pNewObject == NULL) {
        *pHr = E_OUTOFMEMORY;
    }
	return pNewObject;
} 

HRESULT OGMDecodeFilter::CheckInputType(const CMediaType* inMediaType)
{
	return mInputPin->CheckMediaType(inMediaType);
}
HRESULT OGMDecodeFilter::CheckTransform(const CMediaType* inInputMediaType, const CMediaType* inOutputMediaType)
{

	return S_OK;
}
HRESULT OGMDecodeFilter::DecideBufferSize(IMemAllocator* inAllocator, ALLOCATOR_PROPERTIES* inPropInputRequest)
{

	return S_OK;
}
HRESULT OGMDecodeFilter::GetMediaType(int inPosition, CMediaType* outMediaType)
{
	
	if (inPosition < 0) {
		return E_INVALIDARG;
	}
	
	if ((inPosition == 0) && (mInputPin != NULL) && (mInputPin->IsConnected())) {
		
		VIDEOINFOHEADER* locVideoFormat = (VIDEOINFOHEADER*)outMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
		*locVideoFormat = *mInputPin->getVideoFormatBlock();
		//FillMediaType(outMediaType, locVideoFormat->bmiHeader.biSizeImage);

		return S_OK;
	} else {
		return VFW_S_NO_MORE_ITEMS;
	}


}
HRESULT OGMDecodeFilter::Transform(IMediaSample* inInputSample, IMediaSample* inOutputSample)
{

	return S_OK;
}

CBasePin* OGMDecodeFilter::GetPin(int inPinNo)
{

    HRESULT locHR = S_OK;

    // Create an input pin if necessary

    if (m_pInput == NULL) {

        m_pInput = new OGMDecodeInputPin(this, &locHR);		//Deleted in base destructor

        
        if (m_pInput == NULL) {
            return NULL;
        }

		mInputPin = (OGMDecodeInputPin*)m_pInput;
        m_pOutput = new CTransformOutputPin(NAME("OGM Out"), this, &locHR, L"Video Out");	//Deleted in base destructor
			

        if (m_pOutput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the pin

    if (inPinNo == 0) {
        return m_pInput;
    } else if (inPinNo == 1) {
        return m_pOutput;
    } else {
        return NULL;
    }

}