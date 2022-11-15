#include "bzfsAPI.h"

#include "../../src/bzfs/bzfs.h"
#include "../../src/bzfs/WorldWeapons.h"
#include "WorldEventManager.h"
#include "../../src/bzfs/GameKeeper.h"
#include "../../src/bzfs/FlagInfo.h"

uint32_t bzgr_fireServerShot(const char* shotType, float lifetime, float speed, float origin[3], float vector[3], bz_eTeamType color,
                                   int targetPlayerId = -1)
{
    if (!shotType || !origin)
        return INVALID_SHOT_GUID;

    std::string flagType = shotType;
    FlagTypeMap &flagMap = FlagType::getFlagMap();

    if (flagMap.find(flagType) == flagMap.end())
        return INVALID_SHOT_GUID;

    FlagType *flag = flagMap.find(flagType)->second;

    return world->getWorldWeapons().fireShotLifetime(flag, lifetime, speed, origin, vector, nullptr, (TeamColor)convertTeam(color), targetPlayerId);
}