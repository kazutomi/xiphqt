#define INITGUID

#include <stdio.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "pntypes.h"
#include "pncom.h"
#include "rmacore.h"
#include "rmacomm.h"
#include "rmapckts.h"
#include "rmaplugn.h"
#include "rmarendr.h"
#include "rmaausvc.h"

#include "fivemque.h"
#include "rvorbis.h"

STDAPI RMACreateInstance(IUnknown **ppRendererObj)
{
	*ppRendererObj = (IUnknown *)(IRMAPlugin *)new CVorbisRenderer();
	if (*ppRendererObj != NULL) {
		(*ppRendererObj)->AddRef();
		return PNR_OK;
	}

	return PNR_OUTOFMEMORY;
}

const char *CVorbisRenderer::zm_pDescription = DESCRIPTION;
const char *CVorbisRenderer::zm_pCopyright = COPYRIGHT;
const char *CVorbisRenderer::zm_pMoreInfoURL = MORE_INFO_URL;
const char *CVorbisRenderer::zm_pStreamMimeTypes[] = STREAM_MIME_TYPES;

CVorbisRenderer::CVorbisRenderer(void)
	: m_RefCount(0),
	  m_pClassFactory(NULL),
	  m_bInSeekMode(FALSE),
	  m_bIsGapInStreaming(TRUE),
	  m_oldRealTime(0),
	  m_totalSamples(0)
{
}

STDMETHODIMP CVorbisRenderer::GetPluginInfo(
	REF(BOOL) bLoadMultiple,
	REF(const char *) pDescription,
	REF(const char *) pCopyright,
	REF(const char *) pMoreInfoURL,
	REF(UINT32) versionNumber
)
{
	bLoadMultiple = TRUE;
	pDescription = zm_pDescription;
	pCopyright = zm_pCopyright;
	pMoreInfoURL = zm_pMoreInfoURL;
	versionNumber = PLUGIN_VERSION;

	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::GetRendererInfo(
	REF(const char **) pStreamMimeTypes,
	REF(UINT32) initialGranularity
)
{
	pStreamMimeTypes = zm_pStreamMimeTypes;
	initialGranularity = TIME_SYNC_FREQ;

	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::InitPlugin(IUnknown *pRMACore)
{
	pRMACore->QueryInterface(IID_IRMACommonClassFactory, (void **)&m_pClassFactory);
	if (m_pClassFactory == NULL)
		return PNR_NOTIMPL;

	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::StartStream(IRMAStream *pStream, IRMAPlayer *pPlayer)
{
	m_OggState = OggInitialHeader;

	ogg_sync_init(&m_oy);

	m_pStream = pStream;
	m_pPlayer = pPlayer;

	if (m_pStream) m_pStream->AddRef();
	if (m_pPlayer) m_pPlayer->AddRef();

	// get interface to audio player
	if (PNR_OK != m_pPlayer->QueryInterface(IID_IRMAAudioPlayer, (void **)&m_pAudioPlayer))
		return !PNR_OK;

	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnHeader(IRMAValues *pStreamHeaderObj)
{
	m_audioFmt.uChannels = 2;
	m_audioFmt.uBitsPerSample = 16;
	m_audioFmt.ulSamplesPerSec = 44100;
	m_audioFmt.uMaxBlockSize = 4096;
	
	m_pAudioPlayer->CreateAudioStream(&m_pAudioStream);
	m_pAudioStream->Init(&m_audioFmt, pStreamHeaderObj);

	m_pAudioStream->AddDryNotification(this);

	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnBegin(UINT32 timeAfterBeing)
{
	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::GetDisplayType(
	REF(RMA_DISPLAY_TYPE) displayType,
	REF(IRMABuffer *) pDisplayInfo
)
{
	displayType = RMA_DISPLAY_NONE;
	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnPacket(IRMAPacket *pPacketObj, INT32 streamOffsetTime)
{
	char *buffer;
	ogg_page og;
	ogg_packet op;
	int result;
	int falling_through = 0;

	// ignore pre-seek packets
	if (m_bInSeekMode) return PNR_OK;

	// did we get a valid packet?
	if (pPacketObj->IsLost()) {
		printf("Lost a packet????\n");
		m_bIsGapInStreaming = TRUE;
		return PNR_OK;
	}

	LONG32 lRealTime = pPacketObj->GetTime();
	if (streamOffsetTime > lRealTime)
		lRealTime = 0;
	else
//		lRealTime -= streamOffsetTime;
		;

	printf("streamoffsettime = %d\n", streamOffsetTime);

	// now correct m_oldRealTime
	m_oldRealTime = lRealTime;

	IRMABuffer *pPacketBuff = pPacketObj->GetBuffer();
	if (pPacketBuff != NULL) {
		UCHAR *pPacketData = pPacketBuff->GetBuffer();
		if (pPacketData != NULL) {
			//printf("RENDER: decoding ogg page\n");
			// submit packet to ogg layer
			buffer = ogg_sync_buffer(&m_oy, pPacketBuff->GetSize());
			memcpy(buffer, pPacketData, pPacketBuff->GetSize());
			ogg_sync_wrote(&m_oy, pPacketBuff->GetSize());
			
			if (ogg_sync_pageout(&m_oy, &og) != 1) {
				// not enough data yet
				pPacketBuff->Release();
				return PNR_OK;
			}

			switch (m_OggState) {
			case OggInitialHeader:
				ogg_stream_init(&m_os, ogg_page_serialno(&og));
				vorbis_info_init(&m_vi);
				vorbis_comment_init(&m_vc);
				if (ogg_stream_pagein(&m_os, &og) < 0) {
					pPacketBuff->Release();
					return PNR_UNEXPECTED;
				}
				if (ogg_stream_packetout(&m_os, &op) != 1) {
					pPacketBuff->Release();
					return PNR_UNEXPECTED;
				}
				if (vorbis_synthesis_headerin(&m_vi, &m_vc, &op) < 0) {
					pPacketBuff->Release();
					return PNR_UNEXPECTED;
				}
				m_OggState = OggCommentHeader;
				
				printf("Got Initial Header\n");

				falling_through = 1;
				// fall through intentional
			case OggCommentHeader:
				if (!falling_through) {
					ogg_stream_pagein(&m_os, &og);
				}
				result = ogg_stream_packetout(&m_os, &op);
				if (result < 0) {
					pPacketBuff->Release();
					return PNR_UNEXPECTED;
				}
				// need more data?
				if (result == 0) break;
				vorbis_synthesis_headerin(&m_vi, &m_vc, &op);
				m_OggState = OggCodebookHeader;
				
				printf("Got Comment Header\n");

				falling_through = 1;
				// fall through intentional
			case OggCodebookHeader:
				if (!falling_through) {
					ogg_stream_pagein(&m_os, &og);
				}
				result = ogg_stream_packetout(&m_os, &op);
				if (result < 0) {
					pPacketBuff->Release();
					return PNR_UNEXPECTED;
				}
				// need more data
				if (result == 0) break;
				vorbis_synthesis_headerin(&m_vi, &m_vc, &op);
				vorbis_synthesis_init(&m_vd, &m_vi);
				vorbis_block_init(&m_vd, &m_vb);

				m_OggState = OggPlay;

				printf("Got Codebooks\n");

				falling_through = 1;
				// fall through intentional
			case OggPlay:
				if (!falling_through) {
					ogg_stream_pagein(&m_os, &og);
				}

				printf("OGG PAGE CAME IN\n");
				while (ogg_stream_packetout(&m_os, &op) > 0) {
					// we have a packet to decode
					float **pcm;
					int samples;
					int convsize;
					ogg_int16_t convbuffer[4096];

					convsize = 4096 / m_vi.channels;
					if (vorbis_synthesis(&m_vb, &op) == 0)
						vorbis_synthesis_blockin(&m_vd, &m_vb);
					else
						printf("ERROR ? HMMMMMMM\n");

					//printf("got a packet, get some samples\n");
					while ((samples = vorbis_synthesis_pcmout(&m_vd, &pcm)) > 0) {
						int i, j;
						int clipflag = 0;
						int bout = (samples < convsize ? samples : convsize);
						
						// convert floats to 16bit signed ints (host order) and interleave
						for (i = 0; i < m_vi.channels; i++) {
							ogg_int16_t *ptr = convbuffer + i;
							float *mono = pcm[i];
							
							for (j = 0; j < bout; j++) {
								int val = mono[j] * 32767.f;
								// guard against clipping
								if (val > 32767) {
									val = 32767;
									clipflag = 1;
								}
								if (val < -32768) {
									val = -32768;
									clipflag = 1;
								}
								*ptr = val;
								ptr += 2;
							}
						}
						
						
						// send to audio output
						IRMABuffer *pBuffer = NULL;
						char *tmp;
						// create and fill a buffer
						m_pClassFactory->CreateInstance(CLSID_IRMABuffer, (void **)&pBuffer);
						if (pBuffer == NULL) {
							printf("ERROR - no memory?\n");
							return PNR_OUTOFMEMORY;
						}
						
						pBuffer->SetSize(bout * m_vi.channels * 2);
						tmp = (char *)pBuffer->GetBuffer();
						memcpy(tmp, convbuffer, bout * m_vi.channels * 2);
						
						m_oldRealTime = CalcMs(m_totalSamples * 4);
						m_totalSamples += bout;

						RMAAudioData *pAudioData = new RMAAudioData;
						pAudioData->pData = pBuffer;
						pAudioData->ulAudioTime = m_oldRealTime;

						printf("Sending %d samples (%ld ms) time = %d to audio out\n", bout, CalcMs(bout*4), m_oldRealTime);
						//m_oldRealTime += CalcMs(bout * 4);
						
						if (m_bIsGapInStreaming) {
							pAudioData->uAudioStreamType = TIMED_AUDIO;
							m_bIsGapInStreaming = FALSE;
						} else {
							pAudioData->uAudioStreamType = STREAMING_AUDIO;
						}
						
						m_PacketQueue.Add((void *)pAudioData);
						//m_pAudioStream->Write(pAudioData);
						//pAudioData->pData->Release();
						//delete pAudioData;
						
						// tell vorbis how many samples we consumed
						vorbis_synthesis_read(&m_vd, bout);
					}
					//printf("done decoding packet\n");
				}
				//m_oldRealTime = lRealTime;
				printf("old/real - %d/%d\n", m_oldRealTime, lRealTime);
				printf("OGG PAGE DONE\n");
				break;
			default:
				break;
			}
		}

		pPacketBuff->Release();
	}

	return PNR_OK;
}

UINT32 CVorbisRenderer::CalcMs(UINT32 ulNumBytes)
{
	return ((UINT32)((1000.0 / (m_audioFmt.uChannels * ((m_audioFmt.uBitsPerSample == 8) ? 1 : 2)
				    * m_audioFmt.ulSamplesPerSec)) * ulNumBytes));
}

STDMETHODIMP CVorbisRenderer::OnDryNotification(UINT32 ulCurrentStreamTime, UINT32 ulMinimumDurationRequired)
{
	UINT32 ulNumMsWritten = 0;

	RMAAudioData *pAudioData = (RMAAudioData *)m_PacketQueue.Remove();
	while (pAudioData) {
		m_pAudioStream->Write(pAudioData);
		
		ulNumMsWritten += CalcMs(pAudioData->pData->GetSize());

		pAudioData->pData->Release();
		delete pAudioData;

		if (ulNumMsWritten >= ulMinimumDurationRequired) break;

		pAudioData = (RMAAudioData *)m_PacketQueue.Remove();
	}

	printf("needed %ldms sent %ldms...\n", ulMinimumDurationRequired, ulNumMsWritten);
	
	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnTimeSync(UINT32 currentPlayBackTime)
{
	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnPreSeek(UINT32 timeBeforeSeek, UINT32 timeAfterSeek)
{
	m_bInSeekMode = TRUE;
	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnPostSeek(UINT32 timeBeforeSeek, UINT32 timeAfterSeek)
{
	m_bInSeekMode = FALSE;
	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnPause(UINT32 timeBeforePause)
{
	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnBuffering(UINT32 reason, UINT16 percentComplete)
{
	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::OnEndofPackets(void)
{
	vorbis_comment_clear(&m_vc);
	vorbis_info_clear(&m_vi);
	ogg_stream_clear(&m_os);
	ogg_sync_clear(&m_oy);

	return PNR_OK;
}

STDMETHODIMP CVorbisRenderer::EndStream(void)
{
	if (m_pClassFactory != NULL) {
		m_pClassFactory->Release();
		m_pClassFactory = NULL;
	}
	
	return PNR_OK;
}

CVorbisRenderer::~CVorbisRenderer(void)
{
	EndStream();
}

STDMETHODIMP_(UINT32) CVorbisRenderer::AddRef(void)
{
	return InterlockedIncrement(&m_RefCount);
}

STDMETHODIMP_(UINT32) CVorbisRenderer::Release(void)
{
	if (InterlockedDecrement(&m_RefCount) > 0)
		return m_RefCount;

	delete this;
	return 0;
}

STDMETHODIMP CVorbisRenderer::QueryInterface(REFIID interfaceID, void **ppInterfaceObj)
{
	if (IsEqualIID(interfaceID, IID_IUnknown)) {
		AddRef();
		*ppInterfaceObj = (IUnknown *)(IRMAPlugin *)this;
		return PNR_OK;
	} else if (IsEqualIID(interfaceID, IID_IRMAPlugin)) {
		AddRef();
		*ppInterfaceObj = (IRMAPlugin *)this;
		return PNR_OK;
	} else if (IsEqualIID(interfaceID, IID_IRMARenderer)) {
		AddRef();
		*ppInterfaceObj = (IRMARenderer *)this;
		return PNR_OK;
	}

	*ppInterfaceObj = NULL;
	return PNR_NOINTERFACE;
}
