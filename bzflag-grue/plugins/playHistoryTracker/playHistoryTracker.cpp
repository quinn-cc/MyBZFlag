/*
 * Grue's Play History Tracker
 * - Keeps track of rampages, killing sprees, etc. This version is based off the
 *	 built-in version that comes with BZFlag.
 * - Awards bounty points for stopping rampages by opponents, penalizes
 * 	 teammates that end rampages.
 * - This works with the avengerFlag, dimensionDoorFlag, and annihilationFlag
 * 	 plugin, however those plugins are not necessary to run this one.
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses. 
 * 
 * ./configure --enable-custom-plugins=playHistoryTracker
 */

#include "bzfsAPI.h"
#include <map>
using namespace std;

class PlayHistoryTracker : public bz_Plugin
{
public:
    virtual const char* Name ()
    {
        return "Grue's Play History Tracker";
    }
    virtual void Init (const char*)
    {
        Register(bz_ePlayerDieEvent);
        Register(bz_ePlayerPartEvent);
        Register(bz_ePlayerJoinEvent);
    }

    virtual void Event (bz_EventData *eventData);

private:
	// Map stores a reference between 'playerID' and 'spree', where spree marks
	// how many kills that player has gotten without dying.
    std::map<int, int> spreeCount;
};

BZ_PLUGIN(PlayHistoryTracker)

void PlayHistoryTracker::Event(bz_EventData *eventData)
{
    switch (eventData->eventType)
    {
		case bz_ePlayerDieEvent:
		{
		    bz_PlayerDieEventData_V2 *data = (bz_PlayerDieEventData_V2*) eventData;
		    
			if (!bz_isPlayer(data->playerID) || !bz_isPlayer(data->killerID))
				return;

		    // Create variables to store the callsigns
		    string victim = bz_getPlayerCallsign(data->playerID);
		    string killer = bz_getPlayerCallsign(data->killerID);

			// Store a quick reference to their former spree count
			int spreeTotal = spreeCount[data->playerID];
			int points = 2 * (spreeTotal / 5);
			string pntStr = to_string(points);
		    
		    // Handle the victim
		    if (spreeCount.find(data->playerID) != spreeCount.end())
		    {
		    	bz_ApiString flagHeldWhenKilled = bz_getFlagName(data->flagHeldWhenKilled);
		    	bz_ApiString flagKilledWith = data->flagKilledWith;

				// Being the avenged geno, getting annihilated, and using
				// Dimension Door does not trigger the spree to end.
		    	bool isAvengedGeno = flagHeldWhenKilled == "AV" && bz_isTeamFlag(flagKilledWith.c_str());
		    	bool isAnnihilation = flagHeldWhenKilled == "AN" && data->playerID == data->killerID;
				bool isDimensionDoor = flagHeldWhenKilled == "DD" && data->killerID == BZ_SERVERPLAYER;
		    	
		    	if (!isAvengedGeno && !isAnnihilation && !isDimensionDoor)
		    	{
					// Check if they were on any sort of rampage
					if (spreeTotal >= 5)
					{
						string message;
						string bountyMessage;

						// Generate an appropriate message, if any
						if (spreeTotal >= 5 && spreeTotal < 10)
						{
							if (data->playerID == data->killerID)
							{
								message = victim + " self-pwned, ending that rampage.";
							}
							else if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(data->killerID))
							{
								message = victim + "'s rampage was stopped by teammate " + killer;
								bountyMessage = "Stopping teammate " + victim + "'s rampage lost you 2 extra points";
							}
							else
							{
								message = victim + "'s rampage was stopped by " + killer;
								bountyMessage = "Stopping " + victim + "'s rampage earned you 2 extra bounty points";
							}
						}
						else if (spreeTotal >= 10 && spreeTotal < 15)
						{
							if (data->playerID == data->killerID)
							{
								message = victim + " killed themselves in their own killing spree!";
							}
							else if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(data->killerID))
							{
								message = victim + "'s killing spree was halted by their own teammate " + killer;
								bountyMessage = "Halting teammate " + victim + "'s killing spree lost you 4 extra points";
							}
							else
							{
								message = victim + "'s killing spree was halted by " + killer;
								bountyMessage = "Halting " + victim + "'s killing spree earned you 4 extra bounty points";
							}
						}
						else if (spreeTotal >= 15 && spreeTotal < 20)
						{
							if (data->playerID == data->killerID)
							{
								message = victim + " for some reason blitz'ed themselves...";
							}
							else if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(data->killerID))
							{
								message = victim + "'s blitzkrieg was abrubtly ended by teammate " + killer;
								bountyMessage = "Ending teammate " + victim + "'s blitzkrieg lost you 6 extra points. You're supposed to be family.";
							}
							else
							{
								message = victim + "'s blitzkrieg was finally ended by " + killer;
								bountyMessage = "Ending " + victim + "'s blitzkrieg earned you 6 extra bounty points";
							}
						}
						else if (spreeTotal >= 20)
					   	{
					   		if (data->playerID == data->killerID)
					   		{
								message = "The only force that could stop " + victim + " is " + victim + "... and they did";
							}
							else if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(data->killerID))
							{
								message = "The unstoppable reign of " + victim + " was ended by TEAMMATE " + killer + ". What the hell?!";
								bountyMessage = "Ending teammate " + victim + "'s reign of destruction LOST you " + pntStr + " extra points! C'mon!";
							}
							else
							{
								message = "Finally! The unstoppable reign of " + victim + " was ended by " + killer;
								bountyMessage = "Ending " + victim + "'s reign of destruction earned you " + pntStr + " extra bounty points";
							}
						}

						// Only award bounty if it's not a self-kill and not a teammate & not the server
						if (data->playerID != data->killerID)
						{
							if (bz_getPlayerTeam(data->playerID) == bz_getPlayerTeam(data->killerID))
								bz_incrementPlayerLosses(data->killerID, points);
							else
								bz_incrementPlayerWins(data->killerID, points);
						}
						
						bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, message.c_str());
				    }

				    // Since they died, release their spree counter
				    spreeCount[data->playerID] = 0;
		        }
		    }

		    // Handle the killer (if it wasn't also the victim)
		    if (data->playerID != data->killerID && spreeCount.find(data->killerID) != spreeCount.end())
		    {
		        // Store a quick reference to their newly incremented spree count
		        spreeTotal = ++spreeCount[data->killerID];
		        std::string message;
				points = 2 * (spreeTotal / 5);
				pntStr = to_string(points);

		        // Generate an appropriate message, if any
		        if (spreeTotal == 5)
		            message = killer + " is on a rampage!";
		        else if (spreeTotal == 10)
		            message = killer + " is on a killing spree!";
		        else if (spreeTotal == 15)
		            message = killer + " is going blitz! 6 bounty points on their head!";
		        else if (spreeTotal == 20)
		            message = killer + " is unstoppable!! Come on people; 8 bounty points!!";
		        else if (spreeTotal > 20 && spreeTotal % 5 == 0)
		            message = killer + " continues to obliterate... the bounty is now up to " + pntStr + " points!";

		        // If we have a message to send, then send it
		        if (message.size())
		            bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, message.c_str());
		    }
		} break;
		case bz_ePlayerJoinEvent:
		{
		    // Initialize the spree counter for the player that just joined
		    spreeCount[((bz_PlayerJoinPartEventData_V1*)eventData)->playerID] = 0;
		} break;
		case bz_ePlayerPartEvent:
		{
		    // Erase the spree counter for the player that just left
		    std::map<int, int >::iterator itr = spreeCount.find(((bz_PlayerJoinPartEventData_V1*)eventData)->playerID);
		    if (itr != spreeCount.end())
		        spreeCount.erase(itr);
		} break;
		default:
		    break;
    }
}
