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
#include "GameTime.h"

namespace Playerbot
{

// Rune types for Death Knight abilities
enum class RuneType : uint8
{
    BLOOD = 0,
    FROST = 1,
    UNHOLY = 2,
    DEATH = 3
};

// Rune state tracking
struct RuneInfo
{
    RuneType type;
    bool available;
    uint32 cooldownRemaining;
    uint32 lastUsed;

    RuneInfo() : type(RuneType::BLOOD), available(true), cooldownRemaining(0), lastUsed(0) {}
    RuneInfo(RuneType t) : type(t), available(true), cooldownRemaining(0), lastUsed(0) {}

    bool IsReady() const { return available && cooldownRemaining == 0; }
    void Use() { available = false; cooldownRemaining = 10000; lastUsed = GameTime::GetGameTimeMS(); }
};

} // namespace Playerbot