#include "Log.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_party_handler.h"

#include "PartyPackets.h"

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

    void SendFakePartyUpdate(Player* player)
    {
        if (!player || !player->GetSession())
            return;

        auto activeBots = CollectActiveBots(player);

        SendFakePartyUpdate(player, activeBots);
    }

	void SendFakePartyUpdate(Player* player)
	{
		if (!player || !player->GetSession())
			return;

		auto activeBots = CollectActiveBots(player);

		SendFakePartyUpdate(player, activeBots);
	}


    void SendFakePartyUpdate(Player* player, std::vector<Creature*> const& activeBots)
    {
        if (!player || !player->GetSession())
            return;

        Group* realGroup = player->GetGroup();

        if (activeBots.empty())
        {
            if (!realGroup)
            {
                // Player is solo → send a clean "clear fake party"
                SendClearFakeParty(player);
            }
            else
            {
                // Player is in a real group → send a normal group-only PartyUpdate
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
                    "FSB: Sent real-only PartyUpdate to {} with {} members (no active bots)",
                    player->GetName(), partyUpdate.PlayerList.size());
            }

            return;
        }

        WorldPackets::Party::PartyUpdate partyUpdate;

        if (realGroup)
        {
            // Player is in a real group — augment it with bots
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
            for (Creature* bot : activeBots)
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
            // Player is solo — create a fake standalone party
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

            for (Creature* bot : activeBots)
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
            "FSB: Sent fake PartyUpdate to {} with {} members (including {} bots)",
            player->GetName(), partyUpdate.PlayerList.size(), activeBots.size());
    }

    void SendBotMemberState(Player* player, Creature* bot)
    {
        if (!player || !player->GetSession() || !bot || !bot->IsInWorld())
            return;

        WorldPackets::Party::PartyMemberFullState packet;
        packet.ForEnemy = false;
        packet.MemberGuid = bot->GetGUID();

        auto& stats = packet.MemberStats;

        // Status
        stats.Status = MEMBER_STATUS_ONLINE;
        if (!bot->IsAlive())
            stats.Status |= MEMBER_STATUS_DEAD;

        // Level
        stats.Level = bot->GetLevel();

        // Health
        stats.CurrentHealth = bot->GetHealth();
        stats.MaxHealth = bot->GetMaxHealth();

        // Power
        stats.PowerType = bot->GetPowerType();
        stats.PowerDisplayID = 0;
        stats.CurrentPower = bot->GetPower(bot->GetPowerType());
        stats.MaxPower = bot->GetMaxPower(bot->GetPowerType());

        // Position
        stats.ZoneID = bot->GetZoneId();
        stats.PositionX = int16(bot->GetPositionX());
        stats.PositionY = int16(bot->GetPositionY());
        stats.PositionZ = int16(bot->GetPositionZ());

        // Spec - bots don't have specs, use 0
        stats.SpecID = 0;

        // PartyType
        stats.PartyType[0] = GROUP_TYPE_NORMAL;
        stats.PartyType[1] = 0;

        // Auras - send visible auras for the bot
        for (AuraApplication const* aurApp : bot->GetVisibleAuras())
        {
            WorldPackets::Party::PartyMemberAuraStates& aura = stats.Auras.emplace_back();
            aura.SpellID = aurApp->GetBase()->GetId();
            aura.ActiveFlags = aurApp->GetEffectMask();
            aura.Flags = aurApp->GetFlags();

            if (aurApp->GetFlags() & AFLAG_SCALABLE)
                for (AuraEffect const* aurEff : aurApp->GetBase()->GetAuraEffects())
                    if (aurApp->HasEffect(aurEff->GetEffIndex()))
                        aura.Points.push_back(float(aurEff->GetAmount()));
        }

        player->SendDirectMessage(packet.Write());
    }

    void SendAllBotMemberStates(Player* player, std::vector<Creature*> const& activeBots)
    {
        if (!player)
            return;

        for (Creature* bot : activeBots)
        {
            if (!bot || !bot->IsInWorld())
                continue;

            SendBotMemberState(player, bot);
        }
    }

    void SendClearFakeParty(Player* player)
    {
        if (!player || !player->GetSession())
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

        TC_LOG_DEBUG("scripts.fsb.party", "FSB: Sent clear fake party to {}", player->GetName());
    }

    void PeriodicPartyNeededCheck(Creature* bot)
    {
        if (!bot || !bot->IsInWorld())
            return;

        if (Player* owner = FSBMgr::Get()->GetBotOwner(bot))
        {
            // Build a single activeBots list for this owner
            std::vector<Creature*> activeBots;

            if (auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(owner))
            {
                for (auto const& botData : *botsPtr)
                {
                    if (botData.runtimeGuid.IsEmpty())
                        continue;

                    if (Creature* bot = ObjectAccessor::GetCreatureOrPetOrVehicle(*owner, botData.runtimeGuid))
                    {
                        if (bot->IsInWorld())
                            activeBots.push_back(bot);
                    }
                }
            }

            if (!activeBots.empty())
            {
                // Use the same activeBots for both packets
                SendFakePartyUpdate(owner, activeBots);
                SendAllBotMemberStates(owner, activeBots);
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
            FSBParty::SendFakePartyUpdate(player);

        // Also update for existing members who have bots
        for (auto const& memberSlot : group->GetMemberSlots())
        {
            if (memberSlot.guid == guid)
                continue;
            if (Player* member = ObjectAccessor::FindConnectedPlayer(memberSlot.guid))
            {
                auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(member);
                if (botsPtr && !botsPtr->empty())
                    FSBParty::SendFakePartyUpdate(member);
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
                auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(member);
                if (botsPtr && !botsPtr->empty())
                    FSBParty::SendFakePartyUpdate(member);
            }
        }
    }
}
