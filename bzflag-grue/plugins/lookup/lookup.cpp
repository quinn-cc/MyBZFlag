/*
 Lookup
 
 ./configure --enable-custom-plugins=scarwallServerMessages
*/

#include "bzfsAPI.h"
#include <regex>

using namespace std;

class LookupCommand : public bz_CustomSlashCommandHandler
{
public:
    virtual ~LookupCommand() {};
    virtual bool SlashCommand (int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList *param);
};

LookupCommand lookupCommand;

class Lookup : public bz_Plugin {

    virtual const char* Name()
    {
        return "Lookup";
    }

    virtual void Init(const char*);
    virtual void Event(bz_EventData*);
    ~Lookup();

    virtual void Cleanup(void)
    {
    	bz_removeCustomSlashCommand("lookup");
        Flush();
    }
    
	
};

std::map<std::string, std::vector<std::string>> lookupMap;
BZ_PLUGIN(Lookup)


void Lookup::Init(const char*)
{
    Register(bz_ePlayerJoinEvent);
    bz_registerCustomSlashCommand("lookup", &lookupCommand);
}

Lookup::~Lookup() {}

bool LookupCommand::SlashCommand (int playerID, bz_ApiString command, bz_ApiString, bz_APIStringList* params)
{
	if (command == "lookup")
	{
		if (params->size() != 1)
		{
			bz_sendTextMessage(BZ_SERVER, playerID, "/lookup <player ID|callsign>");
			return true;
		}
		
		const char* victimID = params->get(0).c_str();
        bz_BasePlayerRecord *playerRecord = bz_getPlayerBySlotOrCallsign(victimID);
        
        if (playerRecord)
        {
        	bz_sendTextMessagef(BZ_SERVER, playerID, "Logged usernames under ip address %s:", playerRecord->ipAddress.c_str());
        
        	for (int i = 0; i < lookupMap[playerRecord->ipAddress].size(); i++)
        	{
        		bz_sendTextMessagef(BZ_SERVER, playerID, "- %s", lookupMap[playerRecord->ipAddress][i].c_str());
        	}
        }
        else
        {
        	bz_sendTextMessagef(BZ_SERVER, playerID, "player \"%s\" not found", victimID);
        }
		
		bz_freePlayerRecord(playerRecord);
		
		return true;	
	}
	
	return false;
}




