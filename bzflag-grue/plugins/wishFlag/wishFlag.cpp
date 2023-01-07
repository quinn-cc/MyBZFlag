/*
 * Custom flag: Wish (+W)
 * Grabbing this flag allows you to /wish for any flag.
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=wishFlag
 */

#include "bzfsAPI.h"
#include "plugin_utils.h"
#include <string>

class WishCommand : public bz_CustomSlashCommandHandler
{
public:
    virtual ~WishCommand() {};
    virtual bool SlashCommand (int playerID, bz_ApiString command, bz_ApiString message, bz_APIStringList *param);
};

WishCommand wishCommand;

class WishFlag : public bz_Plugin
{
    virtual const char* Name()
	{
		return "Wish Flag";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~WishFlag() {};

	virtual void Cleanup(void)
	{
        bz_removeCustomSlashCommand("wish");
		Flush();
	}

    void obtainedFlag(int playerID);
};

void WishFlag::obtainedFlag(int playerID)
{
    bz_sendTextMessage(BZ_SERVER, playerID, "Type /wish ##, where ## is any flag's abbreviation.");
    bz_sendTextMessage(BZ_SERVER, playerID, "For example, /wish QT will give the Quick Turn flag.");
}

BZ_PLUGIN(WishFlag)

void WishFlag::Init (const char*)
{
	bz_RegisterCustomFlag("W", "Wish", "Wish for anything and make it count!", 0, eGoodFlag);
    bz_registerCustomSlashCommand("wish", &wishCommand);

    bz_registerCustomBZDBDouble("_wishPowerfulError", 0.9);
    bz_registerCustomBZDBDouble("_wishForWishError", 1);
    bz_registerCustomBZDBDouble("_wishRegularError", 0.1);

    Register(bz_eFlagGrabbedEvent);
    Register(bz_eFlagTransferredEvent);
}

void WishFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
    {
        case bz_eFlagGrabbedEvent:
        {
            bz_FlagGrabbedEventData_V1* data = (bz_FlagGrabbedEventData_V1*) eventData;

            if (strcmp(data->flagType, "W") == 0)
                obtainedFlag(data->playerID);
        } break;
        case bz_eFlagTransferredEvent:
        {
            bz_FlagTransferredEventData_V1* data = (bz_FlagTransferredEventData_V1*) eventData;

            if (strcmp(data->flagType, "W") == 0)
                obtainedFlag(data->toPlayerID);
        } break;
        default:
            break;
    }
}

bool WishCommand::SlashCommand(int playerID, bz_ApiString command, bz_ApiString message,
                                    bz_APIStringList* params)
{
	if (command == "wish")
	{
        if (bz_getPlayerFlagAbbr(playerID) == "W")
        {
            int wishFlagID = bz_getPlayerFlagID(playerID);
            const char* flagWished = params->get(0).c_str();
            flagWished = bz_toupper(flagWished);

            bool error;
            bool success = false;

            if (strcmp("US", flagWished) == 0)
            {
                bz_sendTextMessage(BZ_SERVER, playerID, "Here's a useless flag for you.");

                const char* uselessFlags[6] = { "US", "MQ", "DB", "AC", "RR", "TA" };
                srand(bz_getCurrentTime());
                int flagIndex = rand() % 6;
                bz_givePlayerFlag(playerID, uselessFlags[flagIndex], true);

                error = false;
                success = true;
            }
            else if (strcmp("W", flagWished) == 0)
            {
                error = bz_randFloatBetween(0, 1) < bz_getBZDBDouble("_wishForWishError");

                if (error)
                    bz_sendTextMessage(BZ_SERVER, playerID, "You can't wish for more wishes! Aladdin 101... you have been cursed.");
                else
                    bz_sendTextMessage(BZ_SERVER, playerID, "You wished for another wish? Why?");
            }
            else if (strcmp("RR", flagWished) == 0 || strcmp("DB", flagWished) == 0 ||
                    strcmp("AC", flagWished) == 0 || strcmp("MQ", flagWished) == 0)
            {
                bz_sendTextMessage(BZ_SERVER, playerID, "You had one wish, and you wished for that??");
                error = false;
            }
            else if (strcmp("L", flagWished) == 0 || strcmp("GN", flagWished) == 0 ||
                    strcmp("GM", flagWished) == 0 || strcmp("AN", flagWished) == 0 ||
                    strcmp("FP", flagWished) == 0 || bz_isTeamFlag(flagWished))
            {
                error = bz_randFloatBetween(0, 1) < bz_getBZDBDouble("_wishPowerfulError");

                if (error)
                    bz_sendTextMessage(BZ_SERVER, playerID, "POWERFUL WISHES HAVE POWERFUL CONSEQUENCES");
                else
                    bz_sendTextMessage(BZ_SERVER, playerID, "It's your lucky day.");
            }
            else
            {
                error = bz_randFloatBetween(0, 1) < bz_getBZDBDouble("_wishRegularError");

                if (error)
                    bz_sendTextMessage(BZ_SERVER, playerID, "You don't always get what you want.");
            }

            // Got bad flag
            if (error)
            {
                const char* badFlags[14] = { "B", "BY", "CB", "FO", "JM", "LT", "M", "NJ", "O", "RC", "RO", "RT", "TR", "WA" };
                srand(bz_getCurrentTime());
                int flagIndex = rand() % 14;
                bz_givePlayerFlag(playerID, badFlags[flagIndex], true);
                bz_resetFlag(wishFlagID);
            }
            else
            {
                if (!success)
                    success = bz_givePlayerFlag(playerID, flagWished, true);

                if (success)
                {
                    bz_resetFlag(wishFlagID);
                }
                else
                {
                    bz_sendTextMessagef(BZ_SERVER, playerID, "The flag '%s' does not exist.", flagWished);
                    bz_sendTextMessage(BZ_SERVER, playerID, "The correct format is /wish ##, where ## is any flag's abbreviation.");
                    bz_sendTextMessage(BZ_SERVER, playerID, "For example, /wish QT will give the Quick Turn flag.");
                }
            }

            
        }
        else
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "No genie is here to save you today.");
        }

        return true;
    }

    return false;
}
