/*
 * Shot Assigner
 * All world weapon shots that have metadata "flag" and "playerID" are
 * reassigned to set "playerID" as the killer. This plugin MUST be loaded first
 * in the BZW file so that it assigns shots before any other plugins take
 * place.
 * 
 * ./configure --enable-custom-plugins=shotAssigner
 */
#include "bzfsAPI.h"

class ShotAssigner : public bz_Plugin
{
	virtual const char* Name()
	{
		return "Shot Assigner";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~ShotAssigner() {};

	virtual void Cleanup(void)
	{
		Flush();
    }
};

BZ_PLUGIN(ShotAssigner)

void ShotAssigner::Init(const char*)
{
	Register(bz_ePlayerDieEvent);
}

void ShotAssigner::Event(bz_EventData *eventData)
{
    if (eventData->eventType == bz_ePlayerDieEvent)
    {
        bz_PlayerDieEventData_V1* data = (bz_PlayerDieEventData_V1*) eventData;
        uint32_t shotGUID = bz_getShotGUID(data->killerID, data->shotID);

        if (bz_shotHasMetaData(shotGUID, "flag") && bz_shotHasMetaData(shotGUID, "playerID"))
        {
            // Assign the shot's data
            data->killerID = bz_getShotMetaDataI(shotGUID, "playerID");
            data->killerTeam = bz_getPlayerTeam(data->killerID);
            data->flagKilledWith = bz_getShotMetaDataS(shotGUID, "flag");
        }
    }
}