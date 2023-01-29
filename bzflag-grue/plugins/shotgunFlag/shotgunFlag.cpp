/*	
 * Custom flag: ShotGun (+SG)
 * Tank shoots a spary of shots, tightly clustered
 * 
 * Server Variables:
 * _shotgunAngle
 * 
 * Requires:
 * - Grue's BZFS
 * - ShotAssigner plugin
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=shotgunFlag
 */
#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <math.h>

using namespace std;

class ShotgunFlag : public bz_Plugin
{
    virtual const char* Name ()
    {
        return "Shotgun Flag";
    }
    virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~ShotgunFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(ShotgunFlag)

void ShotgunFlag::Init(const char*)
{
	bz_RegisterCustomFlag("SG", "Shotgun", "Tank fires a spray of shots tightly clustered.", 0, eGoodFlag);
	bz_registerCustomBZDBDouble("_shotgunAngle", 0.03);
    bz_registerCustomBZDBInt("_shotgunCount", 4);
	Register(bz_eShotFiredEvent);
}

void ShotgunFlag::Event(bz_EventData *eventData)
{
	if (eventData->eventType == bz_eShotFiredEvent)
	{
		bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
		bz_BasePlayerRecord *player = bz_getPlayerByIndex(data->playerID);

		if (bz_getPlayerFlagAbbr(player->playerID) == "SG")
		{
            unsigned int seed = (unsigned) data->shotID;
        	std::srand(seed);

            for (int i = 0; i < bz_getBZDBInt("_shotgunCount"); i++)
            {
                double r = bz_randFloatBetween(0, bz_getBZDBDouble("_shotgunAngle"));
                double ang = bz_randFloatBetween(0, 6.283); // 6.283 = 2pi		
                
                double xAngle = r*cos(ang);
                double yAngle = r*sin(ang);

                float pos[3];
                pos[0] = player->lastKnownState.pos[0] + cos(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
                pos[1] = player->lastKnownState.pos[1] + sin(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
                pos[2] = player->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");

                float vel[3];
                vel[0] = (cos(player->lastKnownState.rotation+xAngle) + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"))*cos(xAngle);
                vel[1] = (sin(player->lastKnownState.rotation+xAngle) + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"))*cos(xAngle);
                vel[2] = sin(yAngle);

                bz_fireServerShotAsPlayer("SG", pos, vel, "SG", player->playerID);
            }
		}
		
		bz_freePlayerRecord(player);
	}
}
				
