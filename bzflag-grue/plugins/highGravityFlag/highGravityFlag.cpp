/*
 * Custom flag: High Gravity (+HG)
 * Gravity is higher when holding this flag.
 * 
 * Requires:
 * - Grue's BZFS
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=highGravityFlag
 */

#include "bzfsAPI.h"
#include "plugin_utils.h"

class HighGravityFlag : public bz_Plugin
{
    virtual const char* Name()
	{
		return "High Gravity Flag";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~HighGravityFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}

    void applyEffect(int playerID);
    void removeEffect(int playerID);
};

void HighGravityFlag::applyEffect(int playerID)
{
    double gravity = bz_getBZDBDouble("_gravity");
    double jumpVel = bz_getBZDBDouble("_jumpVelocity");

    bz_setServerVariableForPlayer(
        playerID, "_gravity",
        std::to_string(gravity * bz_getBZDBDouble("_highGravityMult"))
    );
    bz_setServerVariableForPlayer(
        playerID, "_jumpVelocity",
        std::to_string(jumpVel * bz_getBZDBDouble("_highGravityJumpVelMult"))
    );
}

void HighGravityFlag::removeEffect(int playerID)
{
    bz_setServerVariableForPlayer(
        playerID, "_gravity",
        std::to_string(bz_getBZDBDouble("_gravity"))
    );

    bz_setServerVariableForPlayer(
        playerID, "_jumpVelocity",
        std::to_string(bz_getBZDBDouble("_jumpVelocity"))
    );
}

BZ_PLUGIN(HighGravityFlag)

void HighGravityFlag::Init (const char*)
{
	bz_RegisterCustomFlag("HG", "High Gravity", "Tank's jumps are short, good for dodging shots.", 0, eGoodFlag);
    bz_registerCustomBZDBDouble("_highGravityMult", 2.0);
    bz_registerCustomBZDBDouble("_highGravityJumpVelMult", 1.4);
    Register(bz_eFlagGrabbedEvent);
    Register(bz_eFlagDroppedEvent);
    Register(bz_eFlagTransferredEvent);
}

void HighGravityFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
    {
        case bz_eFlagGrabbedEvent:
        {
            bz_FlagGrabbedEventData_V1* data = (bz_FlagGrabbedEventData_V1*) eventData;
            
            if (strcmp(data->flagType, "HG") == 0)
                applyEffect(data->playerID);
        } break;
        case bz_eFlagDroppedEvent:
        {
            bz_FlagGrabbedEventData_V1* data = (bz_FlagGrabbedEventData_V1*) eventData;

            if (strcmp(data->flagType, "HG") == 0)
                removeEffect(data->playerID);
        } break;
        case bz_eFlagTransferredEvent:
        {
            bz_FlagTransferredEventData_V1* data = (bz_FlagTransferredEventData_V1*) eventData;

            if (strcmp(data->flagType, "HG") == 0)
            {
                removeEffect(data->fromPlayerID);
                applyEffect(data->toPlayerID);
            }
        } break;
        default:
            break;
    }
}