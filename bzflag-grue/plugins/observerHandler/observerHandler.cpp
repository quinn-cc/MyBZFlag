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
        Flush();
    }
    void sendPlayerToObserver(int playerID);
};

BZ_PLUGIN(ObserverHandler)

void ObserverHandler::Init (const char*)
{
    Register(bz_ePlayerPausedEvent);
    Register(bz_eTickEvent);
    Register(bz_ePlayerJoinEvent);
    bz_registerCustomSlashCommand("join", &observerHandlerCommands);
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
            bz_eTeamType team;

            if (bz_getTeamCount(eGreenTeam) < bz_getTeamCount(eRedTeam))
                team = eGreenTeam;
            else if (bz_getTeamCount(eGreenTeam) > bz_getTeamCount(eRedTeam))
                team = eRedTeam;
            else
                team = bz_randFloatBetween(0,1) < 0.5 ? eGreenTeam : eRedTeam;

            bz_changeTeam(playerID, team);
            bz_sendTextMessagef(BZ_SERVER, playerID, "You have joined the %s team", bz_eTeamTypeLiteral(team));
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "You are already in the game!");
        }

        return true;
    }

    return false;
}