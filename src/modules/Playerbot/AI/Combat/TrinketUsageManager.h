/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * ON-USE TRINKET AUTOMATION MANAGER
 *
 * Automatically uses equipped on-use trinkets aligned with burst windows
 * and defensive needs. Scans trinket equipment slots for items with
 * ITEM_SPELLTRIGGER_ON_USE effects, classifies them, and triggers them
 * at optimal times during combat.
 *
 * Architecture:
 *   - Per-bot component, called during combat update
 *   - Scans EQUIPMENT_SLOT_TRINKET1/TRINKET2 for on-use effects
 *   - Classifies effects: offensive (damage/haste/crit buffs),
 *     defensive (absorb/heal/DR), utility (movement/CC-break)
 *   - Aligns offensive trinkets with burst phases (opener, on-CD)
 *   - Uses defensive trinkets reactively (health < threshold)
 *   - Respects cooldowns via SpellHistory
 *   - Re-scans when equipment changes
 *
 * Integration Points:
 *   - CombatPhaseDetector: GetPhase()/GetGuidance() for burst timing
 *   - PreBurstResourcePooling: IsPoolingActive() for pre-burst delay
 *   - SpellHistory: HasCooldown() for cooldown tracking
 *   - Player::CastSpell with CastSpellExtraArgs(item) for casting
 *
 * Trinket Classification Heuristics:
 *   Offensive: Spell applies a buff with damage/haste/crit/mastery/vers
 *              aura effect types (SPELL_AURA_MOD_DAMAGE_PERCENT_DONE,
 *              SPELL_AURA_HASTE_SPELLS, etc.)
 *   Defensive: Spell applies absorb, damage reduction, or healing
 *              (SPELL_AURA_SCHOOL_ABSORB, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN)
 *   Utility:   Anything else (movement, CC-break, summoning)
 */

#pragma once

#include "Define.h"
#include "DBCEnums.h"
#include <array>
#include <string>
#include <vector>

class Item;
class Player;
class Unit;
class SpellInfo;

namespace Playerbot
{

// ============================================================================
// TRINKET EFFECT CLASSIFICATION
// ============================================================================

/// What kind of effect does this trinket's on-use provide?
enum class TrinketEffectType : uint8
{
    UNKNOWN     = 0,
    OFFENSIVE   = 1,    // Damage, haste, crit, mastery, versatility buffs
    DEFENSIVE   = 2,    // Absorb shields, damage reduction, healing
    UTILITY     = 3,    // Movement, CC-break, miscellaneous
    PVP_TRINKET = 4     // PvP CC-break trinket (special handling)
};

/// When should this trinket be used?
enum class TrinketUsagePolicy : uint8
{
    ON_BURST        = 0,    // Use during burst/opener phase
    ON_COOLDOWN     = 1,    // Use whenever available (maximize uptime)
    ON_LOW_HEALTH   = 2,    // Use when health drops below threshold
    ON_CC           = 3,    // Use when crowd-controlled (PvP trinket)
    MANUAL          = 4     // Don't auto-use (reserved for special trinkets)
};

// ============================================================================
// TRINKET INFO
// ============================================================================

/// Cached information about an equipped on-use trinket
struct OnUseTrinketInfo
{
    uint8 equipSlot = 0;                // EQUIPMENT_SLOT_TRINKET1 or _TRINKET2
    uint32 itemEntry = 0;               // Item template entry
    int32 onUseSpellId = 0;             // The spell triggered by on-use
    TrinketEffectType effectType = TrinketEffectType::UNKNOWN;
    TrinketUsagePolicy usagePolicy = TrinketUsagePolicy::ON_COOLDOWN;
    int32 cooldownMs = 0;               // Cooldown from ItemEffectEntry
    ::std::string itemName;             // For logging

    bool IsValid() const { return itemEntry > 0 && onUseSpellId > 0; }
};

// ============================================================================
// TRINKET USAGE STATISTICS
// ============================================================================

struct TrinketUsageStats
{
    uint32 totalUses = 0;               // Total trinket activations
    uint32 burstAlignedUses = 0;        // Uses aligned with burst windows
    uint32 defensiveUses = 0;           // Defensive trinket activations
    uint32 pvpTrinketUses = 0;          // PvP trinket CC-break uses
    uint32 cooldownWastes = 0;          // Times we wanted to use but was on CD
    uint32 rescanCount = 0;             // Equipment re-scans
};

// ============================================================================
// TRINKET USAGE MANAGER
// ============================================================================

class TC_GAME_API TrinketUsageManager
{
public:
    explicit TrinketUsageManager(Player* bot);
    ~TrinketUsageManager() = default;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /// Initialize and scan equipped trinkets
    void Initialize();

    /// Update trinket usage decisions. Call once per combat update.
    /// @param diff Time since last update in milliseconds
    void Update(uint32 diff);

    /// Re-scan trinkets (call when equipment changes)
    void OnEquipmentChanged();

    /// Reset on combat start
    void OnCombatStart();

    /// Reset on combat end
    void OnCombatEnd();

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Get info about trinket in slot 1
    OnUseTrinketInfo const& GetTrinket1() const { return _trinkets[0]; }

    /// Get info about trinket in slot 2
    OnUseTrinketInfo const& GetTrinket2() const { return _trinkets[1]; }

    /// Does the bot have any on-use trinkets equipped?
    bool HasOnUseTrinkets() const;

    /// Is a specific trinket ready to use? (not on cooldown)
    bool IsTrinketReady(uint8 slotIndex) const;

    /// Get usage statistics
    TrinketUsageStats const& GetStats() const { return _stats; }

    /// Get a text summary for debugging
    ::std::string GetDebugSummary() const;

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /// Scan one trinket slot and populate info
    void ScanTrinketSlot(uint8 slotIndex, uint8 equipmentSlot);

    /// Classify a spell's effect type based on its aura types
    TrinketEffectType ClassifySpellEffect(SpellInfo const* spellInfo) const;

    /// Determine when this trinket should be used based on its type
    TrinketUsagePolicy DetermineUsagePolicy(TrinketEffectType effectType) const;

    /// Check if conditions are met to use an offensive trinket
    bool ShouldUseOffensiveTrinket() const;

    /// Check if conditions are met to use a defensive trinket
    bool ShouldUseDefensiveTrinket() const;

    /// Check if conditions are met to use PvP trinket (CC break)
    bool ShouldUsePvPTrinket() const;

    /// Actually cast the trinket's on-use spell
    bool ActivateTrinket(uint8 slotIndex);

    /// Get the bot's current target (for offensive trinket alignment)
    Unit* GetTarget() const;

    // ========================================================================
    // STATE
    // ========================================================================

    Player* _bot;
    bool _initialized = false;
    bool _inCombat = false;

    /// Cached trinket info for both slots [0] = trinket1, [1] = trinket2
    ::std::array<OnUseTrinketInfo, 2> _trinkets;

    /// Statistics
    TrinketUsageStats _stats;

    /// Update throttle
    uint32 _updateTimer = 0;
    static constexpr uint32 UPDATE_INTERVAL_MS = 500;   // Check every 500ms

    /// Defensive health threshold (use defensive trinket below this %)
    float _defensiveHealthThreshold = 35.0f;

    /// Whether trinkets were used this combat (to avoid spamming logs)
    ::std::array<bool, 2> _usedThisCombat = {false, false};

    /// Equipment version tracking (detect changes without events)
    uint32 _lastEquipChecksum = 0;
};

} // namespace Playerbot
