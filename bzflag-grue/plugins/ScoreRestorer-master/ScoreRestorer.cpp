/*
 * Copyright (C) 2019 Vladimir "allejo" Jimenez
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

#include <map>

#include "bzfsAPI.h"

// Define plugin name
const std::string PLUGIN_NAME = "Score Restorer";

const int MAJOR = 1;
const int MINOR = 1;
const int REV = 1;
const int BUILD = 21;

class ScoreRestorer : public bz_Plugin
{
public:
    virtual const char* Name ();
    virtual void Init (const char* config);
    virtual void Event (bz_EventData *eventData);
    virtual void Cleanup (void);

private:
    struct PlayerRecord
    {
        std::string callsign;
        std::string ipAddress;

        int wins;
        int losses;
        int teamKills;

        double expireTime;
    };

    std::map<std::string, PlayerRecord> savedRecords;
    std::string bzdb_saveTimeVar;
};

BZ_PLUGIN(ScoreRestorer)

const char* ScoreRestorer::Name (void)
{
    static std::string pluginName;

    if (pluginName.empty())
    {
        pluginName = bz_format("%s %d.%d.%d (%d)", PLUGIN_NAME.c_str(), MAJOR, MINOR, REV, BUILD);
    }

    return pluginName.c_str();
}

void ScoreRestorer::Init (const char* /*commandLine*/)
{
    Register(bz_ePlayerJoinEvent);
    Register(bz_ePlayerPartEvent);

    bzdb_saveTimeVar = "_scoreSaveTime";

    bz_registerCustomBZDBInt(bzdb_saveTimeVar.c_str(), 120);
}

void ScoreRestorer::Cleanup (void)
{
    Flush();

    bz_removeCustomBZDBVariable(bzdb_saveTimeVar.c_str());
}

void ScoreRestorer::Event (bz_EventData *eventData)
{
    switch (eventData->eventType)
    {
        case bz_ePlayerJoinEvent:
        {
            bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;

            std::string playerCallsign = bz_tolower(joinData->record->callsign.c_str());

            // Check if we have a record
            if (savedRecords.count(playerCallsign))
            {
                PlayerRecord &record = savedRecords[playerCallsign];

                // Make sure the record hasn't expired
                if (record.expireTime + bz_getBZDBDouble(bzdb_saveTimeVar.c_str()) > bz_getCurrentTime())
                {
                    // Verify their identity by checking the IP address
                    if (joinData->record->ipAddress == record.ipAddress)
                    {
                        if (joinData->record->team == eObservers)
                        {
                            bz_sendTextMessage(BZ_SERVER, joinData->playerID, "Your score record will be saved while you are in observer mode.");
                        }
                        else
                        {
                            bz_setPlayerWins(joinData->playerID, record.wins);
                            bz_setPlayerLosses(joinData->playerID, record.losses);
                            bz_setPlayerTKs(joinData->playerID, record.teamKills);

                            bz_sendTextMessage(BZ_SERVER, joinData->playerID, "Your score has been restored.");

                            // Erase the record because we've used it. When this player leaves, a new record will be
                            // saved for them.
                            savedRecords.erase(playerCallsign);
                        }
                    }
                }
                else
                {
                    // Erase the record since it's been too long since their last join
                    savedRecords.erase(playerCallsign);
                }
            }
        }
        break;

        case bz_ePlayerPartEvent:
        {
            bz_PlayerJoinPartEventData_V1* partData = (bz_PlayerJoinPartEventData_V1*)eventData;

            // We'll store callsigns in lower case for sanity's sake
            std::string playerCallsign = bz_tolower(partData->record->callsign.c_str());

            // If an observer has a saved scored, extend their score expiration from the time they left as an observer
            if (savedRecords.count(playerCallsign) > 0 && partData->record->team == eObservers)
            {
                savedRecords[playerCallsign].expireTime = bz_getCurrentTime();
            }
            else
            {
                PlayerRecord newRecord;

                newRecord.callsign = playerCallsign;
                newRecord.ipAddress = partData->record->ipAddress;
                newRecord.wins = partData->record->wins;
                newRecord.losses = partData->record->losses;
                newRecord.teamKills = partData->record->teamKills;
                newRecord.expireTime = bz_getCurrentTime();

                // Only save a record if they've actually got kills and deaths to save
                if (newRecord.wins != 0 || newRecord.losses != 0)
                {
                    savedRecords[playerCallsign] = newRecord;
                }
            }
        }
        break;

        default: break;
    }
}
