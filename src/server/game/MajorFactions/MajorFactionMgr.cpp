/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MajorFactionMgr.h"
#include "CollectionMgr.h"
#include "ConditionMgr.h"
#include "DB2Stores.h"
#include "DatabaseEnv.h"
#include "DB2Structure.h"
#include "Item.h"
#include "Log.h"
#include "Mail.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "ReputationMgr.h"
#include "Timer.h"
#include "WorldDatabase.h"
#include "WorldSession.h"
#include <algorithm>

MajorFactionMgr* MajorFactionMgr::instance()
{
    static MajorFactionMgr instance;
    return &instance;
}

void MajorFactionMgr::Load()
{
    uint32 oldMSTime = getMSTime();

    _majorFactionIDs.clear();
    _majorFactionSet.clear();
    _factionToCovenant.clear();
    _covenantToFaction.clear();
    _covenantByFaction.clear();
    _renownRewardsByFaction.clear();
    _maxRenownLevelByFaction.clear();
    _paragonByFaction.clear();

    IndexFactions();
    IndexCovenants();
    IndexRenownRewards();
    IndexParagons();

    TC_LOG_INFO("server.loading", "MajorFactionMgr: indexed {} Major Factions, {} covenants, {} factions with renown rewards in {} ms",
        _majorFactionIDs.size(), _covenantToFaction.size(), _renownRewardsByFaction.size(),
        GetMSTimeDiffToNow(oldMSTime));
}

void MajorFactionMgr::IndexFactions()
{
    for (FactionEntry const* faction : sFactionStore)
    {
        if (!faction)
            continue;

        // Identity rule: ParagonFactionID && RenownCurrencyID. Matches the
        // client UI rule (C_MajorFactions consumers walk Faction.db2 with
        // exactly this predicate).
        if (faction->ParagonFactionID == 0 || faction->RenownCurrencyID == 0)
            continue;

        _majorFactionIDs.push_back(faction->ID);
        _majorFactionSet.insert(faction->ID);
    }

    std::sort(_majorFactionIDs.begin(), _majorFactionIDs.end());
}

void MajorFactionMgr::IndexCovenants()
{
    for (CovenantEntry const* covenant : sCovenantStore)
    {
        if (!covenant || covenant->FactionID == 0)
            continue;

        uint32 factionId = uint32(covenant->FactionID);
        _factionToCovenant[factionId] = covenant->ID;
        _covenantToFaction[covenant->ID] = factionId;
        _covenantByFaction[factionId] = covenant;
    }
}

void MajorFactionMgr::IndexRenownRewards()
{
    // Walk RenownRewards rows and bucket by FactionID via the Covenant join.
    // Rows whose CovenantID has no matching Covenant row (or whose covenant
    // does not map to a known Major Faction) are silently skipped - they
    // belong to legacy Shadowlands covenant tracks or to data drift.
    for (RenownRewardsEntry const* reward : sRenownRewardsStore)
    {
        if (!reward)
            continue;

        auto factionItr = _covenantToFaction.find(uint32(reward->CovenantID));
        if (factionItr == _covenantToFaction.end())
            continue;

        uint32 factionId = factionItr->second;
        if (!_majorFactionSet.contains(factionId))
            continue;

        _renownRewardsByFaction[factionId].push_back(reward);

        if (reward->Level > 0)
        {
            uint32& maxLevel = _maxRenownLevelByFaction[factionId];
            maxLevel = std::max(maxLevel, uint32(reward->Level));
        }
    }

    // Sort each bucket by (Level asc, UiOrder asc) for deterministic
    // dispatch order; the client renders rewards in this order too.
    for (auto& [factionId, rewards] : _renownRewardsByFaction)
    {
        std::sort(rewards.begin(), rewards.end(), [](RenownRewardsEntry const* a, RenownRewardsEntry const* b)
        {
            if (a->Level != b->Level)
                return a->Level < b->Level;
            if (a->UiOrder != b->UiOrder)
                return a->UiOrder < b->UiOrder;
            return a->ID < b->ID;
        });
    }
}

void MajorFactionMgr::IndexParagons()
{
    for (FactionEntry const* faction : sFactionStore)
    {
        if (!faction || !_majorFactionSet.contains(faction->ID))
            continue;

        // ParagonReputation rows are keyed by the paragon-flavour faction
        // (Faction.ParagonFactionID), not by the major faction itself.
        for (ParagonReputationEntry const* paragon : sParagonReputationStore)
        {
            if (paragon && uint32(paragon->FactionID) == uint32(faction->ParagonFactionID))
            {
                _paragonByFaction[faction->ID] = paragon;
                break;
            }
        }
    }
}

void MajorFactionMgr::LoadWorldData()
{
    uint32 oldMSTime = getMSTime();
    _configByFaction.clear();

    //                                                  0          1                          2                  3
    QueryResult result = WorldDatabase.Query("SELECT factionId, hiddenFromExpansionPage, displayAsJourney, useJourneyRewardTrack, "
        //  4                       5            6              7                    8
        "useJourneyUnlockToast, uiPriority, introQuestId, playerCompanionId, renownCampaignId "
        "FROM major_faction_config");

    uint32 count = 0;
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 factionId = fields[0].GetUInt32();

            if (!IsMajorFaction(factionId))
            {
                TC_LOG_ERROR("sql.sql", "Table `major_faction_config` references factionId {} which is not a Major Faction, skipped", factionId);
                continue;
            }

            MajorFactions::MajorFactionConfig& config = _configByFaction[factionId];
            config.HiddenFromExpansionPage = fields[1].GetBool();
            config.DisplayAsJourney        = fields[2].GetBool();
            config.UseJourneyRewardTrack   = fields[3].GetBool();
            config.UseJourneyUnlockToast   = fields[4].GetBool();
            config.UiPriority              = fields[5].GetInt32();
            config.IntroQuestID            = fields[6].GetUInt32();
            config.PlayerCompanionID       = fields[7].GetUInt32();
            config.RenownCampaignID        = fields[8].GetUInt32();

            // Validate Campaign FK if set.
            if (config.RenownCampaignID && !sCampaignStore.LookupEntry(config.RenownCampaignID))
            {
                TC_LOG_ERROR("sql.sql", "Table `major_faction_config` row factionId {} references unknown renownCampaignId {}, clearing",
                    factionId, config.RenownCampaignID);
                config.RenownCampaignID = 0;
            }

            ++count;
        } while (result->NextRow());
    }

    TC_LOG_INFO("server.loading", "Loaded {} Major Faction configs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

// ----- Identity ------------------------------------------------------------

bool MajorFactionMgr::IsMajorFaction(uint32 factionId) const
{
    return _majorFactionSet.contains(factionId);
}

bool MajorFactionMgr::IsMajorFaction(FactionEntry const* factionEntry) const
{
    return factionEntry && _majorFactionSet.contains(factionEntry->ID);
}

std::vector<uint32> MajorFactionMgr::GetMajorFactionIDsForExpansion(uint8 expansion) const
{
    std::vector<uint32> result;
    result.reserve(_majorFactionIDs.size());
    for (uint32 factionId : _majorFactionIDs)
    {
        FactionEntry const* faction = sFactionStore.LookupEntry(factionId);
        if (faction && faction->Expansion == expansion)
            result.push_back(factionId);
    }
    return result;
}

// ----- Renown --------------------------------------------------------------

uint32 MajorFactionMgr::GetMaxRenownLevel(uint32 factionId) const
{
    auto itr = _maxRenownLevelByFaction.find(factionId);
    if (itr != _maxRenownLevelByFaction.end())
        return itr->second;

    // Fallback: read CurrencyTypes.MaxQty of the renown currency. Used for
    // factions whose RenownRewards.db2 rows have not been seeded yet.
    if (FactionEntry const* faction = sFactionStore.LookupEntry(factionId))
        if (faction->RenownCurrencyID > 0)
            if (CurrencyTypesEntry const* currency = sCurrencyTypesStore.LookupEntry(uint32(faction->RenownCurrencyID)))
                return currency->MaxQty;

    return 0;
}

uint32 MajorFactionMgr::GetRenownCurrencyID(uint32 factionId) const
{
    if (FactionEntry const* faction = sFactionStore.LookupEntry(factionId))
        return uint32(faction->RenownCurrencyID);
    return 0;
}

uint32 MajorFactionMgr::GetReputationPerRenownLevel(uint32 /*factionId*/) const
{
    // Currently constant. Kept as a per-faction lookup so future hotfix
    // tuning can override per-faction (e.g. Ritual Sites short track) without
    // touching ReputationMgr.
    return ReputationPerRenownLevel;
}

// ----- Rewards -------------------------------------------------------------

std::vector<RenownRewardsEntry const*> MajorFactionMgr::GetRenownRewardsForLevel(uint32 factionId, uint32 renownLevel) const
{
    std::vector<RenownRewardsEntry const*> result;

    auto itr = _renownRewardsByFaction.find(factionId);
    if (itr == _renownRewardsByFaction.end())
        return result;

    for (RenownRewardsEntry const* reward : itr->second)
        if (uint32(reward->Level) == renownLevel)
            result.push_back(reward);

    return result;
}

std::vector<RenownRewardsEntry const*> MajorFactionMgr::GetRenownRewardsBetween(uint32 factionId, uint32 fromLevel, uint32 toLevel) const
{
    std::vector<RenownRewardsEntry const*> result;

    if (fromLevel >= toLevel)
        return result;

    auto itr = _renownRewardsByFaction.find(factionId);
    if (itr == _renownRewardsByFaction.end())
        return result;

    for (RenownRewardsEntry const* reward : itr->second)
    {
        uint32 level = uint32(reward->Level);
        if (level > fromLevel && level <= toLevel)
            result.push_back(reward);
    }

    return result;
}

std::vector<RenownRewardsEntry const*> const* MajorFactionMgr::GetAllRenownRewards(uint32 factionId) const
{
    auto itr = _renownRewardsByFaction.find(factionId);
    if (itr == _renownRewardsByFaction.end())
        return nullptr;
    return &itr->second;
}

// ----- Paragon -------------------------------------------------------------

ParagonReputationEntry const* MajorFactionMgr::GetParagonForMajorFaction(uint32 factionId) const
{
    auto itr = _paragonByFaction.find(factionId);
    return itr != _paragonByFaction.end() ? itr->second : nullptr;
}

// ----- Covenant ------------------------------------------------------------

CovenantEntry const* MajorFactionMgr::GetCovenantForFaction(uint32 factionId) const
{
    auto itr = _covenantByFaction.find(factionId);
    return itr != _covenantByFaction.end() ? itr->second : nullptr;
}

uint32 MajorFactionMgr::GetCovenantIDForFaction(uint32 factionId) const
{
    auto itr = _factionToCovenant.find(factionId);
    return itr != _factionToCovenant.end() ? itr->second : 0u;
}

uint32 MajorFactionMgr::GetFactionForCovenant(uint32 covenantId) const
{
    auto itr = _covenantToFaction.find(covenantId);
    return itr != _covenantToFaction.end() ? itr->second : 0u;
}

// ----- Config --------------------------------------------------------------

MajorFactions::MajorFactionConfig const* MajorFactionMgr::GetConfig(uint32 factionId) const
{
    auto itr = _configByFaction.find(factionId);
    return itr != _configByFaction.end() ? &itr->second : nullptr;
}

bool MajorFactionMgr::IsHiddenFromExpansionPage(uint32 factionId) const
{
    MajorFactions::MajorFactionConfig const* config = GetConfig(factionId);
    return config && config->HiddenFromExpansionPage;
}

bool MajorFactionMgr::ShouldDisplayAsJourney(uint32 factionId) const
{
    MajorFactions::MajorFactionConfig const* config = GetConfig(factionId);
    return config && config->DisplayAsJourney;
}

bool MajorFactionMgr::ShouldUseJourneyRewardTrack(uint32 factionId) const
{
    MajorFactions::MajorFactionConfig const* config = GetConfig(factionId);
    return config && config->UseJourneyRewardTrack;
}

bool MajorFactionMgr::ShouldUseJourneyUnlockToast(uint32 factionId) const
{
    MajorFactions::MajorFactionConfig const* config = GetConfig(factionId);
    return config && config->UseJourneyUnlockToast;
}

uint32 MajorFactionMgr::GetPlayerCompanionID(uint32 factionId) const
{
    MajorFactions::MajorFactionConfig const* config = GetConfig(factionId);
    return config ? config->PlayerCompanionID : 0u;
}

uint32 MajorFactionMgr::GetIntroQuestID(uint32 factionId) const
{
    MajorFactions::MajorFactionConfig const* config = GetConfig(factionId);
    return config ? config->IntroQuestID : 0u;
}

uint32 MajorFactionMgr::GetRenownCampaignID(uint32 factionId) const
{
    MajorFactions::MajorFactionConfig const* config = GetConfig(factionId);
    return config ? config->RenownCampaignID : 0u;
}

std::string_view MajorFactionMgr::GetTextureKitPrefix(uint32 factionId) const
{
    // Resolution chain: faction -> renown campaign -> Campaign.UiTextureKitID
    //                  -> UiTextureKit.KitPrefix (the string the client uses
    //                     as atlas-name suffix, e.g. "MajorFaction-DragonscaleExpedition").
    uint32 campaignId = GetRenownCampaignID(factionId);
    if (!campaignId)
        return {};

    CampaignEntry const* campaign = sCampaignStore.LookupEntry(campaignId);
    if (!campaign || campaign->UiTextureKitID <= 0)
        return {};

    UiTextureKitEntry const* kit = sUiTextureKitStore.LookupEntry(uint32(campaign->UiTextureKitID));
    if (!kit || !kit->KitPrefix)
        return {};

    return kit->KitPrefix;
}

// ----- Player view ---------------------------------------------------------

bool MajorFactionMgr::IsUnlockedForPlayer(Player const* player, uint32 factionId) const
{
    if (!player || !IsMajorFaction(factionId))
        return false;

    FactionEntry const* faction = sFactionStore.LookupEntry(factionId);
    if (!faction)
        return false;

    // If an intro quest is configured, treat the faction as locked until the
    // quest is rewarded.
    if (uint32 introQuest = GetIntroQuestID(factionId))
        if (!player->GetQuestRewardStatus(introQuest))
            return false;

    // Otherwise: unlocked iff the faction is visible to the player.
    ReputationMgr const& repMgr = player->GetReputationMgr();
    if (FactionState const* state = repMgr.GetState(faction))
        return state->Flags.HasFlag(ReputationFlags::Visible);

    return false;
}

uint32 MajorFactionMgr::GetPlayerRenownLevel(Player const* player, uint32 factionId) const
{
    if (!player)
        return 0;

    FactionEntry const* faction = sFactionStore.LookupEntry(factionId);
    if (!faction)
        return 0;

    return uint32(player->GetReputationMgr().GetRenownLevel(faction));
}

uint32 MajorFactionMgr::GetPlayerReputationThisLevel(Player const* player, uint32 factionId) const
{
    if (!player || !IsMajorFaction(factionId))
        return 0;

    FactionEntry const* faction = sFactionStore.LookupEntry(factionId);
    if (!faction)
        return 0;

    int32 standing = player->GetReputationMgr().GetReputation(faction);
    if (standing <= 0)
        return 0;

    uint32 perLevel = GetReputationPerRenownLevel(factionId);
    if (perLevel == 0)
        return 0;

    return uint32(standing) % perLevel;
}

// ----- Reward dispatch -----------------------------------------------------

void MajorFactionMgr::GrantRenownLevelRewards(Player* player, uint32 factionId, uint32 fromLevel, uint32 toLevel, bool accountSync) const
{
    if (!player || fromLevel >= toLevel)
        return;

    auto rewards = GetRenownRewardsBetween(factionId, fromLevel, toLevel);
    if (rewards.empty())
        return;

    for (RenownRewardsEntry const* reward : rewards)
    {
        bool isAccountUnlock = (uint32(reward->Flags) & uint32(MajorFactions::RenownRewardFlags::AccountUnlock)) != 0;

        // On account-sync (alt inherits renown from a higher toon), skip
        // account-wide rewards that have already been granted on the
        // original toon. Character-bound rewards still need to be granted
        // because they're per-toon (e.g. Crafter's Knowledge).
        if (accountSync && isAccountUnlock)
            continue;

        if (player->GetReputationMgr().IsRenownRewardGranted(reward->ID, isAccountUnlock))
            continue;

        GrantSingleRenownReward(player, reward);
        player->GetReputationMgr().MarkRenownRewardGranted(reward->ID, isAccountUnlock);
    }
}

void MajorFactionMgr::GrantSingleRenownReward(Player* player, RenownRewardsEntry const* reward) const
{
    if (!player || !reward)
        return;

    // PlayerConditionID gate: if set and not satisfied, skip silently. The
    // client itself filters the reward row in this case so this is the
    // server mirror.
    if (reward->PlayerConditionID > 0)
    {
        PlayerConditionEntry const* condition = sPlayerConditionStore.LookupEntry(uint32(reward->PlayerConditionID));
        if (condition && !ConditionMgr::IsPlayerMeetingCondition(player, condition))
            return;
    }

    // Item grant.
    if (reward->ItemID > 0)
    {
        if (!player->AddItem(uint32(reward->ItemID), 1))
        {
            TC_LOG_WARN("server.major-factions", "Player {} GUID {} inventory full for RenownReward {} ItemID {} - sending via mail",
                player->GetName(), player->GetGUID().ToString(), reward->ID, reward->ItemID);
            // Fallback: mail the item so the grant is not silently lost.
            MailDraft draft("Renown Reward", "");
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            if (Item* item = Item::CreateItem(uint32(reward->ItemID), 1, ItemContext::NONE, player))
            {
                item->SaveToDB(trans);
                draft.AddItem(item);
            }
            draft.SendMailTo(trans, MailReceiver(player), MailSender(MAIL_CREATURE, 0, MAIL_STATIONERY_DEFAULT));
            CharacterDatabase.CommitTransaction(trans);
        }
    }

    // Spell grant.
    if (reward->SpellID > 0)
        player->LearnSpell(uint32(reward->SpellID), false);

    // Mount grant (account-wide via CollectionMgr - Mount.SourceSpellID).
    if (reward->MountID > 0)
    {
        if (MountEntry const* mount = sMountStore.LookupEntry(uint32(reward->MountID)))
            if (CollectionMgr* coll = player->GetSession()->GetCollectionMgr())
                coll->AddMount(uint32(mount->SourceSpellID), MountStatusFlags::MOUNT_STATUS_NONE, false, false);
    }

    // Transmog appearance grant (ItemModifiedAppearance.ID).
    if (reward->TransmogID > 0)
    {
        if (ItemModifiedAppearanceEntry const* appearance = sItemModifiedAppearanceStore.LookupEntry(uint32(reward->TransmogID)))
            if (CollectionMgr* coll = player->GetSession()->GetCollectionMgr())
                coll->AddItemAppearance(appearance->ItemID, uint32(appearance->ItemAppearanceModifierID));
    }

    // Transmog set grant.
    if (reward->TransmogSetID > 0)
        if (CollectionMgr* coll = player->GetSession()->GetCollectionMgr())
            coll->AddTransmogSet(uint32(reward->TransmogSetID));

    // Transmog illusion grant.
    if (reward->TransmogIllusionID > 0)
        if (CollectionMgr* coll = player->GetSession()->GetCollectionMgr())
            coll->AddTransmogIllusion(uint32(reward->TransmogIllusionID));

    // Title grant.
    if (reward->CharTitlesID > 0)
        if (CharTitlesEntry const* title = sCharTitlesStore.LookupEntry(uint32(reward->CharTitlesID)))
            player->SetTitle(title, false);

    // GarrFollower grant - vestigial Shadowlands soulbind path (covenant.ID < 5).
    // Only fire for SL-era covenants to avoid grant-storms on misdata; since
    // Phase 10 targets WoW 12.0.5+ which does not run the SL covenant system,
    // we log and skip rather than fabricate a Garrison instance.
    if (reward->GarrFollowerID > 0 && uint32(reward->CovenantID) < 5)
    {
        TC_LOG_DEBUG("server.major-factions",
            "Skipping vestigial SL covenant GarrFollower {} from RenownReward {} (CovenantID {}) - garrison subsystem inactive on Midnight 12.0.5",
            reward->GarrFollowerID, reward->ID, reward->CovenantID);
    }

    // QuestID dispatch (Dream Wardens / 10.2 pattern: rewards may include
    // a quest auto-granted on renown level-up to gate the next story chapter).
    if (reward->QuestID > 0)
    {
        if (Quest const* quest = sObjectMgr->GetQuestTemplate(uint32(reward->QuestID)))
            if (player->CanTakeQuest(quest, false))
                player->AddQuestAndCheckCompletion(quest, nullptr);
    }

    TC_LOG_DEBUG("server.major-factions", "Granted RenownReward {} (Level {}, CovenantID {}) to player {} GUID {}",
        reward->ID, reward->Level, reward->CovenantID, player->GetName(), player->GetGUID().ToString());
}
