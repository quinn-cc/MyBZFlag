// class-interface header
#include "../../src/bzfs/WorldWeapons.h"

#include "../../src/bzfs/WorldInfo.h"
// system headers
#include <vector>

#include "bzfsAPI.h"

// bzfs specific headers
#include "../../src/bzfs/bzfs.h"
#include "../../src/bzfs/ShotManager.h"

uint32_t BZGR_fireShot(FlagType* type, const float origin[3], const float vector[3], int *shotID,
                                TeamColor teamColor, PlayerId targetPlayerID)
{
    
    if (!BZDB.isTrue(StateDatabase::BZDB_WEAPONS))
        return INVALID_SHOT_GUID;

    void *buf, *bufStart = getDirectMessageBuffer();

    FiringInfo firingInfo;
    firingInfo.timeSent = (float)TimeKeeper::getCurrent().getSeconds();
    firingInfo.flagType = type;
    firingInfo.lifetime = BZDB.eval(StateDatabase::BZDB_RELOADTIME);
    firingInfo.shot.player = ServerPlayer;
    memmove(firingInfo.shot.pos, origin, 3 * sizeof(float));

    const float shotSpeed = BZDB.eval(StateDatabase::BZDB_SHOTSPEED);

    for (int i = 0; i < 3; i++)
        firingInfo.shot.vel[i] = vector[i] * shotSpeed;

    firingInfo.shot.dt = 0;
    firingInfo.shot.team = teamColor;

    if (shotID != nullptr && shotID == 0)
    {
        *shotID = 0;
        firingInfo.shot.id = *shotID;
    }
    else if (shotID == nullptr)
        firingInfo.shot.id = 1;
    else
        firingInfo.shot.id = *shotID;

    bz_AllowServerShotFiredEventData_V1 allowEvent;
    allowEvent.flagType = type->flagAbbv;
    allowEvent.speed = shotSpeed;
    for (int i = 0; i < 3; i++)
    {
        allowEvent.pos[i] = origin[i];
        allowEvent.velocity[i] = firingInfo.shot.vel[i];
    }
    allowEvent.team = convertTeam(teamColor);

    worldEventManager.callEvents(bz_eAllowServerShotFiredEvent, &allowEvent);

    if (!allowEvent.allow)
        return INVALID_SHOT_GUID;

    if (allowEvent.changed)
    {
        FlagTypeMap &flagMap = FlagType::getFlagMap();

        if (flagMap.find(allowEvent.flagType) == flagMap.end())
            return INVALID_SHOT_GUID;

        FlagType *flag = flagMap.find(allowEvent.flagType)->second;
        firingInfo.flagType = flag;
    }

    buf = firingInfo.pack(bufStart);

    broadcastMessage(MsgShotBegin, (char*)buf - (char*)bufStart, bufStart);

    uint32_t shotGUID = ShotManager.AddShot(firingInfo, ServerPlayer);

    // Target the gm, construct it, and send packet
    if (type->flagAbbv == "GM")
    {
        ShotManager.SetShotTarget(shotGUID, targetPlayerID);

        char packet[ShotUpdatePLen + PlayerIdPLen];
        buf = (void*)packet;
        buf = firingInfo.shot.pack(buf);
        buf = nboPackUByte(buf, targetPlayerID);

        broadcastMessage(MsgGMUpdate, sizeof(packet), packet);
    }

    bz_ServerShotFiredEventData_V1 event;
    event.guid = shotGUID;
    event.flagType = allowEvent.flagType;
    event.speed = shotSpeed;
    for (int i = 0; i < 3; i++)
    {
        event.pos[i] = origin[i];
        event.velocity[i] = firingInfo.shot.vel[i];
    }
    event.team = convertTeam(teamColor);

    worldEventManager.callEvents(bz_eServerShotFiredEvent, &event);

    return shotGUID;
}
