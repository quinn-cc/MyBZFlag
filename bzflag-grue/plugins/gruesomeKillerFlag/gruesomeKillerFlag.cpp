/*
 * Custom flag: Gruesome Killer (+GK)
 * Kills explode in a Shock Wave. Kills from the Shock Wave also explode in a
 * shock wave.
 * 
 * Server Variables (default):
 *  _gruesomeKillerBlossomCount (12): Number of shots in the death blossom
 * 	explosion at ground levelS
 * 
 * Requires:
 * - Grue's BZFS
 * - ShotAssigner plugin
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=gruesomeKillerFlag
 */
 
#include "bzfsAPI.h"
#include <math.h>
using namespace std;

class GruesomeKillerFlag : public bz_Plugin
{
	virtual const char* Name()
	{
		return "Gruesome Killer Flag";
	}

	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~GruesomeKillerFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(GruesomeKillerFlag)

void GruesomeKillerFlag::Init(const char*)
{	
	bz_RegisterCustomFlag("GK", "Gruesome Killer", "Kills explode in a shock wave and shrapnel.", 0, eGoodFlag);
	bz_registerCustomBZDBInt("_gruesomeKillerBlossomCount", 12);
	bz_registerCustomBZDBDouble("_gruesomeKillerShockwaveLifetime", 3);
	bz_registerCustomBZDBDouble("_gruesomeKillerShrapnelLifetime", 4);
	Register(bz_ePlayerDieEvent);
}

/*
  This function is used in the detonate function below. The bool "up" indicates
  whether the shots fire upward at the 45 degree angle.
*/
void fireShot(float playerPos[3], int killerID, float rotation, bool up)
{
	float vel[3] = { cos(rotation), sin(rotation), 0 };
	float pos[3] = { playerPos[0], playerPos[1], (float)bz_getBZDBDouble("_muzzleHeight") };
	
	if (up)
	{
		vel[2] = sqrt(2)/2.0;
		vel[0] *= sqrt(2)/2.0;
		vel[1] *= sqrt(2)/2.0;
	}
	
	bz_fireServerShotAsPlayer("GK", pos, vel, "GK", killerID,
			bz_getBZDBDouble("_gruesomeKillerShrapnelLifetime"));
}

/*
  This function detonates the gruesome killer explosion effect. There are a ring
  of death blossom shots that explode on the ground, a second ring that fire at a
  45 degree angle, one singluar shot upward, and shock wave. All of these shots
  can cascade and cause more gruesome killer detonations.
*/
void detonate(float playerPos[3], float rotation, int killerID)
{
	// Shock wave
	float vel[3] = { 0, 0, 0 };
  	bz_fireServerShotAsPlayer("SW", playerPos, vel, "GK", killerID,
			bz_getBZDBDouble("_gruesomeKillerShockwaveLifetime"));

	// Ground-level death blossom
	for (int i = 0; i < bz_getBZDBInt("_gruesomeKillerBlossomCount"); i++)
	{
		fireShot(playerPos, killerID, i*((2*3.142)/bz_getBZDBInt("_gruesomeKillerBlossomCount")) + rotation, false);
	}
	
	// Death blossom at 45 degree angle
	int upCount = bz_getBZDBInt("_gruesomeKillerBlossomCount")/sqrt(2);
	for (int i = 0; i < upCount; i++)
	{
		fireShot(playerPos, killerID, i*((2*3.142)/upCount) + rotation, true);
	}
	
	// Shot going directly upward
	vel[0] = 0;
	vel[1] = 0;
	vel[2] = 1;
	bz_fireServerShotAsPlayer("GK", playerPos, vel, "GK", killerID,
			bz_getBZDBDouble("_gruesomeKillerShrapnelLifetime"));
}


void GruesomeKillerFlag::Event(bz_EventData *eventData)
{
 	if (eventData->eventType == bz_ePlayerDieEvent)
 	{
   		bz_PlayerDieEventData_V2* data = (bz_PlayerDieEventData_V2*) eventData;
    	uint32_t shotGUID = bz_getShotGUID(data->shotID, BZ_SERVER);

    	if (data->flagKilledWith == "GK")
		{
			bz_ApiString flagHeldWhenKilled = bz_getFlagName(data->flagHeldWhenKilled);
			// If we have Avenger, detonate our killer
			if (flagHeldWhenKilled == "AV")
			{
				bz_BasePlayerRecord *killerRecord = bz_getPlayerByIndex(data->killerID);
				
				if (killerRecord)
				{
					detonate(killerRecord->lastKnownState.pos, killerRecord->lastKnownState.rotation, data->playerID);
				}
				
				bz_freePlayerRecord(killerRecord);
			}
			// Otherwise, detonate ourselves
			else
			{
				detonate(data->state.pos, data->state.rotation, data->killerID);
			}
		}
 	}
}





















