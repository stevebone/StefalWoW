/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Healing Efficiency Manager: Gates spell selection by current mana percentage.
 * Healers register their spells into efficiency tiers at construction.
 * Before each heal attempt, IsSpellAllowedAtCurrentMana() checks if the
 * spell's tier is permitted at the current mana level.
 *
 * Tank targets get +20% mana threshold tolerance (more willing to cast
 * expensive heals on tanks).
 */

#pragma once

#include "HealingSpellTierData.h"
#include "Define.h"
#include <unordered_map>
#include <vector>

class Player;
class Unit;

namespace Playerbot
{

/// Per-bot healing efficiency tracker.
/// Each healer spec creates one and registers its spells.
class TC_GAME_API HealingEfficiencyManager
{
public:
    explicit HealingEfficiencyManager(Player* bot);
    ~HealingEfficiencyManager() = default;

    /// Register a spell with its efficiency tier.
    void RegisterSpell(uint32 spellId, HealingSpellTier tier, const std::string& name = "");

    /// Register multiple spells at once for a tier.
    void RegisterSpells(HealingSpellTier tier, std::initializer_list<uint32> spellIds);

    /// Check if a spell is allowed at the current mana level.
    /// tankTarget: if true, applies +20% mana threshold tolerance.
    bool IsSpellAllowedAtCurrentMana(uint32 spellId, bool tankTarget = false) const;

    /// Check if a spell is allowed given an explicit mana percentage.
    bool IsSpellAllowedAtMana(uint32 spellId, float manaPercent, bool tankTarget = false) const;

    /// Get the tier for a registered spell (returns VERY_HIGH for unregistered spells).
    HealingSpellTier GetSpellTier(uint32 spellId) const;

    /// Get the current effective mana threshold for a tier.
    /// Adjusts for tank targets and returns the percentage below which
    /// spells of this tier should NOT be cast.
    static float GetEffectiveThreshold(HealingSpellTier tier, bool tankTarget);

    /// Get current mana percentage of the owning bot.
    float GetCurrentManaPercent() const;

    /// Get the highest tier that is currently allowed at the bot's mana level.
    HealingSpellTier GetMaxAllowedTier(bool tankTarget = false) const;

    /// Get all registered spell IDs for a given tier.
    std::vector<uint32> GetSpellsForTier(HealingSpellTier tier) const;

    /// Get total number of registered spells.
    size_t GetRegisteredSpellCount() const { return _spellTiers.size(); }

private:
    Player* _bot;
    std::unordered_map<uint32, HealingSpellTierEntry> _spellTiers;

    static constexpr float TANK_THRESHOLD_BONUS = 20.0f;  // +20% tolerance for tanks
};

} // namespace Playerbot
