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

#include "VorbDecDS.h"
#include "..\resource.h"
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <stdio.h>
//
// Property Page
//

CVorbisDecPropPage::CVorbisDecPropPage(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("Vorbis decoder property page"), pUnk,
					  IDD_PROPPAGE_VORBDEC, IDS_PROPPAGE_VORBDEC_TITLE)
{
	m_pPropBag = NULL;
	*phr = NOERROR;
}

STDMETHODIMP CVorbisDecPropPage::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPropertyBag*)(this), ppv);
	}
    return CBasePropertyPage::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CVorbisDecPropPage::OnConnect(IUnknown *pUnknown)
{
	return pUnknown->QueryInterface(IID_IPersistPropertyBag, (void **)&m_pPropBag);
}

HRESULT CVorbisDecPropPage::OnDisconnect()
{
	if (m_pPropBag)
	{
		m_pPropBag->Release();
		m_pPropBag = NULL;
	}
    return NOERROR;
}

HRESULT CVorbisDecPropPage::OnActivate()
{
	return m_pPropBag->Save(this, TRUE, TRUE);
}
	
BOOL CVorbisDecPropPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND)
	{
		if (HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == IDC_FLOAT_PREFERRED ||LOWORD(wParam) == IDC_INT_PREFERRED)
			{
		        m_bDirty = TRUE;
				m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
				return TRUE;
			}
		}
	}
	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CVorbisDecPropPage::OnApplyChanges(void)
{
	return m_pPropBag->Load(this, NULL);
}

HRESULT STDMETHODCALLTYPE CVorbisDecPropPage::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
	char					szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));

	if (strcmp(szName, idFloatModeFirst) == 0)
	{
		VariantClear(pVar); pVar->vt = VT_I4;
		pVar->lVal = SendDlgItemMessage(m_Dlg, IDC_FLOAT_PREFERRED, BM_GETCHECK , 0, 0) == BST_CHECKED ? 1 : 0;
		return NOERROR;
	}
	
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CVorbisDecPropPage::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	char	szBuffer[128];
	char	szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));

	if (strcmp(szName, idPostGain) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_R4);
		sprintf(szBuffer, "%#4f", pVar->fltVal);
		SendDlgItemMessage(m_Dlg, IDC_DEC_POSTGAIN, WM_SETTEXT, 0, (LPARAM)szBuffer);
		return NOERROR;
	}
	if (strcmp(szName, idFloatModeFirst) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);
		SendDlgItemMessage(m_Dlg, IDC_FLOAT_PREFERRED, BM_SETCHECK , pVar->lVal != 0 ? BST_CHECKED : BST_UNCHECKED, 0);
		SendDlgItemMessage(m_Dlg, IDC_INT_PREFERRED, BM_SETCHECK , pVar->lVal == 0 ? BST_CHECKED : BST_UNCHECKED, 0);
		return NOERROR;
	}
	if (strcmp(szName, idOutputMode) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_I4);

		if (pVar->lVal == WAVE_FORMAT_PCM)
			SendDlgItemMessage(m_Dlg, IDC_DEC_OUTFORMAT, WM_SETTEXT, 0, (LPARAM)szFormatPCM);
		else if (pVar->lVal == WAVE_FORMAT_IEEE_FLOAT)
			SendDlgItemMessage(m_Dlg, IDC_DEC_OUTFORMAT, WM_SETTEXT, 0, (LPARAM)szFormatIEEEFloat);
		else
			SendDlgItemMessage(m_Dlg, IDC_DEC_OUTFORMAT, WM_SETTEXT, 0, (LPARAM)"\0");
		return NOERROR;
	}
	if (strcmp(szName, idChannels) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_BSTR);
		wcstombs(szBuffer, pVar->bstrVal, sizeof(szBuffer));
		SendDlgItemMessage(m_Dlg, IDC_DEC_CHANNELS, WM_SETTEXT, 0, (LPARAM)szBuffer);
		return NOERROR;
	}
	if (strcmp(szName, idSamplesPerSec) == 0)
	{
		VariantChangeType(pVar, pVar, 0, VT_BSTR);
		wcstombs(szBuffer, pVar->bstrVal, sizeof(szBuffer));
		SendDlgItemMessage(m_Dlg, IDC_DEC_SAMPLESPERSEC, WM_SETTEXT, 0, (LPARAM)szBuffer);
		return NOERROR;
	}

	return E_FAIL;
}

CUnknown *CVorbisDecPropPage::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CVorbisDecPropPage(pUnk, phr);
}
