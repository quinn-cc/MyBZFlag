/*	
 * A bunch of shit flags
 * 
 * ./configure --enable-custom-plugins=shittyFlags
 */
#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <math.h>

using namespace std;

class ShittyFlags : public bz_Plugin
{
    virtual const char* Name ()
    {
        return "The Scarwall Secret";
    }
    virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~ShittyFlags() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(ShittyFlags)

void ShittyFlags::Init(const char*) {
	
	bz_RegisterCustomFlag("DB", "Death Blossom", "Tank shoots extra bullets in random directions.", 0, eGoodFlag);
    bz_RegisterCustomFlag("AC", "Ass Cannon", "Tank has the shits, extra shot comes out the ass.", 0, eGoodFlag);
    bz_RegisterCustomFlag("RR", "Retroreflector", "Kinda like Avenger, but stupid.", 0, eGoodFlag);
	Register(bz_eShotFiredEvent);
}

void ShittyFlags::Event(bz_EventData *ed)
{
	switch (ed->eventType)
	{
        case bz_eShotFiredEvent:
        {
            bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) ed;
            bz_BasePlayerRecord *player = bz_getPlayerByIndex(data->playerID);

            if (bz_getPlayerFlagAbbr(player->playerID) == "AC")
            {
                float pos[3];
                pos[0] = player->lastKnownState.pos[0] + cos(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
                pos[1] = player->lastKnownState.pos[1] + sin(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
                pos[2] = player->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");

                // Ass shot
                float vel1[3];
                vel1[0] = cos(player->lastKnownState.rotation - 3.14 + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"));
                vel1[1] = sin(player->lastKnownState.rotation - 3.14 + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"));
                vel1[2] = 0;
                bz_fireServerShotAsPlayer("AC", pos, vel1, "AC", player->playerID);
            }
            else if (bz_getPlayerFlagAbbr(player->playerID) == "DB")
            {
                float pos[3];
                pos[0] = player->lastKnownState.pos[0] + cos(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
                pos[1] = player->lastKnownState.pos[1] + sin(player->lastKnownState.rotation)*bz_getBZDBDouble("_muzzleFront");
                pos[2] = player->lastKnownState.pos[2] + bz_getBZDBDouble("_muzzleHeight");

                // Ass shot
                float vel1[3];
                vel1[0] = cos(player->lastKnownState.rotation - 3.14 + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"));
                vel1[1] = sin(player->lastKnownState.rotation - 3.14 + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"));
                vel1[2] = 0;
                bz_fireServerShotAsPlayer("DB", pos, vel1, "DB", player->playerID);

                // Side shot
                vel1[0] = cos(player->lastKnownState.rotation - 1.04 + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"));
                vel1[1] = sin(player->lastKnownState.rotation - 1.04 + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"));
                vel1[2] = 0;
                bz_fireServerShotAsPlayer("DB", pos, vel1, "DB", player->playerID);

                // Side shot
                vel1[0] = cos(player->lastKnownState.rotation - 2.08 + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"));
                vel1[1] = sin(player->lastKnownState.rotation - 2.08 + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"));
                vel1[2] = 0;
                bz_fireServerShotAsPlayer("DB", pos, vel1, "DB", player->playerID);

                // Side shot
                vel1[0] = cos(player->lastKnownState.rotation + 1.04 + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"));
                vel1[1] = sin(player->lastKnownState.rotation + 1.04 + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"));
                vel1[2] = 0;
                bz_fireServerShotAsPlayer("DB", pos, vel1, "DB", player->playerID);

                // Side shot
                vel1[0] = cos(player->lastKnownState.rotation + 2.08 + player->lastKnownState.velocity[0]/bz_getBZDBDouble("_shotSpeed"));
                vel1[1] = sin(player->lastKnownState.rotation + 2.08 + player->lastKnownState.velocity[1]/bz_getBZDBDouble("_shotSpeed"));
                vel1[2] = 0;
                bz_fireServerShotAsPlayer("DB", pos, vel1, "DB", player->playerID);
            }
            
            bz_freePlayerRecord(player);
        } break;
        case bz_ePlayerDieEvent:
        {
            bz_PlayerDieEventData_V2 *data = (bz_PlayerDieEventData_V2*) ed;

            // Don't cover the case of non-shots and don't cover case of shooting yourself.
            if (data->shotID == -1 || data->playerID == data->killerID)
                return;
            
            bz_ApiString flagHeldWhenKilled = bz_getFlagName(data->flagHeldWhenKilled);
                
            // If the victim was holding Avenger when they died...
            if (flagHeldWhenKilled == "RR")
            {
                uint32_t shotGUID = bz_getShotGUID(data->killerID, data->shotID);
                
                // If the killer had a flag...
                if (data->flagKilledWith != "" && data->flagKilledWith == "L")
                {
                    bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(data->killerID);
                    
                    if (playerRecord && playerRecord->spawned == true)		// Make sure the player exists and is alive
                    {
                        bz_killPlayer(data->killerID, false, data->playerID, "AV");
                        bz_sendTextMessage(BZ_SERVER, data->killerID, "You managed to shoot someone with Retroreflector, you die too.");
                    }
                    
                    bz_freePlayerRecord(playerRecord);
                }
            }
        } break;
        default:
            break;
	}
}
