#ifndef __FVORBIS_H__
#define __FVORBIS_H__

#define PLUGIN_VERSION 0
#define DESCRIPTION "Ogg Vorbis File Format Plugin"
#define COPYRIGHT "(c) 2001 Xiphophorus, All rights reserved."
#define MORE_INFO_URL "http://www.xiph.org"
#define FILE_MIME_TYPES {"application/x-ogg", NULL}
#define FILE_EXTENSIONS {"ogg", NULL}
#define FILE_OPEN_NAMES {"Ogg Vorbis (*.ogg)", NULL}
#define STREAM_NO 0
#define STREAM_MIME_TYPE "application/x-ogg"
#define READ_LENGTH 4096
#ifdef _WIN32
#define strncasecmp strnicmp
#endif

typedef enum
{
	Ready,
	InitPending,
	GetFileHeaderStatPending,

	GFHInitialHeaderReadPending,
	GFHInitialHeaderCommentReadPending,
	GFHInitialHeaderCodebookReadPending,

	GFHSeekEndPending,
	GFHSeekEndSearching,
	GFHReadEndSearching,
	GFHSeekBeginPending,

	GetPacketSeekPending,
	GetPacketReadPending,
	SeekSeekPending
} PluginState;

class CVorbisFileFormat :	public IRMAFileFormatObject,
				public IRMAFileResponse,
				public IRMAFileStatResponse,
				public IRMAPlugin
{
	public:

	CVorbisFileFormat(void);

	// IRMAFileFormatObject interface
	STDMETHOD(GetFileFormatInfo)
		(THIS_
			REF(const char **)pFileMimeTypes,
			REF(const char **)pFileExtensions,
			REF(const char **)pFileOpenNames
		);

	STDMETHOD(InitFileFormat)
		(THIS_
			IRMARequest *pRequest,
			IRMAFormatResponse *pFormatResponse,
			IRMAFileObject *pFileObject
		);

	STDMETHOD(GetFileHeader) (THIS);
	STDMETHOD(GetStreamHeader) (THIS_ UINT16 streamNo);
	STDMETHOD(GetPacket) (THIS_ UINT16 streamNo);
	STDMETHOD(Seek) (THIS_ UINT32 requestedTime);
	STDMETHOD(Close) (THIS);

	// IRMAFileResponse Interface
	STDMETHOD(InitDone) (THIS_ PN_RESULT status);
	STDMETHOD(SeekDone) (THIS_ PN_RESULT status);
	STDMETHOD(ReadDone) (THIS_ PN_RESULT status, IRMABuffer *pBuffer);
	STDMETHOD(WriteDone) (THIS_ PN_RESULT status);
	STDMETHOD(CloseDone) (THIS_ PN_RESULT status);

	// IRMAFileStatResponse Interface
	STDMETHOD(StatDone) 
		(THIS_ 
			PN_RESULT status, 
			UINT32 ulSize, 
			UINT32 ulCreationTime, 
			UINT32 ulAccessTime, 
			UINT32 ulModificationTime,
			UINT32 ulMode
		 );

	// IRMAPlugin Interface
	STDMETHOD(GetPluginInfo)
		(THIS_
			REF(BOOL) bLoadMultiple,
			REF(const char *) pDescription,
			REF(const char *) pCopyright,
			REF(const char *) pMoreInfoURL,
			REF(UINT32) versionNumber
		);

	STDMETHOD(InitPlugin) (THIS_ IUnknown *pRMACore);

	// IUnknown COM Interface
	STDMETHOD(QueryInterface) (THIS_ REFIID ID, void **ppInterfaceObj);
	STDMETHOD_(UINT32, AddRef) (THIS);
	STDMETHOD_(UINT32, Release) (THIS);

	private:

	// private class variables
	UINT32 m_FileTime;      // total time of the file in milliseconds
	UINT32 m_FileSize;      // file size of the current file
	INT32 m_RefCount;	// Objects reference count
	IRMACommonClassFactory *m_pClassFactory; // creates common RMA classes
	IRMAFileObject *m_pFileObj;	// Used for file i/o
	IRMAFormatResponse *m_pStatus;	// Reports status to RMA core
	PluginState m_State;	// Status used for async calls
	UINT32 m_NextPacketDeliveryTime;	// Delivery time of next packet

	// ogg vorbis stuff
	int m_ogg_inited;
	ogg_int64_t m_granulepos;
	ogg_int64_t m_offset;
	ogg_int64_t m_begin;
	long m_initial_serialno;
	ogg_sync_state m_oy;

	//int m_links;
	//ogg_int64_t *m_offsets;
	//ogg_int64_t *m_dataoffsets;
	//long *m_serialnos;
	//ogg_int64_t *m_pcmlengths;
	//vorbis_info *m_vi;
	//vorbis_comment *m_vc;
	vorbis_info m_initial_i;
	vorbis_comment m_initial_c;

	// vorbis working state
	//ogg_int64_t m_pcmoffset;
	//long m_current_serialno;
	//int m_current_link;

	ogg_stream_state m_os;

	// private static class variables
	int m_instno;
	static int instno;
	static const char *zm_pDescription;
	static const char *zm_pCopyright;
	static const char *zm_pMoreInfoURL;
	static const char *zm_pFileMimeTypes[];
	static const char *zm_pFileExtensions[];
	static const char *zm_pFileOpenNames[];

	// private class methods
	~CVorbisFileFormat(void);
	void CreateFileHeaderObj(void);
	void CreateStreamHeaderObj(void);
	void CreatePacketObj(ogg_page *og, ogg_int64_t granulepos);

	PRIVATE_DESTRUCTORS_ARE_NOT_A_CRIME
};

#endif  // __FVORBIS_H__
