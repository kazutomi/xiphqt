#pragma once

#include <libOOOgg/IStampedOggPacketSink.h>
class TestPatternOggPacketChecker
	:	public IStampedOggPacketSink
{
public:
	TestPatternOggPacketChecker(void);
	~TestPatternOggPacketChecker(void);

	//IStampedOggPacketSink
	virtual bool acceptStampedOggPacket(StampedOggPacket* inPacket);
};
