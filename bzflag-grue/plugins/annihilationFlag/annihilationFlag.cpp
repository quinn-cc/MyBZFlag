/*
 * Custom flag: ANnihilation (+AN)
 * Shooting yourself causes everyone on the map to die. If the server setting
 * _annihilationRequireEligibility is turned on, then the player using AN must
 * have a minimum wins-to-losses ratio to use the flag. Can be used to prevent
 * trolls. Furthermore, if _annihilationBUImmune is turned on, then those
 * holding the Burrow flag are immune to AN.
 * 
 * Server Variables (default):
 * _annihilationRequireEligibility (false): Is eligibilty ratio in use
 * _annihilationEligibleRatio (1.2): The minimum wins-to-losses ratio required
 * 	to operate the flag if _annihilationRequireEligibility is in use.
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

using namespace std;

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
};

BZ_PLUGIN(AnnihilationFlag)

void AnnihilationFlag::Init(const char*)
{
    bz_RegisterCustomFlag("AN", "Annihilation", "Shoot yourself and everyone dies.", 0, eGoodFlag);
    bz_registerCustomBZDBBool("_annihilationRequireEligibility", false);
    bz_registerCustomBZDBDouble("_annihilationEligibleRatio", 1.2);
    bz_registerCustomBZDBBool("_annihilationUseFX", true);
    bz_registerCustomBZDBInt("_annihilationFXSWCount", 20);
    bz_registerCustomBZDBInt("_annihilationFXSWMinHeight", 10);	
    bz_registerCustomBZDBInt("_annihilationFXSWMaxHeight", 20);
	bz_registerCustomBZDBBool("_annihilationBUImmune", true);
    Register(bz_ePlayerDieEvent);
}

void AnnihilationFlag::Event(bz_EventData *eventData)
{
    if (eventData->eventType == bz_ePlayerDieEvent)
    {
        bz_PlayerDieEventData_V2 *data = (bz_PlayerDieEventData_V2*) eventData;

		// If the player shot themselves with AN
        if (data->flagKilledWith == "AN" && data->killerID == data->playerID)
        {
        	// Check if the player is eligible to use AN if the eligibility
			// ratio is turned on.
        	if (bz_getBZDBBool("_annihilationRequireEligibility") && bz_getPlayerLosses(data->playerID) != 0 &&
        		bz_getPlayerWins(data->playerID) / bz_getPlayerLosses(data->playerID) >= bz_getBZDBDouble("_annihilationEligibleRatio"))
        	{
        		bz_sendTextMessage(BZ_SERVER, data->playerID, "Your wins to losses ratio makes you uneligible for annihilaiton.");
        		return;
        	}
        
            bz_APIIntList* playerList = bz_getPlayerIndexList();
			bz_sendTextMessagef(BZ_SERVER, BZ_ALLUSERS, "BOOM! %s just annihilated the map!", bz_getPlayerCallsign(data->playerID));
			
			// Kill all the players
            for (int i = 0; i < bz_getPlayerCount(); i++)
            {
            	bz_BasePlayerRecord* player = bz_getPlayerByIndex(playerList->get(i));
            	
				// If the player is alive, kill them.
				if (player && player->spawned)
				{
					// If the burrow immune setting is on, make sure they dont
					// have Burrow.
					if (!bz_getBZDBBool("_annihilationBUImmune") ||
						(bz_getPlayerFlag(player->playerID) &&
						(bz_getPlayerFlag(player->playerID) == "BU")))
					{
						bz_killPlayer(playerList->get(i), false, data->playerID, "AN");
						// bz_killPlayer automatically decrements their score,
						// so this adds it back.
						bz_incrementPlayerLosses(playerList->get(i), -1);
						
						if (playerList->get(i) != data->playerID)
		            		bz_sendTextMessagef(playerList->get(i), playerList->get(i), "Map annihilated by player %s.", player->callsign.c_str());
                	}
				}
				
				bz_freePlayerRecord(player);
            }
            
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
        	
        	// Reset the flag into the world
        	bz_resetFlag(data->flagHeldWhenKilled);
        }
    }
}

