/*
    Copyright (C) 2016 Vladimir "allejo" Jimenez
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#ifndef BZTOOLKIT_H
#define BZTOOLKIT_H

#include <cmath>
#include <iterator>
#include <random>
#include <sstream>
#include <time.h>

#include "/home/quinn/Documents/bzflag-2.4.25/bzflag-2.4/src/bzfs/bzToolkit.h"

// Plugin Naming + Versioning
extern std::string PLUGIN_NAME;
extern int MAJOR;
extern int MINOR;
extern int REV;
extern int BUILD;

const char* bztk_pluginName (void)
{
    static std::string pluginBuild = "";

    if (!pluginBuild.size())
    {
        std::ostringstream pluginBuildStream;

        pluginBuildStream << PLUGIN_NAME << " " << MAJOR << "." << MINOR << "." << REV << " (" << BUILD << ")";
        pluginBuild = pluginBuildStream.str();
    }

    return pluginBuild.c_str();
}

void bztk_forcePlayerSpawn (int playerID)
{
    forcePlayerSpawn(playerID);
}

bool bztk_isTeamFlag (std::string flagAbbr)
{
    return (flagAbbr == "R*" || flagAbbr == "G*" || flagAbbr == "B*" || flagAbbr == "P*");
}

const char* bztk_getFlagFromTeam(bz_eTeamType _team)
{
    switch (_team)
    {
        case eRedTeam:
            return "R*";

        case eGreenTeam:
            return "G*";

        case eBlueTeam:
            return "B*";

        case ePurpleTeam:
            return "P*";

        default:
            return "";
    }
}

bz_eTeamType bztk_getTeamFromFlag(std::string flagAbbr)
{
    if (bztk_isTeamFlag(flagAbbr))
    {
        if      (flagAbbr == "R*") return eRedTeam;
        else if (flagAbbr == "G*") return eGreenTeam;
        else if (flagAbbr == "B*") return eBlueTeam;
        else if (flagAbbr == "P*") return ePurpleTeam;
    }

    return eNoTeam;
}

void bztk_killAll(bz_eTeamType _team = eNoTeam, bool spawnOnBase = false, int killerID = -1, std::string flagID = NULL)
{
    // Create a list of players
    std::unique_ptr<bz_APIIntList> playerList(bz_getPlayerIndexList());

    // Be sure the playerlist exists
    if (playerList)
    {
        // Loop through all of the players' callsigns
        for (unsigned int i = 0; i < playerList->size(); i++)
        {
            // If the team isn't specified, then kill all of the players
            if (_team == eNoTeam)
            {
                bz_killPlayer(playerList->get(i), spawnOnBase, killerID, flagID.c_str());
            }
            // Kill only the players belonging to the specified team
            else if (bz_getPlayerTeam(playerList->get(i)) == _team)
            {
                bz_killPlayer(playerList->get(i), spawnOnBase, killerID, flagID.c_str());
            }
        }
    }
}

int bztk_getPlayerCount(bool observers = false)
{
    return (bz_getTeamCount(eRogueTeam) +
            bz_getTeamCount(eRedTeam) +
            bz_getTeamCount(eGreenTeam) +
            bz_getTeamCount(eBlueTeam) +
            bz_getTeamCount(ePurpleTeam) +
            bz_getTeamCount(eRabbitTeam) +
            bz_getTeamCount(eHunterTeam) +
            (observers ? bz_getTeamCount(eObservers) : 0));
}

bool bztk_anyPlayers(bool observers = false)
{
    return (bool)(bztk_getPlayerCount(observers));
}

const char* bztk_eTeamTypeLiteral(bz_eTeamType _team)
{
    switch (_team)
    {
        case eNoTeam:
            return "No";

        case eRogueTeam:
            return "Rogue";

        case eRedTeam:
            return "Red";

        case eGreenTeam:
            return "Green";

        case eBlueTeam:
            return "Blue";

        case ePurpleTeam:
            return "Purple";

        case eRabbitTeam:
            return "Rabbit";

        case eHunterTeam:
            return "Hunter";

        case eObservers:
            return "Observer";

        case eAdministrators:
            return "Administrator";

        default:
            return "No";
    }
}

bz_eTeamType bztk_eTeamType(std::string teamColor)
{
    teamColor = bz_tolower(teamColor.c_str());

    if (teamColor == "rogue")
    {
        return eRogueTeam;
    }
    else if (teamColor == "red")
    {
        return eRedTeam;
    }
    else if (teamColor == "green")
    {
        return eGreenTeam;
    }
    else if (teamColor == "blue")
    {
        return eBlueTeam;
    }
    else if (teamColor == "purple")
    {
        return ePurpleTeam;
    }
    else if (teamColor == "rabbit")
    {
        return eRabbitTeam;
    }
    else if (teamColor == "hunter")
    {
        return eHunterTeam;
    }
    else if (teamColor == "observer")
    {
        return eObservers;
    }
    else if (teamColor == "administrator")
    {
        return eAdministrators;
    }
    else
    {
        return eNoTeam;
    }
}

void bztk_foreachPlayer(void (*function)(int))
{
    std::shared_ptr<bz_APIIntList> playerList(bz_getPlayerIndexList());

    for (unsigned int i = 0; i < playerList->size(); i++)
    {
        (*function)(playerList->get(i));
    }
}

bz_BasePlayerRecord* bztk_getPlayerByBZID(int BZID)
{
    std::shared_ptr<bz_APIIntList> playerList(bz_getPlayerIndexList());

    for (unsigned int i = 0; i < playerList->size(); i++)
    {
        if (bz_getPlayerByIndex(playerList->get(i))->bzID == intToString(BZID))
        {
            int playerID = playerList->get(i);

            return bz_getPlayerByIndex(playerID);
        }
    }

    return NULL;
}

bool bztk_changeTeam(int playerID, bz_eTeamType _team)
{
    GameKeeper::Player *playerData = GameKeeper::Player::getPlayerByIndex(playerID);

    if (!playerData)
    {
        bz_debugMessagef(2, "bzToolkit -> bztk_changeTeam() :: Player ID %d not found.", playerID);
        return false;
    }
    else if ((_team != eRogueTeam)  && (_team != eRedTeam)  &&
             (_team != eGreenTeam)  && (_team != eBlueTeam) &&
             (_team != ePurpleTeam) && (_team != eObservers))
    {
        bz_debugMessagef(2, "bzToolkit -> bztk_changeTeam() :: Warning! Players cannot be swapped to the %s team through this function.", bztk_eTeamTypeLiteral(_team));
        return false;
    }
    else if (bz_getTeamPlayerLimit(_team) <= 0)
    {
        bz_debugMessagef(2, "bzToolkit -> bztk_changeTeam() :: Warning! The %s team does not exist on this server.");
        return false;
    }

    // No need to change them if they're in the same team they're being moved to
    if (playerData->player.getTeam() == eTeamTypeToTeamColor(_team))
    {
        return false;
    }

    // If the player is being moved to the observer team, we need to kill them so they can't pause/shoot while in observer
    if (_team == eObservers)
    {
        bz_killPlayer(playerID, false);
        playerData->player.setDead();
    }

    removePlayer(playerID);

    // If the player is currently an observer, we need to prevent them from getting idle kicked. A player's idle time can only
    // be updated when they're alive and they can only spawn when they're marked as dead.
    if (playerData->player.getTeam() == ObserverTeam)
    {
        playerData->player.setAlive();
        playerData->player.updateIdleTime();
        playerData->player.setDead();
    }

    playerData->player.setTeam(eTeamTypeToTeamColor(_team));

    addPlayer(playerData);
    sendPlayerInfo();
    sendIPUpdate(-1, playerID);

    return true;
}

bz_APIIntList* bztk_getTeamPlayerIndexList(bz_eTeamType _team)
{
    std::shared_ptr<bz_APIIntList> playerList(bz_getPlayerIndexList());

    bz_APIIntList* resp = bz_newIntList();

    for (unsigned int i = 0; i < playerList->size(); i++)
    {
        if (bz_getPlayerTeam(playerList->get(i)) == _team)
        {
          resp->push_back(playerList->get(i));
        }
    }

    return resp;
}

bool bztk_isValidPlayerID(int playerID)
{
    // Use another smart pointer so we don't forget about freeing up memory
    std::shared_ptr<bz_BasePlayerRecord> playerData(bz_getPlayerByIndex(playerID));

    // If the pointer doesn't exist, that means the playerID does not exist
    return (playerData) ? true : false;
}

int bztk_randomPlayer(bz_eTeamType _team = eNoTeam)
{
    srand(time(NULL));

    if (_team == eNoTeam)
    {
        if (bztk_anyPlayers())
        {
            bz_APIIntList *playerlist = bz_getPlayerIndexList();
            int picked = (*playerlist)[rand()%playerlist->size()];
            bz_deleteIntList(playerlist);

            return picked;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if (bz_getTeamCount(_team) > 0)
        {
            int picked = 0;
            bz_APIIntList* playerlist = bz_getPlayerIndexList();

            while (true)
            {
                picked = rand() % playerlist->size();

                if (bz_getPlayerTeam(picked) == _team)
                {
                    break;
                }
            }

            bz_deleteIntList(playerlist);

            return picked;
        }
        else
        {
            return -1;
        }
    }
}

bool bztk_registerCustomBoolBZDB(const char* bzdbVar, bool value, int perms = 0, bool persistent = false)
{
    if (!bz_BZDBItemExists(bzdbVar))
    {
        bz_setBZDBBool(bzdbVar, value, perms, persistent);
        return value;
    }

    return bz_getBZDBBool(bzdbVar);
}

double bztk_registerCustomDoubleBZDB(const char* bzdbVar, double value, int perms = 0, bool persistent = false)
{
    if (!bz_BZDBItemExists(bzdbVar))
    {
        bz_setBZDBDouble(bzdbVar, value, perms, persistent);
        return value;
    }

    return bz_getBZDBDouble(bzdbVar);
}

int bztk_registerCustomIntBZDB(const char* bzdbVar, int value, int perms = 0, bool persistent = false)
{
    if (!bz_BZDBItemExists(bzdbVar))
    {
        bz_setBZDBInt(bzdbVar, value, perms, persistent);
        return value;
    }

    return bz_getBZDBInt(bzdbVar);
}

const char* bztk_registerCustomStringBZDB(const char* bzdbVar, const char* value, int perms = 0, bool persistent = false)
{
    if (!bz_BZDBItemExists(bzdbVar))
    {
        bz_setBZDBString(bzdbVar, value, perms, persistent);
        return value;
    }

    return bz_getBZDBString(bzdbVar).c_str();
}

template<typename Iter, typename RandomGenerator>
Iter bztk_select_randomly(Iter start, Iter end, RandomGenerator& g)
{
    std::uniform_int_distribution<> dis(0, (int)(std::distance(start, end) - 1));
    std::advance(start, dis(g));

    return start;
}

template<typename Iter>
Iter bztk_select_randomly(Iter start, Iter end)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());

    return bztk_select_randomly(start, end, gen);
}

void bztk_fileToVector (const char* filePath, std::vector<std::string> &storage)
{
    std::ifstream file(filePath);
    std::string str;

    while (std::getline(file, str))
    {
        if (str.empty())
        {
            str = " ";
        }

        storage.push_back(str);
    }
}

void bztk_sendToPlayers (bz_eTeamType _team, std::string message)
{
    bz_APIIntList *playerList = bz_newIntList();
    bz_getPlayerIndexList(playerList);

    for (unsigned int i = 0; i < playerList->size(); i++)
    {
        int playerID = playerList->get(i);

        if (bz_getPlayerByIndex(playerID)->team == _team)
        {
            bz_sendTextMessagef(playerID, playerID, message.c_str());
        }
    }

    bz_deleteIntList(playerList);
}

#endif
