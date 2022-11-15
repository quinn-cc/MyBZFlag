/*	
 * Custom flag: Triple Barrel (+TB)
 * Tank shoots a spray of three shots.
 * 
 * Server Variables:
 * _tripleBarrelAngle (0.12): Angle between the middle shot and one of the side
 * 	shots.
 * 
 * Requires:
 * - Grue's BZFS
 * - ShotAssigner plugin
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=tripleBarrelFlag
 */
#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <math.h>

using namespace std;

class TripleBarrelFlag : public bz_Plugin
{
    virtual const char* Name ()
    {
        return "Triple Barrel Flag";
    }
    virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~TripleBarrelFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(TripleBarrelFlag)

void TripleBarrelFlag::Init(const char*) {
	
	bz_RegisterCustomFlag("TB", "Triple Barrel", "Tank shoots a spray of three shots.", 0, eGoodFlag);
	bz_registerCustomBZDBDouble("_tripleBarrelAngle", 0.12);
	Register(bz_eShotFiredEvent);
}

void TripleBarrelFlag::Event(bz_EventData *ed)
{
	if (ed->eventType == bz_eShotFiredEvent)
	{
		bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) ed;
		bz_BasePlayerRecord *player = bz_getPlayerByIndex(data->playerID);

		if (bz_getPlayerFlagAbbr(player->playerID) == "TB")
		{
			float pos[3];
			pos[0] = player->lastKnownState.pos[0] + cos(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
			pos[1] = player->lastKnownState.pos[1] + sin(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
			pos[2] = player->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");
			
			// Left shot
			float vel1[3];
			vel1[0] = cos(player->lastKnownState.rotation - bz_getBZDBDouble("_tripleBarrelAngle")) +
				player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed");
			vel1[1] = sin(player->lastKnownState.rotation - bz_getBZDBDouble("_tripleBarrelAngle")) +
				player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed");
			vel1[2] = 0;
			bz_fireServerShotAsPlayer("TB", pos, vel1, "TB", player->playerID);
			
			// Right shot
			float vel2[3];
			vel2[0] = cos(player->lastKnownState.rotation + bz_getBZDBDouble("_tripleBarrelAngle")) +
				player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed");
			vel2[1] = sin(player->lastKnownState.rotation + bz_getBZDBDouble("_tripleBarrelAngle")) +
				player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed");
			vel2[2] = 0;
			bz_fireServerShotAsPlayer("TB", pos, vel2, "TB", player->playerID);
		}
		
		bz_freePlayerRecord(player);
	}
}
