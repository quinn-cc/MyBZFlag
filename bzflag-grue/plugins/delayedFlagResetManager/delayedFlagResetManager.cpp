/*
 * Delayed Flag Reset Manager
 */

#include "bzfsAPI.h"
#include "plugin_utils.h"

class DelayedFlagResetManager : public bz_Plugin
{
    virtual const char* Name()
	{
		return "Delayed Flag Reset Manager";
	}
	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	~DelayedFlagResetManager() {};

	virtual void Cleanup(void)
	{
		Flush();
	}
};

BZ_PLUGIN(DelayedFlagResetManager)

void DelayedFlagResetManager::Init (const char*)
{
    bz_registerCustomBZDBDouble("_delayedFlagResetTime", 2.0);
    Register(bz_eTickEvent);
}

void DelayedFlagResetManager::Event(bz_EventData *eventData)
{
	if (eventData->eventType == bz_eTickEvent)
	{		
		if (!delayedFlagResetQueue.empty())
        {
            if (bz_getCurrentTime() - std::get<0>(delayedFlagResetQueue.front()) > bz_getBZDBDouble("_delayedFlagResetTime"))
            {
                bz_resetFlag(std::get<1>(delayedFlagResetQueue.front()));
                delayedFlagResetQueue.pop();
            }
        }
	}
}
