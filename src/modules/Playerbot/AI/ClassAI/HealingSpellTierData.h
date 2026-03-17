/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Per-class spell tier mappings for healing efficiency management.
 * Each healer spec registers its spells into efficiency tiers,
 * enabling dynamic mana-based spell gating.
 *
 * Tier System:
 *   VERY_HIGH  - Most efficient spells (always allowed)
 *   HIGH       - Good efficiency (blocked below 30% mana)
 *   MEDIUM     - Moderate efficiency (blocked below 50% mana)
 *   LOW        - Expensive spells (blocked below 70% mana)
 *   EMERGENCY  - Always allowed regardless of mana (defensive CDs)
 */

#pragma once

#include "Define.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace Playerbot
{

/// Mana efficiency tier for healing spells
enum class HealingSpellTier : uint8
{
    EMERGENCY   = 0,   // Always allowed (Lay on Hands, Guardian Spirit, etc.)
    VERY_HIGH   = 1,   // Most efficient, always allowed (Heal, Renew, Rejuv)
    HIGH        = 2,   // Good efficiency, blocked at <30% mana
    MEDIUM      = 3,   // Moderate efficiency, blocked at <50% mana
    LOW         = 4,   // Expensive, blocked at <70% mana
};

/// Configuration for a spell's efficiency tier
struct HealingSpellTierEntry
{
    uint32 spellId;
    HealingSpellTier tier;
    std::string spellName;    // For logging/debugging

    HealingSpellTierEntry(uint32 id, HealingSpellTier t, std::string name = "")
        : spellId(id), tier(t), spellName(std::move(name)) {}
};

/// Returns the mana threshold below which a tier is blocked.
/// Below this threshold, spells of this tier cannot be cast.
inline float GetManaThresholdForTier(HealingSpellTier tier)
{
    switch (tier)
    {
        case HealingSpellTier::EMERGENCY:  return 0.0f;   // Never blocked
        case HealingSpellTier::VERY_HIGH:  return 0.0f;   // Never blocked
        case HealingSpellTier::HIGH:       return 30.0f;  // Blocked below 30%
        case HealingSpellTier::MEDIUM:     return 50.0f;  // Blocked below 50%
        case HealingSpellTier::LOW:        return 70.0f;  // Blocked below 70%
        default:                           return 0.0f;
    }
}

/// Returns the tier name as a string for logging
inline const char* GetTierName(HealingSpellTier tier)
{
    switch (tier)
    {
        case HealingSpellTier::EMERGENCY:  return "Emergency";
        case HealingSpellTier::VERY_HIGH:  return "VeryHigh";
        case HealingSpellTier::HIGH:       return "High";
        case HealingSpellTier::MEDIUM:     return "Medium";
        case HealingSpellTier::LOW:        return "Low";
        default:                           return "Unknown";
    }
}

} // namespace Playerbot
