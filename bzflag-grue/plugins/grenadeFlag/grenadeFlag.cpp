/*
 * Custom flag: Grenade (+GN)
 * First shot fires the grenade, second shot detonates it. The grenade is
 * represented by two Phantom Zoned shots that fire out ahead of the tank. The
 * second shot will create a Shock Wave at the position of those phantom zoned
 * shots.
 * 
 * Server Variables (default):
 * _grenadeSpeedAdVel (4.0): Multiplied by normal shot speed to determine
 * 	grenade speed.
 * _grenadeLifetime (1.5): The time in seconds that the grenade lasts before
 * 	expiring.
 * _grenadeVerticalVelocity (true): Whether or not the grenades use vertical
 * 	velocity.
 * _grenadeWidth (2.0): Distance from middle shot to side grenade PZ shot
 * _grenadeAccuracy (0.02): Grenade's level of accuracy. Lower number is more
 * 	accurate; zero is perfect accuracy.
 * _grenadeExplosionLifetime (8.0): Time in seconds the Shock Wave explosion
 * 	lasts.
 * 
 * Requires:
 * - Grue's BZFS
 * - ShotAssigner
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=grenadeFlag
 */
 
#include "bzfsAPI.h"
#include <math.h>
#include <map>
#include <queue>
using namespace std;

/*
 * Each player has exactly one Grenade object assigned to them. The Grenade
 * object is created once when they join the game and deleted when they leave.
 * The object is made active when a grenade is shot, and the object is made
 * inactive when the grenade has been detonated or expired.
 */
class Grenade
{
private:
	bool active = false;		// Is this object in use
	float origin[3];			// Position where player first shot grenade
	float velocity[3];			// Velocity of the grenade
	double initialTime;			// Initial time when grenade was fired
	uint32_t pzShots[2];		// GUIDs of PZ shots fired as grenade markers

public:
	Grenade() {};
	void init(float*, float*, uint32_t, uint32_t);
	void clear();
	bool isActive();
	
	float* getPosition();
	bool isExpired();
	uint32_t* getPZShots();
};

/*
 * Initializes the Grenade object; this method is called when a player fires
 * a grenade.
 */
void Grenade::init(float* pos, float* vel, uint32_t pz1, uint32_t pz2)
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

/*
 * Deactivates the Grenade object; this method is called when the grenade either
 * expires or detonates.
 */
void Grenade::clear()
{
	active = false;
}

bool Grenade::isActive()
{
	return active;
}

/*
 * Calculates the position of the grenade based on its initial time and 
 * velocity.
 */
float* Grenade::getPosition()
{
	double elapsedTime = bz_getCurrentTime() - initialTime;
	float* pos = new float[3];

	pos[0] = origin[0] + velocity[0]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	pos[1] = origin[1] + velocity[1]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	pos[2] = origin[2] + velocity[2]*elapsedTime*bz_getBZDBInt("_shotSpeed");
	
	return pos;
}

/*
 * Checks if the grenade is expired.
 */
bool Grenade::isExpired()
{
	bool expired = false;

	// If the grenade is below the ground level or has exceeded its liftime,
	// then it is expired.
	if (getPosition()[2] <= 0 || bz_getCurrentTime() - initialTime > bz_getBZDBDouble("_grenadeLifetime"))
		expired = true;
	
	return expired;
}

/*
 * Returns an array of two elements containing the GUIDs of the PZ shots fired
 * as markers for the grenade.
 */
uint32_t* Grenade::getPZShots()
{
	return pzShots;
}

struct GrenadeExplosion
{
	double time;
	uint32_t shotGUID;
};

class GrenadeFlag : public bz_Plugin
{
	virtual const char* Name()
	{
		return "Grenade Flag";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~GrenadeFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
	
private:
	// Stores a map between each player and a singular Grenade object.
    map<int, Grenade*> grenadeMap;
	// Stores a map of all grenade SW explosions and their explosion time
	queue<GrenadeExplosion> explosionQueue;
};

BZ_PLUGIN(GrenadeFlag)

void GrenadeFlag::Init(const char*)
{
	bz_RegisterCustomFlag("GN", "Grenade", "First shot fires the grenade, second shot detonates it.", 0, eGoodFlag);
	
	bz_registerCustomBZDBDouble("_grenadeSpeedAdVel", 4.0);
	bz_registerCustomBZDBDouble("_grenadeLifetime", 1.5);
	bz_registerCustomBZDBDouble("_grenadeVerticalVelocity", true);
	bz_registerCustomBZDBDouble("_grenadeWidth", 2.0);
	bz_registerCustomBZDBDouble("_grenadeAccuracy", 0.02);
	bz_registerCustomBZDBDouble("_grenadeExplosionLifetime", 10);
	bz_registerCustomBZDBDouble("_grenadeExplosionLifetimeEndShort", 1.5);
	
	Register(bz_eShotFiredEvent);
	Register(bz_ePlayerJoinEvent);
	Register(bz_ePlayerPartEvent);
	Register(bz_eTickEvent);

	if (MaxWaitTime > 0.5)
		MaxWaitTime = 0.5;
}

void GrenadeFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case bz_eShotFiredEvent:
		{
			bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
			bz_BasePlayerRecord* player = bz_getPlayerByIndex(data->playerID);
			
			if (bz_getPlayerFlagAbbr(player->playerID) == "GN")
			{
				// If the grenade is active but expired, clear it.
				if (grenadeMap[data->playerID]->isExpired())
					grenadeMap[data->playerID]->clear();

				// If this player does not have an active grenade, launch one.
				if (!grenadeMap[data->playerID]->isActive())
				{
					float vel[3];		// PZ shot's velocity
					float pos[3];		// PZ shot's base position
					float offset[2];	// PZ shot's offset from base position
					float pos1[3];		// Position of one PZ shot
					float pos2[3];		// Position of the other PZ shot
					float innacuracy = bz_randFloatBetween(-bz_getBZDBDouble("_grenadeAccuracy")/2, bz_getBZDBDouble("_grenadeAccuracy")/2);
					
					// Base/center position of the two PZ shots
					pos[0] = player->lastKnownState.pos[0] + cos(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
					pos[1] = player->lastKnownState.pos[1] + sin(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
					pos[2] = player->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");
					
					// The offset of the PZ shots
					offset[0] = -sin(player->lastKnownState.rotation)*bz_getBZDBDouble("_grenadeWidth");
					offset[1] = cos(player->lastKnownState.rotation)*bz_getBZDBDouble("_grenadeWidth");
					
					// Velocity of the PZ shots
					vel[0] = cos(player->lastKnownState.rotation+innacuracy)*bz_getBZDBDouble("_grenadeSpeedAdVel");
					vel[1] = sin(player->lastKnownState.rotation+innacuracy)*bz_getBZDBDouble("_grenadeSpeedAdVel");
					vel[2] = sin(abs(innacuracy));
					
					// If the vertical velocity option is turned on, then apply
					// it to the PZ shots.
					if (bz_getBZDBBool("_grenadeVerticalVelocity"))
						vel[2] += player->lastKnownState.velocity[2]/bz_getBZDBDouble("_shotSpeed");
						
					// PZ shot 1
					pos1[0] = pos[0] + offset[0];
					pos1[1] = pos[1] + offset[1];
					pos1[2] = pos[2];
					uint32_t pzShot1 = bz_fireServerShotAsPlayer(
						"PZ", pos1, vel, "GN",
						player->playerID, bz_getBZDBDouble("_grenadeLifetime")
					);
					
					// PZ shot 2
					pos2[0] = pos[0] - offset[0];
					pos2[1] = pos[1] - offset[1];
					pos2[2] = pos[2];
					uint32_t pzShot2 = bz_fireServerShotAsPlayer(
						"PZ", pos2, vel, "GN",
						player->playerID, bz_getBZDBDouble("_grenadeLifetime")
					);
					
					grenadeMap[data->playerID]->init(pos, vel, pzShot1, pzShot2);
				}
				// Otherwise, detonate it.
				else
				{
					float vel[3] = { 0, 0, 0 };
					float* pos = grenadeMap[data->playerID]->getPosition();
					//float* pos = bz_getServerShotPos(grenadeMap[data->playerID]->getPZShots()[0]);

					uint32_t shotGUID = bz_fireServerShotAsPlayer("SW", pos, vel, "GN", player->playerID,
						bz_getBZDBDouble("_grenadeExplosionLifetime"));

					GrenadeExplosion explosion;
					explosion.shotGUID = shotGUID;
					explosion.time = bz_getCurrentTime();
					explosionQueue.push(explosion);

					// End the PZ shots in a firey explosion
					bz_endServerShot(grenadeMap[data->playerID]->getPZShots()[0]);
					bz_endServerShot(grenadeMap[data->playerID]->getPZShots()[1]);

					grenadeMap[data->playerID]->clear();
				}
			}
		
			bz_freePlayerRecord(player);
		} break;
		case bz_eTickEvent:
		{
			while (explosionQueue.size() > 0 &&
					bz_getCurrentTime() - explosionQueue.front().time > bz_getBZDBDouble("_grenadeExplosionLifetimeEndShort"))
			{
				bz_endServerShot(explosionQueue.front().shotGUID, false);
				explosionQueue.pop();
			}
		} break;
		case bz_ePlayerJoinEvent:
		{
			// Assign a Grenade to the new player
		    grenadeMap[((bz_PlayerJoinPartEventData_V1*) eventData)->playerID] = new Grenade();
		} break;
		case bz_ePlayerPartEvent:
		{
			bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;
			// Clear the player's Grenade
			delete grenadeMap[data->playerID];
		    grenadeMap.erase(data->playerID);
		} break;
		default:
			break;
	}
}



