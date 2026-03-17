/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Spell Fallback Chain implementation.
 */

#include "SpellFallbackChain.h"
#include "../Combat/TTKEstimator.h"
#include "Player.h"
#include "Unit.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "Map.h"
#include <algorithm>

namespace Playerbot
{

void SpellFallbackChain::SetPrimary(uint32 spellId)
{
    // Insert at front with weight 1.0
    _entries.insert(_entries.begin(), FallbackSpellEntry(spellId, 1.0f));
    _castTimesCached = false;
}

void SpellFallbackChain::AddAlternative(uint32 spellId, float priorityWeight)
{
    _entries.emplace_back(spellId, priorityWeight);
    _castTimesCached = false;
}

void SpellFallbackChain::Clear()
{
    _entries.clear();
    _castTimesCached = false;
}

uint32 SpellFallbackChain::GetPrimarySpell() const
{
    return _entries.empty() ? 0 : _entries.front().spellId;
}

void SpellFallbackChain::CacheCastTimes(Player* bot) const
{
    if (_castTimesCached || !bot || !bot->IsInWorld())
        return;

    Difficulty diff = bot->GetMap()->GetDifficultyID();

    for (auto& entry : _entries)
    {
        if (const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(entry.spellId, diff))
        {
            entry.cachedCastTimeMs = spellInfo->CalcCastTime();
            entry.isInstant = (entry.cachedCastTimeMs == 0);
        }
    }

    _castTimesCached = true;
}

bool SpellFallbackChain::ShouldSkipForTTK(const FallbackSpellEntry& entry, Unit* target,
                                           TTKEstimator* ttkEstimator, Player* /*bot*/) const
{
    if (!ttkEstimator || !target || entry.isInstant)
        return false;

    return ttkEstimator->ShouldSkipLongCast(entry.cachedCastTimeMs, target);
}

uint32 SpellFallbackChain::SelectBestAvailable(Player* bot, Unit* target,
                                                const CanCastCallback& canCast,
                                                TTKEstimator* ttkEstimator) const
{
    if (_entries.empty() || !bot)
        return 0;

    CacheCastTimes(bot);

    // Iterate in priority order (entries are ordered by insertion: primary first, then alternatives)
    for (const auto& entry : _entries)
    {
        // Skip if TTK check says this spell is too slow
        if (ShouldSkipForTTK(entry, target, ttkEstimator, bot))
            continue;

        // Check if the spell can be cast
        if (canCast(entry.spellId, target))
            return entry.spellId;
    }

    return 0;
}

uint32 SpellFallbackChain::SelectBestAvailableBasic(Player* bot, Unit* target,
                                                     TTKEstimator* ttkEstimator) const
{
    if (_entries.empty() || !bot || !bot->IsInWorld())
        return 0;

    CacheCastTimes(bot);
    Difficulty diff = bot->GetMap()->GetDifficultyID();

    for (const auto& entry : _entries)
    {
        // Skip if TTK check says this spell is too slow
        if (ShouldSkipForTTK(entry, target, ttkEstimator, bot))
            continue;

        const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(entry.spellId, diff);
        if (!spellInfo)
            continue;

        // Check if the bot knows this spell
        if (!bot->HasSpell(entry.spellId))
            continue;

        // Check cooldown
        if (bot->GetSpellHistory()->HasCooldown(entry.spellId))
            continue;

        // Check if target is in range (if target provided)
        if (target)
        {
            if (!target->IsAlive())
                continue;

            float range = spellInfo->GetMaxRange(false, bot, nullptr);
            if (range > 0.0f && bot->GetDistance(target) > range)
                continue;
        }

        // Check mana/resource cost
        SpellPowerCost const* powerCost = nullptr;
        auto costs = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
        for (auto const& cost : costs)
        {
            if (cost.Power == bot->GetPowerType())
            {
                powerCost = &cost;
                break;
            }
        }
        if (powerCost && static_cast<int32>(bot->GetPower(bot->GetPowerType())) < powerCost->Amount)
            continue;

        return entry.spellId;
    }

    return 0;
}

} // namespace Playerbot
