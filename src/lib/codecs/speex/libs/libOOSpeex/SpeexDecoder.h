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

	enum eSpeexResult {
		SPEEX_DATA_OK,
		SPEEX_HEADER_OK,
		SPEEX_COMMENT_OK,
		SPEEX_EXTRA_HEADER_OK,
		SPEEX_BAD_HEADER = 64,
		SPEEX_CORRUPTED_BITSTREAM,
		SPEEX_CORRUPTED_UNDERFLOW,
		SPEEX_INVALID_SPEEX_VERSION,
		SPEEX_INITIALISATION_FAILED,

	};

	bool setDecodeParams(SpeexDecodeSettings inSettings);
	eSpeexResult decodePacket(StampedOggPacket* inPacket, short* outSamples, unsigned long inBufferSize); 

	int frameSize()	{	return mFrameSize;	}
	int numChannels()	{	return mNumChannels;	}
protected:
	eSpeexResult decodeHeader(StampedOggPacket* inPacket);
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
