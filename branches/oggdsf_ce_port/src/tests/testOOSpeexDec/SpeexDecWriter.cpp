#include "StdAfx.h"
#include "SpeexDecWriter.h"

SpeexDecWriter::SpeexDecWriter(string inFilename)
{
	mPacketiser.setPacketSink(this);
	mOutputBuffer = new short[OUTPUT_BUFFER_SIZE];
	mOutputFile.open(inFilename.c_str(), ios_base::out | ios_base::binary);
}

SpeexDecWriter::~SpeexDecWriter(void)
{
	mOutputFile.close();
	delete[] mOutputBuffer;
}

bool SpeexDecWriter::acceptStampedOggPacket(StampedOggPacket* inPacket)
{
	bool retVal = mSpeexDecoder.decodePacket(inPacket, mOutputBuffer, OUTPUT_BUFFER_SIZE);
	mOutputFile.write((char*)mOutputBuffer, mSpeexDecoder.frameSize() * mSpeexDecoder.numChannels() * sizeof(short));
	return retVal;
}
bool SpeexDecWriter::acceptOggPage(OggPage* inOggPage)
{
	return mPacketiser.acceptOggPage(inOggPage);
}