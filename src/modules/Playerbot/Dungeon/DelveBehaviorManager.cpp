/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DelveBehaviorManager.h"
#include "Log.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Map.h"
#include "GameTime.h"
#include "ObjectAccessor.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR
// ============================================================================

DelveBehaviorManager::DelveBehaviorManager(Player* bot)
    : _bot(bot)
{
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void DelveBehaviorManager::OnDelveEntered(uint32 mapId, uint8 tier)
{
    if (!_bot)
        return;

    _currentMapId = mapId;
    _currentTier = tier;
    _tierConfig = GetTierConfig(tier);
    _delveStartTime = GameTime::GetGameTimeMS();

    _objectives.clear();
    _currentObjectiveIndex = 0;
    _discoveredChests.clear();
    _lootedChests.clear();
    _companion = DelveCompanionInfo{};

    _stats.delvesEntered++;

    TransitionTo(DelveState::ENTERING);

    TC_LOG_INFO("module.playerbot", "DelveBehaviorManager: Bot {} entered delve (map={}, tier={})",
        _bot->GetName(), mapId, tier);
}

void DelveBehaviorManager::OnDelveExited()
{
    if (_state == DelveState::COMPLETED)
    {
        _stats.delvesCompleted++;
        if (_currentTier > _stats.highestTierCompleted)
            _stats.highestTierCompleted = _currentTier;
    }
    else if (_state != DelveState::IDLE)
    {
        _stats.delvesFailed++;
    }

    uint32 duration = GetDelveDuration();
    _stats.totalDelveDurationMs += duration;

    TC_LOG_INFO("module.playerbot", "DelveBehaviorManager: Bot {} exited delve (tier={}, duration={}ms, state={})",
        _bot ? _bot->GetName() : "unknown", _currentTier, duration, GetStateString());

    TransitionTo(DelveState::IDLE);
    _currentMapId = 0;
    _currentTier = 0;
}

void DelveBehaviorManager::OnObjectiveCompleted(uint32 objectiveId)
{
    for (auto& obj : _objectives)
    {
        if (obj.objectiveId == objectiveId && !obj.completed)
        {
            obj.completed = true;
            _stats.objectivesCompleted++;
            TC_LOG_DEBUG("module.playerbot", "DelveBehaviorManager: Objective {} completed for bot {}",
                objectiveId, _bot ? _bot->GetName() : "unknown");
            break;
        }
    }

    // Check if all objectives are done
    bool allDone = true;
    for (auto const& obj : _objectives)
    {
        if (!obj.completed)
        {
            allDone = false;
            break;
        }
    }

    if (allDone && !_objectives.empty())
    {
        TC_LOG_INFO("module.playerbot", "DelveBehaviorManager: All objectives completed for bot {}",
            _bot ? _bot->GetName() : "unknown");
        TransitionTo(DelveState::LOOTING);
    }
}

void DelveBehaviorManager::OnBossKilled(uint32 creatureEntry)
{
    _stats.bossesKilled++;

    // Mark boss objective as completed
    for (auto& obj : _objectives)
    {
        if (obj.isBoss && obj.creatureEntry == creatureEntry)
        {
            obj.completed = true;
            _stats.objectivesCompleted++;
        }
    }

    TC_LOG_INFO("module.playerbot", "DelveBehaviorManager: Boss {} killed in delve by bot {}",
        creatureEntry, _bot ? _bot->GetName() : "unknown");

    // After boss, transition to looting
    TransitionTo(DelveState::LOOTING);
}

void DelveBehaviorManager::OnDeathInDelve()
{
    _stats.deathsInDelves++;

    // Higher tiers - death is more impactful
    if (_currentTier >= 8)
    {
        TC_LOG_WARN("module.playerbot", "DelveBehaviorManager: Bot {} died in tier {} delve",
            _bot ? _bot->GetName() : "unknown", _currentTier);
    }
}

// ============================================================================
// UPDATE
// ============================================================================

void DelveBehaviorManager::Update(uint32 diff)
{
    if (_state == DelveState::IDLE)
        return;

    if (!_bot || !_bot->IsInWorld())
    {
        OnDelveExited();
        return;
    }

    _stateTimer += diff;
    _scanTimer += diff;
    _companionCheckTimer += diff;

    // Periodic scanning for objectives, chests, and companion
    if (_scanTimer >= SCAN_INTERVAL)
    {
        _scanTimer = 0;
        ScanForLootChests();
    }

    if (_companionCheckTimer >= COMPANION_CHECK_INTERVAL)
    {
        _companionCheckTimer = 0;
        UpdateCompanionTracking();
    }

    // Auto-detect combat state changes
    if (_bot->IsInCombat() && _state == DelveState::EXPLORING)
    {
        TransitionTo(DelveState::COMBAT);
    }
    else if (!_bot->IsInCombat() && _state == DelveState::COMBAT)
    {
        TransitionTo(DelveState::EXPLORING);
    }

    // State-specific updates
    switch (_state)
    {
        case DelveState::ENTERING:    UpdateEnteringState(diff); break;
        case DelveState::EXPLORING:   UpdateExploringState(diff); break;
        case DelveState::OBJECTIVE:   UpdateObjectiveState(diff); break;
        case DelveState::COMBAT:      UpdateCombatState(diff); break;
        case DelveState::BOSS:        UpdateBossState(diff); break;
        case DelveState::LOOTING:     UpdateLootingState(diff); break;
        case DelveState::COMPLETED:   UpdateCompletedState(diff); break;
        default: break;
    }
}

// ============================================================================
// STATE QUERIES
// ============================================================================

::std::string DelveBehaviorManager::GetStateString() const
{
    switch (_state)
    {
        case DelveState::IDLE:       return "Idle";
        case DelveState::ENTERING:   return "Entering";
        case DelveState::EXPLORING:  return "Exploring";
        case DelveState::OBJECTIVE:  return "Objective";
        case DelveState::COMBAT:     return "Combat";
        case DelveState::BOSS:       return "Boss";
        case DelveState::LOOTING:    return "Looting";
        case DelveState::COMPLETED:  return "Completed";
        case DelveState::FAILED:     return "Failed";
        default:                     return "Unknown";
    }
}

float DelveBehaviorManager::GetProgress() const
{
    if (_objectives.empty())
        return 0.0f;

    uint32 completed = GetCompletedObjectiveCount();
    return static_cast<float>(completed) / static_cast<float>(_objectives.size());
}

uint32 DelveBehaviorManager::GetDelveDuration() const
{
    if (_delveStartTime == 0)
        return 0;

    uint32 now = GameTime::GetGameTimeMS();
    return (now > _delveStartTime) ? (now - _delveStartTime) : 0;
}

uint32 DelveBehaviorManager::GetCompletedObjectiveCount() const
{
    uint32 count = 0;
    for (auto const& obj : _objectives)
    {
        if (obj.completed)
            ++count;
    }
    return count;
}

// ============================================================================
// COMPANION MANAGEMENT
// ============================================================================

bool DelveBehaviorManager::ShouldInteractWithCompanion() const
{
    if (!_companion.isActive)
        return false;

    return _companion.needsInteraction;
}

void DelveBehaviorManager::HandleCompanionInteraction()
{
    if (!_bot || !_companion.isActive)
        return;

    Creature* companion = ObjectAccessor::GetCreature(*_bot, _companion.companionGuid);
    if (!companion)
        return;

    // Check if within interaction range
    if (_bot->GetDistance(companion) > 10.0f)
    {
        TC_LOG_DEBUG("module.playerbot", "DelveBehaviorManager: Bot {} moving to companion for interaction",
            _bot->GetName());
        return;
    }

    _companion.needsInteraction = false;
    TC_LOG_DEBUG("module.playerbot", "DelveBehaviorManager: Bot {} interacted with companion",
        _bot->GetName());
}

void DelveBehaviorManager::UpdateCompanionTracking()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // If we already have a companion tracked, update position
    if (_companion.isActive)
    {
        Creature* companion = ObjectAccessor::GetCreature(*_bot, _companion.companionGuid);
        if (companion && companion->IsAlive())
        {
            _companion.lastKnownPosition = companion->GetPosition();
            _companion.healthPercent = companion->GetHealthPct();
        }
        else
        {
            _companion.isActive = false;
        }
        return;
    }

    // Search for companion NPC nearby
    // Known Brann Bronzebeard delve companion entries
    static const uint32 COMPANION_ENTRIES[] = {
        226653,  // Brann Bronzebeard (Delve companion - TWW)
        226654,  // Brann Bronzebeard (Delve variant)
    };

    for (uint32 entry : COMPANION_ENTRIES)
    {
        Creature* companion = _bot->FindNearestCreature(entry, 100.0f);
        if (companion && companion->IsAlive())
        {
            _companion.companionGuid = companion->GetGUID();
            _companion.companionEntry = entry;
            _companion.isActive = true;
            _companion.lastKnownPosition = companion->GetPosition();
            _companion.healthPercent = companion->GetHealthPct();
            TC_LOG_DEBUG("module.playerbot", "DelveBehaviorManager: Bot {} found companion (entry={})",
                _bot->GetName(), entry);
            return;
        }
    }
}

// ============================================================================
// OBJECTIVE MANAGEMENT
// ============================================================================

DelveObjective const* DelveBehaviorManager::GetCurrentObjective() const
{
    if (_currentObjectiveIndex >= _objectives.size())
        return nullptr;

    return &_objectives[_currentObjectiveIndex];
}

// ============================================================================
// COMBAT BEHAVIOR QUERIES
// ============================================================================

bool DelveBehaviorManager::ShouldUseConsumables() const
{
    return _tierConfig.requiresConsumables;
}

bool DelveBehaviorManager::ShouldBurstCurrentTarget() const
{
    if (!_tierConfig.useBurstOnBoss)
        return false;

    return _state == DelveState::BOSS;
}

float DelveBehaviorManager::GetDefensiveCooldownThreshold() const
{
    return _tierConfig.defensiveCooldownThreshold;
}

bool DelveBehaviorManager::ShouldGroupUp() const
{
    // Group up during boss encounters and high tier objectives
    return _state == DelveState::BOSS ||
           (_state == DelveState::OBJECTIVE && _currentTier >= 6);
}

// ============================================================================
// FORMATTED OUTPUT
// ============================================================================

::std::string DelveBehaviorManager::FormatSummary() const
{
    ::std::ostringstream ss;
    ss << "=== Delve Stats ===\n";
    ss << "Entered: " << _stats.delvesEntered
       << " | Completed: " << _stats.delvesCompleted
       << " | Failed: " << _stats.delvesFailed << "\n";
    ss << "Objectives: " << _stats.objectivesCompleted
       << " | Bosses: " << _stats.bossesKilled
       << " | Chests: " << _stats.chestsLooted << "\n";
    ss << "Highest Tier: " << _stats.highestTierCompleted
       << " | Deaths: " << _stats.deathsInDelves << "\n";

    if (_stats.delvesCompleted > 0)
    {
        uint32 avgDuration = static_cast<uint32>(_stats.totalDelveDurationMs / _stats.delvesCompleted);
        ss << "Avg Duration: " << (avgDuration / 1000) << "s\n";
    }

    if (IsInDelve())
    {
        ss << "\n--- Current Delve ---\n";
        ss << "Tier: " << static_cast<uint32>(_currentTier)
           << " | State: " << GetStateString()
           << " | Progress: " << ::std::fixed << ::std::setprecision(0) << (GetProgress() * 100.0f) << "%\n";
        ss << "Duration: " << (GetDelveDuration() / 1000) << "s\n";
        if (_companion.isActive)
            ss << "Companion: Active (HP: " << ::std::fixed << ::std::setprecision(0) << _companion.healthPercent << "%)\n";
    }

    return ss.str();
}

// ============================================================================
// STATE TRANSITIONS
// ============================================================================

void DelveBehaviorManager::TransitionTo(DelveState newState)
{
    if (_state == newState)
        return;

    DelveState oldState = _state;
    _state = newState;
    _stateTimer = 0;

    TC_LOG_DEBUG("module.playerbot", "DelveBehaviorManager: Bot {} state {} -> {}",
        _bot ? _bot->GetName() : "unknown",
        static_cast<uint32>(oldState), static_cast<uint32>(newState));
}

void DelveBehaviorManager::UpdateEnteringState(uint32 /*diff*/)
{
    if (_stateTimer >= ENTERING_ORIENTATION_TIME)
    {
        // Discover objectives and companion
        DiscoverObjectives();
        DiscoverCompanion();

        TransitionTo(DelveState::EXPLORING);
    }
}

void DelveBehaviorManager::UpdateExploringState(uint32 /*diff*/)
{
    // Check if near any objective
    DelveObjective* nearest = FindNearestIncompleteObjective();
    if (nearest)
    {
        float dist = _bot ? _bot->GetExactDist(nearest->location) : 999.0f;
        if (dist <= OBJECTIVE_PROXIMITY)
        {
            if (nearest->isBoss)
                TransitionTo(DelveState::BOSS);
            else
                TransitionTo(DelveState::OBJECTIVE);
        }
    }

    // Check if all objectives are done
    if (!_objectives.empty() && GetCompletedObjectiveCount() == _objectives.size())
    {
        TransitionTo(DelveState::LOOTING);
    }
}

void DelveBehaviorManager::UpdateObjectiveState(uint32 /*diff*/)
{
    // If no longer near objective or objective completed, return to exploring
    DelveObjective const* current = GetCurrentObjective();
    if (!current || current->completed)
    {
        // Move to next objective
        _currentObjectiveIndex++;
        TransitionTo(DelveState::EXPLORING);
    }
}

void DelveBehaviorManager::UpdateCombatState(uint32 /*diff*/)
{
    // Combat state is primarily driven by Update() auto-detect
    // When combat ends, we return to exploring (handled in Update)
}

void DelveBehaviorManager::UpdateBossState(uint32 /*diff*/)
{
    // Boss state stays until OnBossKilled or combat ends
    if (!_bot || !_bot->IsInCombat())
    {
        // Check if boss objective is done
        DelveObjective const* current = GetCurrentObjective();
        if (current && current->isBoss && current->completed)
        {
            TransitionTo(DelveState::LOOTING);
        }
        else
        {
            TransitionTo(DelveState::EXPLORING);
        }
    }
}

void DelveBehaviorManager::UpdateLootingState(uint32 /*diff*/)
{
    if (_stateTimer >= LOOTING_TIMEOUT)
    {
        TransitionTo(DelveState::COMPLETED);
    }

    // Scan for nearby loot chests
    if (_bot && !_discoveredChests.empty())
    {
        for (auto const& chestGuid : _discoveredChests)
        {
            // Skip already looted
            bool alreadyLooted = false;
            for (auto const& looted : _lootedChests)
            {
                if (looted == chestGuid)
                {
                    alreadyLooted = true;
                    break;
                }
            }
            if (alreadyLooted)
                continue;

            GameObject* chest = _bot->GetMap()->GetGameObject(chestGuid);
            if (chest && _bot->GetDistance(chest) <= CHEST_INTERACTION_RANGE)
            {
                _lootedChests.push_back(chestGuid);
                _stats.chestsLooted++;
                TC_LOG_DEBUG("module.playerbot", "DelveBehaviorManager: Bot {} looted chest in delve",
                    _bot->GetName());
            }
        }
    }

    // If all chests looted or timeout, complete
    if (_lootedChests.size() >= _discoveredChests.size())
    {
        TransitionTo(DelveState::COMPLETED);
    }
}

void DelveBehaviorManager::UpdateCompletedState(uint32 /*diff*/)
{
    if (_stateTimer >= COMPLETED_LINGER_TIME)
    {
        TC_LOG_INFO("module.playerbot", "DelveBehaviorManager: Bot {} delve completed, ready to exit",
            _bot ? _bot->GetName() : "unknown");
        // The bot's main AI loop should handle the exit
    }
}

// ============================================================================
// DISCOVERY
// ============================================================================

void DelveBehaviorManager::DiscoverObjectives()
{
    // In a real implementation, objectives would come from the instance script
    // or scenario system. For now, we create placeholder objectives based on
    // known delve patterns:
    // - Delves have 2-4 objectives (kill groups, interact with objects)
    // - Final objective is always a boss
    // This will be populated from InstanceScript data when available

    TC_LOG_DEBUG("module.playerbot", "DelveBehaviorManager: Discovering objectives for delve (map={}, tier={})",
        _currentMapId, _currentTier);

    // Objectives will be populated dynamically as the bot encounters them
    // through event callbacks from the encounter/scenario system
}

void DelveBehaviorManager::DiscoverCompanion()
{
    UpdateCompanionTracking();
}

void DelveBehaviorManager::ScanForLootChests()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // Look for loot chests nearby
    // Delve reward chests typically appear after boss kills
    ::std::vector<GameObject*> chests;
    // Use standard grid search for GameObjects
    float searchRadius = 50.0f;

    // Simple search using FindNearestGameObject with known chest entries
    // Known delve chest entries (Bountiful/Gilded delve chests)
    static const uint32 DELVE_CHEST_ENTRIES[] = {
        411320,  // Bountiful Delve Chest (Tier 1-3)
        411321,  // Bountiful Delve Chest (Tier 4-7)
        411322,  // Gilded Delve Chest (Tier 8-11)
    };

    for (uint32 entry : DELVE_CHEST_ENTRIES)
    {
        GameObject* chest = _bot->FindNearestGameObject(entry, searchRadius);
        if (chest)
        {
            ObjectGuid guid = chest->GetGUID();
            bool alreadyDiscovered = false;
            for (auto const& existing : _discoveredChests)
            {
                if (existing == guid)
                {
                    alreadyDiscovered = true;
                    break;
                }
            }
            if (!alreadyDiscovered)
            {
                _discoveredChests.push_back(guid);
                TC_LOG_DEBUG("module.playerbot", "DelveBehaviorManager: Bot {} discovered chest (entry={})",
                    _bot->GetName(), entry);
            }
        }
    }
}

DelveObjective* DelveBehaviorManager::FindNearestIncompleteObjective()
{
    if (!_bot || _objectives.empty())
        return nullptr;

    DelveObjective* nearest = nullptr;
    float nearestDist = 999999.0f;

    for (auto& obj : _objectives)
    {
        if (obj.completed)
            continue;

        float dist = _bot->GetExactDist(obj.location);
        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearest = &obj;
        }
    }

    return nearest;
}

// ============================================================================
// TIER CONFIGURATION
// ============================================================================

DelveTierConfig DelveBehaviorManager::GetTierConfig(uint8 tier) const
{
    DelveTierConfig config;
    config.tier = tier;

    // Scale difficulty and requirements by tier
    // Tiers 1-3: Easy, no special requirements
    // Tiers 4-7: Moderate, consumables help
    // Tiers 8-11: Hard, consumables required, full group recommended
    switch (tier)
    {
        case 1:
            config.difficultyMultiplier = 0.5f;
            config.requiresConsumables = false;
            config.requiresFullGroup = false;
            config.expectedDurationMs = 180000;  // 3 min
            config.defensiveCooldownThreshold = 0.3f;
            config.useBurstOnBoss = false;
            break;
        case 2:
            config.difficultyMultiplier = 0.7f;
            config.requiresConsumables = false;
            config.requiresFullGroup = false;
            config.expectedDurationMs = 240000;  // 4 min
            config.defensiveCooldownThreshold = 0.35f;
            config.useBurstOnBoss = false;
            break;
        case 3:
            config.difficultyMultiplier = 0.85f;
            config.requiresConsumables = false;
            config.requiresFullGroup = false;
            config.expectedDurationMs = 300000;  // 5 min
            config.defensiveCooldownThreshold = 0.38f;
            config.useBurstOnBoss = true;
            break;
        case 4:
            config.difficultyMultiplier = 1.0f;
            config.requiresConsumables = true;
            config.requiresFullGroup = false;
            config.expectedDurationMs = 360000;  // 6 min
            config.defensiveCooldownThreshold = 0.40f;
            config.useBurstOnBoss = true;
            break;
        case 5:
            config.difficultyMultiplier = 1.15f;
            config.requiresConsumables = true;
            config.requiresFullGroup = false;
            config.expectedDurationMs = 420000;  // 7 min
            config.defensiveCooldownThreshold = 0.42f;
            config.useBurstOnBoss = true;
            break;
        case 6:
            config.difficultyMultiplier = 1.3f;
            config.requiresConsumables = true;
            config.requiresFullGroup = false;
            config.expectedDurationMs = 480000;  // 8 min
            config.defensiveCooldownThreshold = 0.45f;
            config.useBurstOnBoss = true;
            break;
        case 7:
            config.difficultyMultiplier = 1.5f;
            config.requiresConsumables = true;
            config.requiresFullGroup = true;
            config.expectedDurationMs = 540000;  // 9 min
            config.defensiveCooldownThreshold = 0.48f;
            config.useBurstOnBoss = true;
            break;
        case 8:
        case 9:
        case 10:
        case 11:
        default:
            config.difficultyMultiplier = 1.5f + (tier - 7) * 0.25f;
            config.requiresConsumables = true;
            config.requiresFullGroup = true;
            config.expectedDurationMs = 600000 + (tier - 8) * 60000;  // 10+ min
            config.defensiveCooldownThreshold = 0.50f + (tier - 8) * 0.03f;
            config.useBurstOnBoss = true;
            break;
    }

    return config;
}

} // namespace Playerbot
