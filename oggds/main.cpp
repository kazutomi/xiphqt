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

// This file contains the structures to register the filters and some
// additional code the register file types etc.

#include <streams.h>     // DirectShow (includes windows.h)
#include "initguid.h"
#include "main.h"
#include "tchar.h"
#include <ShObjIdl.h>
#include "OggSplitterDS/OggSplitterDS.h"
#include "OggMuxDS/OggMuxDS.h"
#include "VorbEncDS/VorbEncDS.h"
#include "VorbDecDS/VorbDecDS.h"
#include "AboutPage.h"
#include "common.h"

const AMOVIESETUP_MEDIATYPE sudPCMAudio =
{ &MEDIATYPE_Audio        // clsMajorType
, &MEDIASUBTYPE_PCM };     // clsMinorType

const AMOVIESETUP_MEDIATYPE sudVorbis =
{ &MEDIATYPE_Audio      // clsMajorType
, &MEDIASUBTYPE_Vorbis };  // clsMinorType

const AMOVIESETUP_MEDIATYPE sudOggPackets =
{ &MEDIATYPE_Audio      // clsMajorType
, &MEDIASUBTYPE_Vorbis };  // clsMinorType

const AMOVIESETUP_MEDIATYPE sudOggStreams =
{ &MEDIATYPE_Stream      // clsMajorType
, &MEDIASUBTYPE_Ogg };

static const WCHAR sudOggSplitName[]  = L"Ogg Splitter";
static const WCHAR sudOggMuxName[]    = L"Ogg Multiplexer";
static const WCHAR sudVorbisEncName[] = L"Vorbis Encoder";
static const WCHAR sudVorbisDecName[] = L"Vorbis Decoder";
static const WCHAR sudOggDSAboutPageName[] = L"OggDS about page";
static const WCHAR sudOggMuxPropPageName[] = L"Ogg Multiplexer property page";
static const WCHAR sudOggSplitPropPageName[] = L"Ogg Splitter property page";
static const WCHAR sudVorbisEncPropPageName[] = L"Vorbis Compressor property page";
static const WCHAR sudVorbisDecPropPageName[] = L"Vorbis decoder property page";

#define OggFileDesc  "Ogg Vorbis / Media File (*.ogg;*.ogm)"
#define OggFileDesc2 "Ogg Vorbis / Media File"
#define OggFileTypes "*.ogg;*.ogm"
#define OgmFileDesc  "Ogg media file"

const AMOVIESETUP_PIN sudOggSplitPins[] =
{ { L"Input"            // strName
  , FALSE               // bRendered
  , FALSE               // bOutput
  , FALSE               // bZero
  , FALSE               // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L""                 // strConnectsToPin
  , 1                   // nTypes
  , &sudOggStreams      // lpTypes
  }
, { L"Output"           // strName
  , FALSE               // bRendered
  , TRUE                // bOutput
  , FALSE               // bZero
  , TRUE                // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L""                 // strConnectsToPin
  , 0                   // nTypes
  , NULL				// lpTypes
  }
};

const AMOVIESETUP_FILTER sudOggSplit =
{ &CLSID_OggSplitter      // clsID
, sudOggSplitName            // strName
, MERIT_NORMAL            // dwMerit
, 2                        // nPins
, sudOggSplitPins};	      // lpPin

const AMOVIESETUP_PIN sudOggMuxPins[] =
{ { L"Input"            // strName
  , FALSE               // bRendered
  , FALSE               // bOutput
  , FALSE               // bZero
  , TRUE                // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L""                 // strConnectsToPin
  , 0                   // nTypes
  , NULL				// lpTypes
  }
, { L"Output"           // strName
  , FALSE               // bRendered
  , TRUE                // bOutput
  , FALSE               // bZero
  , FALSE               // bMany
  , &CLSID_NULL         // clsConnectsToFilter
  , L""                 // strConnectsToPin
  , 1                   // nTypes
  , &sudOggStreams      // lpTypes
  }
};

const AMOVIESETUP_FILTER sudOggMux =
{ &CLSID_OggMux                   // clsID
, sudOggMuxName            // strName
, MERIT_UNLIKELY           // dwMerit
, 2                        // nPins
, sudOggMuxPins};	      // lpPin

const AMOVIESETUP_PIN sudVorbisEncPins[] =
{
	{ L"Input"            // strName
	, FALSE               // bRendered
	, FALSE               // bOutput
	, FALSE               // bZero
	, FALSE               // bMany
	, &CLSID_NULL         // clsConnectsToFilter
	, L""                 // strConnectsToPin
	, 1                   // nTypes
	, &sudPCMAudio        // lpTypes
	}
,	{ L"Output"           // strName
	, FALSE               // bRendered
	, TRUE                // bOutput
	, FALSE               // bZero
	, FALSE               // bMany
	, &CLSID_NULL         // clsConnectsToFilter
	, L""                 // strConnectsToPin
	, 1                   // nTypes
	, &sudVorbis          // lpTypes
	}
};

const AMOVIESETUP_FILTER sudVorbisEnc =
{ &CLSID_VorbisEnc                // clsID
, sudVorbisEncName                   // strName
, MERIT_DO_NOT_USE                // dwMerit
, 2                               // nPins
, sudVorbisEncPins};

const REGFILTER2 rf2FilterReg =
{
    1,                  // Version 1 (no pin mediums or pin category).
    MERIT_DO_NOT_USE,   // Merit.
    2,                  // Number of pins.
    (AMOVIESETUP_PIN*)&sudVorbisEncPins   // Pointer to pin information.
};

const AMOVIESETUP_PIN sudVorbisDecPins[] =
{
	{ L"Input"            // strName
	, FALSE               // bRendered
	, FALSE               // bOutput
	, FALSE               // bZero
	, FALSE               // bMany
	, &CLSID_NULL         // clsConnectsToFilter
	, L""                 // strConnectsToPin
	, 1                   // nTypes
	, &sudVorbis          // lpTypes
	}
,	{ L"Output"           // strName
	, FALSE               // bRendered
	, TRUE                // bOutput
	, FALSE               // bZero
	, FALSE               // bMany
	, &CLSID_NULL         // clsConnectsToFilter
	, L""                 // strConnectsToPin
	, 1                   // nTypes
	, &sudPCMAudio        // lpTypes
	}
};

const AMOVIESETUP_FILTER sudVorbisDec =
{ &CLSID_VorbisDec                // clsID
, sudVorbisDecName                // strName
, MERIT_NORMAL                    // dwMerit
, 2                               // nPins
, sudVorbisDecPins};

// Needed for the CreateInstance mechanism
CFactoryTemplate g_Templates[]=
{	{ sudOggSplitName
	, &CLSID_OggSplitter 
	, COggSplitter::CreateInstance
	, NULL
	, &sudOggSplit
	}
,	{ sudOggSplitPropPageName
	, &CLSID_OggSplitPropPage
	, COggSplitPropPage::CreateInstance
	, NULL
	, NULL
	}
,	{ sudOggMuxName
	, &CLSID_OggMux 
	, COggMux::CreateInstance
	, NULL
	, &sudOggMux
	}
,	{ sudOggMuxPropPageName
	, &CLSID_OggMuxPropPage
	, COggMuxPropPage::CreateInstance
	, NULL
	, NULL
	}
,   { sudVorbisEncName
    , &CLSID_VorbisEnc
    , CVorbisEnc::CreateInstance
    , NULL
    , &sudVorbisEnc
	}
,	{ sudVorbisEncPropPageName
	, &CLSID_VorbisEncPropPage
	, CVorbisEncPropPage::CreateInstance
	, NULL
	, NULL
	}
,   { sudVorbisDecName
    , &CLSID_VorbisDec
    , CVorbisDec::CreateInstance
    , NULL
    , &sudVorbisDec
	}
,	{ sudVorbisDecPropPageName
	, &CLSID_VorbisDecPropPage
	, CVorbisDecPropPage::CreateInstance
	, NULL
	, NULL
	}
,	{ sudOggDSAboutPageName
	, &CLSID_OggDSAboutPage
	, COggDSAboutPage::CreateInstance
	, NULL
	, NULL
	}
};
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);


STDAPI DllRegisterServer()
{
    HRESULT hr;

    if FAILED(hr = AMovieDllRegisterServer2(TRUE)) return hr;

	// Register Vorbis Compresor as audio compressor
    IFilterMapper2 *pFM2 = NULL;

    if FAILED(hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
            IID_IFilterMapper2, (void **)&pFM2)) return hr;
    if FAILED(hr = pFM2->RegisterFilter(CLSID_VorbisEnc, sudVorbisEncName, NULL,
							  &CLSID_AudioCompressorCategory, sudVorbisEncName, &rf2FilterReg)) return hr;
    pFM2->Release();

    // Register Stream - Ogg filetype ...
	// This will enable the File Source Filter to recognize that
	// Ogg files contain media type Stream - Ogg
	HKEY	hkey = NULL;
	LONG	lreturn = ERROR_SUCCESS;
	CHAR	chMediaEntry[260];
	CHAR	chPattern[260];
	CHAR	chSourceFilter[CHARS_IN_GUID];
	CHAR	chMediaType[CHARS_IN_GUID];
	CHAR	chMediaSubtype[CHARS_IN_GUID];

	OLECHAR	szMediaType[CHARS_IN_GUID];
	OLECHAR	szMediaSubtype[CHARS_IN_GUID];
	OLECHAR	szAsyncReader[CHARS_IN_GUID];
	OLECHAR szCLSID[CHARS_IN_GUID];

	do
	{
		StringFromGUID2(MEDIATYPE_Stream,  szMediaType, CHARS_IN_GUID);
		StringFromGUID2(MEDIASUBTYPE_Ogg,  szMediaSubtype, CHARS_IN_GUID);
		StringFromGUID2(CLSID_AsyncReader, szAsyncReader, CHARS_IN_GUID);
		wsprintf(chPattern, "0,4,,%x", ('O'<<24) + ('g'<<16) + ('g'<<8) + 'S');
		wsprintf(chSourceFilter, "%ls", szAsyncReader);
		wsprintf(chMediaType, "%ls", szMediaType);
		wsprintf(chMediaSubtype, "%ls", szMediaSubtype);

		// create the mediatype key
		wsprintf(chMediaEntry, "Media Type\\%ls\\%ls", szMediaType, szMediaSubtype);
		lreturn = RegCreateKey( HKEY_CLASSES_ROOT, chMediaEntry, &hkey); 
		if (lreturn != ERROR_SUCCESS) continue;

		// Tell the file source filter how to recognize Ogg Streams
		// Ogg files start with "OggS" ...
		lreturn = RegSetValueEx(hkey, "0", 0, REG_SZ, (CONST BYTE*)chPattern, strlen(chPattern));
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, "Source Filter", 0, REG_SZ, (CONST BYTE*)chSourceFilter, strlen(chSourceFilter));
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		// create the extension entry for ogg
		lreturn = RegCreateKey( HKEY_CLASSES_ROOT, "Media Type\\Extensions\\.ogg", &hkey);
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, "Source Filter", 0, REG_SZ, (CONST BYTE*)chSourceFilter, strlen(chSourceFilter));
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, "Media Type", 0, REG_SZ, (CONST BYTE*)chMediaType, strlen(chMediaType));
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, "Subtype", 0, REG_SZ, (CONST BYTE*)chMediaSubtype, strlen(chMediaSubtype));
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		// create the entry for .ogm
		lreturn = RegCreateKey( HKEY_CLASSES_ROOT, "Media Type\\Extensions\\.ogm", &hkey);
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, "Source Filter", 0, REG_SZ, (CONST BYTE*)chSourceFilter, strlen(chSourceFilter));
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, "Media Type", 0, REG_SZ, (CONST BYTE*)chMediaType, strlen(chSourceFilter));
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, "Subtype", 0, REG_SZ, (CONST BYTE*)chMediaSubtype, strlen(chMediaSubtype));
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		// The following registry entries enable the preview in the windows explorer
		lreturn = RegCreateKey(HKEY_CLASSES_ROOT, ".ogm", &hkey);
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, NULL, 0, REG_SZ, (CONST BYTE*)OgmFileDesc, strlen(OgmFileDesc));
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		lreturn = RegCreateKey( HKEY_CLASSES_ROOT, ".ogm\\ShellEx", &hkey);
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		StringFromGUID2(IID_IExtractImage,  szCLSID, CHARS_IN_GUID);
		wsprintf(chMediaEntry, ".ogm\\ShellEx\\%S", szCLSID);
		lreturn = RegCreateKey( HKEY_CLASSES_ROOT, chMediaEntry, &hkey);
		if (lreturn != ERROR_SUCCESS) continue;
		StringFromGUID2(IID_IExtractImage,  szCLSID, CHARS_IN_GUID);
		wsprintf(chMediaEntry, "{c5a40261-cd64-4ccf-84cb-c394da41d590}");
		lreturn = RegSetValueEx(hkey, NULL, 0, REG_SZ, (CONST BYTE*)chMediaEntry, strlen(chMediaEntry));
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		// The following registry entries enable the preview in the windows explorer
		lreturn = RegCreateKey(HKEY_CLASSES_ROOT, ".ogg", &hkey);
		if (lreturn != ERROR_SUCCESS) continue;
		lreturn = RegSetValueEx(hkey, NULL, 0, REG_SZ, (CONST BYTE*)OgmFileDesc, strlen(OgmFileDesc));
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		lreturn = RegCreateKey( HKEY_CLASSES_ROOT, ".ogg\\ShellEx", &hkey);
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		StringFromGUID2(IID_IExtractImage,  szCLSID, CHARS_IN_GUID);
		wsprintf(chMediaEntry, ".ogg\\ShellEx\\%S", szCLSID);
		lreturn = RegCreateKey( HKEY_CLASSES_ROOT, chMediaEntry, &hkey);
		if (lreturn != ERROR_SUCCESS) continue;
		StringFromGUID2(IID_IExtractImage,  szCLSID, CHARS_IN_GUID);
		wsprintf(chMediaEntry, "{c5a40261-cd64-4ccf-84cb-c394da41d590}");
		lreturn = RegSetValueEx(hkey, NULL, 0, REG_SZ, (CONST BYTE*)chMediaEntry, strlen(chMediaEntry));
		if (lreturn != ERROR_SUCCESS) continue;
		RegCloseKey(hkey);
		hkey = NULL;

		// Register .ogg and .ogm for Media Player
		lreturn = RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\Types", &hkey);
		if (lreturn != ERROR_SUCCESS)
		{
			bool	bOggAlreadyRegistered = false;
			DWORD	index = 0;
			char	pbName[256];
			DWORD	cbName = 256;
			char	pbValue[256];
			DWORD	cbValue = 256;
			
			while(RegEnumValue(hkey, index, pbName, &cbName, NULL, NULL, (BYTE*)pbValue, &cbValue) == ERROR_SUCCESS)
			{
				pbValue[cbValue] = '\0';
				pbName[cbValue] = '\0';
				index++;
				if (strstr(pbValue, ".ogg")) bOggAlreadyRegistered = true;
				cbName = 256;
				cbValue = 256;
			}

			if (!bOggAlreadyRegistered)
			{
				index = atoi(pbName);
				index++;
				wsprintf(pbName, "%d", index);
				RegSetValueEx(hkey, pbName, 0, REG_SZ, (CONST BYTE*)OggFileTypes, strlen(OggFileTypes));
				RegCloseKey(hkey);
				RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\Descriptions", &hkey);
				RegSetValueEx(hkey, pbName, 0, REG_SZ, (CONST BYTE*)OggFileDesc, strlen(OggFileDesc));
				RegCloseKey(hkey);
				RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\MUIDescriptions", &hkey);
				RegSetValueEx(hkey, pbName, 0, REG_SZ, (CONST BYTE*)OggFileDesc2, strlen(OggFileDesc2));
			}
			RegCloseKey(hkey);
			hkey = NULL;		
		}

		// This for MediaPlayer 9
		lreturn = RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Multimedia\\WMPlayer\\Extensions", &hkey);
		if (lreturn == ERROR_SUCCESS)
		{
			HKEY	hKeyNew ;
			DWORD	dwData = 7;

			RegCreateKey(hkey, ".ogg", &hKeyNew);
			RegSetValueEx(hKeyNew, "Runtime", 0, REG_DWORD, (BYTE*)&dwData, 4);
			RegCloseKey(hKeyNew);
			RegCreateKey(hkey, ".ogm", &hKeyNew);
			RegSetValueEx(hKeyNew, "Runtime", 0, REG_DWORD, (BYTE*)&dwData, 4);
			RegCloseKey(hKeyNew);
			RegCloseKey(hkey);
			hkey = NULL;
		}
	} while( 0 );

	if (hkey)
		RegCloseKey(hkey);

    if (lreturn != ERROR_SUCCESS)
		return AmHresultFromWin32(lreturn);

	return NOERROR;
}


STDAPI DllUnregisterServer()
{
    HRESULT hr;

    if FAILED(hr = AMovieDllRegisterServer2(FALSE)) return hr;
 
	// Deregister Vorbis Compressor as audio compressor
    IFilterMapper2 *pFM2 = NULL;

    if FAILED(hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
							IID_IFilterMapper2, (void **)&pFM2)) return hr;
    if FAILED(hr = pFM2->UnregisterFilter(&CLSID_AudioCompressorCategory, 
								sudVorbisEncName, CLSID_VorbisEnc)) return hr;
    pFM2->Release();

	// Remove registered file type ...
	CHAR	chMediaEntry[260];
	OLECHAR	szMediaType[CHARS_IN_GUID];
	OLECHAR	szMediaSubtype[CHARS_IN_GUID];
	
	StringFromGUID2(MEDIATYPE_Stream,  szMediaType, CHARS_IN_GUID);
	StringFromGUID2(MEDIASUBTYPE_Ogg,  szMediaSubtype, CHARS_IN_GUID);
	wsprintf(chMediaEntry, "Media Type\\%ls\\%ls", szMediaType, szMediaSubtype);
	EliminateSubKey(HKEY_CLASSES_ROOT, chMediaEntry);
	EliminateSubKey(HKEY_CLASSES_ROOT, "Media Type\\Extensions\\.ogg");
	EliminateSubKey(HKEY_CLASSES_ROOT, "Media Type\\Extensions\\.ogm");
	EliminateSubKey(HKEY_CLASSES_ROOT, ".ogm");
    return hr;
}

// Necessary to for static link to LIBCMT
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

extern BOOL WINAPI DllMain(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        )
{
	return DllEntryPoint((HINSTANCE)hDllHandle, dwReason, lpreserved);
}
