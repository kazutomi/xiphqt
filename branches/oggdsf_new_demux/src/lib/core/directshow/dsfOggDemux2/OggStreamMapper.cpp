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
			//WARNING::: Partial fall through
			if (inOggPage->header()->isBOS()) {
				return addNewPin(inOggPage);
			} else {
				mStreamState = STRMAP_PARSING_HEADERS;
			}
			//Partial fall through
		case STRMAP_PARSING_HEADERS:
			if (!allStreamsReady()) {
				OggDemuxPageSourcePin* locPin = getMatchingPin(inOggPage->header()->StreamSerialNo());
				//TODO::: NULL pointer check
				IOggDecoder* locDecoder = locPin->getDecoderInterface();
				if (locDecoder == NULL) {
					mStreamState = STRMAP_ERROR;
					delete inOggPage;
				} else {
					IOggDecoder::eAcceptHeaderResult locResult = locDecoder->acceptHeaderPage(inOggPage);
					switch (locResult) {
						case IOggDecoder::eAcceptHeaderResult::AHR_ALL_HEADERS_RECEIVED:
							locPin->setIsStreamReady(true);
							return true;
						case IOggDecoder::eAcceptHeaderResult::AHR_INVALID_HEADER:
							mStreamState = STRMAP_ERROR;
							return false;
						case IOggDecoder::eAcceptHeaderResult::AHR_MORE_HEADERS_TO_COME:
							return true;
						case IOggDecoder::eAcceptHeaderResult::AHR_NULL_POINTER:
							mStreamState = STRMAP_ERROR;
							return false;
						case IOggDecoder::eAcceptHeaderResult::AHR_UNEXPECTED:
							mStreamState = STRMAP_ERROR;
							return false;
						default:
							return false;
						
					}
				}
			} else {
				mStreamState = STRMAP_DATA;

			}
			//Partial fall through
		case STRMAP_DATA:
			{
				OggDemuxPageSourcePin* locPin = getMatchingPin(inOggPage->header()->StreamSerialNo());
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
	//OggDemuxPageSourcePin* locPin = NULL;
	for (size_t i = 0; i < mPins.size(); i++) {
		locAllReady = locAllReady && (mPins[i]->isStreamReady());
	}	

	return locAllReady && (mPins.size() > 0);
}

bool OggStreamMapper::addNewPin(OggPage* inOggPage)
{
	OggDemuxPageSourcePin* locNewPin = new OggDemuxPageSourcePin(NAME("OggPageSourcePin"), mParentFilter, mParentFilterLock, inOggPage);
	mPins.push_back(locNewPin);
	return true;
}

OggDemuxPageSourcePin* OggStreamMapper::getMatchingPin(unsigned long inSerialNo)
{
	OggDemuxPageSourcePin* locPin = NULL;
	for (size_t i = 0; i < mPins.size(); i++) {
		locPin = mPins[i];
		if (locPin->getSerialNo() == inSerialNo) {
			return locPin;
		}
	}
	return NULL;
}