#define INITGUID

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "pntypes.h"
#include "pncom.h"
#include "rmacomm.h"
#include "rmapckts.h"
#include "rmaplugn.h"
#include "rmafiles.h"
#include "rmaformt.h"

#include "fvorbis.h"

// static variables
const char *CVorbisFileFormat::zm_pDescription = DESCRIPTION;
const char *CVorbisFileFormat::zm_pCopyright = COPYRIGHT;
const char *CVorbisFileFormat::zm_pMoreInfoURL = MORE_INFO_URL;
const char *CVorbisFileFormat::zm_pFileMimeTypes[] = FILE_MIME_TYPES;
const char *CVorbisFileFormat::zm_pFileExtensions[] = FILE_EXTENSIONS;
const char *CVorbisFileFormat::zm_pFileOpenNames[] = FILE_OPEN_NAMES;

int CVorbisFileFormat::instno = 0;

STDAPI RMACreateInstance(IUnknown **ppExFileFormatObj)
{
	//printf("RMACreateInstance() called\n");

	*ppExFileFormatObj = (IUnknown *)(IRMAPlugin *)new CVorbisFileFormat();
	if (*ppExFileFormatObj != NULL) {
		(*ppExFileFormatObj)->AddRef();
		return PNR_OK;	
	}

	return PNR_OUTOFMEMORY;
}

// constructor
CVorbisFileFormat::CVorbisFileFormat(void)
	:	m_RefCount(0),
		m_pClassFactory(NULL),
		m_pFileObj(NULL),
		m_pStatus(NULL),
		m_State(Ready),
		m_NextPacketDeliveryTime(0),
	        m_ogg_inited(0)
{
	//printf("Entering CVorbisFileFormat()\n");
	m_instno = instno++;
	//printf("Exiting CVorbisFileFormat(%d)\n", m_instno);
}

STDMETHODIMP CVorbisFileFormat::GetPluginInfo(
	REF(BOOL) bLoadMultiple,
	REF(const char *) pDescription,
	REF(const char *) pCopyright,
	REF(const char *) pMoreInfoURL,
	REF(UINT32) versionNumber
)
{
	//printf("Entering GetPluginInfo(%d)\n", m_instno);

	// File Format plugins must be able to be multiply instantiated
	bLoadMultiple = TRUE;

	pDescription = zm_pDescription;
	pCopyright = zm_pCopyright;
	pMoreInfoURL = zm_pMoreInfoURL;
	versionNumber = PLUGIN_VERSION;

	//printf("Exiting GetPluginInfo(%d)\n", m_instno);

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::GetFileFormatInfo(
	REF(const char **) pFileMimeTypes,
	REF(const char **) pFileExtensions,
	REF(const char **) pFileOpenNames
)
{
	//printf("Entering GetFileFormatInfo(%d)\n", m_instno);

	pFileMimeTypes = zm_pFileMimeTypes;
	pFileExtensions = zm_pFileExtensions;
	pFileOpenNames = zm_pFileOpenNames;

	//printf("Exiting GetFileFormatInfo(%d)\n", m_instno);

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::InitPlugin(IUnknown *pRMACore)
{
	//printf("Entering InitPlugin(%d)\n", m_instno);
	
	// store a reference to the IRMACommonClassFactory interface
	pRMACore->QueryInterface(IID_IRMACommonClassFactory, (void **)&m_pClassFactory);
	if (m_pClassFactory == NULL) return PNR_NOTIMPL;

	// queryinterface adds a reference for us, we are responsible
	// for release the reference when we're done

	//printf("Exiting InitPlugin(%d)\n", m_instno);

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::InitFileFormat(
	IRMARequest *pRequest,
	IRMAFormatResponse *pFormatResponse,
	IRMAFileObject *pFileObject
)
{
	//printf("Entering InitFileFormat(%d)\n", m_instno);

	// the format response object is used to notify rma core of our status
	m_pStatus = pFormatResponse;
	if (m_pStatus != NULL)
		m_pStatus->AddRef();

	// the file object is used to handle file i/o
	m_pFileObj = pFileObject;
	if (m_pFileObj != NULL) {
		m_pFileObj->AddRef();

		// init and check file, then associate file with response object
		m_State = InitPending;
		m_pFileObj->Init(PN_FILE_READ | PN_FILE_BINARY, this);
	} 


	ogg_sync_init(&m_oy);
	vorbis_info_init(&m_initial_i);
	vorbis_comment_init(&m_initial_c);
	m_granulepos = 0;
	m_ogg_inited = 1;

	//printf("Exiting InitFileFormat(%d)\n", m_instno);

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::InitDone(PN_RESULT status)
{
	//printf("Entering InitDone(%d)\n", m_instno);

	if (m_State != InitPending) return PNR_UNEXPECTED;

	// notify RMA core that init started in InitFileFormat is done
	m_State = Ready;
	m_pStatus->InitDone(status);

	//printf("Exiting InitDone(%d)\n", m_instno);

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::GetFileHeader(void)
{
	IRMAFileStat *pFileStatObj;

	//printf("Entering GetFileHeader(%d)\n", m_instno);

	if (m_State != Ready) return PNR_UNEXPECTED;

	// async stuff makes this muddy, but here's the steps
	// 1) stat the file to get the length
	// 2) in StatDone, 

	m_State = GetFileHeaderStatPending;

	m_pFileObj->QueryInterface(IID_IRMAFileStat, (void **)&pFileStatObj);
	if (pFileStatObj != NULL) {
		pFileStatObj->Stat(this);
		pFileStatObj->Release();
	}

	//printf("Exiting GetFileHeader(%d)\n", m_instno);

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::StatDone(PN_RESULT status, UINT32 ulSize, UINT32 ulCreationTime, UINT32 ulAccessTime,
					 UINT32 ulModificationTime, UINT32 ulMode)
{
	//printf("Entering StatDone(%d)\n", m_instno);

	if (m_State != GetFileHeaderStatPending) return PNR_UNEXPECTED;

	m_State = GFHInitialHeaderReadPending;

	m_FileSize = ulSize;

	//printf("Filesize of this ogg file is %ld\n", m_FileSize);

	m_pFileObj->Read(READ_LENGTH);

	//printf("Exiting StatDone(%d)\n", m_instno);

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::ReadDone(PN_RESULT status, IRMABuffer *pBuffer)
{
	char *buffer;
	ogg_page og;
	ogg_packet op;
	int result;
	int falling_through = 0;

	//printf("Enterint ReadDone(%d)\n", m_instno);
	
	switch (m_State) {
	case GFHInitialHeaderReadPending:
		//printf("state = GFHInitialHeadReadPending\n");
		if (pBuffer == NULL) {
			m_State = Ready;
			return PNR_INVALID_FILE;
		}

		// submit what we read to libvorbis
		buffer = ogg_sync_buffer(&m_oy, pBuffer->GetSize());
		memcpy(buffer, pBuffer->GetBuffer(), pBuffer->GetSize());
		ogg_sync_wrote(&m_oy, pBuffer->GetSize());

		//printf("GFHIHRP: submitted %d bytes to ogg\n", pBuffer->GetSize());

		// if we didn't read enough, read some more
		if (ogg_sync_pageout(&m_oy, &og) != 1) {
			m_pFileObj->Read(READ_LENGTH);
			return PNR_OK;
		}
		
		m_initial_serialno = ogg_page_serialno(&og);
		ogg_stream_init(&m_os, ogg_page_serialno(&og));

		//printf("serial number is %ld\n", m_initial_serialno);

		if (ogg_stream_pagein(&m_os, &og) < 0) {
			//printf("GFHIHRP: ogg_stream_pagein failed...\n");
			// error
			m_State = Ready;
			return PNR_INVALID_FILE;
		}

		if (ogg_stream_packetout(&m_os, &op) != 1) {
			//printf("couldn't read initial packet\n");
			// must not be vorbis
			return PNR_INVALID_FILE;
		}

		if (vorbis_synthesis_headerin(&m_initial_i, &m_initial_c, &op) < 0) {
			//printf("GFHIHRP: vorbis_synthesis headerin failed\n");
			// error: not a vorbis header
			m_State = Ready;
			return PNR_INVALID_FILE;
		}

		// we're sure we're vorbis.  need to read the comments and the codebooks
		m_State = GFHInitialHeaderCommentReadPending;
		
		falling_through = 1;
		// pass through intentional
	case GFHInitialHeaderCommentReadPending:
		//printf("state = GFHInitialHeaderCommentReadPending\n");
		if (!falling_through) {
			if (pBuffer == NULL) {
				m_State = Ready;
				return PNR_INVALID_FILE;
			}

			// submit what we read to libvorbis
			buffer = ogg_sync_buffer(&m_oy, pBuffer->GetSize());
			memcpy(buffer, pBuffer->GetBuffer(), pBuffer->GetSize());
			ogg_sync_wrote(&m_oy, pBuffer->GetSize());			
		}

		if (ogg_sync_pageout(&m_oy, &og) != 1) {
			// not enough data
			m_pFileObj->Read(READ_LENGTH);
			return PNR_OK;
		}
		
		//printf("GFHIHCRP: we got a page...\n");

		ogg_stream_pagein(&m_os, &og); // we can ignore these errors, they'll show up later
		
		result = ogg_stream_packetout(&m_os, &op);
		// do we need more data
		if (result == 0) {
			m_pFileObj->Read(READ_LENGTH);
			return PNR_OK;
		}

		// was there an error
		if (result < 0) {
			m_State = Ready;
			return PNR_INVALID_FILE;
		}

		vorbis_synthesis_headerin(&m_initial_i, &m_initial_c, &op);
		
		// got the comments 
		m_State = GFHInitialHeaderCodebookReadPending;
		
		falling_through = 1;
		// fall through intentional
	case GFHInitialHeaderCodebookReadPending:
		//printf("state = GFHInitialHeaderCodebookReadPending\n");

		if (!falling_through) {
			if (pBuffer == NULL) {
				m_State = Ready;
				return PNR_INVALID_FILE;
			}

			// submit what we read to libvorbis
			buffer = ogg_sync_buffer(&m_oy, pBuffer->GetSize());
			memcpy(buffer, pBuffer->GetBuffer(), pBuffer->GetSize());
			ogg_sync_wrote(&m_oy, pBuffer->GetSize());
		}

		if (ogg_sync_pageout(&m_oy, &og) != 1) {
			// not enough data
			m_pFileObj->Read(READ_LENGTH);
			return PNR_OK;
		}

		ogg_stream_pagein(&m_os, &og);

		result = ogg_stream_packetout(&m_os, &op);
		// do we need more data?
		if (result == 0) {
			m_pFileObj->Read(READ_LENGTH);
			return PNR_OK;
		}

		// was there an error?
		if (result < 0) {
			m_State = Ready;
			return PNR_INVALID_FILE;
		}
		
		vorbis_synthesis_headerin(&m_initial_i, &m_initial_c, &op);

		// we know it's vorbis.  that's good enough. reset the stream state.
		ogg_stream_clear(&m_os);

		// and the sync state
		ogg_sync_reset(&m_oy);

		// now we need to seek to the end and see if we're the only logical stream
		m_State = GFHSeekEndPending;

		m_offset = m_FileSize;
		m_begin = m_FileSize;
		m_pFileObj->Seek(m_FileSize, FALSE);
		
		return PNR_OK;
	case GFHReadEndSearching:
		//printf("state = GFHReadEndSearching\n");
		// we're looking for a page

		// if we couldn't read anymore data, seek back some more.
		if (pBuffer == NULL) {
			m_State = GFHSeekEndSearching;
			
			m_begin -= READ_LENGTH;
			m_pFileObj->Seek(m_begin, FALSE);

			return PNR_OK;
		}

		// add the new data
		buffer = ogg_sync_buffer(&m_oy, pBuffer->GetSize());
		memcpy(buffer, pBuffer->GetBuffer(), pBuffer->GetSize());
		ogg_sync_wrote(&m_oy, pBuffer->GetSize());

		if (ogg_sync_pageout(&m_oy, &og) != 1) {
			// more data please
			m_pFileObj->Read(READ_LENGTH);

			return PNR_OK;
		}

		// yay we found a page

		// is there more than one logical bitstream?
		if (ogg_page_serialno(&og) != m_initial_serialno) {
			// this is a chained bitstream
			// this is NOT currently implemented.
			m_State = Ready;
			return PNR_INVALID_FILE;
		} else {
			// now we can get the length of the song
			m_FileTime = (UINT32)((float)(ogg_page_granulepos(&og) / m_initial_i.rate) * 1000.0);

			m_State = GFHSeekBeginPending;
			m_pFileObj->Seek(0, FALSE);

			return PNR_OK;
		}

		break;
	case GetPacketReadPending:
		//printf("state = GetPacketReadPending\n");
		if (pBuffer == NULL) {
			m_State = Ready;
			return PNR_UNEXPECTED;
		}
		
		// send the data to the ogg layer
		buffer = ogg_sync_buffer(&m_oy, pBuffer->GetSize());
		memcpy(buffer, pBuffer->GetBuffer(), pBuffer->GetSize());
		ogg_sync_wrote(&m_oy, pBuffer->GetSize());

		// do we have a page yet?
		if (ogg_sync_pageout(&m_oy, &og) != 1) {
			// get more data
			m_pFileObj->Read(READ_LENGTH);
			return PNR_OK;
		}


		CreatePacketObj(&og, m_granulepos);
		m_granulepos = ogg_page_granulepos(&og);

		return PNR_OK;
	default:
		return PNR_UNEXPECTED;
	}
	
	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::SeekDone(PN_RESULT status)
{
	//printf("Entering SeekDone(%d)\n", m_instno);

	switch (m_State) {
	case GFHSeekEndPending:
		//printf("state = GFHSeekEndPending\n");
		// now we need to read the last page in this file
		
		// start searching
		m_State = GFHSeekEndSearching;

		m_begin -= READ_LENGTH;
		m_pFileObj->Seek(m_begin, FALSE);

		return PNR_OK;
	case GFHSeekEndSearching:
		//printf("state = GFHSeekEndSearching\n");
		// find a page
		m_State = GFHReadEndSearching;

		m_pFileObj->Read(READ_LENGTH);
		
		return PNR_OK;
	case GFHSeekBeginPending:
		//printf("state = GFHSeekBeginPending\n");
		m_State = Ready;

		// let's reset the stream/sync states
		ogg_sync_reset(&m_oy);

		CreateFileHeaderObj();

		return PNR_OK;
	default:
		return PNR_UNEXPECTED;
	}

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::GetStreamHeader(UINT16 streamNo)
{
	//printf("Entering GetStreamHeader(%d)\n", m_instno);

	if (m_State != Ready) return PNR_UNEXPECTED;

	CreateStreamHeaderObj();

	//printf("Exiting GetStreamHeader(%d)\n", m_instno);

	return PNR_OK;
}

void CVorbisFileFormat::CreateFileHeaderObj(void)
{
	//printf("Entering CreateFileHeaderObj(%d)\n", m_instno);
	m_State = Ready;

	// Step 5: create new object containing header data
	IRMAValues *pHeaderObj = NULL;
	m_pClassFactory->CreateInstance(CLSID_IRMAValues, (void **)&pHeaderObj);
	
	if (pHeaderObj != NULL) {
		// required property: StreamCount
		pHeaderObj->SetPropertyULONG32("StreamCount", 1);

		// do the vorbis comments
		IRMABuffer *pTitle = NULL;
		IRMABuffer *pAuthor = NULL;
		IRMABuffer *pCopyright = NULL;
		char *pTitleData;
		char *pAuthorData;
		char *pCopyrightData;

		int i;
		printf("Looking for vorbis comments.. We have %d to choose from...\n", m_initial_c.comments);
		for (i = 0; i < m_initial_c.comments; i++) {
			printf("Comment %d: %s...\n", i, m_initial_c.user_comments[i]);
			if (!strncasecmp("title", m_initial_c.user_comments[i], 5)) {
				char *pos = strchr(m_initial_c.user_comments[i], '=');
				if (pos == NULL) continue;
				pos++;

				printf("adding title comment of %s\n", pos);

				m_pClassFactory->CreateInstance(CLSID_IRMABuffer, (void **)&pTitle);
				if (pTitle == NULL) continue;
				
				pTitle->SetSize(strlen(pos) + 1);
				pTitleData = (char *)pTitle->GetBuffer();
				strcpy(pTitleData, pos);

				pHeaderObj->SetPropertyBuffer("Title", pTitle);
			} else if (!strncasecmp("artist", m_initial_c.user_comments[i], 6)) {
				char *pos = strchr(m_initial_c.user_comments[i], '=');
				if (pos == NULL) continue;
				pos++;

				printf("adding author comment of %s\n", pos);

				m_pClassFactory->CreateInstance(CLSID_IRMABuffer, (void **)&pAuthor);
				if (pAuthor == NULL) continue;

				pAuthor->SetSize(strlen(pos) + 1);
				pAuthorData = (char *)pAuthor->GetBuffer();
				strcpy(pAuthorData, pos);

				pHeaderObj->SetPropertyBuffer("Author", pAuthor);
			} else if (!strncasecmp("copyright", m_initial_c.user_comments[i], 9)) {
				char *pos = strchr(m_initial_c.user_comments[i], '=');
				if (pos == NULL) continue;
				pos++;
				
				printf("adding copyright comment of %s\n", pos);

				m_pClassFactory->CreateInstance(CLSID_IRMABuffer, (void **)&pCopyright);
				if (pCopyright == NULL) continue;

				pCopyright->SetSize(strlen(pos) + 1);
				pCopyrightData = (char *)pCopyright->GetBuffer();
				strcpy(pCopyrightData, pos);

				pHeaderObj->SetPropertyBuffer("Copyright", pCopyright);
			}
		}


		// step 6: notify rma core that header object is ready
		m_pStatus->FileHeaderReady(PN_STATUS_OK, pHeaderObj);

		// release object since we're done with it
		pHeaderObj->Release();
		if (pTitle) pTitle->Release();
		if (pAuthor) pAuthor->Release();
		if (pCopyright) pCopyright->Release();
	}

	//printf("Exiting CreateFileHeaderObj(%d)\n", m_instno);
}

void CVorbisFileFormat::CreateStreamHeaderObj(void)
{
	UINT32 avgBitRate;

	//printf("Entering CreateStreamHeaderObj(%d)\n", m_instno);

	m_State = Ready;

	IRMAValues *pStreamHeaderObj = NULL;
	m_pClassFactory->CreateInstance(CLSID_IRMAValues, (void **)&pStreamHeaderObj);

	if (pStreamHeaderObj != NULL) {
		// these properties are required

		// "StreamNumber" the stream number
		pStreamHeaderObj->SetPropertyULONG32("StreamNumber", STREAM_NO);

		// "AvgBitRate" the average bitrate of this stream
		avgBitRate = (UINT32)((float)((m_FileSize * 8) / (m_FileTime / 1000)));
		pStreamHeaderObj->SetPropertyULONG32("AvgBitRate", avgBitRate);
		//printf("AvgBitRate = %ld\n", avgBitRate);

		// preroll might not be required
		// "Preroll" the amount to prebuffer
		//pStreamHeaderObj->SetPropertyULONG32("Preroll", 0);

		// "Duration" length in milliseconds
		pStreamHeaderObj->SetPropertyULONG32("Duration", m_FileTime);
		//printf("Duration = %ld\n", m_FileTime);

		// "MimeType" the mime-type
		IRMABuffer *pStringObj = NULL;
		m_pClassFactory->CreateInstance(CLSID_IRMABuffer, (void **)&pStringObj);
		if (pStringObj != NULL) {
			char *streamMimeType = STREAM_MIME_TYPE;
			pStringObj->Set((const BYTE *)streamMimeType, strlen(streamMimeType) + 1);
			pStreamHeaderObj->SetPropertyCString("MimeType", pStringObj);
			pStringObj->Release();
		}
	
		// notify RMA core
		m_pStatus->StreamHeaderReady(PN_STATUS_OK, pStreamHeaderObj);

		// release the object
		pStreamHeaderObj->Release();
	}

	//printf("Exiting CreateStreamHeaderObj(%d)\n", m_instno);
}

STDMETHODIMP CVorbisFileFormat::GetPacket(UINT16 streamNo)
{
	//printf("Entering GetPacket(%d)\n", m_instno);

	if (m_State != Ready) return PNR_UNEXPECTED;

	m_State = GetPacketReadPending;

	m_pFileObj->Read(READ_LENGTH);

	//printf("Exiting GetPacket(%d)\n", m_instno);

	return PNR_OK;
}

void CVorbisFileFormat::CreatePacketObj(ogg_page *og, ogg_int64_t granulepos)
{
	char *buffer;

	//printf("Entering CreatePacketObj(%d)\n", m_instno);

	m_State = Ready;

	IRMAPacket *pPacketObj = NULL;
	m_pClassFactory->CreateInstance(CLSID_IRMAPacket, (void **)&pPacketObj);
	if (pPacketObj != NULL) {
		float deliveryTime = (float)granulepos / 44100.0 * 1000.0;
		UINT16 streamNo = 0;

		printf("granulepos = %lld  deliverytime = %f\n", granulepos, deliveryTime);

		IRMABuffer *pBuffer = NULL;
		m_pClassFactory->CreateInstance(CLSID_IRMABuffer, (void **)&pBuffer);
		if (pBuffer != NULL) {
			pBuffer->SetSize(og->header_len + og->body_len);
			buffer = (char *)pBuffer->GetBuffer();
			memcpy(buffer, og->header, og->header_len);
			memcpy(&buffer[og->header_len], og->body, og->body_len);

			pPacketObj->Set(pBuffer, (UINT32)deliveryTime, streamNo, 0, 0);

			m_pStatus->PacketReady(PN_STATUS_OK, pPacketObj);

			pPacketObj->Release();
		} else {
			printf("OH SHIT111!\n");
		}
	} else {
		printf("OH SHIT222!\n");
	}

	//printf("Exiting CreatePacketObj(%d)\n", m_instno);
}

STDMETHODIMP CVorbisFileFormat::Seek(UINT32 requestedTime)
{
	//printf("Entering Seek(%d)\n", m_instno);
	// not implemented

	//printf("Exiting Seek(%d)\n", m_instno);
	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::Close(void)
{
	//printf("Entering Close(%d)\n", m_instno);
	if (m_ogg_inited) {
		vorbis_comment_clear(&m_initial_c);
		vorbis_info_clear(&m_initial_i);
		ogg_sync_clear(&m_oy);
	}

	if (m_pFileObj != NULL) {
		m_pFileObj->Close();
		m_pFileObj->Release();
		m_pFileObj = NULL;
	}

	if (m_pStatus != NULL) {
		m_pStatus->Release();
		m_pStatus = NULL;
	}

	if (m_pClassFactory != NULL) {
		m_pClassFactory->Release();
		m_pClassFactory = NULL;
	}

	//printf("Exiting Close(%d)\n", m_instno);

	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::CloseDone(PN_RESULT result)
{
	//printf("CloseDone() not implemented\n");
	return PNR_OK;
}

STDMETHODIMP CVorbisFileFormat::WriteDone(PN_RESULT result)
{
	//printf("WriteDone() not implemented\n");
	return PNR_OK;
}

CVorbisFileFormat::~CVorbisFileFormat(void)
{
	//printf("Entering ~CVorbisFileFormat(%d)\n", m_instno);
	Close();
	//printf("Exiting ~CVorbisFileFormat(%d)\n", m_instno);
}

STDMETHODIMP_(UINT32) CVorbisFileFormat::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

STDMETHODIMP_(UINT32) CVorbisFileFormat::Release(void)
{
	if (InterlockedDecrement(&m_RefCount) > 0)
		return m_RefCount;

	delete this;
	return 0;
}

STDMETHODIMP CVorbisFileFormat::QueryInterface(REFIID interfaceID, void **ppInterfaceObj)
{
	if (IsEqualIID(interfaceID, IID_IUnknown)) {
		AddRef();
		*ppInterfaceObj = (IUnknown *)(IRMAPlugin *)this;
		return PNR_OK;
	} else if (IsEqualIID(interfaceID, IID_IRMAPlugin)) {
		AddRef();
		*ppInterfaceObj = (IRMAPlugin *)this;
		return PNR_OK;
	} else if (IsEqualIID(interfaceID, IID_IRMAFileResponse)) {
		AddRef();
		*ppInterfaceObj = (IRMAFileResponse *)this;
		return PNR_OK;
	} else if (IsEqualIID(interfaceID, IID_IRMAFileFormatObject)) {
		AddRef();
		*ppInterfaceObj = (IRMAFileFormatObject *)this;
		return PNR_OK;
	} else if (IsEqualIID(interfaceID, IID_IRMAFileStatResponse)) {
		AddRef();
		*ppInterfaceObj = (IRMAFileStatResponse *)this;
		return PNR_OK;
	}

	*ppInterfaceObj = NULL;
	return PNR_NOINTERFACE;
}
