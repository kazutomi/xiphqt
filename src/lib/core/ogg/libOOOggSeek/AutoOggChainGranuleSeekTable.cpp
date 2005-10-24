#include "StdAfx.h"
#include ".\autooggchaingranuleseektable.h"

AutoOggChainGranuleSeekTable::AutoOggChainGranuleSeekTable(string inFilename)
	:	mFilename(inFilename)
	,	mFilePos(0)
	,	mOggDemux(NULL)
	,	mDuration(0)
	,	mIsEnabled(false)
{
	mOggDemux = new OggDataBuffer;
	mOggDemux->registerVirtualCallback(this);

}

AutoOggChainGranuleSeekTable::~AutoOggChainGranuleSeekTable(void)
{
	delete mOggDemux;

	for (size_t i = 0; i < mStreamMaps.size(); i++) {
		delete mStreamMaps[i].mSeekTable;
	}
}


bool AutoOggChainGranuleSeekTable::buildTable()
{
	if (mFilename.find("http") != 0) {
		
		//mSeekMap.clear();
		//addSeekPoint(0, 0);

		mFile.open(mFilename.c_str(), ios_base::in | ios_base::binary);
		//TODO::: Error check
		const unsigned long BUFF_SIZE = 4096;
		unsigned char* locBuff = new unsigned char[BUFF_SIZE];		//Deleted this function.
		while (!mFile.eof()) {
			mFile.read((char*)locBuff, BUFF_SIZE);
			mOggDemux->feed((const unsigned char*)locBuff, mFile.gcount());
		}
		delete[] locBuff;

		mFile.close();
		mIsEnabled = true;
		
	} else {
		mIsEnabled = false;
	}
	return true;
}
OggGranuleSeekTable::tSeekPair AutoOggChainGranuleSeekTable::seekPos(LOOG_INT64 inTime)
{
	unsigned long retEarliestPos = 4294967295UL;

	LOOG_INT64 locStreamTime = -1;
	bool locGotAValidPos = false;


	OggGranuleSeekTable::tSeekPair locSeekInfo;
	OggGranuleSeekTable::tSeekPair retBestSeekInfo;
	for (size_t i = 0; i < mStreamMaps.size(); i++) {

		if ((mStreamMaps[i].mSeekTable != NULL) && (mStreamMaps[i].mSeekInterface != NULL)) {
			//Get the preliminary seek info
			locSeekInfo = mStreamMaps[i].mSeekTable->getStartPos(inTime);
			//1. Get the granule pos in the preliminary seek
			//2. Ask the seek interface what granule we must seek before to make this a valid seek
			//		ie if preroll or keyframes, this value must be less than the original seek value
			//3. Convert the new granule to time
			//4. Repeat the seek
			locStreamTime = mStreamMaps[i].mSeekInterface->convertGranuleToTime(mStreamMaps[i].mSeekInterface->mustSeekBefore(locSeekInfo.second.second));
			locSeekInfo = mStreamMaps[i].mSeekTable->getStartPos(locStreamTime);

			if (retEarliestPos >= locSeekInfo.second.first) {
				//Update the earliest position
				retEarliestPos = locSeekInfo.second.first;
				retBestSeekInfo = locSeekInfo;
				locGotAValidPos = true;
			}
		}
	}	

	return retBestSeekInfo;//retEarliestPos;

}
LOOG_INT64 AutoOggChainGranuleSeekTable::fileDuration()
{
	return mDuration;
}
bool AutoOggChainGranuleSeekTable::acceptOggPage(OggPage* inOggPage)
{
	LOOG_INT64 locGranule = inOggPage->header()->GranulePos();
	unsigned long locSerialNo = inOggPage->header()->StreamSerialNo();
	sStreamMapping locMapping = getMapping(locSerialNo);

	//Exclude pages, with -1 granule pos, or that have only 1 packet and that packet is incomplete
	if ((locGranule != -1) && (!((inOggPage->numPackets() <= 1) && (inOggPage->header()->isContinuation())))) {
		LOOG_INT64 locRealTime = -1;
		if ((locMapping.mSeekInterface != NULL) && (locMapping.mSeekTable != NULL)) {
			//There is valid stream info
			locRealTime = locMapping.mSeekInterface->convertGranuleToTime(locGranule);
			if (locRealTime >= 0) {
				locMapping.mSeekTable->addSeekPoint(locRealTime, mFilePos, locGranule);
				if (locRealTime > mDuration) {
					mDuration = locRealTime;
				}
			}
		}
	}
	mFilePos += inOggPage->pageSize();

	delete inOggPage;
	return true;
}
AutoOggChainGranuleSeekTable::sStreamMapping AutoOggChainGranuleSeekTable::getMapping(unsigned long inSerialNo)
{
	for (size_t i = 0; i < mStreamMaps.size(); i++) {
		if (mStreamMaps[i].mSerialNo == inSerialNo) {
			return mStreamMaps[i];
		}
	}

	sStreamMapping retMapping;
	retMapping.mSeekInterface = NULL;
	retMapping.mSeekTable = NULL;
	retMapping.mSerialNo = 0;

	return retMapping;
}
bool AutoOggChainGranuleSeekTable::addStream(unsigned long inSerialNo, IOggDecoderSeek* inSeekInterface)
{
	sStreamMapping locMapping;
	locMapping.mSerialNo = inSerialNo;
	locMapping.mSeekInterface = inSeekInterface;
	if (inSeekInterface == NULL) {
		locMapping.mSeekTable = NULL;
	} else {
		locMapping.mSeekTable = new OggGranuleSeekTable;
	}
	mStreamMaps.push_back(locMapping);

	return true;

}