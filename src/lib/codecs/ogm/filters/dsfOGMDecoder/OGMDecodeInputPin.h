#pragma once

#include "ogmdecoderdllstuff.h"
#include "IOggDecoder.h"

class OGMDecodeFilter;

class OGMDecodeInputPin
	:	public CTransformInputPin
	,	public IOggDecoder
{
public:
	OGMDecodeInputPin(OGMDecodeFilter* inParent, HRESULT* outHR);
	virtual ~OGMDecodeInputPin(void);

	virtual STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *outRequestedProps);
	virtual HRESULT SetMediaType(const CMediaType* inMediaType);
	virtual HRESULT CheckMediaType(const CMediaType *inMediaType);
	//IOggDecoder Interface
	virtual LOOG_INT64 convertGranuleToTime(LOOG_INT64 inGranule);
	virtual LOOG_INT64 mustSeekBefore(LOOG_INT64 inGranule);
	virtual IOggDecoder::eAcceptHeaderResult showHeaderPacket(OggPacket* inCodecHeaderPacket);
	virtual string getCodecShortName();
	virtual string getCodecIdentString();

	VIDEOINFOHEADER* getVideoFormatBlock()		{		return mVideoFormatBlock;	}
protected:
	enum eOGMSetupState {
		VSS_SEEN_NOTHING,
		VSS_SEEN_BOS,
		VSS_SEEN_COMMENT,
		VSS_ALL_HEADERS_SEEN,
		VSS_ERROR
	};

	eOGMSetupState mSetupState;
	bool handleHeaderPacket(OggPacket* inHeaderPack);

	VIDEOINFOHEADER* mVideoFormatBlock;

	static const unsigned long OGM_IDENT_HEADER_SIZE = 80;
	static const unsigned long OGM_NUM_BUFFERS = 50;
	static const unsigned long OGM_BUFFER_SIZE = 1024*512*3;;

};
