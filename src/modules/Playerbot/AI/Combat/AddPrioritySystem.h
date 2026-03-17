/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * ADD PRIORITY SYSTEM
 *
 * Provides context-aware add classification and prioritization for bot
 * target selection. Automatically identifies add types (healer, explosive,
 * fixate, enrage, shielding) from creature template data and active
 * spells/auras, then generates priority scores adjusted for the bot's role
 * and current encounter context (dungeon vs raid, M+ affixes, group comp).
 *
 * Architecture:
 *   - Per-bot component, created and owned by BotAI or CombatBehaviorIntegration
 *   - Scans nearby hostile creatures each update and classifies them
 *   - Provides priority overrides consumable by TargetSelector and TargetManager
 *   - Works independently of RaidCoordinator (usable in dungeons and open world)
 *   - Thread-safe (called from bot AI update thread only)
 *
 * Integration Points:
 *   - TargetSelector: Call GetAddPriorityScore() in CalculateTargetScore()
 *   - TargetManager: Call GetHighestPriorityAdd() for smart switching
 *   - AoEDecisionManager: Call HasExplosiveAdds() for AoE urgency
 *   - DefensiveBehaviorManager: Call HasFixateOnBot() for kiting decisions
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>
#include <string>

class Player;
class Unit;
class Creature;
class SpellInfo;
struct CreatureTemplate;

namespace Playerbot
{

// ============================================================================
// ADD TYPE CLASSIFICATION
// ============================================================================

/// Classification of an add's combat role/behavior
enum class AddType : uint8
{
    UNKNOWN         = 0,    // Unclassified mob
    HEALER          = 1,    // Heals other enemies (kill first to prevent healing)
    CASTER_DPS      = 2,    // Ranged damage dealer (interrupt / focus)
    MELEE_DPS       = 3,    // Standard melee mob
    TANK_MOB        = 4,    // High HP / high armor absorber
    EXPLOSIVE       = 5,    // Must be killed quickly or explodes (M+ affix, encounter mechanic)
    FIXATE          = 6,    // Fixates a player, ignores threat (kite or burn)
    ENRAGED         = 7,    // Currently enraged (soothe or burst)
    SHIELDING       = 8,    // Shields/buffs other enemies (interrupt shield or kill)
    SUMMONER        = 9,    // Summons more adds (kill to stop reinforcements)
    BERSERKER       = 10,   // Stacking damage buff, gets more dangerous over time
    INTERRUPTIBLE   = 11,   // Currently casting an interruptible high-damage spell
    CROWD_CONTROLLED = 12   // Currently CC'd, do not break
};

/// How urgently this add should be dealt with
enum class AddUrgency : uint8
{
    NONE            = 0,    // Not a concern
    LOW             = 1,    // Background target, handle when convenient
    MODERATE        = 2,    // Should be dealt with soon
    HIGH            = 3,    // Should be current target for some DPS
    CRITICAL        = 4,    // Must be killed/handled immediately by all DPS
    EMERGENCY       = 5     // Wipe-prevention level, drop everything
};

// ============================================================================
// ADD CLASSIFICATION STRUCTURES
// ============================================================================

/// Full classification of a single enemy add
struct AddClassification
{
    ObjectGuid guid;
    uint32 creatureEntry = 0;       // Creature template entry
    AddType primaryType = AddType::UNKNOWN;
    AddType secondaryType = AddType::UNKNOWN; // Some adds are multi-role
    AddUrgency urgency = AddUrgency::NONE;
    float priorityScore = 0.0f;     // Composite score (higher = attack first)
    float healthPercent = 100.0f;
    float distance = 0.0f;
    bool isElite = false;
    bool isBoss = false;
    bool isTargetingBot = false;     // Fixated / threat on this bot
    bool isTargetingHealer = false;  // Targeting a healer in our group
    bool isTargetingTank = false;    // On a tank (expected)
    bool isCrowdControlled = false;
    bool isInterruptible = false;    // Currently casting something interruptible
    uint32 castingSpellId = 0;       // What they're casting (0 = nothing)
    uint32 lastClassifiedMs = 0;     // When last classified
    std::string reason;              // Human-readable priority reason

    bool operator>(AddClassification const& other) const
    {
        return priorityScore > other.priorityScore;
    }
};

/// Summary of all classified adds around the bot
struct AddSituationSummary
{
    uint32 totalAdds = 0;
    uint32 healerAdds = 0;
    uint32 explosiveAdds = 0;
    uint32 fixateAdds = 0;
    uint32 enragedAdds = 0;
    uint32 shieldingAdds = 0;
    uint32 summonerAdds = 0;
    uint32 interruptibleAdds = 0;
    uint32 crowdControlledAdds = 0;

    ObjectGuid highestPriorityGuid;     // Best target to switch to
    float highestPriorityScore = 0.0f;
    AddType mostDangerousType = AddType::UNKNOWN;
    AddUrgency overallUrgency = AddUrgency::NONE;

    bool hasHealerAdd = false;
    bool hasExplosiveAdd = false;
    bool hasFixateOnBot = false;    // Something fixated on THIS bot
    bool hasFixateOnHealer = false; // Something fixated on a healer
    bool hasEnragedAdd = false;
    bool needsImmediateSwitch = false;  // Bot should switch target NOW

    void Reset()
    {
        totalAdds = 0;
        healerAdds = explosiveAdds = fixateAdds = enragedAdds = 0;
        shieldingAdds = summonerAdds = interruptibleAdds = crowdControlledAdds = 0;
        highestPriorityGuid.Clear();
        highestPriorityScore = 0.0f;
        mostDangerousType = AddType::UNKNOWN;
        overallUrgency = AddUrgency::NONE;
        hasHealerAdd = hasExplosiveAdd = hasFixateOnBot = false;
        hasFixateOnHealer = hasEnragedAdd = needsImmediateSwitch = false;
    }
};

// ============================================================================
// ROLE CONTEXT FOR PRIORITY ADJUSTMENT
// ============================================================================

/// Bot's role context that adjusts add priorities
enum class BotRoleContext : uint8
{
    TANK        = 0,    // Tanks: prioritize picking up loose adds, taunt fixates off healers
    MELEE_DPS   = 1,    // Melee DPS: prioritize adds in melee range, cleave targets
    RANGED_DPS  = 2,    // Ranged DPS: prioritize healer adds, explosive adds
    HEALER      = 3     // Healers: rarely switch target, but flag dangerous adds for others
};

/// Encounter context that modifies add urgency
struct EncounterContext
{
    bool isInDungeon = false;
    bool isInRaid = false;
    bool isInMythicPlus = false;
    uint32 mythicPlusLevel = 0;
    bool hasBolsteringAffix = false;    // Don't kill small adds near big ones
    bool hasBurstingAffix = false;      // Don't kill adds too fast in sequence
    bool hasRagingAffix = false;        // Mobs enrage at 30%, execute fast
    bool hasSanguineAffix = false;      // Move mobs away from pools
    bool hasSpitefulAffix = false;      // Ghosts fixate on random player
    bool hasIncorporealAffix = false;   // Must CC incorporeal beings
    bool hasAfflictedAffix = false;     // Must heal/dispel afflicted souls
    uint32 activeBossEncounterId = 0;   // 0 if no boss, affects add urgency
    uint32 groupSize = 5;               // Party or raid size
};

// ============================================================================
// ADD PRIORITY SYSTEM
// ============================================================================

class TC_GAME_API AddPrioritySystem
{
public:
    explicit AddPrioritySystem(Player* bot);
    ~AddPrioritySystem() = default;

    // ========================================================================
    // CORE UPDATE
    // ========================================================================

    /// Update add classifications based on current nearby enemies.
    /// @param diff Time since last update in milliseconds
    void Update(uint32 diff);

    /// Reset all state (e.g., on combat end)
    void Reset();

    // ========================================================================
    // CLASSIFICATION QUERIES
    // ========================================================================

    /// Get the full classification for a specific enemy
    /// @return nullptr if enemy not classified or not nearby
    AddClassification const* GetClassification(ObjectGuid guid) const;

    /// Get the add priority score for a specific enemy (for TargetSelector integration)
    /// @return Score bonus to add to target selection (0.0 if not an interesting add)
    float GetAddPriorityScore(ObjectGuid guid) const;

    /// Get the current add situation summary
    AddSituationSummary const& GetSituation() const { return _situation; }

    /// Get the highest priority add that the bot should switch to
    /// @return GUID of the best add target, or empty if current target is fine
    ObjectGuid GetHighestPriorityAdd() const;

    /// Get all classified adds sorted by priority (highest first)
    std::vector<AddClassification> GetAddsByPriority() const;

    /// Check if the bot should switch from its current target to an add
    /// @param currentTarget GUID of the bot's current target
    /// @return true if an add has significantly higher priority
    bool ShouldSwitchToAdd(ObjectGuid currentTarget) const;

    // ========================================================================
    // SPECIFIC ADD TYPE CHECKS
    // ========================================================================

    /// Check if there are healer adds that need to be killed
    bool HasHealerAdds() const { return _situation.hasHealerAdd; }

    /// Check if there are explosive/time-bomb adds
    bool HasExplosiveAdds() const { return _situation.hasExplosiveAdd; }

    /// Check if something is fixated on this bot
    bool HasFixateOnBot() const { return _situation.hasFixateOnBot; }

    /// Check if something is fixated on a healer in the group
    bool HasFixateOnHealer() const { return _situation.hasFixateOnHealer; }

    /// Check if there are enraged adds that need soothing/killing
    bool HasEnragedAdds() const { return _situation.hasEnragedAdd; }

    /// Get the overall urgency level
    AddUrgency GetOverallUrgency() const { return _situation.overallUrgency; }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /// Set the bot's role context for priority adjustments
    void SetRoleContext(BotRoleContext role) { _roleContext = role; }

    /// Update encounter context (call when entering instance, boss pull, etc.)
    void SetEncounterContext(EncounterContext const& ctx) { _encounterContext = ctx; }

    /// Set how aggressively the system recommends target switches
    /// @param threshold Score difference required to recommend a switch (default 50.0)
    void SetSwitchThreshold(float threshold) { _switchThreshold = threshold; }

    /// Set scan range for detecting adds
    void SetScanRange(float range) { _scanRange = range; }

private:
    // ========================================================================
    // CLASSIFICATION METHODS
    // ========================================================================

    /// Scan for nearby hostile creatures and classify them
    void ScanAndClassifyAdds();

    /// Classify a single creature
    AddClassification ClassifyCreature(Creature* creature) const;

    /// Determine the primary type of a creature from its template and behavior
    AddType DetermineCreatureType(Creature* creature, CreatureTemplate const* tmpl) const;

    /// Check if creature is currently healing (casting heal spells)
    bool IsCreatureHealing(Creature* creature) const;

    /// Check if creature is buffing/shielding allies
    bool IsCreatureShielding(Creature* creature) const;

    /// Check if creature is summoning reinforcements
    bool IsCreatureSummoning(Creature* creature) const;

    /// Check if creature is fixated on a specific player
    bool IsCreatureFixated(Creature* creature) const;

    /// Check if creature has an enrage aura/buff active
    bool IsCreatureEnraged(Creature* creature) const;

    /// Check if this is an explosive-type add (M+ Explosive orb, timed bomb, etc.)
    bool IsExplosiveAdd(Creature* creature, CreatureTemplate const* tmpl) const;

    /// Check if creature has stacking damage buffs (berserker behavior)
    bool IsCreatureBerserking(Creature* creature) const;

    // ========================================================================
    // PRIORITY SCORING
    // ========================================================================

    /// Calculate the composite priority score for a classified add
    float CalculatePriorityScore(AddClassification& classification) const;

    /// Get the base priority for an add type
    float GetBaseTypePriority(AddType type) const;

    /// Apply role-based adjustments to priority
    float ApplyRoleAdjustment(float basePriority, AddClassification const& add) const;

    /// Apply encounter context adjustments (M+ affixes, boss phase, etc.)
    float ApplyEncounterAdjustment(float priority, AddClassification const& add) const;

    /// Apply health-based urgency (low HP adds get bonus to finish them off)
    float ApplyHealthAdjustment(float priority, AddClassification const& add) const;

    /// Apply proximity adjustment (closer adds get slight priority bonus)
    float ApplyDistanceAdjustment(float priority, float distance) const;

    // ========================================================================
    // URGENCY ASSESSMENT
    // ========================================================================

    /// Determine urgency for a classified add
    AddUrgency DetermineUrgency(AddClassification const& add) const;

    /// Update the overall situation summary from all classified adds
    void UpdateSituation();

    // ========================================================================
    // HELPERS
    // ========================================================================

    /// Check if a unit is a healer in the bot's group
    bool IsGroupHealer(Unit* unit) const;

    /// Check if a unit is a tank in the bot's group
    bool IsGroupTank(Unit* unit) const;

    /// Get who the creature is currently targeting
    Unit* GetCreatureTarget(Creature* creature) const;

    /// Detect the bot's role context from its class/spec
    BotRoleContext DetectRoleContext() const;

    /// Update encounter context from current map/instance state
    void UpdateEncounterContext();

    // ========================================================================
    // STATE
    // ========================================================================

    Player* _bot;

    // All classified adds
    std::unordered_map<ObjectGuid, AddClassification> _classifiedAdds;

    // Current situation summary
    AddSituationSummary _situation;

    // Configuration
    BotRoleContext _roleContext = BotRoleContext::MELEE_DPS;
    EncounterContext _encounterContext;
    float _switchThreshold = 50.0f;     // Score difference to recommend switch
    float _scanRange = 40.0f;           // How far to scan for adds

    // Update timers
    uint32 _updateTimer = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 300; // ~3.3 updates/sec

    // Encounter context refresh
    uint32 _contextRefreshTimer = 0;
    static constexpr uint32 CONTEXT_REFRESH_INTERVAL_MS = 5000;

    // Classification history for stability (don't flip-flop types)
    static constexpr uint32 CLASSIFICATION_STALENESS_MS = 2000;

    // Priority score thresholds
    static constexpr float EMERGENCY_THRESHOLD = 200.0f;
    static constexpr float CRITICAL_THRESHOLD = 150.0f;
    static constexpr float HIGH_THRESHOLD = 100.0f;
    static constexpr float MODERATE_THRESHOLD = 50.0f;
};

} // namespace Playerbot
