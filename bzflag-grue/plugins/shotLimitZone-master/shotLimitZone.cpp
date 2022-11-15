/*
 * Copyright (C) 2013-2020 Vladimir "allejo" Jimenez
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <sstream>

#include "bzfsAPI.h"

// Define plugin name
const std::string PLUGIN_NAME = "Shot Limit Zones";

// Define plugin version numbering
const int MAJOR = 1;
const int MINOR = 0;
const int REV = 4;
const int BUILD = 37;
const std::string SUFFIX = "STABLE";

// Define build settings
const int VERBOSITY_LEVEL = 4;

class ShotLimitZone : public bz_CustomZoneObject
{
public:
    ShotLimitZone() : bz_CustomZoneObject()
    {
        shotLimit = 5;
    }

    int shotLimit;
    std::vector<std::string> flags;
};

class ShotLimitZonePlugin : public bz_Plugin, bz_CustomMapObjectHandler
{
public:
    virtual const char* Name ();
    virtual void Init (const char* config);
    virtual void Cleanup (void);
    virtual void Event (bz_EventData *eventData);

    virtual bool MapObject (bz_ApiString object, bz_CustomMapObjectInfo *data);

    // Store all the custom shot limit zones in a struct so we can loop through them
    std::vector<ShotLimitZone> slzs;

    // We'll be keeping track of how many shots a player has remaining in a single array
    // If the value is greater than -1, then that means the player has grabbed a flag
    // with a shot limit so we'll keep count of how many.
    int playerShotsRemaining[256];

    // We'll be keeping track if we need to send a player a message after their first shot to
    // show them that they have a limited number of shots. If the shot limit is 10 and this
    // value is set to true, the we will warn them at 9 shots remaining.
    // FIXME: Unfortunately this is not very useful if the shot limit is 1. Perhaps notify
    // of shot limit on grab rather than on first shot.
    bool firstShotWarning[256];
};

BZ_PLUGIN(ShotLimitZonePlugin)

const char* ShotLimitZonePlugin::Name (void)
{
    static const char *pluginBuild;

    if (!pluginBuild)
    {
        pluginBuild = bz_format("%s %d.%d.%d (%d)", PLUGIN_NAME.c_str(), MAJOR, MINOR, REV, BUILD);

        if (!SUFFIX.empty())
        {
            pluginBuild = bz_format("%s - %s", pluginBuild, SUFFIX.c_str());
        }
    }

    return pluginBuild;
}

void ShotLimitZonePlugin::Init(const char* /*commandLine*/)
{
    // Register our events
    Register(bz_eFlagDroppedEvent);
    Register(bz_eFlagGrabbedEvent);
    Register(bz_eFlagTransferredEvent);
    Register(bz_ePlayerDieEvent);
    Register(bz_ePlayerJoinEvent);
    Register(bz_eShotFiredEvent);

    // Register our custom BZFlag zones
    bz_registerCustomMapObject("ShotLimitZone", this);
}

void ShotLimitZonePlugin::Cleanup()
{
    // Remove all the events this plugin was watching for
    Flush();

    // Remove the custom BZFlag zones
    bz_removeCustomMapObject("ShotLimitZone");
}

bool ShotLimitZonePlugin::MapObject(bz_ApiString object, bz_CustomMapObjectInfo *data)
{
    // We only need to keep track of our special zones, so ignore everything else
    if (object != "SHOTLIMITZONE" || !data)
    {
        return false;
    }

    // We found one of our custom zones so create something we can push to the struct
    ShotLimitZone newSLZ;
    newSLZ.handleDefaultOptions(data);

    // Loop through the information in this zone
    for (unsigned int i = 0; i < data->data.size(); i++)
    {
        // Store the current line we're reading in an easy to access variable
        std::string temp = data->data.get(i).c_str();

        // Create a place to store the values of the custom zone block
        bz_APIStringList *values = bz_newStringList();

        // Tokenize each line by space
        values->tokenize(temp.c_str(), " ", 0, true);

        // If there is more than one value, that means they've put a field and a value
        if (values->size() > 0)
        {
            // Let's not care about case so just make everything lower case
            std::string checkval = bz_tolower(values->get(0).c_str());

            // Check if we found the shot limit specification
            if (checkval == "shotlimit" && values->size() > 1)
            {
                try
                {
                    newSLZ.shotLimit = std::stoi(values->get(1).c_str());
                }
                catch (std::exception const &e)
                {
                    bz_debugMessagef(0, "Found invalid ");
                }
            }

            // Check if we found the flag we'll be limiting
            if (checkval == "flag" && values->size() > 1)
            {
                newSLZ.flags.push_back(bz_toupper(values->get(1).c_str()));
            }
        }

        // Avoid memory leaks
        bz_deleteStringList(values);
    }

    // Add the information we got and add it to the struct
    slzs.push_back(newSLZ);

    // Return true because we handled the map object
    return true;
}

void ShotLimitZonePlugin::Event(bz_EventData *eventData)
{
    // Switch through the events we'll be watching
    switch (eventData->eventType)
    {
        case bz_eFlagDroppedEvent:
        {
            bz_FlagDroppedEventData_V1* flagDropData = (bz_FlagDroppedEventData_V1*)eventData;

            // The playerID here may be -1 if we just called bz_resetFlag or bz_removePlayerFlag. BZFS inadvertently triggers
            // another flag drop event due to only updating the flag status after callEvents.
            // Alternatively we could avoid this problem by calling those flag reset functions on subsequent event calls such as
            // on eTickEvent.
            if (flagDropData->playerID < 0)
            {
                return;
            }

            // If the player who dropped the flag had shots remaining with the flag, don't let them regrab it so reset the flag
            // as soon as they drop it
            if (playerShotsRemaining[flagDropData->playerID] > 0)
            {
                bz_resetFlag(flagDropData->flagID);
            }
        }
        break;

        case bz_eFlagGrabbedEvent:
        {
            bz_FlagGrabbedEventData_V1* flagData = (bz_FlagGrabbedEventData_V1*)eventData;

            playerShotsRemaining[flagData->playerID] = -1;
            firstShotWarning[flagData->playerID] = false;

            // Loop through all the shot limit zones that we have in memory to check if a flag was grabbed
            // inside of a zone
            for (unsigned int i = 0; i < slzs.size(); i++)
            {
                if (slzs[i].pointInZone(flagData->pos))
                {
                    for (auto &flag : slzs[i].flags)
                    {
                        if (bz_getFlagName(flagData->flagID).c_str() == flag)
                        {
                            // Keep track of shot limits here
                            playerShotsRemaining[flagData->playerID] = slzs[i].shotLimit;
                            firstShotWarning[flagData->playerID] = true;

                            bz_sendTextMessagef(BZ_SERVER, flagData->playerID, "%i shot%s left", slzs[i].shotLimit, (slzs[i].shotLimit > 1) ? "s" : "");
                            break;
                        }
                    }
                }
            }
        }
        break;

        case bz_eFlagTransferredEvent:
        {
            bz_FlagTransferredEventData_V1* transferredData = (bz_FlagTransferredEventData_V1*)eventData;

            if (playerShotsRemaining[transferredData->fromPlayerID] >= 0)
            {
                playerShotsRemaining[transferredData->toPlayerID] = playerShotsRemaining[transferredData->fromPlayerID];
                playerShotsRemaining[transferredData->fromPlayerID] = -1;
                firstShotWarning[transferredData->toPlayerID] = true;
                bz_sendTextMessagef(BZ_SERVER, transferredData->toPlayerID, "%i shot%s left", playerShotsRemaining[transferredData->toPlayerID], (playerShotsRemaining[transferredData->toPlayerID] > 1) ? "s" : "");
            }
        }
        break;

        case bz_ePlayerDieEvent: // This event is called each time a tank is killed.
        {
            bz_PlayerDieEventData_V1* dieData = (bz_PlayerDieEventData_V1*)eventData;

            playerShotsRemaining[dieData->playerID] = -1;
        }
        break;

        case bz_ePlayerJoinEvent: // This event is called each time a player joins the game
        {
            bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;

            int playerID = joinData->playerID;
            playerShotsRemaining[playerID] = -1;
        }
        break;

        case bz_eShotFiredEvent:
        {
            bz_ShotFiredEventData_V1* shotData = (bz_ShotFiredEventData_V1*)eventData;
            int playerID = shotData->playerID;

            // If player shots remaining is -1, then we don't have to keep count of them but otherwise we do
            if (playerShotsRemaining[playerID] >= 0)
            {
                // They fired a shot so let's decrement the remaining shots
                playerShotsRemaining[playerID]--;

                if (playerShotsRemaining[playerID] == 0)
                {
                    // Decrement the shot count so we can drop down to -1 so we can ignore it in the future
                    playerShotsRemaining[playerID]--;

                    // Take the player's flag
                    bz_removePlayerFlag(playerID);
                }
                else if ((playerShotsRemaining[playerID] % 5 == 0 || playerShotsRemaining[playerID] <= 3 || firstShotWarning[playerID]) && playerShotsRemaining[playerID] > 0)
                {
                    // If the shot count is less than or equal to 3, is divisable by 5, or it's their first shot
                    // after the flag grab, notify the player
                    bz_sendTextMessagef(BZ_SERVER, playerID, "%i shot%s left", playerShotsRemaining[playerID], (playerShotsRemaining[playerID] > 1) ? "s" : "");

                    // If we have sent their first warning, then let's forget about it
                    if (firstShotWarning[playerID])
                    {
                        firstShotWarning[playerID] = false;
                    }
                }
            }
        }
        break;

        default: break;
    }
}
