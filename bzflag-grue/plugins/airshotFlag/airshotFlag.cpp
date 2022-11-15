/*	
 * Custom flag: AirshoT (+AT)
 * Tank shoots an extra two shots at an angle upward.
 * 
 * Server Variables:
 * _airshotAngle - controls the angle at which the extra bullets shoot upward.
 * 
 * Requires:
 * - Grue's BZFS
 * - ShotAssigner plugin
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=airshotFlag
 */
#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <math.h>

using namespace std;

class AirshotFlag : public bz_Plugin
{
    virtual const char* Name ()
    {
        return "Airshot Flag";
    }
    virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~AirshotFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(AirshotFlag)

void AirshotFlag::Init(const char*)
{
	bz_RegisterCustomFlag("AT", "Airshot", "Extra two shots at an angle upward.", 0, eGoodFlag);
	bz_registerCustomBZDBDouble("_airshotAngle", 0.12);
	Register(bz_eShotFiredEvent);
}

void AirshotFlag::Event(bz_EventData *eventData)
{
	if (eventData->eventType == bz_eShotFiredEvent)
	{
		bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
		bz_BasePlayerRecord *player = bz_getPlayerByIndex(data->playerID);

		if (bz_getPlayerFlagAbbr(player->playerID) == "AT")
		{
			float pos[3];
			pos[0] = player->lastKnownState.pos[0] + cos(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
			pos[1] = player->lastKnownState.pos[1] + sin(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
			pos[2] = player->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");
			
			// Middle shot
			float vel1[3];
			vel1[0] = (cos(player->lastKnownState.rotation) + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"))*cos(bz_getBZDBDouble("_airshotAngle"));
			vel1[1] = (sin(player->lastKnownState.rotation) + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"))*cos(bz_getBZDBDouble("_airshotAngle"));
			vel1[2] = sin(bz_getBZDBDouble("_airshotAngle"));
			bz_fireServerShotAsPlayer("AT", pos, vel1, "AT", player->playerID);
			
			// Top shot
			float vel2[3];
			vel2[0] = (cos(player->lastKnownState.rotation) + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"))*cos(bz_getBZDBDouble("_airshotAngle")*2);
			vel2[1] = (sin(player->lastKnownState.rotation) + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"))*cos(bz_getBZDBDouble("_airshotAngle")*2);
			vel2[2] = sin(bz_getBZDBDouble("_airshotAngle")*2);
			bz_fireServerShotAsPlayer("AT", pos, vel2, "AT", player->playerID);
		}
		
		bz_freePlayerRecord(player);
	}
}
				
