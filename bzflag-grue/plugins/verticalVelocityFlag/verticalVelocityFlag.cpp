/*
 * Custom flag: Vertical Velocity (+VV)
 * Extra two shots travel with vertical velocity.
 * 
 * Server Variables:
 * _verticalVelocityWidth (4.0): Distance between the two Vertical Velocity
 * 	shots.
 * 
 * Requires:
 * - Grue's BZFS
 * - ShotAssigner plugin
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=verticalVelocityFlag
 */

#include "bzfsAPI.h"
#include <math.h>

using namespace std;

class VerticalVelocity : public bz_Plugin
{
	virtual const char* Name()
	{
		return "Vertical Velocity Flag";
	}

	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~VerticalVelocity() {};

	virtual void Cleanup(void)
	{
		Flush();
	}

	void applyEffect(int playerID);
	void removeEffect(int playerID);
};

BZ_PLUGIN(VerticalVelocity)

void VerticalVelocity::Init(const char*)
{
	bz_RegisterCustomFlag("VV", "Vertical Velocity", "Shots travel with vertical velocity.", 0, eGoodFlag);
	bz_registerCustomBZDBDouble("_verticalVelocityWidth", 4.0);
	bz_registerCustomBZDBBool("_verticalVelocityUseLegacyVersion", false);
	Register(bz_eShotFiredEvent);
	Register(bz_eFlagTransferredEvent);
	Register(bz_eFlagGrabbedEvent);
	Register(bz_eFlagDroppedEvent);
}

void VerticalVelocity::applyEffect(int playerID)
{
    bz_setServerVariableForPlayer(playerID, "_shotsKeepVerticalVelocity", "1");
}

void VerticalVelocity::removeEffect(int playerID)
{
    bz_setServerVariableForPlayer(playerID, "_shotsKeepVerticalVelocity", "0");
}

void VerticalVelocity::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case bz_eShotFiredEvent:
		{
			if (bz_getBZDBBool("_verticalVelocityUseLegacyVersion"))
			{
				bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
				bz_BasePlayerRecord *player = bz_getPlayerByIndex(data->playerID);

				if (bz_getPlayerFlagAbbr(data->playerID) == "VV")
				{
					float velocity[3]; // VV shots' velocity
					float pos[3];      // VV shots' base position
					float offset[2];   // VV shots' offset from base position
					float pos1[3];     // Position of one VV shot
					float pos2[3];     // Position of the other VV shot
					
					velocity[0] = cos(player->lastKnownState.rotation) + player->lastKnownState.velocity[0] / bz_getBZDBDouble("_shotSpeed");
					velocity[1] = sin(player->lastKnownState.rotation) + player->lastKnownState.velocity[1] / bz_getBZDBDouble("_shotSpeed");
					velocity[2] = player->lastKnownState.velocity[2] / bz_getBZDBDouble("_shotSpeed");
					
					pos[0] = player->lastKnownState.pos[0] + cos(player->lastKnownState.rotation) * bz_getBZDBDouble("_muzzleFront");
					pos[1] = player->lastKnownState.pos[1] + sin(player->lastKnownState.rotation) * bz_getBZDBDouble("_muzzleFront");
					pos[2] = player->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");
					
					offset[0] = -sin(player->lastKnownState.rotation) * bz_getBZDBDouble("_verticalVelocityWidth") / 2.0;
					offset[1] = cos(player->lastKnownState.rotation) * bz_getBZDBDouble("_verticalVelocityWidth") / 2.0;
					
					// Shot 1
					pos1[0] = pos[0] + offset[0];
					pos1[1] = pos[1] + offset[1];
					pos1[2] = pos[2];
					bz_fireServerShotAsPlayer("VV", pos1, velocity, "VV", player->playerID);
					
					// Shot 2
					pos2[0] = pos[0] - offset[0];
					pos2[1] = pos[1] - offset[1];
					pos2[2] = pos[2];
					bz_fireServerShotAsPlayer("VV", pos2, velocity, "VV", player->playerID);
				}
				
				bz_freePlayerRecord(player);
			}
		} break;
		case bz_eFlagGrabbedEvent:
        {
			if (!bz_getBZDBBool("_verticalVelocityUseLegacyVersion"))
			{
				bz_FlagGrabbedEventData_V1* data = (bz_FlagGrabbedEventData_V1*) eventData;
				
				if (strcmp(data->flagType, "VV") == 0)
					applyEffect(data->playerID);
			}
        } break;
        case bz_eFlagDroppedEvent:
        {
			if (!bz_getBZDBBool("_verticalVelocityUseLegacyVersion"))
			{
				bz_FlagGrabbedEventData_V1* data = (bz_FlagGrabbedEventData_V1*) eventData;

				if (strcmp(data->flagType, "VV") == 0)
					removeEffect(data->playerID);
			}
        } break;
        case bz_eFlagTransferredEvent:
        {
			if (!bz_getBZDBBool("_verticalVelocityUseLegacyVersion"))
			{
				bz_FlagTransferredEventData_V1* data = (bz_FlagTransferredEventData_V1*) eventData;

				if (strcmp(data->flagType, "VV") == 0)
				{
					removeEffect(data->fromPlayerID);
					applyEffect(data->toPlayerID);
				}
			}
        } break;
		default:
			break;
	}
}
