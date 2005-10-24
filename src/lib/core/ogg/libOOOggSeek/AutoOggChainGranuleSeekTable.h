#pragma once

#include <libOOOgg/libOOOgg.h>
#include "OggGranuleSeekTable.h"
#include "IOggDecoderSeek.h"


class LIBOOOGGSEEK_API AutoOggChainGranuleSeekTable
	:	public IOggCallback
{
public:
	AutoOggChainGranuleSeekTable(string inFilename);
	virtual ~AutoOggChainGranuleSeekTable(void);

	/// Builds the actual seek table: only works if we have random access to the file.
	virtual bool buildTable();

	//IOggCallback interface
	virtual bool acceptOggPage(OggPage* inOggPage);

	/// The duration of the file, in DirectShow time units.
	LOOG_INT64 fileDuration();

	bool addStream(unsigned long inSerialNo, IOggDecoderSeek* inSeekInterface);

	unsigned long seekPos(LOOG_INT64 inTime);
protected:

	struct sStreamMapping {
		unsigned long mSerialNo;
		IOggDecoderSeek* mSeekInterface;
		OggGranuleSeekTable* mSeekTable;
	};

	vector<sStreamMapping> mStreamMaps;

	sStreamMapping getMapping(unsigned long inSerialNo);
	fstream mFile;
	string mFilename;
	unsigned long mFilePos;
	OggDataBuffer* mOggDemux;
};
