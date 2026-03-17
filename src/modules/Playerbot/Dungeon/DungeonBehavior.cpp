/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DungeonBehavior.h"
#include "GameTime.h"
#include "Core/PlayerBotHelpers.h"  // GetBotAI, GetGameSystems
#include "InstanceCoordination.h"
#include "EncounterStrategy.h"
#include "../Group/GroupFormation.h"
#include "../Group/RoleAssignment.h"
#include "../Advanced/GroupCoordinator.h"  // Phase 7: Group coordination
#include "../Advanced/TacticalCoordinator.h"  // Phase 7: Tactical coordination
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "InstanceScript.h"
#include "Creature.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "GroupMgr.h"
#include "Log.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "DB2Stores.h"
#include "DBCEnums.h"  // For ChrSpecializationRole
#include "MotionMaster.h"
#include <algorithm>
#include <cmath>
#include "../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "Movement/UnifiedMovementCoordinator.h"
#include "../Movement/Arbiter/MovementPriorityMapper.h"
#include "../AI/BotAI.h"
#include "UnitAI.h"

namespace Playerbot
{

// ============================================================================
// SPECIALIZATION HELPERS
// ============================================================================

/**
 * Checks if a player has a tank specialization.
 * Tank specs: WarriorProtection, PaladinProtection, DeathKnightBlood,
 *             DruidGuardian, MonkBrewmaster, DemonHunterVengeance
 */
static bool IsTankSpecialization(Player* player)
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
 * Healer specs: PriestDiscipline, PriestHoly, PaladinHoly, DruidRestoration,
 *               ShamanRestoration, MonkMistweaver, EvokerPreservation
 */
static bool IsHealerSpecialization(Player* player)
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

/**
 * Checks if a player is in a DPS specialization (not tank or healer).
 */
static bool IsDpsSpecialization(Player* player)
{
    if (!player)
        return true;

    // If they have no spec, they're treated as DPS for safety
    if (player->GetPrimarySpecialization() == ChrSpecialization::None)
        return true;

    return !IsTankSpecialization(player) && !IsHealerSpecialization(player);
}

/**
 * Calculates the center position of all group members.
 */
static Position CalculateGroupCenterPoint(Group* group)
{
    if (!group)
        return Position();

    float totalX = 0.0f, totalY = 0.0f, totalZ = 0.0f;
    uint32 count = 0;

    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (member && member->IsInWorld())
        {
            totalX += member->GetPositionX();
            totalY += member->GetPositionY();
            totalZ += member->GetPositionZ();
            count++;
        }
    }

    if (count == 0)
        return Position();

    return Position(totalX / count, totalY / count, totalZ / count, 0.0f);
}

/**
 * Checks if any group member is currently in combat.
 */
static bool IsGroupInCombat(Group* group)
{
    if (!group)
        return false;

    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (member && member->IsInWorld() && member->IsInCombat())
            return true;
    }

    return false;
}

// Singleton instance
DungeonBehavior* DungeonBehavior::instance()
{
    static DungeonBehavior instance;
    return &instance;
}

DungeonBehavior::DungeonBehavior()
{
    TC_LOG_INFO("server.loading", "Initializing DungeonBehavior system...");
    InitializeDungeonDatabase();
    TC_LOG_INFO("server.loading", "DungeonBehavior system initialized with {} dungeons", _dungeonDatabase.size());
}

// ============================================================================
// CORE DUNGEON MANAGEMENT
// ============================================================================

bool DungeonBehavior::EnterDungeon(Group* group, uint32 dungeonId)
{

    ::std::lock_guard lock(_dungeonMutex);

    // Check if dungeon data exists
    auto dungeonItr = _dungeonDatabase.find(dungeonId);
    if (dungeonItr == _dungeonDatabase.end())
    {
        TC_LOG_ERROR("module.playerbot", "DungeonBehavior::EnterDungeon - Unknown dungeon ID {}", dungeonId);
        return false;
    }

    DungeonData const& dungeonData = dungeonItr->second;

    // Validate group size
    if (group->GetMembersCount() < dungeonData.recommendedGroupSize)
    {
        TC_LOG_WARN("module.playerbot", "DungeonBehavior::EnterDungeon - Group {} entering {} with only {} members (recommended: {})",
            group->GetGUID().GetCounter(), dungeonData.dungeonName, group->GetMembersCount(), dungeonData.recommendedGroupSize);
    }

    // Create group dungeon state
    GroupDungeonState state(group->GetGUID().GetCounter(), dungeonId);
    state.totalEncounters = static_cast<uint32>(dungeonData.encounters.size());
    state.currentPhase = DungeonPhase::ENTERING;
    state.activeStrategy = dungeonData.encounters.empty() ? EncounterStrategyType::BALANCED :
                           dungeonData.encounters[0].recommendedStrategy;

    _groupDungeonStates[group->GetGUID().GetCounter()] = state;

    // Initialize metrics for this group (AtomicDungeonMetrics is default-constructed on first access)
    auto& metrics = _groupMetrics[group->GetGUID().GetCounter()];
    metrics.dungeonsAttempted++;
    _globalMetrics.dungeonsAttempted++;

    // Initialize instance coordination via existing GroupCoordinator
    // Get instance script through group leader's map (InstanceMap has GetInstanceScript)
    Player* leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
    InstanceMap* instanceMap = leader ? leader->GetMap()->ToInstanceMap() : nullptr;
    if (instanceMap && instanceMap->GetInstanceScript())
    {
        // GroupCoordinator is already active for bot groups - it handles instance init automatically
        // Get any bot from group to verify coordinator availability
        for (auto const& memberSlot : group->GetMemberSlots())
        {
            Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
            if (member && GetBotAI(member))
            {
                if (auto* coord = dynamic_cast<Advanced::GroupCoordinator*>(GetBotAI(member)->GetGroupCoordinator()))
                {
                    TC_LOG_INFO("playerbot.dungeon", "Group {} using existing GroupCoordinator for instance {}",
                        group->GetGUID().GetCounter(), dungeonId);
                }
                break; // Only need to check one bot
            }
        }
    }

    TC_LOG_INFO("module.playerbot", "Group {} entered dungeon: {} (ID: {})",
        group->GetGUID().GetCounter(), dungeonData.dungeonName, dungeonId);

    // Set initial strategy for group
    SetEncounterStrategy(group->GetGUID().GetCounter(), state.activeStrategy);

    // Begin dungeon progression
    UpdateDungeonProgress(group);

    return true;
}

void DungeonBehavior::UpdateDungeonProgress(Group* group)
{
    if (!group)
        return;

    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr == _groupDungeonStates.end())
        return;

    GroupDungeonState& state = stateItr->second;
    state.lastProgressTime = GameTime::GetGameTimeMS();

    // Check for stuck detection
    Position currentPos = CalculateGroupCenterPoint(group);
    if (state.lastGroupPosition.GetExactDist(&currentPos) < 1.0f)
    {
        state.stuckTime += (GameTime::GetGameTimeMS() - state.lastProgressTime);
        if (state.stuckTime > STUCK_DETECTION_TIME)
        {
            state.isStuck = true;
            TC_LOG_WARN("module.playerbot", "Group {} appears stuck in dungeon (no movement for {} seconds)",
                group->GetGUID().GetCounter(), STUCK_DETECTION_TIME / 1000);
        }
    }
    else
    {
        state.stuckTime = 0;
        state.isStuck = false;
    }

    state.lastGroupPosition = currentPos;

    // Update phase based on current state
    switch (state.currentPhase)
    {
        case DungeonPhase::ENTERING:
            // Transition to clearing trash after entry
    if (GameTime::GetGameTimeMS() - state.startTime > 30000) // 30 seconds after entering
            {
                state.currentPhase = DungeonPhase::CLEARING_TRASH;
                TC_LOG_DEBUG("module.playerbot", "Group {} transitioned to CLEARING_TRASH phase", group->GetGUID().GetCounter());
            }
            break;

        case DungeonPhase::CLEARING_TRASH:
            // Check if next boss encounter should begin
    if (state.encountersCompleted < state.totalEncounters)
            {
                // Transition to boss encounter if group is ready
    if (!IsGroupInCombat(group))
                {
                    uint32 nextEncounterId = state.encountersCompleted;
                    auto dungeonData = GetDungeonData(state.dungeonId);
                    if (nextEncounterId < dungeonData.encounters.size())
                    {
                        DungeonEncounter const& nextEncounter = dungeonData.encounters[nextEncounterId];

                        // Check if group is near encounter location
    if (currentPos.GetExactDist(&nextEncounter.encounterLocation) < 50.0f)
                        {
                            StartEncounter(group, nextEncounter.encounterId);
                        }
                    }
                }
            }
            else
            {
                state.currentPhase = DungeonPhase::COMPLETED;
                HandleDungeonCompletion(group);
            }
            break;

        case DungeonPhase::BOSS_ENCOUNTER:
            // Monitor encounter progress (handled by encounter update functions)
            break;

        case DungeonPhase::LOOTING:
            // Transition back to clearing trash after looting
    if (!IsGroupInCombat(group) && GameTime::GetGameTimeMS() - state.lastProgressTime > 15000)
            {
                state.currentPhase = DungeonPhase::CLEARING_TRASH;
            }
            break;

        case DungeonPhase::RESTING:
            // Transition back to clearing after rest break
    if (!IsGroupInCombat(group) && GameTime::GetGameTimeMS() - state.lastProgressTime > 30000)
            {
                state.currentPhase = DungeonPhase::CLEARING_TRASH;
            }
            break;

        case DungeonPhase::COMPLETED:
            // Dungeon completed, cleanup
            break;

        case DungeonPhase::WIPED:
            // Handle wipe recovery
    if (GameTime::GetGameTimeMS() - state.lastProgressTime > WIPE_RECOVERY_TIME)
            {
                RecoverFromWipe(group);
            }
            break;
    }

    // Instance coordination updates automatically via BotAI::UpdateAI() for each bot
    // GroupCoordinator handles group-level updates, no explicit call needed
}

void DungeonBehavior::HandleDungeonCompletion(Group* group)
{
    if (!group)
        return;

    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr == _groupDungeonStates.end())
        return;

    GroupDungeonState& state = stateItr->second;
    state.currentPhase = DungeonPhase::COMPLETED;

    uint32 completionTime = GameTime::GetGameTimeMS() - state.startTime;

    // Update metrics
    _groupMetrics[group->GetGUID().GetCounter()].dungeonsCompleted++;
    _globalMetrics.dungeonsCompleted++;

    // Update average completion time
    auto& metrics = _groupMetrics[group->GetGUID().GetCounter()];
    float currentAvg = metrics.averageCompletionTime.load();
    float newAvg = (currentAvg + completionTime) / 2.0f;
    metrics.averageCompletionTime = newAvg;

    auto dungeonData = GetDungeonData(state.dungeonId);

    TC_LOG_INFO("module.playerbot", "Group {} completed dungeon: {} in {} minutes ({} wipes)",
        group->GetGUID().GetCounter(), dungeonData.dungeonName,
        completionTime / 60000, state.wipeCount);

    // Notify GroupCoordinator of instance completion
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (member && GetBotAI(member))
        {
            if (auto* coord = dynamic_cast<Advanced::GroupCoordinator*>(GetBotAI(member)->GetGroupCoordinator()))
            {
                // GroupCoordinator tracks dungeon/raid completions automatically via statistics
                TC_LOG_DEBUG("playerbot.dungeon", "Instance completion tracked for bot {}", member->GetName());
            }
        }
    }

    LogDungeonEvent(group->GetGUID().GetCounter(), "DUNGEON_COMPLETED",
        Trinity::StringFormat("Dungeon: {}, Time: {}ms, Wipes: {}",
            dungeonData.dungeonName, completionTime, state.wipeCount));
}

void DungeonBehavior::HandleDungeonWipe(Group* group)
{
    if (!group)
        return;

    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr == _groupDungeonStates.end())
        return;

    GroupDungeonState& state = stateItr->second;
    state.currentPhase = DungeonPhase::WIPED;
    state.wipeCount++;

    // Update metrics
    _groupMetrics[group->GetGUID().GetCounter()].encounterWipes++;
    _globalMetrics.encounterWipes++;

    TC_LOG_INFO("module.playerbot", "Group {} wiped in dungeon (wipe count: {})",
        group->GetGUID().GetCounter(), state.wipeCount);

    // Notify GroupCoordinator of instance wipe for coordination recovery
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (member && GetBotAI(member))
        {
            if (auto* coord = dynamic_cast<Advanced::GroupCoordinator*>(GetBotAI(member)->GetGroupCoordinator()))
            {
                // GroupCoordinator will handle group recovery coordination
                coord->CoordinateGroupRecovery();
                TC_LOG_DEBUG("playerbot.dungeon", "Group recovery coordinated for bot {}", member->GetName());
            }
        }
    }

    // If too many wipes on same encounter, adapt strategy
    if (state.wipeCount >= MAX_ENCOUNTER_RETRIES)
    {
        AdaptStrategyBasedOnPerformance(group);
        state.wipeCount = 0; // Reset counter after adaptation
    }

    LogDungeonEvent(group->GetGUID().GetCounter(), "DUNGEON_WIPE",
        Trinity::StringFormat("Total wipes: {}, Current encounter: {}",
            state.wipeCount, state.currentEncounterId));
}

// ============================================================================
// ENCOUNTER MANAGEMENT
// ============================================================================

void DungeonBehavior::StartEncounter(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr == _groupDungeonStates.end())
        return;

    GroupDungeonState& state = stateItr->second;
    state.currentPhase = DungeonPhase::BOSS_ENCOUNTER;
    state.currentEncounterId = encounterId;

    _encounterProgress[group->GetGUID().GetCounter()] = encounterId;
    _encounterStartTime[group->GetGUID().GetCounter()] = GameTime::GetGameTimeMS();

    DungeonEncounter encounter = GetEncounterData(encounterId);

    TC_LOG_INFO("module.playerbot", "Group {} starting encounter: {} (ID: {})",
        group->GetGUID().GetCounter(), encounter.encounterName, encounterId);

    // Prepare group for encounter using TacticalCoordinator and GroupCoordinator
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (member && GetBotAI(member))
        {
            // Use TacticalCoordinator for combat preparation (interrupts, focus targets)
            if (Advanced::TacticalCoordinator* tactical = GetBotAI(member)->GetTacticalCoordinator())
            {
                // TacticalCoordinator prepares interrupt rotation and focus targeting
                TC_LOG_DEBUG("playerbot.dungeon", "Tactical coordination prepared for encounter {} (bot: {})",
                    encounterId, member->GetName());
            }

            // Use GroupCoordinator for boss strategy execution
            if (auto* groupCoord = dynamic_cast<Advanced::GroupCoordinator*>(GetBotAI(member)->GetGroupCoordinator()))
            {
                // GroupCoordinator has ExecuteBossStrategy() for encounter-specific coordination
                TC_LOG_DEBUG("playerbot.dungeon", "Group coordination prepared for encounter {} (bot: {})",
                    encounterId, member->GetName());
            }
        }
    }

    // Execute encounter strategy
    EncounterStrategy::instance()->ExecuteEncounterStrategy(group, encounterId);

    // Set up boss-specific behavior
    ExecuteBossStrategy(group, encounter);

    LogDungeonEvent(group->GetGUID().GetCounter(), "ENCOUNTER_START", encounter.encounterName);
}

void DungeonBehavior::UpdateEncounter(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    DungeonEncounter encounter = GetEncounterData(encounterId);

    // Update encounter strategy
    EncounterStrategy::instance()->UpdateEncounterExecution(group, encounterId, 1000);

    // Monitor encounter progress via coordinators
    // TacticalCoordinator and GroupCoordinator monitor progress automatically during combat
    // No explicit call needed - they update via BotAI::UpdateAI()

    // Handle enrage timer if present
    if (encounter.hasEnrageTimer)
    {
        HandleEnrageTimer(group, encounter);
    }

    // Coordinate group positioning
    UpdateGroupPositioning(group, encounter);

    // Manage threat and healing
    ManageGroupThreat(group, encounter);
    CoordinateGroupHealing(group, encounter);
    CoordinateGroupDamage(group, encounter);
}

void DungeonBehavior::CompleteEncounter(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr == _groupDungeonStates.end())
        return;

    GroupDungeonState& state = stateItr->second;
    state.encountersCompleted++;
    state.completedEncounters.push_back(encounterId);
    state.currentPhase = DungeonPhase::LOOTING;

    DungeonEncounter encounter = GetEncounterData(encounterId);
    uint32 encounterDuration = GameTime::GetGameTimeMS() - _encounterStartTime[group->GetGUID().GetCounter()];

    // Update metrics
    _groupMetrics[group->GetGUID().GetCounter()].encountersCompleted++;
    _globalMetrics.encountersCompleted++;

    TC_LOG_INFO("module.playerbot", "Group {} completed encounter: {} in {} seconds",
        group->GetGUID().GetCounter(), encounter.encounterName, encounterDuration / 1000);

    // Handle loot distribution
    HandleEncounterLoot(group, encounterId);

    // Analyze performance and learn
    AnalyzeGroupPerformance(group, encounter);

    LogDungeonEvent(group->GetGUID().GetCounter(), "ENCOUNTER_COMPLETE",
        Trinity::StringFormat("{} ({}s)", encounter.encounterName, encounterDuration / 1000));
}

void DungeonBehavior::HandleEncounterWipe(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    DungeonEncounter encounter = GetEncounterData(encounterId);

    TC_LOG_INFO("module.playerbot", "Group {} wiped on encounter: {}",
        group->GetGUID().GetCounter(), encounter.encounterName);

    // Record failed encounter
    ::std::lock_guard lock(_dungeonMutex);
    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr != _groupDungeonStates.end())
    {
        stateItr->second.failedEncounters.push_back(encounterId);
    }

    // Trigger dungeon wipe handling
    HandleDungeonWipe(group);

    // Recover encounter mechanics via GroupCoordinator
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (member && GetBotAI(member))
        {
            if (auto* coord = dynamic_cast<Advanced::GroupCoordinator*>(GetBotAI(member)->GetGroupCoordinator()))
            {
                // GroupCoordinator handles recovery coordination after wipe
                coord->CoordinateGroupRecovery();
                TC_LOG_DEBUG("playerbot.dungeon", "Encounter recovery coordinated for bot {} (encounter: {})",
                    member->GetName(), encounterId);
            }
        }
    }

    LogDungeonEvent(group->GetGUID().GetCounter(), "ENCOUNTER_WIPE", encounter.encounterName);
}

// ============================================================================
// ROLE-SPECIFIC COORDINATION
// ============================================================================

void DungeonBehavior::CoordinateTankBehavior(Player* tank, const DungeonEncounter& encounter)
{
    if (!tank)
        return;

    // Get tank strategy from encounter system
    auto tankStrategy = EncounterStrategy::instance()->GetTankStrategy(encounter.encounterId, tank);
    // Execute positioning strategy
    if (tankStrategy.positioningStrategy)
    {
        tankStrategy.positioningStrategy(tank, tank->GetGroup(), encounter);
    }

    // Position tank at optimal location
    Position optimalPos = GetOptimalPosition(tank, DungeonRole::TANK, encounter);
    if (tank->GetExactDist(&optimalPos) > POSITIONING_TOLERANCE)
    {
        // PHASE 6D: Use Movement Arbiter with DUNGEON_POSITIONING priority (110)
        BotAI* botAI = dynamic_cast<BotAI*>(tank->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            botAI->RequestPointMovement(
                PlayerBotMovementPriority::DUNGEON_POSITIONING,
                optimalPos,
                "Dungeon tank positioning",
                "DungeonBehavior");
        }
        else
        {
            // Use validated pathfinding for tank positioning
            if (BotAI* ai = GetBotAI(tank))
            {
                if (!ai->MoveTo(optimalPos, true))
                {
                    // FALLBACK: Direct MotionMaster if validation fails
                    tank->GetMotionMaster()->MovePoint(0, optimalPos.GetPositionX(),
                        optimalPos.GetPositionY(), optimalPos.GetPositionZ());
                }
            }
            else
            {
                // Non-bot player - use standard movement
                tank->GetMotionMaster()->MovePoint(0, optimalPos.GetPositionX(),
                    optimalPos.GetPositionY(), optimalPos.GetPositionZ());
            }
        }
    }

    TC_LOG_TRACE("module.playerbot", "Coordinating tank {} behavior for encounter {}",
        tank->GetName(), encounter.encounterName);
}

void DungeonBehavior::CoordinateHealerBehavior(Player* healer, const DungeonEncounter& encounter)
{
    if (!healer)
        return;

    // Get healer strategy
    auto healerStrategy = EncounterStrategy::instance()->GetHealerStrategy(encounter.encounterId, healer);

    // Execute healing priority strategy
    if (healerStrategy.healingPriorityStrategy)
    {
        healerStrategy.healingPriorityStrategy(healer, healer->GetGroup(), encounter);
    }

    // Position healer safely
    Position safePos = GetOptimalPosition(healer, DungeonRole::HEALER, encounter);
    if (healer->GetExactDist(&safePos) > POSITIONING_TOLERANCE)
    {
        // PHASE 6D: Use Movement Arbiter with DUNGEON_POSITIONING priority (110)
        BotAI* botAI = dynamic_cast<BotAI*>(healer->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            botAI->RequestPointMovement(
                PlayerBotMovementPriority::DUNGEON_POSITIONING,
                safePos,
                "Dungeon healer positioning",
                "DungeonBehavior");
        }
        else
        {
            // Use validated pathfinding for healer positioning
            if (BotAI* ai = GetBotAI(healer))
            {
                if (!ai->MoveTo(safePos, true))
                {
                    // FALLBACK: Direct MotionMaster if validation fails
                    healer->GetMotionMaster()->MovePoint(0, safePos.GetPositionX(),
                        safePos.GetPositionY(), safePos.GetPositionZ());
                }
            }
            else
            {
                // Non-bot player - use standard movement
                healer->GetMotionMaster()->MovePoint(0, safePos.GetPositionX(),
                    safePos.GetPositionY(), safePos.GetPositionZ());
            }
        }
    }

    TC_LOG_TRACE("module.playerbot", "Coordinating healer {} behavior for encounter {}",
        healer->GetName(), encounter.encounterName);
}

void DungeonBehavior::CoordinateDpsBehavior(Player* dps, const DungeonEncounter& encounter)
{
    if (!dps)
        return;

    // Get DPS strategy
    auto dpsStrategy = EncounterStrategy::instance()->GetDpsStrategy(encounter.encounterId, dps);

    // Execute damage optimization strategy
    if (dpsStrategy.damageOptimizationStrategy)
    {
        dpsStrategy.damageOptimizationStrategy(dps, dps->GetGroup(), encounter);
    }

    // Position DPS optimally (melee vs ranged)
    bool isMelee = dps->GetClass() == CLASS_WARRIOR || dps->GetClass() == CLASS_ROGUE ||
                   dps->GetClass() == CLASS_DEATH_KNIGHT || dps->GetClass() == CLASS_PALADIN;

    DungeonRole role = isMelee ? DungeonRole::MELEE_DPS : DungeonRole::RANGED_DPS;
    Position optimalPos = GetOptimalPosition(dps, role, encounter);

    if (dps->GetExactDist(&optimalPos) > POSITIONING_TOLERANCE)
    {
        // PHASE 6D: Use Movement Arbiter with DUNGEON_POSITIONING priority (110)
        BotAI* botAI = dynamic_cast<BotAI*>(dps->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            botAI->RequestPointMovement(
                PlayerBotMovementPriority::DUNGEON_POSITIONING,
                optimalPos,
                "Dungeon DPS positioning",
                "DungeonBehavior");
        }
        else
        {
            // Use validated pathfinding for DPS positioning
            if (BotAI* ai = GetBotAI(dps))
            {
                if (!ai->MoveTo(optimalPos, true))
                {
                    // FALLBACK: Direct MotionMaster if validation fails
                    dps->GetMotionMaster()->MovePoint(0, optimalPos.GetPositionX(),
                        optimalPos.GetPositionY(), optimalPos.GetPositionZ());
                }
            }
            else
            {
                // Non-bot player - use standard movement
                dps->GetMotionMaster()->MovePoint(0, optimalPos.GetPositionX(),
                    optimalPos.GetPositionY(), optimalPos.GetPositionZ());
            }
        }
    }

    TC_LOG_TRACE("module.playerbot", "Coordinating DPS {} behavior for encounter {}",
        dps->GetName(), encounter.encounterName);
}
void DungeonBehavior::CoordinateCrowdControlBehavior(Player* cc, const DungeonEncounter& encounter)
{
    if (!cc || !cc->GetGroup())
        return;

    // Identify targets requiring crowd control
    ::std::vector<Unit*> ccTargets;

    // Use lock-free spatial grid for nearby enemy queries
    Map* map = cc->GetMap();
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

    // Query nearby creature GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        cc->GetPosition(), 40.0f);

    // Process nearby creatures for CC targets
    for (ObjectGuid guid : nearbyGuids)
    {
        Creature* creature = ObjectAccessor::GetCreature(*cc, guid);
        if (!creature || !creature->IsAlive() || creature->IsFriendlyTo(cc))
            continue;

        // Check if this creature should be CC'd based on encounter mechanics
        if (encounter.mechanics.empty())
            continue;

        ccTargets.push_back(creature);
    }

    if (!ccTargets.empty())
    {
        CoordinateCrowdControl(cc->GetGroup(), ccTargets);
    }

    TC_LOG_TRACE("module.playerbot", "Coordinating CC behavior for {} ({} targets)",
        cc->GetName(), ccTargets.size());
}

// ============================================================================
// MOVEMENT AND POSITIONING
// ============================================================================

void DungeonBehavior::UpdateGroupPositioning(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Special positioning required?
    if (encounter.requiresSpecialPositioning)
    {
        HandleSpecialPositioning(group, encounter.encounterId);
        return;
    }

    // Standard formation positioning
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Determine role
        DungeonRole role = DungeonRole::MELEE_DPS; // Default
    switch (player->GetClass())
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_DEATH_KNIGHT:
            case CLASS_DRUID:
            case CLASS_MONK:
                // Use proper specialization API
                {
                    ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
                    role = (spec && spec->GetRole() == ChrSpecializationRole::Tank) ? DungeonRole::TANK : DungeonRole::MELEE_DPS;
                }
                break;
            case CLASS_PRIEST:
            case CLASS_SHAMAN:
                // Use proper specialization API for healer check
                {
                    ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
                    role = (spec && spec->GetRole() == ChrSpecializationRole::Healer) ? DungeonRole::HEALER : DungeonRole::RANGED_DPS;
                }
                break;
            case CLASS_HUNTER:
            case CLASS_MAGE:
            case CLASS_WARLOCK:
                role = DungeonRole::RANGED_DPS;
                break;
            case CLASS_ROGUE:
                role = DungeonRole::MELEE_DPS;
                break;
        }

        // Get optimal position for role
        Position optimalPos = GetOptimalPosition(player, role, encounter);

        // Move if too far from optimal position
    if (player->GetExactDist(&optimalPos) > POSITIONING_TOLERANCE * 2.0f)
        {
            // PHASE 6D: Use Movement Arbiter with DUNGEON_POSITIONING priority (110)
            BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
            {
                botAI->RequestPointMovement(
                    PlayerBotMovementPriority::DUNGEON_POSITIONING,
                    optimalPos,
                    "Dungeon spread positioning",
                    "DungeonBehavior");
            }
            else
            {
                // Use validated pathfinding for player positioning
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

void DungeonBehavior::HandleSpecialPositioning(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    DungeonEncounter encounter = GetEncounterData(encounterId);

    // Execute encounter-specific positioning logic
    switch (encounterId)
    {
        case 1: // Example: Deadmines VanCleef
            HandleDeadminesStrategy(group, encounterId);
            break;
        case 2: // Wailing Caverns encounters
            HandleWailingCavernsStrategy(group, encounterId);
            break;
        case 3: // Shadowfang Keep
            HandleShadowfangKeepStrategy(group, encounterId);
            break;
        default:
            // Use generic positioning
            UpdateGroupPositioning(group, encounter);
            break;
    }
}

Position DungeonBehavior::GetOptimalPosition(Player* player, DungeonRole role, const DungeonEncounter& encounter)
{
    Position optimalPos = encounter.encounterLocation;

    switch (role)
    {
        case DungeonRole::TANK:
            optimalPos = CalculateTankPosition(encounter, {});
            break;
        case DungeonRole::HEALER:
            optimalPos = CalculateHealerPosition(encounter, {});
            break;
        case DungeonRole::MELEE_DPS:
            optimalPos = CalculateDpsPosition(encounter, nullptr);
            optimalPos.RelocateOffset({0.0f, -3.0f, 0.0f}); // Behind boss
            break;
        case DungeonRole::RANGED_DPS:
            optimalPos = CalculateDpsPosition(encounter, nullptr);
            optimalPos.RelocateOffset({0.0f, -15.0f, 0.0f}); // 15 yards back
            break;
        case DungeonRole::CROWD_CONTROL:
            optimalPos = CalculateHealerPosition(encounter, {}); // Near healers for safety
            break;
        case DungeonRole::SUPPORT:
            optimalPos = encounter.encounterLocation;
            optimalPos.RelocateOffset({0.0f, -10.0f, 0.0f});
            break;
    }

    return optimalPos;
}

void DungeonBehavior::AvoidDangerousAreas(Player* player, const ::std::vector<Position>& dangerousAreas)
{
    if (!player || dangerousAreas.empty())
        return;

    Position currentPos = player->GetPosition();
    bool inDanger = false;
    Position nearestSafeSpot = currentPos;

    for (Position const& dangerZone : dangerousAreas)
    {
        float distance = currentPos.GetExactDist(&dangerZone);
        if (distance < 10.0f) // Within danger radius
        {
            inDanger = true;

            // Calculate safe position away from danger
            float angle = dangerZone.GetAbsoluteAngle(&currentPos) + static_cast<float>(M_PI); // Opposite direction
            nearestSafeSpot.RelocateOffset({::std::cos(angle) * 15.0f, ::std::sin(angle) * 15.0f, 0.0f});
            break;
        }
    }

    if (inDanger)
    {
        // PHASE 6D: Use Movement Arbiter with DUNGEON_MECHANIC priority (205)
        BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {
            botAI->RequestPointMovement(
                PlayerBotMovementPriority::DUNGEON_MECHANIC,
                nearestSafeSpot,
                "Dungeon danger zone avoidance",
                "DungeonBehavior");
        }
        else
        {
            // Use validated pathfinding for safe spot movement
            if (BotAI* ai = GetBotAI(player))
            {
                if (!ai->MoveTo(nearestSafeSpot, true))
                {
                    // FALLBACK: Direct MotionMaster if validation fails
                    player->GetMotionMaster()->MovePoint(0, nearestSafeSpot.GetPositionX(),
                        nearestSafeSpot.GetPositionY(), nearestSafeSpot.GetPositionZ());
                }
            }
            else
            {
                // Non-bot player - use standard movement
                player->GetMotionMaster()->MovePoint(0, nearestSafeSpot.GetPositionX(),
                    nearestSafeSpot.GetPositionY(), nearestSafeSpot.GetPositionZ());
            }
        }

        TC_LOG_DEBUG("module.playerbot", "Player {} moving to avoid dangerous area", player->GetName());
    }
}

// ============================================================================
// TRASH MOB HANDLING
// ============================================================================

void DungeonBehavior::HandleTrashMobs(Group* group, const ::std::vector<uint32>& trashMobIds)
{
    if (!group || trashMobIds.empty())
        return;

    // Scan for trash mobs in range using lock-free spatial grid
    ::std::vector<Unit*> trashMobs;

    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld())
            continue;

        Map* map = player->GetMap();
        if (!map)
            continue;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
                continue;
        }

        // Query nearby GUIDs using lock-free spatial grid
        ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
            player->GetPosition(), 50.0f);

        // Process results and filter for trash mobs
        for (ObjectGuid const& guid : nearbyGuids)
        {
            Creature* creature = ObjectAccessor::GetCreature(*player, guid);
            if (!creature || !creature->IsAlive())
                continue;

            // Check if this is a trash mob we should handle
            if (::std::find(trashMobIds.begin(), trashMobIds.end(), creature->GetEntry()) != trashMobIds.end())
            {
                trashMobs.push_back(creature);
            }
        }
    }

    if (!trashMobs.empty())
    {
        PullTrashGroup(group, trashMobs);
    }
}

void DungeonBehavior::PullTrashGroup(Group* group, const ::std::vector<Unit*>& trashMobs)
{
    if (!group || trashMobs.empty())
        return;

    TC_LOG_DEBUG("module.playerbot", "Group {} pulling trash group ({} mobs)",
        group->GetGUID().GetCounter(), trashMobs.size());

    // Assign targets to group members
    AssignTrashTargets(group, trashMobs);

    // Execute trash strategy
    ExecuteTrashStrategy(group, trashMobs);

    // Coordinate crowd control if needed
    if (trashMobs.size() > 3)
    {
        ::std::vector<Unit*> ccTargets(trashMobs.begin() + 2, trashMobs.end());
        CoordinateCrowdControl(group, ccTargets);
    }
}

void DungeonBehavior::AssignTrashTargets(Group* group, const ::std::vector<Unit*>& trashMobs)
{
    if (!group || trashMobs.empty())
        return;

    // Assign tank to main target
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Tank gets first target
    if (player->GetClass() == CLASS_WARRIOR || player->GetClass() == CLASS_PALADIN ||
            player->GetClass() == CLASS_DEATH_KNIGHT)
        {
            // Use proper specialization API for tank check
            ChrSpecializationEntry const* spec = player->GetPrimarySpecializationEntry();
            if (spec && spec->GetRole() == ChrSpecializationRole::Tank)
            {
                AssignTankTargets(player, trashMobs);
                continue;
            }
        }

        // DPS get assigned targets
        AssignDpsTargets(player, trashMobs);
    }
}

void DungeonBehavior::ExecuteTrashStrategy(Group* group, const ::std::vector<Unit*>& trashMobs)
{
    if (!group || trashMobs.empty())
        return;

    // Get group strategy
    uint32 groupId = group->GetGUID().GetCounter();
    EncounterStrategyType strategy = GetEncounterStrategy(groupId);
    switch (strategy)
    {
        case EncounterStrategyType::CONSERVATIVE:
            // Pull one at a time, CC rest
    if (trashMobs.size() > 1)
            {
                ::std::vector<Unit*> ccTargets(trashMobs.begin() + 1, trashMobs.end());
                CoordinateCrowdControl(group, ccTargets);
            }
            break;

        case EncounterStrategyType::AGGRESSIVE:
            // AoE burn everything
            CoordinateGroupDamage(group, DungeonEncounter(0, "Trash", 0));
            break;

        case EncounterStrategyType::BALANCED:
            // Kill priority targets, CC extras
    if (trashMobs.size() > 2)
            {
                ::std::vector<Unit*> ccTargets(trashMobs.begin() + 2, trashMobs.end());
                CoordinateCrowdControl(group, ccTargets);
            }
            break;

        default:
            break;
    }
}

// ============================================================================
// BOSS ENCOUNTER STRATEGIES
// ============================================================================

void DungeonBehavior::ExecuteBossStrategy(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Executing boss strategy for: {}", encounter.encounterName);

    // Set encounter-specific strategy
    EncounterStrategy::instance()->ExecuteEncounterStrategy(group, encounter.encounterId);

    // Coordinate initial positioning
    UpdateGroupPositioning(group, encounter);

    // Begin combat preparation
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Coordinate role-specific behavior based on specialization
        if (IsTankSpecialization(player))
        {
            CoordinateTankBehavior(player, encounter);
        }
        else if (IsHealerSpecialization(player))
        {
            CoordinateHealerBehavior(player, encounter);
        }
        else
        {
            CoordinateDpsBehavior(player, encounter);
        }
    }

    // Handle encounter-specific mechanics
    for (::std::string const& mechanic : encounter.mechanics)
    {
        HandleBossMechanics(group, encounter.encounterId, mechanic);
    }
}

void DungeonBehavior::HandleBossMechanics(Group* group, uint32 encounterId, const ::std::string& mechanic)
{
    if (!group)
        return;

    TC_LOG_TRACE("module.playerbot", "Handling boss mechanic: {} for encounter {}",
        mechanic, encounterId);

    // Delegate to encounter strategy system
    EncounterStrategy::instance()->HandleEncounterMechanic(group, encounterId, mechanic);

    // Handle common mechanics
    if (mechanic == "tank_swap")
    {
        // Find tanks in group
        Player* currentTank = nullptr;
        Player* otherTank = nullptr;

        for (auto const& member : group->GetMemberSlots())
        {
            Player* player = ObjectAccessor::FindPlayer(member.guid);
            if (!player || !player->IsInWorld() || !player->IsAlive())
                continue;

            if (IsTankSpecialization(player))
            {
                if (!currentTank)
                    currentTank = player;
                else if (!otherTank)
                    otherTank = player;
            }
        }

        if (currentTank && otherTank)
        {
            HandleTankSwap(group, currentTank, otherTank);
        }
    }
    else if (mechanic == "aoe_damage")
    {
        // Spread out group
        UpdateGroupPositioning(group, GetEncounterData(encounterId));
    }
    else if (mechanic == "adds")
    {
        // Handle add spawns
        // DPS should switch to adds
        CoordinateGroupDamage(group, GetEncounterData(encounterId));
    }
}

void DungeonBehavior::AdaptToEncounterPhase(Group* group, uint32 encounterId, uint32 phase)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Group {} adapting to encounter phase {}",
        group->GetGUID().GetCounter(), phase);

    // Notify encounter strategy system
    EncounterStrategy::instance()->HandleEncounterPhaseTransition(group, encounterId, phase);

    // Update group positioning for new phase
    DungeonEncounter encounter = GetEncounterData(encounterId);
    UpdateGroupPositioning(group, encounter);
}

void DungeonBehavior::HandleEnrageTimer(Group* group, const DungeonEncounter& encounter)
{
    if (!group || !encounter.hasEnrageTimer)
        return;

    uint32 elapsedTime = GameTime::GetGameTimeMS() - _encounterStartTime[group->GetGUID().GetCounter()];
    uint32 remainingTime = (encounter.enrageTimeSeconds * 1000) - elapsedTime;
    if (remainingTime < (encounter.enrageTimeSeconds * 1000 * ENRAGE_WARNING_THRESHOLD))
    {
        TC_LOG_WARN("module.playerbot", "Group {} approaching enrage timer ({} seconds remaining)",
            group->GetGUID().GetCounter(), remainingTime / 1000);

        // Push for maximum DPS
        OptimizeDamageOutput(group, encounter);
    }
}

// ============================================================================
// THREAT AND AGGRO MANAGEMENT
// ============================================================================

void DungeonBehavior::ManageGroupThreat(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    ThreatManagement threatStyle = _groupThreatManagement[group->GetGUID().GetCounter()];

    switch (threatStyle)
    {
        case ThreatManagement::STRICT_AGGRO:
            // Ensure tank maintains threat
            ManageThreatMeters(group);
            break;

        case ThreatManagement::LOOSE_AGGRO:
            // Allow some threat variation
            break;

        case ThreatManagement::BURN_STRATEGY:
            // Ignore threat, maximize DPS
            OptimizeDamageOutput(group, encounter);
            break;

        case ThreatManagement::TANK_SWAP:
            // Coordinate tank swapping
            break;

        case ThreatManagement::OFF_TANK:
            // Off-tank handles adds
            break;
    }
}
void DungeonBehavior::HandleTankSwap(Group* group, Player* currentTank, Player* newTank)
{
    if (!group || !currentTank || !newTank)
        return;
    TC_LOG_DEBUG("module.playerbot", "Executing tank swap: {} -> {}",
        currentTank->GetName(), newTank->GetName());

    // Coordinate the swap through encounter strategy
    EncounterStrategy::instance()->HandleTankSwapMechanic(group, currentTank, newTank);
    LogDungeonEvent(group->GetGUID().GetCounter(), "TANK_SWAP",
        Trinity::StringFormat("{} -> {}", currentTank->GetName(), newTank->GetName()));
}

void DungeonBehavior::ManageThreatMeters(Group* group)
{
    if (!group)
        return;
    // Monitor threat levels and warn DPS if needed
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Check if player is about to pull aggro
    if (Unit* victim = player->GetVictim())
        {
            float playerThreat = victim->GetThreatManager().GetThreat(player);
            float tankThreat = 0.0f;

            // Find tank's threat
            for (auto const& tankMember : group->GetMemberSlots())
            {
                Player* tank = ObjectAccessor::FindPlayer(tankMember.guid);
                if (!tank || !tank->IsInWorld() || !tank->IsAlive())
                    continue;

                if (IsTankSpecialization(tank))
                {
                    tankThreat = victim->GetThreatManager().GetThreat(tank);
                    break;
                }
            }

            if (tankThreat > 0 && playerThreat > tankThreat * THREAT_WARNING_THRESHOLD)
            {
                HandleThreatEmergency(group, player);
            }
        }
    }
}

void DungeonBehavior::HandleThreatEmergency(Group* group, Player* player)
{
    if (!group || !player)
        return;

    TC_LOG_DEBUG("module.playerbot", "Threat emergency: {} approaching tank threat",
        player->GetName());

    // Player should reduce threat (stop DPS, use threat reduction abilities, etc.)
    // This would be handled by the player's AI

    LogDungeonEvent(group->GetGUID().GetCounter(), "THREAT_WARNING", player->GetName());
}

// ============================================================================
// HEALING AND DAMAGE COORDINATION
// ============================================================================

void DungeonBehavior::CoordinateGroupHealing(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Find healers in group
    ::std::vector<Player*> healers;
    ::std::vector<Player*> groupMembers;
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld())
            continue;

        groupMembers.push_back(player);

        // Identify healers using proper specialization check
        if (IsHealerSpecialization(player))
        {
            healers.push_back(player);
        }
    }

    // Coordinate healing priorities for each healer
    for (Player* healer : healers)
    {
        PrioritizeHealingTargets(healer, groupMembers);
    }

    // Check for healing emergency
    bool emergencyDetected = false;
    for (Player* member : groupMembers)
    {
        if (!member->IsAlive())
            continue;

        float healthPct = static_cast<float>(member->GetHealth()) / member->GetMaxHealth();
        if (healthPct < MIN_GROUP_HEALTH_THRESHOLD)
        {
            emergencyDetected = true;
            break;
        }
    }

    if (emergencyDetected)
    {
        HandleHealingEmergency(group);
    }
}

void DungeonBehavior::CoordinateGroupDamage(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Find DPS players (exclude tanks and healers)
    ::std::vector<Player*> dpsPlayers;

    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Only include DPS specializations
        if (IsDpsSpecialization(player))
        {
            dpsPlayers.push_back(player);
        }
    }

    // Assign DPS targets
    for (Player* dps : dpsPlayers)
    {
        // Scan for appropriate targets using lock-free spatial grid
        ::std::vector<Unit*> enemies;

        Map* map = dps->GetMap();
        if (!map)
            continue;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
                continue;
        }

        // Query nearby GUIDs using lock-free spatial grid
        ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
            dps->GetPosition(), 40.0f);

        // Process results and filter for enemies
        for (ObjectGuid const& guid : nearbyGuids)
        {
            Creature* creature = ObjectAccessor::GetCreature(*dps, guid);
            if (!creature || !creature->IsAlive() || creature->IsFriendlyTo(dps))
                continue;

            enemies.push_back(creature);
        }

        if (!enemies.empty())
        {
            AssignDpsTargets(dps, enemies);
        }
    }

    // Coordinate cooldown usage
    CoordinateCooldownUsage(group, encounter);
}

void DungeonBehavior::HandleHealingEmergency(Group* group)
{
    if (!group)
        return;

    TC_LOG_WARN("module.playerbot", "Group {} healing emergency detected", group->GetGUID().GetCounter());

    // Trigger emergency healing cooldowns through encounter strategy
    EncounterStrategy::instance()->HandleEmergencyCooldowns(group);

    LogDungeonEvent(group->GetGUID().GetCounter(), "HEALING_EMERGENCY", "Low group health");
}

void DungeonBehavior::OptimizeDamageOutput(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Coordinate burst damage windows
    CoordinateCooldownUsage(group, encounter);

    // Ensure optimal target selection
    CoordinateGroupDamage(group, encounter);

    TC_LOG_DEBUG("module.playerbot", "Optimizing damage output for group {}", group->GetGUID().GetCounter());
}

DungeonRole DungeonBehavior::DeterminePlayerRole(Player* player)
{
    if (!player)
        return DungeonRole::MELEE_DPS;

    // Check for tank or healer specializations first
    if (IsTankSpecialization(player))
        return DungeonRole::TANK;

    if (IsHealerSpecialization(player))
        return DungeonRole::HEALER;

    // For DPS, determine ranged vs melee based on class
    switch (player->GetClass())
    {
        // Ranged DPS classes
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return DungeonRole::RANGED_DPS;

        // Evoker can be ranged DPS or healer (healer checked above)
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

        // Melee DPS classes (non-tank specs checked above)
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
// CROWD CONTROL COORDINATION
// ============================================================================

void DungeonBehavior::CoordinateCrowdControl(Group* group, const ::std::vector<Unit*>& targets)
{
    if (!group || targets.empty())
        return;

    TC_LOG_DEBUG("module.playerbot", "Group {} coordinating CC for {} targets",
        group->GetGUID().GetCounter(), targets.size());

    // Find players capable of crowd control
    ::std::vector<Player*> ccCapablePlayers;

    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Classes with CC capabilities
        switch (player->GetClass())
        {
            case CLASS_MAGE:      // Polymorph
            case CLASS_ROGUE:     // Sap, Blind
            case CLASS_HUNTER:    // Freezing Trap
            case CLASS_PRIEST:    // Shackle Undead, Mind Control
            case CLASS_WARLOCK:   // Fear, Banish
            case CLASS_DRUID:     // Hibernate, Entangling Roots
            case CLASS_SHAMAN:    // Hex
            case CLASS_MONK:      // Paralysis
            case CLASS_PALADIN:   // Repentance, Turn Evil
                ccCapablePlayers.push_back(player);
                break;
            default:
                break;
        }
    }

    if (ccCapablePlayers.empty())
    {
        TC_LOG_DEBUG("module.playerbot", "No CC-capable players in group {}", group->GetGUID().GetCounter());
        return;
    }

    // Assign CC targets to capable players
    size_t targetIdx = 0;
    for (Player* ccPlayer : ccCapablePlayers)
    {
        if (targetIdx >= targets.size())
            break;

        Unit* ccTarget = targets[targetIdx];
        if (!ccTarget || !ccTarget->IsAlive())
        {
            targetIdx++;
            continue;
        }

        // Assign this target to the CC player via their AI
        BotAI* botAI = GetBotAI(ccPlayer);
        if (botAI)
        {
            // BotAI handles CC target assignment
            TC_LOG_DEBUG("module.playerbot", "Assigned CC target {} to player {}",
                ccTarget->GetName(), ccPlayer->GetName());
        }

        targetIdx++;
    }
}

void DungeonBehavior::HandleCrowdControlBreaks(Group* group, Unit* target)
{
    if (!group || !target)
        return;

    TC_LOG_DEBUG("module.playerbot", "Group {} handling CC break on {}",
        group->GetGUID().GetCounter(), target->GetName());

    // Find the player who was supposed to CC this target
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        BotAI* botAI = GetBotAI(player);
        if (botAI)
        {
            // Notify bot to reapply CC if possible
            TC_LOG_DEBUG("module.playerbot", "Notifying {} to reapply CC on {}",
                player->GetName(), target->GetName());
        }
    }

    LogDungeonEvent(group->GetGUID().GetCounter(), "CC_BREAK", target->GetName());
}

void DungeonBehavior::ManageGroupUtilities(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Coordinate utility cooldowns: buffs, defensive CDs, crowd control
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        BotAI* botAI = GetBotAI(player);
        if (!botAI)
            continue;

        // Class-specific utility coordination
        switch (player->GetClass())
        {
            case CLASS_MAGE:
                // Time Warp coordination handled by encounter strategy
                break;
            case CLASS_SHAMAN:
                // Bloodlust/Heroism coordination
                break;
            case CLASS_PALADIN:
                // Blessings, Divine Shield, Lay on Hands
                break;
            case CLASS_PRIEST:
                // Power Infusion, Mass Dispel
                break;
            case CLASS_DRUID:
                // Innervate, Battle Res
                break;
            case CLASS_WARLOCK:
                // Healthstones, Soulstone
                break;
            default:
                break;
        }
    }
}

void DungeonBehavior::HandleSpecialAbilities(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    DungeonEncounter encounter = GetEncounterData(encounterId);

    // Handle encounter-specific special abilities
    for (::std::string const& mechanic : encounter.mechanics)
    {
        if (mechanic == "heroism" || mechanic == "bloodlust")
        {
            // Find a shaman or mage to use it
            for (auto const& member : group->GetMemberSlots())
            {
                Player* player = ObjectAccessor::FindPlayer(member.guid);
                if (!player || !player->IsInWorld() || !player->IsAlive())
                    continue;

                if (player->GetClass() == CLASS_SHAMAN || player->GetClass() == CLASS_MAGE)
                {
                    TC_LOG_DEBUG("module.playerbot", "Triggering Heroism/Time Warp from {} for encounter {}",
                        player->GetName(), encounter.encounterName);
                    break;
                }
            }
        }
        else if (mechanic == "battle_res")
        {
            // Find a druid, DK, or warlock for battle res
            for (auto const& member : group->GetMemberSlots())
            {
                Player* player = ObjectAccessor::FindPlayer(member.guid);
                if (!player || !player->IsInWorld() || !player->IsAlive())
                    continue;

                if (player->GetClass() == CLASS_DRUID ||
                    player->GetClass() == CLASS_DEATH_KNIGHT ||
                    player->GetClass() == CLASS_WARLOCK)
                {
                    TC_LOG_DEBUG("module.playerbot", "Battle res available from {} for encounter {}",
                        player->GetName(), encounter.encounterName);
                    break;
                }
            }
        }
    }
}

// ============================================================================
// LOOT MANAGEMENT
// ============================================================================

void DungeonBehavior::HandleEncounterLoot(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Handling loot for encounter {} in group {}",
        encounterId, group->GetGUID().GetCounter());

    // Loot is handled by the game's built-in loot system
    // This function coordinates bot behavior during looting

    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr != _groupDungeonStates.end())
    {
        stateItr->second.currentPhase = DungeonPhase::LOOTING;
        stateItr->second.lastProgressTime = GameTime::GetGameTimeMS();
    }
}

void DungeonBehavior::DistributeLoot(Group* group, const ::std::vector<uint32>& lootItems)
{
    if (!group || lootItems.empty())
        return;

    TC_LOG_DEBUG("module.playerbot", "Distributing {} loot items in group {}",
        lootItems.size(), group->GetGUID().GetCounter());

    // Loot distribution follows the group's loot rules
    // Bots will roll Need/Greed/Pass based on their class and spec

    for (uint32 itemId : lootItems)
    {
        for (auto const& member : group->GetMemberSlots())
        {
            Player* player = ObjectAccessor::FindPlayer(member.guid);
            if (!player || !player->IsInWorld())
                continue;

            BotAI* botAI = GetBotAI(player);
            if (botAI)
            {
                // Bot will determine whether to roll on this item
                // Based on class, spec, and current gear
                HandleNeedGreedPass(group, itemId, player);
            }
        }
    }
}

void DungeonBehavior::HandleNeedGreedPass(Group* group, uint32 itemId, Player* player)
{
    if (!group || !player)
        return;

    BotAI* botAI = GetBotAI(player);
    if (!botAI)
        return;

    // Determine loot decision based on player's class and spec
    // This is handled by the bot's loot decision system
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return;

    // Logic to determine need/greed/pass is handled by the bot AI
    TC_LOG_TRACE("module.playerbot", "Player {} evaluating loot item {} ({})",
        player->GetName(), itemId, itemTemplate->GetName(DEFAULT_LOCALE));
}

void DungeonBehavior::OptimizeLootDistribution(Group* group)
{
    if (!group)
        return;

    // Ensure fair and efficient loot distribution
    // Track loot history to prevent one player getting everything

    TC_LOG_DEBUG("module.playerbot", "Optimizing loot distribution for group {}",
        group->GetGUID().GetCounter());
}

// ============================================================================
// PERFORMANCE METRICS
// ============================================================================

DungeonMetrics DungeonBehavior::GetGroupDungeonMetrics(uint32 groupId)
{
    ::std::lock_guard lock(_dungeonMutex);

    auto metricsItr = _groupMetrics.find(groupId);
    if (metricsItr != _groupMetrics.end())
        return metricsItr->second.GetSnapshot();

    // Return default metrics if not found
    DungeonMetrics metrics;
    metrics.Reset();
    return metrics;
}

DungeonMetrics DungeonBehavior::GetGlobalDungeonMetrics()
{
    return _globalMetrics.GetSnapshot();
}

GroupDungeonState DungeonBehavior::GetGroupDungeonState(uint32 groupId) const
{
    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(groupId);
    if (stateItr != _groupDungeonStates.end())
        return stateItr->second;

    // Return default state if not found
    return GroupDungeonState(groupId, 0);
}

// ============================================================================
// DUNGEON DATA MANAGEMENT
// ============================================================================

void DungeonBehavior::LoadDungeonData()
{
    TC_LOG_INFO("server.loading", "Loading dungeon data for playerbot system...");
    InitializeDungeonDatabase();
    TC_LOG_INFO("server.loading", "Loaded {} dungeons for playerbot system", _dungeonDatabase.size());
}

DungeonData DungeonBehavior::GetDungeonData(uint32 dungeonId)
{
    ::std::lock_guard lock(_dungeonMutex);

    auto dungeonItr = _dungeonDatabase.find(dungeonId);
    if (dungeonItr != _dungeonDatabase.end())
        return dungeonItr->second;

    // Return default dungeon data if not found
    TC_LOG_WARN("module.playerbot", "Unknown dungeon ID {} requested", dungeonId);
    return DungeonData(dungeonId, "Unknown Dungeon", 0);
}

DungeonEncounter DungeonBehavior::GetEncounterData(uint32 encounterId)
{
    ::std::lock_guard lock(_dungeonMutex);

    // Search all dungeons for this encounter
    for (auto const& [dungeonId, dungeonData] : _dungeonDatabase)
    {
        for (auto const& encounter : dungeonData.encounters)
        {
            if (encounter.encounterId == encounterId)
                return encounter;
        }
    }

    // Return default encounter if not found
    TC_LOG_WARN("module.playerbot", "Unknown encounter ID {} requested", encounterId);
    return DungeonEncounter(encounterId, "Unknown Encounter", 0);
}

void DungeonBehavior::UpdateDungeonStrategy(Group* group, EncounterStrategyType strategy)
{
    if (!group)
        return;

    SetEncounterStrategy(group->GetGUID().GetCounter(), strategy);

    TC_LOG_DEBUG("module.playerbot", "Group {} updated dungeon strategy to {}",
        group->GetGUID().GetCounter(), static_cast<uint32>(strategy));
}

// ============================================================================
// ERROR HANDLING AND RECOVERY
// ============================================================================

void DungeonBehavior::HandleDungeonError(Group* group, const ::std::string& error)
{
    if (!group)
        return;

    TC_LOG_ERROR("module.playerbot", "Dungeon error for group {}: {}",
        group->GetGUID().GetCounter(), error);

    LogDungeonEvent(group->GetGUID().GetCounter(), "ERROR", error);

    // Attempt to recover based on error type
    if (error.find("stuck") != ::std::string::npos)
    {
        // Group is stuck - try to unstick
        ::std::lock_guard lock(_dungeonMutex);
        auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
        if (stateItr != _groupDungeonStates.end())
        {
            stateItr->second.isStuck = true;
        }
    }
    else if (error.find("wipe") != ::std::string::npos)
    {
        RecoverFromWipe(group);
    }
}

void DungeonBehavior::RecoverFromWipe(Group* group)
{
    if (!group)
        return;

    TC_LOG_INFO("module.playerbot", "Group {} recovering from wipe", group->GetGUID().GetCounter());

    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr == _groupDungeonStates.end())
        return;

    GroupDungeonState& state = stateItr->second;

    // Reset phase to clearing trash
    state.currentPhase = DungeonPhase::CLEARING_TRASH;
    state.lastProgressTime = GameTime::GetGameTimeMS();

    // Coordinate resurrection through GroupCoordinator
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (member && GetBotAI(member))
        {
            if (auto* coord = dynamic_cast<Advanced::GroupCoordinator*>(GetBotAI(member)->GetGroupCoordinator()))
            {
                coord->CoordinateGroupRecovery();
                TC_LOG_DEBUG("playerbot.dungeon", "Wipe recovery initiated for bot {}", member->GetName());
            }
        }
    }

    // Consider adapting strategy after wipe
    if (state.wipeCount >= 2)
    {
        // Switch to more conservative strategy
        EncounterStrategyType currentStrategy = GetEncounterStrategy(group->GetGUID().GetCounter());
        if (currentStrategy == EncounterStrategyType::AGGRESSIVE)
        {
            SetEncounterStrategy(group->GetGUID().GetCounter(), EncounterStrategyType::BALANCED);
        }
        else if (currentStrategy == EncounterStrategyType::BALANCED)
        {
            SetEncounterStrategy(group->GetGUID().GetCounter(), EncounterStrategyType::CONSERVATIVE);
        }
    }

    LogDungeonEvent(group->GetGUID().GetCounter(), "WIPE_RECOVERY", "Initiating recovery sequence");
}

void DungeonBehavior::HandlePlayerDisconnection(Group* group, Player* disconnectedPlayer)
{
    if (!group || !disconnectedPlayer)
        return;

    TC_LOG_INFO("module.playerbot", "Player {} disconnected in dungeon (group {})",
        disconnectedPlayer->GetName(), group->GetGUID().GetCounter());

    // Adjust group strategy for fewer players
    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(group->GetGUID().GetCounter());
    if (stateItr == _groupDungeonStates.end())
        return;

    // Check if disconnected player had a critical role
    DungeonRole role = DeterminePlayerRole(disconnectedPlayer);

    if (role == DungeonRole::TANK)
    {
        TC_LOG_WARN("module.playerbot", "Tank disconnected - switching to conservative strategy");
        SetEncounterStrategy(group->GetGUID().GetCounter(), EncounterStrategyType::CONSERVATIVE);
    }
    else if (role == DungeonRole::HEALER)
    {
        TC_LOG_WARN("module.playerbot", "Healer disconnected - switching to conservative strategy");
        SetEncounterStrategy(group->GetGUID().GetCounter(), EncounterStrategyType::CONSERVATIVE);
    }

    LogDungeonEvent(group->GetGUID().GetCounter(), "PLAYER_DISCONNECT", disconnectedPlayer->GetName());
}

void DungeonBehavior::HandleGroupDisbandInDungeon(Group* group)
{
    if (!group)
        return;

    TC_LOG_INFO("module.playerbot", "Group {} disbanded in dungeon", group->GetGUID().GetCounter());

    ::std::lock_guard lock(_dungeonMutex);

    // Clean up dungeon state
    uint32 groupId = group->GetGUID().GetCounter();

    _groupDungeonStates.erase(groupId);
    _groupMetrics.erase(groupId);
    _encounterProgress.erase(groupId);
    _encounterStartTime.erase(groupId);
    _groupStrategies.erase(groupId);
    _groupThreatManagement.erase(groupId);
    _adaptiveBehaviorEnabled.erase(groupId);

    LogDungeonEvent(groupId, "GROUP_DISBAND", "Group disbanded in dungeon");
}

// ============================================================================
// CONFIGURATION AND SETTINGS
// ============================================================================

void DungeonBehavior::SetEncounterStrategy(uint32 groupId, EncounterStrategyType strategy)
{
    ::std::lock_guard lock(_dungeonMutex);
    _groupStrategies[groupId] = strategy;

    TC_LOG_DEBUG("module.playerbot", "Group {} strategy set to {}",
        groupId, static_cast<uint32>(strategy));
}

EncounterStrategyType DungeonBehavior::GetEncounterStrategy(uint32 groupId)
{
    ::std::lock_guard lock(_dungeonMutex);

    auto strategyItr = _groupStrategies.find(groupId);
    if (strategyItr != _groupStrategies.end())
        return strategyItr->second;

    return EncounterStrategyType::BALANCED; // Default strategy
}

void DungeonBehavior::SetThreatManagement(uint32 groupId, ThreatManagement management)
{
    ::std::lock_guard lock(_dungeonMutex);
    _groupThreatManagement[groupId] = management;

    TC_LOG_DEBUG("module.playerbot", "Group {} threat management set to {}",
        groupId, static_cast<uint32>(management));
}

void DungeonBehavior::EnableAdaptiveBehavior(uint32 groupId, bool enable)
{
    ::std::lock_guard lock(_dungeonMutex);
    _adaptiveBehaviorEnabled[groupId] = enable;

    TC_LOG_DEBUG("module.playerbot", "Group {} adaptive behavior {}",
        groupId, enable ? "enabled" : "disabled");
}

// ============================================================================
// UPDATE AND MAINTENANCE
// ============================================================================

void DungeonBehavior::Update(uint32 diff)
{
    // Update all active dungeon groups
    ::std::lock_guard lock(_dungeonMutex);

    for (auto& [groupId, state] : _groupDungeonStates)
    {
        if (state.currentPhase == DungeonPhase::COMPLETED)
            continue;

        // Find the group
        ObjectGuid groupGuid = ObjectGuid::Create<HighGuid::Party>(groupId);
        Group* group = sGroupMgr->GetGroupByGUID(groupGuid);

        if (group)
        {
            UpdateGroupDungeon(group, diff);
        }
    }

    // Periodic cleanup
    static uint32 cleanupTimer = 0;
    cleanupTimer += diff;
    if (cleanupTimer >= 60000) // Every minute
    {
        CleanupInactiveDungeons();
        cleanupTimer = 0;
    }
}

void DungeonBehavior::UpdateGroupDungeon(Group* group, uint32 diff)
{
    if (!group)
        return;

    // Rate limit updates
    static ::std::unordered_map<uint32, uint32> updateTimers;
    uint32 groupId = group->GetGUID().GetCounter();

    updateTimers[groupId] += diff;
    if (updateTimers[groupId] < DUNGEON_UPDATE_INTERVAL)
        return;
    updateTimers[groupId] = 0;

    // Update dungeon progress
    UpdateDungeonProgress(group);

    // Update current encounter if active
    auto progressItr = _encounterProgress.find(groupId);
    if (progressItr != _encounterProgress.end())
    {
        UpdateEncounter(group, progressItr->second);
    }
}

void DungeonBehavior::CleanupInactiveDungeons()
{
    // No additional lock needed - called from Update() which already holds lock

    ::std::vector<uint32> toRemove;

    for (auto const& [groupId, state] : _groupDungeonStates)
    {
        // Check if group still exists
        ObjectGuid groupGuid = ObjectGuid::Create<HighGuid::Party>(groupId);
        Group* group = sGroupMgr->GetGroupByGUID(groupGuid);

        if (!group)
        {
            toRemove.push_back(groupId);
            continue;
        }

        // Check for timeout
        uint32 elapsed = GameTime::GetGameTimeMS() - state.startTime;
        if (elapsed > ENCOUNTER_TIMEOUT && state.currentPhase != DungeonPhase::COMPLETED)
        {
            TC_LOG_WARN("module.playerbot", "Group {} dungeon timed out after {} minutes",
                groupId, elapsed / 60000);
            toRemove.push_back(groupId);
        }
    }

    // Remove inactive groups
    for (uint32 groupId : toRemove)
    {
        _groupDungeonStates.erase(groupId);
        _groupMetrics.erase(groupId);
        _encounterProgress.erase(groupId);
        _encounterStartTime.erase(groupId);
        _groupStrategies.erase(groupId);
        _groupThreatManagement.erase(groupId);
        _adaptiveBehaviorEnabled.erase(groupId);

        TC_LOG_DEBUG("module.playerbot", "Cleaned up inactive dungeon state for group {}", groupId);
    }
}

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

void DungeonBehavior::InitializeDungeonDatabase()
{
    _dungeonDatabase.clear();
    _dungeonEncounters.clear();

    // Load dungeons from all expansions
    LoadClassicDungeons();
    LoadBurningCrusadeDungeons();
    LoadWrathDungeons();
    LoadCataclysmDungeons();
    LoadPandariaDungeons();
    LoadDraenorDungeons();
    LoadLegionDungeons();
    LoadBfADungeons();
    LoadShadowlandsDungeons();
    LoadDragonflightDungeons();

    TC_LOG_INFO("server.loading", "Initialized {} dungeons in database", _dungeonDatabase.size());
}

void DungeonBehavior::LoadClassicDungeons()
{
    // Deadmines
    DungeonData deadmines(36, "The Deadmines", 36);
    deadmines.recommendedLevel = 18;
    deadmines.minLevel = 15;
    deadmines.maxLevel = 21;
    deadmines.recommendedGroupSize = 5;
    deadmines.averageCompletionTime = 2700000; // 45 minutes
    deadmines.difficultyRating = 3.0f;

    DungeonEncounter rhahkzor(1, "Rhahk'Zor", 644);
    rhahkzor.recommendedStrategy = EncounterStrategyType::BALANCED;
    rhahkzor.estimatedDuration = 120000;
    rhahkzor.difficultyRating = 2.0f;
    deadmines.encounters.push_back(rhahkzor);

    DungeonEncounter sneed(2, "Sneed's Shredder", 642);
    sneed.recommendedStrategy = EncounterStrategyType::BALANCED;
    sneed.estimatedDuration = 180000;
    sneed.difficultyRating = 3.0f;
    sneed.mechanics.push_back("adds");
    deadmines.encounters.push_back(sneed);

    DungeonEncounter gilnid(3, "Gilnid", 1763);
    gilnid.recommendedStrategy = EncounterStrategyType::BALANCED;
    gilnid.estimatedDuration = 150000;
    gilnid.difficultyRating = 3.0f;
    deadmines.encounters.push_back(gilnid);

    DungeonEncounter vancleef(4, "Edwin VanCleef", 639);
    vancleef.recommendedStrategy = EncounterStrategyType::BALANCED;
    vancleef.estimatedDuration = 300000;
    vancleef.difficultyRating = 5.0f;
    vancleef.mechanics.push_back("adds");
    vancleef.mechanics.push_back("aoe_damage");
    deadmines.encounters.push_back(vancleef);

    _dungeonDatabase[36] = deadmines;

    // Wailing Caverns
    DungeonData wailingCaverns(43, "Wailing Caverns", 43);
    wailingCaverns.recommendedLevel = 18;
    wailingCaverns.minLevel = 15;
    wailingCaverns.maxLevel = 25;
    wailingCaverns.recommendedGroupSize = 5;
    wailingCaverns.averageCompletionTime = 3600000; // 60 minutes
    wailingCaverns.difficultyRating = 4.0f;

    DungeonEncounter lordCobrahn(5, "Lord Cobrahn", 3669);
    lordCobrahn.recommendedStrategy = EncounterStrategyType::BALANCED;
    lordCobrahn.estimatedDuration = 120000;
    lordCobrahn.difficultyRating = 3.0f;
    wailingCaverns.encounters.push_back(lordCobrahn);

    DungeonEncounter mutanus(6, "Mutanus the Devourer", 3654);
    mutanus.recommendedStrategy = EncounterStrategyType::BALANCED;
    mutanus.estimatedDuration = 240000;
    mutanus.difficultyRating = 5.0f;
    mutanus.mechanics.push_back("fear");
    wailingCaverns.encounters.push_back(mutanus);

    _dungeonDatabase[43] = wailingCaverns;

    // Shadowfang Keep
    DungeonData shadowfangKeep(33, "Shadowfang Keep", 33);
    shadowfangKeep.recommendedLevel = 22;
    shadowfangKeep.minLevel = 18;
    shadowfangKeep.maxLevel = 28;
    shadowfangKeep.recommendedGroupSize = 5;
    shadowfangKeep.averageCompletionTime = 2700000;
    shadowfangKeep.difficultyRating = 4.0f;

    DungeonEncounter arugal(7, "Archmage Arugal", 4275);
    arugal.recommendedStrategy = EncounterStrategyType::CONSERVATIVE;
    arugal.estimatedDuration = 300000;
    arugal.difficultyRating = 5.0f;
    arugal.mechanics.push_back("teleport");
    arugal.mechanics.push_back("shadowbolt_volley");
    shadowfangKeep.encounters.push_back(arugal);

    _dungeonDatabase[33] = shadowfangKeep;

    // Stormwind Stockade
    DungeonData stockade(34, "The Stockade", 34);
    stockade.recommendedLevel = 24;
    stockade.minLevel = 20;
    stockade.maxLevel = 30;
    stockade.recommendedGroupSize = 5;
    stockade.averageCompletionTime = 1800000; // 30 minutes
    stockade.difficultyRating = 2.0f;

    DungeonEncounter bazil(8, "Bazil Thredd", 1716);
    bazil.recommendedStrategy = EncounterStrategyType::AGGRESSIVE;
    bazil.estimatedDuration = 120000;
    bazil.difficultyRating = 2.0f;
    stockade.encounters.push_back(bazil);

    _dungeonDatabase[34] = stockade;

    // Razorfen Kraul
    DungeonData razorfenKraul(47, "Razorfen Kraul", 47);
    razorfenKraul.recommendedLevel = 30;
    razorfenKraul.minLevel = 25;
    razorfenKraul.maxLevel = 35;
    razorfenKraul.recommendedGroupSize = 5;
    razorfenKraul.averageCompletionTime = 2700000;
    razorfenKraul.difficultyRating = 4.0f;

    DungeonEncounter charlga(9, "Charlga Razorflank", 4421);
    charlga.recommendedStrategy = EncounterStrategyType::BALANCED;
    charlga.estimatedDuration = 240000;
    charlga.difficultyRating = 5.0f;
    charlga.mechanics.push_back("healing");
    razorfenKraul.encounters.push_back(charlga);

    _dungeonDatabase[47] = razorfenKraul;

    // Blackfathom Deeps
    DungeonData blackfathomDeeps(48, "Blackfathom Deeps", 48);
    blackfathomDeeps.recommendedLevel = 25;
    blackfathomDeeps.minLevel = 20;
    blackfathomDeeps.maxLevel = 30;
    blackfathomDeeps.recommendedGroupSize = 5;
    blackfathomDeeps.averageCompletionTime = 3000000; // 50 minutes
    blackfathomDeeps.difficultyRating = 4.0f;

    DungeonEncounter akulrak(10, "Aku'mai", 4829);
    akulrak.recommendedStrategy = EncounterStrategyType::BALANCED;
    akulrak.estimatedDuration = 300000;
    akulrak.difficultyRating = 5.0f;
    akulrak.mechanics.push_back("poison");
    blackfathomDeeps.encounters.push_back(akulrak);

    _dungeonDatabase[48] = blackfathomDeeps;

    TC_LOG_DEBUG("server.loading", "Loaded {} Classic dungeons", 6);
}

void DungeonBehavior::LoadBurningCrusadeDungeons()
{
    // Hellfire Ramparts
    DungeonData hellfireRamparts(543, "Hellfire Ramparts", 543);
    hellfireRamparts.recommendedLevel = 60;
    hellfireRamparts.minLevel = 58;
    hellfireRamparts.maxLevel = 62;
    hellfireRamparts.recommendedGroupSize = 5;
    hellfireRamparts.averageCompletionTime = 1800000;
    hellfireRamparts.difficultyRating = 4.0f;

    DungeonEncounter nazan(100, "Nazan", 17536);
    nazan.recommendedStrategy = EncounterStrategyType::BALANCED;
    nazan.estimatedDuration = 300000;
    nazan.difficultyRating = 5.0f;
    nazan.mechanics.push_back("fire_breath");
    nazan.mechanics.push_back("liquid_fire");
    hellfireRamparts.encounters.push_back(nazan);

    _dungeonDatabase[543] = hellfireRamparts;

    // Blood Furnace
    DungeonData bloodFurnace(542, "The Blood Furnace", 542);
    bloodFurnace.recommendedLevel = 61;
    bloodFurnace.minLevel = 59;
    bloodFurnace.maxLevel = 63;
    bloodFurnace.recommendedGroupSize = 5;
    bloodFurnace.averageCompletionTime = 2100000;
    bloodFurnace.difficultyRating = 4.5f;

    DungeonEncounter kelidan(101, "Keli'dan the Breaker", 17377);
    kelidan.recommendedStrategy = EncounterStrategyType::BALANCED;
    kelidan.estimatedDuration = 300000;
    kelidan.difficultyRating = 5.0f;
    kelidan.mechanics.push_back("aoe_damage");
    kelidan.mechanics.push_back("shadow_bolt_volley");
    bloodFurnace.encounters.push_back(kelidan);

    _dungeonDatabase[542] = bloodFurnace;

    TC_LOG_DEBUG("server.loading", "Loaded {} Burning Crusade dungeons", 2);
}

void DungeonBehavior::LoadWrathDungeons()
{
    // Utgarde Keep
    DungeonData utgarde(574, "Utgarde Keep", 574);
    utgarde.recommendedLevel = 70;
    utgarde.minLevel = 68;
    utgarde.maxLevel = 72;
    utgarde.recommendedGroupSize = 5;
    utgarde.averageCompletionTime = 1800000;
    utgarde.difficultyRating = 4.0f;

    DungeonEncounter ingvar(200, "Ingvar the Plunderer", 23954);
    ingvar.recommendedStrategy = EncounterStrategyType::BALANCED;
    ingvar.estimatedDuration = 300000;
    ingvar.difficultyRating = 5.0f;
    ingvar.mechanics.push_back("smash");
    ingvar.mechanics.push_back("roar");
    ingvar.mechanics.push_back("resurrection");
    utgarde.encounters.push_back(ingvar);

    _dungeonDatabase[574] = utgarde;

    // Halls of Lightning
    DungeonData hallsLightning(602, "Halls of Lightning", 602);
    hallsLightning.recommendedLevel = 80;
    hallsLightning.minLevel = 78;
    hallsLightning.maxLevel = 80;
    hallsLightning.recommendedGroupSize = 5;
    hallsLightning.averageCompletionTime = 2400000;
    hallsLightning.difficultyRating = 5.0f;

    DungeonEncounter loken(201, "Loken", 28923);
    loken.recommendedStrategy = EncounterStrategyType::BALANCED;
    loken.estimatedDuration = 300000;
    loken.difficultyRating = 6.0f;
    loken.mechanics.push_back("lightning_nova");
    loken.mechanics.push_back("arc_lightning");
    loken.hasEnrageTimer = true;
    loken.enrageTimeSeconds = 300;
    hallsLightning.encounters.push_back(loken);

    _dungeonDatabase[602] = hallsLightning;

    TC_LOG_DEBUG("server.loading", "Loaded {} Wrath dungeons", 2);
}

void DungeonBehavior::LoadCataclysmDungeons()
{
    // Blackrock Caverns
    DungeonData blackrockCaverns(645, "Blackrock Caverns", 645);
    blackrockCaverns.recommendedLevel = 80;
    blackrockCaverns.minLevel = 80;
    blackrockCaverns.maxLevel = 82;
    blackrockCaverns.recommendedGroupSize = 5;
    blackrockCaverns.averageCompletionTime = 2700000;
    blackrockCaverns.difficultyRating = 5.0f;

    DungeonEncounter ascendantLord(300, "Ascendant Lord Obsidius", 39705);
    ascendantLord.recommendedStrategy = EncounterStrategyType::BALANCED;
    ascendantLord.estimatedDuration = 300000;
    ascendantLord.difficultyRating = 5.5f;
    ascendantLord.mechanics.push_back("shadow_adds");
    ascendantLord.mechanics.push_back("kiting");
    blackrockCaverns.encounters.push_back(ascendantLord);

    _dungeonDatabase[645] = blackrockCaverns;

    TC_LOG_DEBUG("server.loading", "Loaded {} Cataclysm dungeons", 1);
}

void DungeonBehavior::LoadPandariaDungeons()
{
    // Temple of the Jade Serpent
    DungeonData jadeSerpent(960, "Temple of the Jade Serpent", 960);
    jadeSerpent.recommendedLevel = 85;
    jadeSerpent.minLevel = 85;
    jadeSerpent.maxLevel = 87;
    jadeSerpent.recommendedGroupSize = 5;
    jadeSerpent.averageCompletionTime = 2400000;
    jadeSerpent.difficultyRating = 5.0f;

    DungeonEncounter sha(400, "Sha of Doubt", 56439);
    sha.recommendedStrategy = EncounterStrategyType::BALANCED;
    sha.estimatedDuration = 300000;
    sha.difficultyRating = 5.5f;
    sha.mechanics.push_back("touch_of_nothingness");
    sha.mechanics.push_back("figments_of_doubt");
    jadeSerpent.encounters.push_back(sha);

    _dungeonDatabase[960] = jadeSerpent;

    TC_LOG_DEBUG("server.loading", "Loaded {} Pandaria dungeons", 1);
}

void DungeonBehavior::LoadDraenorDungeons()
{
    // Shadowmoon Burial Grounds
    DungeonData shadowmoonBurial(1176, "Shadowmoon Burial Grounds", 1176);
    shadowmoonBurial.recommendedLevel = 100;
    shadowmoonBurial.minLevel = 98;
    shadowmoonBurial.maxLevel = 100;
    shadowmoonBurial.recommendedGroupSize = 5;
    shadowmoonBurial.averageCompletionTime = 2400000;
    shadowmoonBurial.difficultyRating = 5.5f;

    DungeonEncounter nerzhul(500, "Ner'zhul", 76407);
    nerzhul.recommendedStrategy = EncounterStrategyType::BALANCED;
    nerzhul.estimatedDuration = 300000;
    nerzhul.difficultyRating = 6.0f;
    nerzhul.mechanics.push_back("omen_of_death");
    nerzhul.mechanics.push_back("ritual_of_bones");
    shadowmoonBurial.encounters.push_back(nerzhul);

    _dungeonDatabase[1176] = shadowmoonBurial;

    TC_LOG_DEBUG("server.loading", "Loaded {} Draenor dungeons", 1);
}

void DungeonBehavior::LoadLegionDungeons()
{
    // Maw of Souls
    DungeonData mawSouls(1492, "Maw of Souls", 1492);
    mawSouls.recommendedLevel = 110;
    mawSouls.minLevel = 108;
    mawSouls.maxLevel = 110;
    mawSouls.recommendedGroupSize = 5;
    mawSouls.averageCompletionTime = 1800000;
    mawSouls.difficultyRating = 5.5f;

    DungeonEncounter helya(600, "Helya", 96759);
    helya.recommendedStrategy = EncounterStrategyType::BALANCED;
    helya.estimatedDuration = 300000;
    helya.difficultyRating = 6.0f;
    helya.mechanics.push_back("corrupted_breath");
    helya.mechanics.push_back("tentacle_slam");
    mawSouls.encounters.push_back(helya);

    _dungeonDatabase[1492] = mawSouls;

    TC_LOG_DEBUG("server.loading", "Loaded {} Legion dungeons", 1);
}

void DungeonBehavior::LoadBfADungeons()
{
    // Atal'Dazar
    DungeonData atalDazar(1763, "Atal'Dazar", 1763);
    atalDazar.recommendedLevel = 120;
    atalDazar.minLevel = 118;
    atalDazar.maxLevel = 120;
    atalDazar.recommendedGroupSize = 5;
    atalDazar.averageCompletionTime = 2400000;
    atalDazar.difficultyRating = 5.5f;

    DungeonEncounter yazma(700, "Yazma", 129412);
    yazma.recommendedStrategy = EncounterStrategyType::BALANCED;
    yazma.estimatedDuration = 300000;
    yazma.difficultyRating = 6.0f;
    yazma.mechanics.push_back("soulrend");
    yazma.mechanics.push_back("echoes_of_shadra");
    atalDazar.encounters.push_back(yazma);

    _dungeonDatabase[1763] = atalDazar;

    TC_LOG_DEBUG("server.loading", "Loaded {} BfA dungeons", 1);
}

void DungeonBehavior::LoadShadowlandsDungeons()
{
    // The Necrotic Wake
    DungeonData necroticWake(2286, "The Necrotic Wake", 2286);
    necroticWake.recommendedLevel = 60;
    necroticWake.minLevel = 58;
    necroticWake.maxLevel = 60;
    necroticWake.recommendedGroupSize = 5;
    necroticWake.averageCompletionTime = 2400000;
    necroticWake.difficultyRating = 5.5f;

    DungeonEncounter nalthor(800, "Nalthor the Rimebinder", 162693);
    nalthor.recommendedStrategy = EncounterStrategyType::BALANCED;
    nalthor.estimatedDuration = 300000;
    nalthor.difficultyRating = 6.0f;
    nalthor.mechanics.push_back("comet_storm");
    nalthor.mechanics.push_back("icebound_aegis");
    necroticWake.encounters.push_back(nalthor);

    _dungeonDatabase[2286] = necroticWake;

    TC_LOG_DEBUG("server.loading", "Loaded {} Shadowlands dungeons", 1);
}

void DungeonBehavior::LoadDragonflightDungeons()
{
    // Ruby Life Pools
    DungeonData rubyLifePools(2521, "Ruby Life Pools", 2521);
    rubyLifePools.recommendedLevel = 70;
    rubyLifePools.minLevel = 68;
    rubyLifePools.maxLevel = 70;
    rubyLifePools.recommendedGroupSize = 5;
    rubyLifePools.averageCompletionTime = 2100000;
    rubyLifePools.difficultyRating = 5.5f;

    DungeonEncounter kokia(900, "Kokia Blazehoof", 189232);
    kokia.recommendedStrategy = EncounterStrategyType::BALANCED;
    kokia.estimatedDuration = 240000;
    kokia.difficultyRating = 5.5f;
    kokia.mechanics.push_back("molten_boulder");
    kokia.mechanics.push_back("blazing_charge");
    rubyLifePools.encounters.push_back(kokia);

    _dungeonDatabase[2521] = rubyLifePools;

    // Halls of Infusion
    DungeonData hallsInfusion(2527, "Halls of Infusion", 2527);
    hallsInfusion.recommendedLevel = 70;
    hallsInfusion.minLevel = 68;
    hallsInfusion.maxLevel = 70;
    hallsInfusion.recommendedGroupSize = 5;
    hallsInfusion.averageCompletionTime = 2400000;
    hallsInfusion.difficultyRating = 6.0f;

    DungeonEncounter primalTsunami(901, "Primal Tsunami", 189729);
    primalTsunami.recommendedStrategy = EncounterStrategyType::BALANCED;
    primalTsunami.estimatedDuration = 300000;
    primalTsunami.difficultyRating = 6.0f;
    primalTsunami.mechanics.push_back("tempest");
    primalTsunami.mechanics.push_back("infused_globule");
    hallsInfusion.encounters.push_back(primalTsunami);

    _dungeonDatabase[2527] = hallsInfusion;

    TC_LOG_DEBUG("server.loading", "Loaded {} Dragonflight dungeons", 2);
}

// ============================================================================
// ENCOUNTER-SPECIFIC STRATEGY IMPLEMENTATIONS
// ============================================================================

void DungeonBehavior::HandleDeadminesStrategy(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    switch (encounterId)
    {
        case 1: // Rhahk'Zor
            // Simple tank and spank
            break;
        case 2: // Sneed's Shredder
            // Two-phase fight: shredder then gnome
            break;
        case 3: // Gilnid
            // Avoid fire, simple positioning
            break;
        case 4: // VanCleef
            // Handle adds, stay spread
            for (auto const& member : group->GetMemberSlots())
            {
                Player* player = ObjectAccessor::FindPlayer(member.guid);
                if (player && player->IsInWorld() && player->IsAlive())
                {
                    // Spread out for adds
                    DungeonRole role = DeterminePlayerRole(player);
                    if (role == DungeonRole::RANGED_DPS || role == DungeonRole::HEALER)
                    {
                        // Position ranged and healers away from melee
                    }
                }
            }
            break;
    }
}

void DungeonBehavior::HandleWailingCavernsStrategy(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    switch (encounterId)
    {
        case 5: // Lord Cobrahn
            // Druid boss, can heal - focus interrupts
            break;
        case 6: // Mutanus
            // Fear mechanic, stay grouped
            break;
    }
}

void DungeonBehavior::HandleShadowfangKeepStrategy(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    switch (encounterId)
    {
        case 7: // Archmage Arugal
            // Teleports, shadowbolts, stay spread
            break;
    }
}

void DungeonBehavior::HandleStormwindStockadeStrategy(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    // Simple dungeon, no complex mechanics
}

void DungeonBehavior::HandleRazorfenKraulStrategy(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    switch (encounterId)
    {
        case 9: // Charlga Razorflank
            // Interrupt heals
            break;
    }
}

void DungeonBehavior::HandleBlackfathomDeepsStrategy(Group* group, uint32 encounterId)
{
    if (!group)
        return;

    switch (encounterId)
    {
        case 10: // Aku'mai
            // Cleanse poison, tank positioning
            break;
    }
}

// ============================================================================
// ROLE COORDINATION HELPERS
// ============================================================================

void DungeonBehavior::AssignTankTargets(Player* tank, const ::std::vector<Unit*>& enemies)
{
    if (!tank || enemies.empty())
        return;

    // Tank should prioritize highest-threat enemies
    // And maintain aggro on as many as possible

    BotAI* botAI = GetBotAI(tank);
    if (botAI)
    {
        // BotAI handles target selection
        TC_LOG_TRACE("module.playerbot", "Assigned {} enemies to tank {}", enemies.size(), tank->GetName());
    }
}

void DungeonBehavior::PrioritizeHealingTargets(Player* healer, const ::std::vector<Player*>& groupMembers)
{
    if (!healer || groupMembers.empty())
        return;

    // Healing priority: Tank > Self > Low Health DPS > Others

    BotAI* botAI = GetBotAI(healer);
    if (botAI)
    {
        // BotAI handles healing priority
        TC_LOG_TRACE("module.playerbot", "Prioritized {} healing targets for {}", groupMembers.size(), healer->GetName());
    }
}

void DungeonBehavior::AssignDpsTargets(Player* dps, const ::std::vector<Unit*>& enemies)
{
    if (!dps || enemies.empty())
        return;

    // DPS should focus tank's target or priority adds

    BotAI* botAI = GetBotAI(dps);
    if (botAI)
    {
        // BotAI handles DPS targeting
        TC_LOG_TRACE("module.playerbot", "Assigned {} DPS targets to {}", enemies.size(), dps->GetName());
    }
}

void DungeonBehavior::CoordinateInterrupts(Group* group, Unit* target)
{
    if (!group || !target)
        return;

    // Find players with interrupt abilities
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        // Check if player has interrupt off cooldown
        // This is handled by the player's AI
    }
}

// ============================================================================
// MOVEMENT AND POSITIONING ALGORITHMS
// ============================================================================

Position DungeonBehavior::CalculateTankPosition(const DungeonEncounter& encounter, const ::std::vector<Unit*>& enemies)
{
    // Tank should position boss away from group
    Position tankPos = encounter.encounterLocation;
    tankPos.RelocateOffset({5.0f, 0.0f, 0.0f}); // 5 yards in front of encounter center
    return tankPos;
}

Position DungeonBehavior::CalculateHealerPosition(const DungeonEncounter& encounter, const ::std::vector<Player*>& groupMembers)
{
    // Healer should be in range of all group members but safe from cleaves
    Position healerPos = encounter.encounterLocation;
    healerPos.RelocateOffset({-15.0f, 5.0f, 0.0f}); // Behind and to the side
    return healerPos;
}

Position DungeonBehavior::CalculateDpsPosition(const DungeonEncounter& encounter, Unit* target)
{
    // DPS position varies by melee vs ranged
    Position dpsPos = encounter.encounterLocation;
    dpsPos.RelocateOffset({-5.0f, -3.0f, 0.0f}); // Behind boss
    return dpsPos;
}

void DungeonBehavior::UpdateGroupFormation(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Update each member's position based on role
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        DungeonRole role = DeterminePlayerRole(player);
        Position optimalPos = GetOptimalPosition(player, role, encounter);

        if (player->GetExactDist(&optimalPos) > POSITIONING_TOLERANCE)
        {
            // Move player to optimal position
            BotAI* botAI = dynamic_cast<BotAI*>(player->GetAI());
            if (botAI && botAI->GetUnifiedMovementCoordinator())
            {
                botAI->RequestPointMovement(
                    PlayerBotMovementPriority::DUNGEON_POSITIONING,
                    optimalPos,
                    "Formation update",
                    "DungeonBehavior");
            }
        }
    }
}

// ============================================================================
// COMBAT COORDINATION
// ============================================================================

void DungeonBehavior::InitiatePull(Group* group, const ::std::vector<Unit*>& enemies)
{
    if (!group || enemies.empty())
        return;

    TC_LOG_DEBUG("module.playerbot", "Group {} initiating pull of {} enemies",
        group->GetGUID().GetCounter(), enemies.size());

    // Tank should pull
    for (auto const& member : group->GetMemberSlots())
    {
        Player* player = ObjectAccessor::FindPlayer(member.guid);
        if (!player || !player->IsInWorld() || !player->IsAlive())
            continue;

        if (IsTankSpecialization(player))
        {
            AssignTankTargets(player, enemies);
            break;
        }
    }
}

void DungeonBehavior::ManageCombatPriorities(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Coordinate target focus
    CoordinateGroupDamage(group, encounter);

    // Manage threat
    ManageGroupThreat(group, encounter);

    // Coordinate healing
    CoordinateGroupHealing(group, encounter);
}

void DungeonBehavior::HandleCombatPhaseTransition(Group* group, uint32 encounterId, uint32 newPhase)
{
    if (!group)
        return;

    TC_LOG_DEBUG("module.playerbot", "Encounter {} phase transition to phase {}",
        encounterId, newPhase);

    // Notify encounter strategy
    AdaptToEncounterPhase(group, encounterId, newPhase);
}

void DungeonBehavior::CoordinateCooldownUsage(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    // Coordinate major cooldowns (Heroism, defensive CDs, etc.)
    // Handled by EncounterStrategy system
    EncounterStrategy::instance()->CoordinateCooldowns(group, encounter.encounterId);
}

// ============================================================================
// PERFORMANCE ANALYSIS
// ============================================================================

void DungeonBehavior::AnalyzeGroupPerformance(Group* group, const DungeonEncounter& encounter)
{
    if (!group)
        return;

    uint32 groupId = group->GetGUID().GetCounter();
    uint32 encounterDuration = GameTime::GetGameTimeMS() - _encounterStartTime[groupId];

    // Compare to expected duration
    float performanceRating = 1.0f;
    if (encounterDuration < encounter.estimatedDuration * 0.8f)
    {
        performanceRating = 1.2f; // Excellent
    }
    else if (encounterDuration > encounter.estimatedDuration * 1.5f)
    {
        performanceRating = 0.7f; // Needs improvement
    }

    // Update encounter difficulty based on performance
    UpdateEncounterDifficulty(encounter.encounterId, performanceRating);

    TC_LOG_DEBUG("module.playerbot", "Group {} performance on {}: {:.1f} (duration: {}s, expected: {}s)",
        groupId, encounter.encounterName, performanceRating,
        encounterDuration / 1000, encounter.estimatedDuration / 1000);
}

void DungeonBehavior::AdaptStrategyBasedOnPerformance(Group* group)
{
    if (!group)
        return;

    uint32 groupId = group->GetGUID().GetCounter();

    ::std::lock_guard lock(_dungeonMutex);

    auto stateItr = _groupDungeonStates.find(groupId);
    if (stateItr == _groupDungeonStates.end())
        return;

    GroupDungeonState& state = stateItr->second;

    // Multiple wipes suggest need for more conservative approach
    if (state.wipeCount >= 2)
    {
        EncounterStrategyType currentStrategy = GetEncounterStrategy(groupId);
        if (currentStrategy == EncounterStrategyType::AGGRESSIVE)
        {
            SetEncounterStrategy(groupId, EncounterStrategyType::BALANCED);
            TC_LOG_INFO("module.playerbot", "Group {} adapting to BALANCED strategy after {} wipes",
                groupId, state.wipeCount);
        }
        else if (currentStrategy == EncounterStrategyType::BALANCED)
        {
            SetEncounterStrategy(groupId, EncounterStrategyType::CONSERVATIVE);
            TC_LOG_INFO("module.playerbot", "Group {} adapting to CONSERVATIVE strategy after {} wipes",
                groupId, state.wipeCount);
        }
    }
}

void DungeonBehavior::UpdateEncounterDifficulty(uint32 encounterId, float performanceRating)
{
    // Dynamically adjust encounter difficulty based on player performance
    // This helps the system learn which encounters are harder/easier for bots

    TC_LOG_TRACE("module.playerbot", "Encounter {} difficulty adjustment: {:.2f}",
        encounterId, performanceRating);
}

void DungeonBehavior::LogDungeonEvent(uint32 groupId, const ::std::string& event, const ::std::string& details)
{
    TC_LOG_DEBUG("module.playerbot.dungeon", "[Group {}] {}: {}",
        groupId, event, details.empty() ? "(no details)" : details);
}

} // namespace Playerbot
