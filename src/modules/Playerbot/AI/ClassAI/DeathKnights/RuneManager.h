/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "DeathKnightTypes.h"
#include <array>
#include <chrono>
#include <memory>

class Player;

namespace Playerbot
{

class TC_GAME_API RuneManager
{
public:
    explicit RuneManager(Player* bot);
    ~RuneManager() = default;

    // Core rune management
    void Update(uint32 diff);
    void UpdateRunes(uint32 diff) { Update(diff); } // Alias for compatibility
    bool HasAvailableRunes(RuneType type, uint32 count = 1) const;
    void ConsumeRunes(RuneType type, uint32 count = 1);
    void ConsumeRunes(uint8 blood, uint8 frost, uint8 unholy);
    uint32 GetAvailableRunes(RuneType type) const
    {
        uint32 count = 0;
        for (const auto& rune : _runes)
        {
            if ((rune.type == type || rune.type == RuneType::DEATH) && rune.IsReady())
                count++;
        }
        return count;
    }

    uint32 GetTotalAvailableRunes() const
    {
        uint32 count = 0;
        for (const auto& rune : _runes)
        {
            if (rune.IsReady())
                count++;
        }
        return count;
    }

    // Spell-specific rune checks (blood, frost, unholy counts)
    bool HasRunes(uint32 bloodCount, uint32 frostCount, uint32 unholyCount) const
    {
        uint32 availableBlood = GetAvailableRunes(RuneType::BLOOD);
        uint32 availableFrost = GetAvailableRunes(RuneType::FROST);
        uint32 availableUnholy = GetAvailableRunes(RuneType::UNHOLY);
        return availableBlood >= bloodCount && availableFrost >= frostCount && availableUnholy >= unholyCount;
    }


    // Death runes
    bool CanConvertRune(RuneType from, RuneType to) const;
    void ConvertRune(RuneType from, RuneType to);

    // Rune regeneration
    void RegenerateRunes(uint32 diff);
    uint32 GetRuneCooldown(uint8 runeIndex) const;
    bool IsRuneReady(uint8 runeIndex) const;

    // Utility
    void ResetAllRunes()
    {
        for (auto& rune : _runes)
        {
            rune.available = true;
            rune.cooldownRemaining = 0;
        }
    }
    void ResetRunes() { ResetAllRunes(); } // Alias for compatibility
    void ApplyRuneRegenModifier(float modifier);

private:
    Player* _bot;
    ::std::array<RuneInfo, 6> _runes;
    uint32 _lastRegenUpdate;
    float _regenModifier;

    // Rune indices: 0-1 Blood, 2-3 Frost, 4-5 Unholy
    static constexpr uint8 BLOOD_RUNE_START = 0;
    static constexpr uint8 FROST_RUNE_START = 2;
    static constexpr uint8 UNHOLY_RUNE_START = 4;
};

} // namespace Playerbot