#pragma once
class IOggDecoder 
{
public:
	enum eAcceptHeaderResult {
		AHR_ALL_HEADERS_RECEIVED,
		AHR_MORE_HEADERS_TO_COME,
		AHR_INVALID_HEADER,
		AHR_UNEXPECTED,
		AHR_NULL_POINTER,

	};
	virtual LOOG_INT64 convertGranuleToTime(LOOG_INT64 inGranule) = 0;
	virtual eAcceptHeaderResult showHeaderPacket(OggPacket* inCodecHeaderPacket) = 0;
	virtual string getCodecShortName() = 0;
	virtual string getCodecIdentString() = 0;
	
};