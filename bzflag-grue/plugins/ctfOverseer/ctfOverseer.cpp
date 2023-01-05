/*
 * This plugin oversees a large part of Scarwall's CTF mechanics. Only works
 * for Red and Green teams.
 * 
 * - No team caputres: By setting _noTeamCapture to 1 in the bzw file, teammates
 *   will no longer be able to capture their own team flag.
 * 
 * - Capture reward points: Points are awarded to players capping the opposing
 *   team's flag. The number of points awarded is calculated by a formula that
 *   increases when the team inbalance is greater. The idea is that when it is
 *   harder to cap, more points are awarded. The formula for calculating the
 *   points if team A caps team B 
 * 		= (_baseCapMult)*B + (B - A)(_unbalancedCapMult)
 *   where _unbalancedCapMult is a server variable. However, note that points
 *   are only awarded to players if the cap is fair (see unfair capture penalty)
 * 		
 * - Unfair capture penalty: Points are only awarded to players if the cap was
 *   fair. If the teams are too inbalanced, and the player capping is on the
 *   team with more players, then that player will recieve a point penalty for
 *   capping unfairly. Team A capping team B is fair if
 * 		A / B < _fairCap
 *   where _fairCap is a server variable. If team A capping team B is unfair,
 *   player A will recieve a point penalty calculated by
 * 		(_unfairCapMult)(A - B)
 * 
 * - Switch teams on unfair cap: If the server variable _switchTeamsOnUnfairCap
 *   is turned on, then the capper will be switched to the other team if they
 *   capture unfairly.  
 * 
 * - Own team capture penalty: Players that capture their own team flag are
 *   recieve a point penalty. If team A captures their own team flag, the
 *   penalty amount is calculated by
 * 		(_selfCapMult)*A
 * 
 * - Wait time between caps: Prevents players from capping a team over and over.
 *   If team A caps team B, team A must wait _capWaitTime before they can grab
 *   team B's flag.
 * 
 * - F12ing before the cap: Players who f12 or quit rapidly within
 *   _cachedMemoryTime of the cap will be called out by the server. Furthermore,
 *   cappers will not be penalized if the f12'ers quit to make the teams
 *   unbalanced. If these f12'ers join back into the game within
 *   _quitterMemoryTime seconds, the server will call them out again. The
 *   server recongnizes the players by ipAddress, not callsign. If a player
 *   attempts to join back under a different callsign, the server will
 *   recognize that and call them out further.
 * 
 * Includes:
 * - bzToolkit: A plugin written by allejo called 'bzToolkit' is requried to
 *   run this plugin. bzToolkit can be found on GitHub under allejo's account.
 * 
 * Copyright 2022 Quinn Carmack
 * May be redistributed under either the LGPL or MIT licenses.
 * 
 * Server variables (default):
 * _noTeamCapture (true)
 * _switchTeamsOnUnfairCap (true)
 * _fairCapRatio (1.34)
 * _unbalancedCapMult (8)
 * _baseCapMult (3)
 * _unfairCapMult (5)
 * _capWaitTime (30)
 * _cachedMemoryTime (5.0)
 * _quitterMemoryTime (60)
 * 
 * Requries:
 * - Grue's BZFS
 * 
 * After importing this plugin into the plugins directory, run this command:
 * ./configure --enable-custom-plugins=ctfOverseer
 */

#include "bzfsAPI.h"
#include "plugin_config.h"
#include <map>
#include <queue>
using namespace std;

/*
 * This struct stores a collection of data necessary for caching players.
 * Caching players is explained later, however this struct is used for storing
 * cached player's data.
 */
struct CachedPlayerData
{
	// These first three variables are pretty self-explanatory; they store the
	// player's team, callsign, and ipAddress
	bz_eTeamType team;
	bz_ApiString callsign;
	bz_ApiString ipAddress;
	// This is only used if this CachedPlayerData is being used to cache a
	// quitter's data. In that case, this field stores the time at which the
	// quitter left the game.
	double quitTime = -1;
};

struct CachedGameState
{
	vector<CachedPlayerData> playerList;
	double time;
};



class ctfOverseer : public bz_Plugin
{
	virtual const char* Name()
	{
		return "CTF Overseer";
	}

	virtual void Init(const char*);
	virtual void Event(bz_EventData*);
	virtual void loadConfiguration(const char* config);
	~ctfOverseer();

	virtual void Cleanup(void)
	{
		Flush();
	}

private:
	bz_eTeamType TEAM1;
	bz_eTeamType TEAM2;

	// The last team that was capped	
	bz_eTeamType lastTeamCapped = eNoTeam;
	// The time at which the last team was capped.
	double lastCapTime = 0;
	// This map stores a link between playerID (int) and time (double). When
	// an opposing team tries to grab a defendant's team flag shortly after the
	// defendant's team was capped, the opposing team will be unable to. There
	// is a short wait period (determined by server setting) before the player
	// can grab the flag. When the player does try to grab the flag, there is
	// a message telling them they must wait. In order to prevent spawn
	// messages, this map stores the last time the server warned the player that
	// they must give the team flag time before they can pick it up.
	std::map<int, double> lastFlagWarnMsg;

	int getCapReward(bz_eTeamType, bz_eTeamType);
	bool fairCap(int, int);
	bool fairCap(bz_eTeamType, bz_eTeamType);
	bool cachedFairCap(bz_eTeamType, bz_eTeamType);
	bz_eTeamType oppositeTeam(bz_eTeamType);
	
	/*
	 * Cached Player List
	 * 
	 * This cached player list catches people who quit right before the cap to
	 * disrupt the amount of points the capper should have gotten. The cached
	 * player normally keeps a record of everyone that is currently playing the
	 * game. However, when someone joins or parts the game, the cachedPlayerList
	 * is not updated immediately. There is a latency, set by server variable
	 * _cachedMemoryTime. This determines how much time must go by for the
	 * cachedPlayerList to update to the current player list. With this
	 * functionality, the cachedPlayerList effectively keeps a record of what the
	 * player list was _cachedMemoryTime seconds ago.
	 *
	 * When a player caps, points are NOT rewareded based on the current player
	 * list, but instead rewarded on the cachedPlayerList. This way, players that
	 * quit shortly before the cap do not affect the capper's points.
	 *
	 * Furthermore, the server will call out those on the opposing team who quit
	 * right before the cap by observing the opposing team differences between
	 * the current player list and the recent player list. When these players are
	 * called out, they are added to the Quitters list (see below)
	 */
	//vector<CachedPlayerData> cachedPlayers;
	// This method immediately updates the cached players to be what the player
	// list currently is. This method should *not* be called when a player joins
	// or parts.
	//void updateCachedPlayers();
	// This bool should be set to true when a player joins or parts the game.
	//bool cachedPlayersNeedsUpdate = false;
	// The timestamp of when the cached players was last updated
	//double lastCachedPlayersUpdate = 0;
	// Returns a certain team count on the cached player list.
	//int getCachedTeamCount(bz_eTeamType);

	queue<CachedGameState> cachedGameStates;
	void cacheGameState();
	double lastCacheTime = 0;
	int getCachedTeamCount(bz_eTeamType);
	CachedGameState getReferenceState();
	
	/*
	* Quitters
	* 
	* A player may decide to f12 right before the cap and try to stiff the capper
	* of points. But with the recentPlayerList, we have solved that issue. The
	* capper still gets the same amount of points, and the players that have quit
	* right before the cap are called out. However, let's not stop there ;)
	*
	* The quitters list stores those who quit right before the cap, however we now
	* make use of the RecentPlayerData's timeQuit variable by recording the time
	* they quit. If the player then tries to join back within a certain
	* time period set by server variable _quitterMemoryTime, the server will
	* continue to call them out more. If they join back after _quitterMemoryTime,
	* it is assumed that it was a mistake, and no futher slander is done.
	*
	* RecentPlayerData stores the player's ipAddress as well, so if the player 
	* tries to change their callsign to avoid getting called out, the server will
	* key on their ipAddress.
	*
	* The server will always key on ipAddress first, and then key on callsign if
	* the same callsign is available.
	*/
	vector<CachedPlayerData> quitters;
	void addQuitter(CachedPlayerData);
	void refreshQuitters();			
	bool isQuitter(bz_ApiString, bz_ApiString&);		// ipAddress, oldCallsign
	bool removeQuitter(bz_ApiString, bz_ApiString);		// ipAddress, callsign
};

ctfOverseer::~ctfOverseer() {}
BZ_PLUGIN(ctfOverseer)

void ctfOverseer::Init(const char* config)
{
	loadConfiguration(config);

	bz_registerCustomBZDBBool("_noTeamCapture", true);
	bz_registerCustomBZDBBool("_switchTeamsOnUnfairCap", true);
	bz_registerCustomBZDBDouble("_fairCapRatio", 1.34);
	bz_registerCustomBZDBInt("_unbalancedCapMult", 8);
	bz_registerCustomBZDBInt("_baseCapMult", 3);
	bz_registerCustomBZDBInt("_unfairCapMult", 5);
	bz_registerCustomBZDBInt("_selfCapMult", 5);
	bz_registerCustomBZDBInt("_capWaitTime", 30);
	bz_registerCustomBZDBDouble("_cachedMemoryTime", 10.0);
	bz_registerCustomBZDBDouble("_quitterMemoryTime", 60);
	bz_registerCustomBZDBDouble("_gameStateCacheInterval", 4.0);
	
    Register(bz_eFlagGrabbedEvent);
    Register(bz_eCaptureEvent);
 	Register(bz_eAllowCTFCaptureEvent);
 	Register(bz_eAllowFlagGrab);
 	Register(bz_eTickEvent);
 	Register(bz_ePlayerJoinEvent);
}

/*
 * Returns the opposite team of the parameter. Recall this plugin only works
 * with two teams. 't' is used instead of 'team' because there is a global
 * bzflag variable called 'team'
 */
bz_eTeamType ctfOverseer::oppositeTeam(bz_eTeamType t)
{
	if (t == TEAM1)
		return TEAM2;
	else
		return TEAM1;
}

CachedGameState ctfOverseer::getReferenceState()
{
	return cachedGameStates.front();
}

/*
 * Returns the point reward for 'a' capping 'b'. The value returned is always
 * non-negative. Depending on whether the cap is fair, the return value from
 * this method should either be added to the wins or losses of the player.
 */
int ctfOverseer::getCapReward(bz_eTeamType capping, bz_eTeamType capped)
{
	int reward = 0;

	int a = getCachedTeamCount(capping);
	int b = getCachedTeamCount(capped);

	if (capping == capped)
	{
		reward = bz_getBZDBDouble("_selfCapMult") * a;
	}
	else
	{
		if (cachedFairCap(capping, capped))
			reward = bz_getBZDBInt("_baseCapMult") * b
				+ bz_getBZDBInt("_unbalancedCapMult") * (b - a);
		else
			reward = bz_getBZDBInt("_unfairCapMult") * (a - b);
	}

	return reward;
}

/*
 * Returns true if 'a' capping 'b' is a fair cap, returns false otherwise. Note
 * that if 'b' is zero, the cap is considered unfair.
 */ 
bool ctfOverseer::fairCap(int a, int b)
{
	bool isFair = false;

	// If the ratio a/b is less than or equal to the custom server setting
	// _fairCapRatio, then it is a fair cap.
	if (b != 0 && (float) a/(float) b <= bz_getBZDBDouble("_fairCapRatio") + 0.001)
		isFair = true;
	
	return isFair;
}

bool ctfOverseer::fairCap(bz_eTeamType teamA, bz_eTeamType teamB)
{
	int a = bz_getTeamCount(teamA);
	int b = bz_getTeamCount(teamB);

	return fairCap(a, b);
}

bool ctfOverseer::cachedFairCap(bz_eTeamType teamA, bz_eTeamType teamB)
{
	
	int a = getCachedTeamCount(teamA);
	int b = getCachedTeamCount(teamB);

	return fairCap(a, b);
}

/*
 * Returns how many players were on a team, given by the parameter 'team', in 
 * the cached player list.
 */
int ctfOverseer::getCachedTeamCount(bz_eTeamType t)
{
	int count = 0;
	vector<CachedPlayerData> playerList = getReferenceState().playerList;

	for (int i = 0; i < playerList.size(); i++)
		if (playerList[i].team == t)
			count++;

	return count;
}

void ctfOverseer::cacheGameState()
{
	bz_APIIntList* playerList = bz_getPlayerIndexList();
	CachedGameState gameState;
	
	gameState.time = bz_getCurrentTime();

	for (int i = 0; i < bz_getPlayerCount(); i++)
	{
		bz_BasePlayerRecord* player = bz_getPlayerByIndex(playerList->get(i));

		if (player)
		{
			CachedPlayerData cachedData;
			cachedData.callsign = player->callsign;
			cachedData.ipAddress = player->ipAddress;
			cachedData.team = player->team;
			gameState.playerList.push_back(cachedData);
		}

		bz_freePlayerRecord(player);
	}

	cachedGameStates.push(gameState);
	bz_deleteIntList(playerList);
}

/*void ctfOverseer::updateCachedPlayers()
{
	bz_APIIntList* playerList = bz_getPlayerIndexList();

	if (!playerList)
		return;

	// This loops through all the players and finds any player that is holding
	// the opposing team's flag. If teams have gone from fair to unfair, or vice
	// versa, let that player know.
	for (int i = 0; i < bz_getPlayerCount(); i++)
	{
		bz_BasePlayerRecord *player = bz_getPlayerByIndex(playerList->get(i));
		
		if (!player)
			continue;
		
		// If the player is holding the opposing team's flag
		if ((bz_getPlayerFlagAbbr(player->playerID) == "G*" && player->team == eRedTeam) ||
			(bz_getPlayerFlagAbbr(player->playerID) == "R*" && player->team == eGreenTeam))
		{
			// Gets what the offensive's player team count is and their
			// opposing team count.
			int thisCurrentTeamCount = bz_getTeamCount(player->team);
			int otherCurrentTeamCount = bz_getTeamCount(oppositeTeam(player->team));

			// Gets what the offensive's player team count was (in cache) and
			// their opposing team count.
			int thisCachedTeamCount	= getCachedTeamCount(player->team);
			int otherCachedTeamCount = getCachedTeamCount(oppositeTeam(player->team));

			// Would it be a fair cap using the cached player list?
			bool cachedFair = fairCap(thisCachedTeamCount, otherCachedTeamCount);
			// Is it a fair cap using the current player list?
			bool currentlyFair = fairCap(thisCurrentTeamCount, otherCurrentTeamCount);
			
			// If the teams went from unfair to fair, tell the player who is
			// currently holding the opposing team's flag that they are now
			// permitted to cap fairly.
			if (!cachedFair && currentlyFair)
			{
				bz_sendTextMessagef(
					BZ_SERVER,
					playerList->get(i),
					"It's %d v %d now. Fair game!",
					thisCurrentTeamCount,
					otherCurrentTeamCount
				);
			}
			// If the teams went from fair to unfair, then tell the player who
			// is currently holding the opposing team's flag that they can no
			// longer cap fairly.
			else if (cachedFair && !currentlyFair)
			{
				bz_sendTextMessagef(
					BZ_SERVER,
					playerList->get(i),
					"It's %d v %d now... let's play fair here.",
					thisCurrentTeamCount,
					otherCurrentTeamCount
				);
			}
		}
		
		bz_freePlayerRecord(player);
	}
	// Clear out the cached player list. The for loop directly below will update
	// the cached players to be what they are currently.
	cachedPlayers.clear();
	// Loop over all the players in the game that are on one of the two teams
	// (aka not observer) and add them to the cached player list.
	for (int i = 0; i < bz_getPlayerCount(); i++)
	{
		bz_BasePlayerRecord *player = bz_getPlayerByIndex(playerList->get(i));

		if (!player)
			continue;

		if (player->team == eRedTeam || player->team == eGreenTeam)
		{
			CachedPlayerData data;
			data.callsign = player->callsign;
			data.team = player->team;
			data.ipAddress = player->ipAddress;
			cachedPlayers.push_back(data);
		}

		bz_freePlayerRecord(player);
	}
	// Since the cached player list was just updated to be the current player
	// list, the last update time is now.
	lastCachedPlayersUpdate = bz_getCurrentTime();
	// The cached player list has just been updated, so it is no longer in need
	// of an update. This will in the future be set to true again once a player
	// quits or joins the game.
	cachedPlayersNeedsUpdate = false;
}*/

/*
 * Adds the given CachedPlayerData to the quitters list
 */
void ctfOverseer::addQuitter(CachedPlayerData data)
{
	data.quitTime = bz_getCurrentTime();
	quitters.push_back(data);
}

/*
 * Removes the quitter given by ipAddress and callsign.
 * Returns:
 * - bool: true if successfully removed, false if not
 */
bool ctfOverseer::removeQuitter(bz_ApiString ipAddress, bz_ApiString callsign)
{
	for (int i = 0; i < quitters.size(); i++)
	{
		if (quitters[i].ipAddress == ipAddress
			&& quitters[i].callsign == callsign)
		{
			quitters.erase(quitters.begin() + i);
			return true;
		}
	}
	return false;
}

/*
 * This clears out any quitters that still been gone for more than the memory
 * time. This method should be called every so often, maybe 15 minutes, to
 * prevent data from accummulating.
 * NOTE: This method is not yet used.
 */
void ctfOverseer::refreshQuitters()
{
	for (int i = 0; i < quitters.size(); i++)
	{
		if (bz_getCurrentTime() - quitters[i].quitTime >
			bz_getBZDBDouble("_quitterMemoryTime"))
		{
			quitters.erase(quitters.begin() + i);
		}
	}
}

/*
 * Returns true if the player with the given callsign is a quitter. The
 * reference parameter 'cachedCallsign' is assigned to be the callsign the
 * player was cached with. The cachedCallsign reference parameter should be
 * passed as what their current callsign is. If their cached callsign was
 * something different, then it will be changed.
 */
bool ctfOverseer::isQuitter(bz_ApiString ipAddress, bz_ApiString &cachedCallsign)
{
	bool quitter = false;

	for (int i = 0; i < quitters.size(); i++)
	{
		if (quitters[i].ipAddress == ipAddress)
		{
			// If they are still within valid time, they are a quitter.
			if (bz_getCurrentTime() - quitters[i].quitTime < bz_getBZDBDouble("_quitterMemoryTime"))
			{
				cachedCallsign = quitters[i].callsign;
				quitter = true;
			}
			// Too late to call them out, remove them from the quitters list
			else
			{
				quitters.erase(quitters.begin() + i);
			}
			return quitter;
		}
	}
	
	return quitter;
}

void ctfOverseer::loadConfiguration(const char* configPath)
{
	/*PluginConfig config = PluginConfig(configPath);
    string section = "two_team";

    if (config.errors)
    {
        bz_debugMessage(0, "Your configuration file has errors");
        return;
    }

    TEAM1 = bz_stringToTeamType(config.item(section, "TEAM1"));
    TEAM2 = bz_stringToTeamType(config.item(section, "TEAM2"));*/

	TEAM1 = eRedTeam;
	TEAM2 = eBlueTeam;
}

void ctfOverseer::Event(bz_EventData *eventData)
{
	switch (eventData->eventType)
	{
		/*
		 * Listens for a capture is about to be made. Functionality within this
		 * event includes:
		 * - Disabling own team capture if the setting is turned on.
		 */
		case bz_eAllowCTFCaptureEvent:
		{
			if (bz_getBZDBBool("_noTeamCapture"))
			{
				bz_AllowCTFCaptureEventData_V1 *data
					= (bz_AllowCTFCaptureEventData_V1*) eventData;
				data->allow = (bz_getPlayerTeam(data->playerCapping)
					!= data->teamCapped);
			}
		} break;
		/*
		 * Listens for when a flag is grabbed. Functionality within this event
		 * includes:
		 * - If a player grabs the other team's flag when teams are unfair, a
		 *   message is sent to them.
		 */
		case bz_eFlagGrabbedEvent:
		{
			bz_FlagGrabbedEventData_V1 *data = (bz_FlagGrabbedEventData_V1*) eventData;
			
			// First get the player's flag.
			bz_ApiString flag = bz_getPlayerFlagAbbr(data->playerID);
			
			bz_BasePlayerRecord *player = bz_getPlayerByIndex(data->playerID);

			if (!player)
				return;

			// If a player grabs the opposing team's flag
			if (bz_isTeamFlag(flag.c_str()) && bz_getTeamFromFlag(flag.c_str()) != player->team)
			{
				int thisTeamCount = bz_getTeamCount(player->team);
				int otherTeamCount = bz_getTeamCount(oppositeTeam(player->team));

				// If it's not fair, let them know they're being a dick.
				if (!fairCap(thisTeamCount, otherTeamCount))
				{
					bz_sendTextMessagef(
						BZ_SERVER,
						data->playerID, 
						"%d v %d? Don't be that person.",
						thisTeamCount,
						otherTeamCount
					);
				}
			}
			
			bz_freePlayerRecord(player);
		} break;
		/*
		 * This event listens for when team flag is captured. Functionality
		 * includes:
		 * - Awarding points for capping fairly
		 * - Penalty points for capping unfarily
		 * - Switching player team for capping unfairly
		 * - Calling out and recording those who quit shortly before the cap
		 */
		case bz_eCaptureEvent:
	 	{
	 		bz_CTFCaptureEventData_V1* data =
				(bz_CTFCaptureEventData_V1*) eventData;

			// Own team capture
			if (data->teamCapping == data->teamCapped)
			{
				int reward = getCapReward(data->teamCapping, data->teamCapped);

				bz_sendTextMessagef(
					BZ_SERVER,
					BZ_ALLUSERS,
					"%s captured their own team flag. How ignorant. -%d point"
						" penalty.",
					bz_getPlayerCallsign(data->playerCapping),
					to_string(reward).c_str()
				);

				bz_incrementPlayerLosses(data->playerCapping, reward);
				break;
			}

	 		// Set the last team capped to be this team.
	 		lastTeamCapped = data->teamCapped;
	 		// Set the last cap time to be now.
			lastCapTime = bz_getCurrentTime();
	 		
	 		bool cachedFair = cachedFairCap(data->teamCapping, data->teamCapped);
	 		bool currentlyFair = fairCap(data->teamCapping, data->teamCapped);
	 		
	 		int reward = getCapReward(data->teamCapping, data->teamCapped);
	 		
	 		// Fair -> unfair
	 		if (cachedFair && !currentlyFair)
	 		{
	 			// This list temporarily stores the players that quit during 
	 			// this cap.
	 			vector<bz_ApiString> newQuitters;
	 		
	 			// Add any quitters into the quitters list, and add them into
				// the current quitters.
	 			for (int i = 0; i < getReferenceState().playerList.size(); i++)
	 			{
	 				// Quitters can only come from the team that was capped
	 				if (getReferenceState().playerList[i].team == data->teamCapped)
	 				{
	 					bz_BasePlayerRecord* playerRecord = bz_getPlayerBySlotOrCallsign(getReferenceState().playerList[i].callsign.c_str());
	 					
		 				if (!playerRecord)
		 				{
		 					addQuitter(getReferenceState().playerList[i]);
		 					newQuitters.push_back(getReferenceState().playerList[i].callsign);
		 				}
						else if (playerRecord->team == eObservers)
						{
							newQuitters.push_back(getReferenceState().playerList[i].callsign);
						}
		 					
		 				bz_freePlayerRecord(playerRecord);
	 				}
	 			}
	 			
	 			// Note that there may not actually be any quitters. Players
				// could have quickly joined the capper's team causing an
				// inbalance.
	 			if (newQuitters.size() > 0)
	 			{
					// Stores a string of the quitters
		 			bz_ApiString quitterList = "";
		 			
		 			for (int i = 0; i < newQuitters.size(); i++)
		 			{
		 				quitterList += newQuitters[i];

						if (newQuitters.size() == 2 && i == 0)
							quitterList += " and ";
						else if (i < newQuitters.size() - 1) {
							if (newQuitters.size() > 2)
								quitterList += ", ";
							if (i == newQuitters.size() - 1)
								quitterList += "and ";
						}
		 			}
		 			
					// There were quitters during this cap, so send a message
					// announcing the quitters.
		 			bz_sendTextMessagef(
						BZ_SERVER,
						BZ_ALLUSERS,
						"It was just %d v %d a second ago. %s quit before the cap. Those f12'ers...",
						getCachedTeamCount(data->teamCapping),
						getCachedTeamCount(data->teamCapped),
						quitterList.c_str()
					);
	 			}
	 			else
	 			{
					// No quitters during this cap, just announce what the teams
					// were recently.
	 				bz_sendTextMessagef(
						BZ_SERVER,
						BZ_ALLUSERS,
						"It was just %d v %d a second ago.",
						getCachedTeamCount(data->teamCapping),
						getCachedTeamCount(data->teamCapped)
					);
	 			}
	 	
				// Whether there were quitters or not, if the teams became
				// unbalanced recently, then announce the capper is still
				// getting fair points for the cap.
	 			bz_sendTextMessagef(
					BZ_SERVER,
					BZ_ALLUSERS,
					"%s is still getting %s points for the cap.",
					bz_getPlayerCallsign(data->playerCapping),
					to_string(reward).c_str()
				);

				bz_incrementPlayerWins(data->playerCapping, reward);
	 		}
	 		else
	 		{
		 		// Opponents had no players
		 		if (bz_getTeamCount(data->teamCapped) == 0)
		 		{
		 			bz_sendTextMessagef(
						BZ_SERVER,
						BZ_ALLUSERS,
						"Thats weird... %s just captured the flag, but the %s team didn't have any players.",
						bz_getPlayerCallsign(data->playerCapping),
						bz_eTeamTypeLiteral(data->teamCapped)
					);
				}
		 		// Fair capping
		 		else if (fairCap(bz_getTeamCount(data->teamCapping),
					bz_getTeamCount(data->teamCapped)))
		 		{
		 			bz_sendTextMessagef(
						BZ_SERVER,
						BZ_ALLUSERS,
						"Score! %s just captured the flag, with a %s point CTF bonus.",
						bz_getPlayerCallsign(data->playerCapping),
						to_string(reward).c_str()
					);
		 			bz_incrementPlayerWins(data->playerCapping, reward);
				}
				// Unfair capping
				else
				{
					bz_sendTextMessagef(
						BZ_SERVER,
						BZ_ALLUSERS,
						"Come on! %s capped unfairly, taking a %s point CTF "
							"penalty.",
						bz_getPlayerCallsign(data->playerCapping),
						to_string(reward).c_str()
					);
					
					// Switch the player to the other team if that setting is
					// turned on.
					if (bz_getBZDBBool("_switchTeamsOnUnfairCap"))
					{
						bz_killPlayer(data->playerCapping, false);
						bz_incrementPlayerLosses(data->playerCapping, -1);

						// Tell everyone that the capper has been switched to
						// the other team
						bz_sendTextMessagef(
							BZ_SERVER,
							BZ_ALLUSERS,
							"%s has been switched to the %s team to help them out.",
							bz_getPlayerCallsign(data->playerCapping),
							bz_eTeamTypeLiteral(data->teamCapped)
						);
						// Tell the capper that they have been switched to the
						// other team (personal message)
						bz_sendTextMessagef(
							BZ_SERVER,
							data->playerCapping,
							"You have been switched to the %s team.",
							bz_eTeamTypeLiteral(data->teamCapped)
						);

						bz_changeTeam(data->playerCapping, data->teamCapped);
					}
					
					bz_incrementPlayerLosses(data->playerCapping, reward);
				}
			}
	 	} break;
		/*
		 * This event listens for when flag is about to be grabbed.
		 * Functionality includes:
		 * - Not allowing a player to grab the other team's flag if they capped
		 *   recently
		 */
	 	case bz_eAllowFlagGrab:
	 	{
	 		bz_AllowFlagGrabData_V1 *data = (bz_AllowFlagGrabData_V1*) eventData;
	 		
	 		if (bz_getCurrentTime() - lastCapTime < bz_getBZDBDouble("_capWaitTime"))
	 		{
	 			bz_ApiString flag = bz_getFlagName(data->flagID);
	 			
	 			// Clear the cap time wait if:
	 			// (a) The last team capped grabs their own team flag
	 			// (b) The last team capped grabs their opponents' team flag
	 			if ((flag == bz_getFlagFromTeam(lastTeamCapped) &&
						bz_getPlayerTeam(data->playerID) == lastTeamCapped) ||
	 				(flag == bz_getFlagFromTeam(oppositeTeam(lastTeamCapped)) &&
						bz_getPlayerTeam(data->playerID) == lastTeamCapped))
	 			{
	 				lastCapTime = 0;
	 			}
	 			else if (flag == bz_getFlagFromTeam(lastTeamCapped) &&
					bz_getPlayerTeam(data->playerID) == oppositeTeam(lastTeamCapped))
	 			{
	 				data->allow = false;
		 				
	 				// Wait 5 seconds between spamming with wait messages.
	 				if (bz_getCurrentTime() > lastFlagWarnMsg[data->playerID] + 5)
	 				{
		 				bz_sendTextMessagef(
							BZ_SERVER,
							data->playerID,
							"You must wait another %d seconds to grab the flag. Don't spawn-cap.",
							(int) (lastCapTime + bz_getBZDBDouble("_capWaitTime") - bz_getCurrentTime())
						);
		 				lastFlagWarnMsg[data->playerID] = bz_getCurrentTime();
	 				}
	 			}
	 		}
	 	} break;
        case bz_eTickEvent:
        {
			// Clears all stores flag warn times when the flag is eligible to
			// be grabbed.
			if (bz_getCurrentTime() - lastCapTime >= bz_getBZDBDouble("_capWaitTime") &&
				!lastFlagWarnMsg.empty())
			{
            	lastFlagWarnMsg.clear();
			}
            
			if (bz_getCurrentTime() - lastCacheTime > bz_getBZDBDouble("_gameStateCacheInterval"))
			{
				cacheGameState();
				lastCacheTime = bz_getCurrentTime();
			}

			if (bz_getCurrentTime() - getReferenceState().time > bz_getBZDBDouble("_cachedMemoryTime"))
			{
				cachedGameStates.pop();
			}
        } break;
        case bz_ePlayerJoinEvent:
		{
			bz_PlayerJoinPartEventData_V1* data = (bz_PlayerJoinPartEventData_V1*) eventData;
		    
			// This local variable is passed by reference into the isQuitter
			// method. The isQuitter method detects if the joining ipAddress
			// is a quitter. If they are, then isQuitter will set this
			// reference variable to be the callsign they used previously
			// at the time of their f12 quit.
		    bz_ApiString previousCallsign;
		    
		    if (isQuitter(data->record->ipAddress, previousCallsign))
		    {
		    	// The quitter came back with the same callsign
		    	if (data->record->callsign == previousCallsign)
		    	{
		    		bz_sendTextMessagef(
						BZ_SERVER,
						BZ_ALLUSERS,
						"%s just f12'ed before a cap, and now came right back! What a punk!",
						previousCallsign.c_str()
					);
		    	}
		    	// The quitter came back under a different callsign
		    	else
		    	{
		    		bz_sendTextMessagef(
						BZ_SERVER,
						BZ_ALLUSERS,
						"%s just f12'ed before a cap, and is now trying to hide under the alias '%s'. WHAMMO! is onto you!",
						previousCallsign.c_str(), data->record->callsign.c_str()
					);

					bz_sendTextMessage(
						BZ_SERVER,
						data->playerID,
						"Jokes on you, they didn't lose points from your f12'ing."
					);
		    	}
		    	
		    	removeQuitter(data->record->ipAddress, data->record->callsign);
		    }
		    
		} break;
	 	default: break;
 	}
}














