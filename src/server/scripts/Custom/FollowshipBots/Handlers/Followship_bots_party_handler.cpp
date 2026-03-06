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
    uint8 FSBClassToWoWClass(FSB_Class botClass)
    {
        // FSB_Class values don't map 1:1 to WoW Classes enum
        switch (botClass)
        {
        case FSB_Class::Warrior:    return 1;  // CLASS_WARRIOR
        case FSB_Class::Paladin:    return 2;  // CLASS_PALADIN
        case FSB_Class::Hunter:     return 3;  // CLASS_HUNTER
        case FSB_Class::Rogue:      return 4;  // CLASS_ROGUE
        case FSB_Class::Priest:     return 5;  // CLASS_PRIEST
        case FSB_Class::Shaman:     return 7;  // CLASS_SHAMAN
        case FSB_Class::Mage:       return 8;  // CLASS_MAGE
        case FSB_Class::Warlock:    return 9;  // CLASS_WARLOCK
        case FSB_Class::Monk:       return 10; // CLASS_MONK
        case FSB_Class::Druid:      return 11; // CLASS_DRUID
        case FSB_Class::DeathKnight:return 6;  // CLASS_DEATH_KNIGHT
        default:                    return 1;  // fallback to warrior
        }
    }

    uint8 FSBRaceToWoWRace(FSB_Race botRace)
    {
        // FSB_Race maps to Races enum in SharedDefines.h
        switch (botRace)
        {
        case FSB_Race::Human:    return 1;  // RACE_HUMAN
        case FSB_Race::Dwarf:    return 3;  // RACE_DWARF
        case FSB_Race::NightElf: return 4;  // RACE_NIGHTELF
        case FSB_Race::Gnome:    return 7;  // RACE_GNOME
        case FSB_Race::Draenei:  return 11; // RACE_DRAENEI
        case FSB_Race::Worgen:   return 22; // RACE_WORGEN
        case FSB_Race::Pandaren: return 24; // RACE_PANDAREN_NEUTRAL
        case FSB_Race::VoidElf:  return 29; // RACE_VOID_ELF
        default:                 return 1;  // fallback to human
        }
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

    void SendFakePartyUpdate(Player* player)
    {
        if (!player || !player->GetSession())
            return;

        auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(player);
        if (!botsPtr || botsPtr->empty())
            return;

        // Count active (spawned) bots
        std::vector<Creature*> activeBots;
        for (auto const& botData : *botsPtr)
        {
            if (botData.runtimeGuid.IsEmpty())
                continue;

            if (Creature* bot = ObjectAccessor::GetCreatureOrPetOrVehicle(*player, botData.runtimeGuid))
            {
                if (bot->IsInWorld())
                    activeBots.push_back(bot);
            }
        }

        if (activeBots.empty())
            return;

        Group* realGroup = player->GetGroup();

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

            // Add real group members first using MemberSlots (same approach as Group::SendUpdateToPlayer)
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

            // Now append bots
            for (Creature* bot : activeBots)
            {
                FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
                FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());

                WorldPackets::Party::PartyPlayerInfo& info = partyUpdate.PlayerList.emplace_back();
                info.GUID = bot->GetGUID();
                info.Name = bot->GetName();
                info.Class = FSBClassToWoWClass(botClass);
                info.FactionGroup = Player::GetFactionGroupForRace(FSBRaceToWoWRace(botRace));
                info.Connected = true;
                info.Subgroup = 0;
                info.RolesAssigned = GetLfgRoleForBot(bot);
            }

            // Re-add loot/difficulty settings
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

            // Use player's GUID as the fake party GUID
            partyUpdate.PartyGUID = player->GetGUID();
            partyUpdate.LeaderGUID = player->GetGUID();
            partyUpdate.LeaderFactionGroup = Player::GetFactionGroupForRace(player->GetRace());
            partyUpdate.SequenceNum = player->NextGroupUpdateSequenceNumber(GROUP_CATEGORY_HOME);

            // Add the player first
            WorldPackets::Party::PartyPlayerInfo& playerInfo = partyUpdate.PlayerList.emplace_back();
            playerInfo.GUID = player->GetGUID();
            playerInfo.Name = player->GetName();
            playerInfo.Class = player->GetClass();
            playerInfo.FactionGroup = Player::GetFactionGroupForRace(player->GetRace());
            playerInfo.Connected = true;
            playerInfo.Subgroup = 0;
            playerInfo.RolesAssigned = 0;

            partyUpdate.MyIndex = 0;

            // Add bots
            for (Creature* bot : activeBots)
            {
                FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
                FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());

                WorldPackets::Party::PartyPlayerInfo& info = partyUpdate.PlayerList.emplace_back();
                info.GUID = bot->GetGUID();
                info.Name = bot->GetName();
                info.Class = FSBClassToWoWClass(botClass);
                info.FactionGroup = Player::GetFactionGroupForRace(FSBRaceToWoWRace(botRace));
                info.Connected = true;
                info.Subgroup = 0;
                info.RolesAssigned = GetLfgRoleForBot(bot);
            }
        }

        player->SendDirectMessage(partyUpdate.Write());

        TC_LOG_DEBUG("scripts.fsb.party", "FSB: Sent fake PartyUpdate to {} with {} members (including {} bots)",
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

        // Spec — bots don't have specs, use 0
        stats.SpecID = 0;

        // PartyType
        stats.PartyType[0] = GROUP_TYPE_NORMAL;
        stats.PartyType[1] = 0;

        // Auras — send visible auras for the bot
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

    void SendAllBotMemberStates(Player* player)
    {
        if (!player)
            return;

        auto botsPtr = FSBMgr::Get()->GetPersistentBotsForPlayer(player);
        if (!botsPtr)
            return;

        for (auto const& botData : *botsPtr)
        {
            if (botData.runtimeGuid.IsEmpty())
                continue;

            if (Creature* bot = ObjectAccessor::GetCreatureOrPetOrVehicle(*player, botData.runtimeGuid))
            {
                if (bot->IsInWorld())
                    SendBotMemberState(player, bot);
            }
        }
    }

    void SendClearFakeParty(Player* player)
    {
        if (!player || !player->GetSession())
            return;

        // Only clear if the player is NOT in a real group
        if (player->GetGroup())
            return;

        WorldPackets::Party::PartyUpdate partyUpdate;
        partyUpdate.PartyFlags = GROUP_FLAG_NONE;
        partyUpdate.PartyIndex = GROUP_CATEGORY_HOME;
        partyUpdate.PartyType = GROUP_TYPE_NONE;
        partyUpdate.PartyGUID = player->GetGUID();
        partyUpdate.LeaderGUID = ObjectGuid::Empty;
        partyUpdate.SequenceNum = player->NextGroupUpdateSequenceNumber(GROUP_CATEGORY_HOME);
        partyUpdate.MyIndex = -1;

        player->SendDirectMessage(partyUpdate.Write());

        TC_LOG_DEBUG("scripts.fsb.party", "FSB: Sent clear fake party to {}", player->GetName());
    }
}
