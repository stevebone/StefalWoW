/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "EncounterStrategy.h"
#include "SpellHistory.h"
#include "GameTime.h"
#include "DungeonBehavior.h"
#include "Player.h"
#include "Group.h"
#include "Unit.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "MotionMaster.h"
#include "Spell.h"
#include "SpellAuras.h"  // For Aura and AuraApplication
#include "DynamicObject.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "DBCEnums.h"  // For ChrSpecialization
#include <algorithm>
#include <cmath>
#include "../Spatial/SpatialGridManager.h"
#include "../Spatial/DoubleBufferedSpatialGrid.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "../Movement/Arbiter/MovementPriorityMapper.h"
#include "../AI/BotAI.h"
#include "UnitAI.h"
#include "../Core/PlayerBotHelpers.h"

namespace Playerbot
{

// ============================================================================
// SPECIALIZATION HELPERS
// ============================================================================

/**
 * Checks if a player has a tank specialization.
 */
static bool IsTankSpec(Player* player)
{
    if (!player)
        return false;

    ChrSpecialization spec = player->GetPrimarySpecialization();
    switch (spec)
    {
        case ChrSpecialization::WarriorProtection:
        case ChrSpecialization::PaladinProtection:
        case ChrSpecialization::DeathKnightBlood:
        case ChrSpecialization::DruidGuardian:
        case ChrSpecialization::MonkBrewmaster:
        case ChrSpecialization::DemonHunterVengeance:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if a player has a healer specialization.
 */
static bool IsHealerSpec(Player* player)
{
    if (!player)
        return false;

    ChrSpecialization spec = player->GetPrimarySpecialization();
    switch (spec)
    {
        case ChrSpecialization::PriestDiscipline:
        case ChrSpecialization::PriestHoly:
        case ChrSpecialization::PaladinHoly:
        case ChrSpecialization::DruidRestoration:
        case ChrSpecialization::ShamanRestoration:
        case ChrSpecialization::MonkMistweaver:
        case ChrSpecialization::EvokerPreservation:
            return true;
        default:
            return false;
    }
}

// Singleton instance
EncounterStrategy* EncounterStrategy::instance()
{
    static EncounterStrategy instance;
    return &instance;
}

EncounterStrategy::EncounterStrategy()
    : _adaptiveStrategiesEnabled(true)
    , _mechanicResponseTime(DEFAULT_MECHANIC_RESPONSE_TIME)
    , _strategyComplexity(0.7f)
{
    TC_LOG_INFO("server.loading", "Initializing EncounterStrategy system...");
    InitializeStrategyDatabase();
    TC_LOG_INFO("server.loading", "EncounterStrategy system initialized with {} encounter strategies", _tankStrategies.size());
}

// ============================================================================
// CORE STRATEGY MANAGEMENT
// ============================================================================

void EncounterStrategy::ExecuteEncounterStrategy(Group* group, uint32 encounterId)
{

    ::std::lock_guard lock(_strategyMutex);

    TC_LOG_INFO("module.playerbot", "Executing encounter strategy for encounter {} (group {})",
        encounterId, group->GetGUID().GetCounter());

    // Update metrics - use emplace to default-construct if not present
    auto [iter, inserted] = _encounterMetrics.emplace(std::piecewise_construct,
        std::forward_as_tuple(encounterId), std::forward_as_tuple());
    iter->second.strategiesExecuted++;
    _globalMetrics.strategiesExecuted++;

    // Get dungeon encounter data
    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);

    // Adapt strategy to group composition
    AdaptStrategyToGroupComposition(group, encounterId);

    // Execute role-specific strategies for each group member
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Determine role and execute appropriate strategy
        DungeonRole role = DeterminePlayerRole(player);
        ExecuteRoleStrategy(player, encounterId, role);
    }

    // Coordinate group-wide strategies
    PlanCooldownUsage(group, encounter);
    UpdateEncounterPositioning(group, encounterId);
}

void EncounterStrategy::UpdateEncounterExecution(Group* group, uint32 encounterId, uint32 diff)
{
    if (!group)
        return;

    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);

    // Update positioning
    UpdateEncounterPositioning(group, encounterId);

    // Coordinate cooldowns
    CoordinateGroupCooldowns(group, encounterId);

    // Handle any active mechanics
    for (auto const& mechanic : encounter.mechanics)
    {
        HandleEncounterMechanic(group, encounterId, mechanic);
    }

    // Optimize resource usage
    OptimizeResourceUsage(group, encounterId);
}

void EncounterStrategy::HandleEncounterMechanic(Group* group, uint32 encounterId, const ::std::string& mechanic)
{
    if (!group)
        return;

    ::std::lock_guard lock(_strategyMutex);

    TC_LOG_DEBUG("module.playerbot", "Handling mechanic '{}' for encounter {}", mechanic, encounterId);

    // Update metrics
    _encounterMetrics[encounterId].mechanicsHandled++;
    _globalMetrics.mechanicsHandled++;

    // Find mechanic handler
    auto mechanicsItr = _encounterMechanics.find(encounterId);
    if (mechanicsItr != _encounterMechanics.end())
    {
        for (auto const& encounterMechanic : mechanicsItr->second)
        {
            if (encounterMechanic.mechanicName == mechanic && encounterMechanic.handler)
            {
                encounterMechanic.handler(group, encounterMechanic);

                // Record success for learning
                UpdateLearningData(encounterId, mechanic, true);

                _encounterMetrics[encounterId].mechanicsSuccessful++;
                _globalMetrics.mechanicsSuccessful++;
                return;
            }
        }
    }

    // Generic mechanic handling
    if (mechanic == "tank_swap")
    {
        HandleTankSwapGeneric(group);
    }
    else if (mechanic == "aoe_damage")
    {
        HandleAoEDamageGeneric(group);
    }
    else if (mechanic == "add_spawns")
    {
        HandleAddSpawnsGeneric(group);
    }
    else if (mechanic == "stacking_debuff")
    {
        HandleStackingDebuffGeneric(group);
    }

    TC_LOG_TRACE("module.playerbot", "Mechanic '{}' handled for encounter {}", mechanic, encounterId);
}

void EncounterStrategy::AdaptStrategyToGroupComposition(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    // Analyze group composition
    uint32 tankCount = 0;
    uint32 healerCount = 0;
    uint32 dpsCount = 0;
    uint32 rangedCount = 0;
    uint32 meleeCount = 0;

    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld())
            continue;

        DungeonRole role = DeterminePlayerRole(player);
        switch (role)
        {
            case DungeonRole::TANK:
                tankCount++;
                break;
            case DungeonRole::HEALER:
                healerCount++;
                break;
            case DungeonRole::MELEE_DPS:
                meleeCount++;
                dpsCount++;
                break;
            case DungeonRole::RANGED_DPS:
                rangedCount++;
                dpsCount++;
                break;
            default:
                break;
        }
    }

    // Adjust strategy complexity based on composition
    float complexityAdjustment = 0.0f;

    if (tankCount < 1)
    {
        complexityAdjustment -= 0.2f; // Simpler strategy without proper tank
    }
    if (healerCount < 1)
    {
        complexityAdjustment -= 0.3f; // Much simpler without healer
    }
    if (dpsCount < 3)
    {
        complexityAdjustment -= 0.1f; // Slightly simpler with low DPS
    }

    float adjustedComplexity = ::std::max(0.1f, ::std::min(1.0f, _strategyComplexity.load() + complexityAdjustment));

    TC_LOG_DEBUG("module.playerbot", "Adapted strategy complexity to {} for group {} (T:{} H:{} D:{})",
        adjustedComplexity, group->GetGUID().GetCounter(), tankCount, healerCount, dpsCount);
}

// ============================================================================
// PHASE-BASED ENCOUNTER MANAGEMENT
// ============================================================================

void EncounterStrategy::HandleEncounterPhaseTransition(Group* group, uint32 encounterId, uint32 newPhase)
{
    if (!group)
        return;

    TC_LOG_INFO("module.playerbot", "Group {} transitioning to phase {} for encounter {}",
        group->GetGUID().GetCounter(), newPhase, encounterId);

    // Prepare for phase transition
    PrepareForPhaseTransition(group, encounterId, newPhase);

    // Execute phase-specific strategy
    ExecutePhaseStrategy(group, encounterId, newPhase);

    // Update positioning for new phase
    UpdateEncounterPositioning(group, encounterId);
}

void EncounterStrategy::ExecutePhaseStrategy(Group* group, uint32 encounterId, uint32 phase)
{
    if (!group)
        return;

    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);

    TC_LOG_DEBUG("module.playerbot", "Executing phase {} strategy for encounter {}", phase, encounterId);

    // Phase-specific adjustments
    switch (phase)
    {
        case 1:
            // Phase 1: Initial engagement
            CoordinateGroupCooldowns(group, encounterId);
            break;

        case 2:
            // Phase 2: Often more mechanics
            HandleEmergencyCooldowns(group);
            break;

        case 3:
            // Phase 3: Burn phase
            OptimizeResourceUsage(group, encounterId);
            break;

        default:
            break;
    }
}

void EncounterStrategy::PrepareForPhaseTransition(Group* group, uint32 encounterId, uint32 upcomingPhase)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Preparing for phase transition to phase {} (encounter {})",
        upcomingPhase, encounterId);

    // Ensure group resources are optimal before phase transition
    OptimizeResourceUsage(group, encounterId);

    // Coordinate major cooldowns for upcoming phase
    PlanCooldownUsage(group, DungeonBehavior::instance()->GetEncounterData(encounterId));
}

// ============================================================================
// MECHANIC-SPECIFIC HANDLERS
// ============================================================================

void EncounterStrategy::HandleTankSwapMechanic(Group* group, Player* currentTank, Player* newTank)
{
    if (!group || !currentTank || !newTank)
        return;

    TC_LOG_INFO("module.playerbot", "Executing tank swap: {} -> {}",
        currentTank->GetName(), newTank->GetName());

    // Current tank reduces threat (stop attacking, use threat reduction abilities)
    // New tank taunts and builds threat
    // This would be handled by the tank's AI

    TC_LOG_DEBUG("module.playerbot", "Tank swap coordinated successfully");
}

void EncounterStrategy::HandleStackingDebuffMechanic(Group* group, Player* affectedPlayer)
{
    if (!group || !affectedPlayer)
        return;

    TC_LOG_DEBUG("module.playerbot", "Handling stacking debuff on {}", affectedPlayer->GetName());
    // If player has too many stacks, they should:
    // 1. Use defensive cooldowns
    // 2. Move to a safe position
    // 3. Alert healers for emergency healing
    // This would be handled by player's AI and healer AI
}

void EncounterStrategy::HandleAoEDamageMechanic(Group* group, const Position& dangerZone, float radius)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Handling AoE damage mechanic at ({}, {})",
        dangerZone.GetPositionX(), dangerZone.GetPositionY());

    // All players should move away from danger zone
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        float distance = player->GetExactDist(&dangerZone);
        if (distance < radius)
        {
            // Calculate safe position away from danger
            float angle = dangerZone.GetAbsoluteAngle(player) + static_cast<float>(M_PI); // Opposite direction
            Position safePos = dangerZone;
            safePos.RelocateOffset({::std::cos(angle) * (radius + 5.0f), ::std::sin(angle) * (radius + 5.0f), 0.0f});

            // PHASE 6B: Use Movement Arbiter with DUNGEON_MECHANIC priority (205)
            BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
            {
                bool accepted = botAI->RequestPointMovement(
                    PlayerBotMovementPriority::DUNGEON_MECHANIC,
                    safePos,
                    "AoE avoidance positioning",
                    "EncounterStrategy");

                if (!accepted)
                {
                    TC_LOG_TRACE("playerbot.movement.arbiter",
                        "EncounterStrategy: AoE avoidance rejected for {} - higher priority active",
                        player->GetName());
                }
                else
                {
                    TC_LOG_TRACE("module.playerbot", "Player {} moving to avoid AoE", player->GetName());
                }
            }
            else
            {
                // Use validated pathfinding for AoE avoidance
                if (BotAI* ai = GetBotAI(player))
                {
                    if (!ai->MoveTo(safePos, true))
                    {
                        // FALLBACK: Direct MotionMaster if validation fails
                        player->GetMotionMaster()->MovePoint(0, safePos.GetPositionX(),
                            safePos.GetPositionY(), safePos.GetPositionZ());
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    player->GetMotionMaster()->MovePoint(0, safePos.GetPositionX(),
                        safePos.GetPositionY(), safePos.GetPositionZ());
                }
                TC_LOG_TRACE("module.playerbot", "Player {} moving to avoid AoE", player->GetName());
            }
        }
    }
}

void EncounterStrategy::HandleAddSpawnMechanic(Group* group, const ::std::vector<Unit*>& adds)
{
    if (!group || adds.empty())
        return;

    TC_LOG_DEBUG("module.playerbot", "Handling add spawns ({} adds)", adds.size());

    // DPS should switch to adds
    // Tanks may need to pick up adds
    // This coordination happens through DPS target assignment
}

void EncounterStrategy::HandleChanneledSpellMechanic(Group* group, Unit* caster, uint32 spellId)
{
    if (!group || !caster)
        return;

    TC_LOG_DEBUG("module.playerbot", "Handling channeled spell {} from {}", spellId, caster->GetName());

    // Interrupt rotation should be coordinated
    // Find players with interrupts and coordinate the interrupt
}

void EncounterStrategy::HandleEnrageMechanic(Group* group, Unit* boss, uint32 timeRemaining)
{
    if (!group || !boss)
        return;

    TC_LOG_WARN("module.playerbot", "Boss {} enraging in {} seconds", boss->GetName(), timeRemaining / 1000);

    // Coordinate burst DPS
    // Use all remaining cooldowns
    HandleEmergencyCooldowns(group);
}

// ============================================================================
// ROLE-SPECIFIC STRATEGIES
// ============================================================================

TankStrategy EncounterStrategy::GetTankStrategy(uint32 encounterId, Player* tank)
{
    ::std::lock_guard lock(_strategyMutex);

    auto itr = _tankStrategies.find(encounterId);
    if (itr != _tankStrategies.end())
        return itr->second;

    // Return default tank strategy
    TankStrategy defaultStrategy;
    defaultStrategy.optimalPosition = CalculateTankPosition(encounterId, nullptr);
    defaultStrategy.threatThreshold = 1.5f;
    defaultStrategy.requiresMovement = false;

    return defaultStrategy;
}

HealerStrategy EncounterStrategy::GetHealerStrategy(uint32 encounterId, Player* healer)
{
    ::std::lock_guard lock(_strategyMutex);
    auto itr = _healerStrategies.find(encounterId);
    if (itr != _healerStrategies.end())
        return itr->second;

    // Return default healer strategy
    HealerStrategy defaultStrategy;
    defaultStrategy.safePosition = CalculateHealerPosition(encounterId, nullptr);
    defaultStrategy.healingThreshold = 0.7f; // Heal when below 70%
    defaultStrategy.requiresMovement = false;

    return defaultStrategy;
}

DpsStrategy EncounterStrategy::GetDpsStrategy(uint32 encounterId, Player* dps)
{
    ::std::lock_guard lock(_strategyMutex);

    auto itr = _dpsStrategies.find(encounterId);
    if (itr != _dpsStrategies.end())
        return itr->second;

    // Return default DPS strategy
    DpsStrategy defaultStrategy;
    defaultStrategy.optimalPosition = CalculateDpsPosition(encounterId, nullptr, false);
    defaultStrategy.threatLimit = 0.8f; // Stay below 80% of tank threat
    defaultStrategy.canMoveDuringCast = false;

    return defaultStrategy;
}

// ============================================================================
// POSITIONING AND MOVEMENT
// ============================================================================

void EncounterStrategy::UpdateEncounterPositioning(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);

    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        DungeonRole role = DeterminePlayerRole(player);
        Position optimalPos = CalculateOptimalPosition(player, encounterId, role);

        // Move if too far from optimal position
    if (player->GetExactDist(&optimalPos) > POSITIONING_TOLERANCE * 2.0f)
        {
            // PHASE 6B: Use Movement Arbiter with DUNGEON_MECHANIC priority (205)
            BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
            {
                bool accepted = botAI->RequestPointMovement(
                    PlayerBotMovementPriority::DUNGEON_MECHANIC,
                    optimalPos,
                    "Encounter optimal positioning",
                    "EncounterStrategy");

                if (!accepted)
                {
                    TC_LOG_TRACE("playerbot.movement.arbiter",
                        "EncounterStrategy: Optimal positioning rejected for {} - higher priority active",
                        player->GetName());
                }
            }
            else
            {
                // Use validated pathfinding for optimal positioning
                if (BotAI* ai = GetBotAI(player))
                {
                    if (!ai->MoveTo(optimalPos, true))
                    {
                        // FALLBACK: Direct MotionMaster if validation fails
                        player->GetMotionMaster()->MovePoint(0, optimalPos.GetPositionX(),
                            optimalPos.GetPositionY(), optimalPos.GetPositionZ());
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    player->GetMotionMaster()->MovePoint(0, optimalPos.GetPositionX(),
                        optimalPos.GetPositionY(), optimalPos.GetPositionZ());
                }
            }
        }
    }
}

void EncounterStrategy::HandleMovementMechanic(Group* group, uint32 encounterId, const ::std::string& mechanic)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Handling movement mechanic: {}", mechanic);

    // Update positioning based on mechanic
    UpdateEncounterPositioning(group, encounterId);
}

Position EncounterStrategy::CalculateOptimalPosition(Player* player, uint32 encounterId, DungeonRole role)
{
    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);
    Position optimalPos = encounter.encounterLocation;

    switch (role)
    {
        case DungeonRole::TANK:
            optimalPos = CalculateTankPosition(encounterId, nullptr);
            break;
        case DungeonRole::HEALER:
            optimalPos = CalculateHealerPosition(encounterId, nullptr);
            break;
        case DungeonRole::MELEE_DPS:
            optimalPos = CalculateDpsPosition(encounterId, nullptr, true);
            break;
        case DungeonRole::RANGED_DPS:
            optimalPos = CalculateDpsPosition(encounterId, nullptr, false);
            break;
        default:
            break;
    }

    return optimalPos;
}

void EncounterStrategy::AvoidMechanicAreas(Group* group, const ::std::vector<Position>& dangerAreas)
{
    if (!group || dangerAreas.empty())
        return;

    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        for (Position const& dangerZone : dangerAreas)
        {
            float distance = player->GetExactDist(&dangerZone);
            if (distance < 10.0f)
            {
                // Move away from danger
                float angle = dangerZone.GetAbsoluteAngle(player) + static_cast<float>(M_PI);
                Position safePos = dangerZone;
                safePos.RelocateOffset({::std::cos(angle) * 15.0f, ::std::sin(angle) * 15.0f, 0.0f});

                // PHASE 6B: Use Movement Arbiter with DUNGEON_MECHANIC priority (205)
                BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
                if (botAI && botAI->GetUnifiedMovementCoordinator())
                {
                    bool accepted = botAI->RequestPointMovement(
                        PlayerBotMovementPriority::DUNGEON_MECHANIC,
                        safePos,
                        "Danger zone avoidance",
                        "EncounterStrategy");

                    if (!accepted)
                    {
                        TC_LOG_TRACE("playerbot.movement.arbiter",
                            "EncounterStrategy: Danger zone avoidance rejected for {} - higher priority active",
                            player->GetName());
                    }
                }
                else
                {
                    // Use validated pathfinding for danger zone avoidance
                    if (BotAI* ai = GetBotAI(player))
                    {
                        if (!ai->MoveTo(safePos, true))
                        {
                            // FALLBACK: Direct MotionMaster if validation fails
                            player->GetMotionMaster()->MovePoint(0, safePos.GetPositionX(),
                                safePos.GetPositionY(), safePos.GetPositionZ());
                        }
                    }
                    else
                    {
                        // Non-bot player - use standard movement
                        player->GetMotionMaster()->MovePoint(0, safePos.GetPositionX(),
                            safePos.GetPositionY(), safePos.GetPositionZ());
                    }
                }
            }
        }
    }
}

// ============================================================================
// COOLDOWN AND RESOURCE MANAGEMENT
// ============================================================================

void EncounterStrategy::CoordinateGroupCooldowns(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Coordinating group cooldowns for encounter {}", encounterId);

    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);
    PlanCooldownUsage(group, encounter);
}

void EncounterStrategy::PlanCooldownUsage(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Analyze encounter duration and plan cooldown usage
    // Major cooldowns should be used at optimal times:
    // - Hero/Bloodlust: Often at start or burn phase
    // - Tank CDs: When taking high damage
    // - Healer CDs: When group health is critical
    // - DPS CDs: During burn phases or add spawns

    TC_LOG_TRACE("module.playerbot", "Cooldown plan established for encounter {}", encounter.encounterName);
}

void EncounterStrategy::HandleEmergencyCooldowns(Group* group)
{
    if (!group)
        return;

    TC_LOG_WARN("module.playerbot", "Emergency cooldowns activated for group {}", group->GetGUID().GetCounter());

    // All players should use their major defensive or healing cooldowns
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // This would be handled by each player's AI to use their specific emergency abilities
    }
}

void EncounterStrategy::OptimizeResourceUsage(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    // Monitor mana usage, ensure healers have sufficient mana
    // Optimize DPS resource expenditure based on encounter duration
    // This involves coordination between all role-specific AIs
}

// ============================================================================
// ADAPTIVE STRATEGY SYSTEM
// ============================================================================

void EncounterStrategy::AnalyzeEncounterPerformance(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Analyzing encounter performance for group {} (encounter {})",
        group->GetGUID().GetCounter(), encounterId);

    AnalyzeGroupPerformance(group, encounterId);
}

void EncounterStrategy::AdaptStrategyBasedOnFailures(Group* group, uint32 encounterId)
{
    if (!group || !_adaptiveStrategiesEnabled.load())
        return;

    ::std::lock_guard lock(_strategyMutex);

    auto learningItr = _learningData.find(encounterId);
    if (learningItr == _learningData.end())
        return;

    StrategyLearningData& learning = learningItr->second;
    learning.totalEncountersAttempted++;

    TC_LOG_INFO("module.playerbot", "Adapting strategy for encounter {} based on {} previous attempts",
        encounterId, learning.totalEncountersAttempted);

    // Analyze failure patterns
    OptimizeStrategyBasedOnLearning(encounterId);

    // Adjust strategy complexity if needed
    AdaptStrategyComplexity(encounterId);
}

void EncounterStrategy::LearnFromSuccessfulEncounters(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    ::std::lock_guard lock(_strategyMutex);

    auto learningItr = _learningData.find(encounterId);
    if (learningItr == _learningData.end())
    {
        _learningData[encounterId] = StrategyLearningData();
        learningItr = _learningData.find(encounterId);
    }

    StrategyLearningData& learning = learningItr->second;
    learning.totalEncountersSuccessful++;
    learning.lastLearningUpdate = GameTime::GetGameTimeMS();

    TC_LOG_INFO("module.playerbot", "Learning from successful encounter {} (success rate: {}/{})",
        encounterId, learning.totalEncountersSuccessful, learning.totalEncountersAttempted);

    // Update strategy effectiveness
    float successRate = static_cast<float>(learning.totalEncountersSuccessful) / learning.totalEncountersAttempted;
    uint32 strategyHash = GenerateMechanicHash("current_strategy");
    learning.strategyEffectiveness[strategyHash] = successRate;
}

void EncounterStrategy::AdjustDifficultyRating(uint32 encounterId, float performanceRating)
{
    // This would adjust internal difficulty assessment based on group performance
    TC_LOG_TRACE("module.playerbot", "Adjusted difficulty rating for encounter {} to {}",
        encounterId, performanceRating);
}

// ============================================================================
// ENCOUNTER-SPECIFIC IMPLEMENTATIONS
// ============================================================================

void EncounterStrategy::ExecuteDeadminesStrategies(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Executing Deadmines strategy for encounter {}", encounterId);

    // VanCleef encounter specific logic would go here
    // Phase 1: Add management
    // Phase 2: Ground fire avoidance, positioning changes
}

void EncounterStrategy::ExecuteWailingCavernsStrategies(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Executing Wailing Caverns strategy for encounter {}", encounterId);

    // Mutanus encounter: Sleep dispels, positioning
}

void EncounterStrategy::ExecuteShadowfangKeepStrategies(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Executing Shadowfang Keep strategy for encounter {}", encounterId);

    // Arugal encounter: Teleportation, add management
}

void EncounterStrategy::ExecuteStockadeStrategies(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Executing Stormwind Stockade strategy for encounter {}", encounterId);

    // Hogger encounter: Fear resistance, enrage management
}

void EncounterStrategy::ExecuteRazorfenKraulStrategies(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Executing Razorfen Kraul strategy for encounter {}", encounterId);
}

// ============================================================================
// PERFORMANCE METRICS
// ============================================================================

StrategyMetrics EncounterStrategy::GetStrategyMetrics(uint32 encounterId)
{
    ::std::lock_guard lock(_strategyMutex);

    auto itr = _encounterMetrics.find(encounterId);
    if (itr != _encounterMetrics.end())
        return itr->second.ToSnapshot();

    return StrategyMetrics();  // Return default snapshot
}

StrategyMetrics EncounterStrategy::GetGlobalStrategyMetrics()
{
    return _globalMetrics.ToSnapshot();
}

void EncounterStrategy::SetStrategyComplexity(uint32 encounterId, float complexity)
{
    ::std::lock_guard lock(_strategyMutex);

    // Clamp complexity to valid range
    float clampedComplexity = ::std::max(0.0f, ::std::min(1.0f, complexity));

    TC_LOG_DEBUG("module.playerbot", "Setting strategy complexity for encounter {} to {}",
        encounterId, clampedComplexity);

    // Store per-encounter complexity if needed, otherwise just update global
    _strategyComplexity = clampedComplexity;
}

void EncounterStrategy::CoordinateCooldowns(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    CoordinateGroupCooldowns(group, encounterId);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void EncounterStrategy::InitializeStrategyDatabase()
{
    TC_LOG_INFO("server.loading", "Initializing encounter strategy database...");

    LoadTankStrategies();
    LoadHealerStrategies();
    LoadDpsStrategies();
    LoadEncounterMechanics();
    TC_LOG_INFO("server.loading", "Encounter strategy database initialized");
}

void EncounterStrategy::LoadTankStrategies()
{
    // Load tank strategies for various encounters
    // Example: Deadmines VanCleef
    TankStrategy vancleef;
    vancleef.optimalPosition = Position(0, 0, 0); // Would be actual coordinates
    vancleef.threatThreshold = 1.5f;
    vancleef.requiresMovement = true;
    _tankStrategies[1] = vancleef;

    TC_LOG_DEBUG("server.loading", "Loaded {} tank strategies", _tankStrategies.size());
}

void EncounterStrategy::LoadHealerStrategies()
{
    // Load healer strategies
    HealerStrategy vancleef;
    vancleef.safePosition = Position(0, 0, 0);
    vancleef.healingThreshold = 0.7f;
    vancleef.requiresMovement = false;
    _healerStrategies[1] = vancleef;

    TC_LOG_DEBUG("server.loading", "Loaded {} healer strategies", _healerStrategies.size());
}

void EncounterStrategy::LoadDpsStrategies()
{
    // Load DPS strategies
    DpsStrategy vancleef;
    vancleef.optimalPosition = Position(0, 0, 0);
    vancleef.threatLimit = 0.8f;
    vancleef.canMoveDuringCast = false;
    _dpsStrategies[1] = vancleef;

    TC_LOG_DEBUG("server.loading", "Loaded {} DPS strategies", _dpsStrategies.size());
}

void EncounterStrategy::LoadEncounterMechanics()
{
    // Load encounter mechanics database
    // Example: VanCleef mechanics
    ::std::vector<EncounterMechanic> vancleefMechanics;

    EncounterMechanic addSpawns("add_spawns", "Adds spawn throughout fight");
    addSpawns.dangerLevel = 6.0f;
    vancleefMechanics.push_back(addSpawns);

    EncounterMechanic groundFire("ground_fire", "Fire on ground to avoid");
    groundFire.dangerLevel = 7.0f;
    vancleefMechanics.push_back(groundFire);

    _encounterMechanics[1] = vancleefMechanics;

    TC_LOG_DEBUG("server.loading", "Loaded encounter mechanics for {} encounters", _encounterMechanics.size());
}

void EncounterStrategy::ExecuteRoleStrategy(Player* player, uint32 encounterId, DungeonRole role)
{
    if (!player)
        return;

    switch (role)
    {
        case DungeonRole::TANK:
        {
            TankStrategy strategy = GetTankStrategy(encounterId, player);
            if (strategy.positioningStrategy)
                strategy.positioningStrategy(player, player->GetGroup(), DungeonBehavior::instance()->GetEncounterData(encounterId));
            break;
        }
        case DungeonRole::HEALER:
        {
            HealerStrategy strategy = GetHealerStrategy(encounterId, player);
            if (strategy.healingPriorityStrategy)
                strategy.healingPriorityStrategy(player, player->GetGroup(), DungeonBehavior::instance()->GetEncounterData(encounterId));
            break;
        }
        case DungeonRole::MELEE_DPS:
        case DungeonRole::RANGED_DPS:
        {
            DpsStrategy strategy = GetDpsStrategy(encounterId, player);
            if (strategy.damageOptimizationStrategy)
                strategy.damageOptimizationStrategy(player, player->GetGroup(), DungeonBehavior::instance()->GetEncounterData(encounterId));
            break;
        }
        default:
            break;
    }
}

void EncounterStrategy::HandleSpecificMechanic(Group* group, const EncounterMechanic& mechanic)
{
    if (!group)
        return;

    if (mechanic.handler)
    {
        mechanic.handler(group, mechanic);
    }
}

void EncounterStrategy::CoordinateGroupResponse(Group* group, const ::std::string& mechanic)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Coordinating group response to mechanic: {}", mechanic);
}

void EncounterStrategy::ValidateStrategyExecution(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    // Validate that strategy is being executed properly
    // Check positioning, cooldown usage, mechanics handling
}

Position EncounterStrategy::CalculateTankPosition(uint32 encounterId, Group* group)
{
    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);
    Position tankPos = encounter.encounterLocation;
    tankPos.RelocateOffset({0.0f, 5.0f, 0.0f}); // 5 yards in front
    return tankPos;
}

Position EncounterStrategy::CalculateHealerPosition(uint32 encounterId, Group* group)
{
    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);
    Position healerPos = encounter.encounterLocation;
    healerPos.RelocateOffset({0.0f, -15.0f, 0.0f}); // 15 yards back (safe distance)
    return healerPos;
}

Position EncounterStrategy::CalculateDpsPosition(uint32 encounterId, Group* group, bool isMelee)
{
    DungeonEncounter encounter = DungeonBehavior::instance()->GetEncounterData(encounterId);
    Position dpsPos = encounter.encounterLocation;

    if (isMelee)
    {
        dpsPos.RelocateOffset({0.0f, -3.0f, 0.0f}); // 3 yards behind boss (melee range)
    }
    else
    {
        dpsPos.RelocateOffset({0.0f, -20.0f, 0.0f}); // 20 yards back (ranged)
    }

    return dpsPos;
}

void EncounterStrategy::UpdateGroupFormation(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    UpdateEncounterPositioning(group, encounterId);
}

void EncounterStrategy::UpdateLearningData(uint32 encounterId, const ::std::string& mechanic, bool wasSuccessful)
{
    if (!_adaptiveStrategiesEnabled.load())
        return;

    ::std::lock_guard lock(_strategyMutex);

    if (_learningData.find(encounterId) == _learningData.end())
    {
        _learningData[encounterId] = StrategyLearningData();
    }

    StrategyLearningData& learning = _learningData[encounterId];
    uint32 mechanicHash = GenerateMechanicHash(mechanic);

    if (wasSuccessful)
    {
        learning.mechanicSuccesses[mechanicHash]++;
    }
    else
    {
        learning.mechanicFailures[mechanicHash]++;
    }

    learning.lastLearningUpdate = GameTime::GetGameTimeMS();
}

void EncounterStrategy::AdaptStrategyComplexity(uint32 encounterId)
{
    ::std::lock_guard lock(_strategyMutex);

    auto learningItr = _learningData.find(encounterId);
    if (learningItr == _learningData.end())
        return;

    StrategyLearningData const& learning = learningItr->second;
    float successRate = learning.totalEncountersAttempted > 0 ?
        static_cast<float>(learning.totalEncountersSuccessful) / learning.totalEncountersAttempted : 0.0f;

    // Adjust complexity based on success rate
    if (successRate < 0.5f)
    {
        // Low success rate, simplify strategy
        float newComplexity = ::std::max(0.3f, _strategyComplexity.load() - 0.1f);
        _strategyComplexity = newComplexity;

        TC_LOG_INFO("module.playerbot", "Simplified strategy complexity to {} for encounter {} (success rate: {})",
            newComplexity, encounterId, successRate);
    }
    else if (successRate > 0.9f)
    {
        // High success rate, can increase complexity for optimization
        float newComplexity = ::std::min(1.0f, _strategyComplexity.load() + 0.05f);
        _strategyComplexity = newComplexity;

        TC_LOG_DEBUG("module.playerbot", "Increased strategy complexity to {} for encounter {}",
            newComplexity, encounterId);
    }
}

void EncounterStrategy::OptimizeStrategyBasedOnLearning(uint32 encounterId)
{
    ::std::lock_guard lock(_strategyMutex);

    auto learningItr = _learningData.find(encounterId);
    if (learningItr == _learningData.end())
        return;

    StrategyLearningData const& learning = learningItr->second;

    // Analyze mechanic failure patterns
    for (auto const& [mechanicHash, failures] : learning.mechanicFailures)
    {
        uint32 successes = 0;
        auto successItr = learning.mechanicSuccesses.find(mechanicHash);
        if (successItr != learning.mechanicSuccesses.end())
        {
            successes = successItr->second;
        }

        float failureRate = static_cast<float>(failures) / (failures + successes);
        if (failureRate > 0.5f)
        {
            TC_LOG_WARN("module.playerbot", "High failure rate ({}) detected for mechanic hash {} in encounter {}",
                failureRate, mechanicHash, encounterId);

            // Would adjust strategy to focus more on this problematic mechanic
        }
    }
}

uint32 EncounterStrategy::GenerateMechanicHash(const ::std::string& mechanic)
{
    // Simple hash function for mechanic names
    uint32 hash = 0;
    for (char c : mechanic)
    {
        hash = hash * 31 + static_cast<uint32>(c);
    }
    return hash;
}

void EncounterStrategy::AnalyzeGroupPerformance(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    // Analyze metrics:
    // - Time to complete encounter
    // - Number of deaths
    // - Mechanic failures
    // - Resource efficiency
    // This data feeds into adaptive learning
}

void EncounterStrategy::IdentifyPerformanceBottlenecks(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    // Identify specific issues:
    // - Low DPS
    // - Insufficient healing
    // - Poor threat management
    // - Mechanic failures
}

void EncounterStrategy::RecommendStrategyAdjustments(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    // Based on performance analysis, recommend adjustments:
    // - Use more conservative strategy
    // - Focus on specific mechanics
    // - Adjust positioning
    // - Change cooldown usage patterns
}

DungeonRole EncounterStrategy::DeterminePlayerRole(Player* player)
{
    if (!player)
        return DungeonRole::MELEE_DPS;

    // Check for tank or healer specializations first
    if (IsTankSpec(player))
        return DungeonRole::TANK;

    if (IsHealerSpec(player))
        return DungeonRole::HEALER;

    // For DPS, determine ranged vs melee based on class and specialization
    switch (player->GetClass())
    {
        // Ranged DPS classes
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_EVOKER:
            return DungeonRole::RANGED_DPS;

        // Hybrid classes - check specialization for ranged vs melee
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
        {
            ChrSpecialization spec = player->GetPrimarySpecialization();
            // Ranged DPS specs
            if (spec == ChrSpecialization::PriestShadow ||
                spec == ChrSpecialization::ShamanElemental ||
                spec == ChrSpecialization::DruidBalance)
            {
                return DungeonRole::RANGED_DPS;
            }
            // Feral druid and Enhancement shaman are melee
            return DungeonRole::MELEE_DPS;
        }

        // Melee DPS classes
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
        case CLASS_ROGUE:
        case CLASS_MONK:
        case CLASS_DEMON_HUNTER:
        default:
            return DungeonRole::MELEE_DPS;
    }
}

// ============================================================================
// STATIC GENERIC MECHANIC HANDLERS (Public API)
// ============================================================================

void EncounterStrategy::HandleGenericInterrupts(::Player* player, ::Creature* boss)
{
    if (!player || !boss)
        return;

    // Check if boss is casting
    if (!boss->HasUnitState(UNIT_STATE_CASTING))
        return;

    ::Spell* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!currentSpell || !currentSpell->m_spellInfo)
        return;

    SpellInfo const* spellInfo = currentSpell->m_spellInfo;
    uint32 spellId = spellInfo->Id;

    // Interrupt priority assessment
    uint32 interruptPriority = 0;

    // Highest priority: Healing spells
    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) ||
        spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT))
    {
        interruptPriority = 100;
    }
    // High priority: High damage spells
    else if (spellInfo->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
    {
        interruptPriority = 75;
    }
    // Medium priority: CC spells
    else if (spellInfo->HasEffect(SPELL_EFFECT_APPLY_AURA) &&
             (spellInfo->HasAura(SPELL_AURA_MOD_STUN) ||
              spellInfo->HasAura(SPELL_AURA_MOD_FEAR) ||
              spellInfo->HasAura(SPELL_AURA_MOD_CHARM)))
    {
        interruptPriority = 50;
    }
    // Low priority: Buffs
    else
    {
        interruptPriority = 25;
    }

    // Check if player has interrupt available
    uint32 interruptSpell = 0;
    switch (player->GetClass())
    {
        case CLASS_WARRIOR: interruptSpell = 6552; break;  // Pummel
        case CLASS_PALADIN: interruptSpell = 96231; break; // Rebuke
        case CLASS_HUNTER: interruptSpell = 187650; break; // Counter Shot
        case CLASS_ROGUE: interruptSpell = 1766; break;    // Kick
        case CLASS_PRIEST: interruptSpell = 15487; break;  // Silence
        case CLASS_DEATH_KNIGHT: interruptSpell = 47528; break; // Mind Freeze
        case CLASS_SHAMAN: interruptSpell = 57994; break;  // Wind Shear
        case CLASS_MAGE: interruptSpell = 2139; break;     // Counterspell
        case CLASS_WARLOCK: interruptSpell = 119910; break; // Spell Lock
        case CLASS_MONK: interruptSpell = 116705; break;   // Spear Hand Strike
        case CLASS_DRUID: interruptSpell = 106839; break;  // Skull Bash
        case CLASS_DEMON_HUNTER: interruptSpell = 183752; break; // Disrupt
        case CLASS_EVOKER: interruptSpell = 351338; break; // Quell
        default: return;
    }

    if (interruptSpell == 0 || player->GetSpellHistory()->HasCooldown(interruptSpell))
        return;

    // Interrupt if priority is high enough (50+)
    if (interruptPriority >= 50)
    {
        TC_LOG_DEBUG("module.playerbot", "EncounterStrategy::HandleGenericInterrupts - Player {} interrupting spell {} (priority {})",
            player->GetGUID().GetCounter(), spellId, interruptPriority);

        // Would cast interrupt spell here
        // player->CastSpell(interruptSpell, false, boss);
    }
}

void EncounterStrategy::HandleGenericGroundAvoidance(::Player* player, ::Creature* boss)
{
    if (!player || !boss)
        return;

    // Find dangerous ground effects near player using spatial grid
    Map* map = player->GetMap();
    if (!map)
        return;

    // Use lock-free spatial grid to avoid deadlocks
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
    }

    if (!spatialGrid)
        return;

    // Query nearby dynamic objects using snapshot query
    auto dynamicObjSnapshots = spatialGrid->QueryNearbyDynamicObjects(
        player->GetPosition(), 15.0f);

    for (auto const& snapshot : dynamicObjSnapshots)
    {
        DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*player, snapshot.guid);
        if (!dynObj || dynObj->GetCaster() != boss)
            continue;

        // Check if spell is dangerous
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(dynObj->GetSpellId(), DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        bool isDangerous = spellInfo->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
                          spellInfo->HasEffect(SPELL_EFFECT_HEALTH_LEECH) ||
                          spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE);

        if (isDangerous)
        {
            float distance = player->GetExactDist(dynObj);
            if (distance < 5.0f)
            {
                // Move away from ground effect
                float angle = dynObj->GetAbsoluteAngle(player); // Direction away from effect
                float x = player->GetPositionX() + 10.0f * ::std::cos(angle);
                float y = player->GetPositionY() + 10.0f * ::std::sin(angle);
                float z = player->GetPositionZ();
                Position safePos(x, y, z);

                TC_LOG_DEBUG("module.playerbot", "EncounterStrategy::HandleGenericGroundAvoidance - Player {} moving away from spell {}",
                    player->GetGUID().GetCounter(), spellInfo->Id);

                // PHASE 6B: Use Movement Arbiter with DUNGEON_MECHANIC priority (205)
                BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
                if (botAI && botAI->GetUnifiedMovementCoordinator())
                {
                    bool accepted = botAI->RequestPointMovement(
                        PlayerBotMovementPriority::DUNGEON_MECHANIC,
                        safePos,
                        "Ground effect avoidance",
                        "EncounterStrategy");

                    if (!accepted)
                    {
                        TC_LOG_TRACE("playerbot.movement.arbiter",
                            "EncounterStrategy: Ground avoidance rejected for {} - higher priority active",
                            player->GetName());
                    }
                }
                else
                {
                    // Use validated pathfinding for positioning
                    Position dest(x, y, z, 0.0f);
                    if (BotAI* ai = GetBotAI(player))
                    {
                        if (!ai->MoveTo(dest, true))
                        {
                            // FALLBACK: Direct MotionMaster if validation fails
                            player->GetMotionMaster()->MovePoint(0, x, y, z);
                        }
                    }
                    else
                    {
                        // Non-bot player - use standard movement
                        player->GetMotionMaster()->MovePoint(0, x, y, z);
                    }
                }
                return;
            }
        }
    }
}

void EncounterStrategy::HandleGenericAddPriority(::Player* player, ::Creature* boss)
{
    if (!player || !boss)
        return;

    // ENTERPRISE: Use lock-free spatial grid for thread-safe creature queries
    Map* map = player->GetMap();
    if (!map)
        return;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return;
    }

    // Query nearby creatures using immutable snapshots (lock-free!)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(player->GetPosition(), 50.0f);

    ::Creature* highestPriorityAdd = nullptr;
    uint32 highestPriority = 0;

    for (auto const& snapshot : creatureSnapshots)
    {
        // Filter using snapshot data - no ObjectAccessor calls needed for filtering!
        if (snapshot.guid == boss->GetGUID())
            continue;

        if (!snapshot.isInCombat || !snapshot.IsAlive() || !snapshot.isHostile)
            continue;

        uint32 priority = 50; // Base priority

        // Casters (healers and mages) get highest priority
        // For NPCs: PALADIN (2) = support/healer type, MAGE (8) = caster type
        uint8 unitClass = snapshot.classId;
        if (unitClass == UNIT_CLASS_PALADIN)
            priority += 100;  // Potential healers/support
        else if (unitClass == UNIT_CLASS_MAGE)
            priority += 75;   // Ranged casters

        // Low health gets bonus priority
        if (snapshot.GetHealthPercent() < 30)
            priority += 30;

        // Closest gets slight bonus
        float distance = player->GetExactDist(snapshot.position);
        if (distance < 10.0f)
            priority += 10;

        if (priority > highestPriority)
        {
            // Only get actual Creature* for the best priority candidate
            if (Creature* creature = ObjectAccessor::GetCreature(*player, snapshot.guid))
            {
                highestPriority = priority;
                highestPriorityAdd = creature;
            }
        }
    }

    if (highestPriorityAdd)
    {
        TC_LOG_DEBUG("module.playerbot", "EncounterStrategy::HandleGenericAddPriority - Player {} targeting add {} (priority {})",
            player->GetGUID().GetCounter(), highestPriorityAdd->GetEntry(), highestPriority);

        player->SetSelection(highestPriorityAdd->GetGUID());
    }
}

void EncounterStrategy::HandleGenericPositioning(::Player* player, ::Creature* boss)
{
    if (!player || !boss)
        return;

    // Determine role
    DungeonRole role = DeterminePlayerRole(player);

    Position targetPos;
    float angle = 0.0f;
    float distance = 0.0f;

    switch (role)
    {
        case DungeonRole::TANK:
            // Tank: 5 yards in front of boss
            angle = boss->GetOrientation();
            distance = 5.0f;
            targetPos = boss->GetPosition();
            targetPos.RelocateOffset({::std::cos(angle) * distance, ::std::sin(angle) * distance, 0.0f});
            break;

        case DungeonRole::MELEE_DPS:
            // Melee: Behind boss
            angle = boss->GetOrientation() + static_cast<float>(M_PI); // Opposite direction
            distance = 5.0f;
            targetPos = boss->GetPosition();
            targetPos.RelocateOffset({::std::cos(angle) * distance, ::std::sin(angle) * distance, 0.0f});
            break;

        case DungeonRole::RANGED_DPS:
            // Ranged: 20-30 yards away
            angle = player->GetAbsoluteAngle(boss);
            distance = 25.0f;
            targetPos = boss->GetPosition();
            targetPos.RelocateOffset({::std::cos(angle) * distance, ::std::sin(angle) * distance, 0.0f});
            break;

        case DungeonRole::HEALER:
            // Healer: 15-20 yards away (safe distance)
            angle = player->GetAbsoluteAngle(boss);
            distance = 18.0f;
            targetPos = boss->GetPosition();
            targetPos.RelocateOffset({::std::cos(angle) * distance, ::std::sin(angle) * distance, 0.0f});
            break;

        default:
            return;
    }

    // Move if too far from optimal position
    if (player->GetExactDist(&targetPos) > 5.0f)
    {
        TC_LOG_DEBUG("module.playerbot", "EncounterStrategy::HandleGenericPositioning - Player {} moving to optimal position",
            player->GetGUID().GetCounter());
        // PHASE 6B: Use Movement Arbiter with DUNGEON_MECHANIC priority (205)
        BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::DUNGEON_MECHANIC,
                targetPos,
                "Generic optimal positioning",
                "EncounterStrategy");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "EncounterStrategy: Generic positioning rejected for {} - higher priority active",
                    player->GetName());
            }
        }
        else
        {
            // Use validated pathfinding for target positioning
            if (BotAI* ai = GetBotAI(player))
            {
                if (!ai->MoveTo(targetPos, true))
                {
                    // FALLBACK: Direct MotionMaster if validation fails
                    player->GetMotionMaster()->MovePoint(0, targetPos.GetPositionX(),
                        targetPos.GetPositionY(), targetPos.GetPositionZ());
                }
            }
            else
            {
                // Non-bot player - use standard movement
                player->GetMotionMaster()->MovePoint(0, targetPos.GetPositionX(),
                    targetPos.GetPositionY(), targetPos.GetPositionZ());
            }
        }
    }
}

void EncounterStrategy::HandleGenericDispel(::Player* player, ::Creature* boss)
{
    if (!player || !boss)
        return;
    // Check if player can dispel
    bool canDispelMagic = false;
    bool canDispelCurse = false;
    bool canDispelDisease = false;
    bool canDispelPoison = false;

    switch (player->GetClass())
    {
        case CLASS_PRIEST:
        case CLASS_PALADIN:
        case CLASS_SHAMAN:
            canDispelMagic = true;
            break;
        case CLASS_DRUID:
            canDispelMagic = true;
            canDispelCurse = true;
            canDispelPoison = true;
            break;
        case CLASS_MAGE:
            canDispelCurse = true;
            break;
        default:
            break;
    }

    if (!canDispelMagic && !canDispelCurse && !canDispelDisease && !canDispelPoison)
        return;

    // Find group members with dispellable debuffs
    Group* group = player->GetGroup();
    if (!group)
        return;

    for (auto const& member : group->GetMemberSlots())
    {
        Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
        if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
            continue;

        // Check for dispellable auras
        Unit::AuraApplicationMap const& appliedAuras = groupMember->GetAppliedAuras();
        for (auto const& [spellId, auraApp] : appliedAuras)
        {
            if (!auraApp || !auraApp->GetBase())
                continue;

            Aura* aura = auraApp->GetBase();
            if (aura->GetCaster() != boss)
                continue;
            SpellInfo const* spellInfo = aura->GetSpellInfo();
            if (!spellInfo)
                continue;

            // Check if harmful and dispellable
    if (!spellInfo->IsPositive())
            {
                TC_LOG_DEBUG("module.playerbot", "EncounterStrategy::HandleGenericDispel - Player {} attempting dispel on {}",
                    player->GetGUID().GetCounter(), groupMember->GetGUID().GetCounter());

                // Would cast dispel spell here
                // player->CastSpell(dispelSpellId, false, groupMember);
                return;
            }
        }
    }
}

void EncounterStrategy::HandleGenericMovement(::Player* player, ::Creature* boss)
{
    if (!player || !boss)
        return;

    // Maintain optimal range based on role
    DungeonRole role = DeterminePlayerRole(player);
    float currentDistance = player->GetExactDist(boss);
    float optimalDistance = 0.0f;

    switch (role)
    {
        case DungeonRole::TANK:
        case DungeonRole::MELEE_DPS:
            optimalDistance = 5.0f; // Melee range
            break;

        case DungeonRole::RANGED_DPS:
            optimalDistance = 25.0f; // Ranged optimal
            break;

        case DungeonRole::HEALER:
            optimalDistance = 18.0f; // Safe healing range
            break;

        default:
            return;
    }

    // Adjust position if out of range
    if (::std::abs(currentDistance - optimalDistance) > 5.0f)
    {
        TC_LOG_DEBUG("module.playerbot", "EncounterStrategy::HandleGenericMovement - Player {} adjusting range (current: {}, optimal: {})",
            player->GetGUID().GetCounter(), currentDistance, optimalDistance);

        HandleGenericPositioning(player, boss);
    }
}

void EncounterStrategy::HandleGenericSpread(::Player* player, ::Creature* boss, float distance)
{
    if (!player || !boss)
        return;

    Group* group = player->GetGroup();
    if (!group)
        return;

    // Check distance to other group members
    for (auto const& member : group->GetMemberSlots())
    {
        Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
        if (!groupMember || groupMember == player || !groupMember->IsInWorld() || !groupMember->IsAlive())
            continue;

        float distanceToMember = player->GetExactDist(groupMember);
        if (distanceToMember < distance)
        {
            // Too close, move away
            float angle = groupMember->GetAbsoluteAngle(player); // Direction away from member
            float x = player->GetPositionX() + (distance - distanceToMember) * ::std::cos(angle);
            float y = player->GetPositionY() + (distance - distanceToMember) * ::std::sin(angle);
            float z = player->GetPositionZ();
            Position spreadPos(x, y, z);

            TC_LOG_DEBUG("module.playerbot", "EncounterStrategy::HandleGenericSpread - Player {} spreading {} yards from {}",
                player->GetGUID().GetCounter(), distance, groupMember->GetGUID().GetCounter());

            // PHASE 6B: Use Movement Arbiter with DUNGEON_MECHANIC priority (205)
            BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
            {
                bool accepted = botAI->RequestPointMovement(
                    PlayerBotMovementPriority::DUNGEON_MECHANIC,
                    spreadPos,
                    "Spread positioning",
                    "EncounterStrategy");

                if (!accepted)
                {
                    TC_LOG_TRACE("playerbot.movement.arbiter",
                        "EncounterStrategy: Spread positioning rejected for {} - higher priority active",
                        player->GetName());
                }
            }
            else
            {
                // Use validated pathfinding for positioning
                Position dest(x, y, z, 0.0f);
                if (BotAI* ai = GetBotAI(player))
                {
                    if (!ai->MoveTo(dest, true))
                    {
                        // FALLBACK: Direct MotionMaster if validation fails
                        player->GetMotionMaster()->MovePoint(0, x, y, z);
                    }
                }
                else
                {
                    // Non-bot player - use standard movement
                    player->GetMotionMaster()->MovePoint(0, x, y, z);
                }
            }
            return;
        }
    }
}

void EncounterStrategy::HandleGenericStack(::Player* player, ::Creature* boss)
{
    if (!player || !boss)
        return;

    Group* group = player->GetGroup();
    if (!group)
        return;

    // Find tank
    Player* tank = nullptr;
    for (auto const& member : group->GetMemberSlots())
    {
        Player* groupMember = ObjectAccessor::FindPlayer(member.guid);
        if (!groupMember || !groupMember->IsInWorld() || !groupMember->IsAlive())
            continue;

        if (DeterminePlayerRole(groupMember) == DungeonRole::TANK)
        {
            tank = groupMember;
            break;
        }
    }

    if (!tank)
        return;

    // Stack on tank position
    float distanceToTank = player->GetExactDist(tank);
    if (distanceToTank > 3.0f)
    {
        TC_LOG_DEBUG("module.playerbot", "EncounterStrategy::HandleGenericStack - Player {} stacking on tank",
            player->GetGUID().GetCounter());

        Position tankPos(tank->GetPositionX(), tank->GetPositionY(), tank->GetPositionZ());

        // PHASE 6B: Use Movement Arbiter with DUNGEON_MECHANIC priority (205)
        BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            bool accepted = botAI->RequestPointMovement(
                PlayerBotMovementPriority::DUNGEON_MECHANIC,
                tankPos,
                "Stack on tank positioning",
                "EncounterStrategy");

            if (!accepted)
            {
                TC_LOG_TRACE("playerbot.movement.arbiter",
                    "EncounterStrategy: Stack positioning rejected for {} - higher priority active",
                    player->GetName());
            }
        }
        else
        {
            // Use validated pathfinding for stacking on tank
            Position dest(tank->GetPositionX(), tank->GetPositionY(), tank->GetPositionZ(), 0.0f);
            if (BotAI* ai = GetBotAI(player))
            {
                if (!ai->MoveTo(dest, true))
                {
                    // FALLBACK: Direct MotionMaster if validation fails
                    player->GetMotionMaster()->MovePoint(0, tank->GetPositionX(),
                        tank->GetPositionY(), tank->GetPositionZ());
                }
            }
            else
            {
                // Non-bot player - use standard movement
                player->GetMotionMaster()->MovePoint(0, tank->GetPositionX(),
                    tank->GetPositionY(), tank->GetPositionZ());
            }
        }
    }
}

// ============================================================================
// LEGACY GENERIC HANDLERS (Deprecated - kept for compatibility)
// ============================================================================

// Generic mechanic handlers
void EncounterStrategy::HandleTankSwapGeneric(Group* group)
{
    if (!group)
        return;

    // Find tanks and coordinate swap
    ::std::vector<Player*> tanks;
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld())
            continue;

        if (DeterminePlayerRole(player) == DungeonRole::TANK)
        {
            tanks.push_back(player);
        }
    }

    if (tanks.size() >= 2)
    {
        HandleTankSwapMechanic(group, tanks[0], tanks[1]);
    }
}

void EncounterStrategy::HandleAoEDamageGeneric(Group* group)
{
    if (!group)
        return;

    // Spread out group members
    UpdateEncounterPositioning(group, 0); // Generic positioning
}

void EncounterStrategy::HandleAddSpawnsGeneric(Group* group)
{
    if (!group)
        return;

    // DPS should switch to adds
    TC_LOG_DEBUG("module.playerbot", "Handling generic add spawns for group {}", group->GetGUID().GetCounter());
}

void EncounterStrategy::HandleStackingDebuffGeneric(Group* group)
{
    if (!group)
        return;

    // Monitor stacking debuffs and coordinate responses
    TC_LOG_DEBUG("module.playerbot", "Handling generic stacking debuff for group {}", group->GetGUID().GetCounter());
}

} // namespace Playerbot
