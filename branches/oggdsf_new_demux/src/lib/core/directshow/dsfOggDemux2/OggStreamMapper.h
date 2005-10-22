#pragma once

//#include "OggDemuxPageSourcePin.h"
//#include "OggDemuxPageSourceFilter.h"

#include <libOOOgg/IOggCallback.h>
#include <vector>
using namespace std;


class OggStreamMapper
	:	public IOggCallback
{
public:

	enum eStreamState {
		STRMAP_READY,
		STRMAP_PARSING_BOS_PAGES,
		STRMAP_PARSING_HEADERS,
		STRMAP_DATA,
		STRMAP_FINISHED,
		STRMAP_ERROR

	};
	OggStreamMapper(OggDemuxPageSourceFilter* inParentFilter, CCritSec* inParentFilterLock);
	virtual ~OggStreamMapper(void);

	//IOggCallback Interface
	virtual bool acceptOggPage(OggPage* inOggPage);

	eStreamState streamState();

protected:
	eStreamState mStreamState;
	vector<OggDemuxPageSourcePin*> mPins;
	OggDemuxPageSourceFilter* mParentFilter;
	CCritSec* mParentFilterLock;

	bool addNewPin(OggPage* inOggPage);
};
