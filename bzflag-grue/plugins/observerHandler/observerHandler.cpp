/*
 * ./configure --enable-custom-plugins=observerHandler
 */

#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <map>

using namespace std;

class ObserverHandlerCommands : public bz_CustomSlashCommandHandler
{
public:
    virtual ~ObserverHandlerCommands() {};
    virtual bool SlashCommand (int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList *param);
};

ObserverHandlerCommands observerHandlerCommands;

class ObserverHandler : public bz_Plugin {
    virtual const char* Name()
    {
        return "Observer Handler";
    }

    virtual void Init(const char*);
    virtual void Event(bz_EventData*);
    ~ObserverHandler() {};

    virtual void Cleanup(void)
    {
        bz_removeCustomSlashCommand("join");
        bz_removeCustomSlashCommand("leave");
        Flush();
    }
};

BZ_PLUGIN(ObserverHandler)

void ObserverHandler::Init (const char*)
{
    Register(bz_ePlayerPausedEvent);
    Register(bz_eTickEvent);
    Register(bz_ePlayerJoinEvent);
    bz_registerCustomSlashCommand("join", &observerHandlerCommands);
    bz_registerCustomSlashCommand("leave", &observerHandlerCommands);
    bz_registerCustomBZDBBool("_noPausing", true);
    bz_registerCustomBZDBDouble("_idleTimeToObserver", 60);
}

void ObserverHandler::Event(bz_EventData* eventData)
{
    switch (eventData->eventType)
    {
        case bz_ePlayerPausedEvent:
        {
            bz_PlayerPausedEventData_V1* data = (bz_PlayerPausedEventData_V1*) eventData;

            if (data->pause && bz_getBZDBBool("_noPausing"))
            {
                bz_changeTeam(data->playerID, eObservers);
                bz_sendTextMessage(BZ_SERVER, data->playerID, "This is a high-stakes game, no pausing allowed. You have been switched to Observer instead.");
                bz_sendTextMessage(BZ_SERVER, data->playerID, "You may join back in the game anytime by typing /join");
            }
        } break;
        case bz_eTickEvent:
        {
            bz_APIIntList* playerList = bz_getPlayerIndexList();

            for (int i = 0; i < playerList->size(); i++)
            {
                int playerID = playerList->get(i);

                if (bz_getIdleTime(playerID) > bz_getBZDBDouble("_idleTimeToObserver"))
                {
                    bz_changeTeam(playerID, eObservers);
                    bz_sendTextMessage(BZ_SERVER, playerID, "This is a high-stakes game, do not remain idle for long. You have been switched to Observer.");
                    bz_sendTextMessage(BZ_SERVER, playerID, "You may join back in the game anytime by typing /join");
                }
            }

            delete playerList;
        } break;
        case bz_ePlayerJoinEvent:
		{
			bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;

            if (bz_getPlayerTeam(data->playerID) == eObservers)
                bz_sendTextMessage(BZ_SERVER, data->playerID, "Welcome to Observer! Feel free to join the game anytime by typing /join");
            else
                bz_sendTextMessage(BZ_SERVER, data->playerID, "If you want a break, type /leave to switch to Observer.");
		} break;
        default:
            break;
    }
}

bool ObserverHandlerCommands::SlashCommand (int playerID, bz_ApiString command, bz_ApiString, bz_APIStringList* params)
{
    if (command == "join")
    {
        if (bz_getPlayerTeam(playerID) == eObservers)
        {
            bz_eTeamType team = bz_getUnbalancedTeam(eRedTeam, eGreenTeam);
            bz_changeTeam(playerID, team);

            bz_sendTextMessagef(BZ_SERVER, playerID, "You have joined the %s team", bz_eTeamTypeLiteral(team));
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "You are already in the game!");
        }

        return true;
    }
    else if (command == "leave")
    {
        if (bz_getPlayerTeam(playerID) != eObservers)
        {
            bz_BasePlayerRecord* record = bz_getPlayerByIndex(playerID);

            if (record->spawned)
            {
                bz_sendTextMessage(BZ_SERVER, playerID, "You must be dead to leave to observer.");
            }
            else
            {
                bz_changeTeam(playerID, eObservers);
                bz_sendTextMessage(BZ_SERVER, playerID, "You have switched to Observer. Feel free to jump back in by typing /join");
            }

            bz_freePlayerRecord(record);
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "You aren't in the game to begin with!");
        }

        return true;
    }

    return false;
}