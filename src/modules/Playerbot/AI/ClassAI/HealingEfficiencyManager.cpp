/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Healing Efficiency Manager implementation.
 */

#include "HealingEfficiencyManager.h"
#include "Player.h"
#include "Unit.h"
#include "Log.h"

namespace Playerbot
{

HealingEfficiencyManager::HealingEfficiencyManager(Player* bot)
    : _bot(bot)
{
}

void HealingEfficiencyManager::RegisterSpell(uint32 spellId, HealingSpellTier tier, const std::string& name)
{
    _spellTiers.emplace(spellId, HealingSpellTierEntry(spellId, tier, name));
}

void HealingEfficiencyManager::RegisterSpells(HealingSpellTier tier, std::initializer_list<uint32> spellIds)
{
    for (uint32 id : spellIds)
        _spellTiers.emplace(id, HealingSpellTierEntry(id, tier));
}

bool HealingEfficiencyManager::IsSpellAllowedAtCurrentMana(uint32 spellId, bool tankTarget) const
{
    float manaPercent = GetCurrentManaPercent();
    return IsSpellAllowedAtMana(spellId, manaPercent, tankTarget);
}

bool HealingEfficiencyManager::IsSpellAllowedAtMana(uint32 spellId, float manaPercent, bool tankTarget) const
{
    auto it = _spellTiers.find(spellId);
    if (it == _spellTiers.end())
    {
        // Unregistered spells are always allowed (conservative approach)
        return true;
    }

    float threshold = GetEffectiveThreshold(it->second.tier, tankTarget);
    return manaPercent >= threshold;
}

HealingSpellTier HealingEfficiencyManager::GetSpellTier(uint32 spellId) const
{
    auto it = _spellTiers.find(spellId);
    if (it != _spellTiers.end())
        return it->second.tier;
    return HealingSpellTier::VERY_HIGH;  // Default: always allowed
}

float HealingEfficiencyManager::GetEffectiveThreshold(HealingSpellTier tier, bool tankTarget)
{
    float baseThreshold = GetManaThresholdForTier(tier);

    if (tankTarget && baseThreshold > 0.0f)
    {
        // Subtract 20% tolerance for tank targets
        // e.g., LOW tier normally blocked at <70%, for tanks blocked at <50%
        baseThreshold = std::max(0.0f, baseThreshold - TANK_THRESHOLD_BONUS);
    }

    return baseThreshold;
}

float HealingEfficiencyManager::GetCurrentManaPercent() const
{
    if (!_bot || !_bot->IsInWorld())
        return 100.0f;

    uint32 maxMana = _bot->GetMaxPower(POWER_MANA);
    if (maxMana == 0)
        return 100.0f;

    return static_cast<float>(_bot->GetPower(POWER_MANA)) / static_cast<float>(maxMana) * 100.0f;
}

HealingSpellTier HealingEfficiencyManager::GetMaxAllowedTier(bool tankTarget) const
{
    float manaPercent = GetCurrentManaPercent();

    // Check from most restrictive tier down
    if (manaPercent >= GetEffectiveThreshold(HealingSpellTier::LOW, tankTarget))
        return HealingSpellTier::LOW;  // All tiers allowed
    if (manaPercent >= GetEffectiveThreshold(HealingSpellTier::MEDIUM, tankTarget))
        return HealingSpellTier::MEDIUM;
    if (manaPercent >= GetEffectiveThreshold(HealingSpellTier::HIGH, tankTarget))
        return HealingSpellTier::HIGH;
    return HealingSpellTier::VERY_HIGH;  // Only most efficient + emergency
}

std::vector<uint32> HealingEfficiencyManager::GetSpellsForTier(HealingSpellTier tier) const
{
    std::vector<uint32> result;
    for (const auto& [id, entry] : _spellTiers)
    {
        if (entry.tier == tier)
            result.push_back(id);
    }
    return result;
}

} // namespace Playerbot
