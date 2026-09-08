/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PacketOperators.h"
#include "ScriptHelpers.h"

#include "Log.h"
#include "Battleground.h"
#include "Group.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"

#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_battleground_handler.h"
#include "Followship_bots_utils.h"

#include <algorithm>

#include "Followship_bots_events_handler.h"
#include "Followship_bots_party_handler.h"
#include "Followship_bots_pet_handler.h"

#include "Timer.h"

#include <unordered_map>

namespace FSBParty
{
    struct PartyThrottleState
    {
        uint32 compositionHash = 0;
        bool hadBots = false;
    };

    struct BgRaidThrottleState
    {
        uint32 nextSliceMs = 0;
        uint32 nextBotIndex = 0;
        std::unordered_map<ObjectGuid::LowType, uint32> rosterHashByPlayer;
    };

    static constexpr uint32 BG_MEMBER_SLICES = 10;

    // Per-owner throttle for party frame packets. Map updates for a
    // player+bots are same-map / same-thread in normal TC usage.
    static std::unordered_map<ObjectGuid::LowType, PartyThrottleState> s_partyThrottle;

    // One raid-frame broadcast per BG instance + team, not per bot.
    static std::unordered_map<uint64, BgRaidThrottleState> s_bgRaidThrottle;

    static uint32 HashActiveBots(std::vector<Creature*> const& bots)
    {
        uint32 hash = static_cast<uint32>(bots.size());
        for (Creature const* b : bots)
        {
            if (!b)
                continue;
            uint32 c = static_cast<uint32>(b->GetGUID().GetCounter());
            hash ^= c + 0x9e3779b9u + (hash << 6) + (hash >> 2);
        }
        return hash;
    }

    // Only one bot per owner should drive party packets, otherwise N bots
    // each fire PeriodicPartyNeededCheck every 1s → N full PartyUpdates/s.
    static bool IsPartyUpdateDriver(Creature* bot, std::vector<Creature*> const& activeBots)
    {
        if (!bot || activeBots.empty())
            return false;

        ObjectGuid minGuid = activeBots.front()->GetGUID();
        for (Creature const* b : activeBots)
        {
            if (b && b->GetGUID() < minGuid)
                minGuid = b->GetGUID();
        }
        return bot->GetGUID() == minGuid;
    }

    static uint64 MakeBgRaidThrottleKey(BattlegroundMap const* bgMap, Team team)
    {
        return (uint64(bgMap->GetInstanceId()) << 32) ^ (uint64(bgMap->GetId()) << 16) ^ uint64(team);
    }

    static uint8 GetLfgRoleForBot(Creature* bot)
    {
        FSB_Roles role = FSBMgr::Get()->GetRole(bot);
        // LFG role flags: TANK=2, HEALER=4, DAMAGE=8
        switch (role)
        {
        case FSB_ROLE_TANK:          return 2;
        case FSB_ROLE_HEALER:        return 4;
        default:                     return 8; // damage
        }
    }

	std::vector<Creature*> CollectActiveBots(Player* player)
	{
		std::vector<Creature*> result;

		if (!player)
			return result;

		auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(player);
		if (!botsPtr)
			return result;

		for (auto const& botData : *botsPtr)
		{
			if (botData.runtimeGuid.IsEmpty())
				continue;

			if (Creature* bot = ObjectAccessor::GetCreatureOrPetOrVehicle(*player, botData.runtimeGuid))
			{
				if (bot->IsInWorld())
					result.push_back(bot);
			}
		}

		return result;
	}

    void SendFakePartyUpdate(Player* player, Creature* bot)
    {
        if (!player || !player->GetSession())
            return;

        if (!player->IsInWorld() || player->IsBeingTeleportedNear() || player->IsBeingTeleportedFar() || player->IsBeingTeleported())
            return;

        if (!bot)
            return;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->GetAI());
        if (!baseAI)
            return;

        auto& activeBots = baseAI->partyBots;

        Group* realGroup = player->GetGroup();

        if (activeBots.empty())
        {
            if (!realGroup || realGroup->GetMembersCount() == 1)
            {
                // Player is solo ? send a clean "clear fake party"
                SendClearFakeParty(player);
            }
            else
            {
                // Player is in a real group ? send a normal group-only PartyUpdate
                ScriptHelpers::SendFakePartyUpdate(player, std::vector<Creature*>(), std::vector<uint8>(), std::vector<uint8>(), std::vector<uint8>());

                TC_LOG_DEBUG("scripts.fsb.party",
                    "FSB: SendFakePartyUpdate Sent real-only PartyUpdate to {} (no active bots)",
                    player->GetName());
            }

            return;
        }

        // ------------------------------------------------------------
        // LIMIT PARTY SIZE TO MAX 5 (PLAYER + 4 OTHERS)
        // ------------------------------------------------------------
        // Count real group members (if any)
        size_t realCount = 0;
        if (realGroup)
            realCount = realGroup->GetMemberSlots().size();
        else
            realCount = 1; // solo player counts as 1

        // How many bots can we show?
        size_t maxBotsAllowed = (realCount >= MAX_CLIENT_PARTY)
            ? 0
            : (MAX_CLIENT_PARTY - realCount);

        // Trim activeBots if needed
        std::vector<Creature*> trimmedBots = activeBots;
        if (trimmedBots.size() > maxBotsAllowed)
            trimmedBots.resize(maxBotsAllowed);

        // filter bots and collect class/race/role info
        std::vector<Creature*> safeBots;
        std::vector<uint8> botClasses;
        std::vector<uint8> botRaces;
        std::vector<uint8> botRoles;
        for (Creature* bot : trimmedBots)
        {
            if (bot && bot->IsInWorld())
            {
                safeBots.push_back(bot);
                FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
                FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());
                botClasses.push_back(FSBUtils::FSBToTCClass(botClass));
                botRaces.push_back(FSBUtils::BotRaceToTC(botRace));
                botRoles.push_back(GetLfgRoleForBot(bot));
            }
        }

        baseAI->partyBots = safeBots;

        // ------------------------------------------------------------

        ScriptHelpers::SendFakePartyUpdate(player, safeBots, botClasses, botRaces, botRoles);

        TC_LOG_DEBUG("scripts.fsb.party",
            "FSB: SendFakePartyUpdate sent to player {} with {} bots",
            player->GetName(), safeBots.size());
    }

    void SendBattlegroundRaidUpdate(Player* player, std::vector<Creature*> const& bots)
    {
        if (!player || !player->GetSession())
            return;

        if (!player->IsInWorld() || player->IsBeingTeleportedNear() || player->IsBeingTeleportedFar() || player->IsBeingTeleported())
            return;

        if (player->GetSession()->PlayerLoading())
            return;

        if (bots.empty())
            return;

        std::vector<Creature*> safeBots;
        std::vector<uint8> botClasses;
        std::vector<uint8> botRaces;
        std::vector<uint8> botRoles;
        for (Creature* bot : bots)
        {
            if (bot && bot->IsInWorld())
            {
                safeBots.push_back(bot);
                FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
                FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());
                botClasses.push_back(FSBUtils::FSBToTCClass(botClass));
                botRaces.push_back(FSBUtils::BotRaceToTC(botRace));
                botRoles.push_back(GetLfgRoleForBot(bot));
            }
        }

        if (safeBots.empty())
            return;

        // Sort bots so hired bots (those with a non-null owner) come before non-hired bots.
        // This preserves the priority: real players, then hired bots, then remaining spawn bots.
        std::stable_sort(safeBots.begin(), safeBots.end(), [](Creature* a, Creature* b)
        {
            bool aHired = FSBMgr::Get()->GetBotOwner(a) != nullptr;
            bool bHired = FSBMgr::Get()->GetBotOwner(b) != nullptr;
            return aHired && !bHired;
        });

        // Rebuild class/race/role vectors to match the sorted order.
        for (size_t i = 0; i < safeBots.size(); ++i)
        {
            FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(safeBots[i]->GetEntry());
            FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(safeBots[i]->GetEntry());
            botClasses[i] = FSBUtils::FSBToTCClass(botClass);
            botRaces[i] = FSBUtils::BotRaceToTC(botRace);
            botRoles[i] = GetLfgRoleForBot(safeBots[i]);
        }

        ScriptHelpers::SendFakeBGRaidUpdate(player, safeBots, botClasses, botRaces, botRoles);

        TC_LOG_DEBUG("scripts.fsb.party",
            "FSB: SendBattlegroundRaidUpdate sent to player {} with {} bots",
            player->GetName(), safeBots.size());
    }

    void SendBotMemberState(Player* player, Creature* bot, bool includeAuras)
    {
        if (!player || !player->GetSession() || !player->IsInWorld() || player->IsBeingTeleportedNear() || player->IsBeingTeleported() || player->IsBeingTeleportedFar())
            return;

        if (player->GetSession()->PlayerLoading())
            return;

        if (!bot || !bot->IsInWorld())
            return;

        if (player->GetMapId() != bot->GetMapId())
            return;

        // Collect bot data
        uint32 level = bot->GetLevel();
        uint32 currentHealth = bot->GetHealth();
        uint32 maxHealth = bot->GetMaxHealth();
        uint8 powerType = bot->GetPowerType();
        uint32 currentPower = bot->GetPower(bot->GetPowerType());
        uint32 maxPower = bot->GetMaxPower(bot->GetPowerType());
        uint32 zoneID = bot->GetZoneId();
        float positionX = bot->GetPositionX();
        float positionY = bot->GetPositionY();
        float positionZ = bot->GetPositionZ();

        std::vector<uint32> auraSpellIds;
        if (includeAuras)
            auraSpellIds = GetBotAppliedAuras(bot);

        TC_LOG_DEBUG("scripts.fsb.party", "FSB: SendBotMemberState Bot {} sending {} auras", bot->GetEntry(), auraSpellIds.size());

        ScriptHelpers::SendBotMemberState(player, bot, level, currentHealth, maxHealth, powerType, currentPower, maxPower, 
                                         zoneID, positionX, positionY, positionZ, auraSpellIds);
    }

    void SendClearFakeParty(Player* player)
    {
        ScriptHelpers::SendClearFakeParty(player);

        TC_LOG_DEBUG("scripts.fsb.party", "FSB: SendClearFakeParty sent to player {}", player->GetName());
    }

    void PeriodicPartyNeededCheck(Creature* bot)
    {
        if (!bot || !bot->IsInWorld())
            return;

        Player* owner = FSBMgr::Get()->GetBotOwner(bot);
        if (!owner || !owner->GetSession() || owner->IsBeingTeleportedNear() || owner->IsBeingTeleported() || owner->IsBeingTeleportedFar() || !owner->IsInWorld())
            return;

        // In battlegrounds the raid-frame update handles all bots centrally; skip the capped party update here.
        if (owner->GetMap()->IsBattleground())
            return;

        std::vector<Creature*> activeBots = CollectActiveBots(owner);

        // Cheap: keep partyBots cache warm on the calling bot even if it is
        // not the packet driver.
        if (auto* baseAI = dynamic_cast<FSB_BaseAI*>(bot->GetAI()))
            baseAI->partyBots = activeBots;

        if (activeBots.empty())
        {
            ObjectGuid::LowType ownerKey = owner->GetGUID().GetCounter();
            auto it = s_partyThrottle.find(ownerKey);
            if (it == s_partyThrottle.end() || it->second.hadBots)
            {
                SendClearFakeParty(owner);
                s_partyThrottle[ownerKey] = PartyThrottleState{};
            }
            return;
        }

        // One driver bot per owner — avoids N× PartyUpdate spam / second.
        if (!IsPartyUpdateDriver(bot, activeBots))
            return;

        for (Creature* b : activeBots)
        {
            if (auto* ai = dynamic_cast<FSB_BaseAI*>(b->GetAI()))
                ai->partyBots = activeBots;
        }

        ObjectGuid::LowType ownerKey = owner->GetGUID().GetCounter();
        PartyThrottleState& throttle = s_partyThrottle[ownerKey];
        uint32 hash = HashActiveBots(activeBots);

        // Full PartyUpdate only when the bot roster changes. Rebuilding the
        // client party frame every second is what hitching the main thread /
        // client FPS on the 1s cadence.
        if (!throttle.hadBots || throttle.compositionHash != hash)
        {
            SendFakePartyUpdate(owner, bot);
            throttle.compositionHash = hash;
            throttle.hadBots = true;
        }

        // HP/mana/position/auras — once per owner per maintenance tick
        // (driver only), covering every active bot.
        for (Creature* b : activeBots)
            SendBotMemberState(owner, b);
    }

    void PeriodicBattlegroundRaidUpdate(Creature* bot)
    {
        if (!bot || !bot->IsInWorld())
            return;

        BattlegroundMap* bgMap = bot->GetMap()->ToBattlegroundMap();
        if (!bgMap || !bgMap->GetBG())
            return;

        Battleground* bg = bgMap->GetBG();

        Team botTeam = ScriptHelpers::GetBotTeam(bot);
        if (botTeam != ALLIANCE && botTeam != HORDE)
            return;

        uint64 const key = MakeBgRaidThrottleKey(bgMap, botTeam);
        uint32 const nowMs = getMSTime();
        BgRaidThrottleState& throttle = s_bgRaidThrottle[key];
        if (nowMs < throttle.nextSliceMs)
            return;

        // One shared 1s slice per team. Spreads HP updates and avoids a 10s
        // burst of SMSG_PARTY_UPDATE (full raid-frame rebuild on the client).
        throttle.nextSliceMs = nowMs + 1000;

        std::vector<Creature*> bots = FSBBattleground::CollectBotsOnTeam(bgMap, botTeam);
        if (bots.empty())
        {
            s_bgRaidThrottle.erase(key);
            return;
        }

        uint32 const hash = HashActiveBots(bots);

        uint32 const sliceCount = std::max<uint32>(1, (uint32(bots.size()) + BG_MEMBER_SLICES - 1) / BG_MEMBER_SLICES);
        if (throttle.nextBotIndex >= bots.size())
            throttle.nextBotIndex = 0;
        uint32 const sliceStart = throttle.nextBotIndex;

        for (auto const& [guid, _] : bg->GetPlayers())
        {
            Player* player = ObjectAccessor::GetPlayer(bgMap, guid);
            if (!player || !player->GetSession() || !player->IsInWorld())
                continue;

            if (player->IsBeingTeleportedNear() || player->IsBeingTeleportedFar() || player->IsBeingTeleported())
                continue;

            if (player->GetSession()->PlayerLoading())
                continue;

            if (player->GetTeam() != botTeam)
                continue;

            ObjectGuid::LowType const playerKey = player->GetGUID().GetCounter();
            uint32& sentHash = throttle.rosterHashByPlayer[playerKey];
            if (sentHash != hash)
            {
                SendBattlegroundRaidUpdate(player, bots);
                sentHash = hash;
            }

            for (uint32 i = 0; i < sliceCount; ++i)
            {
                Creature* teamBot = bots[(sliceStart + i) % bots.size()];
                bool const includeAuras = FSBMgr::Get()->GetBotOwner(teamBot) == player;
                SendBotMemberState(player, teamBot, includeAuras);
            }
        }

        throttle.nextBotIndex = (sliceStart + sliceCount) % uint32(bots.size());
    }

    void OnMemberAdd(Group* group, ObjectGuid guid)
    {
        if (!group || !guid)
            return;

        // In battlegrounds the raid-frame update handles bots centrally.
        if (group->isBGGroup())
            return;

        // When a player joins a group, re-send the party update with bots included
        if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        {
            if (!player ||
                !player->IsInWorld() ||
                player->IsBeingTeleportedNear() ||
                player->IsBeingTeleportedFar() ||
                player->IsBeingTeleported())
                return;

            // Get the first active bot for this player to use as the base AI
            std::vector<Creature*> activeBots = CollectActiveBots(player);
            if (!activeBots.empty())
                SendFakePartyUpdate(player, activeBots[0]);
        }

        // Also update for existing members who have bots
        for (auto const& memberSlot : group->GetMemberSlots())
        {
            if (memberSlot.guid == guid)
                continue;
            if (Player* member = ObjectAccessor::FindConnectedPlayer(memberSlot.guid))
            {
                if (!member ||
                    !member->IsInWorld() ||
                    member->IsBeingTeleportedNear() ||
                    member->IsBeingTeleportedFar() ||
                    member->IsBeingTeleported())
                    return;

                auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(member);
                if (botsPtr && !botsPtr->empty())
                {
                    std::vector<Creature*> activeBots = CollectActiveBots(member);
                    if (!activeBots.empty())
                        SendFakePartyUpdate(member, activeBots[0]);
                }
            }
        }
    }

    void OnMemberRemove(Group* group, ObjectGuid guid)
    {
        if (!group || !guid)
            return;

        // In battlegrounds the raid-frame update handles bots centrally.
        if (group->isBGGroup())
            return;

        // When a player leaves a group, clear their fake party if they had bots
        if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        {
            if (!player ||
                !player->IsInWorld() ||
                player->IsBeingTeleportedNear() ||
                player->IsBeingTeleportedFar() ||
                player->IsBeingTeleported())
                return;

            auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(player);
            if (botsPtr && !botsPtr->empty())
            {
                // Re-send as standalone fake party (player + bots only)
                FSBParty::SendFakePartyUpdate(player);
            }
        }

        // Update remaining members who have bots
        for (auto const& memberSlot : group->GetMemberSlots())
        {
            if (memberSlot.guid == guid)
                continue;
            if (Player* member = ObjectAccessor::FindConnectedPlayer(memberSlot.guid))
            {
                if (!member ||
                    !member->IsInWorld() ||
                    member->IsBeingTeleportedNear() ||
                    member->IsBeingTeleportedFar() ||
                    member->IsBeingTeleported())
                    return;

                auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(member);
                if (botsPtr && !botsPtr->empty())
                    FSBParty::SendFakePartyUpdate(member);
            }
        }
    }

    std::vector<uint32> GetBotAppliedAuras(Creature* bot)
    {
        std::vector<uint32> result;

        if (!bot)
            return result;

        for (AuraApplication const* aurApp : bot->GetVisibleAuras())
        {
            if (!aurApp)
                continue;

            Aura const* aura = aurApp->GetBase();
            if (!aura)
                continue;

            result.push_back(aura->GetId());
        }

        return result;
    }
}
