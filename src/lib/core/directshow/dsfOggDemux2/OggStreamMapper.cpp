#include "StdAfx.h"
#include ".\oggstreammapper.h"

OggStreamMapper::OggStreamMapper(OggDemuxPageSourceFilter* inParentFilter, CCritSec* inParentFilterLock)
	:	mStreamState(eStreamState::STRMAP_READY)
	,	mParentFilter(inParentFilter)
	,	mParentFilterLock(inParentFilterLock)
{
}

OggStreamMapper::~OggStreamMapper(void)
{
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
			return addNewPin(inOggPage);
		case STRMAP_PARSING_HEADERS:
		case STRMAP_DATA:
		case STRMAP_FINISHED:
		case STRMAP_ERROR:
			break;
	}
	

}

bool OggStreamMapper::addNewPin(OggPage* inOggPage)
{
	OggDemuxPageSourcePin* locNewPin = new OggDemuxPageSourcePin(NAME("OggPageSourcePin"), mParentFilter, mParentFilterLock, inOggPage);
	mPins.push_back(locNewPin);
	return true;
}