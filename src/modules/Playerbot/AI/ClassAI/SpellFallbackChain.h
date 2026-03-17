/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * Spell Fallback Chain: When a primary spell fails (CD, range, mana, TTK),
 * automatically tries alternatives in priority order. Prevents wasting GCDs.
 *
 * Usage in specialization:
 *   SpellFallbackChain singleTargetDamage;
 *   singleTargetDamage.SetPrimary(FIREBALL);
 *   singleTargetDamage.AddAlternative(FIRE_BLAST, 0.9f);   // Slightly lower priority
 *   singleTargetDamage.AddAlternative(SCORCH, 0.7f);       // Instant fallback
 *
 *   uint32 bestSpell = singleTargetDamage.SelectBestAvailable(bot, target, canCastFn);
 *   if (bestSpell) CastSpell(bestSpell, target);
 */

#pragma once

#include "Define.h"
#include <vector>
#include <functional>

class Player;
class Unit;

namespace Playerbot
{

class TTKEstimator;

/// Entry in the fallback chain
struct FallbackSpellEntry
{
    uint32 spellId;
    float priorityWeight;       // 1.0 = primary priority, lower = less preferred
    bool isInstant;             // Cached: true if cast time is 0
    uint32 cachedCastTimeMs;    // Cached cast time in ms (0 = not yet cached)

    FallbackSpellEntry(uint32 id, float weight)
        : spellId(id), priorityWeight(weight), isInstant(false), cachedCastTimeMs(0) {}
};

/// Callback type for checking if a spell can be cast on a target.
/// Signature: bool(uint32 spellId, Unit* target)
using CanCastCallback = std::function<bool(uint32, Unit*)>;

/// Spell Fallback Chain: ordered list of spell alternatives with automatic selection.
class TC_GAME_API SpellFallbackChain
{
public:
    SpellFallbackChain() = default;
    ~SpellFallbackChain() = default;

    // Build the chain
    void SetPrimary(uint32 spellId);
    void AddAlternative(uint32 spellId, float priorityWeight = 0.8f);
    void Clear();

    /// Select the best available spell from the chain.
    /// Checks each spell in priority order using the canCast callback.
    /// If ttkEstimator is provided, also skips spells with cast times that exceed TTK.
    /// Returns 0 if no spell is available.
    uint32 SelectBestAvailable(Player* bot, Unit* target,
                               const CanCastCallback& canCast,
                               TTKEstimator* ttkEstimator = nullptr) const;

    /// Select the best available spell without a callback (uses basic range/CD checks).
    /// Less flexible than the callback version but simpler to use.
    uint32 SelectBestAvailableBasic(Player* bot, Unit* target,
                                    TTKEstimator* ttkEstimator = nullptr) const;

    // Query
    bool IsEmpty() const { return _entries.empty(); }
    uint32 GetPrimarySpell() const;
    size_t GetChainLength() const { return _entries.size(); }
    const std::vector<FallbackSpellEntry>& GetEntries() const { return _entries; }

private:
    // Ensure cast time cache is populated
    void CacheCastTimes(Player* bot) const;

    // Check if a spell should be skipped due to TTK
    bool ShouldSkipForTTK(const FallbackSpellEntry& entry, Unit* target,
                          TTKEstimator* ttkEstimator, Player* bot) const;

    mutable std::vector<FallbackSpellEntry> _entries;  // mutable for lazy cache population
    mutable bool _castTimesCached = false;
};

} // namespace Playerbot
