#ifndef __RVORBIS_H__
#define __RVORBIS_H__

#define PLUGIN_VERSION 0
#define DESCRIPTION "Ogg Vorbis Renderer Plugin"
#define COPYRIGHT "(c) 2001 Xiphophorus, All Rights Reserved"
#define MORE_INFO_URL "http://www.xiph.org"
#define STREAM_MIME_TYPES { "application/x-ogg", NULL }
#define TIME_SYNC_FREQ 100

typedef enum
{
	OggInitialHeader,
	OggCommentHeader,
	OggCodebookHeader,
	OggPlay
} OggState;

class CVorbisRenderer: public IRMARenderer,
		       public IRMAPlugin,
		       public IRMADryNotification
{
 public:
	CVorbisRenderer(void);

	STDMETHOD(StartStream) (THIS_ IRMAStream *pStream, IRMAPlayer *pPlayer);
	STDMETHOD(EndStream) (THIS);
	STDMETHOD(OnHeader) (THIS_ IRMAValues *pStreamHeaderObj);
	STDMETHOD(OnPacket) (THIS_ IRMAPacket *pPacketObj, INT32 timeOffset);
	STDMETHOD(OnTimeSync) (THIS_ UINT32 currentPlayBackTime);
	STDMETHOD(OnPreSeek) (THIS_ UINT32 timeBefore, UINT32 timeAfter);
	STDMETHOD(OnPostSeek) (THIS_ UINT32 timeBefore, UINT32 timeAfter);
	STDMETHOD(OnPause) (THIS_ UINT32 timeBeforePause);
	STDMETHOD(OnBegin) (THIS_ UINT32 timeAfterBegin);
	STDMETHOD(OnBuffering) (THIS_ UINT32 reason, UINT16 percentComplete);
	STDMETHOD(GetRendererInfo)
		(THIS_
		 REF(const char **) pStreamMimeTypes,
		 REF(UINT32) initialGranularity
		);
	STDMETHOD(GetDisplayType)
		(THIS_
		 REF(RMA_DISPLAY_TYPE) displayType,
		 REF(IRMABuffer *) pDisplayInfo
		);
	STDMETHOD(OnEndofPackets) (THIS);

	STDMETHOD(GetPluginInfo)
		(THIS_
		 REF(BOOL) bLoadMultiple,
		 REF(const char *) pDescription,
		 REF(const char *) pCopyright,
		 REF(const char *) pMoreInfoURL,
		 REF(UINT32) versionNumber
		);
	STDMETHOD(InitPlugin) (THIS_ IUnknown *pRMACore);

	// IRMADryNotification Interface
	STDMETHOD(OnDryNotification)
		(THIS_
		 UINT32 ulCurrentStreamTime,
		 UINT32 ulMinimumDurationRequired
		);

	STDMETHOD(QueryInterface) (THIS_ REFIID interfaceID, void **ppInterfaceObj);
	STDMETHOD_(UINT32, AddRef) (THIS);
	STDMETHOD_(UINT32, Release) (THIS);

 private:

	UINT32 CalcMs(UINT32 ulNumBytes);
	~CVorbisRenderer(void);

	INT32 m_RefCount;
	IRMACommonClassFactory *m_pClassFactory;
	BOOL m_bInSeekMode;
	BOOL m_bIsGapInStreaming;

	// vorbis decoding
	OggState m_OggState;
	ogg_sync_state m_oy;
	ogg_stream_state m_os;
	vorbis_info m_vi;
	vorbis_comment m_vc;
	vorbis_dsp_state m_vd;
	vorbis_block m_vb;

	// more stuff
	ogg_int64_t m_totalSamples;
	UINT32 m_oldRealTime;
	IRMAStream *m_pStream;
	IRMAPlayer *m_pPlayer;
	IRMAAudioPlayer *m_pAudioPlayer;
	IRMAAudioStream *m_pAudioStream;
	RMAAudioFormat m_audioFmt;
	FiveMinuteQueue m_PacketQueue;

	static const char *zm_pDescription;
	static const char *zm_pCopyright;
	static const char *zm_pMoreInfoURL;
	static const char *zm_pStreamMimeTypes[];

	PRIVATE_DESTRUCTORS_ARE_NOT_A_CRIME
};

#endif  // __RVORBIS_H__
