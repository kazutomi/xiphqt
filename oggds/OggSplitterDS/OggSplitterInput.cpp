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

//
// COggSplitInputPin methods ...
//
COggSplitInputPin::COggSplitInputPin(TCHAR *pObjectName, COggSplitter *pFilter, CCritSec *pLock, HRESULT *phr, LPCWSTR pName):
	CBaseInputPin(pObjectName, pFilter, pLock, phr, pName),
	m_evWaitHighWatermark(FALSE),
	m_pReader(NULL),
	m_pOggSplitter(pFilter)
{
	ogg_sync_init(&m_oy);
};

COggSplitInputPin::~COggSplitInputPin()
{
	ogg_sync_clear(&m_oy);
}

// Only Ogg streams, please
HRESULT COggSplitInputPin::CheckMediaType(const CMediaType* pmt)
{
	if (*(pmt->Type())    != MEDIATYPE_Stream ||
		*(pmt->Subtype()) != MEDIASUBTYPE_Ogg) return VFW_E_TYPE_NOT_ACCEPTED;
	return NOERROR;
}

// Release the IASyncReader Interface and
// remove all output pins
HRESULT COggSplitInputPin::BreakConnect()
{
	HRESULT	hr;
	hr = CBaseInputPin::BreakConnect();
	if FAILED(hr) return hr;

	if (m_pReader)
	{
		m_pReader->Release();
		m_pReader = NULL;
	}
	m_pOggSplitter->DeleteStreams();
	m_pOggSplitter->DeleteOutputPins();
	return NOERROR;
}

// Check if the other pin has IAsyncReader Interface
HRESULT COggSplitInputPin::CheckConnect(IPin* pPin)
{
	HRESULT	hr;
	hr = CBaseInputPin::CheckConnect(pPin);
	if FAILED(hr) return hr;

	hr = pPin->QueryInterface(IID_IAsyncReader, (void**)&m_pReader);
	if FAILED(hr) return S_FALSE;
	return NOERROR;
}

// Identify the streams ....
HRESULT COggSplitInputPin::CompleteConnect(IPin *pReceivePin)
{
	if (!m_pReader) return E_UNEXPECTED;

	HRESULT		hr = NOERROR;
	LONGLONG	avail;
	LONGLONG	pos;
	long		len;
	char*		buffer;
	CAutoLock	lock(m_pLock);
	ogg_page	og;

	m_pReader->Length(&m_iStreamLen, &avail);
	len = (long) __min(65536, m_iStreamLen);

	ogg_sync_reset(&m_oy);
	buffer = ogg_sync_buffer(&m_oy, len);
	if (m_pReader->SyncRead(0, len, (BYTE*)buffer) != S_OK)	return VFW_E_TYPE_NOT_ACCEPTED;
	ogg_sync_wrote(&m_oy, len);

	// Analyze all pages in the first 64KB
	while (ogg_sync_pageout(&m_oy, &og) > 0)
	{
		int iStreamID = ogg_page_serialno(&og);
		COggStream *pStream = m_pOggSplitter->FindStreamByID(iStreamID);
		// If new stream create a stream object ...
		if (pStream == NULL)
		{
			hr = m_pOggSplitter->AddStream(&pStream, iStreamID, false);
			if FAILED(hr) return hr;
		} 
		pStream->IdentifyType(&og);
	}

	// Classify the streams and set GroupIDs
	m_pOggSplitter->CreateChapterList();
	m_pOggSplitter->SetGroupID();
	m_pOggSplitter->CreateOutputPins();
	
	// Get the duration ...
	REFERENCE_TIME		rtDuration = 0;

	if (m_pOggSplitter->GetStreamCount() > 0)
	{
		pos = (m_iStreamLen - 65536) & 0xfffffffffffffff0; if (pos < 0) pos = 0;
		len	= (long) (m_iStreamLen - pos);

		ogg_sync_reset(&m_oy);
		buffer = ogg_sync_buffer(&m_oy, len);
		hr = m_pReader->SyncRead(pos, len, (BYTE*)buffer) ;
		ogg_sync_wrote(&m_oy, len);

		while (ogg_sync_pageout(&m_oy, &og) != 0)
		{
			COggStream *pStream = m_pOggSplitter->FindStreamByID(ogg_page_serialno(&og));
			if (pStream)
			{
				REFERENCE_TIME rtCurPos = pStream->MediaTimeToRefTime(ogg_page_granulepos(&og));
				if (rtCurPos > rtDuration)	rtDuration	  = rtCurPos;
			}
		}
	}

	m_iStreamPos = 0;
	m_pOggSplitter->SetDuration(rtDuration);
	return NOERROR;
};


DWORD COggSplitInputPin::ThreadProc(void)
{
    Command com;

	do
	{
		com = (Command)GetRequest();
		switch (com)
		{
		case CMD_EXIT:
		    Reply(NOERROR);
			break;

		case CMD_STOP:
		    Reply(NOERROR);
		    break;

		case CMD_RUN:
			DoBufferProcessingLoop();
			break;
		}
	} while (com != CMD_EXIT);

	return NOERROR;
}

//
HRESULT COggSplitInputPin::DoBufferProcessingLoop(void)
{
    Command		com;
	bool		bHighWatermarkDetected = false;
	bool		bWasWaitingLowWatermark = false;
	bool		bFirstPageOut = true;
	bool		bHighWatermarkReached = false;

	ogg_page	og;

	m_Abort = FALSE;
	m_evWaitHighWatermark.Reset();
	ogg_sync_reset(&m_oy);
	Reply(NOERROR);

	do
	{
		while (ogg_sync_pageout(&m_oy, &og) <= 0)
		{
			if (bFirstPageOut) bFirstPageOut = false;
			else  // Read from file ...
			{
				if (m_iStreamPos >= m_iStreamLen) // End of file ?
				{
					m_pOggSplitter->NotifyEOF();
					return NOERROR; // we are done
				}
				// We don't want memory fragmentation therefore we keep
				// the OggSync at BUFFERSIZE (OggSync only reallocates
				// if we exceed the buffer used before)
				long len = BUFFERSIZE + m_oy.returned - m_oy.fill;
				if (m_iStreamPos + len > m_iStreamLen)
					len = (long) (m_iStreamLen - m_iStreamPos);
				BYTE* buffer = (BYTE*) ogg_sync_buffer(&m_oy, len);

				m_pReader->SyncRead(m_iStreamPos, len, buffer);
				ogg_sync_wrote(&m_oy, len);
				m_iStreamPos += len;
			}
		}

		// Now we have a page
		// Find the corresponding stream
		// There should not be any stream without pin but to be on the safe side ..
		COggStream *pStream = m_pOggSplitter->FindStreamByID(ogg_page_serialno(&og));

		if (pStream)
			bHighWatermarkReached = pStream->DeliverPage(&og);
		else bHighWatermarkReached = false;

		if (bHighWatermarkReached)
			m_evWaitHighWatermark.Wait(INFINITE);
		else
			m_evWaitHighWatermark.Reset();

	} while (!CheckRequest((DWORD*)&com) && !m_Abort);
    return NOERROR;
}


// The pin is active - start up the worker thread
HRESULT COggSplitInputPin::Active()
{
    HRESULT	hr;
	
	if (m_pOggSplitter->IsActive()) return S_FALSE;
    if (!IsConnected()) return NOERROR;
    hr = CBaseInputPin::Active();
	if FAILED(hr) return hr;
	
	if (!Create()) return E_FAIL;
	CallWorker(CMD_RUN);

	return NOERROR;
}
											
HRESULT COggSplitInputPin::Inactive()
{
	// Stop the input pin and exit the thread
    if (ThreadExists())
	{
		m_Abort = TRUE;
		m_evWaitHighWatermark.Set();	 // Thread may be waiting
		CallWorker(CMD_EXIT);
		Close();						// Wait for the thread to exit, then tidy up.
    } 

	m_iStreamPos = 0;

    return 	CBasePin::Inactive();
}

HRESULT COggSplitInputPin::BeginFlush()
{
	// Stop the input pin ...
    if (!ThreadExists()) return NOERROR;

	m_Abort = TRUE;
	m_evWaitHighWatermark.Set();	 // Thread may be waiting
	CallWorker(CMD_STOP);

    return 	NOERROR;
}

HRESULT	COggSplitInputPin::EndFlush()
{
	if (ThreadExists()) CallWorker(CMD_RUN);

	return NOERROR;
}
