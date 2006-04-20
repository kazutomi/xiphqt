#pragma once

#include "SpeexDecodeSettings.h"
#include <libOOOgg/dllstuff.h>
#include <libOOOgg/StampedOggPacket.h>
extern "C" {
#include "speex/speex.h"
#include "speex/speex_header.h"
#include "speex/speex_callbacks.h"
#include "speex/speex_stereo.h"
}

class SpeexDecoder
{
public:
	SpeexDecoder(void);
	~SpeexDecoder(void);

	bool setDecodeParams(SpeexDecodeSettings inSettings);
	bool decodePacket(StampedOggPacket* inPacket, short* outSamples, unsigned long inBufferSize); 

	int frameSize()	{	return mFrameSize;	}
	int numChannels()	{	return mNumChannels;	}
protected:
	bool decodeHeader(StampedOggPacket* inPacket);
	unsigned long mPacketCount;

	int mFrameSize;
	int mNumChannels;
	int mSampleRate;
	int mNumFrames;
	int mNumExtraHeaders;
	bool mIsVBR;

	SpeexStereoState* mStereoState;
	SpeexBits mSpeexBits;
	void* mSpeexState;

	SpeexDecodeSettings mDecoderSettings;

};
