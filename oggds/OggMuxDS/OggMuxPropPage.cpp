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

#include "OggMuxDS.h"
#include "..\resource.h"

//
// Property Page
//

COggMuxPropPage::COggMuxPropPage(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("Ogg Multiplexer property page"), pUnk,
					  IDD_PROPPAGE_MUXER, IDS_PROPPAGE_MUXER_TITLE),
	m_pFilter(NULL)
{
	m_pFilter = NULL;
	*phr = NOERROR;
}

STDMETHODIMP COggMuxPropPage::NonDelegatingQueryInterface(REFIID riid, void **ppv) // For PropertyPages
{
    if(riid == IID_IPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPropertyBag*)(this), ppv);
    }
	return CBasePropertyPage::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT COggMuxPropPage::OnConnect(IUnknown *pUnknown)
{
	return pUnknown->QueryInterface(IID_IBaseFilter, (void **)&m_pFilter);
}

HRESULT COggMuxPropPage::OnDisconnect()
{
	if (m_pFilter)
	{
		m_pFilter->Release();
		m_pFilter = NULL;
	}
    return NOERROR;
}

HRESULT COggMuxPropPage::OnActivate()
{
	HRESULT		hr;
	IEnumPins	*pEnum;
	IPin		*pPin;
	ULONG		cFetched;
	VARIANT		V;

	VariantInit(&V);

	// Load the values for the language combo box
	SendDlgItemMessage(m_Dlg, IDC_LANGSELECT, CB_LIMITTEXT, (WPARAM)127, 0);
	for (int i=0; i<SIZEOF_ARRAY(aLCID); i++)
		SendDlgItemMessage(m_Dlg, IDC_LANGSELECT, CB_ADDSTRING, 0, (LPARAM)aLCID[i].caption);

	// Walk through all pins ...
	m_pFilter->EnumPins(&pEnum);
	hr = pEnum->Next(1, &pPin, &cFetched);
	while (SUCCEEDED(hr) && cFetched != 0)
	{
		PIN_INFO			PinInfo;

		pPin->QueryPinInfo(&PinInfo);
		PinInfo.pFilter->Release();

		if (PinInfo.dir == PINDIR_INPUT)
		{
			int					iIndex;
			IPropertyBag		*pPropBag;
			IPersistPropertyBag	*pPersistPropBag;
			char				cCaption[256];
			
			wcstombs(cCaption, PinInfo.achName, sizeof(cCaption));
			for (int i=strlen(cCaption); i<12; i++)
				cCaption[i] = ' ';
			pPin->QueryInterface(IID_IPropertyBag, (void**)&pPropBag);
			if SUCCEEDED(pPropBag->Read(L"LANGUAGE", &V, NULL))
				wcstombs(cCaption+12, V.bstrVal, sizeof(cCaption)-12);
			else
				cCaption[12] = '\0';
			pPropBag->Release();
			iIndex = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_ADDSTRING, 0, (LPARAM)cCaption); 

			pPin->QueryInterface(IID_IPersistPropertyBag, (void**)&pPersistPropBag);
			SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_SETITEMDATA, iIndex, (LPARAM) pPersistPropBag); 
		}
		pPin->Release();
		hr = pEnum->Next(1, &pPin, &cFetched);
	}
	pEnum->Release();	

	VariantClear(&V);

	// Select the first stream ...
	SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_SETCURSEL, 0, 0);
	LoadCurrentStreamComments();

	return NOERROR;
}
	
HRESULT COggMuxPropPage::OnDeactivate()
{
	// Release the IPersistPropertyBag pointers
	int iCount = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETCOUNT, 0, 0);
	for (int i=0; i<iCount; i++)
	{
		IPersistPropertyBag*	pPPBag;
		pPPBag = (IPersistPropertyBag*) SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETITEMDATA, i, 0);
		pPPBag->Release();
	}
	return NOERROR;
}

void COggMuxPropPage::LoadCurrentStreamComments()
{
	long					iIndex;
	IPersistPropertyBag*	pPPBag;

	SendDlgItemMessage(m_Dlg, IDC_TAGS, LB_RESETCONTENT, 0, 0);
	SendDlgItemMessage(m_Dlg, IDC_LANGSELECT, CB_SETCURSEL, (WPARAM)-1, 0);
	iIndex = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETCURSEL, 0, 0);

	if (iIndex >= 0)
	{
		long	iItemData;
		char	cCaption[1024];

		SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETTEXT, iIndex, (LPARAM)cCaption);
		cCaption[12] = '\0';
		iItemData = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETITEMDATA, iIndex, 0);
		SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_DELETESTRING, iIndex, 0);
		iIndex = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_INSERTSTRING, iIndex, (LPARAM)cCaption);
		SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_SETITEMDATA, iIndex, (LPARAM)iItemData); 
		SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_SETCURSEL, iIndex, 0);

		pPPBag = (IPersistPropertyBag*) SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETITEMDATA, iIndex, 0);
		pPPBag->Save(this, TRUE, TRUE);
	}

	EnableWindow(GetDlgItem(m_Dlg, IDC_COMMDELETE), FALSE);
	EnableWindow(GetDlgItem(m_Dlg, IDC_COMMENT_SET), FALSE);

	char	cEmpty = '\0';

	SendDlgItemMessage(m_Dlg, IDC_COMMENT_TAG, WM_SETTEXT, 0, (WPARAM)&cEmpty);
	SendDlgItemMessage(m_Dlg, IDC_COMMENT_VALUE, WM_SETTEXT, 0, (WPARAM)&cEmpty);
}

void COggMuxPropPage::SetProperty(char* pProp, char* pVal)
{
	VARIANT					V;
	IPersistPropertyBag*	pPPBag;
	IPropertyBag*			pPBag;
	long					iIndex;
	wchar_t				wcProp[32];

	V.vt = VT_BSTR;
	V.bstrVal = SysAllocStringLen(NULL, strlen(pVal)+1);
	mbstowcs(V.bstrVal, pVal, strlen(pVal)+1);
	mbstowcs(wcProp, pProp, strlen(pProp)+1);

	iIndex = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETCURSEL, 0, 0);
	pPPBag = (IPersistPropertyBag*) SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETITEMDATA, iIndex, 0);
	pPPBag->QueryInterface(IID_IPropertyBag, (void**)&pPBag);
	pPBag->Write(wcProp, &V);
	pPBag->Release();
	VariantClear(&V);
	
	LoadCurrentStreamComments();
}

BOOL COggMuxPropPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	long			iIndex;
	char			cTag[1024];
	char			cValue[1024];
	char			cEmpty = '\0';

	if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_COMMENT_TAG)
		if (HIWORD(wParam)  == EN_CHANGE)
		{
			SendDlgItemMessage(m_Dlg, IDC_COMMENT_TAG, WM_GETTEXT, 1024, (WPARAM)cTag);
			EnableWindow(GetDlgItem(m_Dlg, IDC_COMMDELETE), strlen(cTag) > 0);
			EnableWindow(GetDlgItem(m_Dlg, IDC_COMMENT_SET), strlen(cTag) > 0);
			return TRUE;
		}

	if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_COMMDELETE)
		if (HIWORD(wParam)  == BN_CLICKED)
		{
			SendDlgItemMessage(m_Dlg, IDC_COMMENT_TAG, WM_GETTEXT, 1024, (WPARAM)cTag);
			SetProperty(cTag, "");
			LoadCurrentStreamComments();
			return TRUE;
		}
		
	if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_COMMENT_SET)
		if (HIWORD(wParam)  == BN_CLICKED)
		{
			SendDlgItemMessage(m_Dlg, IDC_COMMENT_TAG, WM_GETTEXT, 1024, (WPARAM)cTag);
			SendDlgItemMessage(m_Dlg, IDC_COMMENT_VALUE, WM_GETTEXT, 1024, (WPARAM)cValue);
			SetProperty(cTag, cValue);
			LoadCurrentStreamComments();
			return TRUE;
		}

	if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_MUXER_CHANNELS)
		if (HIWORD(wParam)  == LBN_SELCHANGE)
		{
			LoadCurrentStreamComments();
			return TRUE;
		}
		
	if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_TAGS)
		if (HIWORD(wParam)  == LBN_SELCHANGE)
		{
			iIndex = SendDlgItemMessage(m_Dlg, IDC_TAGS, LB_GETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(m_Dlg, IDC_COMMDELETE), iIndex >= 0);
			EnableWindow(GetDlgItem(m_Dlg, IDC_COMMENT_SET), iIndex >= 0);
			
			char*	pPos;

			if (iIndex < 0)
			{
				SendDlgItemMessage(m_Dlg, IDC_COMMENT_TAG, WM_SETTEXT, 0, (WPARAM)&cEmpty);
				SendDlgItemMessage(m_Dlg, IDC_COMMENT_VALUE, WM_SETTEXT, 0, (WPARAM)&cEmpty);
				return TRUE;
			}

			SendDlgItemMessage(m_Dlg, IDC_TAGS, LB_GETTEXT, (LPARAM)iIndex, (WPARAM)cTag);
			pPos = strstr(cTag, "=");
			if (pPos != NULL) *pPos = '\0';

			SendDlgItemMessage(m_Dlg, IDC_COMMENT_TAG, WM_SETTEXT, 0, (WPARAM)cTag);
			if (pPos)
				SendDlgItemMessage(m_Dlg, IDC_COMMENT_VALUE, WM_SETTEXT, 0, (WPARAM)(pPos+1));
			else
				SendDlgItemMessage(m_Dlg, IDC_COMMENT_VALUE, WM_SETTEXT, 0, (WPARAM)&cEmpty);
			return TRUE;
		}

	if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_LANGSELECT)
	{
		char					cLanguage[256];

		if (HIWORD(wParam)  == CBN_SELCHANGE)
		{
			int iIndex = SendDlgItemMessage(m_Dlg, IDC_LANGSELECT, CB_GETCURSEL, 0, 0);
			SendDlgItemMessage(m_Dlg, IDC_LANGSELECT, CB_GETLBTEXT, (WPARAM)iIndex, (LPARAM)cLanguage);
			SetProperty("LANGUAGE", cLanguage);
			LoadCurrentStreamComments();
			return TRUE;
		}
	}		
		
	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT COggMuxPropPage::OnApplyChanges(void)
{
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE COggMuxPropPage::Read(LPCOLESTR pszPropName, VARIANT *pVar,
												IErrorLog *pErrorLog)
{
	return NOERROR;
}

HRESULT	STDMETHODCALLTYPE COggMuxPropPage::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	char	szName[128];
	char	szCaption[1024];

	wcstombs(szName, pszPropName, sizeof(szName));

	strupr(szName);
	VariantChangeType(pVar, pVar, 0, VT_BSTR);
	
	if (strcmp(szName, "LANGUAGE") == 0)
	{
		int iIndex;
			
		// update the combo box
		wcstombs(szCaption, pVar->bstrVal, sizeof(szCaption));
		iIndex = SendDlgItemMessage(m_Dlg, IDC_LANGSELECT, CB_FINDSTRING, (WPARAM)-1, (LPARAM)szCaption);
		SendDlgItemMessage(m_Dlg, IDC_LANGSELECT, CB_SETCURSEL, iIndex, 0);

		//update the channel list
		iIndex = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETCURSEL, 0, 0);
		if (iIndex >= 0)
		{
			long	iItemData;

			SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETTEXT, iIndex, (LPARAM)szCaption);
			wcstombs(szCaption+12, pVar->bstrVal, sizeof(szCaption)-12);
			iItemData = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_GETITEMDATA, iIndex, 0);
			SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_DELETESTRING, iIndex, 0);
			iIndex = SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_INSERTSTRING, iIndex, (LPARAM)szCaption);
			SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_SETITEMDATA, iIndex, (LPARAM)iItemData); 
			SendDlgItemMessage(m_Dlg, IDC_MUXER_CHANNELS, LB_SETCURSEL, iIndex, 0);
		}
	}

	wsprintf(szCaption, "%s=%S", szName, pVar->bstrVal);
	SendDlgItemMessage(m_Dlg, IDC_TAGS, LB_ADDSTRING, 0, (LPARAM)szCaption);

	return NOERROR;
}


CUnknown *COggMuxPropPage::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new COggMuxPropPage(pUnk, phr);
}
