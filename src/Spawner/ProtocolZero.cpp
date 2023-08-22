#include "ProtocolZero.h"
#include "ProtocolZero.LatencyLevel.h"

#include "Spawner.h"
#include <Ext/Event/Body.h>
#include <Utilities/Debug.h>
#include <HouseClass.h>
#include <SessionClass.h>
#include <IPXManagerClass.h>

bool ProtocolZero::Enable = false;
int ProtocolZero::WorstMaxAhead = 24;
unsigned char ProtocolZero::MaxLatencyLevel = 0xff;

void ProtocolZero::SendResponseTime2()
{
	if (SessionClass::IsSingleplayer())
		return;

	static int NextSendFrame = 6 * SendResponseTimeInterval;
	int currentFrame = Unsorted::CurrentFrame;

	if (NextSendFrame >= currentFrame)
		return;

	const int ipxResponseTime = IPXManagerClass::Instance->ResponseTime();
	if (ipxResponseTime <= -1)
		return;

	EventExt event;
	event.Type = EventTypeExt::ResponseTime2;
	event.HouseIndex = (char)HouseClass::CurrentPlayer->ArrayIndex;
	event.Frame = currentFrame + Game::Network::MaxAhead;
	event.ResponseTime2.MaxAhead = (int8_t)ipxResponseTime + 1;
	event.ResponseTime2.LatencyLevel = (uint8_t)LatencyLevel::FromResponseTime((uint8_t)ipxResponseTime);

	if (event.AddEvent())
	{
		NextSendFrame = currentFrame + SendResponseTimeInterval;
		Debug::Log("[Spawner] Player %d sending response time of %d, LatencyMode = %d, Frame = %d\n"
			, event.HouseIndex
			, event.ResponseTime2.MaxAhead
			, event.ResponseTime2.LatencyLevel
			, currentFrame
		);
	}
	else
	{
		++NextSendFrame;
	}
}

void ProtocolZero::HandleResponseTime2(EventExt* event)
{
	if (ProtocolZero::Enable == false || SessionClass::IsSingleplayer())
		return;

	if (event->ResponseTime2.MaxAhead == 0)
	{
		Debug::Log("[Spawner] Returning because event->MaxAhead == 0\n");
		return;
	}

	static int32_t PlayerMaxAheads[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	static uint8_t PlayerLatencyMode[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	static int32_t PlayerLastTimingFrame[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	int32_t houseIndex = event->HouseIndex;
	PlayerMaxAheads[houseIndex] = (int32_t)event->ResponseTime2.MaxAhead;
	PlayerLatencyMode[houseIndex] = event->ResponseTime2.LatencyLevel;
	PlayerLastTimingFrame[houseIndex] = event->Frame;

	uint8_t setLatencyMode = 0;
	int maxMaxAheads = 0;

	for (char i = 0; i < 8; ++i)
	{
		if (Unsorted::CurrentFrame >= (PlayerLastTimingFrame[i] + (SendResponseTimeInterval * 4)))
		{
			PlayerMaxAheads[i] = 0;
			PlayerLatencyMode[i] = 0;
		}
		else
		{
			maxMaxAheads = PlayerMaxAheads[i] > maxMaxAheads ? PlayerMaxAheads[i] : maxMaxAheads;
			if (PlayerLatencyMode[i] > setLatencyMode)
				setLatencyMode = PlayerLatencyMode[i];
		}
	}

	ProtocolZero::WorstMaxAhead = maxMaxAheads;
	LatencyLevel::Apply(setLatencyMode);
}
