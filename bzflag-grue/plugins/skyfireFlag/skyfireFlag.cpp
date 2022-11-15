/*
 * Custom flag: SkyFire (+SF)
 * Firing triggers a hail of missiles from the sky falling in front of your tank.
 *  
 * Server variables (default):
 * _skyfireHeight (50): Height at which the missiles start
 * _skyfireRadius (30): Radius of which the missiles spawn in
 * _skyfireCount (10): Number of missiles
 * _skyfireVertSpeedAdVel (1): Multiplied by normal shot speed to determine
 *  speed the missiles come down at
 * _skyfireHorizSpeedAdVel (5): Multiplied by tank speed to determine the speed
 *  at which the missiles travel at
 * 
 * Requires:
 * - Grue's BZFS
 * - ShotAssigner plugin
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=skyfireFlag
 */

#include "bzfsAPI.h"
#include <math.h>
#include <cstdlib>
#include <ctime>

using namespace std;

class SkyfireFlag : public bz_Plugin {

    virtual const char* Name()
    {
        return "Skyfire Flag";
    }

    virtual void Init(const char*);
    virtual void Event(bz_EventData*);
    ~SkyfireFlag() {};

    virtual void Cleanup(void)
    {
        Flush();
    }
};

BZ_PLUGIN(SkyfireFlag)

void SkyfireFlag::Init(const char*)
{
    bz_RegisterCustomFlag("SF", "Skyfire", "Firing triggers a hail of missiles from the sky falling in front of your tank.", 0, eGoodFlag);
    bz_registerCustomBZDBDouble("_skyfireHeight", 50);		
    bz_registerCustomBZDBDouble("_skyfireRadius", 30);	
    bz_registerCustomBZDBInt("_skyfireCount", 10);		
    bz_registerCustomBZDBDouble("_skyfireVertSpeedAdVel", 1);
    bz_registerCustomBZDBDouble("_skyfireHorizSpeedAdVel", 5);
    Register(bz_eShotFiredEvent);
    Register(bz_ePlayerDieEvent);
}

void SkyfireFlag::Event(bz_EventData *eventData)
{
    if (eventData->eventType == bz_eShotFiredEvent)
    {
        bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
        bz_BasePlayerRecord* player = bz_getPlayerByIndex(data->playerID);
        
        if (player && player->currentFlag == "SkyFire (+SF)")
        {
        	unsigned int seed = (unsigned) data->shotID;
        	std::srand(seed);
        
            // Velocity of missile
            float vel[3];
            vel[0] = (cos(player->lastKnownState.rotation) +
                2*player->lastKnownState.velocity[0]/bz_getBZDBDouble("_tankSpeed"))*bz_getBZDBDouble("_skyfireHorizSpeedAdVel");
            vel[1] = (sin(player->lastKnownState.rotation) +
                2*player->lastKnownState.velocity[1]/bz_getBZDBDouble("_tankSpeed"))*bz_getBZDBDouble("_skyfireHorizSpeedAdVel");
            vel[2] = (-1) * bz_getBZDBDouble("_skyfireVertSpeedAdVel");

            // Fire _skyfireCount number of missiles
		    for (int i = 0; i < bz_getBZDBInt("_skyfireCount"); i++)
		    {		    		    	
		    	double r = bz_randFloatBetween(0, bz_getBZDBDouble("_skyfireRadius"));		// radius from the center
		    	double ang = bz_randFloatBetween(0, 6.283); // 6.283 = 2pi					// angle
		    	
                // Position of missile start
		    	float pos[3];
		    	pos[0] = r*cos(ang) + player->lastKnownState.pos[0];
		    	pos[1] = r*sin(ang) + player->lastKnownState.pos[1];
		    	pos[2] = bz_getBZDBDouble("_skyfireHeight");
		    	
		    	bz_fireServerShotAsPlayer("GM", pos, vel, "SF", player->playerID);
		    }
        }
        
        bz_freePlayerRecord(player);
    }
}

