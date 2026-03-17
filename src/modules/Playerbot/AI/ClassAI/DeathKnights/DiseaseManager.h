/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Timer.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include "GameTime.h"

// Forward declarations
class Player;
class Unit;

namespace Playerbot
{

enum class DiseaseType : uint8
{
    BLOOD_PLAGUE = 0,
    FROST_FEVER = 1,
    NECROTIC_STRIKE = 2
};

struct DiseaseInfo
{
    DiseaseType type;
    uint32 spellId;
    uint32 expirationTime;
    uint32 remainingTime;
    uint32 stacks;
    bool needsRefresh;

    DiseaseInfo() : type(DiseaseType::BLOOD_PLAGUE), spellId(0), expirationTime(0), remainingTime(0), stacks(0), needsRefresh(false) {}
    DiseaseInfo(DiseaseType t, uint32 spell, uint32 duration)
        : type(t), spellId(spell), expirationTime(GameTime::GetGameTimeMS() + duration), remainingTime(duration), stacks(1), needsRefresh(false) {}

    bool IsActive() const { return remainingTime > 0; }
};

class TC_GAME_API DiseaseManager
{
public:
    explicit DiseaseManager(Player* bot) : _bot(bot) {}

    void UpdateDiseases(Unit* target);
    bool HasDisease(Unit* target, DiseaseType type);
    bool ShouldApplyDisease(Unit* target, DiseaseType type);
    void ApplyDisease(Unit* target, DiseaseType type, uint32 spellId);
    uint32 GetDiseaseTimeRemaining(Unit* target, DiseaseType type);
    void CleanupExpiredDiseases();

    // Legacy methods for compatibility
    bool HasBothDiseases(Unit* target);
    bool NeedsFrostFever(Unit* target);
    bool NeedsBloodPlague(Unit* target);
    float GetDiseaseUptime();

private:
    Player* _bot;
    ::std::unordered_map<ObjectGuid, ::std::vector<DiseaseInfo>> _activeDiseases;

    // Disease spell IDs
    static constexpr uint32 BLOOD_PLAGUE_SPELL = 55078;
    static constexpr uint32 FROST_FEVER_SPELL = 55095;
    static constexpr uint32 NECROTIC_STRIKE_SPELL = 73975;
    static constexpr uint32 DISEASE_DURATION = 30000; // 30 seconds
};

} // namespace Playerbot