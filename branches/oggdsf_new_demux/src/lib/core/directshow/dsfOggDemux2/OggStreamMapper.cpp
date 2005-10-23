#include "StdAfx.h"
#include ".\oggstreammapper.h"

OggStreamMapper::OggStreamMapper(OggDemuxPacketSourceFilter* inParentFilter, CCritSec* inParentFilterLock)
	:	mStreamState(eStreamState::STRMAP_READY)
	,	mParentFilter(inParentFilter)
	,	mParentFilterLock(inParentFilterLock)
{
}

OggStreamMapper::~OggStreamMapper(void)
{
	for (size_t i = 0; i < mPins.size(); i++) {
		delete mPins[i];
	}
}
OggDemuxPacketSourcePin* OggStreamMapper::getPinByIndex(unsigned long inIndex)
{
	if (inIndex < mPins.size()) {
		return mPins[inIndex];
	} else {
		return NULL;
	}
}
bool OggStreamMapper::acceptOggPage(OggPage* inOggPage)
{
	
	switch (mStreamState) {
		case STRMAP_READY:
			//WARNING::: Partial fall through
			if (inOggPage->header()->isBOS()) {
				mStreamState = STRMAP_PARSING_BOS_PAGES;
			} else {
				mStreamState = STRMAP_ERROR;
				delete inOggPage;
				return false;
			}
			//Partial fall through
		case STRMAP_PARSING_BOS_PAGES:
			//WARNING::: Partial fall through
			if (!allStreamsReady()) {
				if (inOggPage->header()->isBOS()) {
					return addNewPin(inOggPage);
				} else {
					mStreamState = STRMAP_DATA;
				}
			}
			//Partial fall through
		case STRMAP_DATA:
			{
				OggDemuxPacketSourcePin* locPin = getMatchingPin(inOggPage->header()->StreamSerialNo());
				return locPin->acceptOggPage(inOggPage);
			}
		case STRMAP_FINISHED:
			return false;
		case STRMAP_ERROR:
			return false;
			
	}
	

}

bool OggStreamMapper::allStreamsReady()
{
	bool locAllReady = true;
	//OggDemuxPacketSourcePin* locPin = NULL;
	for (size_t i = 0; i < mPins.size(); i++) {
		locAllReady = locAllReady && (mPins[i]->isStreamReady());
	}	

	return locAllReady && (mPins.size() > 0);
}

bool OggStreamMapper::addNewPin(OggPage* inOggPage)
{
	OggDemuxPacketSourcePin* locNewPin = new OggDemuxPacketSourcePin(NAME("OggPageSourcePin"), mParentFilter, mParentFilterLock, inOggPage->getPacket(0)->clone(), inOggPage->header()->StreamSerialNo());
	delete inOggPage;
	mPins.push_back(locNewPin);
	return true;
}

OggDemuxPacketSourcePin* OggStreamMapper::getMatchingPin(unsigned long inSerialNo)
{
	OggDemuxPacketSourcePin* locPin = NULL;
	for (size_t i = 0; i < mPins.size(); i++) {
		locPin = mPins[i];
		if (locPin->getSerialNo() == inSerialNo) {
			return locPin;
		}
	}
	return NULL;
}