/*
 * 
 *
 * ./configure --enable-custom-plugins=gruesTurretDefenseMaster
 */


#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <vector>
#include <map>
#include <math.h>

class Bomb
{
public:
	int playerID;
	float initPos[3];
	double initTime;
	float vel[3];
	
	Bomb();
	
	float* getCurrentPos();
	void explode();
};

Bomb::Bomb()
{
	initTime = bz_getCurrentTime();
}

float* Bomb::getCurrentPos()
{
	float* currentPos = new float[3];
	double timeDiff = bz_getCurrentTime() - initTime;
	
	currentPos[0] = initPos[0] + timeDiff*vel[0]*bz_getBZDBDouble("_shotSpeed");
	currentPos[1] = initPos[1] + timeDiff*vel[1]*bz_getBZDBDouble("_shotSpeed");
	currentPos[2] = initPos[2] + timeDiff*vel[2]*bz_getBZDBDouble("_shotSpeed");
	
	return currentPos;
}

void Bomb::explode()
{
	float v[3] = { 0, 0, 0 };
	float p[3];
	p[0] = getCurrentPos()[0];
	p[1] = getCurrentPos()[1];
	p[2] = 0;			// Always make SW at ground level
	bz_fireServerShotAsPlayer("SW", p, v, "BM", playerID);
}

// Stores the bombs for the Bomber flag
std::vector<Bomb> bombs;

class HelpCommands : public bz_CustomSlashCommandHandler
{
public:
    virtual ~HelpCommands() {};
    virtual bool SlashCommand (int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList *param);
};

HelpCommands helpCommands;
std::map<int, bool>	switchedControls;				// ids of players that have swiched controls

/*
 * This zone is where the invaders must reach in order to switch teams.
 */
class WinZone : public bz_CustomZoneObject
{
public:
	WinZone() : bz_CustomZoneObject() {}
};

/*
 * This zone is used for both the invader's spawn zone and the turret's spawn
 * zone. 
 */ 
class SpawnZone : public bz_CustomZoneObject
{
public:
	SpawnZone() : bz_CustomZoneObject() {}
};

/*
 * These zones will mark where each individual turret location takes place.
 * Other than being a zone with dimensions in the bzflag world, it will also
 * turret functionality such as firing position and reload time.
 */
class TurretZone : public bz_CustomZoneObject
{
public:
	int index;					// unique id
	int playerID;				// playerID occupying this turret; -1 if unoccupied
	double lastShotTime; 		// last time this turret shot (used for reloading)
	float firePosHeight;		// the height at which to fire the shots
	TurretZone() : bz_CustomZoneObject()
	{
		playerID = -1;
		lastShotTime = bz_getCurrentTime();
	}
	double getReloadTime();
	void fireShot();
};

/*
 * This fucntion returns the time (in seconds) between firing shots. It will
 * detect if the player has a flag that makes reload time faster.
 */
double TurretZone::getReloadTime()
{
	double time = bz_getBZDBDouble("_turretReloadTime");

	float flagMult = 1;
	bz_ApiString flagMultSetting = "_turretReloadMult";
	flagMultSetting += bz_getPlayerFlagAbbr(playerID);

	if (bz_BZDBItemExists(flagMultSetting.c_str()))
		flagMult = bz_getBZDBDouble(flagMultSetting.c_str());
	
	time *= flagMult;

	return time;
}

void TurretZone::fireShot()
{
	if (playerID == -1) return;

	bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(playerID);
	// But also check what flag the turret player has, if any.
	
	bz_ApiString flag = bz_getPlayerFlagAbbr(playerRecord->playerID);
	
	// Calculate velocity & angle of shot
	float yDiff = yMax - playerRecord->lastKnownState.pos[1];
	
	if (switchedControls[playerID])
		yDiff = playerRecord->lastKnownState.pos[1] - yMin;
	
	float yRatio = yDiff / (yMax - yMin);
	float angleDiff = bz_getBZDBDouble("_turretMaxAngle") - bz_getBZDBDouble("_turretMinAngle");
	float angle = bz_getBZDBDouble("_turretMinAngle") + yRatio*angleDiff;
	
	
	// Determine the position
	float pos[3];
	pos[0] = playerRecord->lastKnownState.pos[0] + cos(playerRecord->lastKnownState.rotation)*4;
	pos[1] = playerRecord->lastKnownState.pos[1] + sin(playerRecord->lastKnownState.rotation)*4;
	pos[2] = firePosHeight;
	
	// Determine the inaccuracy
	float inaccuracy[3] = { 0, 0, 0 };
	if (bz_getBZDBDouble("_turretAccuracy") > 0)
	{
		float inacFlagMult = 1;
		bz_ApiString inacSetting = "_turretAccuracyMult";
		inacSetting += flag;

		if (bz_BZDBItemExists(inacSetting.c_str()))
			inacFlagMult = bz_getBZDBDouble(inacSetting.c_str());

		for (int i = 0; i < 3; i++)
		{
			inaccuracy[i] = bz_randFloatBetween(-bz_getBZDBDouble("_turretAccuracy"), bz_getBZDBDouble("_turretAccuracy"));
			inaccuracy[i] *= inacFlagMult;
		}
	}
	
	// Determine the velocity
	float vel[3];
	vel[0] = cos(playerRecord->lastKnownState.rotation + inaccuracy[0]);
	vel[1] = sin(playerRecord->lastKnownState.rotation + inaccuracy[1]);
	vel[2] = -cos(angle + inaccuracy[2]);

	float velFlagMult = 1;
	bz_ApiString velSetting = "_turretSpeedMult";
	velSetting += flag;

	if (bz_BZDBItemExists(velSetting.c_str()))
		velFlagMult = bz_getBZDBDouble(velSetting.c_str());

	for (int i = 0; i < 3; i++)
		vel[i] *= velFlagMult;
	
	if (flag == "DB")
	{
		float offset[2];   // VV shot's offset from base position
		float pos1[3];     // Position of one DB shot
		float pos2[3];     // Position of the other DB shot
		
		offset[0] = -sin(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_doubleBarrelWidth");
		offset[1] = cos(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_doubleBarrelWidth");
		
		pos1[0] = pos[0] + offset[0];
		pos1[1] = pos[1] + offset[1];
		pos1[2] = pos[2];
		
		pos2[0] = pos[0] - offset[0];
		pos2[1] = pos[1] - offset[1];
		pos2[2] = pos[2];
		
		bz_fireServerShotAsPlayer("", pos1, vel, flag.c_str(), playerRecord->playerID);
		bz_fireServerShotAsPlayer("", pos2, vel, flag.c_str(), playerRecord->playerID);
	}
	else
	{
		bz_ApiString type = "";
		
		// If the player is holding Bomber, the shot
		// type must be GM for special effects.
		if (flag == "BM")	
			type = "GM";
		
		bz_fireServerShotAsPlayer(type.c_str(), pos, vel, flag.c_str(), playerRecord->playerID);
		
		// Aka, if the flag is Bomber
		if (flag == "BM")
		{
			Bomb bomb;
			bomb.playerID = playerID;
			bomb.initPos[0] = pos[0];
			bomb.initPos[1] = pos[1];
			bomb.initPos[2] = pos[2];
			bomb.vel[0] = vel[0];
			bomb.vel[1] = vel[1];
			bomb.vel[2] = vel[2];
			bombs.push_back(bomb);
		}
		else if (flag == "TB")
		{
			float vel1[3];
			float vel2[3];
		
			// Left shot
			vel1[0] = cos(playerRecord->lastKnownState.rotation - bz_getBZDBDouble("_tripleBarrelAngle")) + playerRecord->lastKnownState.velocity[0]*0.01;
			vel1[1] = sin(playerRecord->lastKnownState.rotation - bz_getBZDBDouble("_tripleBarrelAngle")) + playerRecord->lastKnownState.velocity[1]*0.01;
			vel1[2] = -cos(angle + inaccuracy[2]);
			
			// Right shot
			vel2[0] = cos(playerRecord->lastKnownState.rotation + bz_getBZDBDouble("_tripleBarrelAngle")) + playerRecord->lastKnownState.velocity[0]*0.01;
			vel2[1] = sin(playerRecord->lastKnownState.rotation + bz_getBZDBDouble("_tripleBarrelAngle")) + playerRecord->lastKnownState.velocity[1]*0.01;
			vel2[2] = -cos(angle + inaccuracy[2]);
		
			bz_fireServerShotAsPlayer(type.c_str(), pos, vel1, flag.c_str(), playerRecord->playerID);
			bz_fireServerShotAsPlayer(type.c_str(), pos, vel2, flag.c_str(), playerRecord->playerID);
		}
	}
	
	bz_freePlayerRecord(playerRecord);
	lastShotTime = bz_getCurrentTime();
}



//////////////////////////////////////////////////
//// 				Main Plugin				//////
//////////////////////////////////////////////////
class gruesTurretDefenseMaster : public bz_Plugin, public bz_CustomMapObjectHandler
{
    virtual const char* Name() { return "Grue's Turret Defense Master"; }
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	virtual bool MapObject(bz_ApiString object, bz_CustomMapObjectInfo* data);
	virtual void Cleanup(void);
	~gruesTurretDefenseMaster();
	
protected:
	SpawnZone turretSpawnZone;					// zone to spawn turret players in
	SpawnZone invaderSpawnZone;					// zone to spawn invader players in
	
	std::vector<TurretZone> turretZones;		// the list of turrets
	bz_eTeamType turretTeam = eRedTeam;			// stores which team are turrets
	bz_eTeamType getInvaderTeam();
	
	WinZone winZone;							// the zone that is required to reach by invaders
	
	int defenseCountdown;
	double lastDefenseCountdownTime;
	// GTD can only be played if there is at least 1 player on each team. If
	// this variable is true, then the rules of GTD are in play. If this value
	// is false, then the team scoring and rules of GTD are put on pause.
	// The only method that should change this field is refreshGameplay.
	bool gameplay = true;
	
	void invadedTower(int);
	void refreshGameplay();
	void resetDefenseCountdownTimer();
	void defendedTower();
	void cleanupTurretsForPlayer(int);
	
	
};

BZ_PLUGIN(gruesTurretDefenseMaster)

void gruesTurretDefenseMaster::Init (const char*)
{
	// Events
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPartEvent);
    Register(bz_ePlayerDieEvent);
    Register(bz_eGetPlayerSpawnPosEvent);
    Register(bz_ePlayerUpdateEvent);
    Register(bz_eTickEvent);
    Register(bz_eWorldFinalized);
    
    // Zones
    bz_registerCustomMapObject("invaderspawnzone", this);
    bz_registerCustomMapObject("turretspawnzone", this);
    bz_registerCustomMapObject("turretzone", this);
    bz_registerCustomMapObject("winzone", this);
   
    // Custom flags
    bz_RegisterCustomFlag("DB", "Double Barrel", "Turret fires two shots instead of one.", 0, eGoodFlag);
    bz_RegisterCustomFlag("TB", "Triple Barrel", "Turret fires a spread of three shots.", 0, eGoodFlag);
    bz_RegisterCustomFlag("GK", "Gruesome Killer", "Kills explode in a shock wave.", 0, eGoodFlag);
    bz_RegisterCustomFlag("BM", "Bomber", "Turret fires slow bombs that erupt in a shock wave on the ground.", 0, eGoodFlag);
    
    // Generic settings
    bz_registerCustomBZDBDouble("_turretMaxAngle", 1.570);  // radians
    bz_registerCustomBZDBDouble("_turretMinAngle", 1.2);  // radians
    bz_registerCustomBZDBInt("_defenseTime", 60);
    bz_registerCustomBZDBBool("_destroyInvadersOnDefense", false);
    bz_registerCustomBZDBDouble("_turretReloadTime", 0.25);
    bz_registerCustomBZDBDouble("_turretAccuracy", 0.01);

	// Custom flag settings
	bz_registerCustomBZDBDouble("_turretReloadMultDB", 1);
	bz_registerCustomBZDBDouble("_turretReloadMultTB", 1);
	bz_registerCustomBZDBDouble("_turretReloadMultGK", 1);
	bz_registerCustomBZDBDouble("_turretReloadMultMG", 0.5);
	bz_registerCustomBZDBDouble("_turretReloadMultBM", 5.0);
	bz_registerCustomBZDBDouble("_turretReloadMultSB", 1);
	bz_registerCustomBZDBDouble("_turretReloadMultSE", 1);
	bz_registerCustomBZDBDouble("_turretReloadMultR", 1);

	bz_registerCustomBZDBDouble("_turretSpeedMultDB", 1);
	bz_registerCustomBZDBDouble("_turretSpeedMultTB", 1);
	bz_registerCustomBZDBDouble("_turretSpeedMultGK", 1);
	bz_registerCustomBZDBDouble("_turretSpeedMultMG", 1);
	bz_registerCustomBZDBDouble("_turretSpeedMultBM", 0.5);
	bz_registerCustomBZDBDouble("_turretSpeedMultSB", 1);
	bz_registerCustomBZDBDouble("_turretSpeedMultSE", 1);
	bz_registerCustomBZDBDouble("_turretSpeedMultR", 1);

	bz_registerCustomBZDBDouble("_turretAccuracyMultDB", 1);
	bz_registerCustomBZDBDouble("_turretAccuracyMultTB", 1);
	bz_registerCustomBZDBDouble("_turretAccuracyMultGK", 1);
	bz_registerCustomBZDBDouble("_turretAccuracyMultMG", 1);
	bz_registerCustomBZDBDouble("_turretAccuracyMultBM", 1);
	bz_registerCustomBZDBDouble("_turretAccuracyMultSB", 1);
	bz_registerCustomBZDBDouble("_turretAccuracyMultSE", 1);
	bz_registerCustomBZDBDouble("_turretAccuracyMultR", 1);

    bz_registerCustomBZDBDouble("_doubleBarrelWidth", 1.0);
    bz_registerCustomBZDBDouble("_tripleBarrelAngle", 0.15);
	bz_registerCustomBZDBInt("_turretGKBlossomCount", 12);
	bz_registerCustomBZDBDouble("_turretGKShockwaveLifetime", 3);
	bz_registerCustomBZDBDouble("_turretGKShrapnelLifetime", 4);
    
	// Slash command
    bz_registerCustomSlashCommand("switchcontrols", &helpCommands);
}


/*
 * Reads in all the zones from the .bzw file.
 */
bool gruesTurretDefenseMaster::MapObject(bz_ApiString object, bz_CustomMapObjectInfo *data)
{
	if (!data)
		return false;

	if (object == "TURRETSPAWNZONE")
	{
		turretSpawnZone.handleDefaultOptions(data);
		return true;
	}
	else if (object == "INVADERSPAWNZONE")
	{
		invaderSpawnZone.handleDefaultOptions(data);
		return true;
	}
	else if (object == "WINZONE")
	{
		winZone.handleDefaultOptions(data);
		return true;
	}
	else if (object == "TURRETZONE")
	{
		TurretZone turretZone;
		turretZone.handleDefaultOptions(data);
		
		for (unsigned int i = 0; i < data->data.size(); i++)
		{
			std::string line = data->data.get(i).c_str();

			bz_APIStringList nubs;
			nubs.tokenize(line.c_str(), " ", 0, true);

			if (nubs.size() > 1)
			{
				std::string n0 = bz_toupper(nubs.get(0).c_str());
				
				if (n0 == "INDEX")
					turretZone.index = std::stoi(nubs.get(1)); // Set the index of the turret zone
				else if (n0 == "FIREPOSITION")
				{
					// Set the height at which the turret fires at
					// Note: the x and y position of the fire is at the player's
					// x and y position.
					turretZone.firePosHeight = (float) std::stoi(nubs.get(1));
				}
			}
		}
		
		turretZones.push_back(turretZone);
		return true;
	}
	
	return false;
}

void gruesTurretDefenseMaster::Cleanup(void)
{
	Flush();
	bz_removeCustomMapObject("turretspawnzone");
	bz_removeCustomMapObject("invaderspawnzone");
	bz_removeCustomMapObject("turretzone");
	bz_removeCustomMapObject("winzone");
}

gruesTurretDefenseMaster::~gruesTurretDefenseMaster() {}

/*
 * This function resets the timer counting down that determines when the turrets
 * get another team point.
 */
void gruesTurretDefenseMaster::resetDefenseCountdownTimer()
{
	defenseCountdown = bz_getBZDBInt("_defenseTime");
	// This scales the time up for unbalanced teams. For instance, if there are
	// 4 players on the turret team and 2 on the invader team, the timer will be
	// default time * (4/2). So, it will take the turrets twice as long to cap
	// since they twice as many players.
	if (std::min(bz_getTeamCount(turretTeam),bz_getTeamCount(getInvaderTeam())) != bz_getTeamCount(turretTeam))
		defenseCountdown = (int)(bz_getBZDBInt("_defenseTime") * (bz_getTeamCount(turretTeam)/(float)bz_getTeamCount(getInvaderTeam())));
		
	lastDefenseCountdownTime = bz_getCurrentTime();
	bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "Time reset: %d seconds to invade the castle!", defenseCountdown);
}

/*
 * This is called when an invader reaches the turret's side. This function
 * broadcasts the necessary server messages, and switches the turrets and
 * invaders place.
 */
void gruesTurretDefenseMaster::invadedTower(int playerID)
{
	// Server messages
	bz_sendTextMessagef(
		BZ_SERVER, BZ_ALLUSERS,
		"INVADED! %s has successfully invaded castle!!",
		bz_getPlayerCallsign(playerID)
	);
	bz_sendTextMessagef(
		BZ_SERVER, BZ_ALLUSERS,
		"Teams switch; %s are now the turrets, and %s are the invaders.",
		bz_eTeamTypeLiteral(bz_getPlayerTeam(playerID)),
		bz_eTeamTypeLiteral(turretTeam)
	);

	bz_APIIntList* playerList = bz_getPlayerIndexList();	
	// Kill all players
    for (int i = 0; i < bz_getPlayerCount(); i++)
    {
		int currPlayer = playerList->get(i);
        bz_killPlayer(currPlayer, false, playerID);
        
        // Add back lossed points
        if (currPlayer != playerID)
        	bz_incrementPlayerLosses(currPlayer, -1);
        
        // Sends this to each player individually. This makes the text appear
        // in white, and not look like it is coming from the server.
        bz_sendTextMessagef(
			currPlayer,
			currPlayer,
			"Teams switched.",
			bz_getPlayerCallsign(playerID)
		);
    }
    
    // Clear all turrets from occupied players
    for (int i = 0; i < turretZones.size(); i++)
		turretZones[i].playerID = -1;
    
    if (gameplay)
    	resetDefenseCountdownTimer();

    // Switch the teams
    turretTeam = bz_getPlayerTeam(playerID);
}

bz_eTeamType gruesTurretDefenseMaster::getInvaderTeam()
{
	bz_eTeamType invaderTeam;
	if (turretTeam == eRedTeam)
		invaderTeam = eBlueTeam;
	else
		invaderTeam = eRedTeam;
	return invaderTeam;
}

/*
 * Checks whether or not turret defense can be played. There must be at least
 * one person on each team for turret defense to be playable. If there is a change
 * i.e., it was not in play and now it is, or vice versa, this function 
 * broadcasts necessary server messages.
 */
void gruesTurretDefenseMaster::refreshGameplay()
{	
	if (bz_getTeamCount(getInvaderTeam()) > 0 && bz_getTeamCount(turretTeam) > 0)
	{
		if (!gameplay)
		{
			bz_sendTextMessage(
				BZ_SERVER, BZ_ALLUSERS,
				"Game is now in session!"
			);
			bz_sendTextMessagef(
				BZ_SERVER, BZ_ALLUSERS,
				"For every %d seconds the turrets (%s) defend the castle, they get a team point.",
				bz_getBZDBInt("_defenseTime"),
				bz_eTeamTypeLiteral(turretTeam)
			);
			bz_sendTextMessagef(
				BZ_SERVER, BZ_ALLUSERS, 
				"The invaders (%s) try to reach the castle. Go! Time's ticking!",
				bz_eTeamTypeLiteral(getInvaderTeam())
			);
			resetDefenseCountdownTimer();
			gameplay = true;
		}
	}
	else
	{
		if (gameplay)
		{
			bz_sendTextMessage(
				BZ_SERVER, BZ_ALLUSERS,
				"There must be at least one player on both teams to play turret defense."
			);
			bz_sendTextMessage(
				BZ_SERVER, BZ_ALLUSERS,
				"Tower defense has been temporarily disabled."
			);
			gameplay = false;
		}
	}
}

/*
 * This function is called when the turrets have successfully defended the
 * castle after a set amount of time. This function broadcasts necessary
 * messages, and possibly destroys the invaders if that setting is turned on.
 */
void gruesTurretDefenseMaster::defendedTower()
{
	// Broadcast the message out to all
	bz_sendTextMessagef(
		BZ_SERVER, BZ_ALLUSERS,
		"%s has successfully stood their ground. +1 team point.",
		bz_eTeamTypeLiteral(turretTeam)
	);
	bz_incrementTeamWins(turretTeam, 1);
	
	if (bz_getBZDBBool("_destroyInvadersOnDefense"))
	{
		bz_APIIntList* playerList = bz_getPlayerIndexList();
		for (int i = 0; i < bz_getPlayerCount(); i++)
		{
			if (bz_getPlayerTeam(playerList->get(i) != turretTeam))
		    	bz_killPlayer(playerList->get(i), false);
		   
		   	bz_incrementPlayerLosses(playerList->get(i), -1);
		}
	}
}

/*
 * Removes any players from turrets they are no longer in. This is called when
 * someone has died or someone leaves the game, searching for a particular
 * playerID.
 */
void gruesTurretDefenseMaster::cleanupTurretsForPlayer(int playerID)
{
	if (bz_getPlayerTeam(playerID) == turretTeam)
	{
		for (TurretZone &turretZone : turretZones)
		{
			// If the player that left was occupying a turret, remove them.
			if (turretZone.playerID == playerID)
			{
				turretZone.playerID = -1;
			}
		}
    }
}

/*
  This function is used in the detonate function below. The bool "up" indicates
  whether the shots fire upward at the 45 degree angle.
*/
void fireShotGK(float playerPos[3], int killerID, float rotation, bool up)
{
	float vel[3] = { cos(rotation), sin(rotation), 0 };
	float pos[3] = { playerPos[0], playerPos[1], (float)bz_getBZDBDouble("_muzzleHeight") };
	
	if (up)
	{
		vel[2] = sqrt(2)/2.0;
		vel[0] *= sqrt(2)/2.0;
		vel[1] *= sqrt(2)/2.0;
	}
	
	bz_fireServerShotAsPlayer("GK", pos, vel, "GK", killerID, bz_getBZDBDouble("_turretGKShrapnelLifetime"));
}

/*
  This function detonates the gruesome killer explosion effect. There are a ring
  of death blossom shots that explode on the ground, a second ring that fire at a
  45 degree angle, one singluar shot upward, and shock wave. All of these shots
  can cascade and cause more gruesome killer detonations.
*/
void detonateGK(float playerPos[3], float rotation, int killerID)
{
	// Shock wave
	float vel[3] = { 0, 0, 0 };
  	bz_fireServerShotAsPlayer("SW", playerPos, vel, "GK", killerID,
			bz_getBZDBDouble("_turretGKShockwaveLifetime"));

	// Ground-level death blossom
	for (int i = 0; i < bz_getBZDBInt("_turretGKBlossomCount"); i++)
	{
		fireShotGK(playerPos, killerID, i*((2*3.142)/bz_getBZDBInt("_turretGKBlossomCount")) + rotation, false);
	}
	
	// Death blossom at 45 degree angle
	int upCount = bz_getBZDBInt("_turretGKBlossomCount")/sqrt(2);
	for (int i = 0; i < upCount; i++)
	{
		fireShotGK(playerPos, killerID, i*((2*3.142)/upCount) + rotation, true);
	}
	
	// Shot going directly upward
	vel[0] = 0;
	vel[1] = 0;
	vel[2] = 1;
	bz_fireServerShotAsPlayer("GK", playerPos, vel, "GK", killerID,
			bz_getBZDBDouble("_turretGKShrapnelLifetime"));
}

void gruesTurretDefenseMaster::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		// When world is initialized and finalized.
		case bz_eWorldFinalized:
		{
			// This ensures that the tick is at least .05 seconds
			gruesTurretDefenseMaster::MaxWaitTime = .05;
			resetDefenseCountdownTimer();
		} break;
		case bz_ePlayerJoinEvent:
		{
			bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;
			
			if (bz_getPlayerTeam(data->playerID) == turretTeam)
			{
				bz_sendTextMessage(BZ_SERVER, data->playerID, "You are on the turret team.");
			}
			else
			{
				bz_sendTextMessage(BZ_SERVER, data->playerID, "You are on the invader team.");
			}
			
			gameplay = true;
			refreshGameplay();
			switchedControls[data->playerID] = false;
		} break;
		case bz_ePlayerPartEvent:
		{
			bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;
			cleanupTurretsForPlayer(data->playerID);
            switchedControls.erase(data->playerID);
		} break;
		case bz_ePlayerDieEvent:
		{
			bz_PlayerDieEventData_V1* data = (bz_PlayerDieEventData_V1*) eventData;

			if (data->flagKilledWith == "GK")
			{
				bz_BasePlayerRecord* player = bz_getPlayerByIndex(data->playerID);
				detonateGK(player->lastKnownState.pos, player->lastKnownState.rotation, data->killerID);
			}

			cleanupTurretsForPlayer(data->playerID);
		} break;
		case bz_eGetPlayerSpawnPosEvent:
		{
			bz_GetPlayerSpawnPosEventData_V1* data = (bz_GetPlayerSpawnPosEventData_V1*) eventData;
			float pos[3];
			
			if (data->team == turretTeam)
				bz_getSpawnPointWithin(&turretSpawnZone, pos);
			else
				bz_getSpawnPointWithin(&invaderSpawnZone, pos);
				
			data->pos[0] = pos[0];
			data->pos[1] = pos[1];
			data->pos[2] = pos[2];
		} break;
		case bz_ePlayerUpdateEvent:
        {
            bz_PlayerUpdateEventData_V1 *data = (bz_PlayerUpdateEventData_V1*) eventData;
            
            for (TurretZone &turretZone : turretZones)
            {
            	// If a turret is not occupied...
            	if (turretZone.playerID == -1)
            	{
		            if (turretZone.pointInZone(data->state.pos))		// .. and if a player is in the zone...
		            {
		                turretZone.playerID = data->playerID;			// Then occupy it with that playerID
		            }
		        }
		        // If the turret zone is occupied with a player, but that player
                // is no longer in the zone, then remove that player from the zone.
                else if (turretZone.playerID == data->playerID)
                {
                	if (!turretZone.pointInZone(data->state.pos))
                	{
                		turretZone.playerID = -1;
                	}
                }
            }
            // If the other team has reached the turret side, switch teams
            if (winZone.pointInZone(data->state.pos) && (bz_getPlayerTeam(data->playerID) != turretTeam))
            {
            	invadedTower(data->playerID);
            }
        } break;
        case bz_eTickEvent:
        {
        	for (TurretZone &turretZone : turretZones)
            {
            	// Make sure this turret is occupied
            	if (turretZone.playerID != -1)
            	{
            		if (bz_getCurrentTime() - turretZone.lastShotTime >= turretZone.getReloadTime())
				    {
				    	turretZone.fireShot();
				    }
            	}
            }

            if (gameplay)
            {
		        // If a second has gone by...
		        if (bz_getCurrentTime() - lastDefenseCountdownTime >= 1)
		        {
		        	lastDefenseCountdownTime = bz_getCurrentTime();
		        	defenseCountdown--;
		        
		        	if (defenseCountdown >= 1)
		        	{
		        		if (defenseCountdown <= 10 || defenseCountdown % 10 == 0)
		        			bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "%d seconds left!", defenseCountdown);
		        	}
		        	else
		        	{
		        		// Reset the time
		        		resetDefenseCountdownTimer();
		        		defendedTower();
		        	}
		        }
            }
            
            int i = 0;
           	while (i < bombs.size())
           	{
           		if (bombs[i].getCurrentPos()[2] <= 0)
           		{
           			bombs[i].explode();
           			bombs.erase(bombs.begin()+i);
           		}
           		else
           			i++;
           	}
           	
            refreshGameplay();
        } break;
		default: break;
	}
}

bool HelpCommands::SlashCommand (int playerID, bz_ApiString command, bz_ApiString message,
                                    bz_APIStringList * /*_param*/)
{
	if (command == "switchcontrols")
	{
		bz_sendTextMessagef(BZ_SERVER, playerID, "Your turret controls have been reversed.");
		switchedControls[playerID] = !switchedControls[playerID];
		return true;
	}
	return false;
}





















