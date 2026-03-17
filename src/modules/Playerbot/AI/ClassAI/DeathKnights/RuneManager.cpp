/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RuneManager.h"
#include "Player.h"
#include "Log.h"
#include "GameTime.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries

namespace Playerbot
{

RuneManager::RuneManager(Player* bot)
    : _bot(bot)
    , _lastRegenUpdate(0)
    , _regenModifier(1.0f)
{
    // Initialize runes: 2 Blood, 2 Frost, 2 Unholy
    _runes[0] = RuneInfo(RuneType::BLOOD);
    _runes[1] = RuneInfo(RuneType::BLOOD);
    _runes[2] = RuneInfo(RuneType::FROST);
    _runes[3] = RuneInfo(RuneType::FROST);
    _runes[4] = RuneInfo(RuneType::UNHOLY);
    _runes[5] = RuneInfo(RuneType::UNHOLY);
}

void RuneManager::Update(uint32 diff)
{
    RegenerateRunes(diff);
}

bool RuneManager::HasAvailableRunes(RuneType type, uint32 count) const
{
    return GetAvailableRunes(type) >= count;
}

void RuneManager::ConsumeRunes(RuneType type, uint32 count)
{
    uint32 consumed = 0;

    for (auto& rune : _runes)
    {
        if (consumed >= count)
            break;

        if (rune.type == type && rune.IsReady())
        {
            rune.Use();
            consumed++;
        }
    }

    // If we couldn't consume enough of the specific type, try death runes
    if (consumed < count)
    {
        for (auto& rune : _runes)
        {
            if (consumed >= count)
                break;

            if (rune.type == RuneType::DEATH && rune.IsReady())
            {
                rune.Use();
                consumed++;
            }
        }
    }
}



bool RuneManager::CanConvertRune(RuneType from, RuneType to) const
{
    // Check if we have available runes of the source type
    for (const auto& rune : _runes)
    {
        if (rune.type == from && !rune.IsReady())
        {
            return true; // Can convert spent runes
        }
    }

    return false;
}

void RuneManager::ConvertRune(RuneType from, RuneType to)
{
    for (auto& rune : _runes)
    {
        if (rune.type == from && !rune.IsReady())
        {
            rune.type = to;
            break;
        }
    }
}

void RuneManager::RegenerateRunes(uint32 diff)
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (auto& rune : _runes)
    {
        if (!rune.available && rune.cooldownRemaining > 0)
        {
            if (rune.cooldownRemaining > diff)
            {
                rune.cooldownRemaining -= diff;
            }
            else
            {
                rune.cooldownRemaining = 0;
                rune.available = true;
            }
        }
    }
}

uint32 RuneManager::GetRuneCooldown(uint8 runeIndex) const
{
    if (runeIndex >= _runes.size())
        return 0;

    return _runes[runeIndex].cooldownRemaining;
}

bool RuneManager::IsRuneReady(uint8 runeIndex) const
{
    if (runeIndex >= _runes.size())
        return false;

    return _runes[runeIndex].IsReady();
}


void RuneManager::ApplyRuneRegenModifier(float modifier)
{
    _regenModifier = modifier;
}

void RuneManager::ConsumeRunes(uint8 blood, uint8 frost, uint8 unholy)
{
    if (blood > 0)
        ConsumeRunes(RuneType::BLOOD, blood);
    if (frost > 0)
        ConsumeRunes(RuneType::FROST, frost);
    if (unholy > 0)
        ConsumeRunes(RuneType::UNHOLY, unholy);
}

} // namespace Playerbot