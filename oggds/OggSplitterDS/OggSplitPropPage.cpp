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
#include "..\resource.h"
#include <atlbase.h>
#include <ocidl.h>

// Property Page
COggSplitPropPage::COggSplitPropPage(LPUNKNOWN pUnk, HRESULT *phr) :
	CBasePropertyPage(NAME("Vorbis Compressor property page"), pUnk,
					  IDD_PROPPAGE_SPLITTER, IDS_PROPPAGE_SPLITTER_TITLE)
{
	m_pPersistPropertyBag = NULL;
	*phr = NOERROR;
}

HRESULT COggSplitPropPage::OnConnect(IUnknown *pUnknown)
{
	return pUnknown->QueryInterface(IID_IPersistPropertyBag, (void **)&m_pPersistPropertyBag);
}

HRESULT COggSplitPropPage::OnDisconnect()
{
	if (m_pPersistPropertyBag)
	{
		m_pPersistPropertyBag->Release();
		m_pPersistPropertyBag = NULL;
	}
    return NOERROR;
}

void COggSplitPropPage::DeleteFourCC(char* wzFOURCC)
{
	int		iIndex = 0;
	char	szBuffer[20];

	while (SendDlgItemMessage(m_Dlg, IDC_FOURCC_MAPPINGS, LB_GETTEXT,
		                       (LPARAM)iIndex, (WPARAM)szBuffer) != LB_ERR)
	{
		if (strncmp(szBuffer, wzFOURCC, 4) == 0)
		{
			SendDlgItemMessage(m_Dlg, IDC_FOURCC_MAPPINGS, LB_DELETESTRING, (LPARAM)iIndex, 0);
			return;
		}
		iIndex++;
	}
}

void COggSplitPropPage::SetProperty(const char* pProp, char* pVal)
{
	VARIANT				V;
	IPropertyBag*		pPBag;
	wchar_t				wzName[128];

	mbstowcs(wzName, pProp, sizeof(wzName));

	V.vt = VT_BSTR;
	V.bstrVal = SysAllocStringLen(NULL, strlen(pVal));
	mbstowcs(V.bstrVal, pVal, strlen(pVal)+1);

	m_pPersistPropertyBag->QueryInterface(IID_IPropertyBag, (void**)&pPBag);
	pPBag->Write(wzName, &V);
	pPBag->Release();
	VariantClear(&V);
}

BOOL COggSplitPropPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int			iIndex;
	char		szMap[20];
	char		szSrc[20];
	char		szDst[20];
	char		szEmpty = '\0';

	if (uMsg == WM_COMMAND)
	{
		if ((LOWORD(wParam) == IDC_SEARCH_KEYFRAME && HIWORD(wParam)  == BN_CLICKED) ||
			(LOWORD(wParam) == IDC_ALWAYS_ENABLE_ALL && HIWORD(wParam)  == BN_CLICKED) ||
			(LOWORD(wParam) == IDC_SHOWTRAYICON && HIWORD(wParam)  == BN_CLICKED))
		{
			m_bDirty = TRUE;
			m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
			return TRUE;
		}

		if (LOWORD(wParam) == IDC_ENABLE_ALL_STREAMS && HIWORD(wParam)  == BN_CLICKED)
		{
			SetProperty(idEnableAllStreams, "1");
			return TRUE;
		}

	
		if (LOWORD(wParam) == IDC_FOURCC_MAPPINGS && HIWORD(wParam)  == LBN_SELCHANGE)
		{
			char*	pPos;

			iIndex = SendDlgItemMessage(m_Dlg, IDC_FOURCC_MAPPINGS, LB_GETCURSEL, 0, 0);
			EnableWindow(GetDlgItem(m_Dlg, IDC_FOURCC_SET), iIndex >= 0);
			EnableWindow(GetDlgItem(m_Dlg, IDC_FOURCC_DEL), iIndex >= 0);

			if (iIndex < 0)
			{
				SendDlgItemMessage(m_Dlg, IDC_SRC_FOURCC, WM_SETTEXT, 0, (WPARAM)&szEmpty);
				SendDlgItemMessage(m_Dlg, IDC_DST_FOURCC, WM_SETTEXT, 0, (WPARAM)&szEmpty);
				return TRUE;
			}

			SendDlgItemMessage(m_Dlg, IDC_FOURCC_MAPPINGS, LB_GETTEXT, (LPARAM)iIndex, (WPARAM)szMap);
			pPos = strstr(szMap, "  ->  ");

			if (pPos != NULL) *pPos = '\0';

			SendDlgItemMessage(m_Dlg, IDC_SRC_FOURCC, WM_SETTEXT, 0, (WPARAM)szMap);
			if (pPos)
				SendDlgItemMessage(m_Dlg, IDC_DST_FOURCC, WM_SETTEXT, 0, (WPARAM)(pPos+6));
			else
				SendDlgItemMessage(m_Dlg, IDC_DST_FOURCC, WM_SETTEXT, 0, (WPARAM)&szEmpty);
			return TRUE;
		}

		if (LOWORD(wParam) == IDC_SRC_FOURCC && HIWORD(wParam)  == EN_CHANGE)
		{
			SendDlgItemMessage(m_Dlg, IDC_SRC_FOURCC, WM_GETTEXT, 20, (WPARAM)szMap);
			EnableWindow(GetDlgItem(m_Dlg, IDC_FOURCC_SET), *szMap != '\0');
			EnableWindow(GetDlgItem(m_Dlg, IDC_FOURCC_DEL), *szMap != '\0');
			return TRUE;
		}
		
		if (LOWORD(wParam) == IDC_FOURCC_SET && HIWORD(wParam)  == BN_CLICKED)
		{
			int i;

			SendDlgItemMessage(m_Dlg, IDC_SRC_FOURCC, WM_GETTEXT, 20, (WPARAM)szSrc);
			SendDlgItemMessage(m_Dlg, IDC_DST_FOURCC, WM_GETTEXT, 20, (WPARAM)szDst);
			for (i=strlen(szSrc); i<4; i++)
				szSrc[i] = L' ';
			szSrc[4] = '\0';
			for (i=strlen(szDst); i<4; i++)
				szDst[i] = L' ';
			szDst[4] = L'\0';

			DeleteFourCC(szSrc);
			wsprintf(szMap, "%s  ->  %s", szSrc, szDst);
			SendDlgItemMessage(m_Dlg, IDC_FOURCC_MAPPINGS, LB_ADDSTRING, 0, (LPARAM)szMap);

			SetProperty(szSrc, szDst);
			return TRUE;
		}

		if (LOWORD(wParam) == IDC_FOURCC_DEL && HIWORD(wParam)  == BN_CLICKED)
		{
			int i;

			SendDlgItemMessage(m_Dlg, IDC_SRC_FOURCC, WM_GETTEXT, 20, (WPARAM)szSrc);
			SendDlgItemMessage(m_Dlg, IDC_DST_FOURCC, WM_GETTEXT, 20, (WPARAM)szDst);
			for (i=strlen(szSrc); i<4; i++)
				szSrc[i] = ' ';
			szSrc[4] = '\0';
			for (i=strlen(szDst); i<4; i++)
				szDst[i] = ' ';
			szDst[4] = '\0';

			DeleteFourCC(szSrc);

			SendDlgItemMessage(m_Dlg, IDC_SRC_FOURCC, WM_SETTEXT, 0, (WPARAM)&szEmpty);
			SendDlgItemMessage(m_Dlg, IDC_DST_FOURCC, WM_SETTEXT, 0, (WPARAM)&szEmpty);

			SetProperty(szSrc, "");
			return TRUE;
		}


	}

	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT COggSplitPropPage::OnActivate(void)
{
	SendDlgItemMessage(m_Dlg, IDC_SRC_FOURCC, EM_LIMITTEXT, 4, 0);
	SendDlgItemMessage(m_Dlg, IDC_DST_FOURCC, EM_LIMITTEXT, 4, 0);
	return m_pPersistPropertyBag->Save(this, TRUE, TRUE);
}

HRESULT COggSplitPropPage::OnApplyChanges(void)
{
	return m_pPersistPropertyBag->Load(this, NULL);
}

HRESULT STDMETHODCALLTYPE COggSplitPropPage::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
	char	szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));
	VariantClear(pVar);
	
	if (strcmp(szName, idSeekToKeyFrame) == 0)
	{
		pVar->vt = VT_BOOL;
		pVar->boolVal = (SendDlgItemMessage(m_Dlg, IDC_SEARCH_KEYFRAME, BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;
		return NOERROR;
	}
	if (strcmp(szName, idAlwaysEnableAllStreams) == 0)
	{
		pVar->vt = VT_BOOL;
		pVar->boolVal = (SendDlgItemMessage(m_Dlg, IDC_ALWAYS_ENABLE_ALL, BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;
		return NOERROR;
	}
	if (strcmp(szName, idShowTrayIcon) == 0)
	{
		pVar->vt = VT_BOOL;
		pVar->boolVal = (SendDlgItemMessage(m_Dlg, IDC_SHOWTRAYICON, BM_GETSTATE, 0, 0) & BST_CHECKED) != 0;
		return NOERROR;
	}
	
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE COggSplitPropPage::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	char	szName[128];

	wcstombs(szName, pszPropName, sizeof(szName));

	if (strcmp(szName, idSeekToKeyFrame) == 0)
	{
		WPARAM	CheckState;

		VariantChangeType(pVar, pVar, 0, VT_BOOL);
		CheckState = pVar->boolVal ? BST_CHECKED : BST_UNCHECKED;
		SendDlgItemMessage(m_Dlg, IDC_SEARCH_KEYFRAME, BM_SETCHECK, CheckState, 0);
		return NOERROR;
	}
	if (strcmp(szName, idAlwaysEnableAllStreams) == 0)
	{
		WPARAM	CheckState;

		VariantChangeType(pVar, pVar, 0, VT_BOOL);
		CheckState = pVar->boolVal ? BST_CHECKED : BST_UNCHECKED;
		SendDlgItemMessage(m_Dlg, IDC_ALWAYS_ENABLE_ALL, BM_SETCHECK, CheckState, 0);
		return NOERROR;
	}
	if (strcmp(szName, idShowTrayIcon) == 0)
	{
		WPARAM	CheckState;

		VariantChangeType(pVar, pVar, 0, VT_BOOL);
		CheckState = pVar->boolVal ? BST_CHECKED : BST_UNCHECKED;
		SendDlgItemMessage(m_Dlg, IDC_SHOWTRAYICON, BM_SETCHECK, CheckState, 0);
		return NOERROR;
	}

	if (wcslen(pszPropName) == 4)
	{
		char	szCaption[15];

		VariantChangeType(pVar, pVar, 0, VT_BSTR);
		wsprintf(szCaption, "%S  ->  %S", pszPropName, pVar->bstrVal);
		SendDlgItemMessage(m_Dlg, IDC_FOURCC_MAPPINGS, LB_ADDSTRING, 0, (LPARAM)szCaption);
	}

	return E_FAIL;

}

STDMETHODIMP COggSplitPropPage::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IPropertyBag)
	{
        CheckPointer(ppv, E_POINTER);
		return GetInterface((IPropertyBag*)(this), ppv);
	}
    else return CBasePropertyPage::NonDelegatingQueryInterface(riid, ppv);
}

CUnknown *COggSplitPropPage::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new COggSplitPropPage(pUnk, phr);
}
