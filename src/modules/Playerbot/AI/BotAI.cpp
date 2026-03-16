/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * REFACTORED BotAI Implementation - Clean Update Chain
 *
 * This implementation provides:
 * 1. Single clean UpdateAI() method - no DoUpdateAI/UpdateEnhanced confusion
 * 2. Clear separation of base behaviors and combat specialization
 * 3. Every-frame updates for smooth movement and following
 * 4. Proper delegation to ClassAI for combat-only updates
 */

#include "BotAI.h"
#include "AdaptiveAIUpdateThrottler.h"
#include "Humanization/Activities/RPGDailyRoutineManager.h"
#include "GameTime.h"
#include "Core/Managers/GameSystemsManager.h"
#include "Lifecycle/BotLifecycleState.h"
#include "Lifecycle/BotFactory.h"
#include "Lifecycle/BotSaveController.h"
#include "Advanced/TacticalCoordinator.h"
#include "Strategy/Strategy.h"
#include "Strategy/GroupCombatStrategy.h"
#include "Strategy/SoloCombatStrategy.h"
#include "Strategy/SoloStrategy.h"
#include "Strategy/QuestStrategy.h"
#include "Strategy/LootStrategy.h"
#include "Strategy/RestStrategy.h"
#include "Strategy/GrindStrategy.h"
#include "Actions/Action.h"
#include "Triggers/Trigger.h"
#include "Movement/LeaderFollowBehavior.h"
#include "Movement/Arbiter/MovementRequest.h"
#include "Movement/Arbiter/MovementPriorityMapper.h"
#include "Session/BotPriorityManager.h"
#include "Core/Diagnostics/BotCheatMask.h"
#include "Spatial/SpatialGridManager.h"
#include "Spatial/DoubleBufferedSpatialGrid.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ThreatManager.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "WorldSession.h"
#include "Log.h"
#include "Timer.h"
#include "DatabaseEnv.h"
#include "QueryResult.h"
#include "Core/PlayerBotHooks.h"
#include "../PvP/BattlegroundAI.h"
#include "../Lifecycle/Instance/BotPostLoginConfigurator.h"
#include "../Lifecycle/Instance/InstanceBotOrchestrator.h"
#include "../Dungeon/DungeonAutonomyManager.h"
#include "Map.h"
#include <chrono>
#include <set>
#include <unordered_map>

namespace Playerbot
{

// TriggerResultComparator implementation
bool TriggerResultComparator::operator()(TriggerResult const& a, TriggerResult const& b) const
{
    // Higher urgency has higher priority (reverse order for max-heap)
    return a.urgency < b.urgency;
}

// ============================================================================
// QUEST HELPER METHODS - Direct player data access (replaces Game/QuestManager)
// ============================================================================

static constexpr uint8 MAX_QUEST_LOG_SLOT = 25;  // Maximum quest log size

uint32 BotAI::GetActiveQuestCount() const
{
    if (!_bot || !_bot->IsInWorld())
        return 0;

    uint32 count = 0;
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SLOT; ++slot)
    {
        if (_bot->GetQuestSlotQuestId(slot) != 0)
            ++count;
    }
    return count;
}

bool BotAI::IsQuestingActive() const
{
    return GetActiveQuestCount() > 0;
}

bool BotAI::HasCompletableQuests() const
{
    if (!_bot || !_bot->IsInWorld())
        return false;

    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SLOT; ++slot)
    {
        uint32 questId = _bot->GetQuestSlotQuestId(slot);
        if (questId != 0 && _bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
            return true;
    }
    return false;
}

std::vector<uint32> BotAI::GetCompletableQuestIds() const
{
    std::vector<uint32> result;
    if (!_bot || !_bot->IsInWorld())
        return result;

    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SLOT; ++slot)
    {
        uint32 questId = _bot->GetQuestSlotQuestId(slot);
        if (questId != 0 && _bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
            result.push_back(questId);
    }
    return result;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

BotAI::BotAI(Player* bot, bool instanceOnlyMode)
    : _bot(bot)
    , _cachedBotGuid(bot ? bot->GetGUID() : ObjectGuid::Empty)  // Cache for safe destructor cleanup
    , _instanceOnlyMode(instanceOnlyMode)
{
    // Initialize performance tracking
    _performanceMetrics.lastUpdate = std::chrono::steady_clock::now();

    // ========================================================================
    // INSTANCE-ONLY MODE - For JIT bots created for BG/LFG queues
    // ========================================================================
    // When instanceOnlyMode is true, GameSystemsManager will skip creating
    // expensive non-essential managers (questing, professions, AH, banking).
    // This significantly reduces CPU overhead for bots that only need combat.
    //
    // AUTO-DETECTION: If not explicitly set, detect JIT bots by checking
    // if they have pending configuration (JITBotFactory creates these) or
    // are managed by the InstanceBotOrchestrator.
    if (!_instanceOnlyMode && _bot)
    {
        ObjectGuid botGuid = _bot->GetGUID();
        bool isJITBot = sBotPostLoginConfigurator->HasPendingConfiguration(botGuid) ||
                        sInstanceBotOrchestrator->IsManagedBot(botGuid);
        if (isJITBot)
        {
            _instanceOnlyMode = true;
            TC_LOG_INFO("module.playerbot.ai", "BotAI: Auto-detected JIT bot {} - enabling INSTANCE-ONLY mode",
                botGuid.ToString());
        }
    }

    if (_instanceOnlyMode)
    {
        TC_LOG_DEBUG("module.playerbot.ai", "BotAI: Creating bot in INSTANCE-ONLY mode (lightweight)");
    }

    // ========================================================================
    // PHASE 6: GAME SYSTEMS FACADE - Consolidate all 17 manager instances
    // ========================================================================
    // Previously: 17 separate manager unique_ptrs created here + timers
    // Now: Single facade owns and manages all 17 managers + timers
    // Benefits: Reduced god class complexity, improved testability, easier maintenance

    _gameSystems = std::make_unique<GameSystemsManager>(_bot, this);
    _gameSystems->Initialize(_bot);

    // CRITICAL: Do NOT access _bot->GetName() in constructor - bot data not initialized yet

    // Phase 4: Initialize Shared Blackboard (thread-safe shared state system)
    _sharedBlackboard = nullptr; // Deferred to first Update() when bot IsInWorld()

    // Initialize behavior priority manager BEFORE strategies
    // This must exist before strategies are added so they can auto-register
    _priorityManager = std::make_unique<BehaviorPriorityManager>(this);

    // ST-1: Initialize adaptive AI update throttler for CPU optimization
    // Bots far from human players will have reduced update frequency
    _aiUpdateThrottler = std::make_unique<AdaptiveAIUpdateThrottler>(_bot, this);

    // NEW: Initialize movement controller and register with global manager
    if (_bot)
    {
        _movementController = std::make_unique<BotMovementController>(_bot);
        sBotMovementManager->RegisterController(_bot);
        TC_LOG_DEBUG("module.playerbot.movement", "BotAI: Movement controller initialized for bot {}",
            _cachedBotGuid.ToString());

        // RPG Daily Routine Manager - autonomous daily activity simulation
        _rpgRoutineManager = std::make_unique<Humanization::RPGDailyRoutineManager>(_bot);
    }

    // Initialize default strategies for basic functionality
    InitializeDefaultStrategies();

    // Initialize default triggers
    sBotAIFactory->InitializeDefaultTriggers(this);

    // Phase 4: Subscribe to all event buses for comprehensive event handling
    SubscribeToEventBuses();

    // CRITICAL FIX: Group validation DEFERRED to first UpdateAI() call!
    // Accessing _bot->GetGroup() and iterating group members in constructor causes
    // ACCESS_VIOLATION because Player/Group internal structures are not initialized yet.
    // The bot is not in world during construction, so GetGroup() may return corrupted data.
    // Group validation logic moved to ValidateExistingGroupMembership() called in UpdateAI().
}

// ============================================================================
// DEFERRED GROUP VALIDATION - Called from first UpdateAI() when bot IsInWorld()
// ============================================================================
// CRITICAL FIX: This logic was moved from constructor to avoid ACCESS_VIOLATION.
// During constructor, bot is not in world and Group/GroupReference data may be invalid.

void BotAI::ValidateExistingGroupMembership()
{
    // Only run once
    if (_groupValidationDone)
        return;
    _groupValidationDone = true;

    // Check if bot is already in a VALID group (e.g., after server restart)
    // Groups should persist if there's at least one real player character
    // who has been offline for less than 1 hour. This applies to all group sizes:
    // - 2-person groups (1 player + 1 bot)
    // - 5-person dungeon groups (1 player + 4 bots)
    // - 40-person raid groups (1 player + 39 bots)
    // If all players are offline for > 1 hour, disband the group
    if (Group* group = _bot->GetGroup())
    {
        bool hasValidPlayer = false;
        ObjectGuid playerGuidToCheck = ObjectGuid::Empty;

        // Check all group members for a real player character
        for (GroupReference const& itr : group->GetMembers())
        {
            Player* member = itr.GetSource();
            if (!member)
                continue;

            // Skip if this is a bot
            if (Playerbot::PlayerBotHooks::IsPlayerBot(member))
                continue;

            // Found a real player character
            // Check if they're online OR offline for less than 1 hour
            if (member->IsInWorld())
            {
                // Player is currently online
                hasValidPlayer = true;
                TC_LOG_DEBUG("module.playerbot.ai", "ValidateExistingGroupMembership: Bot {} found valid online player {} in group",
                             _bot->GetName(), member->GetName());
                break;
            }
            else
            {
                // Player is offline - need to check logout time from database
                playerGuidToCheck = member->GetGUID();
                TC_LOG_DEBUG("module.playerbot.ai", "ValidateExistingGroupMembership: Bot {} found offline player {} in group, checking logout time",
                             _bot->GetName(), member->GetGUID().GetCounter());
                break;
            }
        }

        // If we found an offline player, check their logout time via database query
        if (!hasValidPlayer && !playerGuidToCheck.IsEmpty())
        {
            // Query the characters database for logout_time
            QueryResult result = CharacterDatabase.PQuery(
                "SELECT logout_time FROM characters WHERE guid = {}", playerGuidToCheck.GetCounter());
            if (result)
            {
                Field* fields = result->Fetch();
                uint64 logoutTime = fields[0].GetUInt64();
                uint64 currentTime = static_cast<uint64>(time(nullptr));
                uint64 timeSinceLogout = currentTime - logoutTime;

                // 1 hour = 3600 seconds
                if (timeSinceLogout < 3600)
                {
                    hasValidPlayer = true;
                    TC_LOG_DEBUG("module.playerbot.ai", "ValidateExistingGroupMembership: Bot {} - offline player {} logged out {} seconds ago (valid)",
                                 _bot->GetName(), playerGuidToCheck.GetCounter(), timeSinceLogout);
                }
                else
                {
                    TC_LOG_DEBUG("module.playerbot.ai", "ValidateExistingGroupMembership: Bot {} - offline player {} logged out {} seconds ago (expired)",
                                 _bot->GetName(), playerGuidToCheck.GetCounter(), timeSinceLogout);
                }
            }
        }

        if (hasValidPlayer)
        {
            // Valid group with active or recently logged out player
            TC_LOG_INFO("module.playerbot.ai", "ValidateExistingGroupMembership: Bot {} has valid group, calling OnGroupJoined",
                        _bot->GetName());
            OnGroupJoined(group);
        }
        else
        {
            // No valid player found - all offline > 1 hour or only bots
            TC_LOG_INFO("module.playerbot.ai", "ValidateExistingGroupMembership: Bot {} has no valid player in group, leaving group",
                        _bot->GetName());
            // Leave the invalid group
            _bot->RemoveFromGroup();
            _aiState = BotAIState::SOLO; // Explicitly set to SOLO
        }
    }
    else
    {
        TC_LOG_DEBUG("module.playerbot.ai", "ValidateExistingGroupMembership: Bot {} is not in a group",
                     _bot->GetName());
    }
}

BotAI::~BotAI()
{
    // Phase 4: CRITICAL - Unsubscribe from all event buses to prevent dangling pointers
    UnsubscribeFromEventBuses();

    // NEW: Unregister movement controller from global manager
    // CRITICAL: Use _cachedBotGuid instead of _bot->GetGUID() as _bot may be dangling
    if (_movementController && !_cachedBotGuid.IsEmpty())
    {
        sBotMovementManager->UnregisterController(_cachedBotGuid);
        TC_LOG_DEBUG("module.playerbot.movement", "BotAI: Movement controller unregistered for bot {}",
            _cachedBotGuid.ToString());
    }

    // ========================================================================
    // PHASE 6: GAME SYSTEMS FACADE - Automatic Manager Cleanup
    // ========================================================================
    // The GameSystemsManager facade destructor handles correct cleanup order:
    // 1. Managers destroyed in dependency order (most dependent first)
    // 2. EventDispatcher destroyed last (after all managers unsubscribed)
    // No manual reset() calls needed - facade handles everything!
    //
    // Benefits: Simplified destructor, guaranteed correct cleanup order,
    // no risk of forgetting a manager or getting the order wrong.
    // ========================================================================

    // CRITICAL: Do NOT call _bot->GetName() in destructor!
    // During destruction, _bot may be in invalid state where GetName() returns
    // garbage data, causing std::bad_alloc when string tries huge allocation.

    // Phase 4: Cleanup Shared Blackboard
    // CRITICAL FIX: Use _cachedBotGuid instead of _bot->GetGUID()!
    // During destructor, _bot may be a dangling pointer to already-freed memory.
    // Using _cachedBotGuid is safe - it was captured in the constructor.
    if (_sharedBlackboard && !_cachedBotGuid.IsEmpty())
    {
        BlackboardManager::RemoveBotBlackboard(_cachedBotGuid);
        _sharedBlackboard = nullptr;
    }

    // Facade (_gameSystems) will be automatically destroyed here,
    // which triggers GameSystemsManager::~GameSystemsManager()
    // and cleans up all 17 managers in correct dependency order
}

// ============================================================================
// MAIN UPDATE METHOD - CLEAN SINGLE ENTRY POINT
// ============================================================================

void BotAI::UpdateAI(uint32 diff)
{
    // CRITICAL: This is the SINGLE entry point for ALL AI updates
    // No more confusion with DoUpdateAI/UpdateEnhanced

    // ========================================================================
    // CRITICAL SAFETY CHECK: Validate _bot pointer FIRST before ANY access
    // ========================================================================
    // This check MUST be at the very beginning to prevent use-after-free crashes.
    // The _bot pointer can become invalid if:
    // 1. Exception in HandleBotPlayerLogin deletes Player but not AI
    // 2. Race condition during logout/cleanup
    // 3. Memory corruption
    //
    // We check for common corruption patterns:
    // - nullptr (obvious invalid)
    // - Low addresses (< 0x10000) are never valid heap pointers on any OS
    // - Debug fill patterns: 0xDDDDDDDD (freed), 0xFEEEFEEE (freed heap),
    //   0xCDCDCDCD (uninitialized heap), 0xCCCCCCCC (uninitialized stack)
    {
        uintptr_t botPtr = reinterpret_cast<uintptr_t>(_bot);
        if (botPtr == 0 || botPtr < 0x10000 ||
            botPtr == 0xDDDDDDDDDDDDDDDD || botPtr == 0xFEEEFEEEFEEEFEEE ||
            botPtr == 0xCDCDCDCDCDCDCDCD || botPtr == 0xCCCCCCCCCCCCCCCC ||
            botPtr == 0xDDDDDDDD || botPtr == 0xFEEEFEEE ||
            botPtr == 0xCDCDCDCD || botPtr == 0xCCCCCCCC)
        {
            TC_LOG_ERROR("module.playerbot.ai", "CRITICAL: BotAI::UpdateAI called with invalid _bot pointer 0x{:X} - aborting to prevent crash!", botPtr);
            return;
        }
    }

    // Now safe to access _bot
    if (!_bot->IsInWorld())
        return;

    // ========================================================================
    // BOT-SPECIFIC LOGIN SPELL CLEANUP: Clear events on first update to prevent LOGINEFFECT crash
    // ========================================================================
    // Issue: LOGINEFFECT (Spell 836) is cast during SendInitialPacketsAfterAddToMap() at Player.cpp:24742
    //        and queued in EventProcessor. It fires during first Player::Update() → EventProcessor::Update()
    //        which causes Spell.cpp:603 assertion failure: m_spellModTakingSpell != this
    // Root Cause: Bot doesn't send CMSG_CAST_SPELL ACK packets like real players, leaving stale spell references
    // Solution: Clear ALL pending events HERE on first update, BEFORE EventProcessor::Update() can fire them
    // Timing: This runs in UpdateAI() which is called DURING Player::Update(), BEFORE EventProcessor::Update()
    if (!_firstUpdateComplete)
    {
        // Deferred blackboard initialization - GetGUID() unsafe in constructor
        if (!_sharedBlackboard)
            _sharedBlackboard = BlackboardManager::GetBotBlackboard(_bot->GetGUID());

        // Core Fix Applied: SpellEvent destructor now clears m_spellModTakingSpell
        _bot->m_Events.KillAllEvents(false);
        _firstUpdateComplete = true;
    }

    // DIAGNOSTIC: Log UpdateAI entry for first bot only, once per 10 seconds
    static uint32 lastUpdateLog = 0;
    static bool loggedFirstBot = false;
    uint32 now = GameTime::GetGameTimeMS();

    if (!loggedFirstBot || (now - lastUpdateLog > 10000))
    {
        TC_LOG_INFO("module.playerbot", "✅ UpdateAI active: Bot {} (ID: {}), InWorld={}, InCombat={}, InGroup={}, Strategies={}",
                    _bot->GetName(),
                    _bot->GetGUID().GetCounter(),
                    _bot->IsInWorld(),
                    _bot->IsInCombat(),
                    _bot->GetGroup() != nullptr,
                    _activeStrategies.size());
        lastUpdateLog = now;
        loggedFirstBot = true;
    }

    // ========================================================================
    // DEFERRED GROUP VALIDATION - Now safe because bot is in world
    // ========================================================================
    // CRITICAL FIX: Moved from constructor to avoid ACCESS_VIOLATION.
    // Group iteration in constructor caused crashes because Player/Group data
    // is not initialized during construction.
    ValidateExistingGroupMembership();

    // ========================================================================
    // LIFECYCLE STATE TRANSITION - Two-Phase AddToWorld Pattern
    // ========================================================================
    // On first successful UpdateAI, transition from READY → ACTIVE state.
    // This indicates the bot is now fully operational and all managers are safe to use.
    // The lifecycle manager processes any queued deferred events at this point.
    if (!_lifecycleActivated && _lifecycleManager)
    {
        BotInitState currentState = _lifecycleManager->GetState();

        // Only transition if we're in READY state (safe to become ACTIVE)
        if (currentState == BotInitState::READY)
        {
            if (_lifecycleManager->MarkActive())
            {
                _lifecycleActivated = true;

                TC_LOG_DEBUG("module.playerbot.lifecycle",
                    "Bot {} transitioned to ACTIVE on first UpdateAI",
                    _bot->GetName());

                // Process any deferred events that were queued during initialization
                uint32 processedEvents = _lifecycleManager->ProcessQueuedEvents(
                    [this](BotInitStateManager::DeferredEvent const& event)
                    {
                        // Handle different event types
                        switch (event.type)
                        {
                            case BotInitStateManager::DeferredEventType::GROUP_JOINED:
                                if (Group* group = _bot->GetGroup())
                                    OnGroupJoined(group);
                                break;

                            case BotInitStateManager::DeferredEventType::COMBAT_START:
                                if (::Unit* target = ObjectAccessor::GetUnit(*_bot, event.targetGuid))
                                    OnCombatStart(target);
                                break;

                            case BotInitStateManager::DeferredEventType::CUSTOM:
                                // Callback events are handled internally by ProcessQueuedEvents
                                break;

                            default:
                                TC_LOG_TRACE("module.playerbot.lifecycle",
                                    "Bot {} processing deferred event type {}",
                                    _bot->GetName(), static_cast<uint8>(event.type));
                                break;
                        }
                    });

                if (processedEvents > 0)
                {
                    TC_LOG_INFO("module.playerbot.lifecycle",
                        "Bot {} processed {} deferred events on activation",
                        _bot->GetName(), processedEvents);
                }
            }
        }
        else if (currentState == BotInitState::ACTIVE)
        {
            // Already active (maybe from previous session or manual transition)
            _lifecycleActivated = true;
        }
        else if (currentState == BotInitState::FAILED ||
                 currentState == BotInitState::DESTROYED)
        {
            // Bot is in error state - skip activation
            TC_LOG_WARN("module.playerbot.lifecycle",
                "Bot {} cannot activate - in state {}",
                _bot->GetName(),
                std::string(BotInitStateToString(currentState)));
            _lifecycleActivated = true; // Prevent repeated warnings
        }
    }
    // For bots without lifecycle management (legacy code path), mark as activated
    else if (!_lifecycleActivated && !_lifecycleManager)
    {
        _lifecycleActivated = true;
    }

    // ========================================================================
    // STALL DETECTION - Record update timestamp for health monitoring
    // ========================================================================
    // CRITICAL FIX: BotPriorityManager::RecordUpdateStart() was never called!
    // This caused all bots to be flagged as stalled after stallThresholdMs elapsed.
    // We must call this at the start of EVERY UpdateAI() to keep lastUpdateTime current.
    sBotPriorityMgr->RecordUpdateStart(_bot->GetGUID(), now);

    // ========================================================================
    // RPG-STATE AI BUDGET: Reassess tier every 1 second
    // ========================================================================
    // Maps RPG state to budget tier (FULL/REDUCED/MINIMAL) to skip unnecessary
    // AI phases. Combat and BG/dungeon always force FULL. Checked per-tick for
    // instant combat escalation even between 1s reassessments.
    AIBudgetTier previousBudgetTier = _currentBudgetTier;

    _budgetReassessTimer += diff;
    if (_budgetReassessTimer >= 1000)
    {
        _budgetReassessTimer = 0;

        // Combat ALWAYS forces FULL regardless of RPG state
        if (_bot->IsInCombat() || !_bot->getAttackers().empty())
        {
            _currentBudgetTier = AIBudgetTier::FULL;
        }
        // BG/Dungeon always FULL
        else if (_bot->InBattleground() || (_bot->GetMap() && _bot->GetMap()->IsDungeon()))
        {
            _currentBudgetTier = AIBudgetTier::FULL;
        }
        // In a group — always FULL (human player may be watching)
        else if (_bot->GetGroup())
        {
            _currentBudgetTier = AIBudgetTier::FULL;
        }
        // Use RPG state mapping for autonomous solo bots
        else if (_rpgRoutineManager && _rpgRoutineManager->ShouldBeActive())
        {
            _currentBudgetTier = GetBudgetTierForRPGState(_rpgRoutineManager->GetCurrentState());
        }
        else
        {
            _currentBudgetTier = AIBudgetTier::FULL;  // safe default
        }
    }

    // INSTANT COMBAT ESCALATION: Check EVERY tick (not just every 1s)
    // If attacked between reassessments, immediately go to FULL
    if (_currentBudgetTier != AIBudgetTier::FULL)
    {
        if (_bot->IsInCombat() || !_bot->getAttackers().empty())
            _currentBudgetTier = AIBudgetTier::FULL;
    }

    // P5/P6: Handle budget tier transitions (save tiering + memory reduction)
    if (_currentBudgetTier != previousBudgetTier)
        OnBudgetTierTransition(previousBudgetTier, _currentBudgetTier);

    // ========================================================================
    // DEATH RECOVERY - Runs BEFORE all other AI (even when dead/ghost)
    // ========================================================================
    // Death recovery MUST run even when bot is dead (ghost state)
    // This allows bots to release spirit, run to corpse, and resurrect
    if (auto deathRecovery = GetDeathRecoveryManager())
        deathRecovery->Update(diff);

    // ========================================================================
    // MOVEMENT CONTROLLER - Update validated movement system FIRST
    // ========================================================================
    // NEW: Update movement controller for validated pathfinding and stuck detection
    // This runs after death recovery but before normal AI to ensure movement is always updated
    if (_movementController && _bot->IsAlive())
    {
        _movementController->Update(diff);

        // Check for stuck state and log for debugging
        if (_movementController->IsStuck())
        {
            TC_LOG_DEBUG("module.playerbot.movement", "Bot {} is stuck, recovery is being handled by controller",
                _bot->GetName());
            // Recovery is handled internally by the movement controller
        }
    }

    // Apply cheat effects (health/mana regen) — throttled: 500ms in combat, 2s out of combat
    if (_cheatEffectTimer <= diff)
    {
        if (sBotCheatMask->HasAnyCheats(_bot->GetGUID()))
        {
            sBotCheatMask->ApplyCheatEffects(_bot);
            _cheatEffectTimer = _bot->IsInCombat() ? 500 : 2000;
        }
        else
            _cheatEffectTimer = 2000; // No cheats active, check again in 2s
    }
    else
        _cheatEffectTimer -= diff;

    // PRIORITY: If bot is in death recovery, skip expensive AI updates
    // Death recovery handles its own movement (corpse run), so we don't need strategies/combat
    // But we still allow managers to update (see PHASE 5) to prevent system freezing
    auto deathRecovery = GetDeathRecoveryManager();
    bool isInDeathRecovery = deathRecovery && deathRecovery->IsInDeathRecovery();

    // Performance tracking - declare BEFORE the if block so it's accessible after
    auto startTime = std::chrono::high_resolution_clock::now();
    _performanceMetrics.totalUpdates++;

    // ========================================================================
    // SOLO STRATEGY ACTIVATION - Once per bot after first login
    // ========================================================================
    // MUST run BEFORE throttle/budget/deathRecovery checks to ensure strategies
    // get activated. Dead bots still need strategies so they can act once revived.
    if (!_bot->GetGroup() && !_soloStrategiesActivated)
    {
        TC_LOG_INFO("module.playerbot.ai", "ACTIVATING SOLO STRATEGIES: Bot {} (not in group, first UpdateAI)", _bot->GetName());

        ActivateStrategy("rest");
        ActivateStrategy("solo_combat");

        if (!_instanceOnlyMode)
        {
            ActivateStrategy("quest");
            ActivateStrategy("grind");
            ActivateStrategy("loot");
            ActivateStrategy("solo");
        }
        else
        {
            TC_LOG_INFO("module.playerbot.ai", "Bot {} is JIT/Instance-Only - Skipping quest/grind/solo strategies", _bot->GetName());
        }

        _soloStrategiesActivated = true;

        TC_LOG_INFO("module.playerbot.ai", "SOLO BOT ACTIVATION COMPLETE: Bot {} - {} strategies active", _bot->GetName(), _activeStrategies.size());
    }

    // Only run normal AI if NOT in death recovery
    if (!isInDeathRecovery)
    {
    // ========================================================================
    // BUDGET TIER: MINIMAL skips everything below (safety systems already ran)
    // ========================================================================
    if (_currentBudgetTier == AIBudgetTier::MINIMAL)
        goto throttled_update_complete;

    // ========================================================================
    // ST-1: ADAPTIVE AI UPDATE THROTTLING - CPU optimization for far bots
    // ========================================================================
    // Check if this update should be processed based on:
    // - Proximity to human players (near = full rate, far = reduced)
    // - Combat state (in combat = full rate always)
    // - Bot activity level (idle = minimal updates)
    // This optimization can reduce CPU usage by 10-15% for bots far from players
    if (_aiUpdateThrottler && !_aiUpdateThrottler->ShouldUpdate(diff))
    {
        // Throttled - skip this update cycle
        // Note: Essential systems (death recovery, stall detection) already ran above
        goto throttled_update_complete;
    }

    // ========================================================================
    // BATTLEGROUND AI CONTEXT - Priority handler for BG situations
    // ========================================================================
    // CRITICAL: If bot is in an active battleground, use BattlegroundAI instead
    // of normal solo/group strategies. The BG AI handles all BG-specific logic:
    // - WSG/TP flag capture and defense
    // - AB/BfG base capture and defense
    // - AV tower/graveyard capture, boss kills
    // - EOTS flag + base hybrid strategy
    // - Siege weapon operation (SotA/IoC)
    // - Team coordination and objective-based play
    //
    // This check MUST be before solo/group strategy activation to prevent
    // bots from using dungeon-like follow behavior in battlegrounds.
    if (_bot->InBattleground())
    {
        ::Battleground* bg = _bot->GetBattleground();
        // Activate BG AI during both prep phase (WAIT_JOIN) and active phase (IN_PROGRESS)
        // During prep, bots should move to their positions and prepare for gates to open
        if (bg && (bg->GetStatus() == STATUS_IN_PROGRESS || bg->GetStatus() == STATUS_WAIT_JOIN))
        {
            // DIAGNOSTIC: Log BG AI activation (throttled to once per 10 seconds per bot)
            static std::unordered_map<uint32, uint32> lastBGLog;
            uint32 botId = _bot->GetGUID().GetCounter();
            uint32 nowMs = GameTime::GetGameTimeMS();
            if (!lastBGLog.count(botId) || (nowMs - lastBGLog[botId] > 10000))
            {
                TC_LOG_INFO("module.playerbot.bg", "🎮 BG AI ACTIVE: Bot {} in {} (Instance: {}, Status: IN_PROGRESS)",
                    _bot->GetName(),
                    bg->GetName(),
                    bg->GetInstanceID());
                lastBGLog[botId] = nowMs;
            }

            // Delegate to BattlegroundAI for all BG decision-making
            BattlegroundAI::instance()->Update(_bot, diff);

            // Still process combat for PvP engagements
            UpdateCombatState(diff);
            if (IsInCombat())
            {
                OnCombatUpdate(diff);
            }

            // Skip solo/group strategies - BG AI handles everything
            // Jump directly to game systems update (Phase 6)
            goto bg_update_complete;
        }
    }

    // PHASE 0 - Quick Win #3: Periodic group check REMOVED
    // Now using event-driven GROUP_JOINED/GROUP_LEFT events for instant reactions
    // Events dispatched in BotSession.cpp (GROUP_JOINED) and BotAI.cpp (GROUP_LEFT)
    // This eliminates 1-second polling lag

    // FIX #22: Populate ObjectCache WITHOUT calling ObjectAccessor
    // Bot code provides objects directly from already-available sources
    // ZERO ObjectAccessor calls = ZERO deadlock risk

    // 1. Cache combat target (from GetVictim - no ObjectAccessor needed)
    if (::Unit* victim = _bot->GetVictim())
        _objectCache.SetTarget(victim);
    else
        _objectCache.SetTarget(nullptr);

    // 2. Cache group data (from GetGroup - no ObjectAccessor needed)
    if (Group* group = _bot->GetGroup()){
        // Get group leader from group members directly
        Player* leader = nullptr;
        std::vector<Player*> members;

        for (GroupReference const& itr : group->GetMembers())
        {
            Player* member = itr.GetSource();
            if (!member)
                continue;

            // FIX #3: Comprehensive safety checks to prevent crash when player logs out
            // Multiple conditions ensure member is fully valid before caching
            try
            {
                // Check if member is being destroyed or logging out
                if (!member->IsInWorld())
                    continue;

                WorldSession* session = member->GetSession();
                if (!session)
                    continue;

                // Check if session is valid and not logging out
                if (session->PlayerLogout())
                    continue;// Member is safe to cache
                members.push_back(member);
                if (member->GetGUID() == group->GetLeaderGUID())
                    leader = member;}
            catch (...)
            {
                // Catch any exceptions during member access (e.g., destroyed objects)
TC_LOG_ERROR("playerbot", "Exception while accessing group member for bot {}", _bot->GetName());
                continue;}
        }
        _objectCache.SetGroupLeader(leader);
        _objectCache.SetGroupMembers(members);// Follow target is usually the leader (only if leader is online)
        if (leader)
            _objectCache.SetFollowTarget(leader);
    }
    else{
        _objectCache.SetGroupLeader(nullptr);
        _objectCache.SetGroupMembers({});
        _objectCache.SetFollowTarget(nullptr);
    }
    // ========================================================================
    // PHASE 1: CORE BEHAVIORS - Budget-gated per RPG state
    // ========================================================================
    // At this point MINIMAL tier already jumped to throttled_update_complete.
    // REDUCED and FULL both reach here.

    // Update internal values and caches (REDUCED+)
    UpdateValues(diff);

    // Phase 2 Week 3: Update Hybrid AI (Utility AI + Behavior Trees) — FULL only
    if (_currentBudgetTier == AIBudgetTier::FULL)
        UpdateHybridAI(diff);

    // Update all active strategies (including follow, idle, social) — REDUCED+
    // CRITICAL: Must run every frame for smooth following/movement
    UpdateStrategies(diff);

    // Process all triggers — FULL only
    if (_currentBudgetTier == AIBudgetTier::FULL)
        ProcessTriggers();

    // Execute queued and triggered actions — FULL only
    if (_currentBudgetTier == AIBudgetTier::FULL)
        UpdateActions(diff);

    // Update movement based on strategy decisions — REDUCED+
    // CRITICAL: Must run every frame for smooth movement
    UpdateMovement(diff);

    // ========================================================================
    // DUNGEON AUTONOMY SYSTEM - Autonomous dungeon navigation — FULL only
    // ========================================================================
    // Note: Bots in dungeons always have tier FULL (forced in reassessment),
    // but guard explicitly for safety.
    if (_currentBudgetTier == AIBudgetTier::FULL)
    {
        if (Map* map = _bot->GetMap())
        {
            if (map->IsDungeon() && _bot->GetGroup())
            {
                if (sDungeonAutonomyMgr->UpdateBotInDungeon(_bot, this, diff))
                {
                    // Dungeon autonomy is handling movement/pulling
                }
            }
        }
    }

    // ========================================================================
    // PHASE 2: STATE MANAGEMENT — FULL only
    // ========================================================================
    // REDUCED bots rely on instant combat escalation (per-tick check above)
    // to switch to FULL before needing full combat state processing.
    if (_currentBudgetTier == AIBudgetTier::FULL)
        UpdateCombatState(diff);

    // ========================================================================
    // PHASE 3: CLASS-SPECIFIC UPDATES - Combat and Non-Combat — FULL only
    // ========================================================================
    if (_currentBudgetTier == AIBudgetTier::FULL)
    {
        if (IsInCombat())
        {
            OnCombatUpdate(diff);
        }
        else
        {
            OnNonCombatUpdate(diff);
        }
    }

    // ========================================================================
    // PHASE 4: GROUP INVITATION PROCESSING - Critical for joining groups
    // ========================================================================
    // Process pending group invitations
    // CRITICAL: Must run every frame to accept invitations promptly
    if (auto* groupInvitationHandler = GetGroupInvitationHandler())
    {
        groupInvitationHandler->Update(diff);
    }

    }  // End of if (!isInDeathRecovery) block - normal AI skipped when dead

throttled_update_complete:  // ST-1: Jump here when AI update is throttled
bg_update_complete:
    // ========================================================================
    // PHASE 6: GAME SYSTEMS FACADE - All manager updates delegated to facade
    // ========================================================================
    // Facade handles:
    // - All 17 manager updates (Quest, Trade, Gathering, Auction, etc.)
    // - UnifiedMovementCoordinator (CRITICAL for death recovery corpse navigation)
    // - EventDispatcher and ManagerRegistry processing
    // - Equipment, Profession, Banking automation timers
    // - Singleton manager bridge updates (GatheringMaterialsBridge, etc.)
    //
    // NOTE: Managers continue to update even during death recovery to prevent system freezing
    if (_gameSystems)
    {
        // Pass budget tier to GameSystemsManager for manager-level gating
        if (auto* gsm = dynamic_cast<GameSystemsManager*>(_gameSystems.get()))
            gsm->SetBudgetTier(_currentBudgetTier);
        _gameSystems->Update(diff);
    }

    // Phase 3: Update Tactical Group Coordinator (throttled to 500ms) — FULL only
    if (_currentBudgetTier == AIBudgetTier::FULL)
    {
        if (auto tacticalCoordinator = GetTacticalCoordinator())
        {
            if (_bot->GetGroup())
                tacticalCoordinator->Update(diff);
        }
    }

    // ========================================================================
    // PHASE 7.3: EVENT SYSTEM - Events processed via EventDispatcher
    // ========================================================================
    // Legacy BotEventSystem removed - events now flow through per-bot EventDispatcher

    // ========================================================================
    // PHASE 7: SOLO BEHAVIORS - Only when bot is in solo play mode — FULL only
    // ========================================================================
    if (_currentBudgetTier == AIBudgetTier::FULL && !IsInCombat() && !IsFollowing())
    {
        TC_LOG_TRACE("module.playerbot", "UpdateSoloBehaviors for bot {}", _bot->GetName());
        UpdateSoloBehaviors(diff);
    }

    // ========================================================================
    // PHASE 8: GROUP MANAGEMENT - Check for group changes
    // ========================================================================

    // Check if bot left group and trigger cleanup
    bool isInGroup = (_bot->GetGroup() != nullptr);// FIX #1: Handle bot joining group on server reboot (was already in group before restart)
    if (!_wasInGroup && isInGroup)
    {
        TC_LOG_INFO("playerbot", "Bot {} detected in group (server reboot or first login), calling OnGroupJoined()",
                    _bot->GetName());

        // PHASE 0 - Quick Win #3: Dispatch GROUP_JOINED event
        if (auto* eventDispatcher = GetEventDispatcher())
        {
            Events::BotEvent evt(StateMachine::EventType::GROUP_JOINED, _bot->GetGUID());
            eventDispatcher->Dispatch(std::move(evt));
            TC_LOG_INFO("playerbot", "📢 GROUP_JOINED event dispatched for bot {} (reboot detection)", _bot->GetName());
        }

        OnGroupJoined(_bot->GetGroup());
    }
    // FIX #2: Handle bot leaving group
    else if (_wasInGroup && !isInGroup)
    {
        TC_LOG_INFO("playerbot", "Bot {} left group, calling OnGroupLeft()", _bot->GetName());

        // PHASE 0 - Quick Win #3: Dispatch GROUP_LEFT event for instant cleanup
        if (auto* eventDispatcher = GetEventDispatcher())
        {
            Events::BotEvent evt(StateMachine::EventType::GROUP_LEFT, _bot->GetGUID());
            eventDispatcher->Dispatch(std::move(evt));
            TC_LOG_INFO("playerbot", "📢 GROUP_LEFT event dispatched for bot {}", _bot->GetName());
        }

        OnGroupLeft();
    }
    _wasInGroup = isInGroup;

    // ========================================================================
    // PHASE 9: PERFORMANCE TRACKING
    // ========================================================================

    auto endTime = std::chrono::high_resolution_clock::now();auto updateTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    // Update performance metrics
    if (_performanceMetrics.averageUpdateTime.count() == 0)
        _performanceMetrics.averageUpdateTime = updateTime;
    else
        _performanceMetrics.averageUpdateTime = (_performanceMetrics.averageUpdateTime + updateTime) / 2;
    if (updateTime > _performanceMetrics.maxUpdateTime)
        _performanceMetrics.maxUpdateTime = updateTime;

    _performanceMetrics.lastUpdate = std::chrono::steady_clock::now();
}

// ============================================================================
// STRATEGY UPDATES - Core behavior system
// ============================================================================

void BotAI::UpdateStrategies(uint32 diff)
{
    // CRITICAL: This must run EVERY frame for following to work properly
    // No throttling allowed here!

    // ========================================================================
    // PHASE 1: Collect all active strategies WITHOUT holding lock
    // ========================================================================

    std::vector<Strategy*> strategiesToCheck;
    {
        std::lock_guard lock(_mutex);

        for (auto const& strategyName : _activeStrategies)
        {
            auto it = _strategies.find(strategyName);
            if (it != _strategies.end())
            {
                strategiesToCheck.push_back(it->second.get());
            }
        }
    } // RELEASE LOCK IMMEDIATELY

    // ========================================================================
    // PHASE 2: Filter active strategies and check IsActive()
    // ========================================================================

    std::vector<Strategy*> activeStrategies;

    for (Strategy* strategy : strategiesToCheck)
    {
        if (strategy && strategy->IsActive(this))
        {
            activeStrategies.push_back(strategy);
        }
    }

    // ========================================================================
    // PHASE 3: Use BehaviorPriorityManager to select highest priority strategy
    // ========================================================================

    Strategy* selectedStrategy = nullptr;
    if (_priorityManager && !activeStrategies.empty())
    {
        // Update context (combat state, fleeing, etc.)
        _priorityManager->UpdateContext();

        // Select highest priority valid strategy
        selectedStrategy = _priorityManager->SelectActiveBehavior(activeStrategies);
    }

    // ========================================================================
    // PHASE 4: Execute the selected strategy
    // ========================================================================

    if (selectedStrategy)
    {
        // Special handling for follow strategy - needs every frame update
        if (auto* followBehavior = dynamic_cast<LeaderFollowBehavior*>(selectedStrategy))
        {
            followBehavior->UpdateFollowBehavior(this, diff);
        }
        else
        {
            // Use throttled update - MaybeUpdateBehavior respects update intervals
            // based on CombatContext (SOLO=150ms, ARENA=25ms, etc.)
            // Strategies that need every-frame updates override NeedsEveryFrameUpdate()
            selectedStrategy->MaybeUpdateBehavior(this, diff);
        }

        _performanceMetrics.strategiesEvaluated = 1;
    }
    else
    {
        _performanceMetrics.strategiesEvaluated = 0;
    }
}

// ============================================================================
// MOVEMENT UPDATES - Strategy-controlled movement
// ============================================================================

void BotAI::UpdateMovement(uint32 /*diff*/)
{
    // CRITICAL: Movement is controlled by strategies (especially follow)
    // This method just ensures movement commands are processed
    // Must run every frame for smooth movementif (!_bot || !_bot->IsAlive())
        return;

    // Movement is primarily handled by strategies (follow, combat positioning, etc.)// This is just for ensuring movement updates are processed
    if (_bot->GetMotionMaster())
    {
        // Motion master will handle actual movement updates
        // We just ensure it's being processed
    }
}

// ============================================================================
// COMBAT STATE MANAGEMENT// ============================================================================

void BotAI::UpdateCombatState(uint32 diff)
{
    bool wasInCombat = IsInCombat();
    bool isInCombat = _bot && _bot->IsInCombat();// DIAGNOSTIC: Log combat state every 2 seconds
    static uint32 lastCombatStateLog = 0;
    uint32 now = GameTime::GetGameTimeMS();
    if (now - lastCombatStateLog > 2000)
    {
        TC_LOG_ERROR("module.playerbot", "🔍 UpdateCombatState: Bot {} - wasInCombat={}, isInCombat={}, AIState={}, HasVictim={}",_bot ? _bot->GetName() : "null",
                     wasInCombat, isInCombat,
                     static_cast<uint32>(_aiState),
                     (_bot && _bot->GetVictim()) ? "YES" : "NO");
        lastCombatStateLog = now;
    }

    // Handle combat state transitions
    if (!wasInCombat && isInCombat)
    {
        // Entering combat
        TC_LOG_ERROR("module.playerbot", "⚔️ ENTERING COMBAT: Bot {}", _bot->GetName());
        SetAIState(BotAIState::COMBAT);

        // Find initial target
        // FIX #19: Use ObjectCache instead of ObjectAccessor to avoid TrinityCore deadlock
        ::Unit* target = _objectCache.GetTarget();
        if (target)
        {
            TC_LOG_ERROR("module.playerbot", "🎯 Target from cache: {}", target->GetName());
        }

        // CRITICAL FIX: Try GetVictim() as fallback if cache has no target
        // This is the primary source when bot enters combat reactively
        if (!target)
        {
            target = _bot->GetVictim();
            if (target)
            {
                TC_LOG_ERROR("module.playerbot", "🎯 Target from GetVictim(): {}", target->GetName());
            }
        }

        // CRITICAL FIX #2: If still no target, find who's attacking us (reactive combat)
        // When a mob attacks the bot, GetVictim() is nullptr because we haven't called Attack() yet
        // We need to find the attacker from our threat list or attackers list
        //
        // NOTE: getAttackers() returns a reference to a set that can be modified by the main thread
        // while we iterate. This is a potential race condition. We wrap in try-catch and copy
        // the GUIDs first to minimize the window of potential corruption.
        if (!target)
        {
            try
            {
                // Copy attacker GUIDs first (smaller window for race condition)
                std::vector<ObjectGuid> attackerGuids;
                {
                    Unit::AttackerSet const& attackers = _bot->getAttackers();
                    attackerGuids.reserve(attackers.size());
                    for (Unit* attacker : attackers)
                    {
                        if (attacker)
                            attackerGuids.push_back(attacker->GetGUID());
                    }
                }

                // Now safely iterate the copied GUIDs
                for (ObjectGuid const& guid : attackerGuids)
                {
                    Unit* attacker = ObjectAccessor::GetUnit(*_bot, guid);
                    if (attacker && attacker->IsAlive() && _bot->IsValidAttackTarget(attacker))
                    {
                        target = attacker;
                        TC_LOG_ERROR("module.playerbot", "🎯 Target from getAttackers(): {}", target->GetName());
                        break;
                    }
                }
            }
            catch (...)
            {
                TC_LOG_ERROR("module.playerbot", "⚠️ Exception while iterating attackers for bot {}", _bot->GetName());
            }
        }

        if (target)
        {
            TC_LOG_ERROR("module.playerbot", "✅ Calling OnCombatStart() with target {}", target->GetName());
            OnCombatStart(target);
        }
        else
        {
            TC_LOG_ERROR("module.playerbot", "❌ COMBAT START FAILED: No valid target found!");
        }
    }
    else if (wasInCombat && !isInCombat)
    {
        // Leaving combat
        TC_LOG_ERROR("module.playerbot", "🏳️ LEAVING COMBAT: Bot {}", _bot->GetName());OnCombatEnd();

        // Determine new state
if (_bot->GetGroup() && GetStrategy("follow"))
            SetAIState(BotAIState::FOLLOWING);
        else
            SetAIState(BotAIState::SOLO);
    }
}

// ============================================================================
// TRIGGER PROCESSING
// ============================================================================

void BotAI::ProcessTriggers()
{
    if (!_bot)
        return;

    // Clear previous triggered actions
    while (!_triggeredActions.empty())
        _triggeredActions.pop();

    // Evaluate all triggers
    for (auto const& trigger : _triggers)
    {
        if (trigger && trigger->Check(this))
        {auto result = trigger->Evaluate(this);
            if (result.triggered && result.suggestedAction)
            {
                _triggeredActions.push(result);
                _performanceMetrics.triggersProcessed++;}
        }
    }
}

// ============================================================================
// ACTION EXECUTION
// ============================================================================

void BotAI::UpdateActions(uint32 diff)
{
    // Execute current action if in progress
    if (_currentAction)
    {
        // Check if action is still valid
        if (!_currentAction->IsUseful(this))
        {
            CancelCurrentAction();
        }
        else
        {
            // Action still in progress
            return;
        }
    }

    // Process triggered actions first (higher priority)
    if (!_triggeredActions.empty())
    {
        auto const& result = _triggeredActions.top();
        if (result.suggestedAction && CanExecuteAction(result.suggestedAction.get()))
        {
            auto execResult = ExecuteActionInternal(result.suggestedAction.get(), result.context);
            if (execResult == ActionResult::SUCCESS || execResult == ActionResult::IN_PROGRESS)
            {
                _currentAction = result.suggestedAction;
                _performanceMetrics.actionsExecuted++;
            }
        }
        _triggeredActions.pop();
        return;
    }

    // Process queued actions
    if (!_actionQueue.empty())
    {
        auto [action, context] = _actionQueue.front();
        _actionQueue.pop();

        if (action && CanExecuteAction(action.get()))
        {
            auto result = ExecuteActionInternal(action.get(), context);
            if (result == ActionResult::SUCCESS || result == ActionResult::IN_PROGRESS)
            {
                _currentAction = action;
                _currentContext = context;
                _performanceMetrics.actionsExecuted++;
            }
        }
    }
}

// ============================================================================
// SOLO BEHAVIORS - Autonomous play when not in group
// ============================================================================

void BotAI::UpdateSoloBehaviors(uint32 diff)
{
    // Only run solo behaviors when in solo play mode (not grouped/following)
    if (IsInCombat() || IsFollowing())
        return;

    // ========================================================================
    // RPG DAILY ROUTINE - Autonomous activity simulation for masterless bots
    // ========================================================================
    if (_rpgRoutineManager)
    {
        _rpgRoutineManager->Update(diff);
    }

    uint32 currentTime = GameTime::GetGameTimeMS();

    // ========================================================================
    // AUTONOMOUS TARGET SCANNING - Find enemies when solo
    // ========================================================================

    // For solo bots (not in a group), actively scan for nearby enemies
    if (!_bot->GetGroup())
    {
        if (auto* targetScanner = GetTargetScanner())
        {
            // Check if it's time to scan (throttled for performance)
            if (targetScanner->ShouldScan(currentTime))
            {
                targetScanner->UpdateScanTime(currentTime);

                // Clean up blacklist
                targetScanner->UpdateBlacklist(currentTime);

                // ENTERPRISE-GRADE THREAD-SAFE TARGET RESOLUTION
                // Find best target to engage (returns GUID, thread-safe)
                ObjectGuid bestTargetGuid = targetScanner->FindBestTarget();

                // DEADLOCK FIX: Validate and set target using spatial grid snapshots (lock-free, thread-safe)
                // DO NOT use ObjectAccessor::GetUnit() from worker thread!

                if (!bestTargetGuid.IsEmpty())
                {
                    // Get spatial grid for lock-free snapshot queries
                    auto spatialGrid = sSpatialGridManager.GetGrid(_bot->GetMapId());
                    if (spatialGrid)
                    {
                        // Query nearby creature snapshots (lock-free read from atomic buffer)
                        auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(_bot->GetPosition(), 60.0f);

                        // Find the snapshot matching our target GUID
                        for (auto const& snapshot : creatureSnapshots)
                        {
                            if (snapshot.guid == bestTargetGuid)
                            {
                                // Snapshot found - validate using snapshot data (no Map access needed)
                                // 1. Check if creature is alive (!isDead flag)
                                // 2. Check if creature is attackable (isHostile flag)
                                // 3. Check distance is within engage range (60.0f)

                                float distance = std::sqrt(_bot->GetExactDistSq(snapshot.position)); // Calculate once from squared distance
                                if (!snapshot.isDead &&
                                    snapshot.isHostile &&
                                    distance <= 60.0f)
                                {
                                    // Target is valid based on snapshot data
                                    // SetTarget() is thread-safe (just sets a GUID)
                                    _bot->SetTarget(bestTargetGuid);

                                    TC_LOG_DEBUG("playerbot.solo",
                                        "Solo bot {} selected target Entry {} (level {}) via spatial grid snapshot - combat will engage naturally",
                                        _bot->GetName(), snapshot.entry, snapshot.level);

                                    // Combat will initiate naturally:
                                    // - Bot has target set → ClassAI will cast spells/attack
                                    // - CombatMovementStrategy will handle positioning
                                    // - Threat will be established when damage lands
                                    // NO NEED for explicit Attack() or SetInCombatWith() calls from worker thread
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // ========================================================================
    // GAME SYSTEM MANAGER UPDATES
    // ========================================================================

    // Managers will be updated via UpdateManagers() which is called in UpdateAI()
    // No need to manually update them here since they inherit from BehaviorManager
    // and have their own throttling mechanisms

    // NOTE: Social interactions (chat, emotes) are handled by the Humanization
    // subsystem (CityLifeBehaviorManager, ActivityExecutor) when bots are idle.
}

// ============================================================================
// BUDGET TIER TRANSITION
// ============================================================================

void BotAI::OnBudgetTierTransition(AIBudgetTier oldTier, AIBudgetTier newTier)
{
    // P6: Adjust save frequency based on new tier
    Playerbot::BotSaveController::instance()->OnBudgetTierChange(_bot, newTier);

    // P5: Clear non-essential caches when transitioning to MINIMAL
    if (newTier == AIBudgetTier::MINIMAL)
        _objectCache.ClearNonEssential();

    TC_LOG_DEBUG("module.playerbot",
        "BotAI: {} tier {} -> {} (save interval adjusted, caches {})",
        _bot->GetName(),
        static_cast<uint8>(oldTier),
        static_cast<uint8>(newTier),
        newTier == AIBudgetTier::MINIMAL ? "pruned" : "retained");
}

// ============================================================================
// STATE TRANSITIONS
// ============================================================================

void BotAI::OnCombatStart(::Unit* target)
{
    _currentTarget = target ? target->GetGUID() : ObjectGuid::Empty;
    TC_LOG_DEBUG("playerbot", "Bot {} entering combat with {}",_bot->GetName(), target ? target->GetName() : "unknown");

    // Strategies don't have OnCombatStart - combat is handled by ClassAI
    // through the OnCombatUpdate() method

    // ST-1: Notify throttler to switch to full update rate during combat
    if (_aiUpdateThrottler)
        _aiUpdateThrottler->OnCombatStart();
}

void BotAI::OnCombatEnd()
{
    _currentTarget = ObjectGuid::Empty;
    TC_LOG_DEBUG("playerbot", "Bot {} leaving combat", _bot->GetName());

    // FIX #4: Resume following after combat ends (if in group)
    if (_bot->GetGroup())
    {
        TC_LOG_INFO("playerbot", "Bot {} combat ended, resuming follow behavior", _bot->GetName());
        SetAIState(BotAIState::FOLLOWING);

        // Clear ONLY non-follow movement types to allow follow strategy to take over
        // Don't clear if already following, as that would cause stuttering
        MotionMaster* mm = _bot->GetMotionMaster();
        if (mm)
        {
            ::MovementGeneratorType currentType = mm->GetCurrentMovementGeneratorType(MOTION_SLOT_ACTIVE);
            if (currentType != FOLLOW_MOTION_TYPE && currentType != IDLE_MOTION_TYPE)
            {
                TC_LOG_ERROR("playerbot", "🧹 OnCombatEnd: Clearing {} motion type for bot {} to allow follow",
                            static_cast<uint32>(currentType), _bot->GetName());
                mm->Clear();
            }
        }
    }
    else
    {
        // Not in group, return to solo play mode
        SetAIState(BotAIState::SOLO);
    }

    // Strategies don't have OnCombatEnd - combat is handled by ClassAI
    // through the OnCombatUpdate() method

    // ST-1: Notify throttler that combat ended - will reassess update rate
    if (_aiUpdateThrottler)
        _aiUpdateThrottler->OnCombatEnd();
}

::SpellCastResult BotAI::CastSpell(uint32 spellId, ::Unit* target)
{
    // Base implementation - ClassAI overrides with actual spell casting logic
    // This is just a fallback for non-ClassAI bots
    return SPELL_FAILED_NOT_READY;
}

void BotAI::OnDeath()
{
    SetAIState(BotAIState::DEAD);
    CancelCurrentAction();

    // Clear action queue
    while (!_actionQueue.empty())
        _actionQueue.pop();

    // Pause quest completion (Phase 0 hook integration)
    if (auto* questCompletion = GetQuestCompletion())
    {
        questCompletion->PauseQuestCompletion(_bot->GetGUID().GetCounter());
        TC_LOG_DEBUG("playerbots.ai", "Bot {} died - quest completion paused", _bot->GetName());
    }

    // Initiate death recovery process
    if (auto* deathRecoveryManager = GetDeathRecoveryManager())
    {
        TC_LOG_ERROR("playerbots.ai", "Bot {} died - calling DeathRecoveryManager::OnDeath()", _bot->GetName());
        deathRecoveryManager->OnDeath();
    }
    else
    {
        TC_LOG_ERROR("playerbots.ai", "Bot {} died but GetDeathRecoveryManager() returned nullptr! _gameSystems={}",
            _bot->GetName(), _gameSystems ? "valid" : "null");
    }

    TC_LOG_DEBUG("playerbots.ai", "Bot {} died, AI state reset, death recovery initiated", _bot->GetName());
}

void BotAI::OnRespawn()
{
    SetAIState(BotAIState::SOLO);
    Reset();

    // Complete death recovery process
    if (auto* deathRecoveryManager = GetDeathRecoveryManager())
        deathRecoveryManager->OnResurrection();

    // Resume quest completion (Phase 0 hook integration)
    if (auto* questCompletion = GetQuestCompletion())
    {
        questCompletion->ResumeQuestCompletion(_bot->GetGUID().GetCounter());
        TC_LOG_DEBUG("playerbots.ai", "Bot {} respawned - quest completion resumed", _bot->GetName());
    }

    TC_LOG_DEBUG("playerbots.ai", "Bot {} respawned, AI reset, death recovery completed", _bot->GetName());
}

void BotAI::Reset()
{
    _currentTarget = ObjectGuid::Empty;
    _aiState = BotAIState::SOLO;

    CancelCurrentAction();

    while (!_actionQueue.empty())
        _actionQueue.pop();

    while (!_triggeredActions.empty())
        _triggeredActions.pop();
}

// ============================================================================
// GROUP MANAGEMENT
// ============================================================================

void BotAI::OnGroupJoined(Group* group)
{
    // Get group from bot if not provided (handles login scenario)
    if (!group && _bot)
        group = _bot->GetGroup();

    TC_LOG_INFO("module.playerbot.ai", "🚨 OnGroupJoined called for bot {}, provided group={}, bot's group={}",
                _bot ? _bot->GetName() : "NULL", (void*)group, _bot ? (void*)_bot->GetGroup() : nullptr);

    if (!group)
    {
        TC_LOG_INFO("module.playerbot.ai", "❌ OnGroupJoined: No group available for bot {}",
                    _bot ? _bot->GetName() : "NULL");
        return;
    }

    TC_LOG_INFO("module.playerbot.ai", "Bot {} joined group {}, activating follow and combat strategies",
                _bot->GetName(), (void*)group);// DEADLOCK FIX #12: This method was acquiring mutex MULTIPLE times:
    // 1. GetStrategy("follow") - shared_lock
    // 2. AddStrategy() - unique_lock
    // 3. GetStrategy("group_combat") - shared_lock
    // 4. AddStrategy() - unique_lock
    // 5. ActivateStrategy("follow") - unique_lock
    // 6. ActivateStrategy("group_combat") - unique_lock
    // 7. Final confirmation block - shared_lock
    //
    // When UpdateStrategies() runs in another thread and releases its lock,
    // then ActivateStrategy() tries to acquire unique_lock, and THIS method
    // tries to acquire another lock, DEADLOCK occurs due to writer-preference!
    //
    // Solution: Do ALL mutex operations in a SINGLE critical section,
    // then call OnActivate() callbacks AFTER releasing the lock

    std::vector<Strategy*> strategiesToActivate;

    // PHASE 1: Check strategy existence and activate - ALL UNDER ONE LOCK
    {
        std::lock_guard lock(_mutex);

        // Check if follow strategy existsif (_strategies.find("follow") == _strategies.end())
        {
            TC_LOG_ERROR("playerbot", "CRITICAL: Follow strategy not found for bot {} - creating emergency fallback",_bot->GetName());

            // Create it immediately while we hold the lock
            auto followBehavior = std::make_unique<LeaderFollowBehavior>();
            _strategies["follow"] = std::move(followBehavior);TC_LOG_WARN("playerbot", "Created emergency follow strategy for bot {}", _bot->GetName());
        }

        // Check if group combat strategy exists
        if (_strategies.find("group_combat") == _strategies.end())
        {
            TC_LOG_ERROR("playerbot", "CRITICAL: GroupCombat strategy not found for bot {} - creating emergency fallback",_bot->GetName());

            // Create it immediately while we hold the lock
            auto groupCombat = std::make_unique<GroupCombatStrategy>();
            _strategies["group_combat"] = std::move(groupCombat);TC_LOG_WARN("playerbot", "Created emergency group_combat strategy for bot {}", _bot->GetName());
        }

        // Activate follow strategy (while still holding lock)
        {
            bool alreadyInList = std::find(_activeStrategies.begin(), _activeStrategies.end(), "follow") != _activeStrategies.end();
            auto it = _strategies.find("follow");
            if (it != _strategies.end())
            {
                bool wasActive = it->second->IsActive(this);

                TC_LOG_ERROR("playerbot", "🔍 OnGroupJoined: Bot {} follow strategy - alreadyInList={}, wasActive={}",_bot->GetName(), alreadyInList, wasActive);

                if (!alreadyInList)
                    _activeStrategies.push_back("follow");

                it->second->SetActive(true);

                // CRITICAL FIX: ALWAYS call OnActivate to ensure follow target is set
                // This handles server restart where bot loads with group but follow not initialized
                strategiesToActivate.push_back(it->second.get());

                TC_LOG_ERROR("playerbot", "✅ OnGroupJoined: Bot {} queued follow strategy for OnActivate callback", _bot->GetName());
            }
        }

        // Activate group combat strategy (while still holding lock)
        {
            bool alreadyInList = std::find(_activeStrategies.begin(), _activeStrategies.end(), "group_combat") != _activeStrategies.end();
            auto it = _strategies.find("group_combat");if (it != _strategies.end())
            {
                bool wasActive = it->second->IsActive(this);

                if (!alreadyInList)
                    _activeStrategies.push_back("group_combat");

                it->second->SetActive(true);// CRITICAL FIX: Call OnActivate if newly added OR not properly initialized
                if (!alreadyInList || !wasActive)
                    strategiesToActivate.push_back(it->second.get());
            }
        }

        // Confirm activation (still under same lock)
        bool followActive = std::find(_activeStrategies.begin(), _activeStrategies.end(), "follow") != _activeStrategies.end();
        bool combatActive = std::find(_activeStrategies.begin(), _activeStrategies.end(), "group_combat") != _activeStrategies.end();

        if (followActive && combatActive)
        {TC_LOG_INFO("playerbot", "✅ Successfully activated follow and group_combat strategies for bot {}", _bot->GetName());
        }
        else
        {
            TC_LOG_ERROR("playerbot", "❌ Strategy activation FAILED for bot {} - follow={}, combat={}",_bot->GetName(), followActive, combatActive);
        }
    } // RELEASE LOCK - all operations completed

    // PHASE 2: Call OnActivate() callbacks WITHOUT holding lock
    for (Strategy* strategy : strategiesToActivate)
    {
        if (strategy)
            strategy->OnActivate(this);
    }

    // CRITICAL FIX: Deactivate ALL solo strategies when joining a group
    // These strategies have the same priority (FOLLOW=50) as the follow strategy,
    // and would compete with it, preventing follow from being selected.
    // Solo strategies should only be active when bot is NOT in a group.
    DeactivateStrategy("solo");
    DeactivateStrategy("quest");      // Solo questing - groups do group content
    DeactivateStrategy("solo_combat"); // Solo combat - groups use group_combat
    DeactivateStrategy("loot");       // Auto-looting can interfere with group loot rules
    // Note: "rest" strategy remains active as it has FLEEING priority (90) for health/mana recovery

    // Phase 3: Tactical coordinator now integrated into Advanced/GroupCoordinator
    // TacticalCoordinator is created by GroupCoordinator::Initialize()

    // Set state to following if not in combat (no lock needed - atomic operation)
    if (!IsInCombat())
        SetAIState(BotAIState::FOLLOWING);

    _wasInGroup = true;
}

void BotAI::OnGroupLeft()
{
    TC_LOG_INFO("playerbot", "Bot {} left group, deactivating follow and combat strategies",_bot->GetName());

    // DEADLOCK FIX #12: Same as OnGroupJoined - do all operations under one lock
    std::vector<Strategy*> strategiesToDeactivate;

    {
        std::lock_guard lock(_mutex);

        // Deactivate follow strategy
        auto followIt = _strategies.find("follow");
        if (followIt != _strategies.end())
        {
            followIt->second->SetActive(false);
            strategiesToDeactivate.push_back(followIt->second.get());
        }

        _activeStrategies.erase(
            std::remove(_activeStrategies.begin(), _activeStrategies.end(), "follow"),
            _activeStrategies.end()
        );

        // Deactivate group combat strategy
        auto combatIt = _strategies.find("group_combat");
        if (combatIt != _strategies.end()){
            combatIt->second->SetActive(false);
            strategiesToDeactivate.push_back(combatIt->second.get());
        }

        _activeStrategies.erase(
            std::remove(_activeStrategies.begin(), _activeStrategies.end(), "group_combat"),
            _activeStrategies.end()
        );
    } // RELEASE LOCK

    // Call OnDeactivate() callbacks WITHOUT holding lock
    for (Strategy* strategy : strategiesToDeactivate)
    {
        if (strategy)
            strategy->OnDeactivate(this);
    }

    // Phase 3: TacticalCoordinator cleanup
    // NOTE: TacticalCoordinator is now owned by GroupCoordinator (in GameSystemsManager),
    // so we don't need to manually reset it here. GroupCoordinator handles its lifecycle.
    TC_LOG_INFO("playerbot.coordination", "🔴 Bot {} left group, TacticalCoordinator cleanup handled by GroupCoordinator",
        _bot->GetName());

    // Activate all solo strategies when leaving a group
    // These are the same strategies activated in UpdateAI() for solo bots
    ActivateStrategy("rest");        // Priority: FLEEING (90) - eating/drinking
    ActivateStrategy("solo_combat"); // Priority: COMBAT (100) - solo combat handling
    ActivateStrategy("quest");       // Priority: FOLLOW (50) - quest objectives
    ActivateStrategy("grind");       // Priority: GRIND (40) - fallback when quests unavailable
    ActivateStrategy("loot");        // Priority: MOVEMENT (45) - corpse looting
    ActivateStrategy("solo");        // Priority: SOLO (10) - fallback coordinator
    TC_LOG_INFO("module.playerbot.ai", "🎯 SOLO BOT REACTIVATION: Bot {} reactivated solo strategies after leaving group", _bot->GetName());

    // Set state to solo if not in combatif (!IsInCombat())
        SetAIState(BotAIState::SOLO);_wasInGroup = false;
}

void BotAI::HandleGroupChange()
{
    // Check current group status
    bool inGroup = (_bot && _bot->GetGroup() != nullptr);if (inGroup && !_wasInGroup)
    {
        OnGroupJoined(_bot->GetGroup());
    }
    else if (!inGroup && _wasInGroup)
    {
        OnGroupLeft();
    }
}
// ============================================================================
// STRATEGY MANAGEMENT
// ============================================================================

void BotAI::AddStrategy(std::unique_ptr<Strategy> strategy)
{
    if (!strategy)
        return;

    std::lock_guard lock(_mutex);
    std::string const& name = strategy->GetName();
    Strategy* strategyPtr = strategy.get();
    _strategies[name] = std::move(strategy);

    // Auto-register with priority manager based on strategy name
    if (_priorityManager)
    {
        BehaviorPriority priority = BehaviorPriority::SOLO; // Default
        bool exclusive = false;

        // Determine priority from strategy name
        if (name.find("combat") != std::string::npos || name.find("group_combat") != std::string::npos)
        {
            priority = BehaviorPriority::COMBAT;
            exclusive = true; // Combat gets exclusive control
        }
        else if (name == "follow")
        {
            priority = BehaviorPriority::FOLLOW;}
        else if (name.find("flee") != std::string::npos){
            priority = BehaviorPriority::FLEEING;
            exclusive = true;
        }
        else if (name.find("cast") != std::string::npos)
        {priority = BehaviorPriority::CASTING;
        }
        else if (name == "quest")
        {
            // Quest strategy gets MOVEMENT priority (45)
            // Lower than loot so bots loot corpses before continuing quests
            priority = BehaviorPriority::MOVEMENT;
        }
        else if (name == "loot")
        {
            // Loot strategy gets FOLLOW priority (50) - HIGHER than quest
            // Ensures bots loot corpses immediately after combat before wandering
            priority = BehaviorPriority::FOLLOW;
        }
        else if (name == "rest")
        {
            // Rest strategy gets FLEEING priority (90) - HIGHEST for survival// Bots must rest when health/mana low before doing anything else
            priority = BehaviorPriority::FLEEING;}
        else if (name.find("gather") != std::string::npos)
        {
            priority = BehaviorPriority::GATHERING;
        }
        else if (name.find("trade") != std::string::npos)
        {
            priority = BehaviorPriority::TRADING;
        }
        else if (name == "solo")
        {
            priority = BehaviorPriority::SOLO;
        }

        _priorityManager->RegisterStrategy(strategyPtr, priority, exclusive);

        TC_LOG_DEBUG("module.playerbot.ai", "Registered strategy '{}' with priority {} (exclusive={})",
                     name, static_cast<uint8>(priority), exclusive);
    }
}

void BotAI::RemoveStrategy(std::string const& name)
{
    std::lock_guard lock(_mutex);

    // Unregister from priority manager before removing
    auto it = _strategies.find(name);
    if (it != _strategies.end() && _priorityManager)
    {
        _priorityManager->UnregisterStrategy(it->second.get());
    }

    _strategies.erase(name);

    // Also remove from active strategies
    _activeStrategies.erase(
        std::remove(_activeStrategies.begin(), _activeStrategies.end(), name),_activeStrategies.end()
    );
}

Strategy* BotAI::GetStrategy(std::string const& name) const
{
    std::lock_guard lock(_mutex);
    auto it = _strategies.find(name);
    return it != _strategies.end() ? it->second.get() : nullptr;
}

std::vector<Strategy*> BotAI::GetActiveStrategies() const
{
    // DEADLOCK FIX #11: Release lock BEFORE returning vector
    // The previous implementation held the lock during return, which means
    // the lock was held while the vector was being copied/moved.
    // If another thread requested unique_lock during that time,
    // and the calling code then tried to call GetStrategy(), DEADLOCK!
    std::vector<Strategy*> result;
    {
        std::lock_guard lock(_mutex);

        // Access _strategies directly to avoid recursive mutex acquisition
        for (auto const& name : _activeStrategies)
        {
            auto it = _strategies.find(name);
            if (it != _strategies.end())
                result.push_back(it->second.get());
        }
    } // RELEASE LOCK BEFORE RETURN

    return result;  // Lock already released
}

void BotAI::ActivateStrategy(std::string const& name)
{
    // DEADLOCK FIX: Collect strategy pointer FIRST, then release lock BEFORE calling OnActivate
    // OnActivate callbacks may call GetStrategy() which acquires shared_lock
    // If another thread holds shared_lock and we hold unique_lock, calling OnActivate
    // which tries to acquire another shared_lock will deadlock due to writer-preference
    Strategy* strategy = nullptr;
    bool needsOnActivate = false;
    {
        std::lock_guard lock(_mutex);

        // Check if strategy exists
        auto it = _strategies.find(name);
        if (it == _strategies.end())
            return;

        // Check if already active
        bool alreadyInList = std::find(_activeStrategies.begin(), _activeStrategies.end(), name) != _activeStrategies.end();
        bool wasActive = it->second->IsActive(this);  // Check BEFORE setting active

        if (!alreadyInList)
        {
            _activeStrategies.push_back(name);
        }

        // CRITICAL FIX: Set the strategy's internal _active flag so IsActive() returns true
        it->second->SetActive(true);

        // CRITICAL FIX: Call OnActivate if strategy is newly activated OR was never properly initialized
        // This handles both: new activations and re-activation of strategies that were improperly added
        needsOnActivate = !alreadyInList || !wasActive;

        TC_LOG_ERROR("module.playerbot.ai", "🔥 ACTIVATED STRATEGY: '{}' for bot {}, alreadyInList={}, wasActive={}, needsOnActivate={}",name, _bot->GetName(), alreadyInList, wasActive, needsOnActivate);

        // Get strategy pointer for callback
        strategy = it->second.get();
    } // RELEASE LOCK BEFORE CALLBACK

    // Call OnActivate hook WITHOUT holding lock if needed
    if (strategy && needsOnActivate)
    {
        TC_LOG_ERROR("module.playerbot.ai", "🎬 Calling OnActivate() for strategy '{}' on bot {}", name, _bot->GetName());
        strategy->OnActivate(this);
        TC_LOG_DEBUG("playerbot", "Activated strategy '{}' for bot {}", name, _bot->GetName());
    }
}

void BotAI::DeactivateStrategy(std::string const& name)
{
    // DEADLOCK FIX: Collect strategy pointer FIRST, then release lock BEFORE calling OnDeactivate
    // OnDeactivate callbacks may call GetStrategy() which acquires shared_lock
    // If another thread holds shared_lock and we hold unique_lock, calling OnDeactivate
    // which tries to acquire another shared_lock will deadlock due to writer-preference
    Strategy* strategy = nullptr;
    {
        std::lock_guard lock(_mutex);

        // Find the strategy
        auto it = _strategies.find(name);
        if (it != _strategies.end())
        {
            // Set the strategy's internal _active flag to false
            it->second->SetActive(false);

            // Get strategy pointer for callback
            strategy = it->second.get();
        }

        _activeStrategies.erase(
            std::remove(_activeStrategies.begin(), _activeStrategies.end(), name),
            _activeStrategies.end()
        );
    } // RELEASE LOCK BEFORE CALLBACK

    // Call OnDeactivate hook WITHOUT holding lock
    if (strategy)
    {
        strategy->OnDeactivate(this);TC_LOG_DEBUG("playerbot", "Deactivated strategy '{}' for bot {}", name, _bot->GetName());
    }
}

// ============================================================================
// ACTION EXECUTION
// ============================================================================

bool BotAI::ExecuteAction(std::string const& actionName)
{
    return ExecuteAction(actionName, ActionContext());
}

bool BotAI::ExecuteAction(std::string const& name, ActionContext const& context)
{
    // Look up action by name using ActionFactory
    if (!sActionFactory->HasAction(name))
    {
        TC_LOG_DEBUG("bot.ai.action", "Unknown action requested: {}", name);
        return false;
    }

    // Create action instance from factory
    std::shared_ptr<Action> action = sActionFactory->CreateAction(name);

    // Check if action is possible before executing
    if (!CanExecuteAction(action.get()))
    {
        TC_LOG_TRACE("bot.ai.action", "Action {} not possible for bot {}", name, _bot->GetName());
        return false;
    }

    // Execute the action
    ActionResult result = action->Execute(this, context);

    // Log execution result
    switch (result)
    {
        case ActionResult::SUCCESS:
            TC_LOG_TRACE("bot.ai.action", "Action {} executed successfully for bot {}", name, _bot->GetName());
            return true;
        case ActionResult::FAILED:
            TC_LOG_DEBUG("bot.ai.action", "Action {} failed for bot {}", name, _bot->GetName());
            return false;
        case ActionResult::CANCELLED:
            TC_LOG_DEBUG("bot.ai.action", "Action {} cancelled for bot {}", name, _bot->GetName());
            return false;
        case ActionResult::IN_PROGRESS:
            // Queue action for continued execution
            QueueAction(action, context);
            TC_LOG_TRACE("bot.ai.action", "Action {} queued for bot {}", name, _bot->GetName());
            return true;
        default:
            return false;
    }
}

bool BotAI::IsActionPossible(std::string const& actionName) const
{
    // Check if action exists in factory
    if (!sActionFactory->HasAction(actionName))
        return false;

    // Create temporary action instance to check possibility
    std::shared_ptr<Action> action = sActionFactory->CreateAction(actionName);
    if (!action)
        return false;

    // Use existing CanExecuteAction check
    return CanExecuteAction(action.get());
}

uint32 BotAI::GetActionPriority(std::string const& actionName) const
{
    // Check if action exists
    if (!sActionFactory->HasAction(actionName))
        return 0;

    // Create action instance to get priority
    std::shared_ptr<Action> action = sActionFactory->CreateAction(actionName);
    if (!action)
        return 0;

    // Get priority from action
    // Actions have internal priority based on their type and context
    // Combat actions are high priority, movement actions are lower
    uint32 basePriority = action->GetPriority();

    // Adjust priority based on current bot state
    if (IsInCombat() && dynamic_cast<CombatAction*>(action.get()))
    {
        basePriority += 50; // Boost combat action priority during combat
    }

    return basePriority;
}

void BotAI::QueueAction(std::shared_ptr<Action> action, ActionContext const& context)
{
    if (!action)
        return;

    _actionQueue.push({action, context});
}

void BotAI::CancelCurrentAction()
{
    _currentAction = nullptr;
    _currentContext = ActionContext();
}

bool BotAI::CanExecuteAction(Action* action) const
{
    if (!action || !_bot)
        return false;

    return action->IsPossible(const_cast<BotAI*>(this)) && action->IsUseful(const_cast<BotAI*>(this));
}

ActionResult BotAI::ExecuteActionInternal(Action* action, ActionContext const& context)
{
    if (!action)
        return ActionResult::FAILED;

    return action->Execute(this, context);
}

// ============================================================================
// TARGET MANAGEMENT
// ============================================================================

::Unit* BotAI::GetTargetUnit() const
{
    if (!_bot || _currentTarget.IsEmpty())
        return nullptr;

    // FIX #19: Use ObjectCache instead of ObjectAccessor to avoid TrinityCore deadlock
    return _objectCache.GetTarget();
}

// ============================================================================
// MOVEMENT CONTROL
// ============================================================================

void BotAI::MoveTo(float x, float y, float z)
{if (!_bot || !_bot->IsAlive())
        return;

    _bot->GetMotionMaster()->MovePoint(0, x, y, z);
}

void BotAI::Follow(::Unit* target, float distance)
{if (!_bot || !_bot->IsAlive() || !target)
        return;

    _bot->GetMotionMaster()->MoveFollow(target, distance, 0.0f);
}

void BotAI::StopMovement()
{
    if (!_bot)
        return;

    _bot->StopMoving();
    _bot->GetMotionMaster()->Clear();
}

bool BotAI::IsMoving() const
{
    return _bot && _bot->isMoving();
}

// ============================================================================
// HELPER METHODS
// ============================================================================

void BotAI::SetAIState(BotAIState state)
{
    if (_aiState != state)
    {
        TC_LOG_DEBUG("playerbot", "Bot {} state change: {} -> {}",_bot->GetName(),
                     static_cast<int>(_aiState),
                     static_cast<int>(state));
        _aiState = state;
    }
}

void BotAI::InitializeDefaultStrategies()
{
    // CRITICAL FIX: Create and register the follow strategy so it exists when activated
    // Without this, ActivateStrategy("follow") fails silently when bot joins group
    auto followBehavior = std::make_unique<LeaderFollowBehavior>();
    AddStrategy(std::move(followBehavior));

    // CRITICAL FIX: Create and register group combat strategy for combat assistance
    // This strategy makes bots attack when group members enter combat
    auto groupCombat = std::make_unique<GroupCombatStrategy>();
    AddStrategy(std::move(groupCombat));

    // CRITICAL: Create and register solo combat strategy for solo bot combat (Priority 70)
    // This strategy handles ALL combat when bot is solo (not in group):
    //  - Quest combat (kills quest targets)
    //  - Gathering defense (attacked while gathering)
    //  - Autonomous combat (bot finds hostile mob)
    //  - Trading interruption (attacked at vendor)
    // It provides positioning coordination and lets ClassAI handle spell rotation
    auto soloCombat = std::make_unique<SoloCombatStrategy>();
    AddStrategy(std::move(soloCombat));

    // Create quest strategy for quest objective navigation and completion
    // This strategy drives bots to quest locations, kills mobs, collects items, and turns in quests
    auto questStrategy = std::make_unique<QuestStrategy>();
    AddStrategy(std::move(questStrategy));

    // Create loot strategy for corpse looting and item pickup
    // This strategy drives bots to loot kills and collect valuable items
    auto lootStrategy = std::make_unique<LootStrategy>();
    AddStrategy(std::move(lootStrategy));

    // Create rest strategy for eating, drinking, and healing
    // This strategy drives bots to rest when resources are low
    auto restStrategy = std::make_unique<RestStrategy>();
    AddStrategy(std::move(restStrategy));

    // Create solo strategy for solo bot behavior coordination
    // This strategy coordinates all solo behaviors (questing, gathering, autonomous combat, etc.)
    auto soloStrategy = std::make_unique<SoloStrategy>();
    AddStrategy(std::move(soloStrategy));

    // Create grind strategy as fallback when quests are unavailable
    // This strategy activates when:
    // - No active quests available
    // - No quest givers found within 300 yards
    // - No suitable quest hubs for level range
    // - Quest search fails 3+ times consecutively
    // Priority 40 (below Quest=50, above Solo=10) ensures it only activates as fallback
    auto grindStrategy = std::make_unique<GrindStrategy>();
    AddStrategy(std::move(grindStrategy));

    // NOTE: Mutual exclusion rules are automatically configured in BehaviorPriorityManager constructor
    // No need to add them here - they're already set up when _priorityManager is initialized
    TC_LOG_INFO("module.playerbot.ai", "✅ Initialized follow, group_combat, solo_combat, quest, loot, rest, solo, and grind strategies for bot {}", _bot->GetName());

    // NOTE: Do NOT activate strategies here!
    // Strategy activation happens AFTER bot is fully loaded:
    // - For bots in groups: OnGroupJoined() activates follow/combat (called from BotSession after login)
    // - For solo bots: First UpdateAI() will activate solo strategies (see UpdateAI after basic checks)

    // Combat strategies are added by ClassAI
    // Additional strategies can be added based on configuration
}

void BotAI::UpdateValues(uint32 diff)
{
    // Update cached values used by triggers and actions
    // This includes distances, health percentages, resource levels, etc.
}

// ============================================================================
// UNIFIED MOVEMENT COORDINATOR DELEGATION - Phase 6 Facade Pattern
// ============================================================================
// These methods delegate to UnifiedMovementCoordinator via the facade.
// Previously accessed _unifiedMovementCoordinator directly, now uses facade.

bool BotAI::RequestMovement(MovementRequest const& request)
{
    auto movementCoordinator = GetUnifiedMovementCoordinator();
    if (!movementCoordinator)
        return false;

    return movementCoordinator->RequestMovement(request);
}

bool BotAI::RequestPointMovement(
    PlayerBotMovementPriority priority,
    Position const& position,
    std::string const& reason,
    std::string const& sourceSystem)
{
    auto movementCoordinator = GetUnifiedMovementCoordinator();
    if (!movementCoordinator)
        return false;

    MovementRequest req = MovementRequest::MakePointMovement(
        priority,
        position,
        true,  // generatePath
        {},    // finalOrient
        {},    // speed
        {},    // closeEnoughDistance
        reason,
        sourceSystem);

    return movementCoordinator->RequestMovement(req);
}

bool BotAI::RequestChaseMovement(
    PlayerBotMovementPriority priority,
    ObjectGuid targetGuid,
    std::string const& reason,
    std::string const& sourceSystem)
{
    auto movementCoordinator = GetUnifiedMovementCoordinator();
    if (!movementCoordinator)
        return false;

    MovementRequest req = MovementRequest::MakeChaseMovement(
        priority,
        targetGuid,
        {},  // range
        {},  // angle
        reason,
        sourceSystem);

    return movementCoordinator->RequestMovement(req);
}

bool BotAI::RequestFollowMovement(
    PlayerBotMovementPriority priority,
    ObjectGuid targetGuid,
    float distance,
    std::string const& reason,
    std::string const& sourceSystem)
{
    auto* movementCoord = GetUnifiedMovementCoordinator();
    if (!movementCoord)
        return false;

    MovementRequest req = MovementRequest::MakeFollowMovement(
        priority,
        targetGuid,
        distance,
        {},  // angle
        {},  // duration
        reason,
        sourceSystem);

    return movementCoord->RequestMovement(req);
}

void BotAI::StopAllMovement()
{
    if (auto* movementCoord = GetUnifiedMovementCoordinator())
        movementCoord->StopMovement();
}

// NOTE: BotAIFactory implementation is in BotAIFactory.cpp
// Do not duplicate method definitions here to avoid linker errors

// ============================================================================
// LIFECYCLE MANAGEMENT IMPLEMENTATION - Two-Phase AddToWorld Pattern
// ============================================================================

bool BotAI::IsLifecycleSafe() const
{
    if (!_lifecycleManager)
        return true; // Legacy bots without lifecycle management are assumed safe
    return Playerbot::IsPlayerDataSafe(_lifecycleManager->GetState());
}

bool BotAI::IsLifecycleOperational() const
{
    if (!_lifecycleManager)
        return true; // Legacy bots without lifecycle management are assumed operational
    return Playerbot::IsFullyOperational(_lifecycleManager->GetState());
}

BotInitState BotAI::GetLifecycleState() const
{
    if (!_lifecycleManager)
        return BotInitState::ACTIVE; // Legacy bots treated as always active
    return _lifecycleManager->GetState();
}

// ============================================================================
// ST-1: ADAPTIVE AI UPDATE THROTTLING ACCESSORS
// ============================================================================

bool BotAI::IsAIUpdateThrottled() const
{
    if (!_aiUpdateThrottler)
        return false;

    // Not throttled if in full rate tier
    return _aiUpdateThrottler->GetCurrentTier() != ThrottleTier::FULL_RATE;
}

uint32 BotAI::GetAIThrottleTier() const
{
    if (!_aiUpdateThrottler)
        return 0; // FULL_RATE

    return static_cast<uint32>(_aiUpdateThrottler->GetCurrentTier());
}

// ============================================================================
// MOVEMENT SYSTEM INTEGRATION - NEW: Validated movement helper methods
// ============================================================================

bool BotAI::MoveTo(Position const& dest, bool validated)
{
    if (!_movementController)
    {
        TC_LOG_WARN("module.playerbot.movement", "BotAI::MoveTo: Movement controller not initialized for bot {}",
            _bot ? _bot->GetName() : "UNKNOWN");
        return false;
    }

    if (validated)
    {
        // Use validated pathfinding with ground/collision/liquid checks
        return _movementController->MoveToPosition(dest, false);
    }
    else
    {
        // Fallback to unvalidated movement (legacy behavior)
        if (_bot && _bot->IsInWorld())
        {
            _bot->GetMotionMaster()->MovePoint(0, dest);
            return true;
        }
        return false;
    }
}

bool BotAI::MoveToUnit(::Unit* target, float distance)
{
    if (!_movementController)
    {
        TC_LOG_WARN("module.playerbot.movement", "BotAI::MoveToUnit: Movement controller not initialized for bot {}",
            _bot ? _bot->GetName() : "UNKNOWN");
        return false;
    }

    if (!target || !target->IsInWorld())
    {
        TC_LOG_DEBUG("module.playerbot.movement", "BotAI::MoveToUnit: Invalid target for bot {}",
            _bot ? _bot->GetName() : "UNKNOWN");
        return false;
    }

    // Use validated follow movement
    return _movementController->MoveFollow(target, distance, 0.0f);
}

bool BotAI::IsMovementBlocked() const
{
    // Movement is blocked if controller indicates stuck or invalid state
    return _movementController && _movementController->IsStuck();
}

bool BotAI::IsStuck() const
{
    return _movementController && _movementController->IsStuck();
}

} // namespace Playerbot