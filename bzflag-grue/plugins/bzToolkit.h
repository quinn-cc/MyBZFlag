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

#include <sstream>

#include "/home/quinn/Documents/bzflag-2.4.25/bzflag-2.4/src/bzfs/GameKeeper.h"
#include "/home/quinn/Documents/bzflag-2.4.25/bzflag-2.4/src/bzfs/bzfs.h"
#include "/home/quinn/Documents/bzflag-2.4.25/bzflag-2.4/src/bzfs/CmdLineOptions.h"

TeamColor eTeamTypeToTeamColor (bz_eTeamType _team)
{
    switch (_team)
    {
        case eRogueTeam:
            return RogueTeam;

        case eRedTeam:
            return RedTeam;

        case eGreenTeam:
            return GreenTeam;

        case eBlueTeam:
            return BlueTeam;

        case ePurpleTeam:
            return PurpleTeam;

        case eObservers:
            return ObserverTeam;

        case eHunterTeam:
            return HunterTeam;

        case eRabbitTeam:
            return RabbitTeam;

        default:
            return NoTeam;
    }
}

void fixTeamCount()
{
    int playerIndex, teamNum;

    for (teamNum = RogueTeam; teamNum < HunterTeam; teamNum++)
    {
        team[teamNum].team.size = 0;
    }

    for (playerIndex = 0; playerIndex < curMaxPlayers; playerIndex++)
    {
        GameKeeper::Player *p = GameKeeper::Player::getPlayerByIndex(playerIndex);

        if (p && p->player.isPlaying())
        {
            teamNum = p->player.getTeam();

            if (teamNum == HunterTeam)
                teamNum = RogueTeam;

            team[teamNum].team.size++;
        }
    }
}

void removePlayer(int playerIndex)
{
    GameKeeper::Player *playerData = GameKeeper::Player::getPlayerByIndex(playerIndex);

    if (!playerData)
        return;

    void *buf, *bufStart = getDirectMessageBuffer();
    buf = nboPackUByte(bufStart, playerIndex);

    broadcastMessage(MsgRemovePlayer, (char*)buf-(char*)bufStart, bufStart);

    int teamNum = int(playerData->player.getTeam());
    --team[teamNum].team.size;
    sendTeamUpdate(-1, teamNum);
    fixTeamCount();
}

void addPlayer(GameKeeper::Player *playerData)
{
    void *bufStart = getDirectMessageBuffer();
    void *buf      = playerData->packPlayerUpdate(bufStart);

    broadcastMessage(MsgAddPlayer, (char*)buf - (char*)bufStart, bufStart);

    int teamNum = int(playerData->player.getTeam());
    team[teamNum].team.size++;
    sendTeamUpdate(-1, teamNum);
    fixTeamCount();
}

void forcePlayerSpawn(int playerID)
{
    playerAlive(playerID);
}

std::string intToString(int number)
{
    std::stringstream string;
    string << number;

    return string.str();
}
