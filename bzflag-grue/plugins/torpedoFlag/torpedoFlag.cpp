/*
 * Custom flag: Torpedo (+TR)
 * Extra Super Bullet shots fires at ground level
 * 
 * Server Variables:
 * _torpedoWidth (4.0): Distance between the two Vertical Velocity
 * 	shots.
 * _torpedoAdVel (1): Multiplied by the normal shot speed to determine torpedo
 *  shot speed.
 * _torpedoLifetime (4): Lifetime the torpedo lasts.
 * _torpedoHeight (0.1): Height off the ground the torpedo fires at
 * 
 * Requires:
 * - Grue's BZFS
 * - ShotAssigner plugin
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=torpedoFlag
 */

#include "bzfsAPI.h"
#include <math.h>

using namespace std;

class TorpedoFlag : public bz_Plugin
{
	virtual const char* Name()
	{
		return "Torpedo Flag";
	}

	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~TorpedoFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(TorpedoFlag)

void TorpedoFlag::Init(const char*)
{
	bz_RegisterCustomFlag("TO", "Torpedo", "Extra Super Bullet shots fire at ground level.", 0, eGoodFlag);
	bz_registerCustomBZDBDouble("_torpedoWidth", 3.0);
    bz_registerCustomBZDBDouble("_torpedoAdVel", 1.0);
    bz_registerCustomBZDBDouble("_torpedoLifetime", 4);
    bz_registerCustomBZDBDouble("_torpedoHeight", 0.1);
	Register(bz_eShotFiredEvent);
}

void TorpedoFlag::Event(bz_EventData *eventData)
{
	if (eventData->eventType == bz_eShotFiredEvent)
	{
		bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
		bz_BasePlayerRecord *player = bz_getPlayerByIndex(data->playerID);

		if (bz_getPlayerFlagAbbr(data->playerID) == "TO")
		{
			float velocity[3]; // TO shots' velocity
			float pos[3];      // TO shots' base position
			float offset[2];   // TO shots' offset from base position
			float pos1[3];     // Position of one TO shot
			float pos2[3];     // Position of the other TO shot
			
			velocity[0] = cos(player->lastKnownState.rotation) + player->lastKnownState.velocity[0] / bz_getBZDBDouble("_shotSpeed");
			velocity[1] = sin(player->lastKnownState.rotation) + player->lastKnownState.velocity[1] / bz_getBZDBDouble("_shotSpeed");
			velocity[2] = 0;

            velocity[0] *= bz_getBZDBDouble("_torpedoAdVel");
            velocity[1] *= bz_getBZDBDouble("_torpedoAdVel");
			
			pos[0] = player->lastKnownState.pos[0] + cos(player->lastKnownState.rotation) * bz_getBZDBDouble("_muzzleFront");
			pos[1] = player->lastKnownState.pos[1] + sin(player->lastKnownState.rotation) * bz_getBZDBDouble("_muzzleFront");
			pos[2] = bz_getBZDBDouble("_torpedoHeight");
			
			offset[0] = -sin(player->lastKnownState.rotation) * bz_getBZDBDouble("_torpedoWidth") / 2.0;
			offset[1] = cos(player->lastKnownState.rotation) * bz_getBZDBDouble("_torpedoWidth") / 2.0;
			
			// Shot 1
			pos1[0] = pos[0] + offset[0];
			pos1[1] = pos[1] + offset[1];
			pos1[2] = pos[2];
			bz_fireServerShotAsPlayer("SB", pos1, velocity, "TO", player->playerID, bz_getBZDBDouble("_torpedoLifetime"));
			
			// Shot 2
			pos2[0] = pos[0] - offset[0];
			pos2[1] = pos[1] - offset[1];
			pos2[2] = pos[2];
			bz_fireServerShotAsPlayer("SB", pos2, velocity, "TO", player->playerID, bz_getBZDBDouble("_torpedoLifetime"));
		}
		
		bz_freePlayerRecord(player);
	}
}
