/*
 * Custom flag: ANnihilation (+AN)
 * Shooting yourself causes everyone on the map to die. If the server setting
 * _annihilationRequireEligibility is turned on, then the player using AN must
 * have a minimum wins-to-losses ratio to use the flag. Can be used to prevent
 * trolls. Furthermore, if _annihilationBUImmune is turned on, then those
 * holding the Burrow flag are immune to AN.
 * 
 * Server Variables (default):
 * _annihilationUseFX (true): If set to true, using the AN flag will cause
 * 	Shock Waves to burst throughout the map as special effects.
 * _annihilationFXSWCount (20): The number of Shock Waves to appear as special
 * 	effects.
 * _annihilationFXSWMinHeight (10): Min height at which the Shock Waves occur.
 * _annihilationFXSWMaxHeight (20): Max height at which the Shock Waves occur.
 * _annihilationBUImmune (true): If set to true, players holding the BU flag are
 * 	immune to AN.
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=annihilationFlag
 */
 
#include "bzfsAPI.h"
#include <math.h>
#include <cstdlib>
#include <ctime>
#include <map>
#include <queue>

using namespace std;

struct PlayerState
{
	float* pos;
	float rot;
	bool toRogue;
};

class AnnihilationFlag : public bz_Plugin {

    virtual const char* Name()
    {
        return "Annihilation Flag";
    }

    virtual void Init(const char*);
    virtual void Event(bz_EventData*);
    ~AnnihilationFlag() {};

    virtual void Cleanup(void)
    {
        Flush();
    }

	map<int, bz_eTeamType> originalPlayerTeam;
	map<int, PlayerState> playerLocMap;
	map<int, double> playerNeedsANTime;
	queue<int> playerSwitchBackQueue;

	void switchPlayer(int, bool, bz_eTeamType);
	void switchPlayer(int playerID, bool toRogue);
};

void AnnihilationFlag::switchPlayer(int playerID, bool toRogue)
{
	// Pack the player's data into the state.
	PlayerState state;
	bz_BasePlayerRecord* record = bz_getPlayerByIndex(playerID);
	state.pos = new float[3];
	state.pos[0] = record->lastKnownState.pos[0];
	state.pos[1] = record->lastKnownState.pos[1];
	state.pos[2] = record->lastKnownState.pos[2];
	state.rot = record->lastKnownState.rotation;
	state.toRogue = toRogue;
	bz_freePlayerRecord(record);
	playerLocMap[playerID] = state;

	bz_incrementPlayerLosses(playerID, -1);
	bz_killPlayer(playerID, false);

	if (toRogue)
	{
		originalPlayerTeam[playerID] = bz_getPlayerTeam(playerID);
		bz_changeTeam(playerID, eRogueTeam);
	}
	else
	{
		bz_changeTeam(playerID, originalPlayerTeam[playerID]);
		originalPlayerTeam.erase(playerID);
	}

	bz_forcePlayerSpawn(playerID);
}

void AnnihilationFlag::switchPlayer(int playerID, bool toRogue, bz_eTeamType team)
{
	PlayerState state;
	bz_BasePlayerRecord* record = bz_getPlayerByIndex(playerID);
	state.pos = new float[3];
	state.pos[0] = record->lastKnownState.pos[0];
	state.pos[1] = record->lastKnownState.pos[1];
	state.pos[2] = record->lastKnownState.pos[2];
	state.rot = record->lastKnownState.rotation;
	state.toRogue = toRogue;
	bz_freePlayerRecord(record);
	playerLocMap[playerID] = state;

	bz_incrementPlayerLosses(playerID, -1);
	bz_killPlayer(playerID, toRogue);
	bz_changeTeam(playerID, team);
	bz_forcePlayerSpawn(playerID);
}

BZ_PLUGIN(AnnihilationFlag)

void AnnihilationFlag::Init(const char*)
{
    bz_RegisterCustomFlag("AN", "Annihilation", "You are rogue now, shoot anyone to destroy the map!", 0, eGoodFlag);

    bz_registerCustomBZDBBool("_annihilationUseFX", true);
    bz_registerCustomBZDBInt("_annihilationFXSWCount", 20);
    bz_registerCustomBZDBInt("_annihilationFXSWMinHeight", 10);	
    bz_registerCustomBZDBInt("_annihilationFXSWMaxHeight", 20);
	bz_registerCustomBZDBBool("_annihilationBUImmune", true);
    
	Register(bz_eFlagGrabbedEvent);
	Register(bz_eFlagDroppedEvent);
	Register(bz_eFlagTransferredEvent);
	Register(bz_eGetPlayerSpawnPosEvent);
	Register(bz_eTickEvent);
	Register(bz_ePlayerDieEvent);
}

void AnnihilationFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case bz_eFlagGrabbedEvent:
		{
			bz_FlagGrabbedEventData_V1 *data = (bz_FlagGrabbedEventData_V1*) eventData;

			if (strcmp(data->flagType, "AN") == 0 && bz_getPlayerTeam(data->playerID) != eRogueTeam)
			{
				bz_sendTextMessagef(
					BZ_SERVER, BZ_ALLUSERS,
					"WARNING! %s has gone rogue and is carrying a weapon of mass destruction!",
					bz_getPlayerCallsign(data->playerID)
				);

				switchPlayer(data->playerID, true);
			}
		} break;
		case bz_eFlagDroppedEvent:
		{
			bz_FlagGrabbedEventData_V1 *data = (bz_FlagGrabbedEventData_V1*) eventData;

			if (strcmp(data->flagType, "AN") == 0 &&
				originalPlayerTeam.find(data->playerID) != originalPlayerTeam.end() &&
				playerLocMap.find(data->playerID) == playerLocMap.end())
			{
				bz_resetFlag(data->flagID);
				switchPlayer(data->playerID, false);
			}
		} break;
		case bz_eFlagTransferredEvent:
		{
			bz_FlagTransferredEventData_V1 *data = (bz_FlagTransferredEventData_V1*) eventData;

			if (strcmp(data->flagType, "AN") == 0)
			{
				switchPlayer(data->fromPlayerID, false);
				switchPlayer(data->toPlayerID, true);
			}
		} break;
		case bz_eGetPlayerSpawnPosEvent:
		{
			bz_GetPlayerSpawnPosEventData_V1* data = (bz_GetPlayerSpawnPosEventData_V1*) eventData;
			
			if (playerLocMap.find(data->playerID) != playerLocMap.end())
			{
				data->pos[0] = playerLocMap[data->playerID].pos[0];
				data->pos[1] = playerLocMap[data->playerID].pos[1];
				data->pos[2] = playerLocMap[data->playerID].pos[2];
				data->rot = playerLocMap[data->playerID].rot;
				

				if (playerLocMap[data->playerID].toRogue)
					playerNeedsANTime[data->playerID] = bz_getCurrentTime();

				playerLocMap.erase(data->playerID);
			}
		} break;
		case bz_eTickEvent:
		{
			int playerID = -1;

			for(auto pair : playerNeedsANTime)
			{
				if (bz_getCurrentTime() - pair.second > 0.5)
				{
					bz_givePlayerFlag(pair.first, "AN", true);
					playerID = pair.first;
				}
			}

			if (playerID != -1)
				playerNeedsANTime.erase(playerID);

			if (!playerSwitchBackQueue.empty())
			{
				int id = playerSwitchBackQueue.front();
				playerSwitchBackQueue.pop();
				bz_killPlayer(id, false);
			}
		} break;
		case bz_ePlayerDieEvent:
		{
			bz_PlayerDieEventData_V2 *data = (bz_PlayerDieEventData_V2*) eventData;

			bz_ApiString flag = bz_getFlagName(data->flagHeldWhenKilled);
			if (flag == "AN")
			{
				data->killerID = BZ_SERVERPLAYER;
				bz_incrementPlayerWins(data->killerID, 1);
			}

			// If the player shoots someone else with AN
			if (data->flagKilledWith == "AN" && data->killerID != data->playerID)
			{
				bz_APIIntList* playerList = bz_getPlayerIndexList();
				bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "BOOM! %s just annihilated the map!", bz_getPlayerCallsign(data->killerID));
				
				// Kill all the players
				for (int i = 0; i < bz_getPlayerCount(); i++)
				{
					bz_BasePlayerRecord* player = bz_getPlayerByIndex(playerList->get(i));
					
					
					if (player && player->team != eObservers)
					{
						// If the burrow immune setting is on, make sure they dont
						// have Burrow.
						if (!(bz_getBZDBBool("_annihilationBUImmune") && bz_getPlayerFlagAbbr(player->playerID) == "BU"))
						{
							if (playerList->get(i) != data->killerID)
							{
								bz_killPlayer(playerList->get(i), false, data->killerID, "");
								// bz_killPlayer automatically decrements their score,
								// so this adds it back.
								bz_incrementPlayerLosses(playerList->get(i), -1);
							}

							bz_sendTextMessagef(
								playerList->get(i), playerList->get(i),
								"Map annihilated by player %s.",
								bz_getPlayerCallsign(data->killerID)
							);
						}
					}
					
					bz_freePlayerRecord(player);
				}

				data->killerTeam = eRogueTeam;
				playerSwitchBackQueue.push(data->killerID);
				bz_deleteIntList(playerList);
				
				// Special effects (shock waves)
				if (bz_getBZDBBool("_annihilationUseFX"))
				{
					std::srand(std::time(nullptr));
				
					for (int i = 0; i < bz_getBZDBInt("_annihilationFXSWCount"); i++)
					{
						float pos[3];
						pos[0] = (std::rand() % (int)bz_getBZDBDouble("_worldSize")) - (bz_getBZDBDouble("_worldSize")/2);
						pos[1] = (std::rand() % (int)bz_getBZDBDouble("_worldSize")) - (bz_getBZDBDouble("_worldSize")/2);
						pos[2] = (std::rand() % (bz_getBZDBInt("_annihilationFXSWMaxHeight") -
							bz_getBZDBInt("_annihilationFXSWMinHeight"))) + bz_getBZDBInt("_annihilationFXSWMinHeight");
						
						float vel[3] = { 0, 0, 0 };
						
						bz_fireServerShot("SW", pos, vel, eRogueTeam);
					}
				}
				
				
			}
		} break;
		default:
			break;
	}
}