/*
 * Must keep a file called lookup.txt
 * 
 * FIX ME: lookup path does not work, it is hardcoded and MUST be at ../lookup.txt
 *
 * ./configure --enable-custom-plugins=lookup
 */

#include "bzfsAPI.h"
#include <regex>
#include <iostream>
#include <fstream>
#include <string>

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
    ~Lookup() {};

    virtual void Cleanup(void)
    {
    	bz_removeCustomSlashCommand("lookup");
        bz_removeCustomSlashCommand("clearLookupCache");
        bz_removeCustomSlashCommand("cacheLookup");
        Flush();
    }
};

double lastLookupWriteTime = 0;
// ipAddress : { name: #, name: # ... }
std::map<std::string, std::map<std::string, int>> lookupMap;
BZ_PLUGIN(Lookup)


void Lookup::Init(const char*)
{
    Register(bz_ePlayerJoinEvent);
    Register(bz_eTickEvent);
    Register(bz_eWorldFinalized);
    bz_registerCustomSlashCommand("lookup", &lookupCommand);
    bz_registerCustomSlashCommand("clearLookupCache", &lookupCommand);
    bz_registerCustomSlashCommand("cacheLookup", &lookupCommand);
    bz_registerCustomBZDBDouble("_lookupCacheInterval", 60); // In minutes
    bz_registerCustomBZDBString("_lookupPath", "lookup.txt");
}

void writeLookupMap()
{
    ofstream file;
    file.open(bz_getBZDBString("_lookupPath"));

    if (!file)
    {
        return;
    }

    file.clear();

    for (auto ipPair : lookupMap)
    {
        file << "IP:" + ipPair.first;
        file << "\n";

        for (auto callsignPair : ipPair.second)
        {
            file << callsignPair.first + ":";
            file << callsignPair.second;
            file << "\n";
        }
    }
    
    file.close();
    lastLookupWriteTime = bz_getCurrentTime();
}

bool LookupCommand::SlashCommand (int playerID, bz_ApiString command, bz_ApiString, bz_APIStringList* params)
{
	if (command == "lookup")
	{
        bz_BasePlayerRecord *commander = bz_getPlayerByIndex(playerID);

        if (commander->admin)
        {
            if (params->size() != 1)
            {
                bz_sendTextMessage(BZ_SERVER, playerID, "format: /lookup <player ID|callsign>");
                return true;
            }
            
            const char* victim = params->get(0).c_str();
            bz_BasePlayerRecord *playerRecord = bz_getPlayerBySlotOrCallsign(victim);
            
            
            if (playerRecord)
            {
                bz_sendTextMessagef(BZ_SERVER, playerID, "Logged usernames under ip address %s:", playerRecord->ipAddress.c_str());
            
                for (auto pair : lookupMap[playerRecord->ipAddress])
                {
                    bz_sendTextMessagef(BZ_SERVER, playerID, "- %s (%d)", pair.first.c_str(), pair.second);
                }
            }
            else
            {
                bz_sendTextMessagef(BZ_SERVER, playerID, "player \"%s\" not found", victim);
            }
            
            bz_freePlayerRecord(playerRecord);
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "You are not authorized to use this command.");
        }
        
        bz_freePlayerRecord(commander);

		return true;	
	}
    else if (command == "clearLookupCache")
    {
        if (bz_hasPerm(playerID, "shutdownServer"))
        {
            lookupMap.clear();
            writeLookupMap();
            bz_sendTextMessage(BZ_SERVER, playerID, "The lookup has been cleared.");
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "You are not authorized to use this command.");
        }
        
        return true;
    }
    else if (command == "cacheLookup")
    {
        if (bz_hasPerm(playerID, "shutdownServer"))
        {
            writeLookupMap();
            bz_sendTextMessage(BZ_SERVER, playerID, "The lookup has been cached.");
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "You are not authorized to use this command.");
        }

        return true;
    }
	
	return false;
}

void Lookup::Event(bz_EventData *eventData)
{
    switch (eventData->eventType)
	{
        case bz_eWorldFinalized:
        {
            ifstream file(bz_getBZDBString("_lookupPath"));
            string line;
            string ipAddress = "";

            while (getline(file, line))
            {
                if (line.length() > 2)
                {
                    string header = line.substr(0,3);

                    if (strcmp(header.c_str(), "IP:") == 0)
                    {
                        ipAddress = line.substr(3, line.length() - 3);
                    }
                    else
                    {
                        string callsign = line.substr(0, line.find_last_of(":"));
                        int num = stoi(line.substr(line.find_last_of(":") + 1, line.length()));

                        lookupMap[ipAddress][callsign] = num;
                    }
                }
            }
            
            file.close();
        } break;
		case bz_ePlayerJoinEvent:
		{
            bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;
            string ipAddress = bz_getPlayerIPAddress(data->playerID);
            string callsign = bz_getPlayerCallsign(data->playerID);

            if (lookupMap.count(ipAddress) && lookupMap[ipAddress].count(callsign))
                lookupMap[ipAddress][callsign]++;
            else
                lookupMap[ipAddress][callsign] = 1;
		} break;
        case bz_eTickEvent:
        {
            if (bz_getCurrentTime() - lastLookupWriteTime > bz_getBZDBDouble("_lookupCacheInterval") * 60)
            {
                writeLookupMap();
            }
        } break;
        default:
            break;
    }
}


