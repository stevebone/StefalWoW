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

#pragma once

#include "Define.h"
#include "SharedDefines.h"
#include "ObjectGuid.h"
#include "../Group/RoleDefinitions.h"
#include <atomic>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>

class Player;

namespace Playerbot
{

/**
 * Talent Manager Statistics
 * Tracks talent application operations
 */
struct TalentStats
{
    uint32 loadoutsLoaded{0};
    uint32 talentsApplied{0};
    uint32 specsSelected{0};
    uint32 dualSpecsEnabled{0};
    uint32 heroTalentsApplied{0};
    uint32 failures{0};
    // Additional tracking fields
    uint32 totalLoadouts{0};
    uint32 specsApplied{0};
    uint32 loadoutsApplied{0};
    uint32 dualSpecsSetup{0};
    uint32 loadoutsWithHeroTalents{0};
    float averageTalentsPerLoadout{0.0f};
    ::std::unordered_map<uint8, uint32> loadoutsPerClass;
};

/**
 * Talent Loadout
 * Represents a complete talent configuration for a spec at a specific level range
 */
struct TalentLoadout
{
    uint8 classId;
    uint8 specId;
    uint32 minLevel;
    uint32 maxLevel;
    ::std::vector<uint32> talentEntries;       // Talent spell IDs to learn
    ::std::vector<uint32> heroTalentEntries;   // Hero talents (71+)
    ::std::string description;

    bool IsValidForLevel(uint32 level) const
    {
        return level >= minLevel && level <= maxLevel;
    }

    bool HasHeroTalents() const
    {
        return !heroTalentEntries.empty();
    }

    uint32 GetTalentCount() const
    {
        return static_cast<uint32>(talentEntries.size() + heroTalentEntries.size());
    }
};

/**
 * Specialization Choice Result
 * Returned when selecting spec for a bot
 */
struct SpecChoice
{
    uint8 specId;
    ::std::string specName;
    GroupRole primaryRole;
    float confidence;  // 0.0-1.0, how confident the selection is

    SpecChoice() : specId(0), primaryRole(GroupRole::NONE), confidence(0.0f) {}
    SpecChoice(uint8 spec, ::std::string name, GroupRole role, float conf)
        : specId(spec), specName(::std::move(name)), primaryRole(role), confidence(conf) {}
};

/**
 * Bot Talent Manager - Automated Talent System for World Population
 *
 * Purpose: Apply talents and specializations to bots during instant level-up
 *
 * Features:
 * - Specialization selection (intelligent role distribution)
 * - Talent loadout application (database-driven)
 * - Dual-spec support (WoW 12.0 feature, unlocks at level 10)
 * - Hero talent support (levels 71-80)
 * - TrinityCore API integration (InitTalentForLevel, LearnTalent, etc.)
 * - Immutable loadout cache (lock-free reads)
 *
 * Integration:
 * - Uses RoleDefinitions for spec metadata (no duplication)
 * - Uses TrinityCore's native talent API
 * - Compatible with ThreadPool worker threads
 *
 * Thread Safety:
 * - Immutable loadout cache after LoadLoadouts()
 * - Lock-free concurrent reads
 * - Atomic initialization flag
 *
 * Performance:
 * - Loadout cache build: <1 second
 * - Spec selection: <0.1ms per bot
 * - Talent application: <1ms per bot (TrinityCore API calls)
 *
 * Usage Workflow (Two-Phase Bot Creation):
 * 1. Worker Thread: SelectSpecialization() - Choose spec based on distribution
 * 2. Worker Thread: GetTalentLoadout() - Retrieve talent list from cache
 * 3. Main Thread: ApplySpecialization() - TrinityCore Player API
 * 4. Main Thread: ApplyTalentLoadout() - TrinityCore Player API
 * 5. Main Thread: (If dual-spec) ActivateSpecialization(spec2) + ApplyTalentLoadout()
 * 6. Main Thread: ActivateSpecialization(spec1) - Return to primary spec
 */
class TC_GAME_API BotTalentManager final 
{
public:
    static BotTalentManager* instance();

    // ====================================================================
    // INITIALIZATION (Called once at server startup)
    // ====================================================================

    /**
     * Load talent loadouts from database
     * MUST be called before any talent operations
     * Single-threaded execution required
     */
    bool LoadLoadouts();

    /**
     * Reload loadouts (for hot-reload during development)
     */
    void ReloadLoadouts();

    /**
     * Check if loadouts are ready
     */
    bool IsReady() const
    {
        return _initialized.load(::std::memory_order_acquire);
    }

    // ====================================================================
    // SPECIALIZATION SELECTION (Thread-safe, for worker threads)
    // ====================================================================

    /**
     * Select primary specialization for bot
     * Thread-safe, can be called from worker threads
     *
     * Uses intelligent distribution:
     * - Hybrid classes: balanced between specs
     * - Pure DPS classes: prefer popular specs
     * - Tanks/Healers: boost selection for role balance
     *
     * @param cls       Player class
     * @param faction   TEAM_ALLIANCE or TEAM_HORDE
     * @param level     Bot level (affects availability)
     * @return          SpecChoice with selected spec
     */
    SpecChoice SelectSpecialization(uint8 cls, TeamId faction, uint32 level);

    /**
     * Select secondary specialization for dual-spec
     * Ensures different from primary spec
     * Prioritizes complementary roles (DPS→Tank, DPS→Healer, etc.)
     *
     * @param cls       Player class
     * @param faction   TEAM_ALLIANCE or TEAM_HORDE
     * @param level     Bot level
     * @param primarySpec Already selected primary spec
     * @return          SpecChoice for secondary spec
     */
    SpecChoice SelectSecondarySpecialization(uint8 cls, TeamId faction, uint32 level, uint8 primarySpec);

    /**
     * Get all available specs for a class
     */
    ::std::vector<uint8> GetAvailableSpecs(uint8 cls) const;

    // ====================================================================
    // TALENT LOADOUT QUERIES (Thread-safe, lock-free cache access)
    // ====================================================================

    /**
     * Get talent loadout for spec and level
     * Thread-safe, returns cached loadout
     *
     * @param cls       Player class
     * @param specId    Specialization ID
     * @param level     Bot level
     * @return          Pointer to loadout, or nullptr if not found
     */
    TalentLoadout const* GetTalentLoadout(uint8 cls, uint8 specId, uint32 level) const;

    /**
     * Get all loadouts for a class/spec combination
     * Useful for debugging and validation
     */
    ::std::vector<TalentLoadout const*> GetAllLoadouts(uint8 cls, uint8 specId) const;

    // ====================================================================
    // TALENT APPLICATION (MAIN THREAD ONLY - Player API)
    // ====================================================================

    /**
     * Apply specialization to bot
     * MUST be called from main thread (Player API)
     *
     * Workflow:
     * 1. Set active spec (Player::SetSpecialization)
     * 2. Learn spec spells (Player::LearnSpecializationSpells)
     *
     * NOTE: Call BEFORE GiveLevel() for proper spell learning
     *
     * @param bot       Bot player object
     * @param specId    Specialization to apply
     * @return          True if successful
     */
    bool ApplySpecialization(Player* bot, uint8 specId);

    /**
     * Apply talent loadout to bot
     * MUST be called from main thread (Player API)
     *
     * Workflow:
     * 1. Get loadout from cache
     * 2. Learn each talent (Player::LearnTalent or AddTalent)
     * 3. Learn hero talents if level 71+
     *
     * NOTE: Call AFTER GiveLevel() and InitTalentForLevel()
     *
     * @param bot       Bot player object
     * @param specId    Specialization ID
     * @param level     Bot level
     * @return          True if successful
     */
    bool ApplyTalentLoadout(Player* bot, uint8 specId, uint32 level);

    /**
     * Activate specialization (switch active spec)
     * Used for dual-spec setup
     *
     * @param bot       Bot player object
     * @param specIndex Spec index (0 = primary, 1 = secondary)
     * @return          True if successful
     */
    bool ActivateSpecialization(Player* bot, uint8 specIndex);

    /**
     * Complete workflow: Apply spec + talents in one call
     * MUST be called from main thread
     *
     * @param bot       Bot player object
     * @param specId    Specialization ID
     * @param level     Bot level
     * @return          True if successful
     */
    bool SetupBotTalents(Player* bot, uint8 specId, uint32 level);

    // ====================================================================
    // DUAL-SPEC SUPPORT (WoW 12.0 Feature)
    // ====================================================================

    /**
     * Check if level supports dual-spec
     * WoW 12.0: Dual-spec unlocks at level 10
     */
    bool SupportsDualSpec(uint32 level) const
    {
        return level >= 10;
    }

    /**
     * Enable dual-spec for bot
     * MUST be called from main thread
     *
     * @param bot       Bot player object
     * @return          True if successful
     */
    bool EnableDualSpec(Player* bot);

    /**
     * Setup dual-spec with both talent loadouts
     * Complete workflow for dual-spec bots
     *
     * @param bot       Bot player object
     * @param spec1     Primary specialization
     * @param spec2     Secondary specialization
     * @param level     Bot level
     * @return          True if successful
     */
    bool SetupDualSpec(Player* bot, uint8 spec1, uint8 spec2, uint32 level);

    // ====================================================================
    // HERO TALENTS (WoW 12.0 Feature, Levels 71-80)
    // ====================================================================

    /**
     * Check if level supports hero talents
     * WoW 12.0: Hero talents unlock at level 71
     */
    bool SupportsHeroTalents(uint32 level) const
    {
        return level >= 71;
    }

    /**
     * Apply hero talents for spec
     * Called automatically by ApplyTalentLoadout() if level >= 71
     *
     * @param bot       Bot player object
     * @param specId    Specialization ID
     * @param level     Bot level (71-80)
     * @return          True if successful
     */
    bool ApplyHeroTalents(Player* bot, uint8 specId, uint32 level);

    // ====================================================================
    // STATISTICS & DEBUGGING
    // ====================================================================

    TalentStats GetStats() const { return _stats; }
    void PrintLoadoutReport() const;
    ::std::string GetLoadoutSummary() const;

private:
    BotTalentManager() = default;
    ~BotTalentManager() = default;
    BotTalentManager(BotTalentManager const&) = delete;
    BotTalentManager& operator=(BotTalentManager const&) = delete;

    // ====================================================================
    // LOADOUT MANAGEMENT
    // ====================================================================

    void LoadLoadoutsFromDatabase();
    void PopulateEmptyLoadoutsFromDB2();  // Auto-populate empty entries from Talent.db2
    void BuildDefaultLoadouts();  // Fallback if database is empty
    void ValidateLoadouts();
    void DumpTalentDatabaseSQL() const;  // Debug: Generate SQL to populate database

    // ====================================================================
    // SPECIALIZATION SELECTION HELPERS
    // ====================================================================

    uint8 SelectByDistribution(uint8 cls, ::std::vector<uint8> const& availableSpecs) const;
    uint8 SelectComplementarySpec(uint8 cls, uint8 primarySpec) const;
    float GetSpecPopularity(uint8 cls, uint8 specId) const;

    // ====================================================================
    // TALENT APPLICATION HELPERS (TrinityCore API)
    // ====================================================================

    bool LearnTalent(Player* bot, uint32 talentEntry);
    bool LearnHeroTalent(Player* bot, uint32 heroTalentEntry);
    void LogTalentApplication(Player* bot, uint8 specId, uint32 talentCount);
    uint32 CalculateTalentPointsForLevel(uint32 level) const;

    // ====================================================================
    // DATA STORAGE
    // ====================================================================

    // Loadout cache: [cls][spec][level_bracket] -> TalentLoadout
    // Key encoding: ((cls << 16) | (spec << 8) | (level/10))
    ::std::unordered_map<uint32, TalentLoadout> _loadoutCache;

    // Quick lookup: class -> available specs
    ::std::unordered_map<uint8, ::std::vector<uint8>> _classSpecs;

    // Statistics
    TalentStats _stats;

    // Initialization flag
    ::std::atomic<bool> _initialized{false};

    // Configuration (loaded from playerbots.conf)
    bool _enabled = true;                // Playerbot.TalentManager.Enable
    bool _useOptimalBuilds = true;       // Playerbot.TalentManager.UseOptimalBuilds
    bool _randomizeMinor = true;         // Playerbot.TalentManager.RandomizeMinor
    bool _adaptToContent = true;         // Playerbot.TalentManager.AdaptToContent
    uint32 _respecFrequencyHours = 24;   // Playerbot.TalentManager.RespecFrequency
    bool _useHeroTalents = true;         // Playerbot.TalentManager.UseHeroTalents

    // ====================================================================
    // HELPER FUNCTIONS
    // ====================================================================

    uint32 MakeLoadoutKey(uint8 cls, uint8 spec, uint32 level) const
    {
        uint32 levelBracket = level / 10;  // Group by 10 levels (0, 1, 2, ... 8)
        return (static_cast<uint32>(cls) << 16) |
               (static_cast<uint32>(spec) << 8) |
               levelBracket;
    }

    uint32 FindBestLoadout(uint8 cls, uint8 specId, uint32 level) const;
};

} // namespace Playerbot

#define sBotTalentManager Playerbot::BotTalentManager::instance()
