/*
 * Custom flag: Low Gravity (+LG)
 * Gravity is less when holding this flag.
 * 
 * Requires:
 * - Grue's BZFS
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * ./configure --enable-custom-plugins=lowGravityFlag
 */

#include "bzfsAPI.h"
#include "plugin_utils.h"

class LowGravityFlag : public bz_Plugin
{
    virtual const char* Name()
	{
		return "Low Gravity Flag";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~LowGravityFlag() {};

	virtual void Cleanup(void)
	{
		Flush();
	}

    void applyEffect(int playerID);
    void removeEffect(int playerID);
};

void LowGravityFlag::applyEffect(int playerID)
{
    double gravity = bz_getBZDBDouble("_gravity");
    double jumpVel = bz_getBZDBDouble("_jumpVelocity");

    bz_setServerVariableForPlayer(
        playerID, "_gravity",
        std::to_string(gravity * bz_getBZDBDouble("_lowGravityMult"))
    );
    bz_setServerVariableForPlayer(
        playerID, "_jumpVelocity",
        std::to_string(jumpVel * bz_getBZDBDouble("_lowGravityJumpVelMult"))
    );
}

void LowGravityFlag::removeEffect(int playerID)
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

BZ_PLUGIN(LowGravityFlag)

void LowGravityFlag::Init (const char*)
{
	bz_RegisterCustomFlag("LG", "Low Gravity", "Gravity is less, tank can jump higher.", 0, eGoodFlag);
    bz_registerCustomBZDBDouble("_lowGravityMult", 0.2);
    bz_registerCustomBZDBDouble("_lowGravityJumpVelMult", 0.2);
    Register(bz_eFlagGrabbedEvent);
    Register(bz_eFlagDroppedEvent);
}

void LowGravityFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
    {
        case bz_eFlagGrabbedEvent:
        {
            bz_FlagGrabbedEventData_V1* data = (bz_FlagGrabbedEventData_V1*) eventData;
            
            if (strcmp(data->flagType, "LG") == 0)
                applyEffect(data->playerID);
        } break;
        case bz_eFlagDroppedEvent:
        {
            bz_FlagGrabbedEventData_V1* data = (bz_FlagGrabbedEventData_V1*) eventData;

            if (strcmp(data->flagType, "LG") == 0)
                removeEffect(data->playerID);
        } break;
        case bz_eFlagTransferredEvent:
        {
            bz_FlagTransferredEventData_V1* data = (bz_FlagTransferredEventData_V1*) eventData;

            if (strcmp(data->flagType, "LG") == 0)
            {
                removeEffect(data->fromPlayerID);
                applyEffect(data->toPlayerID);
            }
        } break;
        default:
            break;
    }
}