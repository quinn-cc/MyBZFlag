/*
 Custom flag: RicoScore (+RS)
 Your shots are worth 2 extra points for each time they ricoshet.
 
 Copyright 2022 Quinn Carmack
 May be redistributed under either the LGPL or MIT licenses.
 
 ./configure --enable-custom-plugins=ricoscoreFlag
*/

#include "bzfsAPI.h"
#include "plugin_utils.h"

class RicoscoreFlag : public bz_Plugin
{
    virtual const char* Name()
	{
		return "Ricoscore Flag";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~RicoscoreFlag();

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(RicoscoreFlag)

void RicoscoreFlag::Init (const char*)
{
	bz_RegisterCustomFlag("RS", "Ricoscore", "Your shots are worth 2 extra points for each time they ricochet.", 0, eGoodFlag);
    Register(bz_ePlayerDieEvent);
    Register(bz_eShotFiredEvent);
    Register(bz_eShotRicochetEvent);
}

RicoscoreFlag::~RicoscoreFlag() {}

void FiftyPointFlag::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		case (bz_ePlayerDieEvent):
		{
			bz_PlayerDieEventData_V2 *data = (bz_PlayerDieEventData_V2*) eventData;
			
			if (data->flagKilledWith == "RS")
			{
				uint32_t shotGUID = bz_getShotGUID(data->playerID, data->shotID);
			
				if (bz_shotHasMetaData(shotGUID, "rico_count")
				{
					int ricoScore = bz_getShotMetaDataI(shotGUID, "rico_count") * 2;
					
					if (data->playerID == data->killerID)
					{
						bz_incrementPlayerLosses(data->killerID, ricoScore);
					}
					else
					{
						bz_incrementPlayerWins(data->killerID, ricoScore);
					}
				}
			}
		} break;
		case (bz_eShotFiredEvent)
		{
			bz_ShotFiredEventData_V1* data = (bz_ShotFiredEventData_V1*) eventData;
			bz_BasePlayerRecord* playerRecord = bz_getPlayerByIndex(data->playerID);
			
			if (playerRecord && playerRecord->currentFlag == "RicoScore (+RS)")
			{
				uint32_t shotGUID = bz_getShotGUID(data->playerID, data->shotID);
				bz_setShotMetaData(shotGUID, "rico_count", 0);
			}
			
			bz_freePlayerRecord(playerRecord);
		} break;
		case (bz_eShotRicochetEvent):
		{
			bz_ShotRicochetEventData_V1* data = (bz_ShotRicochetEventData_V1*) eventData;
			uint32_t shotGUID = bz_getShotGUID(data->playerID, data->shotID);
			
			if (bz_shotHasMetaData(shotGUID, "rico_count")
			{
				bz_setShotMetaDataI(shotGUID, "rico_count", bz_getShotMetaDataI(shotGUID, "rico_count"));
			}
		} break;
		default:
			break;
	}
}
