/*
 * 
 
 ./configure --enable-custom-plugins=gruesTurretDefenseMaster
*/



#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <vector>
#include <map>
#include <math.h>

class HelpCommands : public bz_CustomSlashCommandHandler
{
public:
    virtual ~HelpCommands() {};
    virtual bool SlashCommand (int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList *param);
};

HelpCommands helpCommands;

std::map<int, bool>	switchedControls;				// ids of players that have swiched controls

// teamString
// Helpful function turns a bz_eTeamType into a string.
// Since there are only Red and Blue team on this map, that's all we need to
// worry about.
std::string teamString(bz_eTeamType team)
{
	if (team == eRedTeam)
		return "Red team";
	else if (team == eBlueTeam)
		return "Blue team";
	else return "";
}

// Win Zone
// This zone is where the invaders must reach in order to switch teams.
class WinZone : public bz_CustomZoneObject
{
public:
	WinZone() : bz_CustomZoneObject() {}
};

// Spawn Zone
// This zone is used for both the invader's spawn zone and the turret's spawn
// zone. 
class SpawnZone : public bz_CustomZoneObject
{
public:
	SpawnZone() : bz_CustomZoneObject() {}
};

// Turret Zone
// These zones will mark where each individual turret location takes place.
// Other than being a zone with dimensions in the bzflag world, it will also
// turret functionality such as firing position and reload time.
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
};

// getReloadTime
// This fucntion returns the time (in seconds) between firing shots. It will
// detect if the player has a flag that makes reload time faster.
double TurretZone::getReloadTime()
{
	double time = bz_getBZDBDouble("_turretReloadTime");
	
	if (bz_getPlayerFlag(playerID) != NULL)
	{
		std::string flag = bz_getPlayerFlag(playerID);
		
		if (flag == "MG")
			time *= bz_getBZDBDouble("_turretMGReloadMult");
		else if (flag == "BM")
			time *= bz_getBZDBDouble("_turretBMReloadMult");
	}
	
	return time;
}

class Bomb
{
public:
	uint32_t shotGUID;
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
	uint32_t shotGUIDSW = bz_fireServerShot("SW", p, v, bz_getPlayerTeam(playerID));
	bz_setShotMetaData(shotGUIDSW, "type", "BM");
	bz_setShotMetaData(shotGUIDSW, "owner", playerID);
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
	bool gameInPlay = true;
	
	void switchTeams(int);
	void checkGameInPlay();
	void resetDefenseCountdownTimer();
	void defendedCastle();
	void cleanupTurretsForPlayer(int);
	
	// Stores the bombs for the Bomber flag
	std::vector<Bomb> bombs;
	
	string[] turretFlags;
	void randomizeTurretFlags();
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
    
    // Custom Settings
    bz_registerCustomBZDBDouble("_turretMaxAngle", 1.570);  // radians
    bz_registerCustomBZDBDouble("_turretMinAngle", 1.2);  // radians
    bz_registerCustomBZDBInt("_defenseTime", 60);
    bz_registerCustomBZDBBool("_destroyInvadersOnCap", false);
    bz_registerCustomBZDBDouble("_turretReloadTime", 0.25);
    
    bz_registerCustomBZDBDouble("_turretMGReloadMult", 0.5);
    bz_registerCustomBZDBDouble("_turretBMReloadMult", 5.0);
    bz_registerCustomBZDBDouble("_turretBMSpeedMult", 0.5);
    bz_registerCustomBZDBDouble("_doubleBarrelWidth", 1.0);
    bz_registerCustomBZDBDouble("_tripleBarrelAngle", 0.15);
    
    bz_registerCustomBZDBDouble("_turretAccuracy", 0.01);
    
    bz_registerCustomSlashCommand("switchcontrols", &helpCommands);
    
    randomizeTurretFlags();
}

void gruesTurretDefenseMaster::randomizeTurretFlags()
{
	
}

// MapObject
// This part of the plugin is what reads in all the zones from the .bzw file.
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

// resetDefenseCountdownTimer
// This function resets the timer counting down that determines when the turrets
// get another team point.
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

// switchTeams
// This is called when an invader reaches the turret's side. This function broadcasts
// the necessary server messages, and switches the turrets and invaders place.
void gruesTurretDefenseMaster::switchTeams(int playerID)
{
	// Server messages
	bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "INVADED! %s has successfully invaded castle!!", bz_getPlayerCallsign(playerID));
	bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "Teams switch; %s are now the turrets, and %s are the invaders.", teamString(bz_getPlayerTeam(playerID)).c_str(), teamString(turretTeam).c_str());

	bz_APIIntList* playerList = bz_getPlayerIndexList();	
	// Kill all players
    for (int i = 0; i < bz_getPlayerCount(); i++)
    {
        bz_killPlayer(playerList->get(i), false, playerID);
        
        // Add back lossed points
        if (playerList->get(i) != playerID)
        	bz_incrementPlayerLosses(playerList->get(i), -1);
        
        // Sends this to each player individually. This makes the text appear
        // in white, and not look like it is coming from the server.
        bz_sendTextMessagef(playerList->get(i), playerList->get(i), "Teams switched.", bz_getPlayerCallsign(playerID));
    }
    
    // Clear all turrets from occupied players
    for (int i = 0; i < turretZones.size(); i++)
	{
		turretZones[i].playerID = -1;
	}
    
    if (gameInPlay)
    	resetDefenseCountdownTimer();
    // Switch the teams
    turretTeam = bz_getPlayerTeam(playerID);
}

bz_eTeamType gruesTurretDefenseMaster::getInvaderTeam()
{
	bz_eTeamType invaderTeam;
	if (turretTeam == eRedTeam) invaderTeam = eBlueTeam;
	else invaderTeam = eRedTeam;
	return invaderTeam;
}

// checkGameInPlay
// Checks whether or not turret defense can be played. There must be at least
// one person on each team for turret defense to be playable. If there is a change
// i.e., it was not in play and now it is, or vice versa, this function 
// broadcasts necessary server messages.
void gruesTurretDefenseMaster::checkGameInPlay()
{	
	if (bz_getTeamCount(getInvaderTeam()) > 0 && bz_getTeamCount(turretTeam) > 0)
	{
		if (!gameInPlay)
		{
			resetDefenseCountdownTimer();
			gameInPlay = true;
			
			bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "Game is now in session!");
			bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "For every %d seconds the turrets (%s) defend the castle, they get a team point.", bz_getBZDBInt("_defenseTime"), teamString(turretTeam).c_str());
			bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "The invaders (%s) try to reach the castle. Go! Time's ticking!", teamString(getInvaderTeam()).c_str());
		}
	}
	else
	{
		if (gameInPlay)
		{
			bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "There must be at least one player on both teams to play turret defense.");
			gameInPlay = false;
		}
	}
}

// defendedCastle
// This function is called when the turrets have successfully defended the castle
// after a set amount of time. This function broadcasts necessary messages, and possibly
// destroys the invaders if that setting is turned on.
void gruesTurretDefenseMaster::defendedCastle()
{
	// Broadcast the message out to all
	bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "%s has successfully stood their ground. +1 team point.", teamString(turretTeam).c_str());
	bz_incrementTeamWins(turretTeam, 1);
	
	if (bz_getBZDBBool("_destroyInvadersOnCap"))
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

// Removes any players from turrets they are no longer in. This is called when
// someone has died or someone leaves the game, searching for a particular playerID.
void gruesTurretDefenseMaster::cleanupTurretsForPlayer(int playerID)
{
	if (bz_getPlayerTeam(playerID) == turretTeam)
	{
		for (auto &turretZone : turretZones)
		{
			// If the player that left was occupying a turret, remove them.
			if (turretZone.playerID == playerID)
			{
				turretZone.playerID = -1;
			}
		}
    }
}

float randomFloat(float a, float b)
{
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
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
			
			gameInPlay = true;
			checkGameInPlay();
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
			cleanupTurretsForPlayer(data->playerID);
			
			uint32_t shotGUID = bz_getShotGUID(data->killerID, data->shotID);
			if (bz_shotHasMetaData(shotGUID, "type") && bz_shotHasMetaData(shotGUID, "owner"))
			{
				//bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "Shot has stuff.");
			    std::string flag = bz_getShotMetaDataS(shotGUID, "type");
			    
		        data->killerID = bz_getShotMetaDataI(shotGUID, "owner");
		        data->killerTeam = bz_getPlayerTeam(data->killerID);
		        
		        if (flag == "GK")
		        {
		        	float vel[3] = { 0, 0, 0 };
		        	uint32_t shotGUIDSW = bz_fireServerShot("SW", data->state.pos, vel, data->killerTeam);
					bz_setShotMetaData(shotGUIDSW, "type", "GK");
					bz_setShotMetaData(shotGUIDSW, "owner", data->killerID);
		        }
			}
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
            
            for (auto &turretZone : turretZones)
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
            //bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "PlayerUpdateEvent.");
            // If the other team has reached the turret side, switch teams
            if (winZone.pointInZone(data->state.pos) && (bz_getPlayerTeam(data->playerID) != turretTeam))
            {
            	switchTeams(data->playerID);
            }
        } break;
        case bz_eTickEvent:
        {
        	for (auto &turretZone : turretZones)
            {
            	// Make sure this turret is occupied
            	if (turretZone.playerID != -1)
            	{
            		if (bz_getCurrentTime() - turretZone.lastShotTime >= turretZone.getReloadTime())
				    {
				    	bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(turretZone.playerID);
				    	// FIRE SHOT!!!
				    	// But also check what flag the turret player has, if any.
				    	
				    	std::string flag = "T";
				    	
				    	if (bz_getPlayerFlag(turretZone.playerID) != NULL)
				    		flag = bz_getPlayerFlag(turretZone.playerID);
				    	
				    	// Calculate velocity & angle of shot
						float yDiff = turretZone.yMax - playerRecord->lastKnownState.pos[1];
						
						if (switchedControls[turretZone.playerID])
							yDiff = playerRecord->lastKnownState.pos[1] - turretZone.yMin;
						
						float yRatio = yDiff / (turretZone.yMax - turretZone.yMin);
						float angleDiff = bz_getBZDBDouble("_turretMaxAngle") - bz_getBZDBDouble("_turretMinAngle");
						float angle = bz_getBZDBDouble("_turretMinAngle") + yRatio*angleDiff;
						
						
						float vel[3];
						float pos[3];
						
						pos[0] = playerRecord->lastKnownState.pos[0] + cos(playerRecord->lastKnownState.rotation)*4;
						pos[1] = playerRecord->lastKnownState.pos[1] + sin(playerRecord->lastKnownState.rotation)*4;
						pos[2] = turretZone.firePosHeight;
						
						float off[3] = { 0, 0, 0 };
						if (bz_getBZDBDouble("_turretAccuracy") > 0)
						{
							off[0] = randomFloat(-bz_getBZDBDouble("_turretAccuracy"), bz_getBZDBDouble("_turretAccuracy"));
							off[1] = randomFloat(-bz_getBZDBDouble("_turretAccuracy"), bz_getBZDBDouble("_turretAccuracy"));
							off[2] = randomFloat(-bz_getBZDBDouble("_turretAccuracy"), bz_getBZDBDouble("_turretAccuracy"));
						}
						
						vel[0] = cos(playerRecord->lastKnownState.rotation + off[0]);
						vel[1] = sin(playerRecord->lastKnownState.rotation + off[1]);
						vel[2] = -cos(angle + off[2]);
				    	
				  		if (flag == "DB")
				  		{
				  			// VV shot's base position
							float offset[2];   // VV shot's offset from base position
							float pos1[3];     // Position of one VV shot
							float pos2[3];     // Position of the other VV shot
							
							offset[0] = -sin(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_doubleBarrelWidth");
							offset[1] = cos(playerRecord->lastKnownState.rotation)*bz_getBZDBDouble("_doubleBarrelWidth");
							
							pos1[0] = pos[0] + offset[0];
							pos1[1] = pos[1] + offset[1];
							pos1[2] = pos[2];
							
							pos2[0] = pos[0] - offset[0];
							pos2[1] = pos[1] - offset[1];
							pos2[2] = pos[2];
							
							uint32_t shot1GUID = bz_fireServerShot("DB", pos1, vel, playerRecord->team);
						  	bz_setShotMetaData(shot1GUID, "type", "DB");
							bz_setShotMetaData(shot1GUID, "owner", turretZone.playerID);
							
							uint32_t shot2GUID = bz_fireServerShot("DB", pos2, vel, playerRecord->team);
							bz_setShotMetaData(shot2GUID, "type", "DB");
							bz_setShotMetaData(shot2GUID, "owner", turretZone.playerID);
				  		}
				  		else
				  		{
							// We must make MG be assigned to T or the bullets will go short like MG
							if (flag == "MG")
								flag = "T";
							else if (flag == "BM")
							{
								vel[0] *= bz_getBZDBDouble("_turretBMSpeedMult");
								vel[1] *= bz_getBZDBDouble("_turretBMSpeedMult");
								vel[2] *= bz_getBZDBDouble("_turretBMSpeedMult");
								// If the player is holding Bomber, then we need to fire the shot as if it is a GM
								// so the effect will be a GM
								flag = "GM";
							} 
							
							uint32_t shotGUID = bz_fireServerShot(flag.c_str(), pos, vel, playerRecord->team);
							bz_setShotMetaData(shotGUID, "type", bz_getPlayerFlag(turretZone.playerID));
							bz_setShotMetaData(shotGUID, "owner", turretZone.playerID);
							
							// Aka, if the flag is Bomber
							if (flag == "GM")
							{
								Bomb bomb;
								bomb.shotGUID = shotGUID;
								bomb.playerID = turretZone.playerID;
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
								vel1[2] = -cos(angle + off[2]);
								
								// Right shot
								vel2[0] = cos(playerRecord->lastKnownState.rotation + bz_getBZDBDouble("_tripleBarrelAngle")) + playerRecord->lastKnownState.velocity[0]*0.01;
								vel2[1] = sin(playerRecord->lastKnownState.rotation + bz_getBZDBDouble("_tripleBarrelAngle")) + playerRecord->lastKnownState.velocity[1]*0.01;
								vel2[2] = -cos(angle + off[2]);
							
								uint32_t shotGUID1 = bz_fireServerShot(flag.c_str(), pos, vel1, playerRecord->team);
								bz_setShotMetaData(shotGUID1, "type", bz_getPlayerFlag(turretZone.playerID));
								bz_setShotMetaData(shotGUID1, "owner", turretZone.playerID);
								
								uint32_t shotGUID2 = bz_fireServerShot(flag.c_str(), pos, vel2, playerRecord->team);
								bz_setShotMetaData(shotGUID2, "type", bz_getPlayerFlag(turretZone.playerID));
								bz_setShotMetaData(shotGUID2, "owner", turretZone.playerID);
							}
						}
						
						bz_freePlayerRecord(playerRecord);
						turretZone.lastShotTime = bz_getCurrentTime();
				    }
            	}
            }
            if (gameInPlay)
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
		        		defendedCastle();
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
           	
            checkGameInPlay();
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





















