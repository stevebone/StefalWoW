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

#ifndef TRINITY_MAJOR_FACTION_MGR_H
#define TRINITY_MAJOR_FACTION_MGR_H

#include "Define.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct CovenantEntry;
struct FactionEntry;
struct ParagonReputationEntry;
struct RenownRewardsEntry;
class Player;

namespace MajorFactions
{
    // Bitmask values for RenownRewards.Flags. Source: client UI Lua
    // (RenownConstantsDocumentation.lua, Enum.RenownRewardsFlags).
    enum class RenownRewardFlags : uint32
    {
        None          = 0x0,
        Milestone     = 0x1,
        Capstone      = 0x2,
        Hidden        = 0x4,
        AccountUnlock = 0x8,
    };

    // Client enum mirror: Enum.RenownRewardDisplayType (RenownConstantsDocumentation.lua).
    enum class RenownRewardDisplayType : int32
    {
        None              = 0,
        Item              = 1,
        Spell             = 2,
        Mount             = 3,
        Transmog          = 4,
        TransmogSet       = 5,
        TransmogIllusion  = 6,
        Title             = 7,
        GarrFollower      = 8,
        Currency          = 9,
    };

    // Per-faction server-side configuration loaded from world DB
    // `major_faction_config`. Drives the post-12.0 Journey UI fields the
    // client builds from DB2 metadata + this overlay.
    //
    // NOTE: textureKit is intentionally NOT stored here. It is resolved at
    // read time from the faction's renown Campaign DB2 row via the chain
    // Faction -> RenownCampaignID -> Campaign.UiTextureKitID -> UiTextureKit.KitPrefix.
    // See MajorFactionMgr::GetTextureKitPrefix.
    struct MajorFactionConfig
    {
        bool   HiddenFromExpansionPage = false;
        bool   DisplayAsJourney        = false;
        bool   UseJourneyRewardTrack   = false;
        bool   UseJourneyUnlockToast   = false;
        int32  UiPriority              = 0;
        uint32 IntroQuestID            = 0;   // canonical/Horde intro quest
        uint32 IntroQuestIDAlliance    = 0;   // 0 = use IntroQuestID for both teams
        uint32 PlayerCompanionID       = 0;
        uint32 RenownCampaignID        = 0;   // Campaign.db2 ID for the faction's main renown campaign
    };
}

class TC_GAME_API MajorFactionMgr
{
    public:
        // Reputation required to gain one renown level. Constant across all
        // major factions on retail at build 12.0.5.67186; verified against
        // FactionEntry::ReputationMax[] for every major and the existing
        // ReputationMgr::GetRenownLevelThreshold() implementation.
        static constexpr int32 ReputationPerRenownLevel = 2500;

        static MajorFactionMgr* instance();

        MajorFactionMgr() = default;
        MajorFactionMgr(MajorFactionMgr const&) = delete;
        MajorFactionMgr(MajorFactionMgr&&) = delete;
        MajorFactionMgr& operator=(MajorFactionMgr const&) = delete;
        MajorFactionMgr& operator=(MajorFactionMgr&&) = delete;

        // Called once after sDB2Manager.LoadStores() during World init.
        void Load();

        // Called after world DB init to load major_faction_config.
        void LoadWorldData();

        // -- Identity -------------------------------------------------------

        // A faction is a Major Faction iff Faction.ParagonFactionID != 0 AND
        // Faction.RenownCurrencyID != 0 (mirrors the client rule). Robust
        // against the Flags-bit-0x4 inconsistency (e.g. 2616 Keg Leg
        // Thrasher has Flags=0 yet is a Plunderstorm renown track).
        bool IsMajorFaction(uint32 factionId) const;
        bool IsMajorFaction(FactionEntry const* factionEntry) const;

        std::vector<uint32> const& GetMajorFactionIDs() const { return _majorFactionIDs; }
        std::vector<uint32> GetMajorFactionIDsForExpansion(uint8 expansion) const;

        // -- Renown ---------------------------------------------------------

        // Max renown level for a faction. Computed as the maximum
        // RenownRewards.Level value among rows joined via Covenant.FactionID
        // == factionId. Falls back to CurrencyTypes.MaxQty of the faction's
        // RenownCurrencyID if RenownRewards has no rows for the faction.
        uint32 GetMaxRenownLevel(uint32 factionId) const;

        uint32 GetRenownCurrencyID(uint32 factionId) const;

        // Reputation gained per renown level. Currently a constant
        // (ReputationPerRenownLevel) but exposed as a per-faction lookup so
        // future per-faction tuning hotfixes can override without touching
        // ReputationMgr.
        uint32 GetReputationPerRenownLevel(uint32 factionId) const;

        // -- Rewards --------------------------------------------------------

        // Rewards unlocked at exactly the given renown level for the faction,
        // sorted by UiOrder ascending. Empty if the level has no rewards.
        std::vector<RenownRewardsEntry const*> GetRenownRewardsForLevel(uint32 factionId, uint32 renownLevel) const;

        // Rewards unlocked strictly above `fromLevel` up to and including
        // `toLevel`. Used by the level-cross dispatch in
        // MajorFactionMgr::GrantRenownLevelRewards to handle multi-level
        // jumps from catch-up.
        std::vector<RenownRewardsEntry const*> GetRenownRewardsBetween(uint32 factionId, uint32 fromLevel, uint32 toLevel) const;

        // All RenownRewards rows for the faction, sorted by (Level asc, UiOrder asc).
        std::vector<RenownRewardsEntry const*> const* GetAllRenownRewards(uint32 factionId) const;

        // -- Paragon --------------------------------------------------------

        ParagonReputationEntry const* GetParagonForMajorFaction(uint32 factionId) const;

        // -- Covenant / metadata --------------------------------------------

        CovenantEntry const* GetCovenantForFaction(uint32 factionId) const;
        uint32 GetCovenantIDForFaction(uint32 factionId) const;
        uint32 GetFactionForCovenant(uint32 covenantId) const;

        // -- Per-faction config ---------------------------------------------

        MajorFactions::MajorFactionConfig const* GetConfig(uint32 factionId) const;

        bool IsHiddenFromExpansionPage(uint32 factionId) const;
        bool ShouldDisplayAsJourney(uint32 factionId) const;
        bool ShouldUseJourneyRewardTrack(uint32 factionId) const;
        bool ShouldUseJourneyUnlockToast(uint32 factionId) const;
        uint32 GetPlayerCompanionID(uint32 factionId) const;
        uint32 GetIntroQuestID(uint32 factionId) const;
        // Team-aware variant: returns the Alliance variant for Alliance
        // players if the row provides one, otherwise falls back to the
        // canonical IntroQuestID. Pass nullptr to get the canonical value.
        uint32 GetIntroQuestID(Player const* player, uint32 factionId) const;
        uint32 GetRenownCampaignID(uint32 factionId) const;

        // Walks faction -> renown Campaign -> Campaign.UiTextureKitID ->
        // UiTextureKit.KitPrefix and returns the resolved KitPrefix string
        // (e.g. "MajorFaction-DragonscaleExpedition"). Empty string if any
        // step in the chain fails to resolve.
        std::string_view GetTextureKitPrefix(uint32 factionId) const;

        // -- Player view ----------------------------------------------------

        // True iff the player has the faction's standing >= visible threshold
        // (or has completed the unlock quest if specified in config). Mirror
        // of the MajorFactionData.isUnlocked field.
        bool IsUnlockedForPlayer(Player const* player, uint32 factionId) const;

        // Convenience: current renown level for the player (delegates to
        // ReputationMgr::GetRenownLevel via FactionEntry lookup).
        uint32 GetPlayerRenownLevel(Player const* player, uint32 factionId) const;

        // Reputation earned toward the next renown level (standing modulo
        // ReputationPerRenownLevel). Mirror of
        // MajorFactionData.renownReputationEarned.
        uint32 GetPlayerReputationThisLevel(Player const* player, uint32 factionId) const;

        // -- Reward dispatch (Phase 10C) -----------------------------------

        // Iterate all RenownRewards rows with Level > fromLevel && Level <= toLevel
        // for the given faction and dispatch each reward to the player.
        // Handles multi-level jumps (e.g. catch-up). De-duplicates against
        // ReputationMgr::IsRenownRewardGranted (character-scoped and
        // account-scoped grant tables).
        //
        // If accountSync is true the call is the result of an alt logging
        // in and inheriting renown from a higher-level toon: only
        // character-bound rewards are granted (account-wide rewards were
        // already granted on the original toon and live in
        // warband_renown_rewards_granted).
        void GrantRenownLevelRewards(Player* player, uint32 factionId, uint32 fromLevel, uint32 toLevel, bool accountSync = false) const;

        // Dispatch a single RenownRewards row. Used both by
        // GrantRenownLevelRewards and by admin/test command paths.
        void GrantSingleRenownReward(Player* player, RenownRewardsEntry const* reward) const;

    private:
        void IndexFactions();
        void IndexCovenants();
        void IndexRenownRewards();
        void IndexParagons();

        std::vector<uint32>                                                     _majorFactionIDs;
        std::unordered_set<uint32>                                              _majorFactionSet;
        std::unordered_map<uint32 /*factionId*/, uint32 /*covenantId*/>         _factionToCovenant;
        std::unordered_map<uint32 /*covenantId*/, uint32 /*factionId*/>         _covenantToFaction;
        std::unordered_map<uint32 /*factionId*/, CovenantEntry const*>          _covenantByFaction;
        std::unordered_map<uint32 /*factionId*/, std::vector<RenownRewardsEntry const*>> _renownRewardsByFaction;
        std::unordered_map<uint32 /*factionId*/, uint32>                        _maxRenownLevelByFaction;
        std::unordered_map<uint32 /*factionId*/, ParagonReputationEntry const*> _paragonByFaction;
        std::unordered_map<uint32 /*factionId*/, MajorFactions::MajorFactionConfig> _configByFaction;
};

#define sMajorFactionMgr MajorFactionMgr::instance()

#endif // TRINITY_MAJOR_FACTION_MGR_H
