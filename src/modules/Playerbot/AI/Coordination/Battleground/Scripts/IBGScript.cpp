/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "IBGScript.h"

namespace Playerbot::Coordination::Battleground
{

// ============================================================================
// DEFAULT IMPLEMENTATIONS
// ============================================================================

BGPhaseInfo::Phase IBGScript::GetMatchPhase(uint32 timeRemaining,
    uint32 allianceScore, uint32 hordeScore) const
{
    // Default phase detection based on score progress
    uint32 maxScore = GetMaxScore();
    uint32 highScore = std::max(allianceScore, hordeScore);
    float progress = maxScore > 0 ? static_cast<float>(highScore) / maxScore : 0.0f;

    if (progress >= 0.9f)
        return BGPhaseInfo::Phase::CLOSING;
    if (progress >= 0.66f)
        return BGPhaseInfo::Phase::LATE_GAME;
    if (progress >= 0.33f)
        return BGPhaseInfo::Phase::MID_GAME;
    if (progress >= 0.1f)
        return BGPhaseInfo::Phase::EARLY_GAME;

    return BGPhaseInfo::Phase::OPENING;
}

float IBGScript::CalculateWinProbability(uint32 allianceScore, uint32 hordeScore,
    uint32 timeRemaining, uint32 objectivesControlled, uint32 faction) const
{
    // Simple default win probability calculation
    uint32 maxScore = GetMaxScore();
    if (maxScore == 0)
        return 0.5f;

    uint32 ourScore = (faction == ALLIANCE) ? allianceScore : hordeScore;
    uint32 theirScore = (faction == ALLIANCE) ? hordeScore : allianceScore;

    // Score-based probability
    float scoreFactor = 0.5f;
    if (ourScore + theirScore > 0)
    {
        scoreFactor = static_cast<float>(ourScore) / (ourScore + theirScore);
    }

    // Objective control bonus (0-0.2)
    float controlBonus = std::min(0.2f, objectivesControlled * 0.05f);

    // Clamp to 0.0 - 1.0
    float probability = scoreFactor + (ourScore > theirScore ? controlBonus : -controlBonus);
    return std::clamp(probability, 0.0f, 1.0f);
}

Position IBGScript::GetTacticalPosition(
    BGPositionData::PositionType positionType, uint32 faction) const
{
    // Default: return positions based on type
    auto strategicPositions = GetStrategicPositions();

    for (const auto& pos : strategicPositions)
    {
        if (pos.posType == positionType)
        {
            // Check faction compatibility
            if (pos.faction == 0 || pos.faction == faction)
            {
                return Position(pos.x, pos.y, pos.z, pos.orientation);
            }
        }
    }

    // Fallback: return a spawn position
    auto spawnPositions = GetSpawnPositions(faction);
    if (!spawnPositions.empty())
    {
        const auto& spawn = spawnPositions[0];
        return Position(spawn.x, spawn.y, spawn.z, spawn.orientation);
    }

    // Ultimate fallback: origin (shouldn't happen)
    return Position(0, 0, 0, 0);
}

} // namespace Playerbot::Coordination::Battleground
