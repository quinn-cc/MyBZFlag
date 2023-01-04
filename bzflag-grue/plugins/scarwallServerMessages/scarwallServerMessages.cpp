/*
 * Special server messages for the Scarwall server
 
 ./configure --enable-custom-plugins=scarwallServerMessages
*/

#include "bzfsAPI.h"
#include <regex>

using namespace std;

class HelpCommands : public bz_CustomSlashCommandHandler
{
public:
    virtual ~HelpCommands() {};
    virtual bool SlashCommand (int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList *param);
};

HelpCommands helpCommands;

class ScarwallServerMessages : public bz_Plugin {

    virtual const char* Name()
    {
        return "Scarwall Specific Features";
    }

    virtual void Init(const char*);
    virtual void Event(bz_EventData*);
    ~ScarwallServerMessages();

    virtual void Cleanup(void)
    {
    	bz_removeCustomSlashCommand("teamflaggeno");
    	bz_removeCustomSlashCommand("customflags");
        Flush();
    }

	string mode = "";
	void refreshGameMode(int, int);
};

BZ_PLUGIN(ScarwallServerMessages)


void ScarwallServerMessages::Init(const char* arg)
{
    Register(bz_ePlayerJoinEvent);
    Register(bz_eTickEvent);
	Register(bz_ePlayerDieEvent);
    bz_registerCustomSlashCommand("teamflaggeno", &helpCommands);
    bz_registerCustomSlashCommand("customflags", &helpCommands);
    
}

ScarwallServerMessages::~ScarwallServerMessages() {}

void broadcastMessage(const std::string& msg, int playerID)
{
    std::stringstream ss(msg);
    std::string line;
    
    while (std::getline(ss, line, '\n'))
    {
    	bz_sendTextMessage(BZ_SERVER, playerID, line.c_str());
    }
}

void ScarwallServerMessages::refreshGameMode(int redCount, int greenCount)
{
	string lastMode = mode;

	if (redCount == 1 && greenCount == 1)
		mode = "1v1";
	else if (redCount == 2 && greenCount == 1)
		mode = "red";
	else if (redCount == 1 && greenCount == 2)
		mode = "green";
	else
		mode = "";

	// If there was a change in game mode...
	if (lastMode != mode)
	{
		if (mode == "1v1")
			bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "1 v 1 mode: Your flag will be dropped after you shoot someone.");
		else if (mode == "red")
			bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, "2 v 1 mode: Players on the Red Team will drop their flag after they shoot the Green player.");
		else if (mode == "green")
			bz_sendTextMessage(
				BZ_SERVER, BZ_ALLUSERS,
				"2 v 1 mode: Players on the Green Team will drop their flag after they shoot the Red player."
			);
	}
}

void ScarwallServerMessages::Event(bz_EventData *ed)
{
	switch (ed->eventType)
	{
		case bz_ePlayerJoinEvent:
		{
			bz_PlayerJoinPartEventData_V1 *data = (bz_PlayerJoinPartEventData_V1*) ed;
			
			std::string lines =
			"**********************************************************\n"
			"****************     CASTLE SCARWALL     *****************\n"
			"**********************************************************\n"
			"*           Welcome to Castle Scarwall by Grue!          *\n"
			"* A classic two team capture the flag with lots of fancy *\n"
			"* plugins and new flags to make bzflag all the more fun! *\n"
			"*                                                        *\n"
			"* - The plugin teamflaggeno is in use, so there is no    *\n"
			"*   flag Genocide. Instead, your own team flag acts as   *\n"
			"*   geno! Type /teamflaggeno for more help.              *\n"
			"* - In addition, there are a few new CUSTOM FLAGS added  *\n"
			"*   to the server for more fun! Type /customflags to     *\n"
			"*   learn about them.                                    *\n"
			"*                                                        *\n"
			"* This server is still in the testing phase, so if there *\n"
			"* are any server crashes, bugs, or ideas you would like  *\n"
			"* to suggest, please use the /report command.            *\n"
			"**********************************************************\n"
			"Have fun! And always bring a lantern...";
			broadcastMessage(lines, data->playerID);
			
			if (bz_getPlayerCount() == 1)
			{
				bz_sendTextMessage(BZ_SERVER, data->playerID, "Treading alone I see... don't get eaten by a grue...");
			}

			if (bz_getPlayerTeam(data->playerID) == eRogueTeam)
			{
				bz_eTeamType team = bz_getUnbalancedTeam(eRedTeam, eGreenTeam);

				bz_sendTextMessagef(
					data->playerID, data->playerID,
					"Rogue team was unavailable, you were joined to the %s Team",
					bz_eTeamTypeLiteral(team)
				);

				bz_changeTeam(data->playerID, team);
			}
		} break;
		case bz_ePlayerDieEvent:
		{
			bz_PlayerDieEventData_V2* data = (bz_PlayerDieEventData_V2*) ed;
			
			if (mode == "1v1" ||
				(mode == "red" && bz_getPlayerTeam(data->killerID) == eRedTeam) ||
				(mode == "green" && bz_getPlayerTeam(data->killerID) == eGreenTeam))
			{
				if (!bz_isTeamFlag(bz_getPlayerFlagAbbr(data->killerID).c_str()) &&
					strcmp(bz_getPlayerFlagAbbr(data->killerID).c_str(), "AN") != 0)
					bz_resetFlag(bz_getPlayerFlagID(data->killerID));
			}
		} break;
		case bz_eTickEvent:
		{
			refreshGameMode(bz_getTeamCount(eRedTeam), bz_getTeamCount(eGreenTeam));
		} break;
		case bz_eFlagDroppedEvent:
		{
			bz_FlagDroppedEventData_V1* data = (bz_FlagDroppedEventData_V1*) ed;

			if (bz_isNaturalBadFlag(data->flagType))
				bz_resetFlag(data->flagID);
		}
		default:
			break;
	}
    
}

bool HelpCommands::SlashCommand (int playerID, bz_ApiString command, bz_ApiString message,
                                    bz_APIStringList * /*_param*/)
{
	if (command == "teamflaggeno")
	{
		std::string lines =
		"**********************************************************\n"
		"*                 Team Flag is Genocide!                 *\n"
		"* Instead of the flag Genocide, your own team flag acts  *\n"
		"* as geno! So don't spend time hiding your flag, as it   *\n"
		"* can be used as a deadly weapon                         *\n"
		"*                                                        *\n"
		"* However, watch out for someone holding the Avenger     *\n"
		"* flag. If you shoot someone holding the Avenger flag    *\n"
		"* while you have geno, your team blows up instead of     *\n"
		"* theirs!                                                *\n"
		"**********************************************************";
		broadcastMessage(lines, playerID);
		
		return true;	
	}
	else if (command == "customflags")
	{
		std::string lines =
		"**********************************************************\n"
		"*                      Custom flags                      *\n"
		"*    Here are the current custom flags on the server:    *\n"
		"*                                                        *\n"
		"* - Grenade (GN): The first shot fires the 'grenade',    *\n"
		"*   launching phantom zoned shots out ahead. The second  *\n"
		"*   time you fire, a shock wave explodes at the position *\n"
		"*   of the zoned shots.                                  *\n"
		"*                                                        *\n"
		"* - Vertical Velocity (VV): The tank fires two extra     *\n"
		"*   shots alongside the normal one that travel with      *\n"
		"*   vertical velocity. That means, if your tank is       *\n"
		"*   travelling upward when you shoot, so do those shots. *\n"
		"*                                                        *\n"
		"* - Annihilation (AN): You go rogue when you grab this   *\n"
		"*   flag! Shooting anyone will make the whole map blow   *\n"
		"*   up!                                                  *\n"
		"*                                                        *\n"
		"* - Gruesome Killer (GK): Your kills blow up in a shock  *\n"
		"*   wave and bullets, including the kills done by those  *\n"
		"*   shock waves and shots.								  *\n"
		"*                                                        *\n"
		"* - Triple Barrel (TB): Tank shoots a spread of three    *\n"
		"*   shots.                                               *\n"
		"*                                                        *\n"
		"* - Fifty Point (FP): One shot to make fifty points!     *\n"
		"*   Highly elusive, good luck finding it!                *\n"
		"*                                                        *\n"
		"* - Fifty Fifty (FF): Each shot has a fifty-fifty chance *\n"
		"*   of earning 20 points or losing 20 points.            *\n"
		"*                                                        *\n"
		"* - Airshot (AT): Tank shoots 2 extra shots at upward    *\n"
		"*   angles, good for shooting campers up high.           *\n"
		"*                                                        *\n"
		"* - Avenger (AV): If your killer is holding a flag when  *\n"
		"*   you are holding Avenger, then they die too. If your  *\n"
		"*   killer has geno, their team dies instead of yours!   *\n"
		"*                                                        *\n"
		"* - Skyfire (SF): Firing causes a hail of missiles to    *\n"
		"*   rain down from the sky in front of your tank.        *\n"
		"*                                                        *\n"
		"* - Dimension Door (DD): The first shot fires the        *\n"
		"*  'portal', the second shot teleports you there.        *\n"
		"*  Careful not to get stuck in any buildings! If you do, *\n"
		"*  press [delete] to self-destruct.				      *\n"
		"*                                                        *\n"
		"* - Torpedo (TO): Tank fires an extra two Super Bullet   *\n"
		"*   shots at ground level.          				      *\n"
		"*                                                        *\n"
		"* - Low Gravity (LG): Gravity is lowered, tank can jump  *\n"
		"*   higher.                                              *\n"
		"*                                                        *\n"
		"* - Wish (W): Make a wish for a flag! When you have this *\n"
		"*   flag, type /wish ## where ## is any flag's           *\n"
		"*   abbreviation. Be careful not to wish for something   *\n"
		"*   too powerful or you may face dire consequences!      *\n"
		"*       												  *\n"
		"**********************************************************";
		broadcastMessage(lines, playerID);
		
		return true;
	}
	
	return false;
}










