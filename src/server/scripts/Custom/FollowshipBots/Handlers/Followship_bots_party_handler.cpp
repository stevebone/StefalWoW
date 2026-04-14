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

#include "PartyPackets.h"
#include "PacketOperators.h"

#include "Log.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"

#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include "Followship_bots_mgr.h"

#include "Followship_bots_events_handler.h"
#include "Followship_bots_party_handler.h"
#include "Followship_bots_pet_handler.h"



namespace FSBParty
{
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
                WorldPackets::Party::PartyUpdate partyUpdate;
                partyUpdate.PartyFlags = realGroup->GetGroupFlags();
                partyUpdate.PartyIndex = realGroup->GetGroupCategory();
                partyUpdate.PartyType = GROUP_TYPE_NORMAL;
                partyUpdate.PartyGUID = realGroup->GetGUID();
                partyUpdate.LeaderGUID = realGroup->GetLeaderGUID();
                partyUpdate.LeaderFactionGroup = Player::GetFactionGroupForRace(player->GetRace());
                partyUpdate.SequenceNum = player->NextGroupUpdateSequenceNumber(realGroup->GetGroupCategory());

                int32 myIndex = -1;
                uint8 index = 0;

                for (auto const& memberSlot : realGroup->GetMemberSlots())
                {
                    if (memberSlot.guid == player->GetGUID())
                        myIndex = index;

                    Player* member = ObjectAccessor::FindConnectedPlayer(memberSlot.guid);

                    WorldPackets::Party::PartyPlayerInfo& info = partyUpdate.PlayerList.emplace_back();
                    info.GUID = memberSlot.guid;
                    info.Name = memberSlot.name;
                    info.Class = memberSlot._class;
                    info.FactionGroup = Player::GetFactionGroupForRace(memberSlot.race);
                    info.Connected = member && member->GetSession() && !member->GetSession()->PlayerLogout();
                    info.Subgroup = memberSlot.group;
                    info.Flags = memberSlot.flags;
                    info.RolesAssigned = memberSlot.roles;
                    ++index;
                }

                partyUpdate.MyIndex = myIndex;

                if (partyUpdate.PlayerList.size() > 1)
                {
                    partyUpdate.LootSettings.emplace();
                    partyUpdate.LootSettings->Method = realGroup->GetLootMethod();
                    partyUpdate.LootSettings->Threshold = realGroup->GetLootThreshold();
                    partyUpdate.LootSettings->LootMaster = ObjectGuid::Empty;

                    partyUpdate.DifficultySettings.emplace();
                    partyUpdate.DifficultySettings->DungeonDifficultyID = realGroup->GetDungeonDifficultyID();
                    partyUpdate.DifficultySettings->RaidDifficultyID = realGroup->GetRaidDifficultyID();
                    partyUpdate.DifficultySettings->LegacyRaidDifficultyID = realGroup->GetLegacyRaidDifficultyID();
                }

                player->SendDirectMessage(partyUpdate.Write());

                TC_LOG_DEBUG("scripts.fsb.party",
                    "FSB: SendFakePartyUpdate Sent real-only PartyUpdate to {} with {} members (no active bots)",
                    player->GetName(), partyUpdate.PlayerList.size());
            }

            return;
        }

        WorldPackets::Party::PartyUpdate partyUpdate;

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

        // filter bots
        std::vector<Creature*> safeBots;
        for (Creature* bot : trimmedBots)
        {
            if (bot && bot->IsInWorld())
                safeBots.push_back(bot);
        }

        baseAI->partyBots = safeBots;

        // ------------------------------------------------------------

        if (realGroup)
        {
            // Player is in a real group - augment it with bots
            partyUpdate.PartyFlags = realGroup->GetGroupFlags();
            partyUpdate.PartyIndex = realGroup->GetGroupCategory();
            partyUpdate.PartyType = GROUP_TYPE_NORMAL;
            partyUpdate.PartyGUID = realGroup->GetGUID();
            partyUpdate.LeaderGUID = realGroup->GetLeaderGUID();
            partyUpdate.LeaderFactionGroup = Player::GetFactionGroupForRace(player->GetRace());
            partyUpdate.SequenceNum = player->NextGroupUpdateSequenceNumber(realGroup->GetGroupCategory());

            int32 myIndex = -1;
            uint8 index = 0;

            for (auto const& memberSlot : realGroup->GetMemberSlots())
            {
                if (memberSlot.guid == player->GetGUID())
                    myIndex = index;

                Player* member = ObjectAccessor::FindConnectedPlayer(memberSlot.guid);

                WorldPackets::Party::PartyPlayerInfo& info = partyUpdate.PlayerList.emplace_back();
                info.GUID = memberSlot.guid;
                info.Name = memberSlot.name;
                info.Class = memberSlot._class;
                info.FactionGroup = Player::GetFactionGroupForRace(memberSlot.race);
                info.Connected = member && member->GetSession() && !member->GetSession()->PlayerLogout();
                info.Subgroup = memberSlot.group;
                info.Flags = memberSlot.flags;
                info.RolesAssigned = memberSlot.roles;
                ++index;
            }

            partyUpdate.MyIndex = myIndex;

            // Append bots
            for (Creature* bot : safeBots)
            {
                FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
                FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());

                WorldPackets::Party::PartyPlayerInfo& info = partyUpdate.PlayerList.emplace_back();
                info.GUID = bot->GetGUID();
                info.Name = bot->GetName();
                info.Class = FSBUtils::FSBToTCClass(botClass);
                info.FactionGroup = Player::GetFactionGroupForRace(FSBUtils::BotRaceToTC(botRace));
                info.Connected = true;
                info.Subgroup = 0;
                info.RolesAssigned = GetLfgRoleForBot(bot);
            }

            if (partyUpdate.PlayerList.size() > 1)
            {
                partyUpdate.LootSettings.emplace();
                partyUpdate.LootSettings->Method = realGroup->GetLootMethod();
                partyUpdate.LootSettings->Threshold = realGroup->GetLootThreshold();
                partyUpdate.LootSettings->LootMaster = ObjectGuid::Empty;

                partyUpdate.DifficultySettings.emplace();
                partyUpdate.DifficultySettings->DungeonDifficultyID = realGroup->GetDungeonDifficultyID();
                partyUpdate.DifficultySettings->RaidDifficultyID = realGroup->GetRaidDifficultyID();
                partyUpdate.DifficultySettings->LegacyRaidDifficultyID = realGroup->GetLegacyRaidDifficultyID();
            }
        }
        else
        {
            // Player is solo - create a fake standalone party
            partyUpdate.PartyFlags = GROUP_FLAG_NONE;
            partyUpdate.PartyIndex = GROUP_CATEGORY_HOME;
            partyUpdate.PartyType = GROUP_TYPE_NORMAL;

            partyUpdate.PartyGUID = player->GetGUID();
            partyUpdate.LeaderGUID = player->GetGUID();
            partyUpdate.LeaderFactionGroup = Player::GetFactionGroupForRace(player->GetRace());
            partyUpdate.SequenceNum = player->NextGroupUpdateSequenceNumber(GROUP_CATEGORY_HOME);

            WorldPackets::Party::PartyPlayerInfo& playerInfo = partyUpdate.PlayerList.emplace_back();
            playerInfo.GUID = player->GetGUID();
            playerInfo.Name = player->GetName();
            playerInfo.Class = player->GetClass();
            playerInfo.FactionGroup = Player::GetFactionGroupForRace(player->GetRace());
            playerInfo.Connected = true;
            playerInfo.Subgroup = 0;
            playerInfo.RolesAssigned = 0;

            partyUpdate.MyIndex = 0;

            for (Creature* bot : safeBots)
            {
                FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
                FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());

                WorldPackets::Party::PartyPlayerInfo& info = partyUpdate.PlayerList.emplace_back();
                info.GUID = bot->GetGUID();
                info.Name = bot->GetName();
                info.Class = FSBUtils::FSBToTCClass(botClass);
                info.FactionGroup = Player::GetFactionGroupForRace(FSBUtils::BotRaceToTC(botRace));
                info.Connected = true;
                info.Subgroup = 0;
                info.RolesAssigned = GetLfgRoleForBot(bot);
            }
        }

        player->SendDirectMessage(partyUpdate.Write());

        TC_LOG_DEBUG("scripts.fsb.party",
            "FSB: SendFakePartyUpdate sent to player {} with {} members (including {} bots)",
            player->GetName(), partyUpdate.PlayerList.size(), safeBots.size());
    }

    void SendBotMemberState(Player* player, Creature* bot)
    {
        if (!player || !player->GetSession() || !player->IsInWorld() || player->IsBeingTeleportedNear() || player->IsBeingTeleported() || player->IsBeingTeleportedFar())
            return;

        if (player->GetSession()->PlayerLoading())
            return;

        if (!bot || !bot->IsInWorld())
            return;

        if (player->GetMapId() != bot->GetMapId())
            return;

        if (!player->HaveAtClient(bot))
            return;

        WorldPackets::Party::PartyMemberFullState packet;
        packet.ForEnemy = false;
        packet.MemberGuid = bot->GetGUID();

        auto& stats = packet.MemberStats;

        // Status -- crashing the client for now
        /*
        if (!bot || !bot->IsInWorld())
        {
            stats.Status = MEMBER_STATUS_OFFLINE;
        }
        else if (bot->HasAura(SPELL_SPECIAL_GHOST))
        {
            stats.Status = MEMBER_STATUS_GHOST;
        }
        else if (!bot->IsAlive())
        {
            stats.Status = MEMBER_STATUS_DEAD;
        }
        else
        {
            stats.Status = MEMBER_STATUS_ONLINE;
        }
        */

        // Level
        stats.Level = bot->GetLevel();

        // Health
        stats.CurrentHealth = bot->GetHealth();
        stats.MaxHealth = bot->GetMaxHealth();

        // Power
        stats.PowerType = bot->GetPowerType();
        //stats.PowerDisplayID = 0;
        stats.CurrentPower = bot->GetPower(bot->GetPowerType());
        stats.MaxPower = bot->GetMaxPower(bot->GetPowerType());

        // Position
        stats.ZoneID = bot->GetZoneId();
        stats.PositionX = bot->GetPositionX();
        stats.PositionY = bot->GetPositionY();
        stats.PositionZ = bot->GetPositionZ();

        // Spec - bots don't have specs, use 0
        //stats.SpecID = 0;

        // PartyType
        stats.PartyType[0] = GROUP_TYPE_NORMAL;
        //stats.PartyType[1] = 0;

        stats.Auras = GetBotAppliedAuras(bot);

        // Pet
        /*
        if (FSBPet::BotHasPet(bot))
        {
            Unit* pet = FSBPet::GetBotPet(bot);

            stats.PetStats.emplace();

            stats.PetStats->GUID = pet->GetGUID();
            stats.PetStats->Name = pet->GetName();
            stats.PetStats->ModelId = pet->GetDisplayId();

            stats.PetStats->CurrentHealth = pet->GetHealth();
            stats.PetStats->MaxHealth = pet->GetMaxHealth();

            for (AuraApplication const* aurApp : pet->GetVisibleAuras())
            {
                WorldPackets::Party::PartyMemberAuraStates& aura = stats.PetStats->Auras.emplace_back();

                aura.SpellID = aurApp->GetBase()->GetId();
                aura.ActiveFlags = aurApp->GetEffectMask();
                aura.Flags = aurApp->GetFlags();

                if (aurApp->GetFlags() & AFLAG_SCALABLE)
                    for (AuraEffect const* aurEff : aurApp->GetBase()->GetAuraEffects())
                        if (aurApp->HasEffect(aurEff->GetEffIndex()))
                            aura.Points.push_back(float(aurEff->GetAmount()));
            }
        }
        */
        TC_LOG_DEBUG("scripts.fsb.party", "FSB: SendBotMemberState Bot {} sending {} auras", bot->GetEntry(), stats.Auras.size());

        player->SendDirectMessage(packet.Write());
    }

    void SendClearFakeParty(Player* player)
    {
        if (!player || !player->GetSession() || player->IsBeingTeleportedNear() || player->IsBeingTeleported() || player->IsBeingTeleportedFar() || !player->IsInWorld())
            return;

        // Only clear if the player is NOT in a real group
        if (player->GetGroup() && player->GetGroup()->GetMembersCount() > 1)
            return;

        WorldPackets::Party::PartyUpdate partyUpdate;
        partyUpdate.PartyFlags = GROUP_FLAG_NONE;
        partyUpdate.PartyIndex = GROUP_CATEGORY_HOME;
        partyUpdate.PartyType = GROUP_TYPE_NONE;
        partyUpdate.PartyGUID = player->GetGUID();
        partyUpdate.PartyGUID = ObjectGuid::Empty;
        partyUpdate.LeaderGUID = ObjectGuid::Empty;
        partyUpdate.LeaderFactionGroup = Player::GetFactionGroupForRace(player->GetRace());
        partyUpdate.SequenceNum = player->NextGroupUpdateSequenceNumber(GROUP_CATEGORY_HOME);
        partyUpdate.MyIndex = -1;

        player->SendDirectMessage(partyUpdate.Write());

        TC_LOG_DEBUG("scripts.fsb.party", "FSB: SendClearFakeParty sent to player {}", player->GetName());
    }

    void PeriodicPartyNeededCheck(Creature* bot)
    {
        if (!bot || !bot->IsInWorld())
            return;

        if (Player* owner = FSBMgr::Get()->GetBotOwner(bot))
        {
            if (!owner || !owner->GetSession() || owner->IsBeingTeleportedNear() || owner->IsBeingTeleported() || owner->IsBeingTeleportedFar() || !owner->IsInWorld())
                return;
            // Build a single activeBots list for this owner
            std::vector<Creature*> activeBots = CollectActiveBots(owner);

            if (!activeBots.empty())
            {
                auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->GetAI());
                if (!baseAI)
                    return;

                baseAI->partyBots = activeBots;

                // Use the same activeBots for both packets
                SendFakePartyUpdate(owner, bot);
                SendBotMemberState(owner, bot);
            }
            else
            {
                // No active bots: do NOT send member states
                SendClearFakeParty(owner);
            }
        }
    }

    void OnMemberAdd(Group* group, ObjectGuid guid)
    {
        if (!group || !guid)
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

            SendFakePartyUpdate(player);
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
                    SendFakePartyUpdate(member);
            }
        }
    }

    void OnMemberRemove(Group* group, ObjectGuid guid)
    {
        if (!group || !guid)
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

    std::vector<WorldPackets::Party::PartyMemberAuraStates>
        GetBotAppliedAuras(Creature* bot)
    {
        std::vector<WorldPackets::Party::PartyMemberAuraStates> result;

        if (!bot)
            return result;

        for (AuraApplication const* aurApp : bot->GetVisibleAuras())
        {
            if (!aurApp)
                continue;

            Aura const* aura = aurApp->GetBase();
            if (!aura)
                continue;

            WorldPackets::Party::PartyMemberAuraStates auraState;
            auraState.SpellID = aura->GetId();
            auraState.ActiveFlags = aurApp->GetEffectMask();
            auraState.Flags = aurApp->GetFlags();

            if (aurApp->GetFlags() & AFLAG_SCALABLE)
            {
                for (AuraEffect const* aurEff : aura->GetAuraEffects())
                {
                    if (aurApp->HasEffect(aurEff->GetEffIndex()))
                        auraState.Points.push_back(float(aurEff->GetAmount()));
                }
            }

            result.push_back(auraState);
        }

        return result;
    }
}
