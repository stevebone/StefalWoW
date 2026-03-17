/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RidingManager.h"
#include "MountManager.h"
#include "Log.h"
#include "Player.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "Map.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "GossipDef.h"
#include <cmath>
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// STATIC DATA INITIALIZATION
// ============================================================================

std::vector<RidingTrainerInfo> RidingManager::_allianceTrainers;
std::vector<RidingTrainerInfo> RidingManager::_hordeTrainers;
std::vector<RidingTrainerInfo> RidingManager::_neutralTrainers;
std::vector<MountVendorInfo> RidingManager::_allianceVendors;
std::vector<MountVendorInfo> RidingManager::_hordeVendors;
bool RidingManager::_databaseInitialized = false;
RidingManager::RidingMetrics RidingManager::_globalMetrics;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

RidingManager::RidingManager(Player* bot)
    : _bot(bot)
{
    if (!_databaseInitialized)
    {
        InitializeDatabase();
    }
}

RidingManager::~RidingManager()
{
    // Cleanup if needed
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void RidingManager::Initialize()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    TC_LOG_DEBUG("module.playerbot", "RidingManager::Initialize - Bot {} initialized riding manager",
        _bot->GetName());

    // Reset state on initialization
    _state = RidingAcquisitionState::IDLE;
    _targetSkill = RidingSkillLevel::NONE;
    _updateTimer = 0;
    _stateTimer = 0;
}

void RidingManager::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive())
        return;

    // Throttle updates
    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL)
        return;
    _updateTimer = 0;

    // Check for state timeout
    if (_state != RidingAcquisitionState::IDLE &&
        _state != RidingAcquisitionState::COMPLETE &&
        _state != RidingAcquisitionState::FAILED)
    {
        _stateTimer += UPDATE_INTERVAL;
        if (_stateTimer > STATE_TIMEOUT)
        {
            TC_LOG_WARN("module.playerbot", "RidingManager::Update - Bot {} state timeout in state {}",
                _bot->GetName(), static_cast<uint8>(_state));
            TransitionTo(RidingAcquisitionState::FAILED);
            _metrics.failedAttempts++;
            _globalMetrics.failedAttempts++;
            return;
        }
    }

    // Process current state
    switch (_state)
    {
        case RidingAcquisitionState::IDLE:
            // Auto-acquire check
            if (_autoAcquireEnabled && NeedsRidingSkill() && CanAffordNextSkill())
            {
                StartRidingAcquisition();
            }
            else if (_autoAcquireEnabled && NeedsMount() && CanAffordMount())
            {
                StartMountAcquisition();
            }
            break;

        case RidingAcquisitionState::NEED_RIDING_SKILL:
            ProcessNeedRidingSkill();
            break;

        case RidingAcquisitionState::TRAVELING_TO_TRAINER:
            ProcessTravelingToTrainer();
            break;

        case RidingAcquisitionState::AT_TRAINER:
            ProcessAtTrainer();
            break;

        case RidingAcquisitionState::LEARNING_SKILL:
            ProcessLearningSkill();
            break;

        case RidingAcquisitionState::NEED_MOUNT:
            ProcessNeedMount();
            break;

        case RidingAcquisitionState::TRAVELING_TO_VENDOR:
            ProcessTravelingToVendor();
            break;

        case RidingAcquisitionState::AT_VENDOR:
            ProcessAtVendor();
            break;

        case RidingAcquisitionState::PURCHASING_MOUNT:
            ProcessPurchasingMount();
            break;

        case RidingAcquisitionState::COMPLETE:
            // Reset to idle after completion
            TransitionTo(RidingAcquisitionState::IDLE);
            break;

        case RidingAcquisitionState::FAILED:
            // Reset to idle after failure (will retry on next update if auto-enabled)
            TransitionTo(RidingAcquisitionState::IDLE);
            break;
    }
}

// ============================================================================
// SKILL CHECKING
// ============================================================================

RidingSkillLevel RidingManager::GetCurrentSkillLevel() const
{
    if (!_bot)
        return RidingSkillLevel::NONE;

    // Check from highest to lowest
    if (_bot->HasSpell(SPELL_FLIGHT_MASTERS_LICENSE))
        return RidingSkillLevel::FLIGHT_MASTERS;
    if (_bot->HasSpell(SPELL_COLD_WEATHER_FLYING))
        return RidingSkillLevel::COLD_WEATHER;
    if (_bot->HasSpell(SPELL_MASTER_RIDING))
        return RidingSkillLevel::MASTER;
    if (_bot->HasSpell(SPELL_ARTISAN_RIDING))
        return RidingSkillLevel::ARTISAN;
    if (_bot->HasSpell(SPELL_EXPERT_RIDING))
        return RidingSkillLevel::EXPERT;
    if (_bot->HasSpell(SPELL_JOURNEYMAN_RIDING))
        return RidingSkillLevel::JOURNEYMAN;
    if (_bot->HasSpell(SPELL_APPRENTICE_RIDING))
        return RidingSkillLevel::APPRENTICE;

    return RidingSkillLevel::NONE;
}

RidingSkillLevel RidingManager::GetNextSkillLevel() const
{
    if (!_bot)
        return RidingSkillLevel::NONE;

    uint32 level = _bot->GetLevel();
    RidingSkillLevel current = GetCurrentSkillLevel();

    // Determine next skill based on level thresholds (WoW 12.0)
    if (level >= 80 && current < RidingSkillLevel::MASTER)
        return RidingSkillLevel::MASTER;
    if (level >= 40 && current < RidingSkillLevel::ARTISAN)
        return RidingSkillLevel::ARTISAN;
    if (level >= 30 && current < RidingSkillLevel::EXPERT)
        return RidingSkillLevel::EXPERT;
    if (level >= 20 && current < RidingSkillLevel::JOURNEYMAN)
        return RidingSkillLevel::JOURNEYMAN;
    if (level >= 10 && current < RidingSkillLevel::APPRENTICE)
        return RidingSkillLevel::APPRENTICE;

    return RidingSkillLevel::NONE;
}

bool RidingManager::NeedsRidingSkill() const
{
    return GetNextSkillLevel() != RidingSkillLevel::NONE;
}

bool RidingManager::NeedsMount() const
{
    if (!_bot)
        return false;

    // Need riding skill first
    if (GetCurrentSkillLevel() == RidingSkillLevel::NONE)
        return false;

    // Check if bot has any mounts
    // This requires checking known spells for mount spells
    // For simplicity, we check common mount spell ranges
    // In a full implementation, integrate with MountManager::GetMountCount()

    // Basic check: has riding skill but no mounts
    // We'll use a heuristic - check for common racial mounts
    uint32 race = _bot->GetRace();
    uint32 basicMount = GetRaceAppropriateMount(_bot->GetLevel());

    return basicMount != 0 && !_bot->HasSpell(basicMount);
}

bool RidingManager::CanAffordNextSkill() const
{
    if (!_bot)
        return false;

    RidingSkillLevel next = GetNextSkillLevel();
    if (next == RidingSkillLevel::NONE)
        return false;

    uint64 cost = GetSkillCost(next);
    uint64 available = _bot->GetMoney();

    return available >= (cost + _minReserveGold);
}

bool RidingManager::CanAffordMount() const
{
    if (!_bot)
        return false;

    uint64 cost = GetMountCost(_bot->GetLevel());
    uint64 available = _bot->GetMoney();

    return available >= (cost + _minReserveGold);
}

// ============================================================================
// TRAINER/VENDOR LOOKUP
// ============================================================================

RidingTrainerInfo const* RidingManager::FindNearestTrainer(RidingSkillLevel skillLevel) const
{
    if (!_bot || !_bot->IsInWorld())
        return nullptr;

    // Get appropriate trainer list based on faction
    bool isAlliance = (_bot->GetTeam() == ALLIANCE);
    std::vector<RidingTrainerInfo> const& trainers = isAlliance ? _allianceTrainers : _hordeTrainers;

    if (trainers.empty())
        return nullptr;

    // Find nearest trainer that can teach the requested skill
    float minDistance = std::numeric_limits<float>::max();
    RidingTrainerInfo const* nearest = nullptr;

    for (auto const& trainer : trainers)
    {
        // Check if trainer teaches this skill level
        if (trainer.maxSkill < skillLevel)
            continue;

        // Check if trainer is on same map or calculate cross-map distance
        float distance;
        if (trainer.mapId == _bot->GetMapId())
        {
            distance = _bot->GetDistance(trainer.x, trainer.y, trainer.z);
        }
        else
        {
            // Cross-continent - estimate large distance but still comparable
            distance = 100000.0f + std::abs(static_cast<float>(trainer.mapId) - static_cast<float>(_bot->GetMapId())) * 10000.0f;
        }

        if (distance < minDistance)
        {
            minDistance = distance;
            nearest = &trainer;
        }
    }

    // Also check neutral trainers
    for (auto const& trainer : _neutralTrainers)
    {
        if (trainer.maxSkill < skillLevel)
            continue;

        float distance;
        if (trainer.mapId == _bot->GetMapId())
        {
            distance = _bot->GetDistance(trainer.x, trainer.y, trainer.z);
        }
        else
        {
            distance = 100000.0f + std::abs(static_cast<float>(trainer.mapId) - static_cast<float>(_bot->GetMapId())) * 10000.0f;
        }

        if (distance < minDistance)
        {
            minDistance = distance;
            nearest = &trainer;
        }
    }

    return nearest;
}

MountVendorInfo const* RidingManager::FindNearestMountVendor() const
{
    if (!_bot || !_bot->IsInWorld())
        return nullptr;

    bool isAlliance = (_bot->GetTeam() == ALLIANCE);
    std::vector<MountVendorInfo> const& vendors = isAlliance ? _allianceVendors : _hordeVendors;

    if (vendors.empty())
        return nullptr;

    // Find nearest vendor, prefer same race
    float minDistance = std::numeric_limits<float>::max();
    MountVendorInfo const* nearest = nullptr;

    uint32 botRace = _bot->GetRace();

    for (auto const& vendor : vendors)
    {
        // Prefer vendors of same race
        float racePenalty = (vendor.race != botRace && vendor.race != 0) ? 10000.0f : 0.0f;

        float distance;
        if (vendor.mapId == _bot->GetMapId())
        {
            distance = _bot->GetDistance(vendor.x, vendor.y, vendor.z) + racePenalty;
        }
        else
        {
            distance = 100000.0f + racePenalty + std::abs(static_cast<float>(vendor.mapId) - static_cast<float>(_bot->GetMapId())) * 10000.0f;
        }

        if (distance < minDistance)
        {
            minDistance = distance;
            nearest = &vendor;
        }
    }

    return nearest;
}

std::vector<RidingTrainerInfo> RidingManager::FindAllTrainers() const
{
    std::vector<RidingTrainerInfo> result;

    if (!_bot)
        return result;

    bool isAlliance = (_bot->GetTeam() == ALLIANCE);
    std::vector<RidingTrainerInfo> const& factionTrainers = isAlliance ? _allianceTrainers : _hordeTrainers;

    result.insert(result.end(), factionTrainers.begin(), factionTrainers.end());
    result.insert(result.end(), _neutralTrainers.begin(), _neutralTrainers.end());

    return result;
}

std::vector<MountVendorInfo> RidingManager::FindAllMountVendors() const
{
    if (!_bot)
        return {};

    bool isAlliance = (_bot->GetTeam() == ALLIANCE);
    return isAlliance ? _allianceVendors : _hordeVendors;
}

// ============================================================================
// ACQUISITION STATE MACHINE
// ============================================================================

RidingAcquisitionState RidingManager::GetAcquisitionState() const
{
    return _state;
}

bool RidingManager::StartRidingAcquisition(RidingSkillLevel skillLevel)
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    // Determine skill level to acquire
    if (skillLevel == RidingSkillLevel::NONE)
    {
        skillLevel = GetNextSkillLevel();
    }

    if (skillLevel == RidingSkillLevel::NONE)
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::StartRidingAcquisition - Bot {} doesn't need riding skill",
            _bot->GetName());
        return false;
    }

    // Check affordability
    if (!CanAffordNextSkill())
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::StartRidingAcquisition - Bot {} cannot afford skill {}",
            _bot->GetName(), static_cast<uint32>(skillLevel));
        return false;
    }

    _targetSkill = skillLevel;
    TransitionTo(RidingAcquisitionState::NEED_RIDING_SKILL);

    TC_LOG_INFO("module.playerbot", "RidingManager::StartRidingAcquisition - Bot {} starting acquisition of skill level {}",
        _bot->GetName(), static_cast<uint32>(skillLevel));

    return true;
}

bool RidingManager::StartMountAcquisition()
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    if (!NeedsMount())
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::StartMountAcquisition - Bot {} doesn't need mount",
            _bot->GetName());
        return false;
    }

    if (!CanAffordMount())
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::StartMountAcquisition - Bot {} cannot afford mount",
            _bot->GetName());
        return false;
    }

    TransitionTo(RidingAcquisitionState::NEED_MOUNT);

    TC_LOG_INFO("module.playerbot", "RidingManager::StartMountAcquisition - Bot {} starting mount acquisition",
        _bot->GetName());

    return true;
}

void RidingManager::CancelAcquisition()
{
    if (_state != RidingAcquisitionState::IDLE)
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::CancelAcquisition - Bot {} cancelled acquisition in state {}",
            _bot->GetName(), static_cast<uint8>(_state));
    }

    TransitionTo(RidingAcquisitionState::IDLE);
}

bool RidingManager::IsAcquiring() const
{
    return _state != RidingAcquisitionState::IDLE &&
           _state != RidingAcquisitionState::COMPLETE &&
           _state != RidingAcquisitionState::FAILED;
}

// ============================================================================
// INSTANT LEARNING
// ============================================================================

bool RidingManager::InstantLearnRiding(RidingSkillLevel skillLevel)
{
    if (!_bot)
        return false;

    uint32 spellId = GetSpellIdForSkill(skillLevel);
    if (spellId == 0)
        return false;

    if (_bot->HasSpell(spellId))
        return true; // Already knows

    _bot->LearnSpell(spellId, false);

    _metrics.skillsLearned++;
    _globalMetrics.skillsLearned++;

    TC_LOG_INFO("module.playerbot", "RidingManager::InstantLearnRiding - Bot {} instantly learned skill {} (spell {})",
        _bot->GetName(), static_cast<uint32>(skillLevel), spellId);

    return true;
}

bool RidingManager::InstantLearnMount(uint32 mountSpellId)
{
    if (!_bot)
        return false;

    if (mountSpellId == 0)
    {
        // Auto-select appropriate mount
        mountSpellId = GetRaceAppropriateMount(_bot->GetLevel());
    }

    if (mountSpellId == 0)
        return false;

    if (_bot->HasSpell(mountSpellId))
        return true; // Already knows

    _bot->LearnSpell(mountSpellId, false);

    _metrics.mountsPurchased++;
    _globalMetrics.mountsPurchased++;

    TC_LOG_INFO("module.playerbot", "RidingManager::InstantLearnMount - Bot {} instantly learned mount spell {}",
        _bot->GetName(), mountSpellId);

    return true;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void RidingManager::SetAutoAcquireEnabled(bool enabled)
{
    _autoAcquireEnabled = enabled;
}

bool RidingManager::IsAutoAcquireEnabled() const
{
    return _autoAcquireEnabled;
}

void RidingManager::SetMinReserveGold(uint64 goldCopper)
{
    _minReserveGold = goldCopper;
}

uint64 RidingManager::GetMinReserveGold() const
{
    return _minReserveGold;
}

// ============================================================================
// DATABASE INITIALIZATION
// ============================================================================

void RidingManager::InitializeDatabase()
{
    if (_databaseInitialized)
        return;

    TC_LOG_INFO("module.playerbot", "RidingManager::InitializeDatabase - Initializing riding trainer/vendor database");

    InitializeAllianceTrainers();
    InitializeHordeTrainers();
    InitializeNeutralTrainers();
    InitializeAllianceVendors();
    InitializeHordeVendors();

    _databaseInitialized = true;

    TC_LOG_INFO("module.playerbot", "RidingManager::InitializeDatabase - Loaded {} Alliance trainers, {} Horde trainers, {} neutral trainers",
        _allianceTrainers.size(), _hordeTrainers.size(), _neutralTrainers.size());
    TC_LOG_INFO("module.playerbot", "RidingManager::InitializeDatabase - Loaded {} Alliance vendors, {} Horde vendors",
        _allianceVendors.size(), _hordeVendors.size());
}

void RidingManager::InitializeAllianceTrainers()
{
    // Stormwind Riding Trainer - Randal Hunter
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 4752;
        trainer.mapId = 0; // Eastern Kingdoms
        trainer.x = -9442.0f;
        trainer.y = 72.0f;
        trainer.z = 57.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 469; // Alliance
        trainer.race = 1; // Human
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _allianceTrainers.push_back(trainer);
    }

    // Darnassus Riding Trainer - Jartsam
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 4753;
        trainer.mapId = 1; // Kalimdor
        trainer.x = 10177.0f;
        trainer.y = 2634.0f;
        trainer.z = 1330.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 469;
        trainer.race = 4; // Night Elf
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _allianceTrainers.push_back(trainer);
    }

    // Ironforge Riding Trainer - Ultham Ironhorn
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 4772;
        trainer.mapId = 0;
        trainer.x = -5520.0f;
        trainer.y = -1375.0f;
        trainer.z = 399.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 469;
        trainer.race = 3; // Dwarf
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _allianceTrainers.push_back(trainer);
    }

    // Exodar Riding Trainer - Aalun
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 17483;
        trainer.mapId = 530; // Outland/Exodar
        trainer.x = -4199.0f;
        trainer.y = -12479.0f;
        trainer.z = 45.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 469;
        trainer.race = 11; // Draenei
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _allianceTrainers.push_back(trainer);
    }

    // Gnomeregan Riding Trainer - Binjy Featherwhistle
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 7954;
        trainer.mapId = 0;
        trainer.x = -5408.0f;
        trainer.y = -638.0f;
        trainer.z = 393.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 469;
        trainer.race = 7; // Gnome
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _allianceTrainers.push_back(trainer);
    }
}

void RidingManager::InitializeHordeTrainers()
{
    // Orgrimmar Riding Trainer - Kildar
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 4752; // Using placeholder, actual entry may differ
        trainer.mapId = 1;
        trainer.x = 2132.0f;
        trainer.y = -4738.0f;
        trainer.z = 100.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 67; // Horde
        trainer.race = 2; // Orc
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _hordeTrainers.push_back(trainer);
    }

    // Thunder Bluff Riding Trainer - Kar Stormsinger
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 3690;
        trainer.mapId = 1;
        trainer.x = -1231.0f;
        trainer.y = 133.0f;
        trainer.z = 134.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 67;
        trainer.race = 6; // Tauren
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _hordeTrainers.push_back(trainer);
    }

    // Undercity Riding Trainer - Velma Warnam
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 4773;
        trainer.mapId = 0;
        trainer.x = 2310.0f;
        trainer.y = 276.0f;
        trainer.z = 35.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 67;
        trainer.race = 5; // Undead
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _hordeTrainers.push_back(trainer);
    }

    // Silvermoon Riding Trainer - Perascamin
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 16280;
        trainer.mapId = 530;
        trainer.x = 9295.0f;
        trainer.y = -7225.0f;
        trainer.z = 14.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 67;
        trainer.race = 10; // Blood Elf
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _hordeTrainers.push_back(trainer);
    }

    // Echo Isles Riding Trainer (Troll)
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 7953;
        trainer.mapId = 1;
        trainer.x = -1200.0f;
        trainer.y = -5449.0f;
        trainer.z = 15.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 67;
        trainer.race = 8; // Troll
        trainer.maxSkill = RidingSkillLevel::ARTISAN;
        trainer.goldCostCopper = COST_APPRENTICE;
        _hordeTrainers.push_back(trainer);
    }
}

void RidingManager::InitializeNeutralTrainers()
{
    // Dalaran Flying Trainer (Northrend) - Hira Snowdawn
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 28746;
        trainer.mapId = 571; // Northrend
        trainer.x = 5815.0f;
        trainer.y = 448.0f;
        trainer.z = 659.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 0; // Neutral
        trainer.race = 0; // All
        trainer.maxSkill = RidingSkillLevel::COLD_WEATHER;
        trainer.goldCostCopper = COST_EXPERT;
        _neutralTrainers.push_back(trainer);
    }

    // Pandaria Flying Trainer - Skydancer Shun
    {
        RidingTrainerInfo trainer;
        trainer.creatureEntry = 58773;
        trainer.mapId = 870; // Pandaria
        trainer.x = 1440.0f;
        trainer.y = 457.0f;
        trainer.z = 469.0f;
        trainer.orientation = 0.0f;
        trainer.faction = 0;
        trainer.race = 0;
        trainer.maxSkill = RidingSkillLevel::MASTER;
        trainer.goldCostCopper = COST_MASTER;
        _neutralTrainers.push_back(trainer);
    }
}

void RidingManager::InitializeAllianceVendors()
{
    // Stormwind Horse Vendor - Katie Hunter
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 384;
        vendor.mapId = 0;
        vendor.x = -9441.0f;
        vendor.y = 72.0f;
        vendor.z = 57.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 469;
        vendor.race = 1; // Human
        vendor.mountSpellId = 458; // Brown Horse
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _allianceVendors.push_back(vendor);
    }

    // Darnassus Saber Vendor - Lelanai
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 4730;
        vendor.mapId = 1;
        vendor.x = 10176.0f;
        vendor.y = 2634.0f;
        vendor.z = 1330.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 469;
        vendor.race = 4; // Night Elf
        vendor.mountSpellId = 10789; // Spotted Frostsaber
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _allianceVendors.push_back(vendor);
    }

    // Ironforge Ram Vendor - Veron Amberstill
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 1261;
        vendor.mapId = 0;
        vendor.x = -5520.0f;
        vendor.y = -1376.0f;
        vendor.z = 399.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 469;
        vendor.race = 3; // Dwarf
        vendor.mountSpellId = 6898; // Brown Ram
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _allianceVendors.push_back(vendor);
    }

    // Exodar Elekk Vendor - Torallius the Pack Handler
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 17584;
        vendor.mapId = 530;
        vendor.x = -4196.0f;
        vendor.y = -12478.0f;
        vendor.z = 45.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 469;
        vendor.race = 11; // Draenei
        vendor.mountSpellId = 34406; // Brown Elekk
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _allianceVendors.push_back(vendor);
    }

    // Gnomeregan Mechanostrider Vendor - Milli Featherwhistle
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 7955;
        vendor.mapId = 0;
        vendor.x = -5412.0f;
        vendor.y = -637.0f;
        vendor.z = 393.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 469;
        vendor.race = 7; // Gnome
        vendor.mountSpellId = 10873; // Red Mechanostrider
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _allianceVendors.push_back(vendor);
    }
}

void RidingManager::InitializeHordeVendors()
{
    // Orgrimmar Wolf Vendor - Ogunaro Wolfrunner
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 3362;
        vendor.mapId = 1;
        vendor.x = 2131.0f;
        vendor.y = -4737.0f;
        vendor.z = 100.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 67;
        vendor.race = 2; // Orc
        vendor.mountSpellId = 580; // Timber Wolf
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _hordeVendors.push_back(vendor);
    }

    // Thunder Bluff Kodo Vendor - Harb Clawhoof
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 3685;
        vendor.mapId = 1;
        vendor.x = -1232.0f;
        vendor.y = 132.0f;
        vendor.z = 134.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 67;
        vendor.race = 6; // Tauren
        vendor.mountSpellId = 18989; // Gray Kodo
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _hordeVendors.push_back(vendor);
    }

    // Undercity Skeletal Horse Vendor - Zachariah Post
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 4731;
        vendor.mapId = 0;
        vendor.x = 2309.0f;
        vendor.y = 277.0f;
        vendor.z = 35.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 67;
        vendor.race = 5; // Undead
        vendor.mountSpellId = 64977; // Black Skeletal Horse
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _hordeVendors.push_back(vendor);
    }

    // Silvermoon Hawkstrider Vendor - Winaestra
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 16264;
        vendor.mapId = 530;
        vendor.x = 9295.0f;
        vendor.y = -7224.0f;
        vendor.z = 14.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 67;
        vendor.race = 10; // Blood Elf
        vendor.mountSpellId = 35020; // Blue Hawkstrider
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _hordeVendors.push_back(vendor);
    }

    // Raptor Vendor - Zjolnir
    {
        MountVendorInfo vendor;
        vendor.creatureEntry = 7952;
        vendor.mapId = 1;
        vendor.x = -1197.0f;
        vendor.y = -5447.0f;
        vendor.z = 15.0f;
        vendor.orientation = 0.0f;
        vendor.faction = 67;
        vendor.race = 8; // Troll
        vendor.mountSpellId = 8395; // Emerald Raptor
        vendor.goldCostCopper = COST_MOUNT_BASIC;
        _hordeVendors.push_back(vendor);
    }
}

// ============================================================================
// STATE MACHINE HELPERS
// ============================================================================

void RidingManager::ProcessNeedRidingSkill()
{
    // Find nearest trainer
    RidingTrainerInfo const* trainer = FindNearestTrainer(_targetSkill);
    if (!trainer)
    {
        TC_LOG_WARN("module.playerbot", "RidingManager::ProcessNeedRidingSkill - Bot {} could not find trainer for skill {}",
            _bot->GetName(), static_cast<uint32>(_targetSkill));
        TransitionTo(RidingAcquisitionState::FAILED);
        return;
    }

    _targetTrainer = *trainer;
    _targetX = trainer->x;
    _targetY = trainer->y;
    _targetZ = trainer->z;
    _targetMapId = trainer->mapId;

    if (StartTravelTo(trainer->mapId, trainer->x, trainer->y, trainer->z))
    {
        TransitionTo(RidingAcquisitionState::TRAVELING_TO_TRAINER);
    }
    else
    {
        TC_LOG_WARN("module.playerbot", "RidingManager::ProcessNeedRidingSkill - Bot {} failed to start travel to trainer",
            _bot->GetName());
        TransitionTo(RidingAcquisitionState::FAILED);
    }
}

void RidingManager::ProcessTravelingToTrainer()
{
    if (HasArrivedAtDestination())
    {
        TransitionTo(RidingAcquisitionState::AT_TRAINER);
        _metrics.trainerVisits++;
        _globalMetrics.trainerVisits++;
    }
    // Otherwise, continue waiting for movement to complete
}

void RidingManager::ProcessAtTrainer()
{
    // Wait for interaction delay to simulate realistic behavior
    _interactionTimer += UPDATE_INTERVAL;
    if (_interactionTimer < INTERACTION_DELAY)
        return;

    _interactionTimer = 0;

    Creature* trainer = FindTrainerNPC();
    if (!trainer)
    {
        TC_LOG_WARN("module.playerbot", "RidingManager::ProcessAtTrainer - Bot {} could not find trainer NPC",
            _bot->GetName());
        TransitionTo(RidingAcquisitionState::FAILED);
        return;
    }

    if (InteractWithTrainer(trainer))
    {
        TransitionTo(RidingAcquisitionState::LEARNING_SKILL);
    }
    else
    {
        TC_LOG_WARN("module.playerbot", "RidingManager::ProcessAtTrainer - Bot {} failed to interact with trainer",
            _bot->GetName());
        TransitionTo(RidingAcquisitionState::FAILED);
    }
}

void RidingManager::ProcessLearningSkill()
{
    // Learning is instant via InteractWithTrainer
    // Check if we need a mount too
    if (NeedsMount() && CanAffordMount())
    {
        TransitionTo(RidingAcquisitionState::NEED_MOUNT);
    }
    else
    {
        TransitionTo(RidingAcquisitionState::COMPLETE);
    }
}

void RidingManager::ProcessNeedMount()
{
    // Find nearest mount vendor
    MountVendorInfo const* vendor = FindNearestMountVendor();
    if (!vendor)
    {
        TC_LOG_WARN("module.playerbot", "RidingManager::ProcessNeedMount - Bot {} could not find mount vendor",
            _bot->GetName());
        TransitionTo(RidingAcquisitionState::FAILED);
        return;
    }

    _targetVendor = *vendor;
    _targetX = vendor->x;
    _targetY = vendor->y;
    _targetZ = vendor->z;
    _targetMapId = vendor->mapId;

    if (StartTravelTo(vendor->mapId, vendor->x, vendor->y, vendor->z))
    {
        TransitionTo(RidingAcquisitionState::TRAVELING_TO_VENDOR);
    }
    else
    {
        TC_LOG_WARN("module.playerbot", "RidingManager::ProcessNeedMount - Bot {} failed to start travel to vendor",
            _bot->GetName());
        TransitionTo(RidingAcquisitionState::FAILED);
    }
}

void RidingManager::ProcessTravelingToVendor()
{
    if (HasArrivedAtDestination())
    {
        TransitionTo(RidingAcquisitionState::AT_VENDOR);
        _metrics.vendorVisits++;
        _globalMetrics.vendorVisits++;
    }
}

void RidingManager::ProcessAtVendor()
{
    // Wait for interaction delay
    _interactionTimer += UPDATE_INTERVAL;
    if (_interactionTimer < INTERACTION_DELAY)
        return;

    _interactionTimer = 0;

    Creature* vendor = FindVendorNPC();
    if (!vendor)
    {
        TC_LOG_WARN("module.playerbot", "RidingManager::ProcessAtVendor - Bot {} could not find vendor NPC",
            _bot->GetName());
        TransitionTo(RidingAcquisitionState::FAILED);
        return;
    }

    if (InteractWithVendor(vendor))
    {
        TransitionTo(RidingAcquisitionState::PURCHASING_MOUNT);
    }
    else
    {
        TC_LOG_WARN("module.playerbot", "RidingManager::ProcessAtVendor - Bot {} failed to interact with vendor",
            _bot->GetName());
        TransitionTo(RidingAcquisitionState::FAILED);
    }
}

void RidingManager::ProcessPurchasingMount()
{
    // Purchase is instant via InteractWithVendor
    TransitionTo(RidingAcquisitionState::COMPLETE);
}

void RidingManager::TransitionTo(RidingAcquisitionState newState)
{
    TC_LOG_DEBUG("module.playerbot", "RidingManager::TransitionTo - Bot {} transitioning from {} to {}",
        _bot->GetName(), static_cast<uint8>(_state), static_cast<uint8>(newState));

    _state = newState;
    _stateTimer = 0;
    _interactionTimer = 0;
}

// ============================================================================
// SKILL HELPERS
// ============================================================================

uint32 RidingManager::GetSpellIdForSkill(RidingSkillLevel skillLevel)
{
    switch (skillLevel)
    {
        case RidingSkillLevel::APPRENTICE:    return SPELL_APPRENTICE_RIDING;
        case RidingSkillLevel::JOURNEYMAN:    return SPELL_JOURNEYMAN_RIDING;
        case RidingSkillLevel::EXPERT:        return SPELL_EXPERT_RIDING;
        case RidingSkillLevel::ARTISAN:       return SPELL_ARTISAN_RIDING;
        case RidingSkillLevel::MASTER:        return SPELL_MASTER_RIDING;
        case RidingSkillLevel::COLD_WEATHER:  return SPELL_COLD_WEATHER_FLYING;
        case RidingSkillLevel::FLIGHT_MASTERS: return SPELL_FLIGHT_MASTERS_LICENSE;
        default: return 0;
    }
}

uint64 RidingManager::GetSkillCost(RidingSkillLevel skillLevel)
{
    switch (skillLevel)
    {
        case RidingSkillLevel::APPRENTICE:   return COST_APPRENTICE;
        case RidingSkillLevel::JOURNEYMAN:   return COST_JOURNEYMAN;
        case RidingSkillLevel::EXPERT:       return COST_EXPERT;
        case RidingSkillLevel::ARTISAN:      return COST_ARTISAN;
        case RidingSkillLevel::MASTER:       return COST_MASTER;
        default: return 0;
    }
}

uint32 RidingManager::GetLevelRequirement(RidingSkillLevel skillLevel)
{
    switch (skillLevel)
    {
        case RidingSkillLevel::APPRENTICE:   return 10;
        case RidingSkillLevel::JOURNEYMAN:   return 20;
        case RidingSkillLevel::EXPERT:       return 30;
        case RidingSkillLevel::ARTISAN:      return 40;
        case RidingSkillLevel::MASTER:       return 80;
        case RidingSkillLevel::COLD_WEATHER: return 68;
        default: return 0;
    }
}

bool RidingManager::HasSkill(RidingSkillLevel skillLevel) const
{
    uint32 spellId = GetSpellIdForSkill(skillLevel);
    return spellId != 0 && _bot && _bot->HasSpell(spellId);
}

// ============================================================================
// TRAVEL HELPERS
// ============================================================================

bool RidingManager::StartTravelTo(uint32 mapId, float x, float y, float z)
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    // For now, if on different map, teleport directly (simplification)
    // In full implementation, use flight paths/portals
    if (mapId != _bot->GetMapId())
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::StartTravelTo - Bot {} needs cross-map travel to map {}",
            _bot->GetName(), mapId);
        // For humanization, would use flight master system
        // For now, mark as failed to avoid instant teleport
        return false;
    }

    // Request movement via movement coordinator
    // In full implementation, integrate with UnifiedMovementCoordinator
    // For now, just move directly

    TC_LOG_DEBUG("module.playerbot", "RidingManager::StartTravelTo - Bot {} starting travel to ({}, {}, {})",
        _bot->GetName(), x, y, z);

    return true;
}

bool RidingManager::HasArrivedAtDestination() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    if (_bot->GetMapId() != _targetMapId)
        return false;

    float distance = _bot->GetDistance(_targetX, _targetY, _targetZ);
    return distance < ARRIVAL_THRESHOLD;
}

float RidingManager::GetDistanceToTarget() const
{
    if (!_bot || !_bot->IsInWorld())
        return std::numeric_limits<float>::max();

    if (_bot->GetMapId() != _targetMapId)
        return std::numeric_limits<float>::max();

    return _bot->GetDistance(_targetX, _targetY, _targetZ);
}

// ============================================================================
// NPC INTERACTION HELPERS
// ============================================================================

Creature* RidingManager::FindTrainerNPC() const
{
    if (!_bot || !_bot->IsInWorld())
        return nullptr;

    // Find creature by entry near the target position
    Map* map = _bot->GetMap();
    if (!map)
        return nullptr;

    // Search for the trainer creature
    Creature* trainer = nullptr;
    float minDist = INTERACTION_RANGE * 2;

    // Simple search in nearby area
    // In full implementation, use proper grid search
    for (auto& ref : map->GetPlayers())
    {
        // This is a placeholder - should iterate creatures instead
    }

    // Fallback: try to get creature by entry
    // This is simplified - in production, use proper search
    trainer = _bot->FindNearestCreature(_targetTrainer.creatureEntry, INTERACTION_RANGE * 2);

    return trainer;
}

Creature* RidingManager::FindVendorNPC() const
{
    if (!_bot || !_bot->IsInWorld())
        return nullptr;

    return _bot->FindNearestCreature(_targetVendor.creatureEntry, INTERACTION_RANGE * 2);
}

bool RidingManager::InteractWithTrainer(Creature* trainer)
{
    if (!_bot || !trainer)
        return false;

    // Check distance
    if (_bot->GetDistance(trainer) > INTERACTION_RANGE)
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::InteractWithTrainer - Bot {} too far from trainer",
            _bot->GetName());
        return false;
    }

    // Get skill cost
    uint64 cost = GetSkillCost(_targetSkill);
    if (_bot->GetMoney() < cost)
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::InteractWithTrainer - Bot {} doesn't have enough gold",
            _bot->GetName());
        return false;
    }

    // Learn the skill
    uint32 spellId = GetSpellIdForSkill(_targetSkill);
    if (spellId == 0)
        return false;

    if (_bot->HasSpell(spellId))
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::InteractWithTrainer - Bot {} already has skill",
            _bot->GetName());
        return true;
    }

    // Deduct gold
    _bot->ModifyMoney(-static_cast<int64>(cost));

    // Learn spell
    _bot->LearnSpell(spellId, false);

    // Update metrics
    _metrics.skillsLearned++;
    _metrics.goldSpent += cost;
    _globalMetrics.skillsLearned++;
    _globalMetrics.goldSpent += cost;

    TC_LOG_INFO("module.playerbot", "RidingManager::InteractWithTrainer - Bot {} learned riding skill {} (spell {}) for {} gold",
        _bot->GetName(), static_cast<uint32>(_targetSkill), spellId, cost / 10000);

    return true;
}

bool RidingManager::InteractWithVendor(Creature* vendor)
{
    if (!_bot || !vendor)
        return false;

    // Check distance
    if (_bot->GetDistance(vendor) > INTERACTION_RANGE)
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::InteractWithVendor - Bot {} too far from vendor",
            _bot->GetName());
        return false;
    }

    // Get appropriate mount for bot's race
    uint32 mountSpellId = GetRaceAppropriateMount(_bot->GetLevel());
    if (mountSpellId == 0)
    {
        // Fallback to vendor's mount
        mountSpellId = _targetVendor.mountSpellId;
    }

    if (_bot->HasSpell(mountSpellId))
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::InteractWithVendor - Bot {} already has mount",
            _bot->GetName());
        return true;
    }

    // Get cost
    uint64 cost = GetMountCost(_bot->GetLevel());
    if (_bot->GetMoney() < cost)
    {
        TC_LOG_DEBUG("module.playerbot", "RidingManager::InteractWithVendor - Bot {} doesn't have enough gold for mount",
            _bot->GetName());
        return false;
    }

    // Deduct gold
    _bot->ModifyMoney(-static_cast<int64>(cost));

    // Learn mount
    _bot->LearnSpell(mountSpellId, false);

    // Update metrics
    _metrics.mountsPurchased++;
    _metrics.goldSpent += cost;
    _globalMetrics.mountsPurchased++;
    _globalMetrics.goldSpent += cost;

    TC_LOG_INFO("module.playerbot", "RidingManager::InteractWithVendor - Bot {} purchased mount (spell {}) for {} gold",
        _bot->GetName(), mountSpellId, cost / 10000);

    return true;
}

// ============================================================================
// MOUNT SELECTION HELPERS
// ============================================================================

uint32 RidingManager::GetRaceAppropriateMount(uint32 level) const
{
    if (!_bot)
        return 0;

    uint32 race = _bot->GetRace();
    bool isEpic = (level >= 40);

    // Return race-appropriate mount spell IDs
    switch (race)
    {
        case 1: // Human
            return isEpic ? 23228 : 458;   // Brown Horse / Swift Brown Horse
        case 3: // Dwarf
            return isEpic ? 23238 : 6898;  // Brown Ram / Swift Brown Ram
        case 4: // Night Elf
            return isEpic ? 23221 : 10789; // Spotted Frostsaber / Swift Mistsaber
        case 7: // Gnome
            return isEpic ? 23223 : 10873; // Red Mechanostrider / Swift White Mechanostrider
        case 11: // Draenei
            return isEpic ? 35712 : 34406; // Brown Elekk / Great Blue Elekk
        case 22: // Worgen
            return 87840; // Running Wild (innate mount ability)
        case 2: // Orc
            return isEpic ? 23250 : 580;   // Timber Wolf / Swift Timber Wolf
        case 5: // Undead
            return isEpic ? 17462 : 64977; // Black Skeletal Horse / Red Skeletal Horse
        case 6: // Tauren
            return isEpic ? 23249 : 18989; // Gray Kodo / Great Gray Kodo
        case 8: // Troll
            return isEpic ? 23243 : 8395;  // Emerald Raptor / Swift Olive Raptor
        case 10: // Blood Elf
            return isEpic ? 35025 : 35020; // Blue Hawkstrider / Swift Pink Hawkstrider
        case 9: // Goblin
            return isEpic ? 87091 : 87090; // Goblin Turbo-Trike / Goblin Trike
        case 24: // Pandaren (Alliance)
        case 25: // Pandaren (Horde)
        case 26: // Pandaren (Neutral)
            return isEpic ? 118091 : 118089; // Green Dragon Turtle / Great Green Dragon Turtle
        default:
            // Fallback to generic mount
            return isEpic ? 23228 : 458;
    }
}

uint64 RidingManager::GetMountCost(uint32 level)
{
    if (level >= 40)
        return COST_MOUNT_EPIC;
    return COST_MOUNT_BASIC;
}

} // namespace Playerbot
