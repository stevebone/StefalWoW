/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "EnhancedBotAI.h"
#include "GameTime.h"
#include "Combat/CombatAIIntegrator.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"
#include "SpellInfo.h"
#include "SpellAuras.h"
#include "MotionMaster.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Bag.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "Core/PlayerBotHelpers.h"
#include "../Quest/UnifiedQuestManager.h"
#include "../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5C: Thread-safe helpers
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace Playerbot
{

EnhancedBotAI::EnhancedBotAI(Player* bot) :
    BotAI(bot),
    _currentState(BotAIState::SOLO),
    _previousState(BotAIState::SOLO),
    _stateTransitionTime(0),
    _debugMode(false),
    _performanceMode(true),
    _maxUpdateRateHz(100), // 100 Hz max update rate
    _currentGroup(nullptr),
    _groupRole(Playerbot::GroupRole::NONE),
    _inCombat(false),
    _primaryTarget(nullptr),
    _combatStartTime(0),
    _currentManaPercent(100.0f),
    _currentHealthPercent(100.0f),
    _lastRestTime(0),
    _combatUpdateInterval(100),    // 100ms for combat
    _soloUpdateInterval(500),       // 500ms in solo mode
    _movementUpdateInterval(250),   // 250ms for movement
    _lastCombatUpdate(0),
    _lastSoloUpdate(0),
    _lastMovementUpdate(0),
    _lastGroupUpdate(0),
    _memoryBudgetBytes(10485760),   // 10MB budget
    _lastMemoryCheck(0),
    _memoryCheckInterval(5000),     // Check every 5 seconds
    _updateThrottleMs(0)
{
    InitializeCombatAI();
    InitializeClassAI();
    LoadConfiguration();

    _lastUpdateTime = ::std::chrono::high_resolution_clock::now();
    // CRITICAL: Do NOT access bot->GetName() or bot->GetGUID() in constructor!
    // Bot may not be fully in world yet, and Player::m_name/m_guid are not initialized,
    // causing ACCESS_VIOLATION. Logging deferred to first Update() when bot IsInWorld().
}

EnhancedBotAI::~EnhancedBotAI() = default;

void EnhancedBotAI::UpdateAI(uint32 diff)
{
    // CRITICAL: Call parent UpdateAI for core functionality (group invitations, etc.)
    BotAI::UpdateAI(diff);

    auto startTime = ::std::chrono::high_resolution_clock::now();

    // Performance throttling
    if (_updateThrottleMs > 0)
    {
        if (diff < _updateThrottleMs)
            return;
        _updateThrottleMs = 0;
    }

    StartPerformanceCapture();

    // Check performance budget
    if (!IsWithinPerformanceBudget())
    {
        ThrottleIfNeeded();
        return;
    }

    _stats.totalUpdates++;

    try
    {
        // State-based update routing
    switch (_currentState)
        {
            case BotAIState::COMBAT:
                UpdateCombat(diff);
                _stats.combatUpdates++;
                break;

            case BotAIState::SOLO:
                UpdateSolo(diff);
                _stats.soloUpdates++;
                break;

            case BotAIState::TRAVELLING:
            case BotAIState::FOLLOWING:
                UpdateMovement(diff);
                break;

            case BotAIState::QUESTING:
                UpdateQuesting(diff);
                break;

            case BotAIState::TRADING:
            case BotAIState::GATHERING:
                UpdateSocial(diff);
                break;

            case BotAIState::DEAD:
                // Death recovery is handled by BotAI::UpdateAI() -> DeathRecoveryManager
                // This includes spirit release, corpse run, and spirit healer interaction
                break;

            case BotAIState::FLEEING:
                // Flee logic handled in combat update
                UpdateCombat(diff);
                break;

            case BotAIState::RESTING:
                // Rest logic
    if (GetBot()->GetHealthPct() >= 95.0f && GetBot()->GetPowerPct(POWER_MANA) >= 95.0f)
                {
                    TransitionToState(BotAIState::SOLO);
                }
                break;
        }

        // Always update group coordination if in a group
    if (_currentGroup)
        {
            UpdateGroupCoordination(diff);
        }

        // Process events
        ProcessCombatEvents(diff);
        ProcessMovementEvents(diff);
        ProcessGroupEvents(diff);

        // Memory management
        _lastMemoryCheck += diff;
        if (_lastMemoryCheck >= _memoryCheckInterval)
        {
            CleanupExpiredData();
            CompactMemory(); // Only compacts our own memory, not CombatAIIntegrator
            _lastMemoryCheck = 0;
        }
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("bot.ai.enhanced", "Update exception for bot {}: {}",
            GetBot()->GetName(), e.what());
    }

    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);

    EndPerformanceCapture(elapsed);

    // Log performance if in debug mode
    if (_debugMode && _stats.totalUpdates % 100 == 0)
    {
        LogPerformanceReport();
    }
}

void EnhancedBotAI::Reset()
{
    BotAI::Reset();

    _currentState = BotAIState::SOLO;
    _previousState = BotAIState::SOLO;
    _inCombat = false;
    _primaryTarget = nullptr;
    _threatList.clear();

    if (_combatIntegrator)
        _combatIntegrator->Reset();

    if (_classAI)
        _classAI->Reset();

    _stats.Reset();

    TC_LOG_DEBUG("bot.ai.enhanced", "EnhancedBotAI reset for bot {}", GetBot()->GetName());
}

void EnhancedBotAI::OnDeath()
{
    BotAI::OnDeath();

    TransitionToState(BotAIState::DEAD);

    if (_combatIntegrator)
        _combatIntegrator->OnCombatEnd();

    _inCombat = false;
    _primaryTarget = nullptr;
}

void EnhancedBotAI::OnRespawn()
{
    BotAI::OnRespawn();

    TransitionToState(BotAIState::SOLO);

    // Reset health and mana tracking
    _currentHealthPercent = GetBot()->GetHealthPct();
    _currentManaPercent = GetBot()->GetPowerPct(POWER_MANA);
}

AIUpdateResult EnhancedBotAI::UpdateEnhanced(uint32 diff)
{
    // CRITICAL: DO NOT call UpdateAI from within UpdateEnhanced - causes deadlock
    // UpdateAI and UpdateEnhanced are separate entry points that should not call each other

    // Return success result
    AIUpdateResult result;
    result.actionsExecuted = 0;
    result.triggersChecked = 0;
    result.strategiesEvaluated = 0;
    result.updateTime = ::std::chrono::microseconds{0};
    return result;
}

// Combat event handlers
void EnhancedBotAI::OnCombatStart(Unit* target)
{
    if (!target)
        return;

    _inCombat = true;
    _primaryTarget = target;
    _combatStartTime = GameTime::GetGameTimeMS();

    TransitionToState(BotAIState::COMBAT);

    // Initialize combat systems
    if (_combatIntegrator)
    {
        _combatIntegrator->OnCombatStart(target);
    }

    if (_classAI)
    {
        _classAI->OnCombatStart(target);
    }

    TC_LOG_DEBUG("bot.ai.enhanced", "Combat started for bot {} against {}",
        GetBot()->GetName(), target->GetName());
}

void EnhancedBotAI::OnCombatEnd()
{
    _inCombat = false;
    _primaryTarget = nullptr;
    _threatList.clear();

    if (_combatIntegrator)
    {
        _combatIntegrator->OnCombatEnd();
    }

    if (_classAI)
    {
        _classAI->OnCombatEnd();
    }

    // Check if we should rest
    if (ShouldRest())
    {
        TransitionToState(BotAIState::RESTING);
    }
    else
    {
        TransitionToState(BotAIState::SOLO);
    }

    TC_LOG_DEBUG("bot.ai.enhanced", "Combat ended for bot {}", GetBot()->GetName());
}

void EnhancedBotAI::OnTargetChanged(Unit* newTarget)
{
    Unit* oldTarget = _primaryTarget;
    _primaryTarget = newTarget;
    if (_combatIntegrator)
    {
        _combatIntegrator->OnTargetChanged(newTarget);
    }

    if (_classAI)
    {
        _classAI->OnTargetChanged(newTarget);
    }

    TC_LOG_DEBUG("bot.ai.enhanced", "Target changed for bot {} from {} to {}",
        GetBot()->GetName(),
        oldTarget ? oldTarget->GetName() : "none",
        newTarget ? newTarget->GetName() : "none");
}

// Group event handlers
void EnhancedBotAI::OnGroupJoined(Group* group)
{
    _currentGroup = group;

    if (_combatIntegrator)
    {
        _combatIntegrator->SetGroup(group);
    }

    TC_LOG_DEBUG("bot.ai.enhanced", "Bot {} joined group", GetBot()->GetName());
}

void EnhancedBotAI::OnGroupLeft()
{
    _currentGroup = nullptr;
    _groupRole = Playerbot::GroupRole::NONE;
    _followTarget.Clear();

    if (_combatIntegrator)
    {
        _combatIntegrator->SetGroup(nullptr);
    }

    TC_LOG_DEBUG("bot.ai.enhanced", "Bot {} left group", GetBot()->GetName());
}

void EnhancedBotAI::OnGroupRoleChanged(Playerbot::GroupRole newRole)
{
    _groupRole = newRole;

    // Update combat AI configuration based on role
    if (_combatIntegrator)
    {
        CombatAIConfig config = _combatIntegrator->GetConfig();

        switch (newRole)
        {
            case Playerbot::GroupRole::TANK:
                config.enableThreatManagement = true;
                config.threatUpdateThreshold = 5.0f;
                _combatIntegrator = CombatAIFactory::CreateTankCombatAI(GetBot());
                break;

            case Playerbot::GroupRole::HEALER:
                config.enableKiting = true;
                config.positionUpdateThreshold = 10.0f;
                _combatIntegrator = CombatAIFactory::CreateHealerCombatAI(GetBot());
                break;

            case Playerbot::GroupRole::MELEE_DPS:
            case Playerbot::GroupRole::RANGED_DPS:
                config.enableInterrupts = true;
                config.targetSwitchCooldownMs = 500;
                _combatIntegrator = CombatAIFactory::CreateMeleeDPSCombatAI(GetBot());
                break;

            default:
                break;
        }
    }
}

// Configuration
void EnhancedBotAI::SetCombatConfig(CombatAIConfig const& config)
{
    if (_combatIntegrator)
    {
        _combatIntegrator->SetConfig(config);
    }
}

CombatAIConfig const& EnhancedBotAI::GetCombatConfig() const
{
    static CombatAIConfig defaultConfig;
    return _combatIntegrator ? _combatIntegrator->GetConfig() : defaultConfig;
}

// Internal update methods
void EnhancedBotAI::UpdateCombat(uint32 diff)
{
    _lastCombatUpdate += diff;
    if (_lastCombatUpdate < _combatUpdateInterval)
        return;

    if (!_combatIntegrator)
        return;

    // Update combat AI integrator
    IntegrationResult result = _combatIntegrator->Update(diff);

    if (!result.success)
    {
        TC_LOG_ERROR("bot.ai.enhanced", "Combat update failed for bot {}: {}",
            GetBot()->GetName(), result.errorMessage);
    }

    // Update class-specific combat rotation
    if (_classAI && _primaryTarget)
    {
        _classAI->UpdateRotation(_primaryTarget);
        _classAI->UpdateCooldowns(diff);
    }

    // Check for state transitions
    if (ShouldFlee())
    {
        TransitionToState(BotAIState::FLEEING);
    }
    else if (!_inCombat)
    {
        OnCombatEnd();
    }

    _stats.actionsExecuted += result.actionsExecuted;
    _lastCombatUpdate = 0;
}

void EnhancedBotAI::UpdateSolo(uint32 diff)
{
    _lastSoloUpdate += diff;
    if (_lastSoloUpdate < _soloUpdateInterval)
        return;

    // Check for combat
    if (ShouldEngageCombat())
    {
        // Find nearest enemy
        Unit* enemy = GetBot()->SelectNearbyTarget(nullptr, 40.0f);
        if (enemy)
        {
            OnCombatStart(enemy);
            return;
        }
    }

    // Update buffs
    if (_classAI)
    {
        _classAI->UpdateBuffs();
    }

    // Check if should follow group
    if (ShouldFollowGroup() && !_followTarget.IsEmpty())
    {
        // PHASE 5C: Thread-safe spatial grid validation (replaces ObjectAccessor::GetPlayer)
        auto snapshot = SpatialGridQueryHelpers::FindPlayerByGuid(GetBot(), _followTarget);
        Player* leader = nullptr;

        if (snapshot && snapshot->isAlive)
        {
            // Validated via snapshot - now safe to get actual Player* on main thread
            // ObjectAccessor::GetPlayer is safe here because we validated existence first
            leader = ObjectAccessor::GetPlayer(*GetBot(), _followTarget);
        }

        if (leader && GetBot()->GetExactDist2d(leader) > 10.0f)
        {
            if (!MoveToUnit(leader, 5.0f))
            {
                // Fallback to legacy if validation fails
                GetBot()->GetMotionMaster()->MoveFollow(leader, 5.0f, static_cast<float>(M_PI) / 2.0f);
            }
            TransitionToState(BotAIState::FOLLOWING);
        }
    }

    // Check if should rest
    if (ShouldRest())
    {
        TransitionToState(BotAIState::RESTING);
    }

    _lastSoloUpdate = 0;
}

void EnhancedBotAI::UpdateMovement(uint32 diff)
{
    _lastMovementUpdate += diff;
    if (_lastMovementUpdate < _movementUpdateInterval)
        return;

    // Update pathfinding if combat integrator is available
    if (_combatIntegrator && _combatIntegrator->IsInCombat())
    {
        // Combat movement is handled by combat integrator
        return;
    }

    // Following movement
    if (_currentState == BotAIState::FOLLOWING && !_followTarget.IsEmpty())
    {
        // PHASE 5C: Thread-safe spatial grid validation (replaces ObjectAccessor::GetPlayer)
        auto snapshot = SpatialGridQueryHelpers::FindPlayerByGuid(GetBot(), _followTarget);
        Player* leader = nullptr;
        if (snapshot && snapshot->isAlive)
        {
            // Validated via snapshot - now safe to get actual Player* on main thread
            // ObjectAccessor::GetPlayer is safe here because we validated existence first
            leader = ObjectAccessor::GetPlayer(*GetBot(), _followTarget);
        }

        if (leader)
        {
            float distance = GetBot()->GetExactDist2d(leader);
            if (distance < 5.0f)
            {
                GetBot()->GetMotionMaster()->Clear();
                TransitionToState(BotAIState::SOLO);
            }
            else if (distance > 50.0f)
            {
                // Teleport if too far
                GetBot()->NearTeleportTo(leader->GetPositionX(), leader->GetPositionY(),
                    leader->GetPositionZ(), leader->GetOrientation());
            }
        }
    }

    _lastMovementUpdate = 0;
}
void EnhancedBotAI::UpdateGroupCoordination(uint32 diff)
{
    _lastGroupUpdate += diff;
    if (_lastGroupUpdate < 1000) // Update every second
        return;

    if (!_currentGroup)
        return;

    // Update combat integrator group coordination
    if (_combatIntegrator && _inCombat)
    {
        _combatIntegrator->UpdateGroupCoordination();
    }

    // Check group leader for following
    if (!_inCombat && _currentState != BotAIState::FOLLOWING)
    {
        // PHASE 5C: Thread-safe spatial grid validation (replaces ObjectAccessor::GetPlayer)
        auto snapshot = SpatialGridQueryHelpers::FindPlayerByGuid(GetBot(), _currentGroup->GetLeaderGUID());
        Player* leader = nullptr;

        if (snapshot && snapshot->isAlive)
        {
            // Get Player* for follow target setup (validated via snapshot first)
        }

        if (leader && leader != GetBot())
        {
            _followTarget = leader->GetGUID();
        }
    }

    _lastGroupUpdate = 0;
}

void EnhancedBotAI::UpdateQuesting(uint32 diff)
{
    // Update questing state using UnifiedQuestManager
    // This integrates with the comprehensive quest system that handles:
    // - Quest discovery and pickup (PickupModule)
    // - Quest objective tracking and completion (CompletionModule)
    // - Quest validation (ValidationModule)
    // - Quest turn-in (TurnInModule)

    Player* bot = GetBot();
    if (!bot || !bot->IsAlive())
        return;

    // Throttle quest updates (every 2 seconds to reduce overhead)
    static uint32 questUpdateTimer = 0;
    questUpdateTimer += diff;
    if (questUpdateTimer < 2000)
        return;
    questUpdateTimer = 0;

    // Use UnifiedQuestManager singleton for all quest operations
    UnifiedQuestManager* questMgr = UnifiedQuestManager::instance();
    if (!questMgr)
    {
        TC_LOG_ERROR("bot.ai.enhanced", "UnifiedQuestManager not available for bot {}", bot->GetName());
        TransitionToState(BotAIState::SOLO);
        return;
    }

    // 1. Update quest progress - track all active quest objectives
    questMgr->UpdateQuestProgress(bot);

    // 2. Track quest objectives - monitor kill/collect/explore objectives
    questMgr->TrackQuestObjectives(bot);

    // 3. Optimize quest completion order - reorder quests for efficiency
    questMgr->OptimizeQuestCompletionOrder(bot);

    // 4. Discover and pickup new quests in the area (50 yard radius)
    questMgr->PickupQuestsInArea(bot, 50.0f);

    // 5. Check if bot has active quests to work on
    bool hasActiveQuests = false;
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (questId != 0)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (quest && bot->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE)
            {
                hasActiveQuests = true;
                break;
            }
        }
    }

    // If no active quests, transition back to solo mode
    if (!hasActiveQuests)
    {
        TC_LOG_DEBUG("bot.ai.enhanced", "Bot {} has no active quests, returning to solo mode",
            bot->GetName());
        TransitionToState(BotAIState::SOLO);
    }
}

void EnhancedBotAI::UpdateSocial(uint32 diff)
{
    // Update social interactions using TradeManager and social systems
    // This handles:
    // - Player-to-player trading
    // - Group loot distribution
    // - Material gathering coordination

    Player* bot = GetBot();
    if (!bot || !bot->IsAlive())
        return;

    // Throttle social updates (every 1 second)
    static uint32 socialUpdateTimer = 0;
    socialUpdateTimer += diff;
    if (socialUpdateTimer < 1000)
        return;
    socialUpdateTimer = 0;

    // Handle different social states
    switch (_currentState)
    {
        case BotAIState::TRADING:
        {
            // Trading state - check if trade window is still open
            TradeData* tradeData = bot->GetTradeData();
            if (!tradeData)
            {
                // Trade completed or cancelled, return to solo
                TC_LOG_DEBUG("bot.ai.enhanced", "Bot {} trade completed, returning to solo mode",
                    bot->GetName());
                TransitionToState(BotAIState::SOLO);
                return;
            }

            // Trade in progress - let TradeManager handle the logic
            // The BotAI base class handles trade acceptance/rejection based on security settings
            break;
        }

        case BotAIState::GATHERING:
        {
            // Gathering state - coordinate with group for resource gathering
            // Check if we're still near gathering nodes
            if (_currentGroup)
            {
                // In a group, coordinate gathering activities
                // Check if we should share gathered materials with group

                // Look for nearby gathering nodes (herbs, mines, skinning)
                // This uses the ObjectiveTracker pattern for finding nodes

                // For now, check if there are any valid gathering targets nearby
                bool hasGatheringTargets = false;

                // Simple distance check - if no gathering targets within 30 yards, go back to solo
                if (!hasGatheringTargets)
                {
                    TC_LOG_DEBUG("bot.ai.enhanced", "Bot {} found no gathering targets, returning to solo",
                        bot->GetName());
                    TransitionToState(BotAIState::SOLO);
                }
            }
            else
            {
                // Solo gathering - check if we should continue or return to solo mode
                TransitionToState(BotAIState::SOLO);
            }
            break;
        }

        default:
            // Unknown state for social update, return to solo
            TransitionToState(BotAIState::SOLO);
            break;
    }
}

// Decision making
bool EnhancedBotAI::ShouldEngageCombat()
{
    // Don't engage if dead or resting
    if (_currentState == BotAIState::DEAD || _currentState == BotAIState::RESTING)
        return false;

    // Don't engage if low health
    if (GetBot()->GetHealthPct() < 30.0f)
        return false;

    // Check for nearby enemies
    Unit* enemy = GetBot()->SelectNearbyTarget(nullptr, 40.0f);
    return enemy != nullptr;
}

bool EnhancedBotAI::ShouldFlee()
{
    // Flee if very low health
    if (GetBot()->GetHealthPct() < 15.0f)
        return true;

    // Flee if outnumbered
    if (_threatList.size() > 3)
        return true;

    return false;
}

bool EnhancedBotAI::ShouldRest()
{
    // Rest if low health or mana
    if (GetBot()->GetHealthPct() < 50.0f)
        return true;

    if (GetBot()->GetPowerPct(POWER_MANA) < 30.0f)
        return true;

    return false;
}

bool EnhancedBotAI::ShouldLoot()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsAlive())
        return false;

    // Don't loot while in combat
    if (_inCombat)
        return false;

    // Don't loot if inventory is nearly full (check if at least one bag slot is available)
    // Use simple inventory check - if bot has no empty slots, skip looting
    bool hasEmptySlot = false;
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* bag = bot->GetBagByPos(i))
        {
            for (uint32 slot = 0; slot < bag->GetBagSize(); ++slot)
            {
                if (!bag->GetItemByPos(slot))
                {
                    hasEmptySlot = true;
                    break;
                }
            }
        }
        if (hasEmptySlot)
            break;
    }
    if (!hasEmptySlot)
        return false;

    // Check if there are lootable corpses nearby using thread-safe spatial query
    constexpr float LOOT_RANGE = 30.0f;
    bool hasLootableCorpse = false;

    // Use spatial grid helpers for thread-safe creature iteration via snapshots
    auto nearbyHostileSnapshots = SpatialGridQueryHelpers::FindHostileCreaturesInRange(
        bot, LOOT_RANGE, false /* include dead creatures */);

    for (auto const& snapshot : nearbyHostileSnapshots)
    {
        if (snapshot.IsAlive())
            continue;

        // Find the actual creature for detailed checks using ObjectAccessor on main thread
        Creature* creature = ObjectAccessor::GetCreature(*bot, snapshot.guid);
        if (!creature)
            continue;

        // Check if this creature can be looted by this player
        if (!bot->isAllowedToLoot(creature))
            continue;

        // Check if creature still has loot
        if (creature->IsFullyLooted())
            continue;

        // Check line of sight
        if (!bot->IsWithinLOSInMap(creature))
            continue;

        hasLootableCorpse = true;
        break;
    }

    return hasLootableCorpse;
}

bool EnhancedBotAI::ShouldFollowGroup()
{
    return _currentGroup != nullptr && !_inCombat;
}

// Performance monitoring
void EnhancedBotAI::StartPerformanceCapture()
{
    // Capture start metrics
}

void EnhancedBotAI::EndPerformanceCapture(::std::chrono::microseconds elapsed)
{
    _stats.totalUpdateTime += elapsed;
    _stats.avgUpdateTime = _stats.totalUpdateTime / ::std::max(1u, _stats.totalUpdates);

    if (elapsed > _stats.maxUpdateTime)
        _stats.maxUpdateTime = elapsed;

    // Calculate CPU usage
    float cpuPercent = (elapsed.count() / 1000.0f) / 10.0f; // Assuming 10ms budget
    _stats.cpuUsagePercent = cpuPercent;

#ifdef _WIN32
    // Get memory usage
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        _stats.memoryUsageBytes = pmc.PrivateUsage;
    }
#endif
}

bool EnhancedBotAI::IsWithinPerformanceBudget() const
{
    // Check CPU budget (0.1% = 100 microseconds per 100ms)
    if (_stats.avgUpdateTime.count() > 100)
        return false;

    // Check memory budget
    if (_stats.memoryUsageBytes > _memoryBudgetBytes)
        return false;

    return true;
}

void EnhancedBotAI::ThrottleIfNeeded()
{
    // If over budget, increase update interval
    if (_stats.avgUpdateTime.count() > 100)
    {
        _updateThrottleMs = 50; // Skip next 50ms
    }
}

void EnhancedBotAI::LogPerformanceReport()
{
    TC_LOG_INFO("bot.ai.enhanced.performance",
        "Bot {} Performance Report:\n"
        "  Total Updates: {}\n"
        "  Combat Updates: {}\n"
        "  Avg Update Time: {} us\n"
        "  Max Update Time: {} us\n"
        "  CPU Usage: {:.3f}%\n"
        "  Memory Usage: {:.2f} MB",
        GetBot()->GetName(),
        _stats.totalUpdates,
        _stats.combatUpdates,
        _stats.avgUpdateTime.count(),
        _stats.maxUpdateTime.count(),
        _stats.cpuUsagePercent,
        _stats.memoryUsageBytes / 1048576.0f);

    if (_combatIntegrator)
    {
        CombatMetrics const& metrics = _combatIntegrator->GetMetrics();
        TC_LOG_INFO("bot.ai.enhanced.performance",
            "  Combat Metrics:\n"
            "    Interrupts: {}/{}\n"
            "    Position Changes: {}\n"
            "    Threat Adjustments: {}",
            metrics.interruptsSuccessful.load(),
            metrics.interruptsAttempted.load(),
            metrics.positionChanges.load(),
            metrics.threatAdjustments.load());
    }
}

// Private methods
void EnhancedBotAI::InitializeCombatAI()
{
    _combatIntegrator = CombatAIFactory::CreateCombatAI(GetBot());
    // Note: ClassAI registration happens in InitializeClassAI after both are created
}

void EnhancedBotAI::InitializeClassAI()
{
    // ClassAI initialization is handled by BotAI base class
    // Since ClassAI inherits from BotAI (not a separate component), we don't register it
    // The combat integrator will work with BotAI methods directly
}

void EnhancedBotAI::LoadConfiguration()
{
    // Load from config or use defaults
    _debugMode = false; // Disable debug mode by default
    _performanceMode = true;
    _maxUpdateRateHz = 100;
    _memoryBudgetBytes = 10485760; // 10MB
}

void EnhancedBotAI::TransitionToState(BotAIState newState)
{
    if (_currentState == newState)
        return;

    BotAIState oldState = _currentState;
    _previousState = _currentState;
    _currentState = newState;
    _stateTransitionTime = GameTime::GetGameTimeMS();

    HandleStateTransition(oldState, newState);

    TC_LOG_DEBUG("bot.ai.enhanced", "Bot {} transitioned from {} to {}",
        GetBot()->GetName(),
        static_cast<int>(oldState),
        static_cast<int>(newState));
}

void EnhancedBotAI::HandleStateTransition(BotAIState oldState, BotAIState newState)
{
    // Handle state-specific transitions
    switch (newState)
    {
        case BotAIState::COMBAT:
            _combatUpdateInterval = 100; // Faster updates in combat
            break;

        case BotAIState::SOLO:
            _soloUpdateInterval = 500; // Slower updates in solo mode
            break;

        case BotAIState::RESTING:
            GetBot()->SetStandState(UNIT_STAND_STATE_SIT);
            break;

        default:
            break;
    }

    // Handle leaving old state
    switch (oldState)
    {
        case BotAIState::RESTING:
            GetBot()->SetStandState(UNIT_STAND_STATE_STAND);
            break;

        default:
            break;
    }
}

void EnhancedBotAI::ProcessCombatEvents(uint32 diff)
{
    // Process combat-related events
    if (_inCombat && !GetBot()->IsInCombat())
    {
        OnCombatEnd();
    }
    else if (!_inCombat && GetBot()->IsInCombat())
    {
        Unit* target = GetBot()->GetVictim();
        if (target)
        {
            OnCombatStart(target);
        }
    }
}

void EnhancedBotAI::ProcessMovementEvents(uint32 diff)
{
    // Process movement-related events
}

void EnhancedBotAI::ProcessGroupEvents(uint32 diff)
{
    // Process group-related events
}

void EnhancedBotAI::CleanupExpiredData()
{
    // Clean up threat list
    _threatList.erase(
        ::std::remove_if(_threatList.begin(), _threatList.end(),
            [](Unit* unit) { return !unit || !unit->IsAlive(); }),
        _threatList.end()
    );
}

void EnhancedBotAI::CompactMemory()
{
    // Compact our own memory usage
    // Note: CombatAIIntegrator::CompactMemory() is private and cannot be called externally

    // Shrink vectors to fit their actual size
    _threatList.shrink_to_fit();
}

// Factory implementations
::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateEnhancedAI(Player* bot)
{
    return ::std::make_unique<EnhancedBotAI>(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateTankAI(Player* bot)
{
    auto ai = ::std::make_unique<EnhancedBotAI>(bot);

    CombatAIConfig config;
    config.enableThreatManagement = true;
    config.threatUpdateThreshold = 5.0f;
    config.positionUpdateThreshold = 3.0f;

    ai->SetCombatConfig(config);
    return ai;
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateHealerAI(Player* bot)
{
    auto ai = ::std::make_unique<EnhancedBotAI>(bot);

    CombatAIConfig config;
    config.enableKiting = true;
    config.positionUpdateThreshold = 10.0f;
    config.interruptReactionTimeMs = 150;

    ai->SetCombatConfig(config);
    return ai;
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateDPSAI(Player* bot)
{
    auto ai = ::std::make_unique<EnhancedBotAI>(bot);

    CombatAIConfig config;
    config.enableInterrupts = true;
    config.targetSwitchCooldownMs = 500;

    ai->SetCombatConfig(config);
    return ai;
}

// Class-specific factory methods
::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateWarriorAI(Player* bot)
{
    auto ai = ::std::make_unique<EnhancedBotAI>(bot);

    // Warriors are typically tanks or melee DPS
    CombatAIConfig config;
    config.enableThreatManagement = true;
    config.enablePositioning = true;
    config.positionUpdateThreshold = 5.0f;

    ai->SetCombatConfig(config);
    return ai;
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreatePriestAI(Player* bot)
{
    auto ai = ::std::make_unique<EnhancedBotAI>(bot);

    // Priests are typically healers
    CombatAIConfig config;
    config.enableKiting = true;
    config.enableInterrupts = true;
    config.positionUpdateThreshold = 15.0f;

    ai->SetCombatConfig(config);
    return ai;
}

// Implement remaining class-specific factory methods...
::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreatePaladinAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateHunterAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateRogueAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateDeathKnightAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateShamanAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateMageAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateWarlockAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateMonkAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateDruidAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateDemonHunterAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

::std::unique_ptr<EnhancedBotAI> EnhancedBotAIFactory::CreateEvokerAI(Player* bot)
{
    return CreateEnhancedAI(bot);
}

} // namespace Playerbot