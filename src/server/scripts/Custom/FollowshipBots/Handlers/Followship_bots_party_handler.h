#pragma once

#include "Followship_bots_defines.h"

class Creature;
class Player;

namespace FSBParty
{
    // Converts FSB_Class to WoW Classes enum value for party frame display
    uint8 FSBClassToWoWClass(FSB_Class botClass);

    // Converts FSB_Race to WoW Races enum value for faction group lookup
    uint8 FSBRaceToWoWRace(FSB_Race botRace);

    // Sends a fake SMSG_PARTY_UPDATE to the player that includes their bots
    // If the player is in a real group, bots are appended to the real group members
    // If not in a group, creates a standalone fake party with just the player + bots
    void SendFakePartyUpdate(Player* player);

    // Sends SMSG_PARTY_MEMBER_FULL_STATE for a single bot creature to the owner
    // This updates health, mana, position etc. on the party frame
    void SendBotMemberState(Player* player, Creature* bot);

    // Sends health/mana/position updates for all bots to the owner
    void SendAllBotMemberStates(Player* player);

    // Sends an empty party update to clear the fake group (on dismiss/logout)
    void SendClearFakeParty(Player* player);
}
