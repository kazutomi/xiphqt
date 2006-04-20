#include "StdAfx.h"
#include "SpeexDecoder.h"

SpeexDecoder::SpeexDecoder(void)
	:	mPacketCount(0)
	,	mFrameSize(0)
	,	mNumChannels(0)
	,	mSampleRate(0)
	,	mNumFrames(0)
	,	mNumExtraHeaders(0)
	,	mIsVBR(false)
	,	mSpeexState(NULL)
	,	mStereoState(NULL)
{
}

SpeexDecoder::~SpeexDecoder(void)
{
}

bool SpeexDecoder::setDecodeParams(SpeexDecodeSettings inSettings)
{
	if (mPacketCount == 0) {
		mDecoderSettings = inSettings;
		return true;
	}
	return false;
}

bool SpeexDecoder::decodePacket(StampedOggPacket* inPacket, short* outSamples, unsigned long inBufferSize)
{
	if (mPacketCount == 0) {
		mPacketCount++;
		return decodeHeader(inPacket);
	} else if (mPacketCount == 1) {
		//Comment
		mPacketCount++;
		return true;
	} else if (mPacketCount < 2+mNumExtraHeaders) {
		//Ignore
		mPacketCount++;
		return true;
	} else {
		mPacketCount++;

		speex_bits_read_from(&mSpeexBits, (char*)inPacket->packetData(), inPacket->packetSize());
		
		for (int frame = 0; frame < mNumFrames; frame++) {
			int locRet = speex_decode_int(mSpeexState, &mSpeexBits, outSamples);

			if (locRet == -1) {
				break;
			} else if (locRet == -2) {
				//Corrupted
				return false;
			}

			if (speex_bits_remaining(&mSpeexBits) < 0) {
				//Corrupted
				return false;
			}


			if (mNumChannels == 2) {
				speex_decode_stereo_int(outSamples, mFrameSize, mStereoState);
			}
		}
		return true;

		
	}
}

bool SpeexDecoder::decodeHeader(StampedOggPacket* inPacket)
{

	SpeexHeader* locSpeexHeader = NULL;
	int locModeID = 0;
	const SpeexMode* locMode = NULL;
	void* locState = NULL;
	SpeexCallback locCallback;

	locSpeexHeader = speex_packet_to_header((char*)inPacket->packetData(), inPacket->packetSize());

	if (locSpeexHeader == NULL) {
		//Can't read header
		return false;
	}

	//Check modes?
	locModeID = locSpeexHeader->mode;

	locMode = speex_lib_get_mode(locModeID);

	if (locSpeexHeader->speex_version_id > 1) {
		//Invalid version
		return false;
	}

	//TODO::: Other bitstream version checks

	locState = speex_decoder_init(locMode);

	if (locState == NULL) {
		//Init failed
		return false;
	}

	speex_decoder_ctl(locState, SPEEX_SET_ENH, &mDecoderSettings.mPerceptualEnhancement);
	speex_decoder_ctl(locState, SPEEX_GET_FRAME_SIZE, &mFrameSize);


	if (mDecoderSettings.mForceChannels == SpeexDecodeSettings::SPEEX_CHANNEL_FORCE_STEREO) {
		locCallback.callback_id = SPEEX_INBAND_STEREO;
		locCallback.func = speex_std_stereo_request_handler;
		locCallback.data = mStereoState;
		speex_decoder_ctl(locState, SPEEX_SET_HANDLER, &locCallback);
	}

	//TODO::: Apply rate forces
	mSampleRate = locSpeexHeader->rate;

	speex_decoder_ctl(locState, SPEEX_SET_SAMPLING_RATE, &mSampleRate);

	mNumFrames = locSpeexHeader->frames_per_packet;

	if (mDecoderSettings.mForceChannels == SpeexDecodeSettings::SPEEX_CHANNEL_LEAVE_ALONE) {
		mNumChannels = locSpeexHeader->nb_channels;
	}

	mIsVBR = (locSpeexHeader->vbr != 0);

	mNumExtraHeaders = locSpeexHeader->extra_headers;

	free(locSpeexHeader);
	mSpeexState = locState;

	speex_bits_init(&mSpeexBits);

	return true;


}