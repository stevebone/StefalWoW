/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DiseaseManager.h"
#include "Player.h"
#include "Unit.h"
#include "SpellAuras.h"
#include "../../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "GameTime.h"

namespace Playerbot
{

void DiseaseManager::UpdateDiseases(Unit* target)
{
    if (!target)
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();
    ObjectGuid targetGuid = target->GetGUID();    // Clean up expired diseases
    CleanupExpiredDiseases();

    // Check current diseases on target
    auto it = _activeDiseases.find(targetGuid);
    if (it != _activeDiseases.end())
    {
        for (auto& disease : it->second)
        {
            if (currentTime >= disease.expirationTime)
            {
                disease.needsRefresh = true;
            }
        }
    }
}

bool DiseaseManager::HasDisease(Unit* target, DiseaseType type)
{
    if (!target)
        return false;

    ObjectGuid targetGuid = target->GetGUID();
    auto it = _activeDiseases.find(targetGuid);
    if (it == _activeDiseases.end())
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();
    for (const auto& disease : it->second)
    {
        if (disease.type == type && currentTime < disease.expirationTime)
            return true;
    }

    return false;
}

bool DiseaseManager::ShouldApplyDisease(Unit* target, DiseaseType type)
{
    if (!target)
        return false;

    return !HasDisease(target, type) || GetDiseaseTimeRemaining(target, type) < 5000; // Refresh if < 5 seconds
}

void DiseaseManager::ApplyDisease(Unit* target, DiseaseType type, uint32 spellId)
{
    if (!target)
        return;

    ObjectGuid targetGuid = target->GetGUID();
    DiseaseInfo disease(type, spellId, DISEASE_DURATION);

    _activeDiseases[targetGuid].push_back(disease);
}

uint32 DiseaseManager::GetDiseaseTimeRemaining(Unit* target, DiseaseType type)
{
    if (!target)
        return 0;

    ObjectGuid targetGuid = target->GetGUID();
    auto it = _activeDiseases.find(targetGuid);
    if (it == _activeDiseases.end())
        return 0;

    uint32 currentTime = GameTime::GetGameTimeMS();
    for (const auto& disease : it->second)
    {
        if (disease.type == type && currentTime < disease.expirationTime)
            return disease.expirationTime - currentTime;
    }

    return 0;
}

void DiseaseManager::CleanupExpiredDiseases()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (auto it = _activeDiseases.begin(); it != _activeDiseases.end();)
    {
        auto& diseases = it->second;
        diseases.erase(
            ::std::remove_if(diseases.begin(), diseases.end(),
                [currentTime](const DiseaseInfo& disease) {
                    return currentTime >= disease.expirationTime;
                }),
            diseases.end()
        );

        if (diseases.empty())
            it = _activeDiseases.erase(it);
        else
            ++it;
    }
}

bool DiseaseManager::HasBothDiseases(Unit* target)
{
    if (!target)
        return false;

    return HasDisease(target, DiseaseType::BLOOD_PLAGUE) &&
           HasDisease(target, DiseaseType::FROST_FEVER);
}

bool DiseaseManager::NeedsFrostFever(Unit* target)
{
    if (!target)
        return false;

    return !HasDisease(target, DiseaseType::FROST_FEVER) ||
           GetDiseaseTimeRemaining(target, DiseaseType::FROST_FEVER) < 5000;
}

bool DiseaseManager::NeedsBloodPlague(Unit* target)
{
    if (!target)
        return false;

    return !HasDisease(target, DiseaseType::BLOOD_PLAGUE) ||
           GetDiseaseTimeRemaining(target, DiseaseType::BLOOD_PLAGUE) < 5000;
}

float DiseaseManager::GetDiseaseUptime()
{
    // Simple implementation - could be enhanced with actual tracking
    return 85.0f; // Placeholder return value
}

} // namespace Playerbot