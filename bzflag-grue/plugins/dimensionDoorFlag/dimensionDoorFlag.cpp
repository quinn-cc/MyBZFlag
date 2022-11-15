/*
 * Custom flag: Dimension Door (+DD)
 * First shot fires the portal, second shot teleports you there.
 * - The player world weapon shots make use of metadata 'type' and 'owner'. Type is
 *   is GN and owner is the playerID.
 * - This plugin's structure is almost copied from my Grenade plugin's structure.
 * - As of currently, the flag cannot detect if you are going to teleport inside a building
 *   The player will just end up sealed.
 * 
 * Server Variables:
 * _dimensionDoorAdVel (4.0): Multiplied by normal shot speed to determine speed
 * _dimensionDoorVerticalVelocity (true): Whether or not the portal use vertical
 * 	velocity
 * _dimensionDoorWidth (2.0): distance from middle shot to side grenade PZ shot
 * _dimensionDoorCooldownTime (0.3): The time between teleporting and receiving
 *  the DD flag again. This value should generally not be changed. If the
 * 	server has particuarly bad connection, this may need to be increased.
 * _dimensionDoorLifetime (4.0): The amount in seconds the portal lasts.
 * 
 * Requires:
 * - Grue's BZFS
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=dimensionDoorFlag
 */
 
#include "bzfsAPI.h"
#include <math.h>
#include <map>
using namespace std;

/*
 * Each player has exactly one Portal object assigned to them. The Portal
 * object is created once when they join the game and deleted when they leave.
 * The object is made active when a portal is shot, and the object is made
 * inactive when the portal has been locked or expired.
 * 
 * Since moving players is impossible, this plugin kills the player and
 * relocates them in the new position. Since this may have delay due to
 * connnection, the Portal is "locked" before they teleport so that it doesn't
 * move by the time they need to spawn there.
 */
class Portal
{
private:
	bool active = false;
	float origin[3];
	float velocity[3];
	double initialTime;
	
	float lockedPos[3];
	float lockedRot;
	
public:
	bool spawned = true;
	bool locked = false;
	double lockTime;
	Portal() {};
	void init(float*, float*, uint32_t, uint32_t);
	void clear();
	bool isActive();
	
	float* calculatePosition();
	bool isExpired();
	void lock(int);
	float* getLockedPos();
	float getLockedRot();
	uint32_t* pzShots = new uint32_t[2];
};

void Portal::init(float* pos, float* vel, uint32_t pz1, uint32_t pz2)
{
	active = true;
	origin[0] = pos[0];
	origin[1] = pos[1];
	origin[2] = pos[2];
	velocity[0] = vel[0];
	velocity[1] = vel[1];
	velocity[2] = vel[2];
	initialTime = bz_getCurrentTime();
	pzShots[0] = pz1;
	pzShots[1] = pz2;
}

void Portal::clear()
{
	active = false;
	locked = false;
}

/*
 * Locks the portal down and tracks its current position.
 */
void Portal::lock(int playerID)
{
	bz_endServerShot(pzShots[0], false);
	bz_endServerShot(pzShots[1], false);
	bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(playerID);

	float* pos = calculatePosition();
	lockedPos[0] = pos[0];
	lockedPos[1] = pos[1];
	lockedPos[2] = pos[2];
	lockedRot = playerRecord->lastKnownState.rotation;

	bz_freePlayerRecord(playerRecord);

	locked = true;
	spawned = false;
	lockTime = bz_getCurrentTime();
}

float* Portal::getLockedPos()
{
	return lockedPos;
}
float Portal::getLockedRot()
{
	return lockedRot;
}

bool Portal::isActive()
{
	return active;
}

/*
 * This method calculates the position of where the Portal should be if it
 * continues on its projected trajectory
*/
float* Portal::calculatePosition()
{
	double elapsedTime = bz_getCurrentTime() - initialTime;
	float* pos = new float[3];
	
	pos[0] = origin[0] + velocity[0]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	pos[1] = origin[1] + velocity[1]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	pos[2] = origin[2] + velocity[2]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	
	return pos;
}

bool Portal::isExpired()
{
	bool expired = false;

	// If the shot is below the ground or has exceeded its lieftime, then it is
	// expired.
	if (calculatePosition()[2] <= 0 ||
		bz_getCurrentTime() - initialTime > bz_getBZDBDouble("_dimensionDoorLifetime"))
		expired = true;
	
	return expired;
}

class DimensionDoorFlag : public bz_Plugin
{
	virtual const char* Name()
	{
		return "Dimension Door Flag";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~DimensionDoorFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
	
private:
    map<int, Portal*> portalMap; // playerID, Grenade
};

BZ_PLUGIN(DimensionDoorFlag)

void DimensionDoorFlag::Init(const char*)
{
	bz_RegisterCustomFlag("DD", "Dimension Door", "First shot fires the portal, second shot teleports you there.", 0, eGoodFlag);
	
	bz_registerCustomBZDBDouble("_dimensionDoorAdVel", 4.0);
	bz_registerCustomBZDBDouble("_dimensionDoorVerticalVelocity", true);
	bz_registerCustomBZDBDouble("_dimensionDoorWidth", 2.0);
	bz_registerCustomBZDBDouble("_dimensionDoorCooldownTime", 0.3);
	bz_registerCustomBZDBDouble("_dimensionDoorLifetime", 4);
	
	Register(bz_eShotFiredEvent);
	Register(bz_ePlayerJoinEvent);
	Register(bz_ePlayerPartEvent);
	Register(bz_eGetPlayerSpawnPosEvent);
	Register(bz_ePlayerSpawnEvent);
	Register(bz_ePlayerUpdateEvent);
}

void DimensionDoorFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case bz_eShotFiredEvent:
		{
			bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
			bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(data->playerID);
			
			if (playerRecord && data->type == "DD")
			{
				// If an active portal is expired, clear it from the records.
				if (portalMap[data->playerID]->isActive())
					if (portalMap[data->playerID]->isExpired())
						portalMap[data->playerID]->clear();
			
				// If this player does not have an active portal, we can launch
				// one.
				if (portalMap[data->playerID]->isActive() == false)
				{
					float vel[3];		// PZ shot's velocity
					float pos[3];      // PZ shot's base position
					float offset[2];   // PZ shot's offset from base position
					float pos1[3];     // Position of one PZ shot
					float pos2[3];     // Position of the other PZ shot
					
					// Base/center position of the two PZ shots
					pos[0] = playerRecord->lastKnownState.pos[0] + cos(playerRecord->lastKnownState.rotation)*4;
					pos[1] = playerRecord->lastKnownState.pos[1] + sin(playerRecord->lastKnownState.rotation)*4;
					pos[2] = playerRecord->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");
					
					// The offset of the PZ shots
					offset[0] = -sin(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_dimensionDoorWidth");
					offset[1] = cos(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_dimensionDoorWidth");
					
					// Velocity of the PZ shots
					vel[0] = cos(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_dimensionDoorAdVel");
					vel[1] = sin(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_dimensionDoorAdVel");
					vel[2] = 0;
					
					// If the vertical velocity option is turned on, then apply
					// it to the PZ shots.
					if (bz_getBZDBBool("_dimensionDoorVerticalVelocity"))
						vel[2] = playerRecord->lastKnownState.velocity[2]/bz_getBZDBDouble("_shotSpeed");
					
					// PZ shot 1
					pos1[0] = pos[0] + offset[0];
					pos1[1] = pos[1] + offset[1];
					pos1[2] = pos[2];
					uint32_t shot1 = bz_fireServerShotAsPlayer("PZ", pos1, vel, "DD", playerRecord->playerID,
						bz_getBZDBDouble("_dimensionDoorLifetime"));
					
					// PZ shot 2
					pos2[0] = pos[0] - offset[0];
					pos2[1] = pos[1] - offset[1];
					pos2[2] = pos[2];
					uint32_t shot2 = bz_fireServerShotAsPlayer("PZ", pos2, vel, "DD", playerRecord->playerID,
						bz_getBZDBDouble("_dimensionDoorLifetime"));
					
					portalMap[data->playerID]->init(pos, vel, shot1, shot2);
				}
				// If not, then we teleport there.
				else
				{
					float* pos = portalMap[data->playerID]->calculatePosition();
					float spawnPos[3];
					spawnPos[0] = pos[0];
					spawnPos[1] = pos[1];
					spawnPos[2] = pos[2];
					
					// Make sure its within world boundaries
					if (!bz_isWithinWorldBoundaries(spawnPos))
					{
						bz_sendTextMessage(BZ_SERVER, data->playerID, "You cannot teleport outside the map.");
						portalMap[data->playerID]->clear();
					}
					else
					{
						bz_killPlayer(data->playerID, false, BZ_SERVER, "DD");
						bz_incrementPlayerLosses(data->playerID, -1);
						portalMap[data->playerID]->lock(data->playerID);
						bz_forcePlayerSpawn(data->playerID);
					}
				}
			}
			
			bz_freePlayerRecord(playerRecord);
		} break;
		case bz_eGetPlayerSpawnPosEvent:
		{
			bz_GetPlayerSpawnPosEventData_V1* data = (bz_GetPlayerSpawnPosEventData_V1*) eventData;
			
			// If the portal is active and we haven't spawned yet...
			if (portalMap[data->playerID]->isActive() && !portalMap[data->playerID]->spawned)
			{
				float* spawnPos = portalMap[data->playerID]->getLockedPos();
				data->pos[0] = spawnPos[0];
				data->pos[1] = spawnPos[1];
				data->pos[2] = spawnPos[2];
				data->rot = portalMap[data->playerID]->getLockedRot();
				portalMap[data->playerID]->spawned = true;
			}
		} break;
		case bz_ePlayerUpdateEvent:
		{
			bz_PlayerUpdateEventData_V1* data = (bz_PlayerUpdateEventData_V1*) eventData;
		
			// If the player has both has locked in and spawned, then then teleportation part is complete.
			if (portalMap[data->playerID]->locked && portalMap[data->playerID]->spawned)
			{
				if (bz_getCurrentTime() - portalMap[data->playerID]->lockTime > bz_getBZDBDouble("_dimensionDoorCooldownTime"))
				{
					portalMap[data->playerID]->clear();
					bz_givePlayerFlag(data->playerID, "DD", true);
					bz_sendTextMessage(BZ_SERVER, data->playerID, "If you have teleported inside a building, press [delete] to self-destruct.");
				}
			}
		} break;
		case bz_ePlayerJoinEvent:
		{
		    portalMap[((bz_PlayerJoinPartEventData_V1*) eventData)->playerID] = new Portal();
		} break;
		case bz_ePlayerPartEvent:
		{
			bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;
			delete portalMap[data->playerID];
		    portalMap.erase(data->playerID);
		} break;
		default:
			break;
	}
}



