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

#include "VorbEncDS.h"
#include "..\resource.h"
#include <atlbase.h>
#include <ocidl.h>

//
// Property Page
//

CVorbisEncPropPage::CVorbisEncPropPage(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("Vorbis Compressor property page"), pUnk,
					  IDD_PROPPAGE_VORBIS, IDS_PROPPAGE_VORBIS_TITLE),
	m_pPersistPropertyBag(NULL)
{
	m_pPersistPropertyBag = NULL;
	*phr = NOERROR;
}

STDMETHODIMP CVorbisEncPropPage::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPropertyBag*)(this), ppv);
	}
    return CBasePropertyPage::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CVorbisEncPropPage::OnConnect(IUnknown *pUnknown)
{
	return pUnknown->QueryInterface(IID_IPersistPropertyBag, (void **)&m_pPersistPropertyBag);
}

HRESULT CVorbisEncPropPage::OnDisconnect()
{
	if (m_pPersistPropertyBag)
	{
		m_pPersistPropertyBag->Release();
		m_pPersistPropertyBag = NULL;
	}
    return NOERROR;
}

HRESULT CVorbisEncPropPage::OnActivate()
{
	m_iMessagesFromSet = 0;
	return m_pPersistPropertyBag->Save(this, TRUE, TRUE);
}
	
BOOL CVorbisEncPropPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND && HIWORD(wParam) == EN_CHANGE)
	{
		if (!m_iMessagesFromSet)
		{
	        m_bDirty = TRUE;
			m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
		}
		else m_iMessagesFromSet--;
	}
	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CVorbisEncPropPage::OnApplyChanges(void)
{
	return m_pPersistPropertyBag->Load(this, NULL);
}

HRESULT STDMETHODCALLTYPE CVorbisEncPropPage::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
	char	szName[128];
	char	szBuffer[16];

	wcstombs(szName, pszPropName, sizeof(szName));

	if (strcmp(szName, idMinBitrate) == 0)
		SendDlgItemMessage(m_Dlg, IDC_MINBITRATE, WM_GETTEXT, 16, (LPARAM)szBuffer);
	else if (strcmp(szName, idAvgBitrate) == 0)
		SendDlgItemMessage(m_Dlg, IDC_AVGBITRATE, WM_GETTEXT, 16, (LPARAM)szBuffer);
	else if (strcmp(szName, idMaxBitrate) == 0)
		SendDlgItemMessage(m_Dlg, IDC_MAXBITRATE, WM_GETTEXT, 16, (LPARAM)szBuffer);
	else if (strcmp(szName, idQuality) == 0)
		SendDlgItemMessage(m_Dlg, IDC_QUALITY, WM_GETTEXT, 16, (LPARAM)szBuffer);
	else return E_FAIL;

	VariantClear(pVar);
	pVar->vt = VT_BSTR;
	pVar->bstrVal = SysAllocStringLen(NULL, strlen(szBuffer));
	mbstowcs(pVar->bstrVal, szBuffer, strlen(szBuffer));
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CVorbisEncPropPage::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	char	szName[128];
	char	szBuffer[16];

	VariantChangeType(pVar, pVar, 0, VT_BSTR);
	wcstombs(szName, pszPropName, sizeof(szName));
	wcstombs(szBuffer, pVar->bstrVal, sizeof(szBuffer));

	if (strcmp(szName, idMinBitrate) == 0)
		SendDlgItemMessage(m_Dlg, IDC_MINBITRATE, WM_SETTEXT, 0, (LPARAM)szBuffer);
	else if (strcmp(szName, idAvgBitrate) == 0)
		SendDlgItemMessage(m_Dlg, IDC_AVGBITRATE, WM_SETTEXT, 0, (LPARAM)szBuffer);
	else if (strcmp(szName, idMaxBitrate) == 0)
		SendDlgItemMessage(m_Dlg, IDC_MAXBITRATE, WM_SETTEXT, 0, (LPARAM)szBuffer);
	else if (strcmp(szName, idQuality) == 0)
		SendDlgItemMessage(m_Dlg, IDC_QUALITY, WM_SETTEXT, 0, (LPARAM)szBuffer);
	else return E_FAIL;

	m_iMessagesFromSet++;
	return NOERROR;
}

CUnknown *CVorbisEncPropPage::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CVorbisEncPropPage(pUnk, phr);
}
